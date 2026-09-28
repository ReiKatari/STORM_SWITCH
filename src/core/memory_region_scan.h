// SPDX-FileCopyrightText: Copyright 2026 Lemon-Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "common/common_types.h"

namespace Kernel {
class KProcessPageTable;
enum class KMemoryState : u32;
} // namespace Kernel

namespace Core {

struct MemoryRegion {
    u64 address;
    u64 size;
    Kernel::KMemoryState state;
};

// Only these carry actual guest-written data worth scanning/capturing. Everything else
// (Free, Inaccessible, Kernel, Io*, Ipc/Transferred buffers, ASLR guard gaps, etc.) is
// either unbacked or not meaningful, and including it risks probing enormous unbacked
// address ranges.
//
// Code is deliberately excluded: under NCE those pages are directly-executable host
// memory, and writing new bytes into them doesn't invalidate the CPU's instruction
// cache, so the core can end up fetching stale instructions and fault with SIGILL.
bool IsScannableMemoryState(Kernel::KMemoryState state);

// Walks the process' memory via the same QueryInfo mechanism svcQueryMemory uses,
// collecting every region in a scannable state (see IsScannableMemoryState). Avoids
// probing the full 39-bit address space byte by byte.
std::vector<MemoryRegion> EnumerateScannableRegions(Kernel::KProcessPageTable& page_table);

} // namespace Core
