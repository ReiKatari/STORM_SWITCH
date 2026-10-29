// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "video_core/renderer_vulkan/present/fsr2.h"
#include "common/settings.h"

namespace Vulkan::FSR2 {

QualityConfig GetQualityConfig(Settings::Fsr2QualityMode mode) {
    switch (mode) {
    case Settings::Fsr2QualityMode::Quality:
        return QualityConfig{
            .scale_factor = 1.5f,
            .jitter_phase_count = 16,
            .sharpness_bias = 0.0f,
        };
    case Settings::Fsr2QualityMode::Balanced:
        return QualityConfig{
            .scale_factor = 1.7f,
            .jitter_phase_count = 20,
            .sharpness_bias = 0.1f,
        };
    case Settings::Fsr2QualityMode::Performance:
        return QualityConfig{
            .scale_factor = 2.0f,
            .jitter_phase_count = 24,
            .sharpness_bias = 0.2f,
        };
    case Settings::Fsr2QualityMode::UltraPerformance:
        return QualityConfig{
            .scale_factor = 3.0f,
            .jitter_phase_count = 32,
            .sharpness_bias = 0.3f,
        };
    default:
        return QualityConfig{
            .scale_factor = 1.5f,
            .jitter_phase_count = 16,
            .sharpness_bias = 0.0f,
        };
    }
}

f32 GetScaleFactor(Settings::Fsr2QualityMode mode) {
    return GetQualityConfig(mode).scale_factor;
}

u32 GetJitterPhaseCount(Settings::Fsr2QualityMode mode) {
    return GetQualityConfig(mode).jitter_phase_count;
}

} // namespace Vulkan::FSR2
