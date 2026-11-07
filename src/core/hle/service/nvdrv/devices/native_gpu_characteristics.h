// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstring>
#include <span>
#include "common/common_types.h"
#include "common/swap.h"

namespace Service::Nvidia::Devices {

inline constexpr size_t NativeGpuCharacteristicsSize = 0xd0;

inline std::array<u8, NativeGpuCharacteristicsSize> MakeNativeGpuCharacteristics(
    std::span<const u8, 0xa0> legacy) {
    std::array<u8, NativeGpuCharacteristicsSize> result{};
    // Copy legacy 0xa0 prefix (arch..gr_compbit_store_base_hw), preserving big_page_size=0x20000
    // (+0x28) and available_big_page_sizes=0x30000 (+0x34) expected by Ounce NvRm.
    std::memcpy(result.data(), legacy.data(), 0xa0);

    const u32_le native_arch{0x170};
    const u32_le native_impl{0xe};
    std::memcpy(result.data() + 0x00, &native_arch, sizeof(native_arch));
    std::memcpy(result.data() + 0x04, &native_impl, sizeof(native_impl));

    const u32_le num_tpc{0x6};
    std::memcpy(result.data() + 0x20, &num_tpc, sizeof(num_tpc));

    // NVN2 class lookup requires SM 0x808.
    const u32_le native_sm_version{0x808};
    std::memcpy(result.data() + 0x3c, &native_sm_version, sizeof(native_sm_version));
    std::memcpy(result.data() + 0x40, &native_sm_version, sizeof(native_sm_version));

    // Ampere hardware engine classes at exact IoctlGpuCharacteristics offsets (+0x58..+0x6c)
    const u32_le twod_class{0x902D};
    const u32_le threed_class{0xC797};      // AMPERE_B
    const u32_le compute_class{0xC7C0};     // AMPERE_COMPUTE_B
    const u32_le gpfifo_class{0xC56F};      // AMPERE_CHANNEL_GPFIFO_A
    const u32_le inline_class{0xA140};      // KEPLER_INLINE_TO_MEMORY_B
    const u32_le dma_copy_class{0xC7B5};    // AMPERE_DMA_COPY_B
    std::memcpy(result.data() + 0x58, &twod_class, sizeof(twod_class));
    std::memcpy(result.data() + 0x5c, &threed_class, sizeof(threed_class));
    std::memcpy(result.data() + 0x60, &compute_class, sizeof(compute_class));
    std::memcpy(result.data() + 0x64, &gpfifo_class, sizeof(gpfifo_class));
    std::memcpy(result.data() + 0x68, &inline_class, sizeof(inline_class));
    std::memcpy(result.data() + 0x6c, &dma_copy_class, sizeof(dma_copy_class));

    // Capability flags at +0x50: TSG (8), TSG_SUBCONTEXTS (22), SYNCPOINT_ADDRESS (26),
    // MAPPING_MODIFY (34), and Ampere engine family bits (40..43).
    u64_le flags;
    std::memcpy(&flags, legacy.data() + 0x50, sizeof(flags));
    flags = static_cast<u64>(flags) | (1ULL << 8) | (1ULL << 22) | (1ULL << 26) | (1ULL << 34) | (0xfULL << 40);
    std::memcpy(result.data() + 0x50, &flags, sizeof(flags));

    // Switch 2 keeps nine s16 interface versions at 0x70..0x80 (nvgpu *_ioctl_nr_last);
    // Ounce NvRm requires [0x78] >= 6 and enables TSG subcontexts / setup_bind above 0xa, 0x1f, 0x26.
    constexpr std::array<s16, 9> levels{
        0x1f, 0x0f, -1, 0x1d, 0x14, 0x0a, 0x0a, 0x0a, 0x0a,
    };
    for (size_t i = 0; i < levels.size(); ++i) {
        const u16_le value{static_cast<u16>(levels[i])};
        std::memcpy(result.data() + 0x70 + i * sizeof(value), &value, sizeof(value));
    }

    // Chipname "ga10f" at +0x90 and MaxSubcontexts=64 at +0xb4
    const u64_le chipname{0x6631306167ULL};
    std::memcpy(result.data() + 0x90, &chipname, sizeof(chipname));
    constexpr u32_le max_subcontexts{64};
    std::memcpy(result.data() + 0xb4, &max_subcontexts, sizeof(max_subcontexts));

    return result;
}

} // namespace Service::Nvidia::Devices
