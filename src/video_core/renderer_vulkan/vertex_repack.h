// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <optional>
#include "common/common_types.h"

namespace Vulkan {

constexpr u32 RepackSourceStride = 69;
constexpr u32 RepackDestStride = 72;

struct VertexRepackParameters {
    u32 src_offset_bytes;
    u32 dst_offset_bytes;
    u32 vertex_count;
    u64 source_range;
    u64 dest_range;
};

inline u32 VertexRepackChunkVertices(u64 alignment, u64 max_storage_range, u32 max_dispatch_groups) {
    const u64 limit_vertices = (std::min<u64>)(max_storage_range / RepackDestStride,
                                        static_cast<u64>(max_dispatch_groups) * 64ULL);
    // Align vertex count such that (vertex_count * 72) % alignment == 0
    u32 vertices = static_cast<u32>((std::min<u64>)(limit_vertices, 65536ULL));
    while (vertices > 0 && ((static_cast<u64>(vertices) * RepackDestStride) % alignment != 0)) {
        --vertices;
    }
    return vertices > 0 ? vertices : 64U;
}

inline std::optional<VertexRepackParameters> MakeVertexRepackParameters(
    u32 offset, u64 size, u64 alignment, u64 max_storage_range, u32 max_dispatch_groups) {
    if (size < RepackSourceStride) return std::nullopt;
    const u32 vertex_count = static_cast<u32>(size / RepackSourceStride);
    const u64 dst_size = static_cast<u64>(vertex_count) * RepackDestStride;
    if (dst_size > max_storage_range) return std::nullopt;

    VertexRepackParameters params{};
    params.src_offset_bytes = offset;
    params.dst_offset_bytes = 0;
    params.vertex_count = vertex_count;
    params.source_range = size;
    params.dest_range = dst_size;
    return params;
}

} // namespace Vulkan
