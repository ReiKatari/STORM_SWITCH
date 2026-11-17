// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <optional>

#include "common/common_types.h"

namespace Common {

struct MemoryInfo {
    u64 TotalPhysicalMemory{};
    u64 TotalSwapMemory{};
};

/**
 * Gets the memory info of the host system
 * @return Reference to a MemoryInfo struct with the physical and swap memory sizes in bytes
 */
[[nodiscard]] const MemoryInfo& GetMemInfo();

/// Physical memory the host can hand out now without paging anything out (0 when unknown).
[[nodiscard]] u64 GetAvailablePhysicalMemory();

/// Whether the host is short of RAM: less than a quarter of it, and less than 4 GiB, free.
[[nodiscard]] constexpr bool IsLowOnMemory(u64 available, u64 total) {
    return available != 0 && available < (std::min<u64>)(total / 4, u64{4} << 30);
}

/// IsLowOnMemory for this host, read again at most once a second.
[[nodiscard]] bool IsHostLowOnMemory();

} // namespace Common
