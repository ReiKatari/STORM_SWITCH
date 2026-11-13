// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define stormstorm_aligned_malloc(sz, al) storm_aligned_malloc((sz), (al))
#define stormstorm_aligned_free(p) storm_aligned_free(p)
#else
#include <sys/mman.h>
#include <stdlib.h>
inline void* stormstorm_aligned_malloc(size_t sz, size_t al) {
    void* p = nullptr;
    if (posix_memalign(&p, al, sz) != 0) {
        return nullptr;
    }
    return p;
}
inline void stormstorm_aligned_free(void* p) {
    free(p);
}
#endif

#include "common/common_types.h"

namespace Vulkan {

/// Lock-free High-Frequency Trading (HFT) MPSC TLAB Staging Ring
class StormMpscTlabStagingRing {
public:
    static constexpr size_t CHUNK_SIZE = 65536; // 64 KiB TLAB chunk
    static constexpr size_t NUM_PREPOOLED_OVERFLOW = 32;

    struct Allocation {
        u8* ptr{nullptr};
        size_t offset{0};
        u64 epoch{0};
        bool from_overflow{false};
    };

    explicit StormMpscTlabStagingRing(size_t primary_ring_bytes) {
        ring_capacity = std::bit_ceil((std::max)(primary_ring_bytes, CHUNK_SIZE));
        ring_mask = ring_capacity - 1;
        num_chunks = ring_capacity / CHUNK_SIZE;
        chunk_epochs = std::make_unique<std::atomic<u64>[]>(num_chunks);
        for (size_t i = 0; i < num_chunks; ++i) {
            chunk_epochs[i].store(0, std::memory_order_relaxed);
        }

#ifdef _WIN32
        ring_base = static_cast<u8*>(VirtualAlloc(nullptr, ring_capacity,
                                                  MEM_COMMIT | MEM_RESERVE,
                                                  PAGE_READWRITE));
#else
        ring_base = static_cast<u8*>(mmap(nullptr, ring_capacity,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
#endif

        for (size_t i = 0; i < NUM_PREPOOLED_OVERFLOW; ++i) {
            overflow_slabs[i].ptr = static_cast<u8*>(storm_aligned_malloc(CHUNK_SIZE, 64));
            overflow_slabs[i].size = CHUNK_SIZE;
            overflow_slabs[i].epoch.store(0, std::memory_order_relaxed);
            overflow_slabs[i].in_use.store(false, std::memory_order_relaxed);
        }
    }

    ~StormMpscTlabStagingRing() {
        if (ring_base) {
#ifdef _WIN32
            VirtualFree(ring_base, 0, MEM_RELEASE);
#else
            munmap(ring_base, ring_capacity);
#endif
            ring_base = nullptr;
        }
        for (size_t i = 0; i < NUM_PREPOOLED_OVERFLOW; ++i) {
            if (overflow_slabs[i].ptr) {
                storm_aligned_free(overflow_slabs[i].ptr);
                overflow_slabs[i].ptr = nullptr;
            }
        }
        for (auto& dyn : dynamic_overflow_list) {
            if (dyn.ptr) {
                storm_aligned_free(dyn.ptr);
            }
        }
        dynamic_overflow_list.clear();
    }

    StormMpscTlabStagingRing(const StormMpscTlabStagingRing&) = delete;
    StormMpscTlabStagingRing& operator=(const StormMpscTlabStagingRing&) = delete;

    [[nodiscard]] Allocation Allocate(size_t size, u64 epoch, size_t alignment = 64) noexcept {
        static thread_local ThreadTlab t_tlab;

        // TLAB fastpath: zero atomic instructions and zero mutex locks
        const size_t aligned_used = (t_tlab.used_bytes + (alignment - 1)) & ~(alignment - 1);
        if (t_tlab.owner == this && aligned_used + size <= CHUNK_SIZE) {
            const u64 sub_cursor = t_tlab.chunk_cursor + aligned_used;
            const size_t ring_offset = static_cast<size_t>(sub_cursor & ring_mask);
            u8* ptr = ring_base + ring_offset;
            t_tlab.used_bytes = aligned_used + size;
            if (epoch > t_tlab.current_epoch) {
                t_tlab.current_epoch = epoch;
                const size_t chunk_idx = (t_tlab.chunk_cursor / CHUNK_SIZE) & (num_chunks - 1);
                chunk_epochs[chunk_idx].store(epoch, std::memory_order_release);
            }
            return Allocation{ptr, ring_offset, epoch, false};
        }

        if (size > CHUNK_SIZE) {
            return AllocateLarge(size, epoch, alignment);
        }

        return ClaimNewTlabChunk(t_tlab, size, epoch, alignment);
    }

    [[nodiscard]] Allocation AllocateAndCommit(const void* src, size_t size, u64 epoch, size_t alignment = 64) noexcept {
        Allocation alloc = Allocate(size, epoch, alignment);
        if (alloc.ptr && src && size > 0) {
            std::memcpy(alloc.ptr, src, size);
            std::atomic_thread_fence(std::memory_order_release);
        }
        return alloc;
    }

    void ReclaimCompletedEpoch(u64 completed_epoch) noexcept {
        u64 cur_read = read_cursor.load(std::memory_order_relaxed);
        const u64 cur_write = write_cursor.load(std::memory_order_acquire);

        while (cur_read < cur_write) {
            const size_t chunk_idx = (cur_read / CHUNK_SIZE) & (num_chunks - 1);
            const u64 chunk_epoch = chunk_epochs[chunk_idx].load(std::memory_order_acquire);
            if (chunk_epoch > completed_epoch) {
                break;
            }
            cur_read += CHUNK_SIZE;
        }
        read_cursor.store(cur_read, std::memory_order_release);

        if (overflow_active_count.load(std::memory_order_relaxed) > 0) {
            for (size_t i = 0; i < NUM_PREPOOLED_OVERFLOW; ++i) {
                auto& slab = overflow_slabs[i];
                if (slab.in_use.load(std::memory_order_relaxed)) {
                    if (slab.epoch.load(std::memory_order_acquire) <= completed_epoch) {
                        slab.in_use.store(false, std::memory_order_release);
                        overflow_active_count.fetch_sub(1, std::memory_order_relaxed);
                    }
                }
            }

            if (!dynamic_overflow_list.empty()) {
                std::scoped_lock lock{dynamic_overflow_mutex};
                auto it = dynamic_overflow_list.begin();
                while (it != dynamic_overflow_list.end()) {
                    if (it->epoch <= completed_epoch) {
                        storm_aligned_free(it->ptr);
                        it = dynamic_overflow_list.erase(it);
                        overflow_active_count.fetch_sub(1, std::memory_order_relaxed);
                    } else {
                        ++it;
                    }
                }
            }
        }
    }

    [[nodiscard]] size_t Capacity() const noexcept { return ring_capacity; }
    [[nodiscard]] u8* Data() noexcept { return ring_base; }
    [[nodiscard]] const u8* Data() const noexcept { return ring_base; }

    alignas(64) std::atomic<u64> write_cursor{0};
    alignas(64) std::atomic<u64> read_cursor{0};
    alignas(64) std::atomic<u64> committed_watermark{0};

private:
    struct ThreadTlab {
        const StormMpscTlabStagingRing* owner{nullptr};
        u64 chunk_cursor{0};
        size_t used_bytes{CHUNK_SIZE};
        u64 current_epoch{0};
    };

    struct OverflowSlab {
        u8* ptr{nullptr};
        size_t size{0};
        std::atomic<u64> epoch{0};
        std::atomic<bool> in_use{false};
    };

    struct DynamicOverflow {
        u8* ptr{nullptr};
        u64 epoch{0};
    };

    Allocation ClaimNewTlabChunk(ThreadTlab& tlab, size_t size, u64 epoch, size_t alignment) noexcept {
        const u64 cur_write = write_cursor.load(std::memory_order_relaxed);
        const u64 cur_read = read_cursor.load(std::memory_order_acquire);

        if (cur_write + CHUNK_SIZE - cur_read > ring_capacity) {
            return AllocateOverflow(size, epoch, alignment);
        }

        const u64 claimed_cursor = write_cursor.fetch_add(CHUNK_SIZE, std::memory_order_relaxed);
        if (claimed_cursor + CHUNK_SIZE - read_cursor.load(std::memory_order_acquire) > ring_capacity) {
            return AllocateOverflow(size, epoch, alignment);
        }

        const size_t chunk_idx = (claimed_cursor / CHUNK_SIZE) & (num_chunks - 1);
        chunk_epochs[chunk_idx].store(epoch, std::memory_order_release);

        tlab.owner = this;
        tlab.chunk_cursor = claimed_cursor;
        tlab.used_bytes = size;
        tlab.current_epoch = epoch;

        const size_t ring_offset = static_cast<size_t>(claimed_cursor & ring_mask);
        u8* ptr = ring_base + ring_offset;
        committed_watermark.store(claimed_cursor + size, std::memory_order_release);

        return Allocation{ptr, ring_offset, epoch, false};
    }

    Allocation AllocateLarge(size_t size, u64 epoch, size_t alignment) noexcept {
        const size_t alloc_bytes = (size + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1);
        const u64 cur_write = write_cursor.load(std::memory_order_relaxed);
        const u64 cur_read = read_cursor.load(std::memory_order_acquire);

        if (cur_write + alloc_bytes - cur_read > ring_capacity) {
            return AllocateOverflow(size, epoch, alignment);
        }

        const u64 claimed_cursor = write_cursor.fetch_add(alloc_bytes, std::memory_order_relaxed);
        if (claimed_cursor + alloc_bytes - read_cursor.load(std::memory_order_acquire) > ring_capacity) {
            return AllocateOverflow(size, epoch, alignment);
        }

        const size_t chunk_count = alloc_bytes / CHUNK_SIZE;
        for (size_t c = 0; c < chunk_count; ++c) {
            const size_t chunk_idx = ((claimed_cursor / CHUNK_SIZE) + c) & (num_chunks - 1);
            chunk_epochs[chunk_idx].store(epoch, std::memory_order_release);
        }

        const size_t ring_offset = static_cast<size_t>(claimed_cursor & ring_mask);
        u8* ptr = ring_base + ring_offset;
        committed_watermark.store(claimed_cursor + size, std::memory_order_release);

        return Allocation{ptr, ring_offset, epoch, false};
    }

    Allocation AllocateOverflow(size_t size, u64 epoch, size_t alignment) noexcept {
        for (size_t i = 0; i < NUM_PREPOOLED_OVERFLOW; ++i) {
            auto& slab = overflow_slabs[i];
            if (slab.size >= size) {
                bool expected = false;
                if (slab.in_use.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
                    slab.epoch.store(epoch, std::memory_order_release);
                    overflow_active_count.fetch_add(1, std::memory_order_relaxed);
                    return Allocation{slab.ptr, 0, epoch, true};
                }
            }
        }

        const size_t alloc_sz = (std::max)(size, CHUNK_SIZE);
        u8* dyn = static_cast<u8*>(storm_aligned_malloc(alloc_sz, 64));
        {
            std::scoped_lock lock{dynamic_overflow_mutex};
            dynamic_overflow_list.push_back({dyn, epoch});
            overflow_active_count.fetch_add(1, std::memory_order_relaxed);
        }
        return Allocation{dyn, 0, epoch, true};
    }

    size_t ring_capacity{0};
    size_t ring_mask{0};
    size_t num_chunks{0};
    u8* ring_base{nullptr};
    std::unique_ptr<std::atomic<u64>[]> chunk_epochs;

    std::array<OverflowSlab, NUM_PREPOOLED_OVERFLOW> overflow_slabs{};
    std::atomic<u32> overflow_active_count{0};
    std::mutex dynamic_overflow_mutex;
    std::vector<DynamicOverflow> dynamic_overflow_list;
};

/// 3-Level 64-ary Hierarchical Dirty Bitset for O(dirty_count) cacheline-granularity scanning
class StormHierarchicalDirtyBitset {
public:
    explicit StormHierarchicalDirtyBitset(size_t num_pages) : total_pages(num_pages) {
        subpage_masks.resize(total_pages, 0);
        const size_t l2_size = (total_pages + 63) / 64;
        l2_masks.resize(l2_size, 0);
        const size_t l1_size = (l2_size + 63) / 64;
        l1_masks.resize(l1_size, 0);
    }

    void MarkDirty(size_t page_idx, u64 subpage_64b_mask = ~0ULL) noexcept {
        if (page_idx >= total_pages) [[unlikely]] return;

        subpage_masks[page_idx] |= subpage_64b_mask;

        const size_t l2_idx = page_idx >> 6;
        const size_t l2_bit = page_idx & 63;
        const u64 l2_bitmask = 1ULL << l2_bit;

        if ((l2_masks[l2_idx] & l2_bitmask) == 0) {
            l2_masks[l2_idx] |= l2_bitmask;

            const size_t l1_idx = l2_idx >> 6;
            const size_t l1_bit = l2_idx & 63;
            const u64 l1_bitmask = 1ULL << l1_bit;

            if ((l1_masks[l1_idx] & l1_bitmask) == 0) {
                l1_masks[l1_idx] |= l1_bitmask;
                root_summary |= (1ULL << l1_idx);
            }
        }
    }

    template <typename Fn>
    void ForEachDirtyAndClear(Fn&& fn) noexcept {
        u64 root = root_summary;
        while (root != 0) {
            const unsigned l1_idx = std::countr_zero(root);
            u64& l1_val = l1_masks[l1_idx];
            u64 l1 = l1_val;

            while (l1 != 0) {
                const unsigned l1_bit = std::countr_zero(l1);
                const size_t l2_idx = (static_cast<size_t>(l1_idx) << 6) | l1_bit;
                u64& l2_val = l2_masks[l2_idx];
                u64 l2 = l2_val;

                while (l2 != 0) {
                    const unsigned l2_bit = std::countr_zero(l2);
                    const size_t page_idx = (l2_idx << 6) | l2_bit;

                    if (page_idx < total_pages) [[likely]] {
                        const u64 submask = subpage_masks[page_idx];
                        subpage_masks[page_idx] = 0;
                        fn(page_idx, submask);
                    }

                    l2 &= l2 - 1;
                }

                l2_val = 0;
                l1 &= l1 - 1;
            }

            l1_val = 0;
            root &= root - 1;
        }
        root_summary = 0;
    }

    [[nodiscard]] bool IsDirty(size_t page_idx) const noexcept {
        if (page_idx >= total_pages) return false;
        const size_t l2_idx = page_idx >> 6;
        const size_t l2_bit = page_idx & 63;
        return (l2_masks[l2_idx] & (1ULL << l2_bit)) != 0;
    }

    [[nodiscard]] u64 GetSubpageMask(size_t page_idx) const noexcept {
        if (page_idx >= total_pages) return 0;
        return subpage_masks[page_idx];
    }

    [[nodiscard]] size_t TotalPages() const noexcept { return total_pages; }

private:
    size_t total_pages{0};
    u64 root_summary{0};
    std::vector<u64> l1_masks;
    std::vector<u64> l2_masks;
    std::vector<u64> subpage_masks;
};

} // namespace Vulkan
