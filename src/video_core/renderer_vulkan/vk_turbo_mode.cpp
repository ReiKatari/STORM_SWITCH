// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

#ifdef __ANDROID__
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include <cstring>
#if defined(ARCHITECTURE_arm64)
#include <adrenotools/driver.h>
#endif
#endif

#include "common/literals.h"
#include "common/logging.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_turbo_mode.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

using namespace Common::Literals;

TurboMode::TurboMode(const vk::Instance& instance, const vk::InstanceDispatch& dld) {
    {
        std::scoped_lock lk{m_submission_lock};
        m_submission_time = std::chrono::steady_clock::now();
    }
    m_thread = std::jthread([&](auto stop_token) { Run(stop_token); });
}

TurboMode::~TurboMode() {
#ifdef _WIN32
    timeEndPeriod(1);
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
#if defined(__ANDROID__) && defined(ARCHITECTURE_arm64)
    adrenotools_set_turbo(false);
#endif
}

void TurboMode::QueueSubmitted() {
    std::scoped_lock lk{m_submission_lock};
    m_submission_time = std::chrono::steady_clock::now();
    m_submission_cv.notify_one();
}

void TurboMode::Run(std::stop_token stop_token) {
#ifdef _WIN32
    // Elevate process priority to High Priority for ultra-low latency scheduling
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Disable Windows 10/11 Power Throttling / Efficiency mode across all threads
    PROCESS_POWER_THROTTLING_STATE throttling{};
    throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
                             PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    throttling.StateMask = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &throttling,
                          sizeof(throttling));

    // Request 1ms high precision timer resolution
    timeBeginPeriod(1);

    // Prevent system idle throttling and sleep
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    LOG_INFO(Render_Vulkan, "STORM Turbo Mode enabled: High process priority and unthrottled execution active");
#elif defined(__ANDROID__)
#if defined(ARCHITECTURE_arm64)
    adrenotools_set_turbo(true);
#endif
    // Set Linux process priority to highest interactive priority (-20)
    setpriority(PRIO_PROCESS, 0, -20);

    // Multi-SoC support: Mali (Dimensity, Exynos, Tensor), devfreq, and CPU scaling governors
    static const char* const perf_nodes[] = {
        "/sys/devices/platform/mali.0/power_policy",
        "/sys/devices/platform/13000000.mali/power_policy",
        "/sys/devices/platform/13040000.mali/power_policy",
        "/sys/class/devfreq/mtk-dvfsrc-devfreq/governor",
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/13040000.mali/governor",
        "/sys/class/devfreq/mali.0/governor",
        "/sys/class/kgsl/kgsl-3d0/force_clk_on",
        "/sys/class/kgsl/kgsl-3d0/force_bus_on",
        "/sys/class/kgsl/kgsl-3d0/force_rail_on",
        "/sys/class/kgsl/kgsl-3d0/max_pwrlevel",
        "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor",
        "/sys/devices/system/cpu/cpufreq/policy4/scaling_governor",
        "/sys/devices/system/cpu/cpufreq/policy7/scaling_governor",
    };

    auto write_node = [](const char* path, const char* val) {
        int fd = open(path, O_WRONLY);
        if (fd >= 0) {
            write(fd, val, std::strlen(val));
            close(fd);
        }
    };

    for (const auto* node : perf_nodes) {
        if (std::strstr(node, "governor")) {
            write_node(node, "performance\n");
        } else if (std::strstr(node, "power_policy")) {
            write_node(node, "always_on\n");
        } else {
            write_node(node, "1\n");
        }
    }

    LOG_INFO(Render_Vulkan, "STORM Turbo Mode enabled: Universal mobile CPU/GPU performance governors active");
#endif

    while (!stop_token.stop_requested()) {
        std::unique_lock lk{m_submission_lock};
        m_submission_cv.wait_for(lk, stop_token, std::chrono::seconds{1}, [&] {
            return stop_token.stop_requested();
        });
    }
}

} // namespace Vulkan
