// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "common/adpf.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/service/vi/conductor.h"
#include "core/hle/service/vi/container.h"
#include "core/hle/service/vi/display_list.h"
#include "core/hle/service/vi/vsync_manager.h"

constexpr auto FrameNs = std::chrono::nanoseconds{1000000000 / 60};

constexpr s64 UNLOCKED_TARGET_DIVISOR = 4;

namespace Service::VI {

Conductor::Conductor(Core::System& system, Container& container, DisplayList& displays)
    : m_system(system), m_container(container) {
    displays.ForEachDisplay([&](Display& display) {
        m_vsync_managers.insert({display.GetId(), VsyncManager{}});
    });

    if (system.IsMulticore()) {
        m_event = Core::Timing::CreateEvent(
            "ScreenComposition",
            [this](s64 time,
                   std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
                m_signal.Set();
                return std::chrono::nanoseconds(this->GetNextTicks());
            });

        system.CoreTiming().ScheduleLoopingEvent(FrameNs, FrameNs, m_event);
        m_thread = std::jthread([this](std::stop_token token) { this->VsyncThread(token); });
    } else {
        m_event = Core::Timing::CreateEvent(
            "ScreenComposition",
            [this](s64 time,
                   std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
                this->ProcessVsync();
                return std::chrono::nanoseconds(this->GetNextTicks());
            });

        system.CoreTiming().ScheduleLoopingEvent(FrameNs, FrameNs, m_event);
    }
}

Conductor::~Conductor() {
    m_system.CoreTiming().UnscheduleEvent(m_event);

    if (m_system.IsMulticore()) {
        m_thread.request_stop();
        m_signal.Set();
    }
}

void Conductor::LinkVsyncEvent(u64 display_id, Event* event) {
    if (auto it = m_vsync_managers.find(display_id); it != m_vsync_managers.end()) {
        it->second.LinkVsyncEvent(event);
    }
}

void Conductor::UnlinkVsyncEvent(u64 display_id, Event* event) {
    if (auto it = m_vsync_managers.find(display_id); it != m_vsync_managers.end()) {
        it->second.UnlinkVsyncEvent(event);
    }
}

void Conductor::ProcessVsync() {
    Common::PollThreadPolicies();
    Common::ADPF::SetTargetWorkDuration(std::chrono::nanoseconds{this->GetFramePeriodNs()});

    for (auto& [display_id, manager] : m_vsync_managers) {
        m_container.ComposeOnDisplay(&m_swap_interval, &m_compose_speed_scale, display_id);
        manager.SignalVsync(m_system.Kernel());
    }
}

void Conductor::VsyncThread(std::stop_token token) {
    Common::SetCurrentThreadName("VSyncThread");
    Common::SetCurrentThreadPriority(Common::ThreadPriority::VeryHigh);
    Common::SetCurrentThreadToPerformanceCores();

    while (!token.stop_requested()) {
        m_signal.Wait();

        if (m_system.IsShuttingDown()) {
            return;
        }

        this->ProcessVsync();
    }
}

s64 Conductor::GetNextTicks() const {
    const auto& settings = Settings::values;
    auto speed_scale = 1.f;
    if (settings.use_multi_core.GetValue()) {
        if (settings.use_speed_limit.GetValue()) {
            // Scales the speed based on speed_limit setting on MC. SC is handled by
            // SpeedLimiter::DoSpeedLimiting.
            const auto speed_limit = Settings::SpeedLimit();
            speed_scale = speed_limit > 0 ? (100.f / static_cast<f32>(speed_limit)) : 1.f;
        } else {
            // Run at unlocked framerate.
            speed_scale = 0.01f;
        }
    }

    // Adjust by speed limit determined during composition.
    if (m_compose_speed_scale > 0.05f) {
        speed_scale /= m_compose_speed_scale;
    }

    if (m_system.GetNVDECActive() && settings.use_video_framerate.GetValue()) {
        // Run at intended presentation rate during video playback.
        speed_scale = 1.f;
    }

    const s32 safe_swap_interval = std::clamp(m_swap_interval, 1, 4);
    const f32 effective_fps = 60.f / static_cast<f32>(safe_swap_interval);
    const s64 ticks = static_cast<s64>(speed_scale * (1000000000.f / effective_fps));

    // Hard-clamp tick interval: never allow VSync composition to exceed 33.3ms (minimum 30 FPS compositing)
    // and lower bound to 1ms (up to 1000 FPS unlocked)
    return std::clamp<s64>(ticks, 1'000'000, 33'333'333);
}

s64 Conductor::GetFramePeriodNs() const {
    const auto& settings = Settings::values;
    f32 speed_scale = 1.f;
    bool unlocked = false;
    if (settings.use_multi_core.GetValue()) {
        if (settings.use_speed_limit.GetValue()) {
            const auto speed_limit = Settings::SpeedLimit();
            speed_scale = speed_limit > 0 ? (100.f / static_cast<f32>(speed_limit)) : 1.f;
        } else {
            unlocked = true;
        }
    }
    if (m_compose_speed_scale > 0.05f) {
        speed_scale /= m_compose_speed_scale;
    }

    const s32 safe_swap_interval = std::clamp(m_swap_interval, 1, 4);
    const f32 effective_fps = 60.f / static_cast<f32>(safe_swap_interval);
    s64 period = static_cast<s64>(speed_scale * (1000000000.f / effective_fps));
    if (unlocked) {
        period /= UNLOCKED_TARGET_DIVISOR;
    }
    return std::clamp<s64>(period, 1'000'000, 33'333'333);
}

} // namespace Service::VI
