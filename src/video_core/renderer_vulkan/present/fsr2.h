// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "common/common_types.h"
#include "common/math_util.h"
#include "common/settings_enums.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class Scheduler;

namespace FSR2 {

struct QualityConfig {
    f32 scale_factor;
    u32 jitter_phase_count;
    f32 sharpness_bias;
};

[[nodiscard]] QualityConfig GetQualityConfig(Settings::Fsr2QualityMode mode);
[[nodiscard]] f32 GetScaleFactor(Settings::Fsr2QualityMode mode);
[[nodiscard]] u32 GetJitterPhaseCount(Settings::Fsr2QualityMode mode);

class Pipeline {
public:
    explicit Pipeline(const Device& device, MemoryAllocator& memory_allocator, size_t image_count,
                      VkExtent2D extent);
    ~Pipeline();

    VkImageView Draw(const Device& device, Scheduler& scheduler, size_t image_index,
                     VkImage source_image, VkImageView source_image_view,
                     VkExtent2D input_image_extent, const Common::Rectangle<f32>& crop_rect);

private:
    void CreateImages(const Device& device);
    void CreateRenderPasses(const Device& device);
    void CreateSamplers(const Device& device);
    void CreateShaders(const Device& device);
    void CreateDescriptorPool(const Device& device);
    void CreateDescriptorSetLayouts(const Device& device);
    void CreateDescriptorSets(const Device& device);
    void CreatePipelineLayouts(const Device& device);
    void CreatePipelines(const Device& device);

    MemoryAllocator& m_memory_allocator;
    const size_t m_image_count;
    const VkExtent2D m_extent;

    // Ping-pong history resources for temporal accumulation
    std::array<vk::Image, 2> m_history_images;
    std::array<vk::ImageView, 2> m_history_views;
    std::array<vk::Framebuffer, 2> m_history_framebuffers;
    size_t m_history_index{0};
    bool m_history_valid{false};
    u32 m_frame_index{0};

    // Shaders & render passes
    vk::ShaderModule m_vert_shader;
    vk::ShaderModule m_temporal_shader;
    vk::ShaderModule m_rcas_shader;

    vk::RenderPass m_temporal_renderpass;
    vk::RenderPass m_rcas_renderpass;

    vk::Sampler m_linear_sampler;
    vk::Sampler m_point_sampler;

    vk::DescriptorPool m_descriptor_pool;
    vk::DescriptorSetLayout m_temporal_descriptor_set_layout;
    vk::DescriptorSetLayout m_rcas_descriptor_set_layout;
    vk::PipelineLayout m_temporal_pipeline_layout;
    vk::PipelineLayout m_rcas_pipeline_layout;
    vk::Pipeline m_temporal_pipeline;
    vk::Pipeline m_rcas_pipeline;

    struct FrameResources {
        vk::DescriptorSets temporal_sets;
        vk::DescriptorSets rcas_sets;
        vk::Image rcas_output_image;
        vk::ImageView rcas_output_view;
        vk::Framebuffer rcas_framebuffer;
    };
    std::vector<FrameResources> m_frames;
};

} // namespace FSR2
} // namespace Vulkan
