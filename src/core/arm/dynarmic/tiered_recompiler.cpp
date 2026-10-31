// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include "common/logging.h"
#include "core/arm/dynarmic/tiered_recompiler.h"

namespace Core::Arm {

TieredRecompiler::TieredRecompiler() = default;

TieredRecompiler::~TieredRecompiler() {
    Shutdown();
}

void TieredRecompiler::Initialize() {
    is_running.store(true, std::memory_order_release);
    hot_table.Clear();
    hot_block_count.store(0, std::memory_order_relaxed);
    tier2_optimized_count.store(0, std::memory_order_relaxed);
    ring_write_idx.store(0, std::memory_order_relaxed);
    ring_read_idx.store(0, std::memory_order_relaxed);

    background_thread = std::thread(&TieredRecompiler::BackgroundCompilationLoop, this);
    LOG_INFO(Core_ARM, "TieredRecompiler: Lock-free JIT Profiler & Tier-2 Optimizer Initialized (Threshold = {} executions)",
             TIER2_RECOMPILATION_THRESHOLD);
}

void TieredRecompiler::Shutdown() {
    if (!is_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    if (background_thread.joinable()) {
        background_thread.join();
    }
}

void* TieredRecompiler::ReportExecution(u64 guest_vaddr, size_t code_size, void* current_ptr) {
    if (!is_running.load(std::memory_order_relaxed)) [[unlikely]] {
        return current_ptr;
    }

    auto& entry = hot_table.GetEntry(guest_vaddr);
    const u64 tag = entry.tag.load(std::memory_order_relaxed);

    if (tag != guest_vaddr) [[unlikely]] {
        if (tag == 0 || entry.hits.load(std::memory_order_relaxed) < 5) {
            entry.tag.store(guest_vaddr, std::memory_order_relaxed);
            entry.hits.store(1, std::memory_order_relaxed);
            entry.tier.store(1, std::memory_order_relaxed);
            entry.code_size.store(static_cast<u32>(code_size), std::memory_order_relaxed);
            entry.compiled_ptr.store(reinterpret_cast<uintptr_t>(current_ptr), std::memory_order_relaxed);
            hot_block_count.fetch_add(1, std::memory_order_relaxed);
        }
        return current_ptr;
    }

    const u32 hits = entry.hits.fetch_add(1, std::memory_order_relaxed) + 1;
    if (hits >= TIER2_RECOMPILATION_THRESHOLD && entry.tier.load(std::memory_order_relaxed) == 1) [[unlikely]] {
        u32 expected_tier = 1;
        if (entry.tier.compare_exchange_strong(expected_tier, 2, std::memory_order_acq_rel)) {
            const size_t w = ring_write_idx.fetch_add(1, std::memory_order_acq_rel);
            ring_buffer[w & RING_MASK] = PendingRecompile{guest_vaddr, code_size};
        }
    }

    const uintptr_t ptr = entry.compiled_ptr.load(std::memory_order_acquire);
    return ptr ? reinterpret_cast<void*>(ptr) : current_ptr;
}

void TieredRecompiler::SetTier2RecompileCallback(
    const std::function<void*(u64, size_t)>& callback) {
    tier2_callback = callback;
}

size_t TieredRecompiler::GetHotBlockCount() const {
    return hot_table.CountActive();
}

size_t TieredRecompiler::GetTier2OptimizedBlockCount() const {
    return tier2_optimized_count.load(std::memory_order_relaxed);
}

void TieredRecompiler::BackgroundCompilationLoop() {
    while (is_running.load(std::memory_order_relaxed)) {
        const size_t r = ring_read_idx.load(std::memory_order_acquire);
        const size_t w = ring_write_idx.load(std::memory_order_acquire);

        if (r == w) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const auto item = ring_buffer[r & RING_MASK];
        ring_read_idx.store(r + 1, std::memory_order_release);

        if (item.vaddr != 0 && tier2_callback) {
            void* new_ptr = tier2_callback(item.vaddr, item.size);
            if (new_ptr != nullptr) {
                auto& entry = hot_table.GetEntry(item.vaddr);
                if (entry.tag.load(std::memory_order_relaxed) == item.vaddr) {
                    entry.compiled_ptr.store(reinterpret_cast<uintptr_t>(new_ptr), std::memory_order_release);
                    entry.tier.store(3, std::memory_order_release);
                    tier2_optimized_count.fetch_add(1, std::memory_order_relaxed);
                    StormTier2Compiler::s_tier2_compiled_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    }
}

} // namespace Core::Arm
