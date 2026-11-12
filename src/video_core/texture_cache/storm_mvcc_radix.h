// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "common/common_types.h"

namespace VideoCommon {

/**
 * Candidate image metadata for causal MVCC framebuffer resolution.
 */
struct StormMvccCandidate {
    u32 image_id{};
    u64 cpu_addr{};
    u32 width{};
    u32 height{};
    u32 format_class{};
    u64 cpu_tick{};
    u64 gpu_tick{};
    bool gpu_modified{};
    bool cpu_modified{};
};

/**
 * Select the causally newest framebuffer among candidate images.
 * Enforces exact cpu_addr match (rejecting sub-page offset mismatches), format compatibility,
 * and valid DRS / dynamic resolution aspect ratio match.
 */
inline u32 SelectCausalFramebuffer(const StormMvccCandidate* cands, size_t count,
                                   u64 target_cpu_addr, u32 width, u32 height,
                                   u32 format_class) noexcept {
    if (!cands || count == 0) {
        return 0;
    }

    u32 best_gpu_id = 0;
    u64 best_gpu_tick = 0;

    u32 best_cpu_id = 0;
    u64 best_cpu_tick = 0;

    for (size_t i = 0; i < count; ++i) {
        const auto& c = cands[i];
        if (c.cpu_addr != target_cpu_addr) {
            continue;
        }
        if (format_class != 0 && c.format_class != 0 && c.format_class != format_class) {
            continue;
        }
        if (width != 0 && height != 0 && c.width != 0 && c.height != 0) {
            const bool exact_match = (c.width == width && c.height == height);
            const bool drs_match = (static_cast<u64>(c.width) * height == static_cast<u64>(c.height) * width);
            if (!exact_match && !drs_match) {
                continue;
            }
        }

        if (c.gpu_modified && c.gpu_tick > 0) {
            if (c.gpu_tick > best_gpu_tick || best_gpu_id == 0) {
                best_gpu_tick = c.gpu_tick;
                best_gpu_id = c.image_id;
            }
        } else if (c.cpu_modified) {
            if (c.cpu_tick > best_cpu_tick || best_cpu_id == 0) {
                best_cpu_tick = c.cpu_tick;
                best_cpu_id = c.image_id;
            }
        }
    }

    if (best_gpu_id != 0) {
        return best_gpu_id;
    }
    if (best_cpu_id != 0) {
        return best_cpu_id;
    }
    return 0;
}

/**
 * Compact 16-byte leaf entry supporting up to 2 direct image IDs and an overflow pool index.
 */
struct StormLeafEntry {
    u16 count{0};
    u16 flags{0};
    u32 image_ids[2]{0, 0};
    u32 overflow_head_idx{0};
};

/**
 * Overflow slot in contiguous slab pool.
 */
struct StormOverflowSlot {
    u32 image_id{0};
    u32 next_idx{0};
};

/**
 * Mid-level radix node containing 512 32-bit slab indices to LeafNodes (2 KiB total).
 */
struct StormMidNode {
    std::array<u32, 512> leaf_indices{};
};

/**
 * Leaf radix node containing 512 compact 16-byte LeafEntries (8 KiB total).
 */
struct StormLeafNode {
    std::array<StormLeafEntry, 512> entries{};
};

/**
 * 40-bit 3-level slab-pooled Radix Directory.
 * Address mapping: 10 bits L1 (Root) + 9 bits L2 (MidNode) + 9 bits L3 (LeafNode) + 12 bits page offset = 40 bits.
 * Uses contiguous vector slab pools with 32-bit indices to eliminate pointer overhead and reduce allocation count.
 */
class StormMvccRadixDirectory {
public:
    StormMvccRadixDirectory() {
        root.fill(0);
        mid_nodes.resize(1);
        leaf_nodes.resize(1);
        overflow_pool.resize(1);
    }

    StormMvccRadixDirectory(StormMvccRadixDirectory&&) noexcept = default;
    StormMvccRadixDirectory& operator=(StormMvccRadixDirectory&&) noexcept = default;
    StormMvccRadixDirectory(const StormMvccRadixDirectory&) = default;
    StormMvccRadixDirectory& operator=(const StormMvccRadixDirectory&) = default;

    void RegisterPage(u64 addr, u32 image_id) {
        const size_t l1 = (addr >> 30) & 0x3FF;
        u32 mid_idx = root[l1];
        if (mid_idx == 0) {
            mid_idx = static_cast<u32>(mid_nodes.size());
            mid_nodes.emplace_back();
            root[l1] = mid_idx;
        }

        const size_t l2 = (addr >> 21) & 0x1FF;
        u32 leaf_idx = mid_nodes[mid_idx].leaf_indices[l2];
        if (leaf_idx == 0) {
            leaf_idx = static_cast<u32>(leaf_nodes.size());
            leaf_nodes.emplace_back();
            mid_nodes[mid_idx].leaf_indices[l2] = leaf_idx;
        }

        const size_t l3 = (addr >> 12) & 0x1FF;
        auto& entry = leaf_nodes[leaf_idx].entries[l3];

        if (entry.count == 1) {
            if (entry.image_ids[0] == image_id) {
                return;
            }
        } else if (entry.count >= 2) {
            if (entry.image_ids[0] == image_id || entry.image_ids[1] == image_id) {
                return;
            }
            u32 curr = entry.overflow_head_idx;
            while (curr != 0 && curr < overflow_pool.size()) {
                if (overflow_pool[curr].image_id == image_id) {
                    return;
                }
                curr = overflow_pool[curr].next_idx;
            }
        }

        if (entry.count == 0) {
            entry.image_ids[0] = image_id;
            entry.count = 1;
        } else if (entry.count == 1) {
            entry.image_ids[1] = image_id;
            entry.count = 2;
        } else {
            const u32 new_slot = static_cast<u32>(overflow_pool.size());
            overflow_pool.push_back(StormOverflowSlot{image_id, entry.overflow_head_idx});
            entry.overflow_head_idx = new_slot;
            entry.count++;
        }
    }

    void UnregisterPage(u64 addr, u32 image_id) {
        const size_t l1 = (addr >> 30) & 0x3FF;
        const u32 mid_idx = root[l1];
        if (mid_idx == 0 || mid_idx >= mid_nodes.size()) {
            return;
        }

        const size_t l2 = (addr >> 21) & 0x1FF;
        const u32 leaf_idx = mid_nodes[mid_idx].leaf_indices[l2];
        if (leaf_idx == 0 || leaf_idx >= leaf_nodes.size()) {
            return;
        }

        const size_t l3 = (addr >> 12) & 0x1FF;
        auto& entry = leaf_nodes[leaf_idx].entries[l3];
        if (entry.count == 0) {
            return;
        }

        if (entry.count == 1) {
            if (entry.image_ids[0] == image_id) {
                entry.image_ids[0] = 0;
                entry.count = 0;
            }
            return;
        }

        if (entry.count == 2) {
            if (entry.image_ids[0] == image_id) {
                entry.image_ids[0] = entry.image_ids[1];
                entry.image_ids[1] = 0;
                entry.count = 1;
            } else if (entry.image_ids[1] == image_id) {
                entry.image_ids[1] = 0;
                entry.count = 1;
            }
            return;
        }

        // Swap with tail compaction from overflow_head_idx (zero tombstones)
        const u32 head_slot_idx = entry.overflow_head_idx;
        if (head_slot_idx == 0 || head_slot_idx >= overflow_pool.size()) {
            entry.count = 2;
            return;
        }

        const u32 replacement_id = overflow_pool[head_slot_idx].image_id;
        entry.overflow_head_idx = overflow_pool[head_slot_idx].next_idx;
        entry.count--;

        if (entry.image_ids[0] == image_id) {
            entry.image_ids[0] = replacement_id;
            return;
        }
        if (entry.image_ids[1] == image_id) {
            entry.image_ids[1] = replacement_id;
            return;
        }
        if (replacement_id == image_id) {
            return;
        }

        u32 curr = entry.overflow_head_idx;
        while (curr != 0 && curr < overflow_pool.size()) {
            if (overflow_pool[curr].image_id == image_id) {
                overflow_pool[curr].image_id = replacement_id;
                return;
            }
            curr = overflow_pool[curr].next_idx;
        }

        // Target not found in chain, revert popped head
        overflow_pool[head_slot_idx].image_id = replacement_id;
        overflow_pool[head_slot_idx].next_idx = entry.overflow_head_idx;
        entry.overflow_head_idx = head_slot_idx;
        entry.count++;
    }

    size_t CountOnPage(u64 addr) const noexcept {
        const size_t l1 = (addr >> 30) & 0x3FF;
        const u32 mid_idx = root[l1];
        if (mid_idx == 0 || mid_idx >= mid_nodes.size()) {
            return 0;
        }

        const size_t l2 = (addr >> 21) & 0x1FF;
        const u32 leaf_idx = mid_nodes[mid_idx].leaf_indices[l2];
        if (leaf_idx == 0 || leaf_idx >= leaf_nodes.size()) {
            return 0;
        }

        const size_t l3 = (addr >> 12) & 0x1FF;
        return leaf_nodes[leaf_idx].entries[l3].count;
    }

    template <typename Fn>
    void ForEachOnPage(u64 addr, Fn&& fn) const {
        const size_t l1 = (addr >> 30) & 0x3FF;
        const u32 mid_idx = root[l1];
        if (mid_idx == 0 || mid_idx >= mid_nodes.size()) {
            return;
        }

        const size_t l2 = (addr >> 21) & 0x1FF;
        const u32 leaf_idx = mid_nodes[mid_idx].leaf_indices[l2];
        if (leaf_idx == 0 || leaf_idx >= leaf_nodes.size()) {
            return;
        }

        const size_t l3 = (addr >> 12) & 0x1FF;
        const auto& entry = leaf_nodes[leaf_idx].entries[l3];
        if (entry.count == 0) {
            return;
        }

        fn(entry.image_ids[0]);
        if (entry.count == 1) {
            return;
        }

        fn(entry.image_ids[1]);
        if (entry.count == 2) {
            return;
        }

        u32 curr = entry.overflow_head_idx;
        while (curr != 0 && curr < overflow_pool.size()) {
            fn(overflow_pool[curr].image_id);
            curr = overflow_pool[curr].next_idx;
        }
    }

private:
    std::array<u32, 1024> root{};
    std::vector<StormMidNode> mid_nodes;
    std::vector<StormLeafNode> leaf_nodes;
    std::vector<StormOverflowSlot> overflow_pool;
};

} // namespace VideoCommon
