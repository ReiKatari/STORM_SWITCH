// SPDX-FileCopyrightText: 2026 STORM Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/texture_cache/storm_mvcc_radix.h"
#include "video_core/renderer_vulkan/storm_hft_staging_ring.h"
#include "video_core/renderer_vulkan/storm_direct_uma_arena.h"

namespace VideoCommon {

TEST_CASE("StormMvccRadixDirectory: 40-bit 1-to-N Aliasing and Compaction", "[video_core]") {
    constexpr u64 kBaseAddr = 0x180000000ULL; // > 4 GiB (40-bit address)

    SECTION("1-to-N page aliasing registration and count") {
        StormMvccRadixDirectory dir;
        dir.RegisterPage(kBaseAddr, 100);
        dir.RegisterPage(kBaseAddr, 101);
        dir.RegisterPage(kBaseAddr, 102);

        REQUIRE(dir.CountOnPage(kBaseAddr) == 3);

        std::vector<u32> collected;
        dir.ForEachOnPage(kBaseAddr, [&](u32 id) {
            collected.push_back(id);
        });

        REQUIRE(collected.size() == 3);
        REQUIRE(std::find(collected.begin(), collected.end(), 100) != collected.end());
        REQUIRE(std::find(collected.begin(), collected.end(), 101) != collected.end());
        REQUIRE(std::find(collected.begin(), collected.end(), 102) != collected.end());
    }

    SECTION("Swap-with-tail compaction leaving zero tombstones") {
        StormMvccRadixDirectory dir;
        dir.RegisterPage(kBaseAddr, 201);
        dir.RegisterPage(kBaseAddr, 202);
        dir.RegisterPage(kBaseAddr, 203);

        // Unregister middle element
        dir.UnregisterPage(kBaseAddr, 202);
        REQUIRE(dir.CountOnPage(kBaseAddr) == 2);

        std::vector<u32> after_unreg;
        dir.ForEachOnPage(kBaseAddr, [&](u32 id) {
            after_unreg.push_back(id);
        });
        REQUIRE(after_unreg.size() == 2);
        REQUIRE(std::find(after_unreg.begin(), after_unreg.end(), 202) == after_unreg.end());

        // Unregister remaining elements
        dir.UnregisterPage(kBaseAddr, 201);
        dir.UnregisterPage(kBaseAddr, 203);
        REQUIRE(dir.CountOnPage(kBaseAddr) == 0);
    }
}

TEST_CASE("SelectCausalFramebuffer: Ping-Pong and Static Splash Resolution", "[video_core]") {
    SECTION("Ping-pong dynamic resolution with 3-way aliasing") {
        std::array<StormMvccCandidate, 3> cands{{
            {10, 0x20000000ULL, 1920, 1080, 1, 0, 100, true, false},
            {20, 0x20000000ULL, 1920, 1080, 1, 0, 105, true, false}, // Newest active RT
            {30, 0x20000800ULL, 1920, 1080, 1, 0, 110, true, false}, // Offset mismatch
        }};

        const u32 chosen = SelectCausalFramebuffer(
            cands.data(), cands.size(), 0x20000000ULL, 1920, 1080, 1);
        REQUIRE(chosen == 20);
    }

    SECTION("Absolum static splash causality (gpu_tick = 0, cpu_modified = true)") {
        StormMvccCandidate splash{
            .image_id = 77,
            .cpu_addr = 0x30000000ULL,
            .width = 1280,
            .height = 720,
            .format_class = 1,
            .cpu_tick = 50,
            .gpu_tick = 0,
            .gpu_modified = false,
            .cpu_modified = true,
        };

        const u32 chosen = SelectCausalFramebuffer(
            &splash, 1, 0x30000000ULL, 1280, 720, 1);
        REQUIRE(chosen == 77);
    }
}

} // namespace VideoCommon

namespace Vulkan {

TEST_CASE("StormMpscTlabStagingRing: Multi-thread Concurrency and Reclamation", "[video_core]") {
    StormMpscTlabStagingRing ring(16ULL * 1024ULL * 1024ULL); // 16 MiB ring

    constexpr int kNumProducers = 4;
    constexpr size_t kAllocsPerThread = 500;
    std::atomic<bool> start_signal{false};
    std::atomic<size_t> successful_allocs{0};

    std::vector<std::thread> producers;
    for (int t = 0; t < kNumProducers; ++t) {
        producers.emplace_back([&, t]() {
            while (!start_signal.load(std::memory_order_acquire)) {}
            u8 data[128];
            std::memset(data, 0x10 + t, sizeof(data));
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
                auto alloc = ring.AllocateAndCommit(data, sizeof(data), 1);
                if (alloc.ptr != nullptr && alloc.ptr[0] == static_cast<u8>(0x10 + t)) {
                    successful_allocs.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    start_signal.store(true, std::memory_order_release);
    for (auto& p : producers) {
        p.join();
    }

    REQUIRE(successful_allocs.load() == kNumProducers * kAllocsPerThread);

    // Reclaim epoch
    ring.ReclaimCompletedEpoch(1);
}

TEST_CASE("StormHierarchicalDirtyBitset: tzcnt Accuracy and Clearing", "[video_core]") {
    constexpr size_t kPages = 4096;
    StormHierarchicalDirtyBitset bitset(kPages);

    bitset.MarkDirty(15, 0x1ULL);
    bitset.MarkDirty(128, 0x2ULL);
    bitset.MarkDirty(2047, 0x4ULL);

    std::vector<size_t> scanned_pages;
    bitset.ForEachDirtyAndClear([&](size_t page_idx, u64 submask) {
        scanned_pages.push_back(page_idx);
    });

    REQUIRE(scanned_pages.size() == 3);
    REQUIRE(scanned_pages[0] == 15);
    REQUIRE(scanned_pages[1] == 128);
    REQUIRE(scanned_pages[2] == 2047);

    // Verify bitset is now completely clean
    size_t count_after = 0;
    bitset.ForEachDirtyAndClear([&](size_t, u64) {
        count_after++;
    });
    REQUIRE(count_after == 0);
}

TEST_CASE("StormDirectUmaArena: Strict Mutation Gate Acceptance and Rejection", "[video_core]") {
    constexpr size_t kArenaSize = 65536 * 2;
#ifdef _WIN32
    u8* mock_arena = static_cast<u8*>(_aligned_malloc(kArenaSize, 65536));
#else
    void* ptr = nullptr;
    posix_memalign(&ptr, 65536, kArenaSize);
    u8* mock_arena = static_cast<u8*>(ptr);
#endif
    std::memset(mock_arena, 0x42, kArenaSize);

    SECTION("Valid dynamic uniform buffer accepted") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 2,
            .size = 2048,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == true);

        auto binding = arena.BindStreamingOrFallback(0, 2048, info);
        REQUIRE(binding.ptr != nullptr);
        REQUIRE(binding.zero_copy_used == true);
    }

    SECTION("Static mesh rejected (0 cpu writes)") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 0,
            .size = 4096,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

    SECTION("Oversized buffer rejected (> 256 KiB)") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 4,
            .size = 512 * 1024,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

    SECTION("GPU-written SSBO rejected") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 1,
            .size = 1024,
            .has_gpu_writes = true,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

#ifdef _WIN32
    _aligned_free(mock_arena);
#else
    free(mock_arena);
#endif
}

} // namespace Vulkan
