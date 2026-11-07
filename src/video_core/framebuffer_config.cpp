#include "common/settings.h"
// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging.h"
#include "video_core/framebuffer_config.h"

namespace Tegra {

Common::Rectangle<f32> NormalizeCrop(const FramebufferConfig& framebuffer, u32 texture_width,
                                     u32 texture_height) {
    if (texture_width == 0 || texture_height == 0) {
        return Common::Rectangle<f32>(0.f, 0.f, 1.f, 1.f);
    }

    f32 left, top, right, bottom;

    if (!framebuffer.crop_rect.IsEmpty()) {
        // If crop rectangle is not empty, apply properties from rectangle.
        left = static_cast<f32>(framebuffer.crop_rect.left);
        top = static_cast<f32>(framebuffer.crop_rect.top);
        right = static_cast<f32>(framebuffer.crop_rect.right);
        bottom = static_cast<f32>(framebuffer.crop_rect.bottom);
    } else {
        // Switch 2 handheld mode renders into top-left 1920x1080 of a 4K surface
        if (!Settings::IsDockedMode() && framebuffer.width == 3840 && framebuffer.height == 2160) {
            left = 0.f;
            top = 0.f;
            right = 1920.f;
            bottom = 1080.f;
        } else {
            // Otherwise, fall back to framebuffer dimensions.
            left = 0;
            top = 0;
            right = static_cast<f32>(framebuffer.width);
            bottom = static_cast<f32>(framebuffer.height);
        }
    }

    // Apply transformation flags.
    auto framebuffer_transform_flags = framebuffer.transform_flags;

    if (True(framebuffer_transform_flags & Service::android::BufferTransformFlags::FlipH)) {
        // Switch left and right.
        std::swap(left, right);
    }
    if (True(framebuffer_transform_flags & Service::android::BufferTransformFlags::FlipV)) {
        // Switch top and bottom.
        std::swap(top, bottom);
    }

    framebuffer_transform_flags &= ~Service::android::BufferTransformFlags::FlipH;
    framebuffer_transform_flags &= ~Service::android::BufferTransformFlags::FlipV;
    if (True(framebuffer_transform_flags)) {
        LOG_WARNING(HW_GPU, "Unsupported framebuffer_transform_flags={}",
                    static_cast<u32>(framebuffer_transform_flags));
    }

    // Normalize coordinate space.
    left /= static_cast<f32>(texture_width);
    top /= static_cast<f32>(texture_height);
    right /= static_cast<f32>(texture_width);
    bottom /= static_cast<f32>(texture_height);

    return Common::Rectangle<f32>(left, top, right, bottom);
}

} // namespace Tegra
