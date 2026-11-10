// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

#include "common/logging.h"
#include <ranges>
#include "core/crypto/aes_util.h"
#include "core/crypto/ctr_encryption_layer.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/content_archive.h"
#include <mutex>
#include <filesystem>
#include "common/fs/file.h"
#include "common/fs/path_util.h"
#include "core/file_sys/ncz_virtual_file.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/vfs/vfs_offset.h"
#include "core/loader/loader.h"

#include "core/file_sys/fssystem/fssystem_compression_configuration.h"
#include "core/file_sys/fssystem/fssystem_crypto_configuration.h"
#include "core/file_sys/fssystem/fssystem_nca_file_system_driver.h"

namespace FileSys {

static u8 MasterKeyIdForKeyGeneration(u8 key_generation) {
    return std::max<u8>(key_generation, 1) - 1;
}

namespace {
class DiskVfsFile : public VfsFile {
public:
    DiskVfsFile(std::filesystem::path path_, std::string name_,
                std::optional<std::array<u8, 16>> embedded_key_ = std::nullopt)
        : path(std::move(path_)), name(std::move(name_)), embedded_key(embedded_key_) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        file.Open(path, Common::FS::FileAccessMode::Read, Common::FS::FileType::BinaryFile, Common::FS::FileShareFlag::ShareReadOnly);
    }

    std::string GetName() const override { return name; }
    std::string GetExtension() const override { return name.substr(name.find_last_of('.') + 1); }
    std::size_t GetSize() const override { return file.IsOpen() ? file.GetSize() : 0; }
    bool Resize(std::size_t new_size) override { return false; }
    VirtualDir GetContainingDirectory() const override { return nullptr; }
    bool IsWritable() const override { return false; }
    bool IsReadable() const override { return file.IsOpen(); }
    bool Rename(std::string_view name_) override { return false; }
    bool IsNczFile() const override { return false; }
    bool HasDecryptedSections() const override { return false; }
    std::optional<std::array<u8, 16>> GetEmbeddedCryptoKey() const override { return embedded_key; }

    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override {
        if (!file.IsOpen()) return 0;
        std::lock_guard<std::mutex> lock(io_mutex);
        if (!file.Seek(static_cast<s64>(offset))) return 0;
        return file.ReadSpan(std::span<u8>(data, length));
    }

    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override {
        return 0;
    }

private:
    std::filesystem::path path;
    std::string name;
    std::optional<std::array<u8, 16>> embedded_key;
    mutable Common::FS::IOFile file;
    mutable std::mutex io_mutex;
};

[[maybe_unused]] static VirtualFile DecompressIfNCZ(VirtualFile file) {
    if (file == nullptr) return nullptr;
    auto ncz_file = file->IsNczFile() ? std::static_pointer_cast<NCZVirtualFile>(file) : nullptr;
    if (ncz_file && ncz_file->is_solid_stream) {
        auto embedded_key = ncz_file->GetEmbeddedCryptoKey();
        std::filesystem::path temp_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
        std::error_code ec;
        std::filesystem::create_directories(temp_dir, ec);
        std::filesystem::path cache_path = temp_dir / (file->GetName() + ".v3.decompressed_cache");

        bool cache_valid = false;
        if (std::filesystem::exists(cache_path, ec)) {
            std::size_t disk_size = std::filesystem::file_size(cache_path, ec);
            std::filesystem::path completed_path = cache_path.string() + ".completed";
            if (disk_size == ncz_file->GetSize() && std::filesystem::exists(completed_path, ec)) {
                cache_valid = true;
            }
        }

        if (cache_valid) {
            LOG_INFO(Loader, "NCA: Using cached decompressed solid NCA");
            return std::make_shared<DiskVfsFile>(cache_path, file->GetName(), embedded_key);
        }

        LOG_INFO(Loader, "NCA: Decompressing solid NCZ NCA ({} bytes) to disk cache...", ncz_file->GetSize());
        if (ncz_file->DecompressSolidTo(cache_path)) {
            LOG_INFO(Loader, "NCA: Solid NCZ NCA decompressed and cached to disk successfully.");
            std::filesystem::path completed_path = cache_path.string() + ".completed";
            if (std::FILE* marker = std::fopen(completed_path.string().c_str(), "w")) {
                std::fclose(marker);
            }
            return std::make_shared<DiskVfsFile>(cache_path, file->GetName(), embedded_key);
        } else {
            LOG_ERROR(Loader, "NCA: Failed to decompress solid NCZ NCA");
            return file;
        }
    }
    return file;
}
}

NcaHeaderSummary NCA::PeekHeaderSummary(const VirtualFile& file_) {
    NcaHeaderSummary summary{};
    if (file_ == nullptr) {
        return summary;
    }
    NcaReader header_reader;
    if (R_FAILED(header_reader.Initialize(file_, GetCryptoConfiguration(),
                                          GetNcaCompressionConfiguration()))) {
        return summary;
    }
    summary.valid = true;
    summary.title_id = header_reader.GetProgramId();
    summary.type = static_cast<NCAContentType>(header_reader.GetContentType());
    const s32 fs_count = header_reader.GetFsCount();
    for (s32 i = 0; i < fs_count; ++i) {
        NcaFsHeader fs_header{};
        if (R_SUCCEEDED(header_reader.ReadHeader(&fs_header, i))) {
            if (fs_header.encryption_type == NcaFsHeader::EncryptionType::AesCtrEx ||
                fs_header.encryption_type == NcaFsHeader::EncryptionType::AesCtrExSkipLayerHash) {
                summary.is_update = true;
                break;
            }
        }
    }
    return summary;
}

NCA::NCA(VirtualFile file_, const NCA* base_nca)
    : file(std::move(file_)), keys{Core::Crypto::KeyManager::Instance()} {
    if (file == nullptr) {
        status = Loader::ResultStatus::ErrorNullFile;
        return;
    }

    reader = std::make_shared<NcaReader>();
    if (Result rc = reader->Initialize(file, GetCryptoConfiguration(), GetNcaCompressionConfiguration()); R_FAILED(rc)) {
        status = Loader::ResultStatus::ErrorBadNCAHeader;
        return;
    }

    RightsId rights_id{};
    reader->GetRightsId(rights_id.data(), rights_id.size());
    const u8 master_key_id = MasterKeyIdForKeyGeneration(reader->GetKeyGeneration());

    // Only NCAs without a rights ID use Key Area Keys!
    if (rights_id == RightsId{}) {
        if (!keys.HasKey(Core::Crypto::S128KeyType::KeyArea, master_key_id, reader->GetKeyIndex())) {
            bool all_no_crypto = (reader->GetFsCount() > 0);
            for (s32 i = 0; i < reader->GetFsCount(); ++i) {
                NcaFsHeader fs_header{};
                if (R_FAILED(reader->ReadHeader(&fs_header, i)) ||
                    fs_header.encryption_type != NcaFsHeader::EncryptionType::None) {
                    all_no_crypto = false;
                    break;
                }
            }
            if (!all_no_crypto) {
                LOG_ERROR(Loader, "Cannot decrypt {}: keys for gen 0x{:02X}+ required.", file->GetName(), reader->GetKeyGeneration());
                status = Loader::ResultStatus::ErrorMissingKeyAreaKey;
                return;
            }
            LOG_INFO(Loader, "NCA {} has no KeyAreaKey for gen 0x{:02X}, but all sections are unencrypted (NoCrypto). Proceeding.",
                     file->GetName(), reader->GetKeyGeneration());
        }
    }

    Core::Crypto::Key128 raw_titlekey{};
    Core::Crypto::Key128 decrypted_titlekey{};

    if (rights_id != RightsId{}) {
        u128 rights_id_u128;
        std::memcpy(rights_id_u128.data(), rights_id.data(), sizeof(rights_id));

        auto titlekey =
            keys.GetKey(Core::Crypto::S128KeyType::Titlekey, rights_id_u128[1], rights_id_u128[0]);

        if (titlekey == Core::Crypto::Key128{}) {
            const auto embedded_key = file->GetEmbeddedCryptoKey();
            if (embedded_key && *embedded_key != Core::Crypto::Key128{}) {
                titlekey = *embedded_key;
                raw_titlekey = *embedded_key;
                decrypted_titlekey = *embedded_key;
                LOG_INFO(Loader, "NCA: Using embedded crypto key from NCZSECTN for rights ID {:016X}{:016X}",
                         rights_id_u128[1], rights_id_u128[0]);
            } else {
                // Fallback: use rights_id as TitleKey for direct dumps without tickets/keys.
                std::memcpy(titlekey.data(), rights_id.data(), rights_id.size());
                LOG_DEBUG(Loader, "TitleKey not found for {}, falling back to RightsId.",
                          file->GetName());
                raw_titlekey = titlekey;
                decrypted_titlekey = titlekey;
            }
        } else if (keys.HasKey(Core::Crypto::S128KeyType::Titlekek, master_key_id)) {
            raw_titlekey = titlekey;
            decrypted_titlekey = titlekey;
            auto titlekek = keys.GetKey(Core::Crypto::S128KeyType::Titlekek, master_key_id);
            Core::Crypto::AESCipher<Core::Crypto::Key128> cipher(titlekek, Core::Crypto::Mode::ECB);
            cipher.Transcode(decrypted_titlekey.data(), decrypted_titlekey.size(), decrypted_titlekey.data(),
                             Core::Crypto::Op::Decrypt);
        } else {
            raw_titlekey = titlekey;
            decrypted_titlekey = titlekey;
        }
    }

    const s32 fs_count = reader->GetFsCount();
    auto TryOpenFilesystems = [&](const Core::Crypto::Key128& key) -> bool {
        files.clear();
        dirs.clear();
        romfs = nullptr;
        exefs = nullptr;
        logo = nullptr;
        is_update = false;

        if (rights_id != RightsId{}) {
            reader->SetExternalDecryptionKey(key.data(), key.size());
        }

        NcaFileSystemDriver fs(base_nca ? base_nca->reader : nullptr, reader);
        std::vector<VirtualFile> filesystems(fs_count);
        bool has_sparse_layer = false;
        for (s32 i = 0; i < fs_count; i++) {
            NcaFsHeaderReader header_reader;
            Result rc = fs.OpenStorage(&filesystems[i], &header_reader, i);
            if (R_FAILED(rc)) {
                continue;
            }

            if (header_reader.ExistsSparseLayer()) {
                has_sparse_layer = true;
            }

            if (header_reader.GetEncryptionType() == NcaFsHeader::EncryptionType::AesCtrEx) {
                is_update = true;
            }

            if (header_reader.GetFsType() == NcaFsHeader::FsType::RomFs) {
                files.push_back(filesystems[i]);
                romfs = files.back();
            }

            if (header_reader.GetFsType() == NcaFsHeader::FsType::PartitionFs) {
                auto npfs = std::make_shared<PartitionFilesystem>(filesystems[i]);
                if (npfs->GetStatus() == Loader::ResultStatus::Success) {
                    dirs.push_back(npfs);
                    if (IsDirectoryExeFS(npfs)) {
                        exefs = dirs.back();
                    } else if (IsDirectoryLogoPartition(npfs)) {
                        logo = dirs.back();
                    } else {
                        if (exefs == nullptr && (npfs->GetFile("main.npdm") != nullptr || npfs->GetFile("main") != nullptr)) {
                            exefs = dirs.back();
                        }
                    }
                } else if (rights_id != RightsId{} && !header_reader.ExistsSparseLayer()) {
                    // Encrypted PartitionFS (e.g. ExeFS) failed PFS0 magic validation with this TitleKey
                    files.clear();
                    dirs.clear();
                    romfs = nullptr;
                    exefs = nullptr;
                    logo = nullptr;
                    return false;
                }
            }
        }
        if (GetType() == NCAContentType::Program && exefs == nullptr && !is_update && !has_sparse_layer) {
            return false;
        }
        return !files.empty() || !dirs.empty() || is_update;
    };

    if (rights_id != RightsId{}) {
        bool success = TryOpenFilesystems(decrypted_titlekey);
        if (!success && decrypted_titlekey != raw_titlekey) {
            LOG_DEBUG(Loader, "NCA: Decrypted TitleKey failed, falling back to raw pre-decrypted TitleKey");
            success = TryOpenFilesystems(raw_titlekey);
        }
    } else {
        TryOpenFilesystems({});
    }

    if (is_update && base_nca == nullptr) {
        status = Loader::ResultStatus::ErrorMissingBKTRBaseRomFS;
    } else if (fs_count > 0 && files.empty() && dirs.empty()) {
        status = Loader::ResultStatus::ErrorIncorrectTitlekeyOrTitlekek;
    } else {
        status = Loader::ResultStatus::Success;
    }
}

NCA::~NCA() = default;

Loader::ResultStatus NCA::GetStatus() const {
    return status;
}

std::vector<VirtualFile> NCA::GetFiles() const {
    if (status != Loader::ResultStatus::Success) {
        return {};
    }
    return files;
}

std::vector<VirtualDir> NCA::GetSubdirectories() const {
    if (status != Loader::ResultStatus::Success) {
        return {};
    }
    return dirs;
}

std::string NCA::GetName() const {
    return file->GetName();
}

VirtualDir NCA::GetParentDirectory() const {
    return file->GetContainingDirectory();
}

NCAContentType NCA::GetType() const {
    if (!reader) {
        return NCAContentType::Program;
    }
    return static_cast<NCAContentType>(reader->GetContentType());
}

u64 NCA::GetTitleId() const {
    if (!reader) {
        return 0;
    }
    if (is_update) {
        return reader->GetProgramId() | 0x800;
    }
    return reader->GetProgramId();
}

RightsId NCA::GetRightsId() const {
    RightsId result{};
    if (reader) {
        reader->GetRightsId(result.data(), result.size());
    }
    return result;
}

u32 NCA::GetSDKVersion() const {
    return reader ? reader->GetSdkAddonVersion() : 0;
}

u8 NCA::GetKeyGeneration() const {
    return reader ? reader->GetKeyGeneration() : 0;
}

bool NCA::IsUpdate() const {
    return is_update;
}

VirtualFile NCA::GetRomFS() const {
    if (status != Loader::ResultStatus::Success && status != Loader::ResultStatus::ErrorMissingBKTRBaseRomFS) {
        return nullptr;
    }
    return romfs;
}

VirtualDir NCA::GetExeFS() const {
    if (status != Loader::ResultStatus::Success && status != Loader::ResultStatus::ErrorMissingBKTRBaseRomFS) {
        return nullptr;
    }
    if (exefs != nullptr) {
        return exefs;
    }
    for (const auto& d : dirs) {
        if (d && (d->GetFile("main.npdm") != nullptr || d->GetFile("main") != nullptr)) {
            return d;
        }
    }
    return nullptr;
}

VirtualFile NCA::GetBaseFile() const {
    return file;
}

VirtualDir NCA::GetLogoPartition() const {
    if (status != Loader::ResultStatus::Success) {
        return nullptr;
    }
    return logo;
}

} // namespace FileSys
