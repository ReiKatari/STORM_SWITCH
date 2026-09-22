// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT / ReiKatari
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/logging.h"
#include "common/zbic.h"
#include "common/zbic_compression.h"

namespace Common::Compression {

bool IsZBIC(const void* src, size_t src_size) {
    if (!src || src_size < 4) {
        return false;
    }
    u32 magic = 0;
    std::memcpy(&magic, src, sizeof(u32));
    return magic == ZBIC_MAGICNUMBER; // 0x4349425A ("ZBIC")
}

int DecompressDataZBIC(void* dst, size_t dst_size, const void* src, size_t src_size) {
    if (!dst || !src || dst_size == 0 || src_size == 0) {
        return -1;
    }
    const size_t res = ZBIC_decompress(dst, dst_size, src, src_size);
    if (ZBIC_isError(res)) {
        LOG_ERROR(Common, "ZBIC_decompress failed: {} (code: {:#x})", ZBIC_getErrorString(ZBIC_getErrorCode(res)), res);
        return -1;
    }
    return static_cast<int>(res);
}

std::vector<u8> DecompressDataZBIC(std::span<const u8> compressed, std::size_t uncompressed_size) {
    std::vector<u8> uncompressed(uncompressed_size);
    const int r = DecompressDataZBIC(uncompressed.data(), uncompressed_size, compressed.data(), compressed.size());
    if (r <= 0) {
        return {};
    }
    if (static_cast<size_t>(r) < uncompressed_size) {
        uncompressed.resize(static_cast<size_t>(r));
    }
    return uncompressed;
}

} // namespace Common::Compression
