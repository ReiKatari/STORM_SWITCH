// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include "common/logging.h"
#include "common/settings.h"
#include "core/dynamic_performance_scaler.h"

namespace Core {

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
    std::lock_guard lock(mutex_);

    if (target_fps <= 0.0f) {
        target_fps = 60.0f;
    }
    target_frame_time_ms = 1000.0 / static_cast<double>(target_fps);

    max_level_index = LevelToIndex(user_resolution);
    current_level_index = max_level_index;
    min_level_index = 0; // Res1_4X absolute floor

    // Don't go below native (1x = index 3) unless already below it
    if (max_level_index >= 3) {
        min_level_index = 3; // floor at native 1x when user is at 1x or above
    }

    smoothed_frametime.store(target_frame_time_ms);
    consecutive_overbudget = 0;
    consecutive_underbudget = 0;

    LOG_INFO(Core, "DynamicPerformanceScaler: Initialized — target {:.1f} FPS ({:.2f} ms), "
                   "max level index {} ({}x), min level index {}",
             target_fps, target_frame_time_ms, max_level_index,
             static_cast<float>(Settings::values.resolution_info.up_scale) /
                 (1U << Settings::values.resolution_info.down_shift),
             min_level_index);
}

bool DynamicPerformanceScaler::ReportFrameTime(double frame_time_ms) {
    std::lock_guard lock(mutex_);

    // EMA smoothing (alpha = 0.1)
    const double prev_avg = smoothed_frametime.load(std::memory_order_relaxed);
    const double new_avg = (prev_avg * 0.9) + (frame_time_ms * 0.1);
    smoothed_frametime.store(new_avg, std::memory_order_relaxed);

    bool changed = false;

    if (new_avg > target_frame_time_ms * kOverbudgetMargin) {
        // Frame is too slow
        consecutive_overbudget++;
        consecutive_underbudget = 0;

        if (consecutive_overbudget >= kOverbudgetThreshold &&
            current_level_index > min_level_index) {
            // Scale DOWN one step
            current_level_index--;
            consecutive_overbudget = 0;

            // Apply to settings
            const auto new_level = kLevelOrder[current_level_index];
            Settings::values.resolution_setup.SetValue(new_level);
            Settings::UpdateRescalingInfo();
            changed = true;

            LOG_INFO(Core,
                     "DynamicPerformanceScaler: Scale DOWN to level {} "
                     "(avg {:.2f} ms > budget {:.2f} ms)",
                     current_level_index, new_avg, target_frame_time_ms);
        }
    } else if (new_avg < target_frame_time_ms * kUnderbudgetMargin) {
        // Frame is fast — room to increase quality
        consecutive_underbudget++;
        consecutive_overbudget = 0;

        if (consecutive_underbudget >= kUnderbudgetThreshold &&
            current_level_index < max_level_index) {
            // Scale UP one step
            current_level_index++;
            consecutive_underbudget = 0;

            // Apply to settings
            const auto new_level = kLevelOrder[current_level_index];
            Settings::values.resolution_setup.SetValue(new_level);
            Settings::UpdateRescalingInfo();
            changed = true;

            LOG_INFO(Core,
                     "DynamicPerformanceScaler: Scale UP to level {} "
                     "(avg {:.2f} ms < {:.2f} ms)",
                     current_level_index, new_avg,
                     target_frame_time_ms * kUnderbudgetMargin);
        }
    } else {
        // Within budget — reset both counters
        consecutive_overbudget = 0;
        consecutive_underbudget = 0;
    }

    return changed;
}

Settings::ResolutionSetup DynamicPerformanceScaler::GetCurrentLevel() const {
    std::lock_guard lock(mutex_);
    return kLevelOrder[current_level_index];
}

Settings::ResolutionSetup DynamicPerformanceScaler::GetMaxLevel() const {
    std::lock_guard lock(mutex_);
    return kLevelOrder[max_level_index];
}

double DynamicPerformanceScaler::GetSmoothedFrameTime() const {
    return smoothed_frametime.load(std::memory_order_relaxed);
}

double DynamicPerformanceScaler::GetTargetFrameTime() const {
    std::lock_guard lock(mutex_);
    return target_frame_time_ms;
}

bool DynamicPerformanceScaler::IsScalingDown() const {
    std::lock_guard lock(mutex_);
    return current_level_index < max_level_index;
}

void DynamicPerformanceScaler::Reset() {
    std::lock_guard lock(mutex_);
    current_level_index = max_level_index;
    consecutive_overbudget = 0;
    consecutive_underbudget = 0;
    smoothed_frametime.store(target_frame_time_ms, std::memory_order_relaxed);

    // Restore user's resolution
    const auto user_level = kLevelOrder[max_level_index];
    Settings::values.resolution_setup.SetValue(user_level);
    Settings::UpdateRescalingInfo();
}

} // namespace Core
