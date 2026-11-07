// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include "common/common_types.h"
#include "common/literals.h"

namespace VideoCommon {
struct TextureCacheMemoryThresholds {
    u64 expected, critical, minimum;
};

constexpr TextureCacheMemoryThresholds ClampTextureCacheThresholds(
    TextureCacheMemoryThresholds limits, bool budget_driven, u64 legacy_ceiling) {
    using namespace Common::Literals;
    if (!budget_driven) {
        limits.critical = (std::min<u64>)(limits.critical, legacy_ceiling);
    }
    limits.expected =
        (std::min<u64>)(limits.expected, limits.critical - (std::min<u64>)(limits.critical, u64{768_MiB}));
    limits.minimum =
        (std::min<u64>)(limits.minimum, limits.expected - (std::min<u64>)(limits.expected, u64{1_GiB}));
    return limits;
}
} // namespace VideoCommon
