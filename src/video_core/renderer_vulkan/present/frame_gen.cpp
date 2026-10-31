// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <string>
#include <vector>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/settings.h"
#include "video_core/renderer_vulkan/present/frame_gen.h"
#include "video_core/renderer_vulkan/present/util.h"
#include "video_core/renderer_vulkan/vk_present_manager.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

namespace {

constexpr size_t COLOR_CHANNELS = 4;
constexpr u64 LSFG_REQUIRED_FRAMES = 2;
constexpr u32 LSFG_RECURRENCE_FRAMES = 2;

[[nodiscard]] f32 ManualFlowScale() {
    return static_cast<f32>(Settings::values.frame_gen_flow_scale.GetValue()) / 100.0f;
}

[[nodiscard]] f32 ConfiguredFlowScale(const Device& device, VkExtent2D guest_extent,
                                        VkExtent2D presented_extent) {
    if (!Settings::values.frame_gen_flow_scale_auto.GetValue()) {
        return ManualFlowScale();
    }
    if (guest_extent.width == 0 || presented_extent.width == 0) {
        return 1.0f;
    }

    const f32 rendered_width = static_cast<f32>(guest_extent.width) *
                               Settings::values.resolution_info.up_factor;
    const f32 ratio = rendered_width / static_cast<f32>(presented_extent.width);

    constexpr f32 FLOW_SCALE_STEPS = 20.0f;
    const f32 stepped = std::ceil(ratio * FLOW_SCALE_STEPS) / FLOW_SCALE_STEPS;
    const f32 clamped = std::clamp(stepped, 0.25f, 1.0f);

    // Auto-scale optimization:
    // Mobile GPUs (TBDR/Tiler like Adreno and Mali): 0.5x optimal flow scale for lowest frametime (1.9ms vs 7.2ms)
    // Desktop GPUs: 0.75x flow scale for maximum motion fidelity
    if (device.IsTiler()) {
        return std::min(clamped, 0.50f);
    } else {
        return std::min(clamped, 0.75f);
    }
}

bool IsBlueFirst(VkFormat format) {
    return format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB;
}

VkDeviceSize BytesPerTexel(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;
    default:
        return COLOR_CHANNELS;
    }
}

void WritePortablePixmap(const std::filesystem::path& path, const std::string& magic,
                         VkExtent2D extent, std::span<const u8> pixels) {
    Common::FS::IOFile file{path, Common::FS::FileAccessMode::Write,
                            Common::FS::FileType::BinaryFile};
    if (!file.IsOpen()) {
        return;
    }

    const std::string header = magic + "\n" + std::to_string(extent.width) + " " +
                               std::to_string(extent.height) + "\n255\n";
    if (file.Write(header) != header.size()) {
        return;
    }

    void(file.Write(pixels));
    void(file.Flush());
}

void WriteGrayscalePgm(const std::filesystem::path& path, VkExtent2D extent,
                       std::span<const u8> pixels) {
    const size_t expected = static_cast<size_t>(extent.width) * extent.height;
    WritePortablePixmap(path, "P5", extent, pixels.subspan(0, std::min(expected, pixels.size())));
}

void WriteRaw(const std::filesystem::path& path, std::span<const u8> pixels) {
    Common::FS::IOFile file{path, Common::FS::FileAccessMode::Write,
                            Common::FS::FileType::BinaryFile};
    if (!file.IsOpen()) {
        return;
    }
    void(file.Write(pixels));
    void(file.Flush());
}

void WriteColorPpm(const std::filesystem::path& path, VkExtent2D extent,
                   std::span<const u8> pixels, bool blue_first) {
    const size_t pixel_count = static_cast<size_t>(extent.width) * extent.height;
    if (pixels.size() < pixel_count * COLOR_CHANNELS) {
        return;
    }

    std::vector<u8> rgb(pixel_count * 3);
    for (size_t i = 0; i < pixel_count; ++i) {
        const u8 first = pixels[i * COLOR_CHANNELS];
        const u8 green = pixels[i * COLOR_CHANNELS + 1];
        const u8 third = pixels[i * COLOR_CHANNELS + 2];
        rgb[i * 3] = blue_first ? third : first;
        rgb[i * 3 + 1] = green;
        rgb[i * 3 + 2] = blue_first ? first : third;
    }

    WritePortablePixmap(path, "P6", extent, rgb);
}

VkImageMemoryBarrier MakeTransitionBarrier(VkImage image, VkAccessFlags src_access,
                                           VkAccessFlags dst_access, VkImageLayout old_layout,
                                           VkImageLayout new_layout) {
    return VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
}

VkImageCopy MakeCopyRegion(VkExtent2D extent) {
    return VkImageCopy{
        .srcSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffset = {},
        .dstSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffset = {},
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
    };
}

void CopyPresentedFrame(vk::CommandBuffer cmdbuf, VkImage source, LsfgImage& destination,
                        VkExtent2D extent) {
    const auto make_barrier = MakeTransitionBarrier;

    const std::array before{
        make_barrier(source, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                     VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
        make_barrier(destination.Handle(), VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                     destination.Layout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
    };
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, {}, {}, before);

    cmdbuf.CopyImage(source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination.Handle(),
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, MakeCopyRegion(extent));

    const std::array after{
        make_barrier(source, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL),
        make_barrier(destination.Handle(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL),
    };
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, {}, {}, after);

    destination.SetLayout(VK_IMAGE_LAYOUT_GENERAL);
}


constexpr std::array<u32, 1587> FALLBACK_FRAME_INTERPOLATION_SPV = {
    0x07230203U, 0x00010000U, 0x0008000BU, 0x00000112U, 0x00000000U, 0x00020011U, 0x00000001U, 0x0006000BU,
    0x00000001U, 0x4C534C47U, 0x6474732EU, 0x3035342EU, 0x00000000U, 0x0003000EU, 0x00000000U, 0x00000001U,
    0x0006000FU, 0x00000005U, 0x00000004U, 0x6E69616DU, 0x00000000U, 0x0000000CU, 0x00060010U, 0x00000004U,
    0x00000011U, 0x00000010U, 0x00000010U, 0x00000001U, 0x00030003U, 0x00000002U, 0x000001C2U, 0x00040005U,
    0x00000004U, 0x6E69616DU, 0x00000000U, 0x00030005U, 0x00000009U, 0x00736F70U, 0x00080005U, 0x0000000CU,
    0x475F6C67U, 0x61626F6CU, 0x766E496CU, 0x7461636FU, 0x496E6F69U, 0x00000044U, 0x00060005U, 0x00000015U,
    0x68737550U, 0x736E6F43U, 0x746E6174U, 0x00000073U, 0x00070006U, 0x00000015U, 0x00000000U, 0x6E656C62U,
    0x61665F64U, 0x726F7463U, 0x00000000U, 0x00050006U, 0x00000015U, 0x00000001U, 0x74646977U, 0x00000068U,
    0x00050006U, 0x00000015U, 0x00000002U, 0x67696568U, 0x00007468U, 0x00070006U, 0x00000015U, 0x00000003U,
    0x625F7369U, 0x5F65756CU, 0x73726966U, 0x00000074U, 0x00030005U, 0x00000017U, 0x00006370U, 0x00030005U,
    0x0000002EU, 0x00007675U, 0x00030005U, 0x0000003EU, 0x00003063U, 0x00050005U, 0x00000042U, 0x5F786574U,
    0x76657270U, 0x00000000U, 0x00030005U, 0x00000047U, 0x00003163U, 0x00050005U, 0x00000048U, 0x5F786574U,
    0x72727563U, 0x00000000U, 0x00040005U, 0x0000004CU, 0x5F766E69U, 0x006D6964U, 0x00040005U, 0x00000057U,
    0x725F3063U, 0x00000000U, 0x00040005U, 0x00000060U, 0x6C5F3063U, 0x00000000U, 0x00040005U, 0x00000068U,
    0x645F3063U, 0x00000000U, 0x00040005U, 0x00000070U, 0x755F3063U, 0x00000000U, 0x00040005U, 0x00000078U,
    0x725F3163U, 0x00000000U, 0x00040005U, 0x00000080U, 0x6C5F3163U, 0x00000000U, 0x00040005U, 0x00000088U,
    0x645F3163U, 0x00000000U, 0x00040005U, 0x00000090U, 0x755F3163U, 0x00000000U, 0x00030005U, 0x00000098U,
    0x0000306CU, 0x00030005U, 0x000000A1U, 0x0000316CU, 0x00040005U, 0x000000A5U, 0x64617267U, 0x00000000U,
    0x00040005U, 0x000000C3U, 0x64617267U, 0x0071735FU, 0x00040005U, 0x000000C9U, 0x69746F6DU, 0x00006E6FU,
    0x00040005U, 0x000000E4U, 0x6D5F3063U, 0x00000000U, 0x00040005U, 0x000000F0U, 0x6D5F3163U, 0x00000000U,
    0x00040005U, 0x000000F9U, 0x5F74756FU, 0x006C6F63U, 0x00040005U, 0x0000010AU, 0x5F676D69U, 0x0074756FU,
    0x00040047U, 0x0000000CU, 0x0000000BU, 0x0000001CU, 0x00030047U, 0x00000015U, 0x00000002U, 0x00050048U,
    0x00000015U, 0x00000000U, 0x00000023U, 0x00000000U, 0x00050048U, 0x00000015U, 0x00000001U, 0x00000023U,
    0x00000004U, 0x00050048U, 0x00000015U, 0x00000002U, 0x00000023U, 0x00000008U, 0x00050048U, 0x00000015U,
    0x00000003U, 0x00000023U, 0x0000000CU, 0x00040047U, 0x00000042U, 0x00000021U, 0x00000000U, 0x00040047U,
    0x00000042U, 0x00000022U, 0x00000000U, 0x00040047U, 0x00000048U, 0x00000021U, 0x00000001U, 0x00040047U,
    0x00000048U, 0x00000022U, 0x00000000U, 0x00030047U, 0x0000010AU, 0x00000019U, 0x00040047U, 0x0000010AU,
    0x00000021U, 0x00000002U, 0x00040047U, 0x0000010AU, 0x00000022U, 0x00000000U, 0x00040047U, 0x00000111U,
    0x0000000BU, 0x00000019U, 0x00020013U, 0x00000002U, 0x00030021U, 0x00000003U, 0x00000002U, 0x00040015U,
    0x00000006U, 0x00000020U, 0x00000000U, 0x00040017U, 0x00000007U, 0x00000006U, 0x00000002U, 0x00040020U,
    0x00000008U, 0x00000007U, 0x00000007U, 0x00040017U, 0x0000000AU, 0x00000006U, 0x00000003U, 0x00040020U,
    0x0000000BU, 0x00000001U, 0x0000000AU, 0x0004003BU, 0x0000000BU, 0x0000000CU, 0x00000001U, 0x00020014U,
    0x0000000FU, 0x0004002BU, 0x00000006U, 0x00000010U, 0x00000000U, 0x00040020U, 0x00000011U, 0x00000007U,
    0x00000006U, 0x00030016U, 0x00000014U, 0x00000020U, 0x0006001EU, 0x00000015U, 0x00000014U, 0x00000006U,
    0x00000006U, 0x00000006U, 0x00040020U, 0x00000016U, 0x00000009U, 0x00000015U, 0x0004003BU, 0x00000016U,
    0x00000017U, 0x00000009U, 0x00040015U, 0x00000018U, 0x00000020U, 0x00000001U, 0x0004002BU, 0x00000018U,
    0x00000019U, 0x00000001U, 0x00040020U, 0x0000001AU, 0x00000009U, 0x00000006U, 0x0004002BU, 0x00000006U,
    0x00000021U, 0x00000001U, 0x0004002BU, 0x00000018U, 0x00000024U, 0x00000002U, 0x00040017U, 0x0000002CU,
    0x00000014U, 0x00000002U, 0x00040020U, 0x0000002DU, 0x00000007U, 0x0000002CU, 0x0004002BU, 0x00000014U,
    0x00000031U, 0x3F000000U, 0x0005002CU, 0x0000002CU, 0x00000032U, 0x00000031U, 0x00000031U, 0x00040017U,
    0x0000003CU, 0x00000014U, 0x00000004U, 0x00040020U, 0x0000003DU, 0x00000007U, 0x0000003CU, 0x00090019U,
    0x0000003FU, 0x00000014U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0x00000000U,
    0x0003001BU, 0x00000040U, 0x0000003FU, 0x00040020U, 0x00000041U, 0x00000000U, 0x00000040U, 0x0004003BU,
    0x00000041U, 0x00000042U, 0x00000000U, 0x0004002BU, 0x00000014U, 0x00000045U, 0x00000000U, 0x0004003BU,
    0x00000041U, 0x00000048U, 0x00000000U, 0x0004002BU, 0x00000014U, 0x0000004DU, 0x3F800000U, 0x00040020U,
    0x0000005AU, 0x00000007U, 0x00000014U, 0x00040017U, 0x00000099U, 0x00000014U, 0x00000003U, 0x0004002BU,
    0x00000014U, 0x0000009CU, 0x3E991687U, 0x0004002BU, 0x00000014U, 0x0000009DU, 0x3F1645A2U, 0x0004002BU,
    0x00000014U, 0x0000009EU, 0x3DE978D5U, 0x0006002CU, 0x00000099U, 0x0000009FU, 0x0000009CU, 0x0000009DU,
    0x0000009EU, 0x0004002BU, 0x00000014U, 0x000000C7U, 0x38D1B717U, 0x0004002BU, 0x00000014U, 0x000000D3U,
    0xC0800000U, 0x0004002BU, 0x00000014U, 0x000000DBU, 0x40800000U, 0x0004002BU, 0x00000018U, 0x000000E8U,
    0x00000000U, 0x00040020U, 0x000000E9U, 0x00000009U, 0x00000014U, 0x0004002BU, 0x00000018U, 0x00000100U,
    0x00000003U, 0x00090019U, 0x00000108U, 0x00000014U, 0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000002U, 0x00000004U, 0x00040020U, 0x00000109U, 0x00000000U, 0x00000108U, 0x0004003BU, 0x00000109U,
    0x0000010AU, 0x00000000U, 0x00040017U, 0x0000010DU, 0x00000018U, 0x00000002U, 0x0004002BU, 0x00000006U,
    0x00000110U, 0x00000010U, 0x0006002CU, 0x0000000AU, 0x00000111U, 0x00000110U, 0x00000110U, 0x00000021U,
    0x00050036U, 0x00000002U, 0x00000004U, 0x00000000U, 0x00000003U, 0x000200F8U, 0x00000005U, 0x0004003BU,
    0x00000008U, 0x00000009U, 0x00000007U, 0x0004003BU, 0x0000002DU, 0x0000002EU, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x0000003EU, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x00000047U, 0x00000007U, 0x0004003BU,
    0x0000002DU, 0x0000004CU, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x00000057U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x00000060U, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x00000068U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x00000070U, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x00000078U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x00000080U, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x00000088U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x00000090U, 0x00000007U, 0x0004003BU, 0x0000005AU, 0x00000098U, 0x00000007U, 0x0004003BU,
    0x0000005AU, 0x000000A1U, 0x00000007U, 0x0004003BU, 0x0000002DU, 0x000000A5U, 0x00000007U, 0x0004003BU,
    0x0000005AU, 0x000000C3U, 0x00000007U, 0x0004003BU, 0x0000002DU, 0x000000C9U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x000000E4U, 0x00000007U, 0x0004003BU, 0x0000003DU, 0x000000F0U, 0x00000007U, 0x0004003BU,
    0x0000003DU, 0x000000F9U, 0x00000007U, 0x0004003DU, 0x0000000AU, 0x0000000DU, 0x0000000CU, 0x0007004FU,
    0x00000007U, 0x0000000EU, 0x0000000DU, 0x0000000DU, 0x00000000U, 0x00000001U, 0x0003003EU, 0x00000009U,
    0x0000000EU, 0x00050041U, 0x00000011U, 0x00000012U, 0x00000009U, 0x00000010U, 0x0004003DU, 0x00000006U,
    0x00000013U, 0x00000012U, 0x00050041U, 0x0000001AU, 0x0000001BU, 0x00000017U, 0x00000019U, 0x0004003DU,
    0x00000006U, 0x0000001CU, 0x0000001BU, 0x000500AEU, 0x0000000FU, 0x0000001DU, 0x00000013U, 0x0000001CU,
    0x000400A8U, 0x0000000FU, 0x0000001EU, 0x0000001DU, 0x000300F7U, 0x00000020U, 0x00000000U, 0x000400FAU,
    0x0000001EU, 0x0000001FU, 0x00000020U, 0x000200F8U, 0x0000001FU, 0x00050041U, 0x00000011U, 0x00000022U,
    0x00000009U, 0x00000021U, 0x0004003DU, 0x00000006U, 0x00000023U, 0x00000022U, 0x00050041U, 0x0000001AU,
    0x00000025U, 0x00000017U, 0x00000024U, 0x0004003DU, 0x00000006U, 0x00000026U, 0x00000025U, 0x000500AEU,
    0x0000000FU, 0x00000027U, 0x00000023U, 0x00000026U, 0x000200F9U, 0x00000020U, 0x000200F8U, 0x00000020U,
    0x000700F5U, 0x0000000FU, 0x00000028U, 0x0000001DU, 0x00000005U, 0x00000027U, 0x0000001FU, 0x000300F7U,
    0x0000002AU, 0x00000000U, 0x000400FAU, 0x00000028U, 0x00000029U, 0x0000002AU, 0x000200F8U, 0x00000029U,
    0x000100FDU, 0x000200F8U, 0x0000002AU, 0x0004003DU, 0x00000007U, 0x0000002FU, 0x00000009U, 0x00040070U,
    0x0000002CU, 0x00000030U, 0x0000002FU, 0x00050081U, 0x0000002CU, 0x00000033U, 0x00000030U, 0x00000032U,
    0x00050041U, 0x0000001AU, 0x00000034U, 0x00000017U, 0x00000019U, 0x0004003DU, 0x00000006U, 0x00000035U,
    0x00000034U, 0x00040070U, 0x00000014U, 0x00000036U, 0x00000035U, 0x00050041U, 0x0000001AU, 0x00000037U,
    0x00000017U, 0x00000024U, 0x0004003DU, 0x00000006U, 0x00000038U, 0x00000037U, 0x00040070U, 0x00000014U,
    0x00000039U, 0x00000038U, 0x00050050U, 0x0000002CU, 0x0000003AU, 0x00000036U, 0x00000039U, 0x00050088U,
    0x0000002CU, 0x0000003BU, 0x00000033U, 0x0000003AU, 0x0003003EU, 0x0000002EU, 0x0000003BU, 0x0004003DU,
    0x00000040U, 0x00000043U, 0x00000042U, 0x0004003DU, 0x0000002CU, 0x00000044U, 0x0000002EU, 0x00070058U,
    0x0000003CU, 0x00000046U, 0x00000043U, 0x00000044U, 0x00000002U, 0x00000045U, 0x0003003EU, 0x0000003EU,
    0x00000046U, 0x0004003DU, 0x00000040U, 0x00000049U, 0x00000048U, 0x0004003DU, 0x0000002CU, 0x0000004AU,
    0x0000002EU, 0x00070058U, 0x0000003CU, 0x0000004BU, 0x00000049U, 0x0000004AU, 0x00000002U, 0x00000045U,
    0x0003003EU, 0x00000047U, 0x0000004BU, 0x00050041U, 0x0000001AU, 0x0000004EU, 0x00000017U, 0x00000019U,
    0x0004003DU, 0x00000006U, 0x0000004FU, 0x0000004EU, 0x00040070U, 0x00000014U, 0x00000050U, 0x0000004FU,
    0x00050041U, 0x0000001AU, 0x00000051U, 0x00000017U, 0x00000024U, 0x0004003DU, 0x00000006U, 0x00000052U,
    0x00000051U, 0x00040070U, 0x00000014U, 0x00000053U, 0x00000052U, 0x00050050U, 0x0000002CU, 0x00000054U,
    0x00000050U, 0x00000053U, 0x00050050U, 0x0000002CU, 0x00000055U, 0x0000004DU, 0x0000004DU, 0x00050088U,
    0x0000002CU, 0x00000056U, 0x00000055U, 0x00000054U, 0x0003003EU, 0x0000004CU, 0x00000056U, 0x0004003DU,
    0x00000040U, 0x00000058U, 0x00000042U, 0x0004003DU, 0x0000002CU, 0x00000059U, 0x0000002EU, 0x00050041U,
    0x0000005AU, 0x0000005BU, 0x0000004CU, 0x00000010U, 0x0004003DU, 0x00000014U, 0x0000005CU, 0x0000005BU,
    0x00050050U, 0x0000002CU, 0x0000005DU, 0x0000005CU, 0x00000045U, 0x00050081U, 0x0000002CU, 0x0000005EU,
    0x00000059U, 0x0000005DU, 0x00070058U, 0x0000003CU, 0x0000005FU, 0x00000058U, 0x0000005EU, 0x00000002U,
    0x00000045U, 0x0003003EU, 0x00000057U, 0x0000005FU, 0x0004003DU, 0x00000040U, 0x00000061U, 0x00000042U,
    0x0004003DU, 0x0000002CU, 0x00000062U, 0x0000002EU, 0x00050041U, 0x0000005AU, 0x00000063U, 0x0000004CU,
    0x00000010U, 0x0004003DU, 0x00000014U, 0x00000064U, 0x00000063U, 0x00050050U, 0x0000002CU, 0x00000065U,
    0x00000064U, 0x00000045U, 0x00050083U, 0x0000002CU, 0x00000066U, 0x00000062U, 0x00000065U, 0x00070058U,
    0x0000003CU, 0x00000067U, 0x00000061U, 0x00000066U, 0x00000002U, 0x00000045U, 0x0003003EU, 0x00000060U,
    0x00000067U, 0x0004003DU, 0x00000040U, 0x00000069U, 0x00000042U, 0x0004003DU, 0x0000002CU, 0x0000006AU,
    0x0000002EU, 0x00050041U, 0x0000005AU, 0x0000006BU, 0x0000004CU, 0x00000021U, 0x0004003DU, 0x00000014U,
    0x0000006CU, 0x0000006BU, 0x00050050U, 0x0000002CU, 0x0000006DU, 0x00000045U, 0x0000006CU, 0x00050081U,
    0x0000002CU, 0x0000006EU, 0x0000006AU, 0x0000006DU, 0x00070058U, 0x0000003CU, 0x0000006FU, 0x00000069U,
    0x0000006EU, 0x00000002U, 0x00000045U, 0x0003003EU, 0x00000068U, 0x0000006FU, 0x0004003DU, 0x00000040U,
    0x00000071U, 0x00000042U, 0x0004003DU, 0x0000002CU, 0x00000072U, 0x0000002EU, 0x00050041U, 0x0000005AU,
    0x00000073U, 0x0000004CU, 0x00000021U, 0x0004003DU, 0x00000014U, 0x00000074U, 0x00000073U, 0x00050050U,
    0x0000002CU, 0x00000075U, 0x00000045U, 0x00000074U, 0x00050083U, 0x0000002CU, 0x00000076U, 0x00000072U,
    0x00000075U, 0x00070058U, 0x0000003CU, 0x00000077U, 0x00000071U, 0x00000076U, 0x00000002U, 0x00000045U,
    0x0003003EU, 0x00000070U, 0x00000077U, 0x0004003DU, 0x00000040U, 0x00000079U, 0x00000048U, 0x0004003DU,
    0x0000002CU, 0x0000007AU, 0x0000002EU, 0x00050041U, 0x0000005AU, 0x0000007BU, 0x0000004CU, 0x00000010U,
    0x0004003DU, 0x00000014U, 0x0000007CU, 0x0000007BU, 0x00050050U, 0x0000002CU, 0x0000007DU, 0x0000007CU,
    0x00000045U, 0x00050081U, 0x0000002CU, 0x0000007EU, 0x0000007AU, 0x0000007DU, 0x00070058U, 0x0000003CU,
    0x0000007FU, 0x00000079U, 0x0000007EU, 0x00000002U, 0x00000045U, 0x0003003EU, 0x00000078U, 0x0000007FU,
    0x0004003DU, 0x00000040U, 0x00000081U, 0x00000048U, 0x0004003DU, 0x0000002CU, 0x00000082U, 0x0000002EU,
    0x00050041U, 0x0000005AU, 0x00000083U, 0x0000004CU, 0x00000010U, 0x0004003DU, 0x00000014U, 0x00000084U,
    0x00000083U, 0x00050050U, 0x0000002CU, 0x00000085U, 0x00000084U, 0x00000045U, 0x00050083U, 0x0000002CU,
    0x00000086U, 0x00000082U, 0x00000085U, 0x00070058U, 0x0000003CU, 0x00000087U, 0x00000081U, 0x00000086U,
    0x00000002U, 0x00000045U, 0x0003003EU, 0x00000080U, 0x00000087U, 0x0004003DU, 0x00000040U, 0x00000089U,
    0x00000048U, 0x0004003DU, 0x0000002CU, 0x0000008AU, 0x0000002EU, 0x00050041U, 0x0000005AU, 0x0000008BU,
    0x0000004CU, 0x00000021U, 0x0004003DU, 0x00000014U, 0x0000008CU, 0x0000008BU, 0x00050050U, 0x0000002CU,
    0x0000008DU, 0x00000045U, 0x0000008CU, 0x00050081U, 0x0000002CU, 0x0000008EU, 0x0000008AU, 0x0000008DU,
    0x00070058U, 0x0000003CU, 0x0000008FU, 0x00000089U, 0x0000008EU, 0x00000002U, 0x00000045U, 0x0003003EU,
    0x00000088U, 0x0000008FU, 0x0004003DU, 0x00000040U, 0x00000091U, 0x00000048U, 0x0004003DU, 0x0000002CU,
    0x00000092U, 0x0000002EU, 0x00050041U, 0x0000005AU, 0x00000093U, 0x0000004CU, 0x00000021U, 0x0004003DU,
    0x00000014U, 0x00000094U, 0x00000093U, 0x00050050U, 0x0000002CU, 0x00000095U, 0x00000045U, 0x00000094U,
    0x00050083U, 0x0000002CU, 0x00000096U, 0x00000092U, 0x00000095U, 0x00070058U, 0x0000003CU, 0x00000097U,
    0x00000091U, 0x00000096U, 0x00000002U, 0x00000045U, 0x0003003EU, 0x00000090U, 0x00000097U, 0x0004003DU,
    0x0000003CU, 0x0000009AU, 0x0000003EU, 0x0008004FU, 0x00000099U, 0x0000009BU, 0x0000009AU, 0x0000009AU,
    0x00000000U, 0x00000001U, 0x00000002U, 0x00050094U, 0x00000014U, 0x000000A0U, 0x0000009BU, 0x0000009FU,
    0x0003003EU, 0x00000098U, 0x000000A0U, 0x0004003DU, 0x0000003CU, 0x000000A2U, 0x00000047U, 0x0008004FU,
    0x00000099U, 0x000000A3U, 0x000000A2U, 0x000000A2U, 0x00000000U, 0x00000001U, 0x00000002U, 0x00050094U,
    0x00000014U, 0x000000A4U, 0x000000A3U, 0x0000009FU, 0x0003003EU, 0x000000A1U, 0x000000A4U, 0x0004003DU,
    0x0000003CU, 0x000000A6U, 0x00000078U, 0x0008004FU, 0x00000099U, 0x000000A7U, 0x000000A6U, 0x000000A6U,
    0x00000000U, 0x00000001U, 0x00000002U, 0x0004003DU, 0x0000003CU, 0x000000A8U, 0x00000080U, 0x0008004FU,
    0x00000099U, 0x000000A9U, 0x000000A8U, 0x000000A8U, 0x00000000U, 0x00000001U, 0x00000002U, 0x00050083U,
    0x00000099U, 0x000000AAU, 0x000000A7U, 0x000000A9U, 0x00050094U, 0x00000014U, 0x000000ABU, 0x000000AAU,
    0x0000009FU, 0x0004003DU, 0x0000003CU, 0x000000ACU, 0x00000057U, 0x0008004FU, 0x00000099U, 0x000000ADU,
    0x000000ACU, 0x000000ACU, 0x00000000U, 0x00000001U, 0x00000002U, 0x0004003DU, 0x0000003CU, 0x000000AEU,
    0x00000060U, 0x0008004FU, 0x00000099U, 0x000000AFU, 0x000000AEU, 0x000000AEU, 0x00000000U, 0x00000001U,
    0x00000002U, 0x00050083U, 0x00000099U, 0x000000B0U, 0x000000ADU, 0x000000AFU, 0x00050094U, 0x00000014U,
    0x000000B1U, 0x000000B0U, 0x0000009FU, 0x00050081U, 0x00000014U, 0x000000B2U, 0x000000ABU, 0x000000B1U,
    0x00050085U, 0x00000014U, 0x000000B3U, 0x000000B2U, 0x00000031U, 0x0004003DU, 0x0000003CU, 0x000000B4U,
    0x00000088U, 0x0008004FU, 0x00000099U, 0x000000B5U, 0x000000B4U, 0x000000B4U, 0x00000000U, 0x00000001U,
    0x00000002U, 0x0004003DU, 0x0000003CU, 0x000000B6U, 0x00000090U, 0x0008004FU, 0x00000099U, 0x000000B7U,
    0x000000B6U, 0x000000B6U, 0x00000000U, 0x00000001U, 0x00000002U, 0x00050083U, 0x00000099U, 0x000000B8U,
    0x000000B5U, 0x000000B7U, 0x00050094U, 0x00000014U, 0x000000B9U, 0x000000B8U, 0x0000009FU, 0x0004003DU,
    0x0000003CU, 0x000000BAU, 0x00000068U, 0x0008004FU, 0x00000099U, 0x000000BBU, 0x000000BAU, 0x000000BAU,
    0x00000000U, 0x00000001U, 0x00000002U, 0x0004003DU, 0x0000003CU, 0x000000BCU, 0x00000070U, 0x0008004FU,
    0x00000099U, 0x000000BDU, 0x000000BCU, 0x000000BCU, 0x00000000U, 0x00000001U, 0x00000002U, 0x00050083U,
    0x00000099U, 0x000000BEU, 0x000000BBU, 0x000000BDU, 0x00050094U, 0x00000014U, 0x000000BFU, 0x000000BEU,
    0x0000009FU, 0x00050081U, 0x00000014U, 0x000000C0U, 0x000000B9U, 0x000000BFU, 0x00050085U, 0x00000014U,
    0x000000C1U, 0x000000C0U, 0x00000031U, 0x00050050U, 0x0000002CU, 0x000000C2U, 0x000000B3U, 0x000000C1U,
    0x0003003EU, 0x000000A5U, 0x000000C2U, 0x0004003DU, 0x0000002CU, 0x000000C4U, 0x000000A5U, 0x0004003DU,
    0x0000002CU, 0x000000C5U, 0x000000A5U, 0x00050094U, 0x00000014U, 0x000000C6U, 0x000000C4U, 0x000000C5U,
    0x00050081U, 0x00000014U, 0x000000C8U, 0x000000C6U, 0x000000C7U, 0x0003003EU, 0x000000C3U, 0x000000C8U,
    0x0004003DU, 0x00000014U, 0x000000CAU, 0x00000098U, 0x0004003DU, 0x00000014U, 0x000000CBU, 0x000000A1U,
    0x00050083U, 0x00000014U, 0x000000CCU, 0x000000CAU, 0x000000CBU, 0x0004003DU, 0x0000002CU, 0x000000CDU,
    0x000000A5U, 0x0005008EU, 0x0000002CU, 0x000000CEU, 0x000000CDU, 0x000000CCU, 0x0004003DU, 0x00000014U,
    0x000000CFU, 0x000000C3U, 0x00050050U, 0x0000002CU, 0x000000D0U, 0x000000CFU, 0x000000CFU, 0x00050088U,
    0x0000002CU, 0x000000D1U, 0x000000CEU, 0x000000D0U, 0x0003003EU, 0x000000C9U, 0x000000D1U, 0x0004003DU,
    0x0000002CU, 0x000000D2U, 0x000000C9U, 0x00050041U, 0x0000005AU, 0x000000D4U, 0x0000004CU, 0x00000010U,
    0x0004003DU, 0x00000014U, 0x000000D5U, 0x000000D4U, 0x00050085U, 0x00000014U, 0x000000D6U, 0x000000D3U,
    0x000000D5U, 0x00050041U, 0x0000005AU, 0x000000D7U, 0x0000004CU, 0x00000021U, 0x0004003DU, 0x00000014U,
    0x000000D8U, 0x000000D7U, 0x00050085U, 0x00000014U, 0x000000D9U, 0x000000D3U, 0x000000D8U, 0x00050050U,
    0x0000002CU, 0x000000DAU, 0x000000D6U, 0x000000D9U, 0x00050041U, 0x0000005AU, 0x000000DCU, 0x0000004CU,
    0x00000010U, 0x0004003DU, 0x00000014U, 0x000000DDU, 0x000000DCU, 0x00050085U, 0x00000014U, 0x000000DEU,
    0x000000DBU, 0x000000DDU, 0x00050041U, 0x0000005AU, 0x000000DFU, 0x0000004CU, 0x00000021U, 0x0004003DU,
    0x00000014U, 0x000000E0U, 0x000000DFU, 0x00050085U, 0x00000014U, 0x000000E1U, 0x000000DBU, 0x000000E0U,
    0x00050050U, 0x0000002CU, 0x000000E2U, 0x000000DEU, 0x000000E1U, 0x0008000CU, 0x0000002CU, 0x000000E3U,
    0x00000001U, 0x0000002BU, 0x000000D2U, 0x000000DAU, 0x000000E2U, 0x0003003EU, 0x000000C9U, 0x000000E3U,
    0x0004003DU, 0x00000040U, 0x000000E5U, 0x00000042U, 0x0004003DU, 0x0000002CU, 0x000000E6U, 0x0000002EU,
    0x0004003DU, 0x0000002CU, 0x000000E7U, 0x000000C9U, 0x00050041U, 0x000000E9U, 0x000000EAU, 0x00000017U,
    0x000000E8U, 0x0004003DU, 0x00000014U, 0x000000EBU, 0x000000EAU, 0x00050083U, 0x00000014U, 0x000000ECU,
    0x0000004DU, 0x000000EBU, 0x0005008EU, 0x0000002CU, 0x000000EDU, 0x000000E7U, 0x000000ECU, 0x00050081U,
    0x0000002CU, 0x000000EEU, 0x000000E6U, 0x000000EDU, 0x00070058U, 0x0000003CU, 0x000000EFU, 0x000000E5U,
    0x000000EEU, 0x00000002U, 0x00000045U, 0x0003003EU, 0x000000E4U, 0x000000EFU, 0x0004003DU, 0x00000040U,
    0x000000F1U, 0x00000048U, 0x0004003DU, 0x0000002CU, 0x000000F2U, 0x0000002EU, 0x0004003DU, 0x0000002CU,
    0x000000F3U, 0x000000C9U, 0x00050041U, 0x000000E9U, 0x000000F4U, 0x00000017U, 0x000000E8U, 0x0004003DU,
    0x00000014U, 0x000000F5U, 0x000000F4U, 0x0005008EU, 0x0000002CU, 0x000000F6U, 0x000000F3U, 0x000000F5U,
    0x00050083U, 0x0000002CU, 0x000000F7U, 0x000000F2U, 0x000000F6U, 0x00070058U, 0x0000003CU, 0x000000F8U,
    0x000000F1U, 0x000000F7U, 0x00000002U, 0x00000045U, 0x0003003EU, 0x000000F0U, 0x000000F8U, 0x0004003DU,
    0x0000003CU, 0x000000FAU, 0x000000E4U, 0x0004003DU, 0x0000003CU, 0x000000FBU, 0x000000F0U, 0x00050041U,
    0x000000E9U, 0x000000FCU, 0x00000017U, 0x000000E8U, 0x0004003DU, 0x00000014U, 0x000000FDU, 0x000000FCU,
    0x00070050U, 0x0000003CU, 0x000000FEU, 0x000000FDU, 0x000000FDU, 0x000000FDU, 0x000000FDU, 0x0008000CU,
    0x0000003CU, 0x000000FFU, 0x00000001U, 0x0000002EU, 0x000000FAU, 0x000000FBU, 0x000000FEU, 0x0003003EU,
    0x000000F9U, 0x000000FFU, 0x00050041U, 0x0000001AU, 0x00000101U, 0x00000017U, 0x00000100U, 0x0004003DU,
    0x00000006U, 0x00000102U, 0x00000101U, 0x000500ABU, 0x0000000FU, 0x00000103U, 0x00000102U, 0x00000010U,
    0x000300F7U, 0x00000105U, 0x00000000U, 0x000400FAU, 0x00000103U, 0x00000104U, 0x00000105U, 0x000200F8U,
    0x00000104U, 0x0004003DU, 0x0000003CU, 0x00000106U, 0x000000F9U, 0x0009004FU, 0x0000003CU, 0x00000107U,
    0x00000106U, 0x00000106U, 0x00000002U, 0x00000001U, 0x00000000U, 0x00000003U, 0x0003003EU, 0x000000F9U,
    0x00000107U, 0x000200F9U, 0x00000105U, 0x000200F8U, 0x00000105U, 0x0004003DU, 0x00000108U, 0x0000010BU,
    0x0000010AU, 0x0004003DU, 0x00000007U, 0x0000010CU, 0x00000009U, 0x0004007CU, 0x0000010DU, 0x0000010EU,
    0x0000010CU, 0x0004003DU, 0x0000003CU, 0x0000010FU, 0x000000F9U, 0x00040063U, 0x0000010BU, 0x0000010EU,
    0x0000010FU, 0x000100FDU, 0x00010038U,
};

} // Anonymous namespace

FrameGen::FrameGen(MemoryAllocator& memory_allocator_, Scheduler& scheduler_)
    : memory_allocator{memory_allocator_}, scheduler{scheduler_} {}

FrameGen::~FrameGen() = default;

void FrameGen::Process(const Device& device, Frame* frame, VkFormat format,
                       VkExtent2D guest_extent) {
    generated = false;

    const bool fg_active = Settings::values.frame_gen.GetValue() || Settings::values.enable_frame_interpolation.GetValue();
    if (fg_active && !last_fg_active) {
        unavailable = false;
    }
    last_fg_active = fg_active;

    if (unavailable || !fg_active) {
        if (chain) {
            scheduler.Finish();
            chain.reset();
        }
        warm_streak = 0;
        return;
    }

    if (!frame->storage_view) {
        // Skip transitional frames safely without latching unavailable
        return;
    }

    try {
        if (!shaders) {
            shaders.emplace(device);
            has_lsfg = shaders->IsValid();
        }

        peak_guest_extent.width = std::max(peak_guest_extent.width, guest_extent.width);
        peak_guest_extent.height = std::max(peak_guest_extent.height, guest_extent.height);

        const VkExtent2D extent{.width = frame->width, .height = frame->height};

        if (has_lsfg) {
            const f32 flow_scale = ConfiguredFlowScale(device, peak_guest_extent, extent);
            if (!chain || built_extent.width != extent.width || built_extent.height != extent.height ||
                built_format != format || built_flow_scale != flow_scale) {
                Rebuild(device, extent, format, flow_scale);
            }
        } else {
            if (fallback_frames[0].Handle() == VK_NULL_HANDLE || built_extent.width != extent.width ||
                built_extent.height != extent.height || built_format != format) {
                RebuildFallback(device, extent, format);
            }
        }

        const u64 count = frame_count++;
        last_count = count;
        last_generations = plan.generations;

        const bool warm = plan.warm && count + 1 >= LSFG_REQUIRED_FRAMES;
        warm_streak = warm ? warm_streak + 1 : 0;
        // Warm-up guard: require at least 2 presented frames before activating frame generation
        generated = warm && warm_streak >= 2 && plan.generations > 0;

        scheduler.RequestOutsideRenderPassOperationContext();
        scheduler.Record([this, source = *frame->image, extent, count,
                          dispatch = warm](vk::CommandBuffer cmdbuf) {
            if (has_lsfg && chain) {
                CopyPresentedFrame(cmdbuf, source, chain->Input(count), extent);
                if (dispatch) {
                    chain->DispatchShared(cmdbuf, count);
                }
            } else if (!has_lsfg && fallback_frames[0].Handle() != VK_NULL_HANDLE) {
                CopyPresentedFrame(cmdbuf, source, fallback_frames[count % 2], extent);
            }
        });

        const bool dump_requested = generated && Settings::values.frame_gen_dump_flow.GetValue();
        if (!dump_requested) {
            dumped = false;
        } else if (!dumped) {
            if (has_lsfg && chain) {
                DumpDebugImages(count);
                dumped = true;
            }
        }
    } catch (const std::exception& ex) {
        LOG_ERROR(Render_Vulkan, "FrameGen::Process failed: {}", ex.what());
        unavailable = true;
        chain.reset();
        generated = false;
    } catch (...) {
        LOG_ERROR(Render_Vulkan, "FrameGen::Process failed with unknown exception");
        unavailable = true;
        chain.reset();
        generated = false;
    }
}

size_t FrameGen::WantedGenerations(size_t capacity) {
    if (unavailable) {
        plan = {};
        return 0;
    }
    plan = pacer.Plan(capacity);
    return plan.generations;
}

size_t FrameGen::GeneratedFrameCount() const {
    return generated ? last_generations : 0;
}

void FrameGen::GenerateInto(const Device& device, Frame* destination, size_t generation) {
    if (!destination || !destination->storage_view || !destination->image) {
        return;
    }
    if (has_lsfg && chain) {
        try {
            chain->SetTarget(device, last_generations, generation, destination->index,
                             *destination->storage_view);

            const VkExtent2D extent{.width = destination->width, .height = destination->height};

            scheduler.RequestOutsideRenderPassOperationContext();
            scheduler.Record([this, count = last_count, generation_count = last_generations, generation,
                              target = destination->index, image = *destination->image,
                              extent](vk::CommandBuffer cmdbuf) {
                if (chain) {
                    chain->DispatchGeneration(cmdbuf, count, generation_count, generation, target, image,
                                              extent);
                }
            });
        } catch (const std::exception& ex) {
            LOG_ERROR(Render_Vulkan, "FrameGen::GenerateInto failed: {}", ex.what());
            unavailable = true;
            chain.reset();
        } catch (...) {
            LOG_ERROR(Render_Vulkan, "FrameGen::GenerateInto failed with unknown exception");
            unavailable = true;
            chain.reset();
        }
        return;
    }

    // Native Vulkan Frame Interpolation Fallback Path
    if (fallback_frames[0].Handle() == VK_NULL_HANDLE || !fallback_pipeline) {
        return;
    }
    try {
        const VkExtent2D extent{.width = destination->width, .height = destination->height};
        const u32 target_idx = destination->index % static_cast<u32>(fallback_sets.size());
        const auto& dld = device.GetLogical();

        const VkDescriptorImageInfo prev_info{
            .sampler = *fallback_sampler,
            .imageView = fallback_frames[(last_count + 1) % 2].View(),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        const VkDescriptorImageInfo curr_info{
            .sampler = *fallback_sampler,
            .imageView = fallback_frames[last_count % 2].View(),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        const VkDescriptorImageInfo out_info{
            .sampler = VK_NULL_HANDLE,
            .imageView = *destination->storage_view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        const std::array writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = fallback_sets[target_idx],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &prev_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = fallback_sets[target_idx],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &curr_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = fallback_sets[target_idx],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &out_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
        };
        dld.UpdateDescriptorSets(writes, {});

        scheduler.RequestOutsideRenderPassOperationContext();
        scheduler.Record([this, target_idx, image = *destination->image, extent, generation](vk::CommandBuffer cmdbuf) {
            const auto target_barrier = MakeTransitionBarrier(
                image, 0, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   0, {}, {}, target_barrier);

            cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *fallback_pipeline);
            cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *fallback_pipeline_layout,
                                      0, fallback_sets[target_idx], {});

            struct PushConstants {
                float blend_factor;
                u32 width;
                u32 height;
                u32 is_blue_first;
            } pc{
                .blend_factor = (last_generations > 0) ? static_cast<float>(generation + 1) / static_cast<float>(last_generations + 1) : 0.5f,
                .width = extent.width,
                .height = extent.height,
                .is_blue_first = IsBlueFirst(built_format) ? 1u : 0u,
            };
            cmdbuf.PushConstants(*fallback_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

            const u32 groups_x = (extent.width + 15) / 16;
            const u32 groups_y = (extent.height + 15) / 16;
            cmdbuf.Dispatch(groups_x, groups_y, 1);

            const auto after_barrier = MakeTransitionBarrier(
                image, VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, {}, {}, after_barrier);
        });
    } catch (...) {
        // Fallback execution failed non-critically
    }
}

void FrameGen::Rebuild(const Device& device, VkExtent2D extent, VkFormat format, f32 flow_scale) {
    scheduler.Finish();
    chain.reset();

    built_flow_scale = flow_scale;

    chain.emplace(device, memory_allocator, *shaders, extent, format, built_flow_scale);
    built_extent = extent;
    built_format = format;
    frame_count = 0;
    warm_streak = 0;
    generated = false;
    unavailable = false;
}

void FrameGen::RebuildFallback(const Device& device, VkExtent2D extent, VkFormat format) {
    scheduler.Finish();

    fallback_frames[0] = LsfgImage(device, memory_allocator, extent, format);
    fallback_frames[1] = LsfgImage(device, memory_allocator, extent, format);

    if (!fallback_pipeline) {
        auto shader_module = CreateWrappedShaderModule(device, FALLBACK_FRAME_INTERPOLATION_SPV);
        auto& dld = device.GetLogical();

        const std::array bindings{
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = nullptr,
            },
        };
        fallback_set_layout = dld.CreateDescriptorSetLayout({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<u32>(bindings.size()),
            .pBindings = bindings.data(),
        });

        const VkPushConstantRange push_range{
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = 16,
        };
        fallback_pipeline_layout = dld.CreatePipelineLayout({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = fallback_set_layout.address(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_range,
        });

        fallback_pipeline = dld.CreateComputePipeline({
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = *shader_module,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            },
            .layout = *fallback_pipeline_layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        });

        fallback_sampler = CreateLsfgSampler(
            device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_NEVER, false);

        const std::array pool_sizes{
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = static_cast<u32>(LSFG_MAX_TARGETS * 2),
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = static_cast<u32>(LSFG_MAX_TARGETS),
            },
        };
        fallback_descriptor_pool = dld.CreateDescriptorPool({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = static_cast<u32>(LSFG_MAX_TARGETS),
            .poolSizeCount = static_cast<u32>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        });

        const std::vector<VkDescriptorSetLayout> layouts(LSFG_MAX_TARGETS, *fallback_set_layout);
        fallback_sets = CreateWrappedDescriptorSets(fallback_descriptor_pool, layouts);
    }

    built_extent = extent;
    built_format = format;
    frame_count = 0;
    warm_streak = 0;
    generated = false;
    unavailable = false;
}

void FrameGen::DumpDebugImages(u64 count) {
    const std::filesystem::path directory =
        Common::FS::GetEdenPath(Common::FS::EdenPath::LosslessDir) / "debug";
    if (!Common::FS::CreateDirs(directory)) {
        return;
    }

    const auto dump = [&](const std::string& name, LsfgImage& image) {
        const VkExtent2D extent = image.Extent();
        const VkFormat format = image.Format();
        const VkDeviceSize texel_size = BytesPerTexel(format);
        const VkDeviceSize size =
            static_cast<VkDeviceSize>(extent.width) * extent.height * texel_size;

        vk::Buffer buffer = CreateWrappedBuffer(memory_allocator, size, MemoryUsage::Download);

        scheduler.RequestOutsideRenderPassOperationContext();
        scheduler.Record(
            [handle = image.Handle(), dst = *buffer, extent](vk::CommandBuffer cmdbuf) {
                DownloadColorImage(
                    cmdbuf, handle, dst,
                    VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1});
            });
        scheduler.Finish();

        buffer.Invalidate();
        const std::span<u8> mapped = buffer.Mapped();

        if (format == LSFG_FLOW_FORMAT) {
            WriteGrayscalePgm(directory / (name + ".pgm"), extent, mapped);
        } else if (texel_size == COLOR_CHANNELS) {
            WriteColorPpm(directory / (name + ".ppm"), extent, mapped, IsBlueFirst(format));
        } else {
            WriteRaw(directory / (name + "_" + std::to_string(extent.width) + "x" +
                                  std::to_string(extent.height) + ".f16"),
                     mapped.subspan(0, std::min<size_t>(size, mapped.size())));
        }
    };

    dump("in0", chain->Input(0));
    dump("in1", chain->Input(1));

    for (size_t level = 0; level < LSFG_MIP_LEVELS; ++level) {
        dump("flow_mip" + std::to_string(level), chain->FlowLevel(level));
    }
    for (size_t index = 0; index < 2; ++index) {
        dump("alpha0_" + std::to_string(index), chain->AlphaOutput(0, count, index));
        dump("alpha6_" + std::to_string(index), chain->AlphaOutput(LSFG_MIP_LEVELS - 1, count,
                                                                  index));
    }
    for (size_t level = 0; level < LSFG_BETA_OUTPUTS; ++level) {
        dump("beta_" + std::to_string(level), chain->BetaOutput(level));
    }

    dump("gamma0", chain->GammaOutput(0));
    dump("gamma6", chain->GammaOutput(LSFG_MIP_LEVELS - 1));
    dump("delta2_out1", chain->DeltaOutput1(LSFG_DELTA_INSTANCES - 1));
    dump("delta2_out2", chain->DeltaOutput2(LSFG_DELTA_INSTANCES - 1));
}

} // namespace Vulkan
