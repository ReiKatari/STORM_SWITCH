// SPDX-FileCopyrightText: Copyright 2026 Lemon-Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <span>
#include <vector>

#include "common/div_ceil.h"
#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/logging.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/svc_types.h"
#include "core/memory.h"
#include "core/memory_region_scan.h"
#include "core/savestate.h"

namespace Core::SaveState {

namespace {

constexpr std::array<char, 4> Magic{'L', 'M', 'S', 'S'};
constexpr u32 CurrentVersion = 2;

// Regions are stored in fixed-size chunks, each preceded by a one-byte flag: 1 if the whole
// chunk is zero (skip writing/reading the actual bytes), 0 if it isn't (bytes follow as
// usual). Measured on a real, demanding title (Super Mario 3D World): ~80% of captured bytes
// are zero, and ~68% of 4096-byte-aligned chunks are entirely zero - most of a game's
// reserved-but-untouched heap never gets written to. Skipping those chunks shrinks a ~3.4GB
// capture to roughly a third of that, for the cost of one memcmp per chunk - far cheaper than
// general-purpose compression, and unlike compression this doesn't cost anything on the CPU
// budget during Restore()'s memory writes (a zero chunk there is just a memset).
constexpr size_t ChunkSize = 4096;
constexpr std::array<u8, ChunkSize> ZeroChunk{};

} // namespace

bool Capture(Core::System& system, const std::string& path) {
    auto* process = system.ApplicationProcess();
    if (process == nullptr) {
        LOG_ERROR(Core, "SaveState::Capture: no application process");
        return false;
    }

    auto& memory = system.ApplicationMemory();
    auto& page_table = process->GetPageTable();

    std::vector<Kernel::Svc::ThreadContext> thread_contexts;
    for (auto& thread : process->GetThreadList()) {
        thread_contexts.push_back(thread.GetContext());
    }

    const auto regions = EnumerateScannableRegions(page_table);

    const auto tmp_path = path + ".tmp";
    Common::FS::IOFile file{tmp_path, Common::FS::FileAccessMode::Write};
    if (!file.IsOpen()) {
        LOG_ERROR(Core, "SaveState::Capture: failed to open '{}' for writing", tmp_path);
        return false;
    }

    bool ok = file.WriteSpan<char>(Magic) == Magic.size();
    ok = ok && file.WriteObject(CurrentVersion);

    const auto thread_count = static_cast<u32>(thread_contexts.size());
    ok = ok && file.WriteObject(thread_count);
    ok = ok && file.WriteSpan<Kernel::Svc::ThreadContext>(thread_contexts) ==
                   thread_contexts.size();

    const auto region_count = static_cast<u32>(regions.size());
    ok = ok && file.WriteObject(region_count);

    std::vector<u8> buffer;
    // Built up in memory and flushed with a single WriteSpan() per region, same call count as
    // the old one-big-write approach - writing the flag+data stream one tiny piece at a time
    // instead (one fwrite() per 4K chunk, ~800k+ calls for a real capture) made Capture() slow
    // enough to look like a hang.
    std::vector<u8> encoded;
    u64 data_bytes = 0;
    u64 zero_bytes = 0;
    for (const auto& region : regions) {
        if (!ok) {
            break;
        }

        buffer.resize(region.size);
        // ReadBlockUnsafe(), not ReadBlock() - the regular path calls
        // HandleRasterizerDownload() per chunk to sync with the GPU emulation's cache, and
        // a savestate capture reads the whole multi-GB scannable footprint in one pass. The
        // Lemon Cheater hit this exact issue first (see memory_search.cpp): forcing that
        // much cache invalidation in one go visibly wrecks the game's own rendering
        // performance in a way that persists until the game is reloaded, even though the
        // capture itself never writes anything.
        if (!memory.ReadBlockUnsafe(region.address, buffer.data(), region.size)) {
            LOG_ERROR(Core, "SaveState::Capture: failed to read {} bytes at {:#x}", region.size,
                       region.address);
            ok = false;
            break;
        }

        encoded.clear();
        encoded.reserve(region.size + (region.size + ChunkSize - 1) / ChunkSize);
        for (u64 offset = 0; offset < region.size; offset += ChunkSize) {
            const size_t chunk_len =
                static_cast<size_t>(std::min<u64>(ChunkSize, region.size - offset));
            const u8* chunk = buffer.data() + offset;
            const bool is_zero = std::memcmp(chunk, ZeroChunk.data(), chunk_len) == 0;

            encoded.push_back(static_cast<u8>(is_zero ? 1 : 0));
            if (is_zero) {
                zero_bytes += chunk_len;
            } else {
                encoded.insert(encoded.end(), chunk, chunk + chunk_len);
                data_bytes += chunk_len;
            }
        }

        const auto encoded_size = static_cast<u64>(encoded.size());
        ok = ok && file.WriteObject(region.address);
        ok = ok && file.WriteObject(region.size);
        ok = ok && file.WriteObject(encoded_size);
        ok = ok && file.WriteSpan<u8>(encoded) == encoded.size();
    }

    file.Close();

    if (!ok) {
        LOG_ERROR(Core, "SaveState::Capture: failed while writing '{}'", tmp_path);
        void(Common::FS::RemoveFile(tmp_path));
        return false;
    }

    // RenameFile() refuses to overwrite an existing destination - this is a single
    // overwritable quicksave slot, so clear the previous one first if present.
    if (Common::FS::Exists(path) && !Common::FS::RemoveFile(path)) {
        LOG_ERROR(Core, "SaveState::Capture: failed to remove previous '{}'", path);
        void(Common::FS::RemoveFile(tmp_path));
        return false;
    }

    if (!Common::FS::RenameFile(tmp_path, path)) {
        LOG_ERROR(Core, "SaveState::Capture: failed to rename '{}' to '{}'", tmp_path, path);
        return false;
    }

    LOG_INFO(Core,
              "SaveState::Capture: saved {} thread(s), {} region(s) to '{}' ({} byte(s) written, "
              "{} zero byte(s) skipped, {:.1f}% saved)",
              thread_count, region_count, path, data_bytes, zero_bytes,
              (data_bytes + zero_bytes) > 0
                  ? 100.0 * static_cast<double>(zero_bytes) /
                        static_cast<double>(data_bytes + zero_bytes)
                  : 0.0);
    return true;
}

bool Restore(Core::System& system, const std::string& path) {
    auto* process = system.ApplicationProcess();
    if (process == nullptr) {
        LOG_ERROR(Core, "SaveState::Restore: no application process");
        return false;
    }

    Common::FS::IOFile file{path, Common::FS::FileAccessMode::Read};
    if (!file.IsOpen()) {
        LOG_ERROR(Core, "SaveState::Restore: failed to open '{}' for reading", path);
        return false;
    }

    std::array<char, 4> magic{};
    if (file.ReadSpan<char>(magic) != magic.size() || magic != Magic) {
        LOG_ERROR(Core, "SaveState::Restore: '{}' is not a valid savestate file", path);
        return false;
    }

    u32 version = 0;
    if (!file.ReadObject(version) || version != CurrentVersion) {
        LOG_ERROR(Core, "SaveState::Restore: unsupported savestate version in '{}'", path);
        return false;
    }

    u32 thread_count = 0;
    if (!file.ReadObject(thread_count)) {
        LOG_ERROR(Core, "SaveState::Restore: truncated file '{}'", path);
        return false;
    }

    // Match the live thread list against the captured one before touching anything -
    // same list, so iteration order matches Capture()'s as long as nothing created or
    // destroyed a thread in between. Checked before allocating anything sized from the file,
    // so a corrupt count can't turn into a huge allocation.
    std::vector<Kernel::KThread*> live_threads;
    for (auto& thread : process->GetThreadList()) {
        live_threads.push_back(std::addressof(thread));
    }

    if (live_threads.size() != thread_count) {
        LOG_ERROR(Core,
                   "SaveState::Restore: thread count mismatch (saved {}, live {}) - the guest's "
                   "thread set changed since this savestate was captured, refusing to load",
                   thread_count, live_threads.size());
        return false;
    }

    std::vector<Kernel::Svc::ThreadContext> thread_contexts(thread_count);
    if (thread_count > 0 &&
        file.ReadSpan<Kernel::Svc::ThreadContext>(thread_contexts) != thread_contexts.size()) {
        LOG_ERROR(Core, "SaveState::Restore: truncated thread contexts in '{}'", path);
        return false;
    }

    u32 region_count = 0;
    if (!file.ReadObject(region_count)) {
        LOG_ERROR(Core, "SaveState::Restore: truncated file '{}'", path);
        return false;
    }

    // Validate the whole region table before writing any guest memory. Regions are written
    // one by one below, so failing on region N used to leave regions 0..N-1 already restored
    // with the old CPU state still in place - a half-restored game that keeps running. The
    // realistic case is a Quick Save interrupted mid-write (the app killed while writing ~1GB).
    // Header-only pass: seeks over each region's data rather than reading it.
    const s64 regions_start = file.Tell();
    const u64 file_size = file.GetSize();
    for (u32 i = 0; i < region_count; i++) {
        u64 address = 0;
        u64 size = 0;
        u64 encoded_size = 0;
        if (!file.ReadObject(address) || !file.ReadObject(size) || !file.ReadObject(encoded_size)) {
            LOG_ERROR(Core, "SaveState::Restore: truncated region table in '{}'", path);
            return false;
        }
        const u64 chunks = Common::DivCeil(size, static_cast<u64>(ChunkSize));
        const s64 position = file.Tell();
        // Every chunk costs one flag byte, plus its data unless it was all zeros.
        if (position < 0 || static_cast<u64>(position) > file_size || encoded_size < chunks ||
            encoded_size > size + chunks ||
            encoded_size > file_size - static_cast<u64>(position) ||
            !file.Seek(static_cast<s64>(encoded_size), Common::FS::SeekOrigin::CurrentPosition)) {
            LOG_ERROR(Core, "SaveState::Restore: truncated or corrupt region {} in '{}'", i, path);
            return false;
        }
    }
    if (!file.Seek(regions_start)) {
        LOG_ERROR(Core, "SaveState::Restore: failed to rewind '{}'", path);
        return false;
    }

    // A thread still asleep in a kernel wait (condvar/IPC/WaitSynchronization/address
    // arbiter/sleep) right now, at restore time, is parked inside a live host fiber
    // (KThread::GetHostContext()) - its real "where do I resume" state lives on that
    // fiber's own C++ call stack, not in Svc::ThreadContext, and that call stack expects
    // its own guest stack memory to still hold whatever it was holding when it parked.
    // Blitting captured bytes over it - or overwriting a Svc::ThreadContext that no
    // longer matches which wait it's actually sitting in - is exactly what made real
    // games self-terminate via svcBreak on restore (see savestate.h). A thread whose
    // GetWaitReasonForDebugging() == None was genuinely executing and got stopped
    // cleanly at a scheduler dispatch boundary by the caller's Pause(): that suspend
    // path (SuspendType::System, KernelCore::SuspendEmulation) never touches this field,
    // so checking it here still reflects each thread's state as of this restore.
    std::vector<u64> sleeping_stack_tops;
    // Diagnostic only - which specific wait each excluded thread is parked in. A high count
    // by itself isn't necessarily a problem (e.g. an idle worker pool sitting in ConditionVar
    // between jobs is normal and harmless to leave untouched); what matters for a frozen
    // restore is whether something CRITICAL to game-loop progress is among them.
    std::array<u32, 7> wait_reason_counts{};
    for (auto* thread : live_threads) {
        const auto reason = thread->GetWaitReasonForDebugging();
        if (reason != Kernel::ThreadWaitReasonForDebugging::None) {
            sleeping_stack_tops.push_back(thread->GetUserStackTop().GetValue());
            wait_reason_counts[static_cast<size_t>(reason)]++;
        }
    }
    LOG_INFO(Core,
              "SaveState::Restore: {} thread(s) asleep at restore time - None excluded, "
              "Sleep={} IPC={} Synchronization={} ConditionVar={} Arbitration={} Suspended={}",
              sleeping_stack_tops.size(), wait_reason_counts[1], wait_reason_counts[2],
              wait_reason_counts[3], wait_reason_counts[4], wait_reason_counts[5],
              wait_reason_counts[6]);

    // A sleeping thread's own stack is a single region whose address range contains its
    // stack top - matching on that (rather than persisting KMemoryState in the file, or
    // re-querying it) is enough since capture/restore only ever targets the current
    // process' own live layout.
    const auto is_excluded_stack = [&](u64 address, u64 size) {
        return std::any_of(sleeping_stack_tops.begin(), sleeping_stack_tops.end(),
                            [&](u64 stack_top) {
                                return stack_top > address && stack_top <= address + size;
                            });
    };

    auto& memory = system.ApplicationMemory();
    std::vector<u8> buffer;
    // Read back with a single ReadSpan() per region - same reasoning as Capture()'s encoded
    // buffer, avoids one small fread() per 4K chunk.
    std::vector<u8> encoded;
    u32 sleeping_stack_regions = 0;
    u32 unmapped_regions = 0;
    for (u32 i = 0; i < region_count; i++) {
        u64 address = 0;
        u64 size = 0;
        u64 encoded_size = 0;
        if (!file.ReadObject(address) || !file.ReadObject(size) || !file.ReadObject(encoded_size)) {
            LOG_ERROR(Core, "SaveState::Restore: truncated region table in '{}'", path);
            return false;
        }

        encoded.resize(encoded_size);
        if (file.ReadSpan<u8>(encoded) != encoded_size) {
            LOG_ERROR(Core, "SaveState::Restore: truncated region data in '{}'", path);
            return false;
        }

        buffer.resize(size);
        size_t encoded_pos = 0;
        for (u64 offset = 0; offset < size; offset += ChunkSize) {
            const size_t chunk_len = static_cast<size_t>(std::min<u64>(ChunkSize, size - offset));
            if (encoded_pos >= encoded.size()) {
                LOG_ERROR(Core, "SaveState::Restore: truncated region data in '{}'", path);
                return false;
            }

            const u8 flag = encoded[encoded_pos++];
            u8* chunk = buffer.data() + offset;
            if (flag != 0) {
                std::memset(chunk, 0, chunk_len);
            } else {
                if (encoded_pos + chunk_len > encoded.size()) {
                    LOG_ERROR(Core, "SaveState::Restore: truncated region data in '{}'", path);
                    return false;
                }
                std::memcpy(chunk, encoded.data() + encoded_pos, chunk_len);
                encoded_pos += chunk_len;
            }
        }

        if (is_excluded_stack(address, size)) {
            LOG_DEBUG(Core,
                       "SaveState::Restore: leaving {} byte(s) at {:#x} untouched - owned by a "
                       "thread still asleep in a kernel wait",
                       size, address);
            sleeping_stack_regions++;
            continue;
        }

        // WriteBlockUnsafe(), not WriteBlock() - same reasoning as Capture()'s read: skip
        // the per-chunk GPU rasterizer cache sync so restoring several GB in one pass
        // doesn't wreck rendering performance for the rest of the session.
        //
        // A false return here means part of this region is no longer mapped - a legitimate
        // runtime memory-layout change between capture and restore (e.g. a heap allocation
        // freed by whatever the guest did in between, confirmed via a real repro: dying in
        // Super Mario 3D World between Quick Save and Quick Load unmaps part of a captured
        // region every time), not file corruption. WalkBlock() already wrote every page in
        // this region that IS still mapped; only the unmapped pages within it were skipped.
        // Treating this the same as a truncated/corrupt file - aborting the whole restore -
        // left the rest of memory partially reverted and thread contexts completely
        // untouched (that loop only runs after this one finishes). Skip this region like an
        // excluded sleeping-thread stack instead and keep going.
        if (!memory.WriteBlockUnsafe(address, buffer.data(), size)) {
            LOG_WARNING(Core,
                       "SaveState::Restore: {} byte(s) at {:#x} are no longer fully mapped - "
                       "skipping (already-mapped pages within this region were still written)",
                       size, address);
            unmapped_regions++;
            continue;
        }
    }

    // Everything was read and validated successfully - only now overwrite CPU state,
    // so a truncated/corrupt file never leaves the guest half-restored. Same exclusion
    // as above: a still-sleeping thread's context is left exactly as its live fiber
    // expects to find it.
    for (size_t i = 0; i < live_threads.size(); i++) {
        if (live_threads[i]->GetWaitReasonForDebugging() !=
            Kernel::ThreadWaitReasonForDebugging::None) {
            continue;
        }
        live_threads[i]->GetContext() = thread_contexts[i];
    }

    LOG_INFO(Core,
              "SaveState::Restore: loaded {} thread(s), {} region(s) from '{}' ({} region(s) left "
              "untouched - {} thread(s) still asleep in a kernel wait, {} region(s) skipped - no "
              "longer mapped)",
              thread_count, region_count, path, sleeping_stack_regions + unmapped_regions,
              sleeping_stack_tops.size(), unmapped_regions);
    return true;
}

bool HasRiskyPendingWaits(Core::System& system) {
    auto* process = system.ApplicationProcess();
    if (process == nullptr) {
        return false;
    }

    for (auto& thread : process->GetThreadList()) {
        const auto reason = thread.GetWaitReasonForDebugging();
        if (reason == Kernel::ThreadWaitReasonForDebugging::ConditionVar ||
            reason == Kernel::ThreadWaitReasonForDebugging::Arbitration) {
            return true;
        }
    }
    return false;
}

} // namespace Core::SaveState
