// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/alignment.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/zstd_compression.h"
#include "core/arm/arm_interface.h"
#include "core/arm/exclusive_monitor.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_page_table_base.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/memory.h"
#include "core/save_state.h"
#include "video_core/gpu.h"

namespace Core {

namespace {

constexpr u32 V2_CHUNK_SIZE = 4096; // 4 KB guest page granularity

struct WritableRegion {
    u64 address{};
    u64 size{};
};

struct PackedRegionHeader {
    u64 address{};
    u64 size{};
    u32 num_chunks{};
    u32 stored_chunks{};
};
static_assert(sizeof(PackedRegionHeader) == 24, "PackedRegionHeader must be 24 bytes");

struct CachedSaveState {
    SaveStateHeader header{};
    std::vector<ThreadSnapshot> threads;
    std::vector<u8> uncompressed_payload;
};

std::mutex g_cache_mutex;
std::unordered_map<std::string, std::shared_ptr<CachedSaveState>> g_ram_cache;
std::vector<std::string> g_ram_cache_lru;
constexpr std::size_t MAX_RAM_CACHED_SLOTS = 2;

std::mutex g_flush_mutex;
std::vector<std::jthread> g_flush_threads;

void PutInRamCache(const std::string& key, std::shared_ptr<CachedSaveState> state) {
    std::scoped_lock lk{g_cache_mutex};
    g_ram_cache[key] = std::move(state);
    std::erase(g_ram_cache_lru, key);
    g_ram_cache_lru.push_back(key);
    while (g_ram_cache_lru.size() > MAX_RAM_CACHED_SLOTS) {
        const std::string oldest = g_ram_cache_lru.front();
        g_ram_cache_lru.erase(g_ram_cache_lru.begin());
        g_ram_cache.erase(oldest);
    }
}

std::shared_ptr<CachedSaveState> GetFromRamCache(const std::string& key) {
    std::scoped_lock lk{g_cache_mutex};
    auto it = g_ram_cache.find(key);
    if (it == g_ram_cache.end()) {
        return nullptr;
    }
    std::erase(g_ram_cache_lru, key);
    g_ram_cache_lru.push_back(key);
    return it->second;
}

std::vector<WritableRegion> EnumerateWritableRegions(Kernel::KProcess& process, bool allow_device_shared = false) {
    std::vector<WritableRegion> regions;
    auto& page_table = process.GetPageTable();
    const auto start = page_table.GetAddressSpaceStart();
    const auto end = start + page_table.GetAddressSpaceSize();
    auto address = start;

    while (address < end) {
        Kernel::KMemoryInfo info{};
        Kernel::Svc::PageInfo page_info{};
        if (page_table.QueryInfo(&info, &page_info, address).IsError()) {
            break;
        }

        const auto svc_info = info.GetSvcMemoryInfo();
        const bool is_writable =
            (svc_info.permission & Kernel::Svc::MemoryPermission::Write) !=
            Kernel::Svc::MemoryPermission::None;

        // Only scan pure CPU game data regions (.data, .bss, Static, CPU Heap).
        // Never overwrite Thread Stacks, ThreadLocal (TLS/IPC buffers), SharedMemory,
        // TransferMemory (AudioRenderer), or IPC buffers.
        const bool is_game_data_state =
            svc_info.state == Kernel::Svc::MemoryState::CodeData ||
            svc_info.state == Kernel::Svc::MemoryState::AliasCodeData ||
            svc_info.state == Kernel::Svc::MemoryState::Static ||
            svc_info.state == Kernel::Svc::MemoryState::Normal ||
            svc_info.state == Kernel::Svc::MemoryState::Alias;

        // Exclude kernel-locked, IPC-locked, and GPU-uncached (NVN GPFIFO / command buffer /
        // syncpoint fence / render target) blocks so we never corrupt active HLE/Vulkan state.
        const auto forbidden_attrs =
            Kernel::Svc::MemoryAttribute::Locked |
            Kernel::Svc::MemoryAttribute::IpcLocked |
            Kernel::Svc::MemoryAttribute::Uncached |
            (allow_device_shared ? Kernel::Svc::MemoryAttribute{}
                                 : Kernel::Svc::MemoryAttribute::DeviceShared);

        const bool is_safe_attr =
            static_cast<u32>(svc_info.attribute & forbidden_attrs) == 0 &&
            svc_info.ipc_count == 0 &&
            (allow_device_shared || svc_info.device_count == 0 ||
             svc_info.state == Kernel::Svc::MemoryState::CodeData ||
             svc_info.state == Kernel::Svc::MemoryState::AliasCodeData);

        if (is_writable && is_game_data_state && is_safe_attr && info.GetSize() > 0) {
            const u64 reg_addr = static_cast<u64>(info.GetAddress());
            const u64 reg_size = info.GetSize();
            regions.push_back(WritableRegion{reg_addr, reg_size});
        }

        const Common::ProcessAddress next = info.GetAddress() + info.GetSize();
        if (next <= address) {
            break;
        }
        address = next;
    }

    // Fallback if a game mapped its entire Normal heap with DeviceShared
    if (!allow_device_shared) {
        bool has_heap = false;
        for (const auto& reg : regions) {
            if (reg.size >= 256 * 1024) {
                has_heap = true;
                break;
            }
        }
        if (!has_heap) {
            return EnumerateWritableRegions(process, true);
        }
    }

    return regions;
}

void WaitForCleanFrameBoundary(Kernel::KProcess& process) {
    // Wait up to 8 ms for active userland game threads to reach their natural end-of-frame
    // kernel wait (e.g. WaitSynchronization on VSync / NVN fence) before stalling cores.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        bool any_user_thread_running_code = false;
        {
            auto& thread_list = process.GetThreadList();
            for (const auto& thread : thread_list) {
                if (!thread.IsUserThread() || thread.IsDummyThread()) {
                    continue;
                }
                if (thread.GetState() == Kernel::ThreadState::Runnable) {
                    any_user_thread_running_code = true;
                    break;
                }
            }
        }
        if (!any_user_thread_running_code) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

struct ProtectedAddressRange {
    u64 start{};
    u64 end{};
};

std::vector<ProtectedAddressRange> CollectProtectedKernelSyncRanges(Kernel::KProcess& process) {
    std::vector<ProtectedAddressRange> ranges;
    auto& thread_list = process.GetThreadList();
    for (const auto& thread : thread_list) {
        // 1. Protect nn::os::ThreadType userland structure pointed to by TPIDR_EL0
        const u64 tpidr = thread.GetTpidrEl0();
        if (tpidr >= 0x1000) {
            ranges.push_back({tpidr >= 64 ? tpidr - 64 : tpidr, tpidr + 512});
        }
        // 2. Protect TLS / IPC command buffer region
        const u64 tls = GetInteger(thread.GetTlsAddress());
        if (tls >= 0x1000) {
            ranges.push_back({tls, tls + 0x200});
        }
        // 3. Protect active userland stack area (in case aliased in Normal/Alias heap)
        const u64 stack_top = GetInteger(thread.GetUserStackTop());
        if (stack_top >= 0x40000) {
            ranges.push_back({stack_top - 0x40000, stack_top});
        }
        // 4. Protect active kernel mutex / condvar / address arbiter words in guest RAM
        const u64 addr_key = GetInteger(thread.GetAddressKey());
        if (addr_key >= 0x1000) {
            const u64 aligned = addr_key & ~u64(63);
            ranges.push_back({aligned, aligned + 64});
        }
        const u64 cv_key = thread.GetConditionVariableKey();
        if (cv_key >= 0x1000) {
            const u64 aligned = cv_key & ~u64(63);
            ranges.push_back({aligned, aligned + 64});
        }
    }
    return ranges;
}

bool IsChunkZero4KB(const u8* ptr, std::size_t len) {
    const std::size_t words = len / sizeof(u64);
    const auto* wptr = reinterpret_cast<const u64*>(ptr);
    for (std::size_t i = 0; i < words; ++i) {
        if (wptr[i] != 0) {
            return false;
        }
    }
    for (std::size_t i = words * sizeof(u64); i < len; ++i) {
        if (ptr[i] != 0) {
            return false;
        }
    }
    return true;
}

SaveStateResult WriteSnapshotToDisk(const std::shared_ptr<CachedSaveState>& snapshot,
                                    const std::filesystem::path& path) {
    auto compressed = Common::Compression::CompressDataZSTD(
        snapshot->uncompressed_payload.data(), snapshot->uncompressed_payload.size(), 1);
    if (compressed.empty() && !snapshot->uncompressed_payload.empty()) {
        LOG_ERROR(Core, "SaveState: ZSTD compression failed for {}", path.string());
        return SaveStateResult::ErrorCompress;
    }

    SaveStateHeader header = snapshot->header;
    header.dram_compressed_size = compressed.size();

    auto tmp_path = path;
    tmp_path += ".tmp";

    std::ofstream file(tmp_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOG_ERROR(Core, "SaveState: Failed to open {} for writing", tmp_path.string());
        return SaveStateResult::ErrorFileCreate;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    const u32 thread_data_size =
        static_cast<u32>(snapshot->threads.size() * sizeof(ThreadSnapshot));
    file.write(reinterpret_cast<const char*>(&thread_data_size), sizeof(thread_data_size));
    if (!snapshot->threads.empty()) {
        file.write(reinterpret_cast<const char*>(snapshot->threads.data()), thread_data_size);
    }

    if (!compressed.empty()) {
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    }
    file.flush();
    file.close();

    std::error_code ec;
    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(tmp_path, path, ec);
        if (ec) {
            LOG_ERROR(Core, "SaveState: Failed to rename {} -> {}: {}", tmp_path.string(),
                      path.string(), ec.message());
            return SaveStateResult::ErrorFileCreate;
        }
    }

    LOG_INFO(Core,
             "SaveState: Async disk flush complete for '{}' ({:.2f} MB compressed from {:.2f} MB "
             "sparse payload)",
             path.string(), static_cast<double>(compressed.size()) / (1024.0 * 1024.0),
             static_cast<double>(snapshot->uncompressed_payload.size()) / (1024.0 * 1024.0));
    return SaveStateResult::Success;
}

} // namespace

void FlushPendingSaveStates() {
    std::vector<std::jthread> threads_to_join;
    {
        std::scoped_lock lk{g_flush_mutex};
        threads_to_join.swap(g_flush_threads);
    }
    for (auto& t : threads_to_join) {
        if (t.joinable()) {
            t.join();
        }
    }
}

SaveStateResult CreateSaveState(System& system, const std::filesystem::path& path) {
    if (!system.IsPoweredOn()) {
        return SaveStateResult::ErrorSystemNotRunning;
    }

    auto* process = system.ApplicationProcess();
    if (!process) {
        return SaveStateResult::ErrorSystemNotRunning;
    }

    const auto t_start = std::chrono::steady_clock::now();

    // Ensure any previous background disk write is completed before starting a new snapshot
    FlushPendingSaveStates();

    // Align to a clean frame boundary wait before stalling CPU cores
    WaitForCleanFrameBoundary(*process);

    // Pause emulation briefly while capturing RAM snapshot (must hold returned lock!)
    auto stall_lock = system.StallApplication();

    auto snapshot = std::make_shared<CachedSaveState>();
    const u64 dram_size = Kernel::Board::Nintendo::Nx::KSystemControl::Init::GetIntendedMemorySize();

    // 1. Enumerate only writable CPU game-data regions (skips read-only code & GPU/HLE pools)
    const auto regions = EnumerateWritableRegions(*process);
    auto& memory = system.ApplicationMemory();

    u64 total_writable_bytes = 0;
    for (const auto& reg : regions) {
        total_writable_bytes += reg.size;
    }

    auto& payload = snapshot->uncompressed_payload;
    payload.reserve(std::min<u64>(total_writable_bytes / 2 + 65536, 512ULL * 1024 * 1024));

    const u32 region_count = static_cast<u32>(regions.size());
    payload.resize(sizeof(u32));
    std::memcpy(payload.data(), &region_count, sizeof(u32));

    std::vector<u8> region_buf;
    u32 total_chunks = 0;
    u32 total_stored_chunks = 0;

    for (const auto& reg : regions) {
        region_buf.resize(reg.size);
        if (!memory.ReadBlockUnsafe(reg.address, region_buf.data(), reg.size)) {
            continue;
        }

        const u32 num_chunks = static_cast<u32>((reg.size + V2_CHUNK_SIZE - 1) / V2_CHUNK_SIZE);
        const u32 bitmask_bytes = (num_chunks + 7) / 8;

        const std::size_t header_offset = payload.size();
        payload.resize(header_offset + sizeof(PackedRegionHeader) + bitmask_bytes, 0);

        u32 stored_chunks = 0;
        for (u32 c = 0; c < num_chunks; ++c) {
            const u64 chunk_off = static_cast<u64>(c) * V2_CHUNK_SIZE;
            const std::size_t chunk_len =
                static_cast<std::size_t>(std::min<u64>(V2_CHUNK_SIZE, reg.size - chunk_off));
            const u8* chunk_ptr = region_buf.data() + chunk_off;

            if (!IsChunkZero4KB(chunk_ptr, chunk_len)) {
                payload[header_offset + sizeof(PackedRegionHeader) + (c / 8)] |=
                    static_cast<u8>(1u << (c % 8));
                payload.insert(payload.end(), chunk_ptr, chunk_ptr + chunk_len);
                stored_chunks++;
            }
        }

        PackedRegionHeader reg_hdr{};
        reg_hdr.address = reg.address;
        reg_hdr.size = reg.size;
        reg_hdr.num_chunks = num_chunks;
        reg_hdr.stored_chunks = stored_chunks;
        std::memcpy(payload.data() + header_offset, &reg_hdr, sizeof(PackedRegionHeader));

        total_chunks += num_chunks;
        total_stored_chunks += stored_chunks;
    }

    // 2. Collect thread metadata (Unload already saved true registers into thread.GetContext())
    auto& thread_list = process->GetThreadList();
    for (auto& thread : thread_list) {
        ThreadSnapshot snap{};
        snap.thread_id = static_cast<u32>(thread.GetThreadId());
        snap.core_id = static_cast<u32>(thread.GetActiveCore());
        snap.tls_address = GetInteger(thread.GetTlsAddress());

        const auto& ctx = thread.GetContext();
        std::memcpy(snap.context_data, &ctx, std::min(sizeof(ctx), sizeof(snap.context_data)));
        snapshot->threads.push_back(snap);
    }

    // 3. Populate header
    snapshot->header.magic = SAVE_STATE_MAGIC;
    snapshot->header.version = SAVE_STATE_VERSION;
    snapshot->header.title_id = system.GetApplicationProcessProgramID();
    snapshot->header.dram_size = dram_size;
    snapshot->header.dram_compressed_size = payload.size();
    snapshot->header.num_threads = static_cast<u32>(snapshot->threads.size());
    snapshot->header.compression_level = 1;
    snapshot->header.flags = SaveStateFlag_WritableRegions;
    snapshot->header.sparse_page_size = V2_CHUNK_SIZE;
    snapshot->header.num_sparse_pages = region_count;
    snapshot->header.num_stored_pages = total_stored_chunks;
    snapshot->header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();

    // Resume emulation immediately! ZSTD compression and disk I/O happen asynchronously.
    system.UnstallApplication();
    stall_lock.unlock();

    const auto stall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - t_start)
                              .count();

    // Store in instant RAM cache
    PutInRamCache(path.string(), snapshot);

    LOG_INFO(Core,
             "SaveState: Instant RAM snapshot captured in {} ms ({} writable regions, {} / {} "
             "non-zero 4KB pages = {:.2f} MB vs {:.0f} MB DRAM). Flushing to disk asynchronously...",
             stall_ms, region_count, total_stored_chunks, total_chunks,
             static_cast<double>(payload.size()) / (1024.0 * 1024.0),
             static_cast<double>(dram_size) / (1024.0 * 1024.0));

    // Launch background ZSTD compression + atomic disk write
    {
        std::scoped_lock lk{g_flush_mutex};
        g_flush_threads.emplace_back([snapshot, path]() {
            WriteSnapshotToDisk(snapshot, path);
        });
    }

    return SaveStateResult::Success;
}

SaveStateResult LoadSaveState(System& system, const std::filesystem::path& path) {
    if (!system.IsPoweredOn()) {
        return SaveStateResult::ErrorSystemNotRunning;
    }

    const auto t_start = std::chrono::steady_clock::now();

    // 1. Check instant RAM cache first (0 ms disk/decompression overhead!)
    std::shared_ptr<CachedSaveState> cached = GetFromRamCache(path.string());

    if (!cached) {
        // Ensure any in-flight background write to this file has finished
        FlushPendingSaveStates();

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return SaveStateResult::ErrorFileRead;
        }

        auto disk_state = std::make_shared<CachedSaveState>();
        file.read(reinterpret_cast<char*>(&disk_state->header), sizeof(disk_state->header));
        if (disk_state->header.magic != SAVE_STATE_MAGIC) {
            return SaveStateResult::ErrorInvalidMagic;
        }
        if (disk_state->header.version != 1 && disk_state->header.version != SAVE_STATE_VERSION) {
            return SaveStateResult::ErrorVersionMismatch;
        }

        u32 thread_data_size = 0;
        file.read(reinterpret_cast<char*>(&thread_data_size), sizeof(thread_data_size));
        if (thread_data_size > 0) {
            const u32 num_threads = thread_data_size / sizeof(ThreadSnapshot);
            disk_state->threads.resize(num_threads);
            file.read(reinterpret_cast<char*>(disk_state->threads.data()), thread_data_size);
        }

        std::vector<u8> compressed(disk_state->header.dram_compressed_size);
        file.read(reinterpret_cast<char*>(compressed.data()), disk_state->header.dram_compressed_size);
        file.close();

        disk_state->uncompressed_payload = Common::Compression::DecompressDataZSTD(compressed);
        if (disk_state->uncompressed_payload.empty() && !compressed.empty()) {
            return SaveStateResult::ErrorDecompress;
        }

        cached = std::move(disk_state);
        PutInRamCache(path.string(), cached);
    }

    const auto& header = cached->header;
    const auto& decompressed = cached->uncompressed_payload;

    auto& kernel = system.Kernel();
    auto* process = kernel.ApplicationProcess();
    if (!process) {
        return SaveStateResult::ErrorSystemNotRunning;
    }

    // Align to a clean frame boundary wait before stalling CPU cores
    WaitForCleanFrameBoundary(*process);

    // Pause emulation for differential memory restoration (must hold returned lock!)
    auto stall_lock = system.StallApplication();

    process = kernel.ApplicationProcess();
    if (!process) {
        system.UnstallApplication();
        return SaveStateResult::ErrorSystemNotRunning;
    }

    const auto protected_ranges = CollectProtectedKernelSyncRanges(*process);

    if (header.flags & SaveStateFlag_WritableRegions) {
        // v2: Page-Table Writable Regions + Differential 4KB Page Restoration (WriteChanged)
        if (decompressed.size() < sizeof(u32)) {
            system.UnstallApplication();
            return SaveStateResult::ErrorCorruptedFile;
        }

        u32 region_count = 0;
        std::memcpy(&region_count, decompressed.data(), sizeof(u32));
        std::size_t cursor = sizeof(u32);

        auto& memory = system.ApplicationMemory();
        std::vector<u8> live_page(V2_CHUNK_SIZE);
        std::vector<u8> patched_page(V2_CHUNK_SIZE);
        std::array<u8, V2_CHUNK_SIZE> zero_page{};
        u64 diff_bytes_written = 0;

        for (u32 r = 0; r < region_count; ++r) {
            if (cursor + sizeof(PackedRegionHeader) > decompressed.size()) {
                LOG_ERROR(Core, "SaveState: Truncated region header at region {}", r);
                break;
            }

            PackedRegionHeader reg_hdr{};
            std::memcpy(&reg_hdr, decompressed.data() + cursor, sizeof(PackedRegionHeader));
            cursor += sizeof(PackedRegionHeader);

            const u32 bitmask_bytes = (reg_hdr.num_chunks + 7) / 8;
            if (cursor + bitmask_bytes > decompressed.size()) {
                LOG_ERROR(Core, "SaveState: Truncated region bitmask at region {}", r);
                break;
            }
            const u8* bitmask = decompressed.data() + cursor;
            cursor += bitmask_bytes;

            // Verify whether this region is still a safe CPU game-data region in the live page table
            Kernel::KMemoryInfo live_info{};
            Kernel::Svc::PageInfo live_page_info{};
            bool region_safe = true;
            if (process->GetPageTable().QueryInfo(&live_info, &live_page_info, reg_hdr.address).IsSuccess()) {
                const auto svc_info = live_info.GetSvcMemoryInfo();
                if (svc_info.state == Kernel::Svc::MemoryState::Stack ||
                    svc_info.state == Kernel::Svc::MemoryState::ThreadLocal ||
                    svc_info.state == Kernel::Svc::MemoryState::Ipc ||
                    svc_info.state == Kernel::Svc::MemoryState::NonSecureIpc ||
                    svc_info.state == Kernel::Svc::MemoryState::NonDeviceIpc ||
                    svc_info.state == Kernel::Svc::MemoryState::Shared ||
                    svc_info.state == Kernel::Svc::MemoryState::Transferred ||
                    svc_info.state == Kernel::Svc::MemoryState::SharedTransferred ||
                    static_cast<u32>(svc_info.attribute &
                                     (Kernel::Svc::MemoryAttribute::Locked |
                                      Kernel::Svc::MemoryAttribute::IpcLocked |
                                      Kernel::Svc::MemoryAttribute::Uncached)) != 0) {
                    region_safe = false;
                }
            }

            for (u32 c = 0; c < reg_hdr.num_chunks; ++c) {
                const u64 page_addr = reg_hdr.address + static_cast<u64>(c) * V2_CHUNK_SIZE;
                const std::size_t chunk_len = static_cast<std::size_t>(
                    std::min<u64>(V2_CHUNK_SIZE, reg_hdr.size - static_cast<u64>(c) * V2_CHUNK_SIZE));
                const bool is_stored = (bitmask[c / 8] & (1u << (c % 8))) != 0;

                const u8* target_data = zero_page.data();
                if (is_stored) {
                    if (cursor + chunk_len > decompressed.size()) {
                        LOG_ERROR(Core, "SaveState: Truncated chunk data in region {}", r);
                        break;
                    }
                    target_data = decompressed.data() + cursor;
                    cursor += chunk_len;
                }

                if (!region_safe) {
                    continue;
                }

                // Differential WriteChanged check: only write if the 4KB page differs from live RAM
                if (memory.ReadBlockUnsafe(page_addr, live_page.data(), chunk_len)) {
                    if (std::memcmp(live_page.data(), target_data, chunk_len) != 0) {
                        const u64 page_end = page_addr + chunk_len;
                        const u8* data_to_write = target_data;

                        // Preserve any active kernel/OS synchronization structures on this page
                        bool has_overlap = false;
                        for (const auto& pr : protected_ranges) {
                            if (pr.start < page_end && pr.end > page_addr) {
                                if (!has_overlap) {
                                    std::memcpy(patched_page.data(), target_data, chunk_len);
                                    has_overlap = true;
                                }
                                const u64 ov_start = std::max(page_addr, pr.start) - page_addr;
                                const u64 ov_end = std::min(page_end, pr.end) - page_addr;
                                std::memcpy(patched_page.data() + ov_start,
                                            live_page.data() + ov_start,
                                            static_cast<std::size_t>(ov_end - ov_start));
                            }
                        }
                        if (has_overlap) {
                            data_to_write = patched_page.data();
                            if (std::memcmp(live_page.data(), data_to_write, chunk_len) == 0) {
                                continue;
                            }
                        }

                        memory.WriteBlockUnsafe(page_addr, data_to_write, chunk_len);
                        diff_bytes_written += chunk_len;
                    }
                }
            }
        }

        LOG_INFO(Core, "SaveState: Differential restore wrote {:.2f} MB of changed 4KB pages",
                 static_cast<double>(diff_bytes_written) / (1024.0 * 1024.0));
    } else if (header.flags & SaveStateFlag_SparsePages) {
        // Legacy v1 Sparse Page Encoded (64KB full DRAM)
        auto& device_memory = system.DeviceMemory();
        u8* dram_ptr = device_memory.buffer.BackingBasePointer();
        const u32 page_size = header.sparse_page_size ? header.sparse_page_size : (64 * 1024);
        const u32 total_pages =
            header.num_sparse_pages ? header.num_sparse_pages : static_cast<u32>(header.dram_size / page_size);
        const u32 bitmask_bytes = (total_pages + 7) / 8;
        if (decompressed.size() < bitmask_bytes) {
            system.UnstallApplication();
            return SaveStateResult::ErrorCorruptedFile;
        }

        std::memset(dram_ptr, 0, header.dram_size);
        const u8* bitmask = decompressed.data();
        const u8* page_data = decompressed.data() + bitmask_bytes;
        const u8* data_end = decompressed.data() + decompressed.size();

        for (u32 p = 0; p < total_pages; ++p) {
            if ((bitmask[p / 8] & (1u << (p % 8))) != 0) {
                if (page_data + page_size > data_end) {
                    break;
                }
                std::memcpy(dram_ptr + (static_cast<u64>(p) * page_size), page_data, page_size);
                page_data += page_size;
            }
        }
        system.GPU().InvalidateGPUCache();
    } else {
        // Legacy v1 Uncompressed
        auto& device_memory = system.DeviceMemory();
        u8* dram_ptr = device_memory.buffer.BackingBasePointer();
        if (decompressed.size() != header.dram_size) {
            system.UnstallApplication();
            return SaveStateResult::ErrorDecompress;
        }
        std::memcpy(dram_ptr, decompressed.data(), header.dram_size);
        system.GPU().InvalidateGPUCache();
    }

    // Clear exclusive monitor state across all physical cores before resuming
    auto& monitor = process->GetExclusiveMonitor();
    for (std::size_t c = 0; c < Core::Hardware::NUM_CPU_CORES; ++c) {
        monitor.ClearExclusive(c);
    }

    // Never call kernel.Scheduler(c).RequestScheduleOnInterrupt(kernel) from the GUI thread!
    // UnstallApplication() calls kernel.SuspendEmulation(false), which safely resumes and
    // reschedules all cores from their own threads.
    system.UnstallApplication();
    stall_lock.unlock();

    const auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - t_start)
                             .count();
    LOG_INFO(Core, "SaveState: Instant Quick Load completed in {} ms", load_ms);
    return SaveStateResult::Success;
}

std::filesystem::path GetSaveStatePath(u64 title_id, u32 slot) {
    auto save_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::SaveDir) / "savestates";
    std::filesystem::create_directories(save_dir);
    return save_dir / fmt::format("{:016X}_slot{}.ses", title_id, slot);
}

bool GetSaveStateInfo(const std::filesystem::path& path, SaveStateHeader& header_out) {
    if (auto cached = GetFromRamCache(path.string())) {
        header_out = cached->header;
        return true;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.read(reinterpret_cast<char*>(&header_out), sizeof(header_out));
    return header_out.magic == SAVE_STATE_MAGIC;
}

} // namespace Core
