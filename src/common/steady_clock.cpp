// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#include "common/steady_clock.h"
#include "common/uint128.h"

namespace Common {

#ifdef _WIN32
static u64 WindowsQueryPerformanceFrequency() {
    LARGE_INTEGER frequency{};
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart > 0 ? static_cast<u64>(frequency.QuadPart) : 10'000'000ULL;
}

static u64 WindowsQueryPerformanceCounter() {
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return counter.QuadPart > 0 ? static_cast<u64>(counter.QuadPart) : 0ULL;
}

static s64 GetSystemTimeNS() {
    static auto pf = reinterpret_cast<decltype(&GetSystemTimePreciseAsFileTime)>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandle(TEXT("Kernel32.dll")),
                                               "GetSystemTimePreciseAsFileTime"))); // Windows 8+
    if (pf) {
        // GetSystemTimePreciseAsFileTime returns the file time in 100ns units.
        constexpr u64 Multiplier = 100;
        // Convert Windows epoch to Unix epoch.
        constexpr u64 WindowsEpochToUnixEpoch = 0x19DB1DED53E8000ULL;
        FILETIME filetime{};
        pf(&filetime);
        const u64 raw_ticks =
            (static_cast<u64>(filetime.dwHighDateTime) << 32) | static_cast<u64>(filetime.dwLowDateTime);
        return static_cast<s64>((raw_ticks - WindowsEpochToUnixEpoch) * Multiplier);
    } else {
        // Only Windows XP and below error out here
        LARGE_INTEGER ticks{};
        QueryPerformanceCounter(&ticks);
        return ticks.QuadPart;
    }
}
#endif

SteadyClock::time_point SteadyClock::Now() noexcept {
#if defined(_WIN32)
    static const auto timer_freq = WindowsQueryPerformanceFrequency();
    const auto counter = WindowsQueryPerformanceCounter();
    const auto whole = (counter / timer_freq) * static_cast<u64>(period::den);
    const auto part = MultiplyAndDivide64(counter % timer_freq, static_cast<u64>(period::den), timer_freq);
    return time_point{duration{static_cast<s64>(whole + part)}};
#elif defined(__APPLE__)
    return time_point{duration{clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW)}};
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return time_point{std::chrono::seconds{ts.tv_sec} + std::chrono::nanoseconds{ts.tv_nsec}};
#endif
}

RealTimeClock::time_point RealTimeClock::Now() noexcept {
#if defined(_WIN32)
    return time_point{duration{GetSystemTimeNS()}};
#elif defined(__APPLE__)
    return time_point{duration{clock_gettime_nsec_np(CLOCK_REALTIME)}};
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return time_point{std::chrono::seconds{ts.tv_sec} + std::chrono::nanoseconds{ts.tv_nsec}};
#endif
}

}; // namespace Common
