// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include "common/common_types.h"

namespace Service::Nvidia::Devices {

// Native profile context identities. This classifies allocation metadata;
// engine method execution is handled separately by the GPU frontend.
inline std::optional<u32> GetNativeGpuContextSlot(u32 identity) {
    switch (identity) {
    case 0x902d: return 0;
    case 0xc597:
    case 0xc797: return 1;
    case 0xc5c0:
    case 0xc7c0: return 2;
    case 0xb140: return 3;
    case 0xc5b5:
    case 0xc7b5: return 4;
    case 0xc46f:
    case 0xc56f: return 5;
    default: return std::nullopt;
    }
}

} // namespace Service::Nvidia::Devices
