// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "common/common_types.h"
#include "common/settings_enums.h"

namespace Core {

/// Dynamic Performance Scaler — Atmosphere-style adaptive resolution scaling.
///
/// Monitors per-frame GPU/CPU timing and adjusts the emulator's internal
/// resolution on the fly to maintain a stable target framerate.
///
/// The scaler operates at frame boundaries (called from the renderer's
/// Composite path) and modifies Settings::values.resolution_setup +
/// resolution_info so that downstream rendering picks up the change
/// immediately on the next frame.
///
/// Design principles:
///  • Hysteresis: scale down quickly (5 overbudget frames), recover slowly
///    (30 underbudget frames) to avoid oscillation.
///  • EMA smoothing: frame-time spikes are dampened via an exponential
///    moving average (alpha = 0.1).
///  • Discrete levels: uses the existing ResolutionSetup enum steps,
///    ordered from lowest to highest quality.
///  • Thread-safe: all mutable state is atomic or mutex-protected.
class DynamicPerformanceScaler final {
public:
    DynamicPerformanceScaler();
    ~DynamicPerformanceScaler();

    /// Initialise (or re-initialise) the scaler.
    /// @param user_resolution  The resolution the user has set in settings
    ///                         (used as the ceiling — we never scale above
    ///                         what the user explicitly chose).
    /// @param target_fps       Target framerate derived from frame_pacing_mode.
    void Initialize(Settings::ResolutionSetup user_resolution, float target_fps);

    /// Call once per frame with the most recent frame time in milliseconds.
    /// Returns true if the resolution was changed (caller should call
    /// Settings::UpdateRescalingInfo()).
    bool ReportFrameTime(double frame_time_ms);

    /// Returns the current effective ResolutionSetup level.
    [[nodiscard]] Settings::ResolutionSetup GetCurrentLevel() const;

    /// Returns the user-configured maximum level.
    [[nodiscard]] Settings::ResolutionSetup GetMaxLevel() const;

    /// Returns the smoothed frame time in ms.
    [[nodiscard]] double GetSmoothedFrameTime() const;

    /// Returns the target frame time in ms.
    [[nodiscard]] double GetTargetFrameTime() const;

    /// Returns true when the scaler is active and has reduced resolution
    /// below the user's setting.
    [[nodiscard]] bool IsScalingDown() const;

    /// Reset state without changing target/max level.
    void Reset();

private:
    /// Ordered list of resolution levels from lowest to highest quality.
    static constexpr Settings::ResolutionSetup kLevelOrder[] = {
        Settings::ResolutionSetup::Res1_4X,  // 0.25x — emergency floor
        Settings::ResolutionSetup::Res1_2X,  // 0.50x
        Settings::ResolutionSetup::Res3_4X,  // 0.75x
        Settings::ResolutionSetup::Res1X,    // 1.00x (native)
        Settings::ResolutionSetup::Res5_4X,  // 1.25x
        Settings::ResolutionSetup::Res3_2X,  // 1.50x
        Settings::ResolutionSetup::Res2X,    // 2.00x
        Settings::ResolutionSetup::Res3X,    // 3.00x
        Settings::ResolutionSetup::Res4X,    // 4.00x
        Settings::ResolutionSetup::Res5X,    // 5.00x
        Settings::ResolutionSetup::Res6X,    // 6.00x
        Settings::ResolutionSetup::Res7X,    // 7.00x
        Settings::ResolutionSetup::Res8X,    // 8.00x
    };
    static constexpr size_t kNumLevels = std::size(kLevelOrder);

    /// Map a ResolutionSetup value to its index in kLevelOrder.
    static int LevelToIndex(Settings::ResolutionSetup level);

    // --- State ---
    std::atomic<double> smoothed_frametime{16.666};
    double target_frame_time_ms{16.666};

    int current_level_index{3};  // default = Res1X
    int max_level_index{3};      // user ceiling
    int min_level_index{0};      // absolute floor (Res1_4X)

    u32 consecutive_overbudget{0};
    u32 consecutive_underbudget{0};

    // Thresholds (tunable)
    static constexpr u32 kOverbudgetThreshold = 5;   // frames before scale-down
    static constexpr u32 kUnderbudgetThreshold = 30;  // frames before scale-up
    static constexpr double kOverbudgetMargin = 1.10; // 10% over target = overbudget
    static constexpr double kUnderbudgetMargin = 0.85; // 15% under target = underbudget

    mutable std::mutex mutex_;
};

} // namespace Core
