// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/settings_enums.h"

namespace Vulkan::FSR2 {

struct QualityConfig {
    f32 scale_factor;
    u32 jitter_phase_count;
    f32 sharpness_bias;
};

[[nodiscard]] QualityConfig GetQualityConfig(Settings::Fsr2QualityMode mode);
[[nodiscard]] f32 GetScaleFactor(Settings::Fsr2QualityMode mode);
[[nodiscard]] u32 GetJitterPhaseCount(Settings::Fsr2QualityMode mode);

} // namespace Vulkan::FSR2
