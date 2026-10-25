// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "common/common_types.h"

namespace Core {

class System;

/// Save state file format version (v2: Page-Table Writable Regions + 4KB Sparse + Instant RAM Cache)
constexpr u32 SAVE_STATE_VERSION = 2;

/// Save state magic header
constexpr u32 SAVE_STATE_MAGIC = 0x53455353; // "SESS" (Storm Eden Save State)

/// Header flags for SaveStateHeader::flags
enum SaveStateFlags : u32 {
    SaveStateFlag_SparsePages      = (1u << 0), // v1 64KB full-DRAM sparse encoding
    SaveStateFlag_WritableRegions  = (1u << 1), // v2 Page-Table writable region 4KB sparse encoding
    SaveStateFlag_HasSyncpoints    = (1u << 2), // v2 Host1x/NvCore 192 syncpoint snapshot
    SaveStateFlag_HasGpuSurfaces   = (1u << 3), // v2 Persistent GPU render target & dynamic texture 4KB sparse pages
};

/// Header for save state files (.ses)
struct SaveStateHeader {
    u32 magic{SAVE_STATE_MAGIC};
    u32 version{SAVE_STATE_VERSION};
    u64 title_id{};
    u64 dram_size{};
    u64 dram_compressed_size{};
    u32 num_threads{};
    u32 compression_level{};
    u64 timestamp{};
    u32 flags{};             // Bit 0: v1 Sparse, Bit 1: v2 Writable Regions (4KB chunks)
    u32 sparse_page_size{};  // Chunk size (4096 in v2, 65536 in v1)
    u32 num_sparse_pages{};  // Number of regions (v2) or total pages (v1)
    u32 num_stored_pages{};  // Number of non-zero 4KB chunks stored
    u8 reserved[16]{};
};

static_assert(sizeof(SaveStateHeader) == 80, "SaveStateHeader must be 80 bytes");

/// Thread context snapshot for save states
struct ThreadSnapshot {
    u32 thread_id{};
    u32 core_id{};
    u64 tls_address{};
    // Full Svc::ThreadContext is 0x320 bytes (ARM64 registers + FPU)
    u8 context_data[0x320]{};
};

/// Result of save state operations
enum class SaveStateResult {
    Success,
    ErrorSystemNotRunning,
    ErrorFileCreate,
    ErrorFileRead,
    ErrorCompress,
    ErrorDecompress,
    ErrorVersionMismatch,
    ErrorCorruptedFile,
    ErrorInvalidMagic,
};

/// Create a save state from the current emulation state (instant in-RAM snapshot + async disk flush)
SaveStateResult CreateSaveState(System& system, const std::filesystem::path& path);

/// Load a save state and restore the emulation state (instant RAM hit + differential 4KB page restore)
SaveStateResult LoadSaveState(System& system, const std::filesystem::path& path);

/// Wait for any pending background disk flush operations to finish
void FlushPendingSaveStates();

/// Get the default save state directory for a given title
std::filesystem::path GetSaveStatePath(u64 title_id, u32 slot);

/// Get info about a save state file without loading it
bool GetSaveStateInfo(const std::filesystem::path& path, SaveStateHeader& header_out);

} // namespace Core
