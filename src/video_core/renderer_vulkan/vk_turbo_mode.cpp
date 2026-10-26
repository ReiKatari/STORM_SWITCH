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
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#if defined(ARCHITECTURE_arm64)
#include <adrenotools/driver.h>
#endif
#endif

#include "common/literals.h"
#include "common/logging.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_turbo_mode.h"
#include "video_core/host_shaders/vulkan_turbo_mode_comp_spv.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

using namespace Common::Literals;

TurboMode::TurboMode(const vk::Instance& instance, const vk::InstanceDispatch& dld) {
    m_last_submission_ns.store(
        std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
    m_thread = std::jthread([this, &instance, &dld](auto stop_token) {
        Run(stop_token, instance, dld);
    });
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
    m_last_submission_ns.store(
        std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
}

void TurboMode::Run(std::stop_token stop_token, const vk::Instance& instance,
                    const vk::InstanceDispatch& dld) {
#ifdef _WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    PROCESS_POWER_THROTTLING_STATE throttling{};
    throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
                             PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    throttling.StateMask = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &throttling,
                          sizeof(throttling));

    timeBeginPeriod(1);
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    LOG_INFO(Render_Vulkan, "STORM Turbo Mode enabled: Desktop GPU clock keep-alive and unthrottled scheduling active");
#elif defined(__ANDROID__)
#if defined(ARCHITECTURE_arm64)
    adrenotools_set_turbo(true);
#endif
    // Strictly target GPU power/frequency nodes ONLY — never force CPU prime cores (policy0/4/7)
    // to 'performance' or setpriority(-20), avoiding thermal throttling (90C+) on Snapdragon 8 Gen 3 / 8 Elite.
    static const char* const gpu_perf_nodes[] = {
        "/sys/devices/platform/mali.0/power_policy",
        "/sys/devices/platform/13000000.mali/power_policy",
        "/sys/devices/platform/13040000.mali/power_policy",
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/13040000.mali/governor",
        "/sys/class/devfreq/mali.0/governor",
        "/sys/class/kgsl/kgsl-3d0/force_clk_on",
        "/sys/class/kgsl/kgsl-3d0/force_bus_on",
        "/sys/class/kgsl/kgsl-3d0/force_rail_on",
        "/sys/class/kgsl/kgsl-3d0/max_pwrlevel",
    };

    auto write_node = [](const char* path, const char* val) {
        int fd = open(path, O_WRONLY);
        if (fd >= 0) {
            write(fd, val, std::strlen(val));
            close(fd);
        }
    };

    for (const auto* node : gpu_perf_nodes) {
        if (std::strstr(node, "governor")) {
            write_node(node, "performance\n");
        } else if (std::strstr(node, "power_policy")) {
            write_node(node, "always_on\n");
        } else {
            write_node(node, "1\n");
        }
    }

    LOG_INFO(Render_Vulkan, "STORM Turbo Mode enabled: Mobile GPU turbo active (CPU thermal protection preserved)");
#endif

#ifndef __ANDROID__
    // Desktop / Handheld PC (ROG Ally AMD RDNA3, Steam Deck, Legion Go):
    // Keep a lightweight Vulkan auxiliary command queue alive to prevent aggressive RDNA3/Arc
    // power-saving downclocks (800 MHz drop) between CPU-limited frames.
    try {
        Device turbo_device = CreateDevice(instance, dld, VK_NULL_HANDLE);
        MemoryAllocator alloc(turbo_device);

        const VkBufferCreateInfo buffer_ci{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = 4096,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };
        vk::Buffer dummy_buffer = alloc.CreateBuffer(buffer_ci, MemoryUsage::DeviceLocal);

        const VkDescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        };
        vk::DescriptorSetLayout desc_layout = turbo_device.GetLogical().CreateDescriptorSetLayout({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = 1,
            .pBindings = &binding,
        });
        const VkDescriptorPoolSize pool_size{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        };
        vk::DescriptorPool desc_pool = turbo_device.GetLogical().CreateDescriptorPool({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
        });
        const VkDescriptorSetLayout raw_desc_layout = *desc_layout;
        auto desc_sets = desc_pool.Allocate({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = *desc_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &raw_desc_layout,
        });
        const VkDescriptorBufferInfo buf_info{
            .buffer = *dummy_buffer,
            .offset = 0,
            .range = 4096,
        };
        const VkWriteDescriptorSet write_desc{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = desc_sets[0],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &buf_info,
            .pTexelBufferView = nullptr,
        };
        turbo_device.GetLogical().UpdateDescriptorSets(std::span{&write_desc, 1}, {});

        vk::PipelineLayout pipe_layout = turbo_device.GetLogical().CreatePipelineLayout({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &raw_desc_layout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        });
        vk::ShaderModule comp_shader = BuildShader(turbo_device, VULKAN_TURBO_MODE_COMP_SPV);
        vk::Pipeline comp_pipeline = turbo_device.GetLogical().CreateComputePipeline({
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = *comp_shader,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            },
            .layout = *pipe_layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        });

        const VkCommandPoolCreateInfo pool_ci{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = turbo_device.GetGraphicsFamily(),
        };
        vk::CommandPool cmd_pool = turbo_device.GetLogical().CreateCommandPool(pool_ci);
        auto cmd_bufs = cmd_pool.Allocate(1);
        vk::CommandBuffer cmd_buf{cmd_bufs[0], turbo_device.GetDispatchLoader()};

        const VkFenceCreateInfo fence_ci{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        vk::Fence fence = turbo_device.GetLogical().CreateFence(fence_ci);

        while (!stop_token.stop_requested()) {
            cmd_buf.Begin({
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr,
            });
            cmd_buf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *comp_pipeline);
            const VkDescriptorSet raw_set = desc_sets[0];
            cmd_buf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *pipe_layout, 0,
                                       std::span{&raw_set, 1}, {});
            cmd_buf.Dispatch(32, 4, 1);
            cmd_buf.End();

            const VkCommandBuffer raw_cmd = *cmd_buf;
            const VkSubmitInfo submit_info{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = nullptr,
                .commandBufferCount = 1,
                .pCommandBuffers = &raw_cmd,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr,
            };
            turbo_device.GetGraphicsQueue().Submit(submit_info, *fence);
            fence.Wait();
            fence.Reset();

            std::unique_lock lk{m_submission_lock};
            m_submission_cv.wait_for(lk, stop_token, std::chrono::milliseconds{4}, [&] {
                return stop_token.stop_requested();
            });
        }
        return;
    } catch (const std::exception& e) {
        LOG_WARNING(Render_Vulkan, "Auxiliary TurboMode Vulkan keep-alive unavailable ({}), continuing with OS priority boost", e.what());
    } catch (...) {
        LOG_WARNING(Render_Vulkan, "Auxiliary TurboMode Vulkan keep-alive unavailable, continuing with OS priority boost");
    }
#endif

#if defined(__ANDROID__)
    bool turbo_currently_enabled = true;
    auto read_max_thermal_celsius = []() -> int {
        static const char* const thermal_nodes[] = {
            "/sys/class/kgsl/kgsl-3d0/temp",
            "/sys/class/power_supply/battery/temp",
        };
        int max_c = 0;
        char buf[32]{};
        for (const char* tpath : thermal_nodes) {
            int fd = open(tpath, O_RDONLY);
            if (fd >= 0) {
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                close(fd);
                if (n > 0) {
                    buf[n] = '\0';
                    int raw = std::atoi(buf);
                    int c = raw;
                    if (std::strstr(tpath, "battery") != nullptr) {
                        c = (raw > 1000) ? (raw / 1000) : (raw / 10);
                        if (c >= 45) {
                            max_c = std::max(max_c, 84);
                        }
                    } else {
                        if (raw > 1000) c = raw / 1000;
                        else if (raw > 200) c = raw / 10;
                        if (c > 0 && c < 125) {
                            max_c = std::max(max_c, c);
                        }
                    }
                }
            }
        }
        return max_c;
    };
#endif

    while (!stop_token.stop_requested()) {
        std::unique_lock lk{m_submission_lock};
        m_submission_cv.wait_for(lk, stop_token, std::chrono::seconds{2}, [&] {
            return stop_token.stop_requested();
        });
#if defined(__ANDROID__)
        if (!stop_token.stop_requested()) {
            const int temp_c = read_max_thermal_celsius();
            if (turbo_currently_enabled && temp_c >= 82) {
#if defined(ARCHITECTURE_arm64)
                adrenotools_set_turbo(false);
#endif
                write_node("/sys/class/kgsl/kgsl-3d0/force_clk_on", "0\n");
                write_node("/sys/class/kgsl/kgsl-3d0/force_bus_on", "0\n");
                write_node("/sys/class/kgsl/kgsl-3d0/force_rail_on", "0\n");
                turbo_currently_enabled = false;
                LOG_INFO(Render_Vulkan, "STORM Thermal Guard: Mobile GPU turbo stepped down ({}C) to prevent overheating", temp_c);
            } else if (!turbo_currently_enabled && temp_c > 0 && temp_c <= 64) {
#if defined(ARCHITECTURE_arm64)
                adrenotools_set_turbo(true);
#endif
                turbo_currently_enabled = true;
            }
        }
#endif
    }
}

} // namespace Vulkan
