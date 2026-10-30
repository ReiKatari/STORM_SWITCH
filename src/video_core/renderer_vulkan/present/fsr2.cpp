// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM / Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>

#include "common/settings.h"
#include "video_core/fsr.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr2_temporal_frag_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp16_frag_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp32_frag_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_vert_spv.h"
#include "video_core/renderer_vulkan/present/fsr2.h"
#include "video_core/renderer_vulkan/present/util.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan::FSR2 {

namespace {

struct TemporalPushConstants {
    std::array<f32, 4> render_size;   // xy: current input size, zw: 1.0 / input size
    std::array<f32, 4> display_size;  // xy: output size, zw: 1.0 / output size
    std::array<f32, 4> jitter_params; // xy: subpixel jitter offset in UV, z: alpha, w: gamma
};

using RcasPushConstants = std::array<u32, 4 * 4>;

// Halton (2, 3) low-discrepancy 16-phase subpixel sequence
constexpr std::array<std::pair<float, float>, 16> HALTON_16 = {{
    {0.0f, -0.333333f}, {-0.5f, 0.333333f}, {0.5f, -0.777778f}, {-0.75f, -0.111111f},
    {0.25f, 0.555556f}, {-0.25f, -0.555556f}, {0.75f, 0.111111f}, {-0.875f, 0.777778f},
    {0.125f, -0.925926f}, {-0.375f, -0.259259f}, {0.625f, 0.407407f}, {-0.625f, -0.703704f},
    {0.375f, -0.037037f}, {-0.125f, 0.629630f}, {0.875f, -0.481481f}, {-0.9375f, 0.185185f}
}};

} // anonymous namespace

QualityConfig GetQualityConfig(Settings::Fsr2QualityMode mode) {
    switch (mode) {
    case Settings::Fsr2QualityMode::Quality:
        return QualityConfig{
            .scale_factor = 1.5f,
            .jitter_phase_count = 16,
            .sharpness_bias = 0.0f,
        };
    case Settings::Fsr2QualityMode::Balanced:
        return QualityConfig{
            .scale_factor = 1.7f,
            .jitter_phase_count = 20,
            .sharpness_bias = 0.1f,
        };
    case Settings::Fsr2QualityMode::Performance:
        return QualityConfig{
            .scale_factor = 2.0f,
            .jitter_phase_count = 24,
            .sharpness_bias = 0.2f,
        };
    case Settings::Fsr2QualityMode::UltraPerformance:
        return QualityConfig{
            .scale_factor = 3.0f,
            .jitter_phase_count = 32,
            .sharpness_bias = 0.3f,
        };
    default:
        return QualityConfig{
            .scale_factor = 1.5f,
            .jitter_phase_count = 16,
            .sharpness_bias = 0.0f,
        };
    }
}

f32 GetScaleFactor(Settings::Fsr2QualityMode mode) {
    return GetQualityConfig(mode).scale_factor;
}

u32 GetJitterPhaseCount(Settings::Fsr2QualityMode mode) {
    return GetQualityConfig(mode).jitter_phase_count;
}

Pipeline::Pipeline(const Device& device, MemoryAllocator& memory_allocator, size_t image_count,
                   VkExtent2D extent)
    : m_memory_allocator{memory_allocator}
    , m_image_count{image_count}
    , m_extent{extent}
{
    CreateImages(device);
    CreateRenderPasses(device);
    CreateSamplers(device);
    CreateShaders(device);
    CreateDescriptorPool(device);
    CreateDescriptorSetLayouts(device);
    CreateDescriptorSets(device);
    CreatePipelineLayouts(device);
    CreatePipelines(device);
}

Pipeline::~Pipeline() = default;

void Pipeline::CreateImages(const Device& device) {
    m_history_images[0] = CreateWrappedImage(m_memory_allocator, m_extent, VK_FORMAT_R16G16B16A16_SFLOAT);
    m_history_images[1] = CreateWrappedImage(m_memory_allocator, m_extent, VK_FORMAT_R16G16B16A16_SFLOAT);
    m_history_views[0] = CreateWrappedImageView(device, m_history_images[0], VK_FORMAT_R16G16B16A16_SFLOAT);
    m_history_views[1] = CreateWrappedImageView(device, m_history_images[1], VK_FORMAT_R16G16B16A16_SFLOAT);

    m_frames.resize(m_image_count);
    for (auto& frame : m_frames) {
        frame.rcas_output_image = CreateWrappedImage(m_memory_allocator, m_extent, VK_FORMAT_R16G16B16A16_SFLOAT);
        frame.rcas_output_view = CreateWrappedImageView(device, frame.rcas_output_image, VK_FORMAT_R16G16B16A16_SFLOAT);
    }
}

void Pipeline::CreateRenderPasses(const Device& device) {
    m_temporal_renderpass = CreateWrappedRenderPass(device, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_history_framebuffers[0] = CreateWrappedFramebuffer(device, m_temporal_renderpass, m_history_views[0], m_extent);
    m_history_framebuffers[1] = CreateWrappedFramebuffer(device, m_temporal_renderpass, m_history_views[1], m_extent);

    m_rcas_renderpass = CreateWrappedRenderPass(device, VK_FORMAT_R16G16B16A16_SFLOAT,
                                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    for (auto& frame : m_frames) {
        frame.rcas_framebuffer = CreateWrappedFramebuffer(device, m_rcas_renderpass, frame.rcas_output_view, m_extent);
    }
}

void Pipeline::CreateSamplers(const Device& device) {
    m_linear_sampler = CreateBilinearSampler(device);
    m_point_sampler = CreateNearestNeighborSampler(device);
}

void Pipeline::CreateShaders(const Device& device) {
    m_vert_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_VERT_SPV);
    m_temporal_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR2_TEMPORAL_FRAG_SPV);

    if (device.IsFloat16Supported()) {
        m_rcas_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_RCAS_FP16_FRAG_SPV);
    } else {
        m_rcas_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_RCAS_FP32_FRAG_SPV);
    }
}

void Pipeline::CreateDescriptorPool(const Device& device) {
    // Temporal: 2 descriptors per frame (binding 0: current, binding 1: history)
    // RCAS: 1 descriptor per frame (binding 0: accumulated history)
    // Total descriptors: 3 * m_image_count; total sets: 2 * m_image_count
    m_descriptor_pool = CreateWrappedDescriptorPool(device, 3 * m_image_count, 2 * m_image_count);
}

void Pipeline::CreateDescriptorSetLayouts(const Device& device) {
    m_temporal_descriptor_set_layout = CreateWrappedDescriptorSetLayout(
        device, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER});
    m_rcas_descriptor_set_layout = CreateWrappedDescriptorSetLayout(
        device, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER});
}

void Pipeline::CreateDescriptorSets(const Device& device) {
    std::vector<VkDescriptorSetLayout> temporal_layouts(1, *m_temporal_descriptor_set_layout);
    std::vector<VkDescriptorSetLayout> rcas_layouts(1, *m_rcas_descriptor_set_layout);

    for (auto& frame : m_frames) {
        frame.temporal_sets = CreateWrappedDescriptorSets(m_descriptor_pool, temporal_layouts);
        frame.rcas_sets = CreateWrappedDescriptorSets(m_descriptor_pool, rcas_layouts);
    }
}

void Pipeline::CreatePipelineLayouts(const Device& device) {
    const VkPushConstantRange temporal_range{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(TemporalPushConstants),
    };
    VkPipelineLayoutCreateInfo temporal_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = m_temporal_descriptor_set_layout.address(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &temporal_range,
    };
    m_temporal_pipeline_layout = device.GetLogical().CreatePipelineLayout(temporal_ci);

    const VkPushConstantRange rcas_range{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(RcasPushConstants),
    };
    VkPipelineLayoutCreateInfo rcas_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = m_rcas_descriptor_set_layout.address(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &rcas_range,
    };
    m_rcas_pipeline_layout = device.GetLogical().CreatePipelineLayout(rcas_ci);
}

void Pipeline::CreatePipelines(const Device& device) {
    m_temporal_pipeline = CreateWrappedPipeline(device, m_temporal_renderpass, m_temporal_pipeline_layout,
                                               std::tie(m_vert_shader, m_temporal_shader));
    m_rcas_pipeline = CreateWrappedPipeline(device, m_rcas_renderpass, m_rcas_pipeline_layout,
                                           std::tie(m_vert_shader, m_rcas_shader));
}

VkImageView Pipeline::Draw(const Device& device, Scheduler& scheduler, size_t image_index,
                           VkImage source_image, VkImageView source_image_view,
                           VkExtent2D input_image_extent, const Common::Rectangle<f32>& crop_rect) {
    FrameResources& frame = m_frames[image_index];

    const size_t write_hist_idx = m_history_index;
    const size_t read_hist_idx = 1 - m_history_index;

    VkImage history_write_image = *m_history_images[write_hist_idx];
    VkFramebuffer history_framebuffer = *m_history_framebuffers[write_hist_idx];
    VkImageView history_write_view = *m_history_views[write_hist_idx];

    VkImage history_read_image = *m_history_images[read_hist_idx];
    VkImageView history_read_view = m_history_valid ? *m_history_views[read_hist_idx] : source_image_view;

    VkImage rcas_output_image = *frame.rcas_output_image;
    VkImageView rcas_output_view = *frame.rcas_output_view;
    VkFramebuffer rcas_framebuffer = *frame.rcas_framebuffer;

    VkPipeline temporal_pipeline = *m_temporal_pipeline;
    VkPipeline rcas_pipeline = *m_rcas_pipeline;
    VkPipelineLayout temporal_pipeline_layout = *m_temporal_pipeline_layout;
    VkPipelineLayout rcas_pipeline_layout = *m_rcas_pipeline_layout;
    VkRenderPass temporal_renderpass = *m_temporal_renderpass;
    VkRenderPass rcas_renderpass = *m_rcas_renderpass;
    VkExtent2D extent = m_extent;

    // Subpixel Halton jitter phase calculation
    const auto& jitter = HALTON_16[m_frame_index % 16];
    ++m_frame_index;

    const f32 inv_input_w = 1.0f / static_cast<f32>(input_image_extent.width);
    const f32 inv_input_h = 1.0f / static_cast<f32>(input_image_extent.height);
    const f32 inv_out_w = 1.0f / static_cast<f32>(extent.width);
    const f32 inv_out_h = 1.0f / static_cast<f32>(extent.height);

    TemporalPushConstants temporal_con{};
    temporal_con.render_size = {static_cast<f32>(input_image_extent.width),
                                static_cast<f32>(input_image_extent.height),
                                inv_input_w, inv_input_h};
    temporal_con.display_size = {static_cast<f32>(extent.width),
                                 static_cast<f32>(extent.height),
                                 inv_out_w, inv_out_h};
    temporal_con.jitter_params = {jitter.first * inv_input_w,
                                  jitter.second * inv_input_h,
                                  0.88f, 1.25f};

    // Calculate RCAS sharpening configuration
    const auto fsr2_config = GetQualityConfig(Settings::values.fsr2_quality_mode.GetValue());
    const float raw_sharpening =
        (static_cast<float>(Settings::values.fsr_sharpening_slider.GetValue()) / 100.0f) + fsr2_config.sharpness_bias;
    const float sharpening = std::clamp(raw_sharpening, 0.0f, 1.0f);

    RcasPushConstants rcas_con{};
    ::FSR::FsrRcasCon(rcas_con.data(), sharpening);

    // Update descriptor sets for Pass 1 (Temporal) and Pass 2 (RCAS)
    std::vector<VkDescriptorImageInfo> image_infos;
    image_infos.reserve(3);
    std::vector<VkWriteDescriptorSet> updates{
        CreateWriteDescriptorSet(image_infos, *m_linear_sampler, source_image_view, frame.temporal_sets[0], 0),
        CreateWriteDescriptorSet(image_infos, *m_linear_sampler, history_read_view, frame.temporal_sets[0], 1),
        CreateWriteDescriptorSet(image_infos, *m_linear_sampler, history_write_view, frame.rcas_sets[0], 0),
    };
    device.GetLogical().UpdateDescriptorSets(updates, {});

    VkDescriptorSet temporal_set = frame.temporal_sets[0];
    VkDescriptorSet rcas_set = frame.rcas_sets[0];
    const bool history_valid = m_history_valid;

    scheduler.RequestOutsideRenderPassOperationContext();
    scheduler.Record([=](vk::CommandBuffer cmdbuf) {
        // 1. Ensure source image is in SHADER_READ_ONLY_OPTIMAL for Pass 1 sampling
        TransitionImageLayout(cmdbuf, source_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_GENERAL);

        // 2. Ensure read history is in SHADER_READ_ONLY_OPTIMAL
        if (history_valid) {
            TransitionImageLayout(cmdbuf, history_read_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        // 3. Transition write history image to COLOR_ATTACHMENT_OPTIMAL
        TransitionImageLayout(cmdbuf, history_write_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_UNDEFINED);

        // 4. Pass 1: Temporal Upscale + Motion/Luma Disocclusion Clamp
        BeginRenderPass(cmdbuf, temporal_renderpass, history_framebuffer, extent);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, temporal_pipeline);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, temporal_pipeline_layout, 0,
                                  temporal_set, {});
        cmdbuf.PushConstants(temporal_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, temporal_con);
        cmdbuf.Draw(3, 1, 0, 0);
        cmdbuf.EndRenderPass();

        // 5. Transition newly accumulated history to SHADER_READ_ONLY_OPTIMAL for Pass 2 sampling
        TransitionImageLayout(cmdbuf, history_write_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // 6. Transition RCAS output image to COLOR_ATTACHMENT_OPTIMAL
        TransitionImageLayout(cmdbuf, rcas_output_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_UNDEFINED);

        // 7. Pass 2: Contrast-Adaptive Sharpening (RCAS)
        BeginRenderPass(cmdbuf, rcas_renderpass, rcas_framebuffer, extent);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, rcas_pipeline);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, rcas_pipeline_layout, 0,
                                  rcas_set, {});
        cmdbuf.PushConstants(rcas_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, rcas_con);
        cmdbuf.Draw(3, 1, 0, 0);
        cmdbuf.EndRenderPass();

        // 8. Transition RCAS output to SHADER_READ_ONLY_OPTIMAL for optimal hardware presentation
        TransitionImageLayout(cmdbuf, rcas_output_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    });

    // Advance history ping-pong state
    m_history_valid = true;
    m_history_index = 1 - m_history_index;

    return rcas_output_view;
}

} // namespace Vulkan::FSR2
