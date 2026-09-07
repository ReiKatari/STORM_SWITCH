// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <regex>
#include "common/android/android_common.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/fs_filesystem.h"
#include "core/file_sys/patch_manager.h"
#include "core/loader/loader.h"
#include "core/loader/nro.h"
#include "native.h"

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
};
static ankerl::unordered_dense::map<std::string, RomMetadata> m_rom_metadata_cache;
static ankerl::unordered_dense::map<u64, int> m_aoc_count_cache;
static bool m_aoc_cache_valid = false;

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

static RomMetadata CacheRomMetadata(const std::string& path) {
    auto& instance = EmulationSession::GetInstance();
    const auto file = Core::GetGameFileFromPath(instance.System().GetFilesystem(), path);
    if (auto loader = Loader::GetLoader(instance.System(), file, 0, 0); loader) {
        RomMetadata entry;
        loader->ReadTitle(entry.title);
        loader->ReadProgramId(entry.programId);
        loader->ReadIcon(entry.icon);

        const u64 raw_pid = entry.programId;
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
                if (p.numeric_version > 0 && internal_ver == 0) {
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
        if (IsBaseVersion(entry.version)) {
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
            static const std::regex bracket_ver_regex(R"([\[\(]v?([0-9]+\.[0-9]+(?:\.[0-9]+)*)[\]\)])");
            static const std::regex vnum_regex(R"([\[\(]v?([0-9]{5,9})[\]\)])");

            std::smatch pair_match;
            if (std::regex_search(fn, pair_match, pair_regex)) {
                entry.version = pair_match[1].str();
                if (internal_ver == 0) {
                    try {
                        internal_ver = std::stoul(pair_match[2].str());
                    } catch (...) {}
                }
            } else {
                std::smatch b_match;
                if (std::regex_search(fn, b_match, bracket_ver_regex)) {
                    const std::string cand = b_match[1].str();
                    if (!IsBaseVersion(cand)) {
                        entry.version = cand;
                    }
                }
            }

            if (internal_ver == 0) {
                std::smatch vnum_match;
                if (std::regex_search(fn, vnum_match, vnum_regex)) {
                    try {
                        unsigned long v = std::stoul(vnum_match[1].str());
                        if (v > 0 && v <= 4294967295UL) {
                            internal_ver = static_cast<u32>(v);
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

        // 6. If internal_ver is 0 but display version is known (e.g. 1.0.9), calculate accurate internal version
        if (internal_ver == 0 && !IsBaseVersion(entry.version)) {
            int major = 1, minor = 0, patch_val = 0;
            if (std::sscanf(entry.version.c_str(), "%d.%d.%d", &major, &minor, &patch_val) >= 2) {
                if (major == 1 && minor == 0 && patch_val > 0) {
                    internal_ver = static_cast<u32>(patch_val * 65536);
                } else if (major >= 1) {
                    internal_ver = static_cast<u32>((major - 1) * 655360 + minor * 65536 + patch_val);
                }
            }
        }

        entry.internal_version = std::to_string(internal_ver);

        // Count DLC / Addons for this game from ContentProvider with cache
        int aoc_count = 0;
        if (auto it_aoc = m_aoc_count_cache.find(base_tid); it_aoc != m_aoc_count_cache.end()) {
            aoc_count = it_aoc->second;
        } else {
            const auto dlc_entries = instance.System().GetContentProvider().ListEntriesFilter(
                FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);
            for (const auto& dlc : dlc_entries) {
                if (FileSys::GetBaseTitleID(dlc.title_id) == base_tid) {
                    aoc_count++;
                }
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
        } else {
            entry.isHomebrew = false;
        }
        m_rom_metadata_cache[path] = entry;
        return entry;
    }
    return {};
}

static RomMetadata GetRomMetadata(const std::string& path, bool reload = false) {
    if (reload)
        return CacheRomMetadata(path);
    if (auto it = m_rom_metadata_cache.find(path); it != m_rom_metadata_cache.end())
        return it->second;
    return CacheRomMetadata(path);
}

extern "C" {

jboolean Java_org_yuzu_yuzu_1emu_utils_GameMetadata_getIsValid(JNIEnv* env, jobject obj, jstring jpath) {
    const std::string path_str = Common::Android::GetJString(env, jpath);
    const auto l_path = Common::ToLower(path_str);
    if (l_path.ends_with(".part") || l_path.ends_with(".tmp") ||
        l_path.ends_with(".crdownload") || l_path.ends_with(".downloading") ||
        l_path.ends_with(".incomplete") || l_path.ends_with(".!ut")) {
        return false;
    }

    if (auto const file = EmulationSession::GetInstance().System().GetFilesystem()->OpenFile(path_str, FileSys::OpenMode::Read); file) {
        if (file->GetSize() == 0) {
            return false;
        }
        if (auto loader = Loader::GetLoader(EmulationSession::GetInstance().System(), file); loader) {
            auto const file_type = loader->GetFileType();
            if (file_type == Loader::FileType::Unknown || file_type == Loader::FileType::Error)
                return false;
            if ((file_type == Loader::FileType::NSP || file_type == Loader::FileType::XCI ||
                 file_type == Loader::FileType::NSZ || file_type == Loader::FileType::XCZ) &&
                !Loader::IsBootableGameContainer(file, file_type))
                return false;
            u64 program_id = 0;
            std::vector<u64> pids;
            loader->ReadProgramIds(pids);
            if (!pids.empty()) {
                for (const auto id : pids) {
                    if ((id & 0xFFF) == 0) {
                        program_id = id;
                        break;
                    }
                }
            }
            if (program_id == 0) {
                loader->ReadProgramId(program_id);
            }
            if (file_type == Loader::FileType::NRO) {
                return true;
            }
            if (program_id == 0 || (program_id & 0xFFF) != 0) {
                return false; // Exclude standalone Updates (0x800) and DLCs (0x001+)
            }
            return true;
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
    if (meta.isHomebrew) {
        return jboolean(true);
    }
    return jboolean(meta.raw_program_id != 0 && meta.raw_program_id == meta.programId);
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
    m_rom_metadata_cache.clear();
    m_aoc_count_cache.clear();
    m_aoc_cache_valid = false;
}

} // extern "C"
