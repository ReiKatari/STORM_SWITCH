// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <ankerl/unordered_dense.h>
#include <regex>
#include <set>
#include "common/android/android_common.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/fs_filesystem.h"
#include "core/file_sys/patch_manager.h"
#include "core/loader/loader.h"
#include "core/loader/nro.h"
#include "native.h"
#include <fstream>
#include "common/fs/fs.h"
#include "common/fs/path_util.h"

namespace Common::FS {
#ifndef YuzuPath
using YuzuPath = EdenPath;
#endif
inline const std::filesystem::path& GetYuzuPath(EdenPath p) {
    return GetEdenPath(p);
}
} // namespace Common::FS

struct RomMetadata {
    std::string title;
    u64 programId{0};
    u64 raw_program_id{0};
    std::string developer;
    std::string version;
    std::string internal_version;
    int addon_count{0};
    std::vector<u8> icon;
    bool isHomebrew{false};
    bool is_base_game{false};
    u64 file_size{0};
};

static std::recursive_mutex s_metadata_mutex;
static ankerl::unordered_dense::map<std::string, RomMetadata> m_rom_metadata_cache;
static ankerl::unordered_dense::map<u64, int> m_aoc_count_cache;
static bool m_aoc_cache_valid = false;
static bool s_disk_cache_loaded = false;
static bool s_disk_cache_dirty = false;

static std::filesystem::path GetDiskCachePath() {
    return Common::FS::GetYuzuPath(Common::FS::YuzuPath::CacheDir) / "android_rom_metadata_v2.cache";
}

static void LoadDiskCache() {
    if (s_disk_cache_loaded) {
        return;
    }
    s_disk_cache_loaded = true;

    try {
        const auto cache_path = GetDiskCachePath();
        std::ifstream file(cache_path, std::ios::binary);
        if (!file.is_open()) {
            return;
        }

        u32 magic = 0;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x53574D44) { // 'SWMD'
            return;
        }

        u32 ver = 0;
        file.read(reinterpret_cast<char*>(&ver), sizeof(ver));
        if (ver != 2) {
            return;
        }

        u32 count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (u32 i = 0; i < count && file.good(); ++i) {
            u32 path_len = 0;
            file.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
            if (!file.good() || path_len > 65536) {
                break;
            }
            std::string entry_path(path_len, '\0');
            file.read(entry_path.data(), path_len);

            RomMetadata meta;
            file.read(reinterpret_cast<char*>(&meta.file_size), sizeof(meta.file_size));

            u32 title_len = 0;
            file.read(reinterpret_cast<char*>(&title_len), sizeof(title_len));
            meta.title.resize(title_len);
            if (title_len > 0) {
                file.read(meta.title.data(), title_len);
            }

            file.read(reinterpret_cast<char*>(&meta.programId), sizeof(meta.programId));
            file.read(reinterpret_cast<char*>(&meta.raw_program_id), sizeof(meta.raw_program_id));

            u32 dev_len = 0;
            file.read(reinterpret_cast<char*>(&dev_len), sizeof(dev_len));
            meta.developer.resize(dev_len);
            if (dev_len > 0) {
                file.read(meta.developer.data(), dev_len);
            }

            u32 ver_len = 0;
            file.read(reinterpret_cast<char*>(&ver_len), sizeof(ver_len));
            meta.version.resize(ver_len);
            if (ver_len > 0) {
                file.read(meta.version.data(), ver_len);
            }

            u32 iver_len = 0;
            file.read(reinterpret_cast<char*>(&iver_len), sizeof(iver_len));
            meta.internal_version.resize(iver_len);
            if (iver_len > 0) {
                file.read(meta.internal_version.data(), iver_len);
            }

            file.read(reinterpret_cast<char*>(&meta.addon_count), sizeof(meta.addon_count));

            u8 is_hb = 0, is_bg = 0;
            file.read(reinterpret_cast<char*>(&is_hb), sizeof(is_hb));
            file.read(reinterpret_cast<char*>(&is_bg), sizeof(is_bg));
            meta.isHomebrew = (is_hb != 0);
            meta.is_base_game = (is_bg != 0);

            u32 icon_len = 0;
            file.read(reinterpret_cast<char*>(&icon_len), sizeof(icon_len));
            if (icon_len > 0 && icon_len <= 10 * 1024 * 1024) {
                meta.icon.resize(icon_len);
                file.read(reinterpret_cast<char*>(meta.icon.data()), icon_len);
            }

            if (file.good()) {
                m_rom_metadata_cache[entry_path] = std::move(meta);
            }
        }
    } catch (...) {}
}

static void SaveDiskCache() {
    if (!s_disk_cache_dirty) {
        return;
    }
    try {
        const auto cache_path = GetDiskCachePath();
        void(Common::FS::CreateParentDirs(cache_path));
        std::ofstream file(cache_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return;
        }

        u32 magic = 0x53574D44; // 'SWMD'
        u32 ver = 2;
        u32 count = static_cast<u32>(m_rom_metadata_cache.size());

        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&ver), sizeof(ver));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& [entry_path, meta] : m_rom_metadata_cache) {
            u32 path_len = static_cast<u32>(entry_path.size());
            file.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
            file.write(entry_path.data(), path_len);

            file.write(reinterpret_cast<const char*>(&meta.file_size), sizeof(meta.file_size));

            u32 title_len = static_cast<u32>(meta.title.size());
            file.write(reinterpret_cast<const char*>(&title_len), sizeof(title_len));
            if (title_len > 0) {
                file.write(meta.title.data(), title_len);
            }

            file.write(reinterpret_cast<const char*>(&meta.programId), sizeof(meta.programId));
            file.write(reinterpret_cast<const char*>(&meta.raw_program_id), sizeof(meta.raw_program_id));

            u32 dev_len = static_cast<u32>(meta.developer.size());
            file.write(reinterpret_cast<const char*>(&dev_len), sizeof(dev_len));
            if (dev_len > 0) {
                file.write(meta.developer.data(), dev_len);
            }

            u32 ver_len = static_cast<u32>(meta.version.size());
            file.write(reinterpret_cast<const char*>(&ver_len), sizeof(ver_len));
            if (ver_len > 0) {
                file.write(meta.version.data(), ver_len);
            }

            u32 iver_len = static_cast<u32>(meta.internal_version.size());
            file.write(reinterpret_cast<const char*>(&iver_len), sizeof(iver_len));
            if (iver_len > 0) {
                file.write(meta.internal_version.data(), iver_len);
            }

            file.write(reinterpret_cast<const char*>(&meta.addon_count), sizeof(meta.addon_count));

            u8 is_hb = meta.isHomebrew ? 1 : 0;
            u8 is_bg = meta.is_base_game ? 1 : 0;
            file.write(reinterpret_cast<const char*>(&is_hb), sizeof(is_hb));
            file.write(reinterpret_cast<const char*>(&is_bg), sizeof(is_bg));

            u32 icon_len = static_cast<u32>(meta.icon.size());
            file.write(reinterpret_cast<const char*>(&icon_len), sizeof(icon_len));
            if (icon_len > 0) {
                file.write(reinterpret_cast<const char*>(meta.icon.data()), icon_len);
            }
        }
        s_disk_cache_dirty = false;
    } catch (...) {}
}


static bool IsBaseVersion(std::string_view ver) {
    while (!ver.empty() && (ver.front() == 'v' || ver.front() == 'V' || std::isspace(static_cast<unsigned char>(ver.front())))) {
        ver.remove_prefix(1);
    }
    while (!ver.empty() && std::isspace(static_cast<unsigned char>(ver.back()))) {
        ver.remove_suffix(1);
    }
    return ver.empty() || ver == "0" || ver == "1.0" || ver == "1.0.0" || ver == "1.0.0.0" ||
           ver.starts_with("0.");
}

static RomMetadata CacheRomMetadata(const std::string& raw_path) {
    std::string path = raw_path;
    if (path.starts_with("file://")) {
        path = path.substr(7);
    }
    if (!Core::Crypto::KeyManager::Instance().AreKeysLoaded()) {
        Core::Crypto::KeyManager::Instance().ReloadKeys();
    }
    auto& instance = EmulationSession::GetInstance();
    auto* content_provider = instance.GetContentProvider();
    const auto file = Core::GetGameFileFromPath(instance.System().GetFilesystem(), path);
    if (file && content_provider) {
        content_provider->AddEntriesFromContainer(file);
    }
    if (auto loader = Loader::GetLoader(instance.System(), file, 0, 0); loader) {
        RomMetadata entry;
        loader->ReadTitle(entry.title);
        loader->ReadProgramId(entry.programId);
        loader->ReadIcon(entry.icon);

        u64 raw_pid = entry.programId;
        std::vector<u64> pids;
        loader->ReadProgramIds(pids);
        if (!pids.empty()) {
            for (const auto id : pids) {
                if ((id & 0xFFF) == 0 && (id & 0x800) == 0) {
                    raw_pid = id;
                    break;
                }
            }
        }
        if (raw_pid == 0) {
            std::regex id_regex(R"(0100[0-9a-fA-F]{12})");
            std::smatch match;
            if (std::regex_search(path, match, id_regex)) {
                try {
                    raw_pid = std::stoull(match.str(), nullptr, 16);
                } catch (...) {}
            }
        }

        entry.raw_program_id = raw_pid;
        const u64 base_tid = FileSys::GetBaseTitleID(raw_pid);
        const u64 update_tid = FileSys::GetUpdateTitleID(base_tid);
        entry.programId = base_tid;

        FileSys::NACP nacp{};
        bool has_embedded_nacp = (loader->ReadControlData(nacp) == Loader::ResultStatus::Success);

        const FileSys::PatchManager pm{
            base_tid,
            instance.System().GetFileSystemController(),
            instance.System().GetContentProvider()
        };
        const auto control = pm.GetControlMetadata();
        const auto game_version = pm.GetGameVersion();

        if (control.second != nullptr && control.second->GetSize() > 0) {
            entry.icon = control.second->ReadAllBytes();
        }

        // 1. Resolve internal numeric version directly from ContentProvider (Update TID / Base TID / PM)
        u32 internal_ver = instance.System().GetContentProvider().GetEntryVersion(update_tid).value_or(0);
        if (internal_ver == 0 && game_version.has_value() && *game_version > 0) {
            internal_ver = *game_version;
        }
        if (internal_ver == 0) {
            internal_ver = instance.System().GetContentProvider().GetEntryVersion(base_tid).value_or(0);
        }

        // 2. Resolve developer and display version from Control Metadata (Update NACP if update present, else base NACP)
        if (control.first != nullptr && !control.first->GetVersionString().empty()) {
            entry.version = control.first->GetVersionString();
            entry.developer = control.first->GetDeveloperName();
        } else if (has_embedded_nacp && !nacp.GetVersionString().empty()) {
            entry.version = nacp.GetVersionString();
            entry.developer = nacp.GetDeveloperName();
        } else {
            entry.developer = "";
        }

        // 3. Try highest priority Update patch from PatchManager / ContentProvider (including bundled container updates)
        FileSys::VirtualFile update_raw_file;
        loader->ReadUpdateRaw(update_raw_file);
        const auto all_patches = pm.GetPatches(update_raw_file);
        for (const auto& p : all_patches) {
            if (p.type == FileSys::PatchType::Update && p.enabled) {
                if (p.numeric_version > 0 && (internal_ver == 0 || p.numeric_version > internal_ver)) {
                    internal_ver = p.numeric_version;
                }
                if (IsBaseVersion(entry.version) && !p.version.empty() && p.version != "PACKED" && !p.version.starts_with("0.")) {
                    entry.version = p.version;
                }
                break;
            }
        }

        // Clean version string: remove leading 'v' / 'V' and whitespace
        while (entry.version.starts_with('v') || entry.version.starts_with('V')) {
            entry.version = entry.version.substr(1);
        }
        while (!entry.version.empty() && (entry.version.front() == ' ' || entry.version.front() == '\t')) {
            entry.version.erase(entry.version.begin());
        }
        while (!entry.version.empty() && (entry.version.back() == ' ' || entry.version.back() == '\t')) {
            entry.version.pop_back();
        }

        // 4. Try extracting paired or standalone version from leaf filename only (e.g. "(1.0.9 - 458752)")
        {
            auto url_decode = [](std::string_view in) -> std::string {
                std::string out;
                out.reserve(in.size());
                for (std::size_t i = 0; i < in.size(); ++i) {
                    if (in[i] == '%' && i + 2 < in.size()) {
                        int val = 0;
                        if (std::sscanf(std::string(in.substr(i + 1, 2)).c_str(), "%x", &val) == 1) {
                            out += static_cast<char>(val);
                            i += 2;
                            continue;
                        }
                    } else if (in[i] == '+') {
                        out += ' ';
                        continue;
                    }
                    out += in[i];
                }
                return out;
            };

            const std::string decoded_path = url_decode(path);
            auto get_filename_only = [](const std::string& full) -> std::string {
                auto pos = full.find_last_of("/\\");
                if (pos != std::string::npos && pos + 1 < full.size()) {
                    return full.substr(pos + 1);
                }
                return full;
            };

            const std::string fn = get_filename_only(decoded_path);

            static const std::regex pair_regex(R"([\(\[]([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+)(?:\s*-\s*[0-9A-Fa-f]+)?[\)\]])");
            static const std::regex bracket_ver_regex(R"([\[\(](?:v|ver|upd|update)?\s*([0-9]+\.[0-9]+(?:\.[0-9]+)*)[\]\)])", std::regex::icase);
            static const std::regex vnum_regex(R"([\[\(](?:v|ver|upd|update)?\s*([0-9]{5,9})[\]\)])", std::regex::icase);

            for (std::sregex_iterator it(fn.begin(), fn.end(), pair_regex), end_it; it != end_it; ++it) {
                const std::string cand_ver = (*it)[1].str();
                u32 cand_int_ver = 0;
                try {
                    cand_int_ver = std::stoul((*it)[2].str());
                } catch (...) {}
                if (cand_int_ver > 0 && !IsBaseVersion(cand_ver)) {
                    entry.version = cand_ver;
                    internal_ver = cand_int_ver;
                    break;
                }
            }

            if (IsBaseVersion(entry.version) || internal_ver == 0) {
                for (std::sregex_iterator it(fn.begin(), fn.end(), pair_regex), end_it; it != end_it; ++it) {
                    const std::string cand_ver = (*it)[1].str();
                    if (!IsBaseVersion(cand_ver)) {
                        entry.version = cand_ver;
                        if (internal_ver == 0) {
                            try {
                                internal_ver = std::stoul((*it)[2].str());
                            } catch (...) {}
                        }
                        break;
                    }
                }
            }

            if (IsBaseVersion(entry.version)) {
                for (std::sregex_iterator it(fn.begin(), fn.end(), bracket_ver_regex), end_it; it != end_it; ++it) {
                    const std::string cand = (*it)[1].str();
                    if (!IsBaseVersion(cand)) {
                        entry.version = cand;
                        break;
                    }
                }
            }

            if (internal_ver == 0) {
                for (std::sregex_iterator it(fn.begin(), fn.end(), vnum_regex), end_it; it != end_it; ++it) {
                    try {
                        unsigned long v = std::stoul((*it)[1].str());
                        if (v > 0 && v <= 4294967295UL) {
                            internal_ver = static_cast<u32>(v);
                            break;
                        }
                    } catch (...) {}
                }
            }
        }

        // 5. Format display version from internal numeric version if still base version
        if (IsBaseVersion(entry.version) && internal_ver > 0) {
            u32 update_num = internal_ver / 65536;
            if (update_num > 0) {
                entry.version = fmt::format("1.0.{}", update_num);
            } else {
                u32 major = (internal_ver >> 16) & 0xFF;
                u32 minor = (internal_ver >> 8) & 0xFF;
                u32 patch_val = internal_ver & 0xFF;
                if (major > 0 || minor > 0 || patch_val > 0) {
                    entry.version = fmt::format("{}.{}.{}", major, minor, patch_val);
                } else {
                    entry.version = "1.0.0";
                }
            }
        }

        if (entry.version.empty()) {
            entry.version = "1.0.0";
        }

        // 6. If internal_ver is 0 but display version is known (e.g. 1.0.9 or 1.0.0.13), calculate accurate internal version
        if (internal_ver == 0 && !IsBaseVersion(entry.version)) {
            int major = 1, minor = 0, patch_val = 0, build_val = 0;
            int scanned = std::sscanf(entry.version.c_str(), "%d.%d.%d.%d", &major, &minor, &patch_val, &build_val);
            if (scanned == 4 && major == 1 && minor == 0 && patch_val == 0 && build_val > 0) {
                internal_ver = static_cast<u32>(build_val * 65536);
            } else if (scanned >= 2) {
                if (major == 1 && minor == 0 && patch_val > 0) {
                    internal_ver = static_cast<u32>(patch_val * 65536);
                } else if (major >= 1) {
                    internal_ver = static_cast<u32>((major - 1) * 655360 + minor * 65536 + patch_val);
                }
            }
        }

        entry.internal_version = std::to_string(internal_ver);

        // Count DLC / Addons for this game from ContentProvider freshly so newly mounted DLCs are always included
        int aoc_count = 0;
        {
            const auto dlc_entries = instance.System().GetContentProvider().ListEntriesFilter(
                FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);
            std::set<u64> unique_dlc_tids;
            for (const auto& dlc : dlc_entries) {
                if (FileSys::GetBaseTitleID(dlc.title_id) == base_tid) {
                    unique_dlc_tids.insert(dlc.title_id);
                }
            }
            aoc_count = static_cast<int>(unique_dlc_tids.size());
            if (auto it_aoc = m_aoc_count_cache.find(base_tid); it_aoc != m_aoc_count_cache.end()) {
                aoc_count = std::max(aoc_count, it_aoc->second);
            }
            m_aoc_count_cache[base_tid] = aoc_count;
        }
        if (aoc_count == 0) {
            auto prev_it = m_rom_metadata_cache.find(path);
            if (prev_it != m_rom_metadata_cache.end() && prev_it->second.addon_count > 0) {
                aoc_count = prev_it->second.addon_count;
            }
        }
        entry.addon_count = aoc_count;

        if (loader->GetFileType() == Loader::FileType::NRO) {
            auto loader_nro = reinterpret_cast<Loader::AppLoader_NRO*>(loader.get());
            entry.isHomebrew = loader_nro->IsHomebrew();
            entry.is_base_game = true;
        } else {
            entry.isHomebrew = false;
            const auto ftype = loader->GetFileType();
            const bool is_xci = (ftype == Loader::FileType::XCI || ftype == Loader::FileType::XCZ);
            const bool is_nsp = (ftype == Loader::FileType::NSP || ftype == Loader::FileType::NSZ);
            bool bootable = false;
            if (is_xci) {
                bootable = true;
            } else if (is_nsp) {
                bootable = Loader::IsBootableGameContainer(file, ftype);
                if (!bootable && raw_pid != 0 && (raw_pid & 0xFFF) == 0 && (raw_pid & 0x800) == 0) {
                    bootable = true;
                }
            } else {
                bootable = (raw_pid != 0 && (raw_pid & 0xFFF) == 0 && (raw_pid & 0x800) == 0);
            }
            entry.is_base_game = bootable;
        }
        entry.file_size = file ? file->GetSize() : 0;
        m_rom_metadata_cache[path] = entry;
        s_disk_cache_dirty = true;
        SaveDiskCache();
        return entry;
    }
    return {};
}

static RomMetadata GetRomMetadata(const std::string& raw_path, bool reload = false) {
    std::scoped_lock lock{s_metadata_mutex};
    LoadDiskCache();
    std::string path = raw_path;
    if (path.starts_with("file://")) {
        path = path.substr(7);
    }
    if (!reload) {
        if (auto it = m_rom_metadata_cache.find(path); it != m_rom_metadata_cache.end()) {
            return it->second;
        }
    }
    return CacheRomMetadata(raw_path);
}

extern "C" {

jboolean Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getIsValid(JNIEnv* env, jobject obj, jstring jpath) {
    std::string path_str = Common::Android::GetJString(env, jpath);
    if (path_str.starts_with("file://")) {
        path_str = path_str.substr(7);
    }
    const auto l_path = Common::ToLower(path_str);
    if (l_path.ends_with(".part") || l_path.ends_with(".tmp") ||
        l_path.ends_with(".crdownload") || l_path.ends_with(".downloading") ||
        l_path.ends_with(".incomplete") || l_path.ends_with(".!ut")) {
        return false;
    }

    if (!Core::Crypto::KeyManager::Instance().AreKeysLoaded()) {
        Core::Crypto::KeyManager::Instance().ReloadKeys();
    }

    if (auto const file = EmulationSession::GetInstance().System().GetFilesystem()->OpenFile(path_str, FileSys::OpenMode::Read); file) {
        if (file->GetSize() == 0) {
            return false;
        }
        if (auto loader = Loader::GetLoader(EmulationSession::GetInstance().System(), file); loader) {
            auto const file_type = loader->GetFileType();
            if (file_type == Loader::FileType::Unknown || file_type == Loader::FileType::Error)
                return false;

            if (file_type == Loader::FileType::NRO) {
                return true;
            }

            u64 program_id = 0;
            std::vector<u64> pids;
            loader->ReadProgramIds(pids);
            if (!pids.empty()) {
                for (const auto id : pids) {
                    if ((id & 0xFFF) == 0 && (id & 0x800) == 0) {
                        program_id = id;
                        break;
                    }
                }
            }
            if (program_id == 0) {
                loader->ReadProgramId(program_id);
            }

            const bool is_xci = (file_type == Loader::FileType::XCI || file_type == Loader::FileType::XCZ);
            const bool is_container = (is_xci || file_type == Loader::FileType::NSP || file_type == Loader::FileType::NSZ);

            // Cartridges (XCI/XCZ) are always game images
            if (is_xci) {
                return true;
            }

            // For NSP/NSZ containers, strictly verify that the container holds a bootable base game NCA
            // and is NOT a standalone Update (0x800) or DLC (0x001+) package
            if (is_container) {
                if (Loader::IsBootableGameContainer(file, file_type)) {
                    return true;
                }
                if (program_id != 0 && (program_id & 0xFFF) != 0) {
                    return false;
                }
                for (const auto id : pids) {
                    if ((id & 0xFFF) != 0) {
                        return false;
                    }
                }
            }

            // Exclude standalone Updates (0x800) and DLCs (0x001+)
            if (program_id != 0 && (program_id & 0xFFF) != 0) {
                return false;
            }

            if (program_id != 0 && (program_id & 0xFFF) == 0 && (program_id & 0x800) == 0) {
                return true;
            }

            return false;
        }
    }
    return false;
}

jstring Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getTitle(JNIEnv* env, jobject obj, jstring jpath) {
    return Common::Android::ToJString(env, GetRomMetadata(Common::Android::GetJString(env, jpath)).title);
}

jstring Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getProgramId(JNIEnv* env, jobject obj, jstring jpath) {
    const auto meta = GetRomMetadata(Common::Android::GetJString(env, jpath));
    return Common::Android::ToJString(env, fmt::format("{:016X}", meta.programId));
}

jboolean Java_org_yuzu_yuzu_1emu_utils_GameMetadata_isBaseGame(JNIEnv* env, jobject obj, jstring jpath) {
    const auto meta = GetRomMetadata(Common::Android::GetJString(env, jpath));
    return jboolean(meta.is_base_game);
}

jstring Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getDeveloper(JNIEnv* env, jobject obj, jstring jpath) {
    return Common::Android::ToJString(env, GetRomMetadata(Common::Android::GetJString(env, jpath)).developer);
}

jstring Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getVersion(JNIEnv* env, jobject obj, jstring jpath, jboolean jreload) {
    return Common::Android::ToJString(env, GetRomMetadata(Common::Android::GetJString(env, jpath), jreload).version);
}

jstring Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getInternalVersion(JNIEnv* env, jobject obj, jstring jpath) {
    return Common::Android::ToJString(env, GetRomMetadata(Common::Android::GetJString(env, jpath)).internal_version);
}

jint Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getAddonCount(JNIEnv* env, jobject obj, jstring jpath) {
    return jint(GetRomMetadata(Common::Android::GetJString(env, jpath)).addon_count);
}

jbyteArray Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getIcon(JNIEnv* env, jobject obj, jstring jpath) {
    auto icon_data = GetRomMetadata(Common::Android::GetJString(env, jpath)).icon;
    jbyteArray icon = env->NewByteArray(jsize(icon_data.size()));
    env->SetByteArrayRegion(icon, 0, env->GetArrayLength(icon), reinterpret_cast<jbyte*>(icon_data.data()));
    return icon;
}

jboolean Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getIsHomebrew(JNIEnv* env, jobject obj, jstring jpath) {
    return jboolean(GetRomMetadata(Common::Android::GetJString(env, jpath)).isHomebrew);
}

void Java_org_yuzu_yuzu_1emu_utils_GameMetadata_resetMetadata(JNIEnv* env, jobject obj) {
    std::scoped_lock lock{s_metadata_mutex};
    m_rom_metadata_cache.clear();
    m_aoc_count_cache.clear();
    m_aoc_cache_valid = false;
}

} // extern "C"
