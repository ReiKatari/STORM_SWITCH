// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/logging.h"
#include "core/arm/dynarmic/tiered_recompiler.h"

namespace Core::Arm {

TieredRecompiler::TieredRecompiler() = default;

TieredRecompiler::~TieredRecompiler() {
    Shutdown();
}

void TieredRecompiler::Initialize() {
    std::unique_lock lock(mutex);
    is_running = true;
    block_map.clear();
    pending_recompile_queue.clear();
    tier2_optimized_count = 0;
    for (auto& entry : fast_tier_cache) {
        entry.vaddr.store(0, std::memory_order_relaxed);
        entry.ptr.store(nullptr, std::memory_order_relaxed);
        entry.count.store(0, std::memory_order_relaxed);
        entry.tier.store(0, std::memory_order_relaxed);
        entry.code_size.store(0, std::memory_order_relaxed);
    }

    background_thread = std::thread(&TieredRecompiler::BackgroundCompilationLoop, this);
    LOG_INFO(Core_ARM, "TieredRecompiler: JIT Profiler & Tier-2 Optimizer Initialized (Threshold = {} executions)",
             TIER2_RECOMPILATION_THRESHOLD);
}

void TieredRecompiler::Shutdown() {
    if (!is_running) {
        return;
    }
    is_running = false;
    cv.notify_all();

    if (background_thread.joinable()) {
        background_thread.join();
    }
}

void* TieredRecompiler::ReportExecution(u64 guest_vaddr, size_t code_size, void* current_ptr) {
    if (!is_running.load(std::memory_order_relaxed)) {
        return current_ptr;
    }

    const size_t base_idx = (guest_vaddr ^ (guest_vaddr >> 12)) & (FAST_TIER_CACHE_SIZE - 1);

    // Lock-free 4-way probing in fast tier cache
    for (size_t way = 0; way < 4; ++way) {
        const size_t cache_idx = (base_idx + way) & (FAST_TIER_CACHE_SIZE - 1);
        auto& entry = fast_tier_cache[cache_idx];
        const u64 cached_vaddr = entry.vaddr.load(std::memory_order_relaxed);

        if (cached_vaddr == guest_vaddr) {
            const u32 tier = entry.tier.load(std::memory_order_relaxed);
            if (tier == 3) {
                void* const opt_ptr = entry.ptr.load(std::memory_order_relaxed);
                return opt_ptr ? opt_ptr : current_ptr;
            }
            if (tier == 2) {
                return current_ptr;
            }
            // Tier 1 hot path: lock-free atomic execution counter
            const u32 count = entry.count.fetch_add(1, std::memory_order_relaxed) + 1;
            if (count >= TIER2_RECOMPILATION_THRESHOLD) {
                u32 expected_tier = 1;
                if (entry.tier.compare_exchange_strong(expected_tier, 2, std::memory_order_acq_rel)) {
                    // Try-lock when promoting block to higher tier to never stall the guest thread
                    std::unique_lock lock(mutex, std::try_to_lock);
                    if (lock.owns_lock()) {
                        auto& profile = block_map[guest_vaddr];
                        profile.guest_vaddr = guest_vaddr;
                        profile.code_size = code_size;
                        profile.execution_count = count;
                        profile.tier_level = 2;
                        profile.compiled_func_ptr = current_ptr;
                        pending_recompile_queue.push_back(guest_vaddr);
                        cv.notify_one();
                    } else {
                        // Revert tier so subsequent execution can retry promotion without dropping
                        entry.tier.store(1, std::memory_order_release);
                    }
                }
            }
            return current_ptr;
        }

        // Empty slot: claim lock-free
        if (cached_vaddr == 0) {
            u64 expected_zero = 0;
            if (entry.vaddr.compare_exchange_strong(expected_zero, guest_vaddr, std::memory_order_acq_rel)) {
                entry.code_size.store(code_size, std::memory_order_relaxed);
                entry.ptr.store(current_ptr, std::memory_order_relaxed);
                entry.count.store(1, std::memory_order_relaxed);
                entry.tier.store(1, std::memory_order_release);
                return current_ptr;
            }
            if (entry.vaddr.load(std::memory_order_relaxed) == guest_vaddr) {
                entry.count.fetch_add(1, std::memory_order_relaxed);
                return current_ptr;
            }
        }
    }

    // Fallback on 4-way collision: try-lock block_map so guest threads never block
    std::unique_lock lock(mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return current_ptr;
    }
    auto& profile = block_map[guest_vaddr];
    if (profile.guest_vaddr == 0) {
        profile.guest_vaddr = guest_vaddr;
        profile.code_size = code_size;
        profile.tier_level = 1;
        profile.compiled_func_ptr = current_ptr;
    }

    if (profile.tier_level == 3) {
        return profile.compiled_func_ptr;
    }

    profile.execution_count++;
    if (profile.tier_level == 1 && profile.execution_count >= TIER2_RECOMPILATION_THRESHOLD) {
        profile.tier_level = 2; // Marked as pending Tier-2
        pending_recompile_queue.push_back(guest_vaddr);
        cv.notify_one();
    }

    return profile.compiled_func_ptr;
}

void TieredRecompiler::SetTier2RecompileCallback(
    const std::function<void*(u64, size_t)>& callback) {
    std::unique_lock lock(mutex);
    tier2_callback = callback;
}

size_t TieredRecompiler::GetHotBlockCount() const {
    std::unique_lock lock(mutex);
    return block_map.size();
}

size_t TieredRecompiler::GetTier2OptimizedBlockCount() const {
    return tier2_optimized_count.load();
}

void TieredRecompiler::BackgroundCompilationLoop() {
    while (is_running) {
        u64 vaddr = 0;
        size_t size = 0;
        {
            std::unique_lock lock(mutex);
            cv.wait(lock, [this] {
                return !is_running || !pending_recompile_queue.empty();
            });

            if (!is_running) {
                break;
            }

            vaddr = pending_recompile_queue.front();
            pending_recompile_queue.pop_front();
            size = block_map[vaddr].code_size;
        }

        if (tier2_callback) {
            void* new_ptr = tier2_callback(vaddr, size);
            if (new_ptr != nullptr) {
                std::unique_lock lock(mutex);
                block_map[vaddr].compiled_func_ptr = new_ptr;
                block_map[vaddr].tier_level = 3; // Fully Tier-2 Optimized
                tier2_optimized_count++;
                const size_t base_idx = (vaddr ^ (vaddr >> 12)) & (FAST_TIER_CACHE_SIZE - 1);
                for (size_t way = 0; way < 4; ++way) {
                    const size_t c_idx = (base_idx + way) & (FAST_TIER_CACHE_SIZE - 1);
                    if (fast_tier_cache[c_idx].vaddr.load(std::memory_order_relaxed) == vaddr) {
                        fast_tier_cache[c_idx].ptr.store(new_ptr, std::memory_order_relaxed);
                        fast_tier_cache[c_idx].tier.store(3, std::memory_order_release);
                        break;
                    }
                }
            }
        }
    }
}

} // namespace Core::Arm
