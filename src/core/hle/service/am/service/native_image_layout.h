// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <optional>
#include <span>
#include "common/common_types.h"

namespace Service::AM {

struct NativeImageRequirements {
    u64 size;
    u64 alignment;
};

inline std::optional<NativeImageRequirements> CalculateNativeImageRequirements(
    std::span<const u32> preference, u32 count) {
    if (preference.size() < 16) {
        return std::nullopt;
    }
    const u32 eff_count = (count > 1U ? count : 1U);
    const u64 width = (preference[2] & 0xffffU) + 1;
    const u64 height = (preference[2] >> 16) + 1;
    const u32 bh_log2 = preference[4] <= 8 ? preference[4] : 4U;
    const u64 block_height = 1ULL << bh_log2;
    u64 pitch_gobs = preference[5];
    const u64 min_pitch_gobs = (width * 4 + 63) / 64;
    if (pitch_gobs < min_pitch_gobs || pitch_gobs > 4096) {
        pitch_gobs = (min_pitch_gobs > 1ULL ? min_pitch_gobs : 1ULL);
    }
    const u64 aligned_height = (height + block_height - 1) & ~(block_height - 1);
    const u64 raw_size = pitch_gobs * 64 * aligned_height;
    const u64 unit_size = (raw_size > 0x1000ULL ? raw_size : 0x1000ULL);
    const u64 raw_align = 64 * block_height;
    const u64 alignment = (raw_align > 0x1000ULL ? raw_align : 0x1000ULL);
    return NativeImageRequirements{unit_size * eff_count, alignment};
}

} // namespace Service::AM
