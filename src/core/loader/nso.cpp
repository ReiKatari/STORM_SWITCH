// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <vector>

#include "common/common_funcs.h"
#include "common/hex_util.h"
#include "common/logging.h"
#include "common/lz4_compression.h"
#include "common/settings.h"
#include "common/swap.h"
#include "common/zbic_compression.h"
#include "core/core.h"
#include "core/file_sys/patch_manager.h"
#include "core/hle/kernel/code_set.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/loader/nso.h"
#include "core/memory.h"

#ifdef HAS_NCE
#include "core/arm/nce/patcher.h"
#endif

namespace Loader {
namespace {
struct MODHeader {
    u32_le magic;
    u32_le dynamic_offset;
    u32_le bss_start_offset;
    u32_le bss_end_offset;
    u32_le eh_frame_hdr_start_offset;
    u32_le eh_frame_hdr_end_offset;
    u32_le module_offset; // Offset to runtime-generated module object. typically equal to .bss base
};
static_assert(sizeof(MODHeader) == 0x1c, "MODHeader has incorrect size.");

constexpr u32 PageAlignSize(u32 size) {
    return static_cast<u32>((size + Core::Memory::YUZU_PAGEMASK) & ~Core::Memory::YUZU_PAGEMASK);
}
} // Anonymous namespace

bool NSOHeader::IsSegmentCompressed(size_t segment_num) const {
    ASSERT_MSG(segment_num < 3, "Invalid segment {}", segment_num);
    return ((flags >> segment_num) & 1) != 0;
}

AppLoader_NSO::AppLoader_NSO(FileSys::VirtualFile file_) : AppLoader(std::move(file_)) {}

FileType AppLoader_NSO::IdentifyType(const FileSys::VirtualFile& in_file) {
    u32 magic = 0;
    if (in_file->ReadObject(&magic) != sizeof(magic)) {
        return FileType::Error;
    }

    if (Common::MakeMagic('N', 'S', 'O', '0') != magic) {
        return FileType::Error;
    }

    return FileType::NSO;
}

std::optional<VAddr> AppLoader_NSO::LoadModule(Kernel::KProcess& process, Core::System& system, const FileSys::VfsFile& nso_file, VAddr load_base, bool should_pass_arguments, bool load_into_process, std::optional<FileSys::PatchManager> pm, std::vector<Core::NCE::Patcher>* patches, s32 patch_index) {
    if (nso_file.GetSize() < sizeof(NSOHeader))
        return std::nullopt;
    NSOHeader nso_header{};
    if (sizeof(NSOHeader) != nso_file.ReadObject(&nso_header))
        return std::nullopt;
    if (nso_header.magic != Common::MakeMagic('N', 'S', 'O', '0'))
        return std::nullopt;
    if (nso_header.segments.empty())
        return std::nullopt;

    // Allocate some space at the beginning if we are patching in PreText mode.
    const size_t module_start = [&]() -> size_t {
#ifdef HAS_NCE
        if (patches && load_into_process) {
            auto* patch = &patches->operator[](patch_index);
            if (patch->GetPatchMode() == Core::NCE::PatchMode::PreText) {
                return patch->GetSectionSize();
            } else if (patch->GetPatchMode() == Core::NCE::PatchMode::Split) {
                return patch->GetPreSectionSize();
            }
        }
#endif
        return 0;
    }();

    auto const last_segment_it = &nso_header.segments[nso_header.segments.size() - 1];
    // Build program image directly in codeset memory :)
    Kernel::CodeSet codeset;
    codeset.memory.resize(module_start + last_segment_it->location + last_segment_it->size);
    {
        for (std::size_t i = 0; i < nso_header.segments.size(); ++i) {
            const u32 segment_size = nso_header.segments[i].size;
            if (segment_size == 0) {
                continue;
            }

            u8* const dest_ptr = codeset.memory.data() + module_start + nso_header.segments[i].location;

            if (nso_header.IsSegmentCompressed(i)) {
                const u32 comp_size = nso_header.segments_compressed_size[i];
                const auto file_size = nso_file.GetSize();
                auto file_offset = nso_header.segments[i].offset;

                std::vector<u8> compressed(comp_size);
                size_t bytes_read = nso_file.Read(compressed.data(), comp_size, file_offset);

                std::memset(dest_ptr, 0, segment_size);
                int r = -1;

                // 1. Check for Nintendo Switch 22.0.0+ ZBIC compression
                // Segment data may start directly with 'ZBIC' (0x4349425A) or have a header prefix (e.g. module name)
                constexpr u32 ZBIC_MAGIC = 0x4349425A;
                size_t zbic_offset = 0;
                bool found_zbic = false;
                for (size_t scan = 0; scan + 4 <= bytes_read && scan <= 32; ++scan) {
                    u32 scan_magic = 0;
                    std::memcpy(&scan_magic, compressed.data() + scan, sizeof(u32));
                    if (scan_magic == ZBIC_MAGIC) {
                        zbic_offset = scan;
                        found_zbic = true;
                        break;
                    }
                }

                if (found_zbic) {
                    LOG_INFO(Loader, "NSO segment {} in '{}': detected ZBIC compression at offset +{:#x}",
                             i, nso_file.GetName(), zbic_offset);
                    r = Common::Compression::DecompressDataZBIC(
                        dest_ptr, segment_size, compressed.data() + zbic_offset, bytes_read - zbic_offset);
                    if (r == static_cast<int>(segment_size)) {
                        LOG_INFO(Loader, "NSO segment {} in '{}' decompressed successfully via ZBIC ({} -> {} bytes)",
                                 i, nso_file.GetName(), bytes_read - zbic_offset, segment_size);
                    } else {
                        LOG_WARNING(Loader, "NSO segment {} in '{}': ZBIC decompression returned {} (expected {})",
                                    i, nso_file.GetName(), r, segment_size);
                    }
                }

                // 2. Standard LZ4 decompression if not ZBIC or if ZBIC returned unexpected size
                if (r != static_cast<int>(segment_size)) {
                    r = Common::Compression::DecompressDataLZ4(dest_ptr, segment_size, compressed.data(), bytes_read);
                }

                // 3. Fallback for offset 0x100 vs 0x108
                if (r != static_cast<int>(segment_size) && i == 0 && comp_size > 0) {
                    const u32 alt_offset = (file_offset == sizeof(NSOHeader)) ? (sizeof(NSOHeader) + 8) : static_cast<u32>(sizeof(NSOHeader));
                    if (alt_offset < file_size) {
                        const size_t alt_read = nso_file.Read(compressed.data(), comp_size, alt_offset);
                        std::memset(dest_ptr, 0, segment_size);
                        found_zbic = false;
                        for (size_t scan = 0; scan + 4 <= alt_read && scan <= 32; ++scan) {
                            u32 scan_magic = 0;
                            std::memcpy(&scan_magic, compressed.data() + scan, sizeof(u32));
                            if (scan_magic == ZBIC_MAGIC) {
                                zbic_offset = scan;
                                found_zbic = true;
                                break;
                            }
                        }
                        if (found_zbic) {
                            r = Common::Compression::DecompressDataZBIC(
                                dest_ptr, segment_size, compressed.data() + zbic_offset, alt_read - zbic_offset);
                        } else {
                            r = Common::Compression::DecompressDataLZ4(dest_ptr, segment_size, compressed.data(), alt_read);
                        }
                        if (r == static_cast<int>(segment_size)) {
                            LOG_INFO(Loader, "NSO segment 0 decompressed successfully using alternative offset {:#x}", alt_offset);
                            file_offset = alt_offset;
                        }
                    }
                }

                if (r == static_cast<int>(segment_size)) {
                    LOG_DEBUG(Loader, "NSO segment {} in '{}' decompressed successfully ({} -> {} bytes)",
                              i, nso_file.GetName(), comp_size, segment_size);
                } else {
                    std::string hex_preview;
                    for (size_t b = 0; b < std::min<size_t>(16, compressed.size()); ++b) {
                        hex_preview += fmt::format("{:02X} ", compressed[b]);
                    }
                    LOG_WARNING(Loader, "NSO segment {} in '{}': decompression returned {} (expected {}, "
                                "comp_size={}, offset={:#x}, read={}, first 16 bytes: [{}])",
                                i, nso_file.GetName(), r, segment_size, comp_size, file_offset, bytes_read, hex_preview);

                    std::memset(dest_ptr, 0, segment_size);
                    if (comp_size >= segment_size) {
                        std::memcpy(dest_ptr, compressed.data(), segment_size);
                    } else if (bytes_read > 0) {
                        std::memcpy(dest_ptr, compressed.data(), std::min<size_t>(bytes_read, segment_size));
                    }
                }
            } else {
                const auto read_size = std::min<size_t>(segment_size, nso_file.GetSize() > nso_header.segments[i].offset ? nso_file.GetSize() - nso_header.segments[i].offset : 0);
                nso_file.Read(dest_ptr, read_size, nso_header.segments[i].offset);
            }
            codeset.segments[i].addr = module_start + nso_header.segments[i].location;
            codeset.segments[i].offset = module_start + nso_header.segments[i].location;
            codeset.segments[i].size = segment_size;
        }
    }

    if (should_pass_arguments && !Settings::values.program_args.GetValue().empty()) {
        const auto arg_data{Settings::values.program_args.GetValue()};

        codeset.DataSegment().size += NSO_ARGUMENT_DATA_ALLOCATION_SIZE;
        NSOArgumentHeader args_header{NSO_ARGUMENT_DATA_ALLOCATION_SIZE, static_cast<u32_le>(arg_data.size()), {}};
        const auto end_offset = codeset.memory.size();
        codeset.memory.resize(u32(codeset.memory.size()) + NSO_ARGUMENT_DATA_ALLOCATION_SIZE);
        std::memcpy(codeset.memory.data() + end_offset, &args_header, sizeof(NSOArgumentHeader));
        std::memcpy(codeset.memory.data() + end_offset + sizeof(NSOArgumentHeader), arg_data.data(), arg_data.size());
    }

    codeset.DataSegment().size += nso_header.segments[2].bss_size;
    u32 image_size = PageAlignSize(u32(codeset.memory.size()) + nso_header.segments[2].bss_size);
    codeset.memory.resize(image_size);

    for (std::size_t i = 0; i < nso_header.segments.size(); ++i) {
        codeset.segments[i].size = PageAlignSize(codeset.segments[i].size);
    }

    // Apply patches if necessary
    const auto name = nso_file.GetName();
    if (pm && (pm->HasNSOPatch(nso_header.build_id, name) || Settings::values.dump_nso)) {
        std::span<u8> patchable_section(codeset.memory.data() + module_start, codeset.memory.size() - module_start);
        std::vector<u8> pi_header(sizeof(NSOHeader) + patchable_section.size());
        std::memcpy(pi_header.data(), &nso_header, sizeof(NSOHeader));
        std::memcpy(pi_header.data() + sizeof(NSOHeader), patchable_section.data(),
                    patchable_section.size());

        pi_header = pm->PatchNSO(pi_header, name);

        std::copy(pi_header.begin() + sizeof(NSOHeader), pi_header.end(), patchable_section.data());
    }

#ifdef HAS_NCE
    // If we are computing the process code layout and using nce backend, patch.
    const auto& code = codeset.CodeSegment();
    auto* patch = patches ? &patches->operator[](patch_index) : nullptr;
    if (patch && !load_into_process) {
        //Set module ID using build_id from the NSO header
        patch->SetModuleID(nso_header.build_id);
        // Patch SVCs and MRS calls in the guest code
        while (!patch->PatchText(codeset.memory, code)) {
            patch = &patches->emplace_back();
            patch->SetModuleID(nso_header.build_id);  // In case the patcher is changed for big modules, the new patcher should also have the build_id
        }
    } else if (patch) {
        // Relocate code patch and copy to the program image.
        // Save size before RelocateAndCopy (which may resize)
        const size_t size_before_relocate = codeset.memory.size();
        if (patch->RelocateAndCopy(load_base, code, codeset.memory, &process.GetPostHandlers())) {
            // Update patch section.
            auto& patch_segment = codeset.PatchSegment();
            auto& post_patch_segment = codeset.PostPatchSegment();
            const auto patch_mode = patch->GetPatchMode();
            if (patch_mode == Core::NCE::PatchMode::PreText) {
                patch_segment.addr = 0;
                patch_segment.size = static_cast<u32>(patch->GetSectionSize());
            } else if (patch_mode == Core::NCE::PatchMode::Split) {
                // For Split-mode, we are using pre-patch buffer at start, post-patch buffer at end
                patch_segment.addr = 0;
                patch_segment.size = static_cast<u32>(patch->GetPreSectionSize());
                post_patch_segment.addr = size_before_relocate;
                post_patch_segment.size = static_cast<u32>(patch->GetSectionSize());
            } else {
                patch_segment.addr = image_size;
                patch_segment.size = static_cast<u32>(patch->GetSectionSize());
            }
        }

        // Refresh image_size to take account the patch section if it was added by RelocateAndCopy
        image_size = static_cast<u32>(codeset.memory.size());
    }
#endif

    // If we aren't actually loading (i.e. just computing the process code layout), we are done
    if (!load_into_process) {
#ifdef HAS_NCE
        // Ok, so for Split mode, we need to account for pre-patch and post-patch space
        // which will be added during RelocateAndCopy in the second pass. Where it crashed
        // in Android Studio at PreText. May be a better way. Works for now.
        if (patch && patch->GetPatchMode() == Core::NCE::PatchMode::Split) {
            return load_base + patch->GetPreSectionSize() + image_size + patch->GetSectionSize();
        } else if (patch && patch->GetPatchMode() == Core::NCE::PatchMode::PreText) {
            return load_base + patch->GetSectionSize() + image_size;
        } else if (patch && patch->GetPatchMode() == Core::NCE::PatchMode::PostData) {
            return load_base + image_size + patch->GetSectionSize();
        }
#endif
        return load_base + image_size;
    }

    // Apply cheats if they exist and this is the main game module (or standalone NSO)
    const std::string module_name = nso_file.GetName();
    const bool is_main_module = (module_name == "main" || module_name == "main.nso" ||
                                 module_name == "MAIN" || module_name == "MAIN.NSO" ||
                                 module_name.empty() || module_name.ends_with(".nso") || module_name.ends_with(".NSO"));
    if (is_main_module || system.GetMainNsoBase() == 0) {
        system.SetMainNsoParameters(load_base, image_size);
    }
    if (pm) {
        if (is_main_module || system.GetApplicationProcessBuildID() == Core::System::CurrentBuildProcessID{}) {
            system.SetApplicationProcessBuildID(nso_header.build_id);
            const auto cheats = pm->CreateCheatList(nso_header.build_id);
            system.RegisterCheatList(cheats, nso_header.build_id, load_base, image_size);
        }
    }

    // Load codeset for current process
    process.LoadModule(system.Kernel(), std::move(codeset), load_base);
    return load_base + image_size;
}

AppLoader_NSO::LoadResult AppLoader_NSO::Load(Kernel::KProcess& process, Core::System& system) {
    if (is_loaded) {
        return {ResultStatus::ErrorAlreadyLoaded, {}};
    }

    modules.clear();

    // Load module
    const VAddr base_address = GetInteger(process.GetEntryPoint());
    if (!LoadModule(process, system, *file, base_address, true, true)) {
        return {ResultStatus::ErrorLoadingNSO, {}};
    }

    modules.insert_or_assign(base_address, file->GetName());
    LOG_DEBUG(Loader, "loaded module {} @ {:#x}", file->GetName(), base_address);

    is_loaded = true;
    return {ResultStatus::Success, LoadParameters{Kernel::KThread::DefaultThreadPriority,
                                                  Core::Memory::DEFAULT_STACK_SIZE}};
}

ResultStatus AppLoader_NSO::ReadNSOModules(Modules& out_modules) {
    out_modules = this->modules;
    return ResultStatus::Success;
}

} // namespace Loader
