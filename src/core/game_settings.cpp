// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/game_settings.h"

#include <algorithm>
#include <cctype>

#include "common/logging.h"
#include "common/settings.h"
#include "video_core/renderer_base.h"
#include "core/hle/service/game_fix_database.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace Core::GameSettings {

static GPUVendor GetGPU(const std::string& gpu_vendor_string) {
    struct Entry { const char* name; GPUVendor vendor; };
    static constexpr Entry GpuVendor[] = {
        // NVIDIA
        {"NVIDIA",   GPUVendor::Nvidia},
        {"Nouveau",  GPUVendor::Nvidia},
        {"NVK",      GPUVendor::Nvidia},
        {"Tegra",    GPUVendor::Nvidia},
        // AMD
        {"AMD",       GPUVendor::AMD},
        {"RadeonSI",  GPUVendor::AMD},
        {"RADV",      GPUVendor::AMD},
        {"AMDVLK",    GPUVendor::AMD},
        {"R600",      GPUVendor::AMD},
        // Intel
        {"Intel",     GPUVendor::Intel},
        {"ANV",       GPUVendor::Intel},
        {"i965",      GPUVendor::Intel},
        {"i915",      GPUVendor::Intel},
        {"OpenSWR",   GPUVendor::Intel},
        // Apple
        {"Apple",     GPUVendor::Apple},
        {"MoltenVK",  GPUVendor::Apple},
        // Qualcomm / Adreno
        {"Qualcomm",  GPUVendor::Qualcomm},
        {"Turnip",    GPUVendor::Qualcomm},
        // ARM / Mali
        {"Mali",      GPUVendor::ARM},
        {"PanVK",     GPUVendor::ARM},
        // Imagination / PowerVR
        {"PowerVR",   GPUVendor::Imagination},
        {"PVR",       GPUVendor::Imagination},
        // Microsoft / WARP / D3D12 GL
        {"D3D12",     GPUVendor::Microsoft},
        {"Microsoft", GPUVendor::Microsoft},
        {"WARP",      GPUVendor::Microsoft},
    };

    for (const auto& entry : GpuVendor) {
        if (gpu_vendor_string == entry.name) {
            return entry.vendor;
        }
    }

    // legacy (shouldn't be needed anymore, but just in case)
    std::string gpu = gpu_vendor_string;
    std::transform(gpu.begin(), gpu.end(), gpu.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    if (gpu.find("geforce") != std::string::npos) {
        return GPUVendor::Nvidia;
    }
    if (gpu.find("radeon") != std::string::npos || gpu.find("ati") != std::string::npos) {
        return GPUVendor::AMD;
    }

    return GPUVendor::Unknown;
}

static OS DetectOS() {
#if defined(_WIN32)
    return OS::Windows;
#elif defined(__FIREOS__)
    return OS::FireOS;
#elif defined(__ANDROID__)
    return OS::Android;
#elif defined(__OHOS__)
    return OS::HarmonyOS;
#elif defined(__HAIKU__)
    return OS::HaikuOS;
#elif defined(__DragonFly__)
    return OS::DragonFlyBSD;
#elif defined(__NetBSD__)
    return OS::NetBSD;
#elif defined(__OpenBSD__)
    return OS::OpenBSD;
#elif defined(_AIX)
    return OS::AIX;
#elif defined(__managarm__)
    return OS::Managarm;
#elif defined(__redox__)
    return OS::RedoxOS;
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    return OS::IOS;
#elif defined(__APPLE__)
    return OS::MacOS;
#elif defined(__FreeBSD__)
    return OS::FreeBSD;
#elif defined(__sun) && defined(__SVR4)
    return OS::Solaris;
#elif defined(__linux__)
    return OS::Linux;
#else
    return OS::Unknown;
#endif
}

EnvironmentInfo DetectEnvironment(const VideoCore::RendererBase& renderer) {
    EnvironmentInfo env{};
    env.os = DetectOS();
    env.vendor_string = renderer.GetDeviceVendor();
    env.vendor = GetGPU(env.vendor_string);
    return env;
}

void LoadOverrides(std::uint64_t program_id, const VideoCore::RendererBase& renderer) {
    const auto env = DetectEnvironment(renderer);

    // Synchronize detected GPU vendor with GameFixDatabase and re-apply vendor-specific profile overrides
    Core::GameFixDatabase::SetDetectedGpuVendor(env.vendor_string);
    Core::GameFixDatabase::ReapplyVendorOverrides(program_id);

    const u64 base_tid = program_id & ~0x1FFFULL;
    if (program_id == 0x010015100B514000ULL || base_tid == 0x010015100B514000ULL) {
        // Super Mario Bros. Wonder (agl engine): toxic global experimental toggles deadlock fence/query timing
        Settings::values.early_release_fences.SetGlobal(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.enable_frame_skipping.SetGlobal(false);
        Settings::values.enable_frame_skipping.SetValue(false);
        Settings::values.drs_resolution_lock.SetGlobal(false);
        Settings::values.drs_resolution_lock.SetValue(false);
        Settings::values.sync_memory_operations.SetGlobal(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.vram_budget_governor.SetGlobal(false);
        Settings::values.vram_budget_governor.SetValue(false);
        Settings::values.dynamic_performance_scaler.SetGlobal(false);
        Settings::values.dynamic_performance_scaler.SetValue(false);
        Settings::values.smart_shader_throttle.SetGlobal(false);
        Settings::values.smart_shader_throttle.SetValue(false);
        Settings::values.use_video_framerate.SetGlobal(false);
        Settings::values.use_video_framerate.SetValue(false);
        Settings::values.enable_gpu_buffer_readback.SetGlobal(false);
        Settings::values.enable_gpu_buffer_readback.SetValue(false);
        Settings::values.barrier_feedback_loops.SetGlobal(false);
        Settings::values.barrier_feedback_loops.SetValue(true);
    }

    if (program_id == 0x010099900B224000ULL || base_tid == 0x010099900B224000ULL ||
        program_id == 0x01008C3005A32000ULL || base_tid == 0x01008C3005A32000ULL) {
        // Unravel Two: requires reliable hardware fences, accurate GPU timestamps, and disabled feedback loops
        Settings::values.early_release_fences.SetGlobal(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.use_fast_gpu_time.SetGlobal(false);
        Settings::values.use_fast_gpu_time.SetValue(false);
        Settings::values.barrier_feedback_loops.SetGlobal(false);
        Settings::values.barrier_feedback_loops.SetValue(false);
    }

    if (Settings::IsCurrentProgramSwitch2() || ((program_id >> 52) == 0x040)) {
        // Nintendo Switch 2 (Ampere / nx2:gsv / GpuSplit): enforce clean hardware fences & accurate 31.25 MHz timestamps
        Settings::values.early_release_fences.SetGlobal(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.enable_frame_skipping.SetGlobal(false);
        Settings::values.enable_frame_skipping.SetValue(false);
        Settings::values.drs_resolution_lock.SetGlobal(false);
        Settings::values.drs_resolution_lock.SetValue(false);
        Settings::values.sync_memory_operations.SetGlobal(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.use_fast_gpu_time.SetGlobal(false);
        Settings::values.use_fast_gpu_time.SetValue(false);
        Settings::values.dynamic_performance_scaler.SetGlobal(false);
        Settings::values.dynamic_performance_scaler.SetValue(false);
        Settings::values.smart_shader_throttle.SetGlobal(false);
        Settings::values.smart_shader_throttle.SetValue(false);
        Settings::values.use_video_framerate.SetGlobal(false);
        Settings::values.use_video_framerate.SetValue(false);
        Settings::values.enable_gpu_buffer_readback.SetGlobal(false);
        Settings::values.enable_gpu_buffer_readback.SetValue(false);
        if (Settings::values.gpu_accuracy.GetValue() != Settings::GpuAccuracy::Normal) {
            Settings::values.gpu_accuracy.SetGlobal(false);
            Settings::values.gpu_accuracy.SetValue(Settings::GpuAccuracy::Normal);
        }
        if (Settings::values.gpu_fence_behavior.GetValue() != Settings::GpuFenceBehavior::Default) {
            Settings::values.gpu_fence_behavior.SetGlobal(false);
            Settings::values.gpu_fence_behavior.SetValue(Settings::GpuFenceBehavior::Default);
        }
        if (Settings::values.cpu_accuracy.GetValue() == Settings::CpuAccuracy::Unsafe) {
            Settings::values.cpu_accuracy.SetGlobal(false);
            Settings::values.cpu_accuracy.SetValue(Settings::CpuAccuracy::Auto);
        }
    }

    switch (static_cast<TitleID>(program_id)) {
        case TitleID::NinjaGaidenRagebound:
            Settings::values.use_squashed_iterated_blend = true;
            break;
        default:
            break;
    }

    LOG_INFO(Core, "Applied game settings for title ID {:016X} on OS {}, GPU vendor {} ({})",
             program_id,
             static_cast<int>(env.os),
             static_cast<int>(env.vendor),
             env.vendor_string);
}

} // namespace Core::GameSettings
