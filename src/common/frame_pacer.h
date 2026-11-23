// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <thread>

namespace Common {

/// Stutter-free monotonic frame pacer (inspired by totk-mobile core/gfx/nvn/nvn_pace.h).
/// - Early frame: sleeps until the scheduled deadline and advances the deadline by one period.
/// - Slightly late (< 1 period): presents immediately while keeping cadence phase intact.
/// - Heavily late (>= 1 period, e.g. shader compile or loading spike): resets cadence to `now`
///   so the emulator never emits a burst of fast-forward "catch-up" frames after a lag spike.
class FramePacer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Nanoseconds = std::chrono::nanoseconds;

    void Reset() noexcept {
        m_initialized = false;
        m_next_deadline = {};
    }

    [[nodiscard]] Nanoseconds ComputeWait(TimePoint now, Nanoseconds period) noexcept {
        if (period <= Nanoseconds::zero()) {
            Reset();
            return Nanoseconds::zero();
        }
        if (!m_initialized) {
            m_initialized = true;
            m_next_deadline = now + period;
            return Nanoseconds::zero();
        }
        if (now < m_next_deadline) {
            const Nanoseconds wait = m_next_deadline - now;
            // Guard against huge waits if target FPS suddenly drops or pauses.
            const Nanoseconds max_wait = period * 2;
            if (wait > max_wait) {
                m_next_deadline = now + period;
                return period;
            }
            m_next_deadline += period;
            return wait;
        }
        const Nanoseconds late = now - m_next_deadline;
        if (late < period) {
            // Minor jitter: present immediately and keep phase locked to the original grid.
            m_next_deadline += period;
        } else {
            // Load spike / stutter >= 1 full period: drop banked debt so there is no catch-up burst.
            m_next_deadline = now + period;
        }
        return Nanoseconds::zero();
    }

    void Pace(Nanoseconds period) {
        const TimePoint now = Clock::now();
        const Nanoseconds wait = ComputeWait(now, period);
        if (wait > Nanoseconds::zero()) {
            std::this_thread::sleep_until(now + wait);
        }
    }

private:
    bool m_initialized{false};
    TimePoint m_next_deadline{};
};

} // namespace Common
