// SPDX-FileCopyrightText: Copyright 2026 Lemon-Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/typed_address.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_process_page_table.h"
#include "core/hle/kernel/svc_types.h"
#include "core/memory_region_scan.h"

namespace Core {

bool IsScannableMemoryState(Kernel::KMemoryState state) {
    switch (state) {
    case Kernel::KMemoryState::CodeData:
    case Kernel::KMemoryState::Normal:
    case Kernel::KMemoryState::Stack:
    case Kernel::KMemoryState::ThreadLocal:
    case Kernel::KMemoryState::Shared:
        return true;
    default:
        return false;
    }
}

std::vector<MemoryRegion> EnumerateScannableRegions(Kernel::KProcessPageTable& page_table) {
    std::vector<MemoryRegion> regions;

    const auto start = page_table.GetAddressSpaceStart();
    const auto end = start + page_table.GetAddressSpaceSize();

    auto addr = start;
    while (addr < end) {
        Kernel::KMemoryInfo info{};
        Kernel::Svc::PageInfo page_info{};
        if (page_table.QueryInfo(&info, &page_info, addr).IsError()) {
            break;
        }

        if (IsScannableMemoryState(info.m_state)) {
            regions.push_back(
                {.address = info.m_address, .size = info.m_size, .state = info.m_state});
        }

        const Common::ProcessAddress next = info.m_address + info.m_size;
        if (next <= addr) {
            // Didn't advance - stop rather than loop forever.
            break;
        }
        addr = next;
    }

    return regions;
}

} // namespace Core
