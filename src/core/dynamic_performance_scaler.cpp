// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include "common/logging.h"
#include "common/settings.h"
#include "core/dynamic_performance_scaler.h"

namespace Core {

namespace {
std::atomic<float> s_effective_scale{1.0f};

float LevelToScaleMultiplier(Settings::ResolutionSetup level) noexcept {
    switch (level) {
    case Settings::ResolutionSetup::Res1_4X: return 0.25f;
    case Settings::ResolutionSetup::Res1_2X: return 0.50f;
    case Settings::ResolutionSetup::Res3_4X: return 0.75f;
    case Settings::ResolutionSetup::Res1X:   return 1.00f;
    case Settings::ResolutionSetup::Res5_4X: return 1.25f;
    case Settings::ResolutionSetup::Res3_2X: return 1.50f;
    case Settings::ResolutionSetup::Res2X:   return 2.00f;
    case Settings::ResolutionSetup::Res3X:   return 3.00f;
    case Settings::ResolutionSetup::Res4X:   return 4.00f;
    case Settings::ResolutionSetup::Res5X:   return 5.00f;
    case Settings::ResolutionSetup::Res6X:   return 6.00f;
    case Settings::ResolutionSetup::Res7X:   return 7.00f;
    case Settings::ResolutionSetup::Res8X:   return 8.00f;
    default: return 1.00f;
    }
}
} // Anonymous namespace

float DynamicPerformanceScaler::GetEffectiveScaleFactor() noexcept {
    if (!Settings::values.dynamic_performance_scaler.GetValue()) {
        const auto& ri = Settings::values.resolution_info;
        return static_cast<float>(ri.up_scale) / static_cast<float>(1U << ri.down_shift);
    }
    return s_effective_scale.load(std::memory_order_relaxed);
}

DynamicPerformanceScaler::DynamicPerformanceScaler() = default;

DynamicPerformanceScaler::~DynamicPerformanceScaler() = default;

int DynamicPerformanceScaler::LevelToIndex(Settings::ResolutionSetup level) {
    for (size_t i = 0; i < kNumLevels; ++i) {
        if (kLevelOrder[i] == level) {
            return static_cast<int>(i);
        }
    }
    // Fallback: native (1x)
    return 3;
}

void DynamicPerformanceScaler::Initialize(Settings::ResolutionSetup user_resolution,
                                          float target_fps) {
    if (target_fps <= 0.0f) {
        target_fps = 60.0f;
    }
    const double frame_time = 1000.0 / static_cast<double>(target_fps);
    target_frame_time_ms.store(frame_time, std::memory_order_relaxed);

    const int max_lvl = LevelToIndex(user_resolution);
    max_level_index.store(max_lvl, std::memory_order_relaxed);
    current_level_index.store(max_lvl, std::memory_order_relaxed);
    int min_lvl = 0; // Res1_4X absolute floor

    // Don't go below native (1x = index 3) unless already below it
    if (max_lvl >= 3) {
        min_lvl = 3; // floor at native 1x when user is at 1x or above
    }
    min_level_index.store(min_lvl, std::memory_order_relaxed);

    smoothed_frametime.store(frame_time, std::memory_order_relaxed);
    consecutive_overbudget = 0;
    consecutive_underbudget = 0;
    startup_frames = 0;
    s_effective_scale.store(LevelToScaleMultiplier(kLevelOrder[max_lvl]),
                            std::memory_order_relaxed);

    LOG_INFO(Core, "DynamicPerformanceScaler: Initialized — target {:.1f} FPS ({:.2f} ms), "
                   "max level index {} ({}x), min level index {}",
             target_fps, frame_time, max_lvl,
             static_cast<float>(Settings::values.resolution_info.up_scale) /
                 (1U << Settings::values.resolution_info.down_shift),
             min_lvl);
}

bool DynamicPerformanceScaler::ReportFrameTime(double frame_time_ms) {
    const double target_time = target_frame_time_ms.load(std::memory_order_relaxed);

    // Startup grace period: ignore warmup frames to allow disk loading / pipeline creation
    if (startup_frames < kStartupGraceFrames) {
        startup_frames++;
        smoothed_frametime.store(target_time, std::memory_order_relaxed);
        return false;
    }

    // Ignore single extreme hitches (> 100 ms) such as pipeline compiling or asset disk load
    if (frame_time_ms > 100.0) {
        return false;
    }

    // EMA smoothing (alpha = 0.1)
    const double prev_avg = smoothed_frametime.load(std::memory_order_relaxed);
    const double new_avg = (prev_avg * 0.9) + (frame_time_ms * 0.1);
    smoothed_frametime.store(new_avg, std::memory_order_relaxed);

    bool changed = false;

    const bool thermal_governor = Settings::values.storm_thermal_governor.GetValue();
    const bool eco_thermal = Settings::values.eco_thermal_mode.GetValue();
    const bool thermal_active = thermal_governor || eco_thermal;

    const double overbudget_margin = thermal_active ? 1.08 : kOverbudgetMargin;
    const u32 overbudget_threshold =
        thermal_active ? (std::max)(1U, kOverbudgetThreshold / 2) : kOverbudgetThreshold;
    const int min_lvl = min_level_index.load(std::memory_order_relaxed);
    const int effective_min_level =
        (eco_thermal && min_lvl >= 3) ? 2 : min_lvl;
    const int max_lvl = max_level_index.load(std::memory_order_relaxed);
    int current_lvl = current_level_index.load(std::memory_order_relaxed);

    if (new_avg > target_time * overbudget_margin) {
        // Frame is too slow
        consecutive_overbudget++;
        consecutive_underbudget = 0;

        if (consecutive_overbudget >= overbudget_threshold &&
            current_lvl > effective_min_level) {
            // Under load: record level drop without mutating resolution_setup mid-game
            // Mutating resolution_setup dynamically breaks compiled Maxwell shaders and allocated VkImage dimensions
            const int step = (eco_thermal && new_avg > target_time * 1.35) ? 2 : 1;
            current_lvl = (std::max)(effective_min_level, current_lvl - step);
            current_level_index.store(current_lvl, std::memory_order_relaxed);
            consecutive_overbudget = 0;
            changed = true;
            s_effective_scale.store(LevelToScaleMultiplier(kLevelOrder[current_lvl]),
                                    std::memory_order_relaxed);
            // Do not mutate global smart_shader_throttle setting from scaler

            LOG_DEBUG(Core,
                      "DynamicPerformanceScaler: Under load (avg {:.2f} ms > budget {:.2f} ms), "
                      "stepped down to level index {}",
                      new_avg, target_time, current_lvl);
        }
    } else if (new_avg < target_time * kUnderbudgetMargin) {
        // Frame is fast — room to increase quality
        consecutive_underbudget++;
        consecutive_overbudget = 0;

        if (consecutive_underbudget >= kUnderbudgetThreshold &&
            current_lvl < max_lvl) {
            current_lvl++;
            current_level_index.store(current_lvl, std::memory_order_relaxed);
            consecutive_underbudget = 0;
            changed = true;
            s_effective_scale.store(LevelToScaleMultiplier(kLevelOrder[current_lvl]),
                                    std::memory_order_relaxed);

            LOG_DEBUG(Core,
                      "DynamicPerformanceScaler: Frame stable (avg {:.2f} ms < {:.2f} ms), "
                      "stepped up to level index {}",
                      new_avg, target_time * kUnderbudgetMargin, current_lvl);
        }
    } else {
        // Within budget — reset both counters
        consecutive_overbudget = 0;
        consecutive_underbudget = 0;
    }

    return changed;
}

Settings::ResolutionSetup DynamicPerformanceScaler::GetCurrentLevel() const {
    const int idx = std::clamp(current_level_index.load(std::memory_order_relaxed), 0, static_cast<int>(kNumLevels - 1));
    return kLevelOrder[idx];
}

Settings::ResolutionSetup DynamicPerformanceScaler::GetMaxLevel() const {
    const int idx = std::clamp(max_level_index.load(std::memory_order_relaxed), 0, static_cast<int>(kNumLevels - 1));
    return kLevelOrder[idx];
}

double DynamicPerformanceScaler::GetSmoothedFrameTime() const {
    return smoothed_frametime.load(std::memory_order_relaxed);
}

double DynamicPerformanceScaler::GetTargetFrameTime() const {
    return target_frame_time_ms.load(std::memory_order_relaxed);
}

bool DynamicPerformanceScaler::IsScalingDown() const {
    return current_level_index.load(std::memory_order_relaxed) < max_level_index.load(std::memory_order_relaxed);
}

void DynamicPerformanceScaler::Reset() {
    const int max_lvl = max_level_index.load(std::memory_order_relaxed);
    current_level_index.store(max_lvl, std::memory_order_relaxed);
    consecutive_overbudget = 0;
    consecutive_underbudget = 0;
    startup_frames = 0;
    const double target_time = target_frame_time_ms.load(std::memory_order_relaxed);
    smoothed_frametime.store(target_time, std::memory_order_relaxed);

    // Restore user's resolution
    const auto user_level = kLevelOrder[max_lvl];
    s_effective_scale.store(LevelToScaleMultiplier(user_level), std::memory_order_relaxed);
    Settings::values.resolution_setup.SetValue(user_level);
    Settings::UpdateRescalingInfo();
}

} // namespace Core
