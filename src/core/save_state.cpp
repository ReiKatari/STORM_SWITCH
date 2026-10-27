// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
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
#include "common/thread.h"
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
#include "audio_core/audio_core.h"
#include "core/hle/service/audio/audio_in_manager.h"
#include "core/hle/service/audio/audio_out_manager.h"
#include "core/hle/service/audio/audio_renderer_manager.h"
#include "core/hle/service/sm/sm.h"
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

struct ProtectedAddressRange {
    u64 start{};
    u64 end{};
};

struct NsoModuleSpan {
    u64 code_start{};
    u64 code_end{};
    u64 data_start{};
    u64 data_end{};
};

struct ProcessLayoutAnalysis {
    std::vector<WritableRegion> writable_regions;
    u64 rtld_start{};
    u64 rtld_end{};
    u64 main_code_start{};
    u64 main_code_end{};
    u64 sdk_start{};
    u64 sdk_end{};
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

ProcessLayoutAnalysis AnalyzeProcessLayout(Kernel::KProcess& process, bool allow_device_shared = false) {
    ProcessLayoutAnalysis out{};
    auto& page_table = process.GetPageTable();
    const auto start = page_table.GetAddressSpaceStart();
    const auto end = start + page_table.GetAddressSpaceSize();

    // Pass 1: Identify loaded NSO modules (rtld, main, subsdk*, sdk) in the Code/CodeData range
    std::vector<NsoModuleSpan> modules;
    {
        auto address = start;
        NsoModuleSpan cur_mod{};
        bool in_mod = false;
        bool seen_data = false;

        while (address < end) {
            Kernel::KMemoryInfo info{};
            Kernel::Svc::PageInfo page_info{};
            if (page_table.QueryInfo(&info, &page_info, address).IsError()) {
                break;
            }
            const auto svc = info.GetSvcMemoryInfo();
            const u64 blk_addr = static_cast<u64>(info.GetAddress());
            const u64 blk_end = blk_addr + info.GetSize();

            const bool is_code = svc.state == Kernel::Svc::MemoryState::Code ||
                                 svc.state == Kernel::Svc::MemoryState::AliasCode;
            const bool is_code_data = svc.state == Kernel::Svc::MemoryState::CodeData ||
                                      svc.state == Kernel::Svc::MemoryState::AliasCodeData;
            const bool is_rx = (svc.permission & Kernel::Svc::MemoryPermission::Execute) !=
                               Kernel::Svc::MemoryPermission::None;

            if (is_code && is_rx) {
                if (!in_mod || seen_data) {
                    if (in_mod) {
                        modules.push_back(cur_mod);
                    }
                    cur_mod = NsoModuleSpan{blk_addr, blk_end, blk_end, blk_end};
                    in_mod = true;
                    seen_data = false;
                } else {
                    cur_mod.code_end = blk_end;
                    cur_mod.data_start = blk_end;
                    cur_mod.data_end = blk_end;
                }
            } else if (in_mod && is_code) {
                if (!seen_data) {
                    cur_mod.code_end = blk_end;
                    cur_mod.data_start = blk_end;
                    cur_mod.data_end = blk_end;
                }
            } else if (in_mod && is_code_data) {
                if (!seen_data) {
                    cur_mod.data_start = blk_addr;
                    seen_data = true;
                }
                cur_mod.data_end = blk_end;
            }

            const Common::ProcessAddress next = info.GetAddress() + info.GetSize();
            if (next <= address) {
                break;
            }
            address = next;
        }
        if (in_mod) {
            modules.push_back(cur_mod);
        }
    }

    if (modules.size() >= 2) {
        // First small module (<= 256 KB code) is rtld
        if (modules.front().code_end - modules.front().code_start <= 0x40000) {
            out.rtld_start = modules.front().code_start;
            out.rtld_end = modules.front().data_end;
        }
        // Main executable is the first module after rtld
        for (std::size_t mi = 0; mi < modules.size(); ++mi) {
            if (mi == 0 && out.rtld_start != 0) {
                continue;
            }
            if (modules[mi].code_end > modules[mi].code_start) {
                out.main_code_start = modules[mi].code_start;
                out.main_code_end = modules[mi].code_end;
                break;
            }
        }
        // Last module in the NSO chain is nnSdk (sdk)
        out.sdk_start = modules.back().code_start;
        out.sdk_end = modules.back().data_end;
    }

    // Pass 2: Collect candidate writable game-data regions and find max Normal heap block size
    struct CandidateRegion {
        u64 address{};
        u64 size{};
        Kernel::Svc::MemoryState state{};
    };
    std::vector<CandidateRegion> candidates;
    u64 max_normal_heap_size = 0;
    u64 total_code_data_size = 0;
    bool has_uncached_carvings = false;

    auto address = start;
    while (address < end) {
        Kernel::KMemoryInfo info{};
        Kernel::Svc::PageInfo page_info{};
        if (page_table.QueryInfo(&info, &page_info, address).IsError()) {
            break;
        }

        const auto svc_info = info.GetSvcMemoryInfo();
        if (static_cast<u32>(svc_info.attribute & Kernel::Svc::MemoryAttribute::Uncached) != 0) {
            has_uncached_carvings = true;
        }
        const bool is_writable =
            (svc_info.permission & Kernel::Svc::MemoryPermission::Write) !=
            Kernel::Svc::MemoryPermission::None;

        const bool is_game_data_state =
            svc_info.state == Kernel::Svc::MemoryState::CodeData ||
            svc_info.state == Kernel::Svc::MemoryState::AliasCodeData ||
            svc_info.state == Kernel::Svc::MemoryState::Static ||
            svc_info.state == Kernel::Svc::MemoryState::Normal ||
            svc_info.state == Kernel::Svc::MemoryState::Alias;

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
            const u64 reg_end = reg_addr + reg_size;

            // Skip rtld and nnSdk (.data / .bss) so HLE service handles, SDK mutexes, and
            // thread/allocator structures inside sdk remain synchronized with the host kernel
            const bool in_rtld = (out.rtld_end > out.rtld_start) &&
                                 (reg_addr >= out.rtld_start && reg_end <= out.rtld_end);
            const bool in_sdk = (out.sdk_end > out.sdk_start) &&
                                (reg_addr >= out.sdk_start && reg_end <= out.sdk_end);

            if (!in_rtld && !in_sdk) {
                candidates.push_back(CandidateRegion{reg_addr, reg_size, svc_info.state});
                if (svc_info.state == Kernel::Svc::MemoryState::CodeData ||
                    svc_info.state == Kernel::Svc::MemoryState::AliasCodeData) {
                    total_code_data_size += reg_size;
                } else if ((svc_info.state == Kernel::Svc::MemoryState::Normal ||
                            svc_info.state == Kernel::Svc::MemoryState::Alias) &&
                           reg_size > max_normal_heap_size) {
                    max_normal_heap_size = reg_size;
                }
            }
        }

        const Common::ProcessAddress next = info.GetAddress() + info.GetSize();
        if (next <= address) {
            break;
        }
        address = next;
    }

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& c = candidates[i];
        const bool is_code_data = c.state == Kernel::Svc::MemoryState::CodeData ||
                                  c.state == Kernel::Svc::MemoryState::AliasCodeData;
        if (is_code_data) {
            // Skip tiny single-page (4 KB) .got.plt table separated from .data/.bss
            if (c.size == 0x1000 && (i + 1) < candidates.size() &&
                candidates[i + 1].address > c.address + c.size) {
                continue;
            }
            out.writable_regions.push_back(WritableRegion{c.address, c.size});
        } else {
            // When the game has a massive self-contained .bss / CodeData arena (>= 128 MB) and
            // Uncached GPU carvings inside the Normal heap, the Normal heap is used exclusively
            // by nnSdk / NVN graphics driver allocators and must not be rolled back out of sync
            // with the Uncached GPU state.
            if (total_code_data_size >= 128ULL * 1024 * 1024 && has_uncached_carvings) {
                continue;
            }
            // If the game has a large main Normal heap (>= 64 MB), exclude small carved-out
            // SDK/NVN/TLS/stack startup fragments (< 32 MB) that sit between Uncached GPU blocks.
            if (max_normal_heap_size >= 64ULL * 1024 * 1024 && c.size < 32ULL * 1024 * 1024) {
                continue;
            }
            out.writable_regions.push_back(WritableRegion{c.address, c.size});
        }
    }

    if (!allow_device_shared) {
        bool has_heap = false;
        for (const auto& reg : out.writable_regions) {
            if (reg.size >= 256 * 1024) {
                has_heap = true;
                break;
            }
        }
        if (!has_heap) {
            return AnalyzeProcessLayout(process, true);
        }
    }

    return out;
}

void WaitForCleanFrameBoundary(Kernel::KProcess& process) {
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

struct SyncProtectionContext {
    std::vector<ProtectedAddressRange> ranges;
    std::vector<u32> active_thread_handles;
    std::vector<u64> thread_type_ptrs;
    u64 sdk_start{};
    u64 sdk_end{};

    void AddRange(u64 start, u64 end) {
        if (end <= start || start < 0x1000) {
            return;
        }
        ranges.push_back({start, end});
    }

    void MergeRanges() {
        if (ranges.empty()) {
            return;
        }
        std::sort(ranges.begin(), ranges.end(),
                  [](const ProtectedAddressRange& a, const ProtectedAddressRange& b) {
                      return a.start < b.start;
                  });
        std::vector<ProtectedAddressRange> merged;
        merged.reserve(ranges.size());
        merged.push_back(ranges[0]);
        for (std::size_t i = 1; i < ranges.size(); ++i) {
            if (ranges[i].start <= merged.back().end) {
                merged.back().end = std::max(merged.back().end, ranges[i].end);
            } else {
                merged.push_back(ranges[i]);
            }
        }
        ranges.swap(merged);
    }

    bool IsHandleActive(u32 raw_word) const {
        const u32 h = raw_word & 0x3FFFFFFFu;
        if (h < 0x00010000u || h > 0x00FFFFFFu) {
            return false;
        }
        for (const u32 th : active_thread_handles) {
            if (h == th) {
                return true;
            }
        }
        return false;
    }

    bool IsSdkPointer(u64 val) const {
        return sdk_end > sdk_start && val >= sdk_start && val < sdk_end;
    }

    bool IsThreadTypePointer(u64 val) const {
        if (val < 0x1000) {
            return false;
        }
        for (const u64 ttp : thread_type_ptrs) {
            if (val == ttp) {
                return true;
            }
        }
        return false;
    }
};

SyncProtectionContext BuildSyncProtectionContext(Kernel::KProcess& process,
                                                 Core::Memory::Memory& memory,
                                                 const ProcessLayoutAnalysis& layout) {
    SyncProtectionContext ctx{};
    ctx.sdk_start = layout.sdk_start;
    ctx.sdk_end = layout.sdk_end;
    auto& page_table = process.GetPageTable();
    auto& thread_list = process.GetThreadList();
    bool is_first_user_thread = true;

    std::vector<u64> all_thread_type_ptrs;
    for (const auto& thread : thread_list) {
        if (!thread.IsUserThread() || thread.IsDummyThread()) {
            continue;
        }
        const u64 tls = GetInteger(thread.GetTlsAddress());
        if (tls != 0) {
            if (const u8* tls_ptr = memory.GetPointerSilent(Common::ProcessAddress{tls})) {
                u64 ttp = 0;
                std::memcpy(&ttp, tls_ptr + 0x1F8, sizeof(u64));
                if (ttp >= 0x1000) {
                    all_thread_type_ptrs.push_back(ttp);
                }
            }
        }
    }
    ctx.thread_type_ptrs = all_thread_type_ptrs;

    for (const auto& thread : thread_list) {
        if (!thread.IsUserThread() || thread.IsDummyThread()) {
            continue;
        }
        bool is_main_thread = is_first_user_thread;
        is_first_user_thread = false;
        u64 thread_type_ptr = 0;

        // 1. Protect TLS area (0x200 bytes) and nn::os::ThreadType pointed by TLS + 0x1f8
        const u64 tls = GetInteger(thread.GetTlsAddress());
        if (tls != 0) {
            ctx.AddRange(tls, tls + 0x200);
            if (const u8* tls_ptr = memory.GetPointerSilent(Common::ProcessAddress{tls})) {
                std::memcpy(&thread_type_ptr, tls_ptr + 0x1F8, sizeof(u64));
                if (thread_type_ptr >= 0x1000) {
                    if (const u8* tt_ptr =
                            memory.GetPointerSilent(Common::ProcessAddress{thread_type_ptr})) {
                        // Check thread name at +0x180 to identify MainThread reliably
                        const char* tname = reinterpret_cast<const char*>(tt_ptr + 0x180);
                        if (std::strncmp(tname, "MainThread", 10) == 0) {
                            is_main_thread = true;
                        }
                    }

                    ctx.AddRange(thread_type_ptr >= 0x80 ? thread_type_ptr - 0x80 : thread_type_ptr,
                                 thread_type_ptr + 0x240);

                    // Read nn::os::ThreadType fields:
                    // +0x40: thread_arg, +0x48: orig_stack, +0x50: alias_stack, +0x58: stack_sz, +0x1B0: handle
                    if (const u8* tt_ptr =
                            memory.GetPointerSilent(Common::ProcessAddress{thread_type_ptr})) {
                        u64 thread_arg = 0;
                        u64 orig_stack = 0;
                        u64 alias_stack = 0;
                        u64 stack_sz = 0;
                        u32 handle_cand = 0;
                        std::memcpy(&thread_arg, tt_ptr + 0x40, sizeof(u64));
                        std::memcpy(&orig_stack, tt_ptr + 0x48, sizeof(u64));
                        std::memcpy(&alias_stack, tt_ptr + 0x50, sizeof(u64));
                        std::memcpy(&stack_sz, tt_ptr + 0x58, sizeof(u64));
                        std::memcpy(&handle_cand, tt_ptr + 0x1B0, sizeof(u32));

                        if ((handle_cand & 0x3FFFFFFFu) >= 0x00010000u &&
                            (handle_cand & 0x3FFFFFFFu) <= 0x00FFFFFFu) {
                            ctx.active_thread_handles.push_back(handle_cand & 0x3FFFFFFFu);
                        } else {
                            for (std::size_t off = 0x180; off <= 0x1F0; off += 4) {
                                u32 w = 0;
                                std::memcpy(&w, tt_ptr + off, sizeof(u32));
                                const u32 h = w & 0x3FFFFFFFu;
                                if ((h & 0x7FFFu) >= 0x0001u && (h & 0x7FFFu) <= 0x0400u &&
                                    h >= 0x00018000u && h <= 0x00400000u) {
                                    ctx.active_thread_handles.push_back(h);
                                }
                            }
                        }

                        if (!is_main_thread && stack_sz > 0 && stack_sz <= 0x800000) {
                            if (orig_stack >= 0x1000) {
                                ctx.AddRange(orig_stack, orig_stack + stack_sz);
                            }
                            if (alias_stack >= 0x1000) {
                                ctx.AddRange(alias_stack, alias_stack + stack_sz);
                            }
                        }

                        if (!is_main_thread && thread_arg >= 0x80000000ULL &&
                            memory.GetPointerSilent(Common::ProcessAddress{thread_arg}) != nullptr) {
                            ctx.AddRange(thread_arg >= 0x1000 ? thread_arg - 0x1000 : thread_arg,
                                         thread_arg + 0x10000);
                        }

                        // Background worker threads (AudioThread, FileIO, etc.) whose ThreadType lives in
                        // .bss or heap outside sdk: protect the ThreadType + adjacent worker/audio state.
                        if (!is_main_thread && layout.sdk_start > 0 && thread_type_ptr < layout.sdk_start) {
                            ctx.AddRange(thread_type_ptr >= 0x200 ? thread_type_ptr - 0x200
                                                                  : thread_type_ptr,
                                         thread_type_ptr + 0x140000);
                        }
                    }
                }
            }
        }

        // 2. Protect TPIDR_EL0 thread control block
        const auto& tctx = thread.GetContext();
        if (tctx.tpidr >= 0x1000) {
            ctx.AddRange(tctx.tpidr >= 0x1000 ? tctx.tpidr - 0x1000 : tctx.tpidr,
                         tctx.tpidr + 0x2000);
        }

        // 3. Protect active call stack around SP, FP, and top of stack VMA
        if (tctx.sp >= 0x1000) {
            ctx.AddRange(tctx.sp >= 0x4000 ? tctx.sp - 0x4000 : tctx.sp, tctx.sp + 0x8000);
            Kernel::KMemoryInfo s_info{};
            Kernel::Svc::PageInfo s_pinfo{};
            if (page_table.QueryInfo(&s_info, &s_pinfo, Common::ProcessAddress{tctx.sp}).IsSuccess()) {
                const auto s_svc = s_info.GetSvcMemoryInfo();
                if (s_svc.state != Kernel::Svc::MemoryState::Free) {
                    const u64 stack_top = static_cast<u64>(s_info.GetAddress()) + s_info.GetSize();
                    const u64 protect_from =
                        (stack_top > tctx.sp && (stack_top - tctx.sp) <= 0x100000)
                            ? (tctx.sp >= 0x4000 ? tctx.sp - 0x4000 : tctx.sp)
                            : (stack_top >= 0x20000 ? stack_top - 0x20000 : stack_top);
                    ctx.AddRange(protect_from, stack_top);

                    // Scan active stack [sp, sp + 0x2000] for pointers to heap/bss pages containing
                    // SDK vtables/callbacks (e.g. NVNdevice, NVNqueue, NVNwindow).
                    const u64 scan_end = std::min<u64>(stack_top, tctx.sp + 0x2000);
                    for (u64 s_addr = tctx.sp & ~7ULL; s_addr < scan_end; s_addr += 8) {
                        const u8* sp_ptr = memory.GetPointerSilent(Common::ProcessAddress{s_addr});
                        if (!sp_ptr) {
                            break;
                        }
                        u64 val = 0;
                        std::memcpy(&val, sp_ptr, sizeof(u64));
                        if (val >= 0x80000000ULL && (layout.sdk_start == 0 || val < layout.sdk_start)) {
                            const u64 target_page = val & ~0xFFFULL;
                            bool near_any_ttp = false;
                            for (const u64 ttp : all_thread_type_ptrs) {
                                if ((target_page >= (ttp & ~0xFFFULL) &&
                                     target_page - (ttp & ~0xFFFULL) <= 0x8000) ||
                                    ((ttp & ~0xFFFULL) > target_page &&
                                     (ttp & ~0xFFFULL) - target_page <= 0x8000)) {
                                    near_any_ttp = true;
                                    break;
                                }
                            }
                            if (near_any_ttp) {
                                continue;
                            }
                            if (const u8* tp =
                                    memory.GetPointerSilent(Common::ProcessAddress{target_page})) {
                                bool has_sdk_ref = false;
                                const auto* w64 = reinterpret_cast<const u64*>(tp);
                                for (std::size_t k = 0; k < (V2_CHUNK_SIZE / sizeof(u64)); ++k) {
                                    if (ctx.IsSdkPointer(w64[k])) {
                                        has_sdk_ref = true;
                                        break;
                                    }
                                }
                                if (has_sdk_ref) {
                                    bool has_main_code_ref = false;
                                    u64 min_sdk_off = V2_CHUNK_SIZE;
                                    u64 max_sdk_off = 0;
                                    for (std::size_t k = 0; k < (V2_CHUNK_SIZE / sizeof(u64)); ++k) {
                                        const u64 w = w64[k];
                                        if (ctx.IsSdkPointer(w)) {
                                            const u64 off = k * sizeof(u64);
                                            if (off < min_sdk_off) min_sdk_off = off;
                                            if (off > max_sdk_off) max_sdk_off = off;
                                        } else if (layout.main_code_end > layout.main_code_start &&
                                                   w >= layout.main_code_start && w < layout.main_code_end) {
                                            has_main_code_ref = true;
                                        }
                                    }
                                    if (has_main_code_ref && min_sdk_off < V2_CHUNK_SIZE) {
                                        const u64 prot_start = target_page + (min_sdk_off >= 0x10 ? (min_sdk_off - 0x10) & ~0x7ULL : 0);
                                        const u64 prot_end = std::min<u64>(target_page + V2_CHUNK_SIZE, target_page + max_sdk_off + 0x18ULL);
                                        ctx.AddRange(prot_start, prot_end);
                                    } else {
                                        ctx.AddRange(target_page, target_page + 0x6000);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (tctx.r[29] >= 0x1000) {
            const u64 fp = tctx.r[29];
            ctx.AddRange(fp >= 0x2000 ? fp - 0x2000 : fp, fp + 0x4000);
        }

        // 4. For background threads ONLY (e.g. AudioThread), protect structures pointed to by X0..X28
        if (!is_main_thread) {
            for (std::size_t reg_idx = 0; reg_idx < 29; ++reg_idx) {
                const u64 rv = tctx.r[reg_idx];
                if ((rv & 7ULL) == 0 && rv >= 0x80000000ULL &&
                    memory.GetPointerSilent(Common::ProcessAddress{rv}) != nullptr) {
                    ctx.AddRange(rv >= 0x4000 ? rv - 0x4000 : rv, rv + 0x68000);
                }
            }
        }

        // 5. Protect active kernel Mutex / CondVar wait keys
        const u64 addr_key = GetInteger(thread.GetAddressKey());
        if (addr_key >= 0x1000) {
            ctx.AddRange(addr_key - 0x80, addr_key + 0x80);
        }
        const u64 cv_key = thread.GetConditionVariableKey();
        if (cv_key >= 0x1000) {
            ctx.AddRange(cv_key - 0x80, cv_key + 0x80);
        }
    }

    ctx.MergeRanges();
    return ctx;
}

bool IsChunkZero4KB(const u8* ptr, std::size_t len) {
    if (!ptr) {
        return true;
    }
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

[[maybe_unused]] bool IsPageEdgeNonZero(const u8* ptr) {
    if (!ptr) {
        return false;
    }
    const auto* w = reinterpret_cast<const u64*>(ptr);
    // Check first cache line (64B), middle cache line, and last cache line of the 4KB page
    for (int i = 0; i < 8; ++i) {
        if (w[i] != 0 || w[256 + i] != 0 || w[504 + i] != 0) {
            return true;
        }
    }
    return false;
}

SaveStateResult WriteSnapshotToDisk(const std::shared_ptr<CachedSaveState>& snapshot,
                                    const std::filesystem::path& path) {
    Common::SetCurrentThreadPriority(Common::ThreadPriority::Low);
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

    // 1. Analyze process layout (skips rtld, nnSdk, GOT, and GPU/NVN driver fragments)
    auto layout = AnalyzeProcessLayout(*process);
    const auto& regions = layout.writable_regions;
    auto& memory = system.ApplicationMemory();

    auto& payload = snapshot->uncompressed_payload;
    payload.reserve(450ULL * 1024 * 1024);

    const u32 region_count = static_cast<u32>(regions.size());
    payload.resize(sizeof(u32));
    std::memcpy(payload.data(), &region_count, sizeof(u32));

    u32 total_chunks = 0;
    u32 total_stored_chunks = 0;

    for (const auto& reg : regions) {
        const u32 num_chunks = static_cast<u32>((reg.size + V2_CHUNK_SIZE - 1) / V2_CHUNK_SIZE);
        const u32 bitmask_bytes = (num_chunks + 7) / 8;

        const std::size_t header_offset = payload.size();
        payload.resize(header_offset + sizeof(PackedRegionHeader) + bitmask_bytes, 0);

        u32 stored_chunks = 0;
        [[maybe_unused]] u32 consecutive_zero_pages = 0;
        const bool is_huge_heap = reg.size >= 256ULL * 1024 * 1024;

        for (u32 c = 0; c < num_chunks; ++c) {
            (void)is_huge_heap;

            const u64 chunk_off = static_cast<u64>(c) * V2_CHUNK_SIZE;
            const std::size_t chunk_len =
                static_cast<std::size_t>(std::min<u64>(V2_CHUNK_SIZE, reg.size - chunk_off));
            const u64 page_addr = reg.address + chunk_off;
            const u8* chunk_ptr = memory.GetPointerSilent(Common::ProcessAddress{page_addr});

            if (chunk_ptr && !IsChunkZero4KB(chunk_ptr, chunk_len)) {
                payload[header_offset + sizeof(PackedRegionHeader) + (c / 8)] |=
                    static_cast<u8>(1u << (c % 8));
                payload.insert(payload.end(), chunk_ptr, chunk_ptr + chunk_len);
                stored_chunks++;
                consecutive_zero_pages = 0;
            } else {
                consecutive_zero_pages++;
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

    // 2. Collect thread metadata
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

    // Ensure GPU has finished any in-flight rendering/command buffers before memory changes
    system.GPU().WaitIdleForLoadState();

    process = kernel.ApplicationProcess();
    if (!process) {
        system.UnstallApplication();
        return SaveStateResult::ErrorSystemNotRunning;
    }

    auto& memory = system.ApplicationMemory();
    auto layout = AnalyzeProcessLayout(*process);
    [[maybe_unused]] const auto sync_ctx = BuildSyncProtectionContext(*process, memory, layout);

    if (header.flags & SaveStateFlag_WritableRegions) {
        if (decompressed.size() < sizeof(u32)) {
            system.UnstallApplication();
            return SaveStateResult::ErrorCorruptedFile;
        }

        u32 region_count = 0;
        std::memcpy(&region_count, decompressed.data(), sizeof(u32));
        std::size_t cursor = sizeof(u32);

        // Pre-scan saved regions to dynamically expand guest heap if restoring from cold title screen
        const u64 heap_start = GetInteger(process->GetPageTable().GetHeapRegionStart());
        u64 needed_heap_size = 0;
        {
            std::size_t scan_cursor = cursor;
            for (u32 r = 0; r < region_count; ++r) {
                if (scan_cursor + sizeof(PackedRegionHeader) > decompressed.size()) {
                    break;
                }
                PackedRegionHeader rh{};
                std::memcpy(&rh, decompressed.data() + scan_cursor, sizeof(PackedRegionHeader));
                scan_cursor += sizeof(PackedRegionHeader);
                const u32 bm_bytes = (rh.num_chunks + 7) / 8;
                if (scan_cursor + bm_bytes > decompressed.size()) {
                    break;
                }
                const u8* bm = decompressed.data() + scan_cursor;
                scan_cursor += bm_bytes;
                if (rh.address == heap_start && rh.size > needed_heap_size) {
                    needed_heap_size = rh.size;
                }
                for (u32 c = 0; c < rh.num_chunks; ++c) {
                    if ((bm[c / 8] & (1u << (c % 8))) != 0) {
                        const std::size_t chunk_len = static_cast<std::size_t>(
                            std::min<u64>(V2_CHUNK_SIZE, rh.size - static_cast<u64>(c) * V2_CHUNK_SIZE));
                        scan_cursor += chunk_len;
                    }
                }
            }
        }

        if (needed_heap_size > 0) {
            Kernel::KProcessAddress out_addr{};
            auto res = process->GetPageTable().SetHeapSize(std::addressof(out_addr), needed_heap_size);
            if (res.IsSuccess()) {
                LOG_INFO(Core, "SaveState: Expanded process heap to {} MB for savestate restore",
                         needed_heap_size / (1024 * 1024));
                layout = AnalyzeProcessLayout(*process);
            } else {
                LOG_WARNING(Core, "SaveState: Failed to expand heap to {} MB (res={:#x})",
                            needed_heap_size / (1024 * 1024), res.GetInnerValue());
            }
        }

        std::array<u8, V2_CHUNK_SIZE> patched_page{};
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

            // Check whether this saved region is in our allowed safe writable regions list
            bool region_safe = (reg_hdr.address == heap_start);
            if (!region_safe) {
                for (const auto& allowed : layout.writable_regions) {
                    if (reg_hdr.address >= allowed.address &&
                        (reg_hdr.address + reg_hdr.size) <= (allowed.address + allowed.size)) {
                        region_safe = true;
                        break;
                    }
                }
            }
            if ((layout.rtld_end > layout.rtld_start && reg_hdr.address >= layout.rtld_start &&
                 reg_hdr.address < layout.rtld_end) ||
                (layout.sdk_end > layout.sdk_start && reg_hdr.address >= layout.sdk_start &&
                 reg_hdr.address < layout.sdk_end)) {
                region_safe = false;
            }

            LOG_INFO(Core,
                     "SaveState Diag: SavedRegion[{}] = {:#x} .. {:#x} (size={:#x}, chunks={}, "
                     "stored={}, safe={})",
                     r, reg_hdr.address, reg_hdr.address + reg_hdr.size, reg_hdr.size,
                     reg_hdr.num_chunks, reg_hdr.stored_chunks, region_safe);

            const bool is_huge_heap = reg_hdr.size >= 256ULL * 1024 * 1024;
            u32 consecutive_unstored = 0;

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
                    consecutive_unstored = 0;
                } else {
                    consecutive_unstored++;
                }

                if (!region_safe) {
                    continue;
                }

                (void)is_huge_heap;
                (void)consecutive_unstored;

                u8* live_ptr = memory.GetPointerSilent(Common::ProcessAddress{page_addr});
                if (!live_ptr) {
                    continue;
                }

                if (!is_stored) {
                    if (IsChunkZero4KB(live_ptr, chunk_len)) {
                        continue;
                    }
                } else {
                    if (std::memcmp(live_ptr, target_data, chunk_len) == 0) {
                        continue;
                    }
                }

                const u64 page_end = page_addr + chunk_len;
                bool has_overlap = false;
                bool fully_protected = false;

                for (const auto& pr : sync_ctx.ranges) {
                    if (pr.end <= page_addr) {
                        continue;
                    }
                    if (pr.start >= page_end) {
                        break;
                    }
                    if (pr.start <= page_addr && pr.end >= page_end) {
                        fully_protected = true;
                        break;
                    }
                    if (!has_overlap) {
                        std::memcpy(patched_page.data(), target_data, chunk_len);
                        has_overlap = true;
                    }
                    const u64 ov_start = std::max(page_addr, pr.start);
                    const u64 ov_end = std::min(page_end, pr.end);
                    const std::size_t off = static_cast<std::size_t>(ov_start - page_addr);
                    const std::size_t len = static_cast<std::size_t>(ov_end - ov_start);
                    std::memcpy(patched_page.data() + off, live_ptr + off, len);
                }

                if (fully_protected) {
                    continue;
                }

                // Preserve 8-byte SDK pointers inside small .data/.got sections
                if (reg_hdr.size <= 0x10000) {
                    const std::size_t words64 = chunk_len / sizeof(u64);
                    const auto* live_w64 = reinterpret_cast<const u64*>(live_ptr);
                    const auto* targ_w64 = reinterpret_cast<const u64*>(
                        has_overlap ? patched_page.data() : target_data);
                    for (std::size_t w = 0; w < words64; ++w) {
                        if (live_w64[w] != targ_w64[w] &&
                            (sync_ctx.IsSdkPointer(live_w64[w]) || sync_ctx.IsSdkPointer(targ_w64[w]))) {
                            if (!has_overlap) {
                                std::memcpy(patched_page.data(), target_data, chunk_len);
                                has_overlap = true;
                                targ_w64 = reinterpret_cast<const u64*>(patched_page.data());
                            }
                            std::memcpy(patched_page.data() + w * sizeof(u64),
                                        live_ptr + w * sizeof(u64), sizeof(u64));
                        }
                    }
                }

                // Preserve 64-byte slots containing active kernel thread handles (e.g. locked nn::os::MutexType)
                if (!sync_ctx.active_thread_handles.empty()) {
                    const std::size_t words32 = chunk_len / sizeof(u32);
                    const auto* live_w32 = reinterpret_cast<const u32*>(live_ptr);
                    const auto* targ_w32 = reinterpret_cast<const u32*>(
                        has_overlap ? patched_page.data() : target_data);
                    for (std::size_t w = 0; w < words32; ++w) {
                        const u32 lw32 = live_w32[w];
                        const u32 tw32 = targ_w32[w];
                        if (lw32 == tw32) {
                            continue;
                        }
                        if (sync_ctx.IsHandleActive(lw32) || sync_ctx.IsHandleActive(tw32)) {
                            const std::size_t byte_off = w * sizeof(u32);
                            const std::size_t slot_start = (byte_off >= 32) ? ((byte_off - 32) & ~7ULL) : 0;
                            const std::size_t slot_end = std::min<std::size_t>(chunk_len, (byte_off + 32 + 7) & ~7ULL);
                            bool has_owner_thread_or_sdk = false;
                            for (std::size_t s_off = slot_start; s_off + sizeof(u64) <= slot_end; s_off += sizeof(u64)) {
                                u64 l_val64 = 0;
                                u64 t_val64 = 0;
                                std::memcpy(&l_val64, live_ptr + s_off, sizeof(u64));
                                std::memcpy(&t_val64, target_data + s_off, sizeof(u64));
                                if (sync_ctx.IsThreadTypePointer(l_val64) ||
                                    sync_ctx.IsThreadTypePointer(t_val64) ||
                                    sync_ctx.IsSdkPointer(l_val64) ||
                                    sync_ctx.IsSdkPointer(t_val64)) {
                                    has_owner_thread_or_sdk = true;
                                    break;
                                }
                            }
                            if (has_owner_thread_or_sdk) {
                                if (!has_overlap) {
                                    std::memcpy(patched_page.data(), target_data, chunk_len);
                                    has_overlap = true;
                                    targ_w32 = reinterpret_cast<const u32*>(patched_page.data());
                                }
                                std::memcpy(patched_page.data() + slot_start, live_ptr + slot_start,
                                            slot_end - slot_start);
                            }
                        }
                    }
                }

                const u8* final_src = has_overlap ? patched_page.data() : target_data;
                if (has_overlap && std::memcmp(live_ptr, final_src, chunk_len) == 0) {
                    continue;
                }

                memory.WriteBlockUnsafe(page_addr, final_src, chunk_len);
                diff_bytes_written += chunk_len;
            }
        }

        LOG_INFO(Core, "SaveState: Differential restore wrote {:.2f} MB of changed 4KB pages",
                 static_cast<double>(diff_bytes_written) / (1024.0 * 1024.0));

        if (!layout.writable_regions.empty()) {
            std::vector<std::pair<DAddr, std::size_t>> modified_gpu_ranges;
            modified_gpu_ranges.reserve(layout.writable_regions.size() * 2);
            auto& pt = process->GetPageTable();
            for (const auto& reg : layout.writable_regions) {
                modified_gpu_ranges.emplace_back(reg.address, reg.size);
                Common::PhysicalAddress phys_start{};
                if (pt.GetPhysicalAddress(&phys_start, Common::ProcessAddress{reg.address}) &&
                    GetInteger(phys_start) != 0) {
                    modified_gpu_ranges.emplace_back(GetInteger(phys_start), reg.size);
                }
            }
            system.GPU().SyncAndInvalidateForLoadState(modified_gpu_ranges);
        }
        system.GPU().InvalidateGPUCache();
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

    // Resynchronize AudioCore internal sessions and buffer queues on savestate restore
    // to prevent audio desync and Service::Audio::ResultNotFound (0x2a2) in guest threads
    system.AudioCore().ResynchronizeSaveState();
    if (auto audren_u = system.ServiceManager().GetService<Service::Audio::IAudioRendererManager>("audren:u")) {
        audren_u->Resynchronize();
    }
    if (auto audout_u = system.ServiceManager().GetService<Service::Audio::IAudioOutManager>("audout:u")) {
        audout_u->Resynchronize();
    }
    if (auto audin_u = system.ServiceManager().GetService<Service::Audio::IAudioInManager>("audin:u")) {
        audin_u->Resynchronize();
    }

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
