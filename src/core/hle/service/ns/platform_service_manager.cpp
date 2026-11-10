#include "common/alignment.h"
#include "common/settings.h"
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <vector>
#include <boost/container/static_vector.hpp>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/system_archive/system_archive.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/memory.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/ns/platform_service_manager.h"

namespace Service::NS {

struct FontRegion {
    u32 offset;
    u32 size;
};

// The below data is specific to shared font data dumped from Switch on f/w 2.2
// Virtual address and offsets/sizes likely will vary by dump
[[maybe_unused]] constexpr VAddr SHARED_FONT_MEM_VADDR{0x00000009d3016000ULL};
constexpr u32 EXPECTED_RESULT{0x7f9a0218}; // What we expect the decrypted bfttf first 4 bytes to be
constexpr u32 EXPECTED_MAGIC{0x36f81a1e};  // What we expect the encrypted bfttf first 4 bytes to be
constexpr u64 SHARED_FONT_MEM_SIZE{0x1100000};
constexpr FontRegion EMPTY_REGION{0, 0};

static void DecryptSharedFont(const std::span<u32 const> input, std::span<u8> output, std::size_t& offset) {
    if (input.size() < 2) {
        LOG_ERROR(Service_NS, "Input font too small");
        return;
    }
    if (offset + (input.size() * sizeof(u32)) > SHARED_FONT_MEM_SIZE) {
        LOG_ERROR(Service_NS, "Shared fonts exceeds 17mb!");
        return;
    }
    if (input[0] != EXPECTED_MAGIC) {
        LOG_ERROR(Service_NS, "Failed to derive key, unexpected magic number: {:#x}", input[0]);
        return;
    }
    const u32 KEY = input[0] ^ EXPECTED_RESULT; // Derive key using an inverse xor
    std::vector<u32> transformed_font(input.size());
    // TODO(ogniK): Figure out a better way to do this
    std::transform(input.begin(), input.end(), transformed_font.begin(), [&KEY](u32 font_data) { return Common::swap32(font_data ^ KEY); });
    transformed_font[1] = Common::swap32(transformed_font[1]) ^ KEY; // "re-encrypt" the size
    std::memcpy(output.data() + offset, transformed_font.data(), transformed_font.size() * sizeof(u32));
    offset += transformed_font.size() * sizeof(u32);
}

void DecryptSharedFontToTTF(const std::vector<u32>& input, std::vector<u8>& output) {
    if (input.size() < 2) {
        LOG_ERROR(Service_NS, "Input font is empty");
        return;
    }
    if (input[0] != EXPECTED_MAGIC) {
        LOG_ERROR(Service_NS, "Failed to derive key, unexpected magic number: {:#x}", input[0]);
        return;
    }
    const u32 KEY = input[0] ^ EXPECTED_RESULT; // Derive key using an inverse xor
    std::vector<u32> transformed_font(input.size());
    // TODO(ogniK): Figure out a better way to do this
    std::transform(input.begin(), input.end(), transformed_font.begin(), [&KEY](u32 font_data) { return Common::swap32(font_data ^ KEY); });
    std::memcpy(output.data(), transformed_font.data() + 2, (transformed_font.size() - 2) * sizeof(u32));
}

void EncryptSharedFont(const std::vector<u32>& input, std::vector<u8>& output, std::size_t& offset) {
    if (offset + ((input.size() + 2) * sizeof(u32)) > SHARED_FONT_MEM_SIZE) {
        LOG_ERROR(Service_NS, "Shared fonts exceeds 17mb!");
        return;
    }
    const auto key = Common::swap32(EXPECTED_RESULT ^ EXPECTED_MAGIC);
    std::vector<u32> transformed_font(input.size() + 2);
    transformed_font[0] = Common::swap32(EXPECTED_MAGIC);
    transformed_font[1] = Common::swap32(static_cast<u32>(input.size() * sizeof(u32))) ^ key;
    std::transform(input.begin(), input.end(), transformed_font.begin() + 2, [key](u32 in) { return in ^ key; });
    std::memcpy(output.data() + offset, transformed_font.data(), transformed_font.size() * sizeof(u32));
    offset += transformed_font.size() * sizeof(u32);
}

struct IPlatformServiceManager::Impl {
    const FontRegion& GetSharedFontRegion(std::size_t index) const {
        return index < shared_font_regions.size() ? shared_font_regions[index] : EMPTY_REGION;
    }
    // Automatically populated based on shared_fonts dump or system archives.
    // 6 builtin fonts + extra 2 for whatever may come after
    boost::container::static_vector<FontRegion, 8> shared_font_regions;
    /// Plain TTF copies of the same fonts (Switch 2 GetBuiltInFontData), after the BFTTF data.
    boost::container::static_vector<FontRegion, 8> builtin_font_regions;
    /// Backing memory for the shared font data
    std::array<u8, SHARED_FONT_MEM_SIZE> shared_font;
};

IPlatformServiceManager::IPlatformServiceManager(Core::System& system_, const char* service_name_)
    : ServiceFramework{system_, service_name_}, impl{std::make_unique<Impl>()} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, D<&IPlatformServiceManager::RequestLoad>, "RequestLoad"},
        {1, D<&IPlatformServiceManager::GetLoadState>, "GetLoadState"},
        {2, D<&IPlatformServiceManager::GetSize>, "GetSize"},
        {3, D<&IPlatformServiceManager::GetSharedMemoryAddressOffset>, "GetSharedMemoryAddressOffset"},
        {4, D<&IPlatformServiceManager::GetSharedMemoryNativeHandle>, "GetSharedMemoryNativeHandle"},
        {5, D<&IPlatformServiceManager::GetSharedFontInOrderOfPriority>, "GetSharedFontInOrderOfPriority"},
        {6, D<&IPlatformServiceManager::GetSharedFontInOrderOfPriority>, "GetSharedFontInOrderOfPriorityForSystem"},
        {100, nullptr, "RequestApplicationFunctionAuthorization"},
        {101, nullptr, "RequestApplicationFunctionAuthorizationByProcessId"},
        {102, nullptr, "RequestApplicationFunctionAuthorizationByApplicationId"},
        {103, nullptr, "RefreshApplicationFunctionBlackListDebugRecord"},
        {104, nullptr, "RequestApplicationFunctionAuthorizationByProgramId"},
        {105, nullptr, "GetFunctionBlackListSystemVersionToAuthorize"},
        {106, nullptr, "GetFunctionBlackListVersion"},
        {1000, nullptr, "LoadNgWordDataForPlatformRegionChina"},
        {1001, nullptr, "GetNgWordDataSizeForPlatformRegionChina"},
        {2000, &IPlatformServiceManager::GetBuiltInFontSharedMemory, "GetBuiltInFontSharedMemory (Switch 2)"},
        {2010, &IPlatformServiceManager::GetBuiltInFontData, "GetBuiltInFontData (Switch 2)"},
        {2020, &IPlatformServiceManager::GetBuiltInFontDataSet, "GetBuiltInFontDataSet (Switch 2)"},
        {2100, &IPlatformServiceManager::OpenFont, "OpenFont"},
    };
    // clang-format on
    RegisterHandlers(functions);

    auto& fsc = system.GetFileSystemController();

    // Attempt to load shared font data from disk
    const auto* nand = fsc.GetSystemNANDContents();
    std::size_t offset = 0;
    std::vector<std::vector<u8>> plain_fonts;
    // Rebuild shared fonts from data ncas or synthesize
    for (auto& font : SHARED_FONTS) {
        FileSys::VirtualFile romfs;
        const auto nca =
            nand->GetEntry(static_cast<u64>(font.first), FileSys::ContentRecordType::Data);
        if (nca) {
            romfs = nca->GetRomFS();
        }

        if (!romfs) {
            romfs = FileSys::SystemArchive::SynthesizeSystemArchive(static_cast<u64>(font.first));
        }

        if (!romfs) {
            LOG_ERROR(Service_NS, "Failed to find or synthesize {:016X}! Skipping", font.first);
            continue;
        }

        const auto extracted_romfs = FileSys::ExtractRomFS(romfs);
        if (!extracted_romfs) {
            LOG_ERROR(Service_NS, "Failed to extract RomFS for {:016X}! Skipping", font.first);
            continue;
        }
        const auto font_fp = extracted_romfs->GetFile(font.second);
        if (!font_fp) {
            LOG_ERROR(Service_NS, "{:016X} has no file \"{}\"! Skipping", font.first, font.second);
            continue;
        }
        std::vector<u32> font_data_u32(font_fp->GetSize() / sizeof(u32));
        font_fp->ReadBytes<u32>(font_data_u32.data(), font_fp->GetSize());
        // We need to be BigEndian as u32s for the xor encryption
        std::transform(font_data_u32.begin(), font_data_u32.end(), font_data_u32.begin(),
                       Common::swap32);
        // Font offset and size do not account for the header
        const FontRegion region{u32(offset + 8), u32((font_data_u32.size() * sizeof(u32)) - 8)};
        std::vector<u8> plain(region.size);
        DecryptSharedFontToTTF(font_data_u32, plain);
        plain_fonts.push_back(std::move(plain));
        DecryptSharedFont(font_data_u32, impl->shared_font, offset);
        impl->shared_font_regions.push_back(region);
    }

    for (const auto& plain : plain_fonts) {
        offset = Common::AlignUp(offset, 0x1000);
        if (offset + plain.size() > SHARED_FONT_MEM_SIZE) {
            LOG_WARNING(Service_NS, "No shared memory left for plain built-in font copies");
            break;
        }
        std::memcpy(impl->shared_font.data() + offset, plain.data(), plain.size());
        impl->builtin_font_regions.push_back({u32(offset), u32(plain.size())});
        offset += plain.size();
    }
}

class IFontService final : public ServiceFramework<IFontService> {
public:
    explicit IFontService(Core::System& system_, Kernel::KTransferMemory* memory_,
                          std::vector<FontRegion> regions_)
        : ServiceFramework{system_, "IFontService"},
          memory{memory_},
          regions{std::move(regions_)} {
        if (memory) {
            memory->Open(kernel);
        }
        static const FunctionInfo functions[] = {
            {0, &IFontService::GetFontData, "GetFontData"},
            {1, &IFontService::GetFontDataSet, "GetFontDataSet"},
        };
        RegisterHandlers(functions);
    }

    ~IFontService() override {
        if (memory) {
            memory->Close(kernel);
        }
    }

private:
    struct Descriptor {
        u32_le flags{};
        u32_le offset{};
        u32_le size{};
    };
    static_assert(sizeof(Descriptor) == 12);

    void GetFontData(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u32 arguments = rp.Pop<u32>();
        const auto type = static_cast<u8>(arguments);
        const bool found = type < regions.size() && regions[type].size != 0;
        const auto region = found ? regions[type] : EMPTY_REGION;
        IPC::ResponseBuilder rb{ctx, 6};
        rb.Push(ResultSuccess);
        rb.Push<u32>(found ? 1 : 0);
        rb.Push<u32>(0);
        rb.Push<u32>(region.offset);
        rb.Push<u32>(region.size);
    }

    void GetFontDataSet(HLERequestContext& ctx) {
        const auto types = ctx.ReadBuffer();
        const auto capacity = ctx.GetWriteBufferSize() / sizeof(Descriptor);
        std::vector<Descriptor> descriptors;
        for (const auto type : types) {
            if (descriptors.size() == capacity) break;
            if (type < regions.size()) {
                descriptors.push_back({0, regions[type].offset, regions[type].size});
            }
        }
        ctx.WriteBuffer(descriptors);
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u32>(descriptors.empty() ? 0 : 1);
        rb.Push<u32>(static_cast<u32>(descriptors.size()));
    }

    Kernel::KTransferMemory* memory;
    std::vector<FontRegion> regions;
};

IPlatformServiceManager::~IPlatformServiceManager() = default;

void IPlatformServiceManager::GetBuiltInFontSharedMemory(HLERequestContext& ctx) {
    LOG_WARNING(Service_NS, "(STUBBED) Switch 2 GetBuiltInFontSharedMemory");
    std::memcpy(kernel.GetFontSharedMem().GetPointer(), impl->shared_font.data(), impl->shared_font.size());
    IPC::ResponseBuilder rb{ctx, 3, 1};
    rb.Push(ResultSuccess);
    rb.Push<u32>(static_cast<u32>(SHARED_FONT_MEM_SIZE));
    rb.PushCopyObjects(ctx, kernel.GetFontSharedMem());
}

void IPlatformServiceManager::GetBuiltInFontData(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 type = rp.Pop<u32>();
    const bool flag = rp.Pop<u8>() != 0;
    const auto& regions = impl->builtin_font_regions;
    const bool found = !regions.empty();
    const FontRegion region = found ? regions[std::min<size_t>(type, regions.size() - 1)] : EMPTY_REGION;
    LOG_WARNING(Service_NS, "(STUBBED) Switch 2 GetBuiltInFontData type={} flag={} -> offset={:#x} size={:#x}",
                type, flag, region.offset, region.size);
    // Raw reply: the bool at offset 0, the descriptor at offset 8.
    IPC::ResponseBuilder rb{ctx, 2 + 6};
    rb.Push(ResultSuccess);
    rb.Push<u32>(found ? 1 : 0);
    rb.Push<u32>(0);
    rb.Push<u32>(0);
    rb.Push<u32>(region.offset);
    rb.Push<u32>(region.size);
    rb.Push<u32>(0);
}

void IPlatformServiceManager::GetBuiltInFontDataSet(HLERequestContext& ctx) {
    struct Descriptor {
        u32_le flags;
        u32_le offset;
        u32_le size;
    };
    static_assert(sizeof(Descriptor) == 12);
    const auto types = ctx.ReadBuffer();
    const auto capacity = (std::min<std::size_t>)(6, ctx.GetWriteBufferSize() / sizeof(Descriptor));
    std::vector<Descriptor> descriptors;
    for (const auto type : types) {
        if (descriptors.size() == capacity) break;
        if (type < impl->shared_font_regions.size()) {
            const auto& region = impl->shared_font_regions[type];
            descriptors.push_back({0, region.offset, region.size});
        }
    }
    ctx.WriteBuffer(descriptors);
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<u32>(descriptors.empty() ? 0 : 1);
    rb.Push<u32>(static_cast<u32>(descriptors.size()));
}

void IPlatformServiceManager::OpenFont(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto kind = static_cast<u8>(rp.Pop<u32>());
    const u32 size = rp.Pop<u32>();
    auto memory = ctx.GetObjectFromHandle<Kernel::KTransferMemory>(ctx.GetCopyHandle(0));
    const Result result = kind != 0 ? Kernel::ResultInvalidEnumValue
                          : memory.IsNull() || memory->GetOwner() == nullptr
                              ? Kernel::ResultInvalidHandle
                          : size > memory->GetSize()
                              ? Kernel::ResultInvalidSize
                              : ResultSuccess;
    if (result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }
    auto& owner_memory = memory->GetOwner()->GetMemory();
    const auto address = memory->GetSourceAddress();
    const u64 max_bytes = size != 0 ? size : SHARED_FONT_MEM_SIZE;
    std::vector<FontRegion> out_regions;
    out_regions.reserve(impl->shared_font_regions.size());
    u64 dst_offset = 0;
    for (size_t i = 0; i < impl->shared_font_regions.size(); ++i) {
        const auto& region = impl->shared_font_regions[i];
        if (region.size != 0 &&
            u64{region.offset} + u64{region.size} <= impl->shared_font.size() &&
            dst_offset + region.size <= max_bytes) {
            owner_memory.WriteBlock(address + dst_offset,
                                    impl->shared_font.data() + region.offset,
                                    region.size);
            out_regions.push_back(FontRegion{static_cast<u32>(dst_offset), region.size});
            dst_offset = (dst_offset + region.size + 7ULL) & ~7ULL;
        } else {
            out_regions.push_back(EMPTY_REGION);
        }
    }
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IFontService>(ctx, system, memory.GetPointerUnsafe(), std::move(out_regions));
}

void IPlatformServiceManager::RequestLoadNxFont(HLERequestContext& ctx) {
    OpenFont(ctx);
}

Result IPlatformServiceManager::RequestLoad(SharedFontType type) {
    // Games don't call this so all fonts should be loaded
    LOG_DEBUG(Service_NS, "called, shared_font_type={}", type);
    R_SUCCEED();
}

Result IPlatformServiceManager::GetLoadState(Out<LoadState> out_load_state, SharedFontType type) {
    LOG_DEBUG(Service_NS, "called, shared_font_type={}", type);
    *out_load_state = LoadState::Loaded;
    R_SUCCEED();
}

Result IPlatformServiceManager::GetSize(Out<u32> out_size, SharedFontType type) {
    LOG_DEBUG(Service_NS, "called, shared_font_type={}", type);
    *out_size = impl->GetSharedFontRegion(static_cast<size_t>(type)).size;
    R_SUCCEED();
}

Result IPlatformServiceManager::GetSharedMemoryAddressOffset(Out<u32> out_shared_memory_offset,
                                                             SharedFontType type) {
    LOG_DEBUG(Service_NS, "called, shared_font_type={}", type);
    *out_shared_memory_offset = impl->GetSharedFontRegion(static_cast<size_t>(type)).offset;
    R_SUCCEED();
}

Result IPlatformServiceManager::GetSharedMemoryNativeHandle(OutCopyHandle<Kernel::KSharedMemory> out_shared_memory_native_handle) {
    // Map backing memory for the font data
    LOG_DEBUG(Service_NS, "called");

    // Create shared font memory object
    std::memcpy(kernel.GetFontSharedMem().GetPointer(), impl->shared_font.data(), impl->shared_font.size());

    // FIXME: this shouldn't belong to the kernel
    *out_shared_memory_native_handle = &kernel.GetFontSharedMem();
    R_SUCCEED();
}

Result IPlatformServiceManager::GetSharedFontInOrderOfPriority(
    OutArray<u32, BufferAttr_HipcMapAlias> out_font_codes,
    OutArray<u32, BufferAttr_HipcMapAlias> out_font_offsets,
    OutArray<u32, BufferAttr_HipcMapAlias> out_font_sizes, Out<bool> out_fonts_are_loaded,
    Out<u32> out_font_count, Set::LanguageCode language_code) {
    LOG_DEBUG(Service_NS, "called, language_code={:#x}", language_code);

    // The maximum number of elements that can be returned is 6. Regardless of the available fonts
    // or buffer size.
    constexpr size_t MaxElementCount = 6;

    // TODO(ogniK): Have actual priority order
    const auto max_size = (std::min)({MaxElementCount, out_font_codes.size(), out_font_offsets.size(),
                                    out_font_sizes.size(), impl->shared_font_regions.size()});

    for (size_t i = 0; i < max_size; i++) {
        auto& region = impl->GetSharedFontRegion(i);

        out_font_codes[i] = static_cast<u32>(i);
        out_font_offsets[i] = region.offset;
        out_font_sizes[i] = region.size;
    }

    *out_fonts_are_loaded = true;
    *out_font_count = static_cast<u32>(max_size);
    R_SUCCEED();
}


Result IPlatformServiceManager::IsRunningOnOunce(Out<bool> out_is_ounce) {
    *out_is_ounce = Settings::IsCurrentProgramSwitch2();
    LOG_INFO(Service_NS, "called, is_ounce={}", *out_is_ounce);
    R_SUCCEED();
}

} // namespace Service::NS