// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include "common/common_types.h"
#include "core/arm/dynarmic/storm_tier2_compiler.h"

namespace Core::Arm {

constexpr u32 TIER2_RECOMPILATION_THRESHOLD = 10000;

class TieredRecompiler {
public:
    explicit TieredRecompiler();
    ~TieredRecompiler();

    void Initialize();
    void Shutdown();

    /// Track execution of a JIT basic block (100% lock-free). Returns updated func pointer if Tier-2 is ready
    void* ReportExecution(u64 guest_vaddr, size_t code_size, void* current_ptr);

    /// Register a custom Tier-2 recompiler callback
    void SetTier2RecompileCallback(const std::function<void*(u64 vaddr, size_t size)>& callback);

    [[nodiscard]] size_t GetHotBlockCount() const;
    [[nodiscard]] size_t GetTier2OptimizedBlockCount() const;

private:
    void BackgroundCompilationLoop();

    struct PendingRecompile {
        u64 vaddr{0};
        size_t size{0};
    };

    static constexpr size_t RING_SIZE = 1024;
    static constexpr size_t RING_MASK = RING_SIZE - 1;

    std::array<PendingRecompile, RING_SIZE> ring_buffer{};
    std::atomic<size_t> ring_write_idx{0};
    std::atomic<size_t> ring_read_idx{0};

    HotBlockTable hot_table;
    std::atomic<bool> is_running{false};
    std::atomic<size_t> hot_block_count{0};
    std::atomic<size_t> tier2_optimized_count{0};

    std::function<void*(u64, size_t)> tier2_callback;
    std::thread background_thread;
};

} // namespace Core::Arm
