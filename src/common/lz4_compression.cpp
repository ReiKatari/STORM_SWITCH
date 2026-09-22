// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <lz4hc.h>

#include "common/assert.h"
#include "common/lz4_compression.h"

namespace Common::Compression {

std::vector<u8> CompressDataLZ4(const u8* source, std::size_t source_size) {
    ASSERT_MSG(source_size <= LZ4_MAX_INPUT_SIZE, "Source size exceeds LZ4 maximum input size");

    const auto source_size_int = static_cast<int>(source_size);
    const auto max_compressed_size = static_cast<std::size_t>(LZ4_compressBound(source_size_int));
    std::vector<u8> compressed(max_compressed_size);

    const int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(source), reinterpret_cast<char*>(compressed.data()),
        source_size_int, static_cast<int>(max_compressed_size));

    if (compressed_size <= 0) {
        // Compression failed
        return {};
    }

    compressed.resize(static_cast<std::size_t>(compressed_size));

    return compressed;
}

std::vector<u8> CompressDataLZ4HC(const u8* source, std::size_t source_size,
                                  s32 compression_level) {
    ASSERT_MSG(source_size <= LZ4_MAX_INPUT_SIZE, "Source size exceeds LZ4 maximum input size");

    compression_level = std::clamp(compression_level, LZ4HC_CLEVEL_MIN, LZ4HC_CLEVEL_MAX);

    const auto source_size_int = static_cast<int>(source_size);
    const auto max_compressed_size = static_cast<std::size_t>(LZ4_compressBound(source_size_int));
    std::vector<u8> compressed(max_compressed_size);

    const int compressed_size = LZ4_compress_HC(
        reinterpret_cast<const char*>(source), reinterpret_cast<char*>(compressed.data()),
        source_size_int, static_cast<int>(max_compressed_size), compression_level);

    if (compressed_size <= 0) {
        // Compression failed
        return {};
    }

    compressed.resize(static_cast<std::size_t>(compressed_size));

    return compressed;
}

std::vector<u8> CompressDataLZ4HCMax(const u8* source, std::size_t source_size) {
    return CompressDataLZ4HC(source, source_size, LZ4HC_CLEVEL_MAX);
}

#include <cstring>
#include <lz4.h>
#include <lz4frame.h>

std::vector<u8> DecompressDataLZ4(std::span<const u8> compressed, std::size_t uncompressed_size) {
    std::vector<u8> uncompressed(uncompressed_size);
    const int r = DecompressDataLZ4(uncompressed.data(), uncompressed_size, compressed.data(), compressed.size());
    if (r <= 0) {
        return {};
    }
    if (static_cast<size_t>(r) < uncompressed_size) {
        uncompressed.resize(static_cast<size_t>(r));
    }
    return uncompressed;
}

int DecompressDataLZ4(void* dst, size_t dst_size, const void* src, size_t src_size) {
    if (!dst || !src || dst_size == 0 || src_size == 0) {
        return -1;
    }

    // 1. Check for LZ4 Frame header (magic 0x184D2204)
    if (src_size >= 4) {
        u32 magic = 0;
        std::memcpy(&magic, src, sizeof(u32));
        if (magic == LZ4F_MAGICNUMBER) {
            LZ4F_dctx* dctx = nullptr;
            const auto err = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
            if (!LZ4F_isError(err) && dctx) {
                size_t src_consumed = src_size;
                size_t dst_capacity = dst_size;
                const auto res = LZ4F_decompress(dctx, dst, &dst_capacity, src, &src_consumed, nullptr);
                LZ4F_freeDecompressionContext(dctx);
                if (!LZ4F_isError(res) && dst_capacity > 0) {
                    return static_cast<int>(dst_capacity);
                }
            }
        }
    }

    // 2. Standard LZ4 block decompression
    int res = LZ4_decompress_safe(reinterpret_cast<const char*>(src), reinterpret_cast<char*>(dst),
                                  static_cast<int>(src_size), static_cast<int>(dst_size));
    if (res > 0) {
        return res;
    }

    // 3. Partial safe decompression (block decoding stops when dst_size is satisfied)
    res = LZ4_decompress_safe_partial(reinterpret_cast<const char*>(src), reinterpret_cast<char*>(dst),
                                      static_cast<int>(src_size), static_cast<int>(dst_size),
                                      static_cast<int>(dst_size));
    if (res > 0) {
        return res;
    }

    // 4. Try skipping a 4-byte prefix (e.g. uncompressed size header added by some tools)
    if (src_size > 4) {
        res = LZ4_decompress_safe(reinterpret_cast<const char*>(src) + 4, reinterpret_cast<char*>(dst),
                                  static_cast<int>(src_size - 4), static_cast<int>(dst_size));
        if (res > 0) {
            return res;
        }
        res = LZ4_decompress_safe_partial(reinterpret_cast<const char*>(src) + 4, reinterpret_cast<char*>(dst),
                                          static_cast<int>(src_size - 4), static_cast<int>(dst_size),
                                          static_cast<int>(dst_size));
        if (res > 0) {
            return res;
        }
    }

    // 5. Try skipping an 8-byte prefix
    if (src_size > 8) {
        res = LZ4_decompress_safe(reinterpret_cast<const char*>(src) + 8, reinterpret_cast<char*>(dst),
                                  static_cast<int>(src_size - 8), static_cast<int>(dst_size));
        if (res > 0) {
            return res;
        }
    }

    // 6. Fast decompression fallback (for blocks generated with legacy loose bounds)
    res = LZ4_decompress_fast(reinterpret_cast<const char*>(src), reinterpret_cast<char*>(dst),
                              static_cast<int>(dst_size));
    if (res > 0) {
        return static_cast<int>(dst_size);
    }

    return -1;
}

} // namespace Common::Compression
