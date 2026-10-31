// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "common/common_types.h"
#include "video_core/renderer_vulkan/present/frame_gen_pacer.h"
#include "video_core/renderer_vulkan/present/lsfg_chain.h"
#include "video_core/renderer_vulkan/present/lsfg_shaders.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"

namespace Vulkan {

class Device;
class Scheduler;
struct Frame;

class FrameGen {
public:
    explicit FrameGen(MemoryAllocator& memory_allocator, Scheduler& scheduler);
    ~FrameGen();

    void Process(const Device& device, Frame* frame, VkFormat format, VkExtent2D guest_extent);

    [[nodiscard]] size_t WantedGenerations(size_t capacity);

    [[nodiscard]] size_t GeneratedFrameCount() const;

    void GenerateInto(const Device& device, Frame* destination, size_t generation);

private:
    void Rebuild(const Device& device, VkExtent2D extent, VkFormat format, f32 flow_scale);
    void RebuildFallback(const Device& device, VkExtent2D extent, VkFormat format);
    void DumpDebugImages(u64 count);

    MemoryAllocator& memory_allocator;
    Scheduler& scheduler;

    std::optional<LsfgShaders> shaders;
    std::optional<LsfgChain> chain;
    FrameGenPacer pacer;
    FrameGenPlan plan{};
    VkExtent2D peak_guest_extent{};
    VkExtent2D built_extent{};
    VkFormat built_format{VK_FORMAT_UNDEFINED};
    f32 built_flow_scale{};
    u64 frame_count{};
    u64 last_count{};
    size_t last_generations{};
    u32 warm_streak{};
    bool generated{};
    bool unavailable{};
    bool dumped{};
    bool last_fg_active{};
    bool has_lsfg{};

    // Native Vulkan Frame Interpolation Fallback resources (0% external dependency)
    std::array<LsfgImage, 2> fallback_frames;
    vk::DescriptorSetLayout fallback_set_layout;
    vk::PipelineLayout fallback_pipeline_layout;
    vk::Pipeline fallback_pipeline;
    vk::DescriptorPool fallback_descriptor_pool;
    vk::Sampler fallback_sampler;
    vk::DescriptorSets fallback_sets;
};

} // namespace Vulkan
