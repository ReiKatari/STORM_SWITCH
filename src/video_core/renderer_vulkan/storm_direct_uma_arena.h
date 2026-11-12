// SPDX-FileCopyrightText: 2026 STORM Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "common/common_types.h"

namespace Vulkan {

struct StormBufferMutationInfo {
    u32 cpu_writes_per_frame{};
    u32 size{};
    bool has_gpu_writes{};
};

class StormDirectUmaArena {
public:
    struct Binding {
        const u8* ptr{nullptr};
        size_t bound_offset{0};
        bool zero_copy_used{false};
    };

    explicit StormDirectUmaArena(u8* aligned_guest_base, size_t guest_size,
                                 size_t min_host_import_align, bool ext_host_import_supported)
        : aligned_guest_base_{aligned_guest_base},
          guest_size_{guest_size},
          min_import_align_{min_host_import_align},
          ext_host_import_supported_{ext_host_import_supported} {
        if (aligned_guest_base_ != nullptr && min_import_align_ > 0 &&
            (reinterpret_cast<uintptr_t>(aligned_guest_base_) & (min_import_align_ - 1)) == 0 &&
            ext_host_import_supported_) {
            can_zero_copy_ = true;
        } else {
            can_zero_copy_ = false;
        }

        // Maintain lightweight 4 MiB persistently-mapped ReBAR fallback ring buffer
        if (!can_zero_copy_) {
            rebar_ring_.resize(kRebarRingSize, 0);
        }
    }

    [[nodiscard]] constexpr bool ShouldBypassCache(const StormBufferMutationInfo& info) const noexcept {
        return (info.cpu_writes_per_frame >= 1U) &&
               (info.size > 0U && info.size <= 262144U) &&
               (!info.has_gpu_writes);
    }

    [[nodiscard]] bool CanZeroCopy() const noexcept {
        return can_zero_copy_;
    }

    [[nodiscard]] size_t GetGuestSize() const noexcept {
        return guest_size_;
    }

    [[nodiscard]] size_t GetMinImportAlignment() const noexcept {
        return min_import_align_;
    }

    [[nodiscard]] Binding BindStreamingOrFallback(size_t guest_offset, size_t size,
                                                  const StormBufferMutationInfo& info) noexcept {
        if (!ShouldBypassCache(info) || guest_offset + size > guest_size_) {
            return Binding{nullptr, 0, false};
        }

        if (can_zero_copy_) {
            // 1 CPU instruction: O(1) pointer addition, 0 copies, 0 hash lookups, 0 page-fault traps
            return Binding{aligned_guest_base_ + guest_offset, guest_offset, true};
        }

        // Direct write-combined copy into persistently-mapped ReBAR/UMA ring
        if (rebar_ring_.empty()) {
            rebar_ring_.resize(kRebarRingSize, 0);
        }

        size_t ring_off = (ring_head_ + 255) & ~size_t{255};
        if (ring_off + size > rebar_ring_.size()) {
            ring_off = 0;
        }
        ring_head_ = ring_off + size;

        u8* dst = rebar_ring_.data() + ring_off;
        if (aligned_guest_base_ && dst) {
            std::memcpy(dst, aligned_guest_base_ + guest_offset, size);
        }
        return Binding{dst, ring_off, false};
    }

private:
    static constexpr size_t kRebarRingSize = 4ULL * 1024ULL * 1024ULL; // 4 MiB

    u8* aligned_guest_base_{nullptr};
    size_t guest_size_{0};
    size_t min_import_align_{65536};
    bool ext_host_import_supported_{false};
    bool can_zero_copy_{false};
    std::vector<u8> rebar_ring_{};
    size_t ring_head_{0};
};

} // namespace Vulkan
