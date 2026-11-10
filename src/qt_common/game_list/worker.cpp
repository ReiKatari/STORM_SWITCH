// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <thread>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>

#include "common/cityhash.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/fs_filesystem.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/submission_package.h"
#include "core/loader/loader.h"

#include "qt_common/config/uisettings.h"
#include "qt_common/qt_common.h"

#include "qt_common/game_list/game_list_p.h"

#include "qt_common/game_list/model.h"
#include "qt_common/game_list/worker.h"
#include "qt_common/titledb.h"

#include <regex>

namespace {

QString GetGameListCachedObject(const std::string& filename, const std::string& ext,
                                const std::function<QString()>& generator) {
    if (!UISettings::values.cache_game_list || filename == "0000000000000000") {
        return generator();
    }

    const auto path =
        Common::FS::PathToUTF8String(Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) /
                                     "game_list" / fmt::format("{}.{}", filename, ext));

    void(Common::FS::CreateParentDirs(path));

    if (!Common::FS::Exists(path)) {
        const auto str = generator();

        QFile file{QString::fromStdString(path)};
        if (file.open(QFile::WriteOnly)) {
            file.write(str.toUtf8());
        }

        return str;
    }

    QFile file{QString::fromStdString(path)};
    if (file.open(QFile::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }

    return generator();
}

std::pair<std::vector<u8>, std::string> GetGameListCachedObject(
    const std::string& filename, const std::string& ext,
    const std::function<std::pair<std::vector<u8>, std::string>()>& generator) {
    if (!UISettings::values.cache_game_list || filename == "0000000000000000") {
        return generator();
    }

    const auto game_list_dir =
        Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
    const auto jpeg_name = fmt::format("{}.jpeg", filename);
    const auto app_name = fmt::format("{}.appname.txt", filename);

    const auto path1 = Common::FS::PathToUTF8String(game_list_dir / jpeg_name);
    const auto path2 = Common::FS::PathToUTF8String(game_list_dir / app_name);

    void(Common::FS::CreateParentDirs(path1));

    if (!Common::FS::Exists(path1) || !Common::FS::Exists(path2)) {
        const auto [icon, nacp] = generator();

        if (!icon.empty()) {
            QFile file1{QString::fromStdString(path1)};
            if (file1.open(QFile::WriteOnly)) {
                file1.resize(icon.size());
                file1.write(reinterpret_cast<const char*>(icon.data()), icon.size());
            }
        }

        QFile file2{QString::fromStdString(path2)};
        if (file2.open(QFile::WriteOnly)) {
            file2.write(nacp.data(), nacp.size());
        }

        return std::make_pair(icon, nacp);
    }

    QFile file1(QString::fromStdString(path1));
    QFile file2(QString::fromStdString(path2));

    std::vector<u8> vec;
    if (file1.exists() && file1.open(QFile::ReadOnly)) {
        vec.resize(file1.size());
        if (file1.read(reinterpret_cast<char*>(vec.data()), vec.size()) !=
            static_cast<s64>(vec.size())) {
            vec.clear();
        }
    }

    std::string data_str;
    if (file2.exists() && file2.open(QFile::ReadOnly)) {
        data_str = file2.readAll().toStdString();
    }

    return std::make_pair(std::move(vec), std::move(data_str));
}

void GetMetadataFromControlNCA(const FileSys::PatchManager& patch_manager, const FileSys::NCA& nca,
                               std::vector<u8>& icon, std::string& name) {
    std::tie(icon, name) = GetGameListCachedObject(
        fmt::format("{:016X}", patch_manager.GetTitleID()), {}, [&patch_manager, &nca] {
            const auto [nacp, icon_f] = patch_manager.ParseControlNCA(nca);
            std::vector<u8> icon_bytes;
            if (icon_f != nullptr) {
                icon_bytes = icon_f->ReadAllBytes();
            }
            std::string app_name;
            if (nacp != nullptr) {
                app_name = nacp->GetApplicationName();
            }
            return std::make_pair(std::move(icon_bytes), std::move(app_name));
        });
}

[[maybe_unused]] bool HasSupportedFileExtension(const std::string& file_name) {
    const QFileInfo file = QFileInfo(QString::fromStdString(file_name));
    return QtCommon::supported_file_extensions.contains(file.suffix(), Qt::CaseInsensitive);
}

[[maybe_unused]] bool IsExtractedNCAMain(const std::string& file_name) {
    return QFileInfo(QString::fromStdString(file_name)).fileName() == QStringLiteral("main");
}

// FormatGameName is defined inline in game_list_p.h

QString FormatPatchNameVersions(const FileSys::PatchManager& patch_manager,
                                Loader::AppLoader& loader, bool updatable = true) {
    QString out;
    FileSys::VirtualFile update_raw;
    loader.ReadUpdateRaw(update_raw);
    for (const auto& patch : patch_manager.GetPatches(update_raw)) {
        const bool is_update = patch.name == "Update";
        if (!updatable && is_update) {
            continue;
        }

        const QString type =
            QString::fromStdString(patch.enabled ? patch.name : "[D] " + patch.name);

        if (patch.version.empty()) {
            out.append(QStringLiteral("%1\n").arg(type));
        } else {
            auto ver = patch.version;

            // Display container name for packed updates
            if (is_update && ver == "PACKED") {
                ver = Loader::GetFileTypeString(loader.GetFileType());
            }

            out.append(QStringLiteral("%1 (%2)\n").arg(type, QString::fromStdString(ver)));
        }
    }

    out.chop(1);
    return out;
}

bool IsBaseVersion(const QString& ver) {
    QString v = ver.trimmed();
    while (v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        v.remove(0, 1);
    }
    v = v.trimmed();
    return v.isEmpty() || v == QStringLiteral("0") || v == QStringLiteral("1.0") ||
           v == QStringLiteral("1.0.0") || v == QStringLiteral("1.0.0.0");
}

QString FormatAddonsColumnText(const QString& patch_versions, const QString& base_version = QStringLiteral("1.0.0"), const std::string& path = {}) {
    QString version_num = base_version.trimmed();
    while (version_num.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        version_num.remove(0, 1);
    }
    version_num = version_num.trimmed();
    if (version_num.isEmpty() || version_num == QStringLiteral("0")) {
        version_num = QStringLiteral("1.0.0");
    }

    int dlc_count = 0;
    const QStringList lines = patch_versions.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.contains(QStringLiteral("DLC"), Qt::CaseInsensitive) ||
            line.contains(QStringLiteral("Дополнение"), Qt::CaseInsensitive)) {
            QString clean = line.trimmed();
            clean.replace(QStringLiteral("DLC "), QStringLiteral(""), Qt::CaseInsensitive);
            clean.replace(QStringLiteral("DLC"), QStringLiteral(""), Qt::CaseInsensitive);
            clean.replace(QStringLiteral("Дополнение "), QStringLiteral(""), Qt::CaseInsensitive);
            clean.replace(QStringLiteral("Дополнение"), QStringLiteral(""), Qt::CaseInsensitive);
            clean.remove(QLatin1Char('['));
            clean.remove(QLatin1Char(']'));
            clean.remove(QLatin1Char('D'));
            clean.remove(QLatin1Char('d'));
            clean.remove(QLatin1Char('('));
            clean.remove(QLatin1Char(')'));
            clean = clean.trimmed();
            if (!clean.isEmpty()) {
                const auto parts = clean.split(QLatin1Char(','), Qt::SkipEmptyParts);
                dlc_count += parts.isEmpty() ? 1 : parts.size();
            } else {
                dlc_count += 1;
            }
        }
    }

    // Check stitched game filename tags like (1G+3D) or +3D or +3DLC if no DLC detected from patches
    if (dlc_count == 0 && !path.empty()) {
        static const QRegularExpression fn_dlc_tag{QStringLiteral(R"((?:\+|\b)([0-9]+)D(?:LC)?(?:\b|\)))"), QRegularExpression::CaseInsensitiveOption};
        const auto dm = fn_dlc_tag.match(QString::fromStdString(path));
        if (dm.hasMatch() && dm.hasCaptured(1)) {
            dlc_count = dm.captured(1).toInt();
        }
    }

    QString result = QObject::tr("Версия: %1").arg(version_num);
    if (dlc_count > 0) {
        const QString addons_word = dlc_count == 1 ? QObject::tr("Дополнение") : QObject::tr("Дополнения");
        result.append(QStringLiteral("\n%1: %2").arg(addons_word, QString::number(dlc_count)));
    }
    return result;
}

// MakeCachedGameListEntry is defined inline in game_list_p.h

QList<QStandardItem*> MakeGameListEntry(const std::string& path, const std::string& name,
                                        const std::size_t size, const std::vector<u8>& icon,
                                        Loader::AppLoader& loader, u64 program_id,
                                        const PlayTime::PlayTimeManager& play_time_manager,
                                        const FileSys::PatchManager& patch,
                                        QString* out_file_type = nullptr,
                                        QString* out_patch_versions = nullptr,
                                        QString* out_file_version = nullptr,
                                        QString* out_addons_text = nullptr) {
    auto const file_type = loader.GetFileType();
    QString file_type_string = QString::fromStdString(Loader::GetFileTypeString(file_type));
    const auto ext = Common::ToLower(std::string(Common::FS::GetExtensionFromFilename(path)));
    if (ext == "nsz") {
        file_type_string = QStringLiteral("NSZ");
    } else if (ext == "xcz") {
        file_type_string = QStringLiteral("XCZ");
    } else if (ext == "nup") {
        file_type_string = QStringLiteral("NSP");
    }

    const u64 file_path_hash = Common::CityHash64(path.data(), path.size());
    const std::string cache_key = fmt::format("{:016X}_{:016X}", program_id, file_path_hash);
    QString patch_versions = GetGameListCachedObject(
        cache_key, "pv.txt", [&patch, &loader] {
            return FormatPatchNameVersions(patch, loader, loader.IsRomFSUpdatable());
        });

    // Determine the exact version for this specific file directly from metadata (cached)
    QString file_version = GetGameListCachedObject(
        cache_key, "fv.txt", [&patch, &loader, &path, &patch_versions]() {
            QString ver_result;

            // 1. Try parsing update version from already resolved patch_versions (zero overhead)
            static const QRegularExpression update_ver_regex{QStringLiteral(R"(Update\s*\(([^)]+)\))")};
            const auto um = update_ver_regex.match(patch_versions);
            if (um.hasMatch() && !um.captured(1).isEmpty()) {
                const auto uv = um.captured(1).trimmed();
                if (uv != QStringLiteral("PACKED") && uv != QStringLiteral("NSP") && uv != QStringLiteral("XCI") && !uv.isEmpty()) {
                    ver_result = uv;
                }
            }

            // 2. Try highest priority Update patch from PatchManager / ContentProvider if still needed
            if (IsBaseVersion(ver_result)) {
                FileSys::VirtualFile update_raw_file;
                loader.ReadUpdateRaw(update_raw_file);
                const auto all_patches = patch.GetPatches(update_raw_file);
                for (const auto& p : all_patches) {
                    if (p.type == FileSys::PatchType::Update && p.enabled && !p.version.empty() && p.version != "PACKED") {
                        ver_result = QString::fromStdString(p.version);
                        break;
                    }
                }
            }

            // Priority Check: If filename explicitly encodes an update container with internal_version > 0
            // (e.g. "(1.0.6 - 65536 - 0100811027E58000) (1G+1U)"), extract that version directly,
            // as embedded base NACP in multi-content packages only reports base game initial version (e.g. 1.0.1).
            const QString qpath = QString::fromStdString(path);
            static const QRegularExpression fn_pair_update_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
            const auto pum = fn_pair_update_regex.match(qpath);
            if (pum.hasMatch() && !pum.captured(1).isEmpty()) {
                const u64 int_ver = pum.captured(2).toULongLong();
                if (int_ver > 0) {
                    if (!IsBaseVersion(pum.captured(1))) {
                        ver_result = pum.captured(1);
                    } else if (IsBaseVersion(ver_result) && (int_ver % 65536ULL) == 0 && (int_ver / 65536ULL) <= 200) {
                        ver_result = QStringLiteral("1.0.%1").arg(int_ver / 65536ULL);
                    }
                }
            }

            // 3. Try reading control data directly from loader (fastest, zero PatchRomFS overhead)
            if (IsBaseVersion(ver_result)) {
                FileSys::NACP file_nacp;
                if (loader.ReadControlData(file_nacp) == Loader::ResultStatus::Success) {
                    auto ver = file_nacp.GetVersionString();
                    if (!ver.empty() && ver != "0") {
                        ver_result = QString::fromStdString(ver);
                    }
                }
            }

            // 4. Fallback to PatchManager control metadata (NACP)
            if (IsBaseVersion(ver_result)) {
                if (const auto nacp = patch.GetControlMetadata().first; nacp != nullptr) {
                    const auto ver = nacp->GetVersionString();
                    if (!ver.empty() && ver != "0") {
                        ver_result = QString::fromStdString(ver);
                    }
                }
            }

            // 5. Try extracting paired version from filename (e.g. "(1.0.10 - 655360 - ...)")
            if (IsBaseVersion(ver_result)) {
                static const QRegularExpression fn_pair_ver_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
                const auto fm = fn_pair_ver_regex.match(qpath);
                if (fm.hasMatch() && !fm.captured(1).isEmpty()) {
                    ver_result = fm.captured(1);
                } else {
                    static const QRegularExpression fn_ver_regex{QStringLiteral(R"((?:[\(\[\s_]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))")};
                    const auto m = fn_ver_regex.match(qpath);
                    if (m.hasMatch() && m.hasCaptured(1)) {
                        const QString parsed_ver = m.captured(1);
                        if (!parsed_ver.isEmpty() && !IsBaseVersion(parsed_ver)) {
                            ver_result = parsed_ver;
                        }
                    }
                }
            }

            // 6. Try [v12345] style numeric version in filename if still base version
            if (IsBaseVersion(ver_result)) {
                static const QRegularExpression fn_vnum_regex{QStringLiteral(R"(\[v([0-9]+)\])")};
                const auto vm = fn_vnum_regex.match(qpath);
                if (vm.hasMatch()) {
                    const u32 vnum = vm.captured(1).toUInt();
                    if (vnum > 0) {
                        ver_result = QString::number(vnum);
                    }
                }
            }

            // Clean leading 'v' or whitespace
            while (ver_result.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
                ver_result.remove(0, 1);
            }
            ver_result = ver_result.trimmed();

            if (ver_result.isEmpty() || ver_result == QStringLiteral("0")) {
                ver_result = QStringLiteral("1.0.0");
            }

            return ver_result;
        });

    QString addons_text = GetGameListCachedObject(
        cache_key, "addons.txt", [&patch_versions, &file_version, &path]() {
            return FormatAddonsColumnText(patch_versions, file_version, path);
        });

    if (out_file_type) {
        *out_file_type = file_type_string;
    }
    if (out_patch_versions) {
        *out_patch_versions = patch_versions;
    }
    if (out_file_version) {
        *out_file_version = file_version;
    }
    if (out_addons_text) {
        *out_addons_text = addons_text;
    }

    return MakeCachedGameListEntry(path, name, size, icon, file_type_string, program_id,
                                   play_time_manager, patch_versions, file_version, addons_text);
}
} // Anonymous namespace

GameListWorker::GameListWorker(FileSys::VirtualFilesystem vfs_,
                               FileSys::ManualContentProvider* provider_,
                               QVector<UISettings::GameDir>& game_dirs_,
                               const PlayTime::PlayTimeManager& play_time_manager_,
                               Core::System& system_)
    : vfs{std::move(vfs_)}, provider{provider_}, game_dirs{game_dirs_},
      play_time_manager{play_time_manager_},
      system{system_} {
    // We want the game list to manage our lifetime.
    setAutoDelete(false);
}

GameListWorker::~GameListWorker() {
    this->disconnect();
    stop_requested.store(true);
    processing_completed.Wait();
}

struct CachedGameEntry {
    u64 file_size = 0;
    u64 mtime = 0;
    u64 program_id = 0;
    std::string name;
    std::string file_type_string;
    std::string patch_versions;
    std::string file_version;
    std::string addons_text;
    std::vector<u8> icon;
    bool is_bootable = true;
    u64 mods_mtime = 0;
};

static std::mutex s_mem_cache_mutex;
static std::unordered_map<std::string, CachedGameEntry> s_mem_cache;

static u64 GetGameModsMtime(u64 program_id) {
    if (program_id == 0) return 0;
    const auto load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) /
                          fmt::format("{:016X}", program_id);
    std::error_code ec;
    if (std::filesystem::exists(load_dir, ec)) {
        const auto lwt = std::filesystem::last_write_time(load_dir, ec);
        if (!ec) {
            return static_cast<u64>(lwt.time_since_epoch().count());
        }
    }
    return 0;
}

void GameListWorker::LoadMetadataCache() {
    metadata_cache.clear();
    emitted_entries.clear();
    metadata_cache_dirty = false;
    if (!UISettings::values.cache_game_list) {
        return;
    }
    const auto cache_path = Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list" / "file_metadata_cache.json");
    QFile file(QString::fromStdString(cache_path));
    if (!file.open(QFile::ReadOnly)) {
        return;
    }
    QJsonParseError parse_err{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parse_err);
    if (parse_err.error != QJsonParseError::NoError || !doc.isObject()) {
        LOG_WARNING(Frontend, "Game list cache corrupted ({}), falling back to full scan",
                    parse_err.errorString().toStdString());
        return;
    }
    const auto root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const auto obj = it.value().toObject();
        CachedGameMetadata meta;
        meta.file_size = static_cast<u64>(obj.value(QStringLiteral("size")).toVariant().toULongLong());
        meta.mtime = static_cast<u64>(obj.value(QStringLiteral("mtime")).toVariant().toULongLong());
        meta.program_id = obj.value(QStringLiteral("id")).toString().toULongLong(nullptr, 16);
        meta.name = obj.value(QStringLiteral("name")).toString().toStdString();
        meta.file_type_string = obj.value(QStringLiteral("type")).toString().toStdString();
        meta.patch_versions = obj.value(QStringLiteral("patches")).toString().toStdString();
        meta.file_version = obj.value(QStringLiteral("version")).toString().toStdString();
        meta.addons_text = obj.value(QStringLiteral("addons")).toString().toStdString();
        meta.is_bootable = obj.value(QStringLiteral("bootable")).toBool(true);
        metadata_cache.emplace(it.key().toStdString(), std::move(meta));
    }
}

void GameListWorker::SaveMetadataCache() {
    if (!UISettings::values.cache_game_list || !metadata_cache_dirty) {
        return;
    }
    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
    void(Common::FS::CreateParentDirs(cache_dir / "file_metadata_cache.json"));
    const auto cache_path = Common::FS::PathToUTF8String(cache_dir / "file_metadata_cache.json");
    QJsonObject root;
    for (const auto& [path, meta] : metadata_cache) {
        QJsonObject obj;
        obj.insert(QStringLiteral("size"), static_cast<qint64>(meta.file_size));
        obj.insert(QStringLiteral("mtime"), static_cast<qint64>(meta.mtime));
        obj.insert(QStringLiteral("id"), QStringLiteral("%1").arg(meta.program_id, 16, 16, QLatin1Char('0')).toUpper());
        obj.insert(QStringLiteral("name"), QString::fromStdString(meta.name));
        obj.insert(QStringLiteral("type"), QString::fromStdString(meta.file_type_string));
        obj.insert(QStringLiteral("patches"), QString::fromStdString(meta.patch_versions));
        obj.insert(QStringLiteral("version"), QString::fromStdString(meta.file_version));
        obj.insert(QStringLiteral("addons"), QString::fromStdString(meta.addons_text));
        obj.insert(QStringLiteral("bootable"), meta.is_bootable);
        root.insert(QString::fromStdString(path), obj);
    }
    // Atomic write: write to temp file first, flush, close, then atomically rename
    const auto tmp_path = Common::FS::PathToUTF8String(cache_dir / "file_metadata_cache.json.tmp");
    const QString q_tmp = QString::fromStdString(tmp_path);
    const QString q_dest = QString::fromStdString(cache_path);
    QFile file(q_tmp);
    if (file.open(QFile::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        file.flush();
        file.close();

        // Atomically replace target
        QFile::remove(q_dest);
        if (QFile::rename(q_tmp, q_dest)) {
            metadata_cache_dirty = false;
        }
    }
}

void GameListWorker::ProcessEvents(GameListModel* model) {
    while (true) {
        std::function<void(GameListModel*)> func;
        {
            // Lock queue to protect concurrent modification.
            std::scoped_lock lk(lock);

            // If we can't pop a function, return.
            if (queued_events.empty()) {
                return;
            }

            // Pop a function.
            func = std::move(queued_events.back());
            queued_events.pop_back();
        }

        // Run the function.
        func(model);
    }
}

template <typename F>
void GameListWorker::RecordEvent(F&& func) {
    {
        // Lock queue to protect concurrent modification.
        std::scoped_lock lk(lock);

        // Add the function into the front of the queue.
        queued_events.emplace_front(std::move(func));
    }

    // Data now available.
    emit DataAvailable();
}

void GameListWorker::AddTitlesToGameList(GameListDir* parent_dir) {
    using namespace FileSys;

    const auto& cache = system.GetContentProviderUnion();

    auto installed_games = cache.ListEntriesFilterOrigin(std::nullopt, TitleType::Application,
                                                         ContentRecordType::Program);

    if (parent_dir->type() == static_cast<int>(GameListItemType::SdmcDir)) {
        installed_games = cache.ListEntriesFilterOrigin(
            ContentProviderUnionSlot::SDMC, TitleType::Application, ContentRecordType::Program);
    } else if (parent_dir->type() == static_cast<int>(GameListItemType::UserNandDir)) {
        installed_games = cache.ListEntriesFilterOrigin(
            ContentProviderUnionSlot::UserNAND, TitleType::Application, ContentRecordType::Program);
    } else if (parent_dir->type() == static_cast<int>(GameListItemType::SysNandDir)) {
        installed_games = cache.ListEntriesFilterOrigin(
            ContentProviderUnionSlot::SysNAND, TitleType::Application, ContentRecordType::Program);
    }

    std::vector<QList<QStandardItem*>> installed_batch;
    installed_batch.reserve(installed_games.size());

    for (const auto& [slot, game] : installed_games) {
        if (slot == ContentProviderUnionSlot::FrontendManual || stop_requested) {
            continue;
        }

        const auto file = cache.GetEntryUnparsed(game.title_id, game.type);
        if (!file) {
            continue;
        }

        const std::string full_path = file->GetFullPath();
        const u64 file_sz = file->GetSize();
        std::error_code ec;
        const auto lwt = std::filesystem::last_write_time(Common::FS::ToU8String(full_path), ec);
        const u64 mtime_val = ec ? 0 : static_cast<u64>(lwt.time_since_epoch().count());

        // 1. In-memory cache hit
        bool handled_installed = false;
        {
            std::scoped_lock lk(s_mem_cache_mutex);
            const auto it = s_mem_cache.find(full_path);
            if (it != s_mem_cache.end()) {
                const auto& mem_entry = it->second;
                if (!mem_entry.is_bootable) {
                    continue; // Negative cache hit!
                }
                if (mem_entry.program_id != 0 && (mem_entry.program_id & 0xFFF) == 0) {
                    const std::string dedup_key = fmt::format("{:016X}_{}", mem_entry.program_id & 0xFFFFFFFFFFFFE000ULL, mem_entry.file_type_string);
                    if (!emitted_entries.contains(full_path) && !emitted_entries.contains(dedup_key)) {
                        installed_batch.push_back(MakeCachedGameListEntry(
                            full_path, mem_entry.name, file_sz, mem_entry.icon,
                            QString::fromStdString(mem_entry.file_type_string), mem_entry.program_id,
                            play_time_manager, QString::fromStdString(mem_entry.patch_versions),
                            QString::fromStdString(mem_entry.file_version),
                            QString::fromStdString(mem_entry.addons_text)));
                        emitted_entries.insert(full_path);
                        emitted_entries.insert(dedup_key);
                    }
                    handled_installed = true;
                }
            }
        }
        if (handled_installed) {
            continue;
        }

        // 2. Persistent disk cache hit (file_metadata_cache.json)
        const auto cache_it = metadata_cache.find(full_path);
        if (cache_it != metadata_cache.end() && cache_it->second.file_size == file_sz &&
            (mtime_val == 0 || cache_it->second.mtime == mtime_val)) {
            const auto& cached = cache_it->second;
            if (!cached.is_bootable) {
                // Negative cache hit from persistent cache!
                CachedGameEntry me;
                me.file_size = file_sz;
                me.mtime = mtime_val;
                me.is_bootable = false;
                {
                    std::scoped_lock lk(s_mem_cache_mutex);
                    s_mem_cache[full_path] = std::move(me);
                }
                continue; // SKIP in O(1)!
            }

            const std::string dedup_key = fmt::format("{:016X}_{}", cached.program_id & 0xFFFFFFFFFFFFE000ULL, cached.file_type_string);
            if (!emitted_entries.contains(full_path) && !emitted_entries.contains(dedup_key) &&
                cached.program_id != 0 && (cached.program_id & 0xFFF) == 0) {
                std::vector<u8> icon_bytes;
                installed_batch.push_back(MakeCachedGameListEntry(
                    full_path, cached.name, cached.file_size, icon_bytes,
                    QString::fromStdString(cached.file_type_string), cached.program_id,
                    play_time_manager, QString::fromStdString(cached.patch_versions),
                    QString::fromStdString(cached.file_version),
                    QString::fromStdString(cached.addons_text)));
                emitted_entries.insert(full_path);
                emitted_entries.insert(dedup_key);

                CachedGameEntry me;
                me.file_size = cached.file_size;
                me.mtime = cached.mtime;
                me.program_id = cached.program_id;
                me.name = cached.name;
                me.file_type_string = cached.file_type_string;
                me.patch_versions = cached.patch_versions;
                me.file_version = cached.file_version;
                me.addons_text = cached.addons_text;
                me.icon = icon_bytes;
                me.is_bootable = true;
                me.mods_mtime = GetGameModsMtime(cached.program_id);
                {
                    std::scoped_lock lk(s_mem_cache_mutex);
                    s_mem_cache[full_path] = std::move(me);
                }
            }
            continue;
        }

        // 3. Fallback: Parse loader and PatchManager for new/uncached NCAs
        std::unique_ptr<Loader::AppLoader> loader = Loader::GetLoader(system, file);
        if (!loader) {
            CachedGameMetadata meta;
            meta.file_size = file_sz;
            meta.mtime = mtime_val;
            meta.is_bootable = false;
            metadata_cache[full_path] = std::move(meta);
            metadata_cache_dirty = true;
            CachedGameEntry me;
            me.file_size = file_sz;
            me.mtime = mtime_val;
            me.is_bootable = false;
            {
                std::scoped_lock lk(s_mem_cache_mutex);
                s_mem_cache[full_path] = std::move(me);
            }
            continue;
        }

        std::vector<u8> icon;
        std::string name;
        u64 program_id = 0;
        const auto result = loader->ReadProgramId(program_id);

        if (result != Loader::ResultStatus::Success || program_id == 0 || (program_id & 0xFFF) != 0) {
            CachedGameMetadata meta;
            meta.file_size = file_sz;
            meta.mtime = mtime_val;
            meta.is_bootable = false;
            metadata_cache[full_path] = std::move(meta);
            metadata_cache_dirty = true;
            CachedGameEntry me;
            me.file_size = file_sz;
            me.mtime = mtime_val;
            me.is_bootable = false;
            {
                std::scoped_lock lk(s_mem_cache_mutex);
                s_mem_cache[full_path] = std::move(me);
            }
            continue;
        }

        const PatchManager patch{program_id, system.GetFileSystemController(),
                                 system.GetContentProvider()};
        LOG_INFO(Frontend, "PatchManager initiated for id {:X}", program_id);
        const auto control = cache.GetEntry(game.title_id, ContentRecordType::Control);
        if (control != nullptr) {
            GetMetadataFromControlNCA(patch, *control, icon, name);
        }

        QString ftype, pvers, fvers, atext;
        auto entry = MakeGameListEntry(full_path, name, file_sz, icon, *loader,
                                       program_id, play_time_manager, patch,
                                       &ftype, &pvers, &fvers, &atext);

        CachedGameMetadata meta;
        meta.file_size = file_sz;
        meta.mtime = mtime_val;
        meta.program_id = program_id;
        meta.name = name;
        meta.file_type_string = ftype.toStdString();
        meta.patch_versions = pvers.toStdString();
        meta.file_version = fvers.toStdString();
        meta.addons_text = atext.toStdString();
        meta.is_bootable = true;
        metadata_cache[full_path] = std::move(meta);
        metadata_cache_dirty = true;

        CachedGameEntry me;
        me.file_size = file_sz;
        me.mtime = mtime_val;
        me.program_id = program_id;
        me.name = name;
        me.file_type_string = ftype.toStdString();
        me.patch_versions = pvers.toStdString();
        me.file_version = fvers.toStdString();
        me.addons_text = atext.toStdString();
        me.icon = icon;
        me.is_bootable = true;
        me.mods_mtime = GetGameModsMtime(program_id);
        {
            std::scoped_lock lk(s_mem_cache_mutex);
            s_mem_cache[full_path] = std::move(me);
        }

        const std::string dedup_key = fmt::format("{:016X}_{}", program_id & 0xFFFFFFFFFFFFE000ULL, ftype.toStdString());
        if (!emitted_entries.contains(full_path) && !emitted_entries.contains(dedup_key)) {
            installed_batch.push_back(entry);
            emitted_entries.insert(full_path);
            emitted_entries.insert(dedup_key);
        }
    }

    if (!installed_batch.empty()) {
        RecordEvent([batch = std::move(installed_batch), parent_dir](GameListModel* model) mutable {
            for (auto& entry : batch) {
                model->AddEntry(entry, parent_dir);
            }
            emit model->ShowList(true);
        });
    }
}

static bool ShouldPruneDirectory(const std::filesystem::path& p) {
    const auto name = p.filename().native();
    if (name.empty() || name.size() > 32) return false;
    char ascii_lower[36];
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto c = name[i];
        if (c > 127) return false;
        ascii_lower[i] = static_cast<char>(std::tolower(static_cast<int>(c)));
    }
    const std::string_view s(ascii_lower, name.size());
    static const std::unordered_set<std::string_view> prune_dirs = {
        "work", "romfs", "cheats", "mods", "load", "atmosphere", "layeredfs",
        ".git", ".svn", ".hg", ".vs", ".vscode", ".idea", ".cxx", ".gradle",
        "node_modules", "__pycache__", "venv", ".venv", "build", "build_ninja",
        "out", "obj", "temp", "tmp", "cache", "shaders", "screenshots",
        "backup", "backups", "logs", "textures", "fonts", "extracted",
        "unpacked", "dump", "dumps", "nand", "sdmc"
    };
    return prune_dirs.contains(s);
}

static bool HasSupportedFileExtensionFast(const std::filesystem::path& p) {
    const auto ext = p.extension().native();
    if (ext.size() != 4) {
        return false;
    }
    char buf[4];
    for (int i = 0; i < 4; ++i) {
        auto c = ext[i];
        if (c > 127) return false;
        buf[i] = static_cast<char>(std::tolower(static_cast<int>(c)));
    }
    const std::string_view sv(buf, 4);
    return (sv == ".nsp" || sv == ".nsz" || sv == ".xci" || sv == ".xcz" ||
            sv == ".nca" || sv == ".ncz" || sv == ".nro" || sv == ".nso" ||
            sv == ".kip" || sv == ".nup");
}

static bool IsSupportedRomFileFast(const std::filesystem::path& p) {
    if (HasSupportedFileExtensionFast(p)) {
        return true;
    }
    const auto fn = p.filename().native();
    if (fn.size() == 9) {
        char buf[9];
        for (int i = 0; i < 9; ++i) {
            auto c = fn[i];
            if (c > 127) return false;
            buf[i] = static_cast<char>(std::tolower(static_cast<int>(c)));
        }
        if (std::string_view(buf, 9) == "main.npdm") {
            return true;
        }
    }
    if (fn.size() == 4) {
        char buf[4];
        for (int i = 0; i < 4; ++i) {
            auto c = fn[i];
            if (c > 127) return false;
            buf[i] = static_cast<char>(std::tolower(static_cast<int>(c)));
        }
        if (std::string_view(buf, 4) == "main") {
            // Avoid duplicate entry if main.npdm exists in the same directory
            std::error_code check_ec;
            if (std::filesystem::exists(p.parent_path() / "main.npdm", check_ec)) {
                return false;
            }
            return true;
        }
    }
    return false;
}

void GameListWorker::ScanDirectory(const std::string& dir_path, bool deep_scan,
                                   GameListDir* parent_dir) {
    std::string norm_root = dir_path;
    while (!norm_root.empty() && (norm_root.back() == '/' || norm_root.back() == '\\')) {
        norm_root.pop_back();
    }
    const std::string lower_root = Common::ToLower(norm_root);
    if (lower_root.ends_with("/cheats") || lower_root.ends_with("\\cheats") || lower_root.ends_with("cheats")) {
        return;
    }

    struct FoundFileInfo {
        std::string physical_name;
        u64 file_sz = 0;
        u64 mtime_val = 0;
    };

    std::vector<FoundFileInfo> found_files;
    found_files.reserve(128);

    std::filesystem::path root_fs_path(Common::FS::ToU8String(dir_path));
#ifdef _WIN32
    std::wstring w = root_fs_path.lexically_normal().native();
    for (auto& c : w) { if (c == L'/') c = L'\\'; }
    if (!w.starts_with(L"\\\\?\\") && !w.starts_with(L"\\\\.\\")) {
        if (w.starts_with(L"\\\\")) {
            root_fs_path = std::filesystem::path(L"\\\\?\\UNC\\" + w.substr(2));
        } else if (w.size() >= 2 && w[1] == L':') {
            root_fs_path = std::filesystem::path(L"\\\\?\\" + w);
        }
    }
    auto clean_utf8_path = [](const std::filesystem::path& p) -> std::string {
        std::string s = Common::FS::PathToUTF8String(p);
        if (s.starts_with("\\\\?\\UNC\\")) {
            s = "\\\\" + s.substr(8);
        } else if (s.starts_with("\\\\?\\") || s.starts_with("\\\\.\\")) {
            s.erase(0, 4);
        }
        return s;
    };
#else
    auto clean_utf8_path = [](const std::filesystem::path& p) -> std::string {
        return Common::FS::PathToUTF8String(p);
    };
#endif

    std::error_code ec;
    if (deep_scan) {
        std::filesystem::recursive_directory_iterator it(root_fs_path, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;

        while (!ec && it != end && !stop_requested) {
            std::error_code type_ec;
            if (it->is_directory(type_ec)) {
                if (!type_ec) {
                    if (ShouldPruneDirectory(it->path())) {
                        it.disable_recursion_pending();
                    } else if (watch_list.size() < 64) {
                        const auto dir_phys = clean_utf8_path(it->path());
                        watch_list.append(QString::fromStdString(dir_phys));
                    }
                }
            } else if (it->is_regular_file(type_ec)) {
                if (!type_ec && IsSupportedRomFileFast(it->path())) {
                    const auto physical_name = clean_utf8_path(it->path());
                    const auto l_phys = Common::ToLower(physical_name);
                    if (!l_phys.ends_with(".part") && !l_phys.ends_with(".tmp") &&
                        !l_phys.ends_with(".crdownload") && !l_phys.ends_with(".downloading") &&
                        !l_phys.ends_with(".incomplete") && !l_phys.ends_with(".!ut")) {
                        std::error_code last_ec;
                        const auto last_write = it->last_write_time(last_ec);
                        const u64 mtime_val = last_ec ? 0 : static_cast<u64>(last_write.time_since_epoch().count());
                        std::error_code sz_ec;
                        const u64 file_sz = it->file_size(sz_ec);
                        if (!sz_ec && file_sz > 0) {
                            found_files.push_back(FoundFileInfo{physical_name, file_sz, mtime_val});
                        }
                    }
                }
            }
            it.increment(ec);
            if (ec) {
                ec.clear();
                std::error_code pop_ec;
                if (it.depth() > 0) {
                    it.pop(pop_ec);
                    if (pop_ec) break;
                } else {
                    break;
                }
            }
        }
    } else {
        std::filesystem::directory_iterator it(root_fs_path, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::directory_iterator end;

        while (!ec && it != end && !stop_requested) {
            std::error_code type_ec;
            if (it->is_regular_file(type_ec)) {
                if (!type_ec && IsSupportedRomFileFast(it->path())) {
                    const auto physical_name = clean_utf8_path(it->path());
                    const auto l_phys = Common::ToLower(physical_name);
                    if (!l_phys.ends_with(".part") && !l_phys.ends_with(".tmp") &&
                        !l_phys.ends_with(".crdownload") && !l_phys.ends_with(".downloading") &&
                        !l_phys.ends_with(".incomplete") && !l_phys.ends_with(".!ut")) {
                        std::error_code last_ec;
                        const auto last_write = it->last_write_time(last_ec);
                        const u64 mtime_val = last_ec ? 0 : static_cast<u64>(last_write.time_since_epoch().count());
                        std::error_code sz_ec;
                        const u64 file_sz = it->file_size(sz_ec);
                        if (!sz_ec && file_sz > 0) {
                            found_files.push_back(FoundFileInfo{physical_name, file_sz, mtime_val});
                        }
                    }
                }
            }
            it.increment(ec);
            if (ec) {
                ec.clear();
                break;
            }
        }
    }

    if (stop_requested) {
        return;
    }

    // Step 1: Instant emission of all cached games (0 disk seeks, single batched UI update)
    std::vector<FoundFileInfo> uncached_files;
    uncached_files.reserve(found_files.size());
    std::vector<std::string> deferred_provider_files;
    deferred_provider_files.reserve(found_files.size());
    std::vector<QList<QStandardItem*>> cached_batch;
    cached_batch.reserve(found_files.size());

    const auto cache_dir_base = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";

    for (const auto& file_info : found_files) {
        if (stop_requested) {
            return;
        }

        // 1. Ultra-fast in-memory cache hit (< 0.001 ms, 0 disk I/O, no JSON parsing, no Base64/JPEG decoding)
        bool handled_by_mem = false;
        {
            std::scoped_lock lk(s_mem_cache_mutex);
            const auto it = s_mem_cache.find(file_info.physical_name);
            if (it != s_mem_cache.end()) {
                const auto& mem_entry = it->second;
                if (mem_entry.file_size == file_info.file_sz && mem_entry.mtime == file_info.mtime_val) {
                    const u64 cur_mods_mtime = GetGameModsMtime(mem_entry.program_id);
                    if (mem_entry.mods_mtime == cur_mods_mtime) {
                        handled_by_mem = true;
                        if (!mem_entry.is_bootable) {
                            if (Settings::values.ext_content_from_game_dirs.GetValue()) {
                                deferred_provider_files.push_back(file_info.physical_name);
                            }
                        } else {
                            const std::string dedup_key = fmt::format("{:016X}_{}", mem_entry.program_id & 0xFFFFFFFFFFFFE000ULL, mem_entry.file_type_string);
                            if (!emitted_entries.contains(file_info.physical_name) && !emitted_entries.contains(dedup_key) &&
                                mem_entry.program_id != 0 && (mem_entry.program_id & 0xFFF) == 0) {
                                cached_batch.push_back(MakeCachedGameListEntry(
                                    file_info.physical_name, mem_entry.name, mem_entry.file_size, mem_entry.icon,
                                    QString::fromStdString(mem_entry.file_type_string), mem_entry.program_id,
                                    play_time_manager, QString::fromStdString(mem_entry.patch_versions),
                                    QString::fromStdString(mem_entry.file_version),
                                    QString::fromStdString(mem_entry.addons_text)));
                                emitted_entries.insert(file_info.physical_name);
                                emitted_entries.insert(dedup_key);
                            }
                        }
                    }
                }
            }
        }
        if (handled_by_mem) {
            continue;
        }

        // 2. Persistent disk cache fallback
        const auto cache_it = metadata_cache.find(file_info.physical_name);
        if (cache_it != metadata_cache.end() && cache_it->second.file_size == file_info.file_sz &&
            cache_it->second.mtime == file_info.mtime_val) {
            const auto& cached = cache_it->second;
            if (!cached.is_bootable) {
                if (Settings::values.ext_content_from_game_dirs.GetValue()) {
                    deferred_provider_files.push_back(file_info.physical_name);
                }
                CachedGameEntry me;
                me.file_size = cached.file_size;
                me.mtime = cached.mtime;
                me.program_id = cached.program_id;
                me.is_bootable = false;
                std::scoped_lock lk(s_mem_cache_mutex);
                s_mem_cache[file_info.physical_name] = std::move(me);
                continue;
            }

            const std::string dedup_key = fmt::format("{:016X}_{}", cached.program_id & 0xFFFFFFFFFFFFE000ULL, cached.file_type_string);
            if (!emitted_entries.contains(file_info.physical_name) && !emitted_entries.contains(dedup_key) &&
                cached.program_id != 0 && (cached.program_id & 0xFFF) == 0) {
                // Do NOT synchronously open/read jpeg here - thumbnails are loaded lazily & async
                std::vector<u8> icon_bytes;
                cached_batch.push_back(MakeCachedGameListEntry(
                    file_info.physical_name, cached.name, cached.file_size, icon_bytes,
                    QString::fromStdString(cached.file_type_string), cached.program_id,
                    play_time_manager, QString::fromStdString(cached.patch_versions),
                    QString::fromStdString(cached.file_version),
                    QString::fromStdString(cached.addons_text)));
                emitted_entries.insert(file_info.physical_name);
                emitted_entries.insert(dedup_key);

                // Populate in-memory cache so subsequent scans run with 0 ms latency
                CachedGameEntry me;
                me.file_size = cached.file_size;
                me.mtime = cached.mtime;
                me.program_id = cached.program_id;
                me.name = cached.name;
                me.file_type_string = cached.file_type_string;
                me.patch_versions = cached.patch_versions;
                me.file_version = cached.file_version;
                me.addons_text = cached.addons_text;
                me.icon = icon_bytes;
                me.is_bootable = true;
                me.mods_mtime = GetGameModsMtime(cached.program_id);
                std::scoped_lock lk(s_mem_cache_mutex);
                s_mem_cache[file_info.physical_name] = std::move(me);
            }
        } else {
            uncached_files.push_back(file_info);
        }
    }

    if (!cached_batch.empty()) {
        RecordEvent([batch = std::move(cached_batch), parent_dir](GameListModel* model) mutable {
            for (auto& entry : batch) {
                model->AddEntry(entry, parent_dir);
            }
            emit model->ShowList(true);
        });
    }

    // Register external content containers (updates/DLCs) so ContentProvider has all updates & DLCs
    for (const auto& phys_path : deferred_provider_files) {
        if (stop_requested) {
            return;
        }
        const auto file = vfs->OpenFile(phys_path, FileSys::OpenMode::Read);
        if (file && file->GetSize() > 0) {
            provider->AddEntriesFromContainer(file, true);
        }
    }

    if (uncached_files.empty() || stop_requested) {
        return;
    }

    // Pre-register all uncached containers in ContentProvider BEFORE building game entries
    // so base games always see Updates and DLCs located in the same directory regardless of file order
    if (Settings::values.ext_content_from_game_dirs.GetValue()) {
        for (const auto& file_info : uncached_files) {
            if (stop_requested) {
                return;
            }
            const auto ext = Common::ToLower(std::string(Common::FS::GetExtensionFromFilename(file_info.physical_name)));
            if (ext == "nsp" || ext == "nsz" || ext == "xci" || ext == "xcz" || ext == "nup") {
                const auto file = vfs->OpenFile(file_info.physical_name, FileSys::OpenMode::Read);
                if (file && file->GetSize() > 0) {
                    provider->AddEntriesFromContainer(file, true);
                }
            }
        }
    }

    // Step 2: Parallelized bounded processing of uncached games
    const std::size_t hardware_threads = std::max(1u, std::thread::hardware_concurrency());
    const std::size_t num_threads = std::min(std::size_t{8}, std::max(std::size_t{2}, hardware_threads));
    const std::size_t actual_threads = std::min(num_threads, uncached_files.size());

    std::mutex cache_mutex;
    std::vector<QList<QStandardItem*>> uncached_batch;
    std::atomic<std::size_t> next_file_idx{0};

    auto process_file = [&](const FoundFileInfo& file_info) {
        if (stop_requested) {
            return;
        }

        try {
            const auto file = vfs->OpenFile(file_info.physical_name, FileSys::OpenMode::Read);
            if (!file || file->GetSize() == 0) {
                return;
            }

            auto loader = Loader::GetLoader(system, file);
            if (!loader) {
                return;
            }

            const auto file_type = loader->GetFileType();
            if (file_type == Loader::FileType::Unknown || file_type == Loader::FileType::Error) {
                return;
            }

            u64 program_id = 0;
            loader->ReadProgramId(program_id);

            std::vector<u64> program_ids;
            loader->ReadProgramIds(program_ids);

            // Check if this container contains any base application (TitleId ending in 000)
            bool has_base_game = (program_id != 0 && (program_id & 0xFFF) == 0);
            if (!has_base_game) {
                for (const auto id : program_ids) {
                    if ((id & 0xFFF) == 0) {
                        has_base_game = true;
                        if (program_id == 0) {
                            program_id = id;
                        }
                        break;
                    }
                }
            }

            // For NSP/NSZ containers, verify that the container actually holds a bootable base game NCA
            // and is not a standalone Update (0x800) or DLC (0x001+) package
            if (file_type == Loader::FileType::NSP || file_type == Loader::FileType::NSZ) {
                if (!Loader::IsBootableGameContainer(file, file_type)) {
                    has_base_game = false;
                }
            }

            // Fallback: extract base Title ID from leaf filename ONLY for cartridge dumps (XCI/XCZ)
            if (!has_base_game && (file_type == Loader::FileType::XCI || file_type == Loader::FileType::XCZ)) {
                const std::string leaf_name = std::filesystem::path(file_info.physical_name).filename().string();
                static const std::regex base_id_pattern(R"([0-9a-fA-F]{13}000)");
                std::smatch match;
                if (std::regex_search(leaf_name, match, base_id_pattern)) {
                    try {
                        const u64 parsed_id = std::stoull(match.str(), nullptr, 16);
                        if (parsed_id != 0) {
                            program_id = parsed_id;
                            has_base_game = true;
                            auto specialized_loader = Loader::GetLoader(system, file, program_id);
                            if (specialized_loader) {
                                loader = std::move(specialized_loader);
                            }
                        }
                    } catch (...) {}
                }
            }

            // Register in provider if NCA or container with external content enabled
            if (file_type == Loader::FileType::NCA) {
                if (program_id != 0) {
                    std::scoped_lock cache_lock(cache_mutex);
                    provider->AddEntry(FileSys::TitleType::Application,
                                       FileSys::GetCRTypeFromNCAType(FileSys::NCA{file}.GetType()),
                                       program_id, file);
                }
            } else if (Settings::values.ext_content_from_game_dirs.GetValue() &&
                       (file_type == Loader::FileType::XCI || file_type == Loader::FileType::XCZ ||
                        file_type == Loader::FileType::NSP || file_type == Loader::FileType::NSZ)) {
                std::scoped_lock cache_lock(cache_mutex);
                provider->AddEntriesFromContainer(file, true);
            }

            if (!has_base_game) {
                std::scoped_lock cache_lock(cache_mutex);
                CachedGameMetadata meta;
                meta.file_size = file_info.file_sz;
                meta.mtime = file_info.mtime_val;
                meta.is_bootable = false;
                metadata_cache[file_info.physical_name] = std::move(meta);
                metadata_cache_dirty = true;
                CachedGameEntry me;
                me.file_size = file_info.file_sz;
                me.mtime = file_info.mtime_val;
                me.is_bootable = false;
                {
                    std::scoped_lock lk(s_mem_cache_mutex);
                    s_mem_cache[file_info.physical_name] = std::move(me);
                }
                return;
            }

            const auto addEntry = [this, &file_info, parent_dir, &cache_mutex, &uncached_batch, &cache_dir_base](
                                      std::unique_ptr<Loader::AppLoader>& app_loader,
                                      const u64 id) {
                if (id == 0 || (id & 0xFFF) != 0) {
                    return;
                }

                const FileSys::PatchManager patch{id, system.GetFileSystemController(),
                                                  system.GetContentProvider()};

                std::vector<u8> icon;
                [[maybe_unused]] const auto res1 = app_loader->ReadIcon(icon);
                std::string name = " ";
                [[maybe_unused]] const auto res3 = app_loader->ReadTitle(name);

                if (icon.empty() || name.empty() || name == " ") {
                    const auto control = patch.GetControlMetadata();
                    if (icon.empty() && control.second != nullptr) {
                        icon = control.second->ReadAllBytes();
                    }
                    if ((name.empty() || name == " ") && control.first != nullptr) {
                        name = control.first->GetApplicationName();
                    }
                }
                if (name.empty() || name == " ") {
                    const auto opt_entry = TitleDB::TitleDatabase::Instance().Lookup(id);
                    if (opt_entry && !opt_entry->name.empty()) {
                        name = opt_entry->name;
                    } else {
                        const std::string filename_str =
                            std::filesystem::path(file_info.physical_name).stem().string();
                        if (!filename_str.empty()) {
                            name = filename_str;
                        }
                    }
                }

                if (icon.empty()) {
                    const auto icon_file_path = Common::FS::PathToUTF8String(
                        cache_dir_base / fmt::format("{:016X}.jpeg", id));
                    QFile ifile(QString::fromStdString(icon_file_path));
                    if (ifile.open(QFile::ReadOnly)) {
                        const auto qdata = ifile.readAll();
                        icon.assign(qdata.begin(), qdata.end());
                    }
                }

                if (!icon.empty()) {
                    const auto icon_file_path = Common::FS::PathToUTF8String(
                        cache_dir_base / fmt::format("{:016X}.jpeg", id));
                    void(Common::FS::CreateParentDirs(icon_file_path));
                    QFile ifile(QString::fromStdString(icon_file_path));
                    if (ifile.open(QFile::WriteOnly)) {
                        ifile.write(reinterpret_cast<const char*>(icon.data()), icon.size());
                    }

                    const auto thumb_file_path = cache_dir_base / "icons" / fmt::format("{:016X}.png", id);
                    if (!Common::FS::Exists(thumb_file_path)) {
                        void(Common::FS::CreateParentDirs(thumb_file_path));
                        QImage img;
                        if (img.loadFromData(icon.data(), static_cast<u32>(icon.size()))) {
                            img = img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                            img.save(QString::fromStdString(Common::FS::PathToUTF8String(thumb_file_path)), "PNG", 90);
                        }
                    }
                }

                QString file_type_str, pv_str, fv_str, addons_str;
                auto entry = MakeGameListEntry(
                    file_info.physical_name, name, file_info.file_sz, icon, *app_loader,
                    id, play_time_manager, patch,
                    &file_type_str, &pv_str, &fv_str, &addons_str);

                std::scoped_lock cache_lock(cache_mutex);
                CachedGameMetadata meta;
                meta.file_size = file_info.file_sz;
                meta.mtime = file_info.mtime_val;
                meta.program_id = id;
                meta.name = name;
                meta.file_type_string = file_type_str.toStdString();
                meta.patch_versions = pv_str.toStdString();
                meta.file_version = fv_str.toStdString();
                meta.addons_text = addons_str.toStdString();
                meta.is_bootable = true;
                metadata_cache[file_info.physical_name] = std::move(meta);
                metadata_cache_dirty = true;
                CachedGameEntry me;
                me.file_size = file_info.file_sz;
                me.mtime = file_info.mtime_val;
                me.program_id = id;
                me.name = name;
                me.file_type_string = file_type_str.toStdString();
                me.patch_versions = pv_str.toStdString();
                me.file_version = fv_str.toStdString();
                me.addons_text = addons_str.toStdString();
                me.icon = icon;
                me.is_bootable = true;
                me.mods_mtime = GetGameModsMtime(id);
                {
                    std::scoped_lock lk(s_mem_cache_mutex);
                    s_mem_cache[file_info.physical_name] = std::move(me);
                }
                const std::string dedup_key = fmt::format("{:016X}_{}", id & 0xFFFFFFFFFFFFE000ULL, file_type_str.toStdString());
                const bool already_emitted = emitted_entries.contains(file_info.physical_name) || emitted_entries.contains(dedup_key);
                emitted_entries.insert(file_info.physical_name);
                emitted_entries.insert(dedup_key);

                if (!already_emitted) {
                    uncached_batch.push_back(entry);
                    if (uncached_batch.size() >= 32) {
                        RecordEvent([batch = std::move(uncached_batch), parent_dir](GameListModel* model) mutable {
                            for (auto& e : batch) {
                                model->AddEntry(e, parent_dir);
                            }
                            emit model->ShowList(true);
                        });
                        uncached_batch.clear();
                    }
                }
            };

            if (program_ids.size() > 1 &&
                (file_type == Loader::FileType::XCI || file_type == Loader::FileType::XCZ ||
                 file_type == Loader::FileType::NSP || file_type == Loader::FileType::NSZ)) {
                for (const auto id : program_ids) {
                    if (stop_requested || (id & 0xFFF) != 0) {
                        continue;
                    }
                    auto sub_loader = Loader::GetLoader(system, file, id);
                    if (!sub_loader) {
                        continue;
                    }
                    addEntry(sub_loader, id);
                }
            } else {
                if (!stop_requested && program_id != 0 && (program_id & 0xFFF) == 0) {
                    addEntry(loader, program_id);
                }
            }

        } catch (const std::exception& e) {
            LOG_WARNING(Frontend, "Exception while scanning file {}: {}", file_info.physical_name,
                        e.what());
        } catch (...) {
            LOG_WARNING(Frontend, "Unknown exception while scanning file {}",
                        file_info.physical_name);
        }
    };

    if (actual_threads <= 1) {
        for (const auto& file_info : uncached_files) {
            if (stop_requested) {
                break;
            }
            process_file(file_info);
        }
    } else {
        std::vector<std::thread> workers;
        workers.reserve(actual_threads);
        for (std::size_t t = 0; t < actual_threads; ++t) {
            workers.emplace_back([&]() {
                while (!stop_requested) {
                    const std::size_t idx = next_file_idx.fetch_add(1);
                    if (idx >= uncached_files.size()) {
                        break;
                    }
                    process_file(uncached_files[idx]);
                }
            });
        }
        for (auto& worker_thread : workers) {
            if (worker_thread.joinable()) {
                worker_thread.join();
            }
        }
    }

    {
        std::scoped_lock cache_lock(cache_mutex);
        if (!uncached_batch.empty()) {
            RecordEvent([batch = std::move(uncached_batch), parent_dir](GameListModel* model) mutable {
                for (auto& e : batch) {
                    model->AddEntry(e, parent_dir);
                }
                emit model->ShowList(true);
            });
            uncached_batch.clear();
        }
    }
}

void GameListWorker::run() {
    const auto start_time = std::chrono::steady_clock::now();
    try {
        watch_list.clear();
        provider->ClearAllEntries();
        LoadMetadataCache();

        const auto DirEntryReady = [&](GameListDir* game_list_dir) {
            RecordEvent([=](GameListModel* model) { model->AddDirEntry(game_list_dir); });
        };

        for (UISettings::GameDir& game_dir : game_dirs) {
            if (stop_requested) {
                break;
            }

            GameListDir* game_list_dir;
            bool scan = false;

            if (game_dir.path.empty()) {
                continue;
            }

            if (game_dir.path == std::string("SDMC")) {
                game_list_dir = new GameListDir(game_dir, GameListItemType::SdmcDir);
            } else if (game_dir.path == std::string("UserNAND")) {
                game_list_dir = new GameListDir(game_dir, GameListItemType::UserNandDir);
            } else if (game_dir.path == std::string("SysNAND")) {
                game_list_dir = new GameListDir(game_dir, GameListItemType::SysNandDir);
            } else {
                const QString qpath = QString::fromStdString(game_dir.path);
                if (!QDir(qpath).exists()) {
                    continue;
                }
                if (watch_list.size() < 64) {
                    watch_list.append(qpath);
                }

                game_list_dir = new GameListDir(game_dir);
                scan = true;
            }

            DirEntryReady(game_list_dir);
            if (scan) {
                ScanDirectory(game_dir.path, game_dir.deep_scan, game_list_dir);
            } else {
                AddTitlesToGameList(game_list_dir);
            }
        }

        if (metadata_cache_dirty) {
            SaveMetadataCache();
        }

        const auto end_time = std::chrono::steady_clock::now();
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        LOG_INFO(Frontend, "GameList populated {} entries across {} dirs in {} ms", emitted_entries.size(), game_dirs.size(), elapsed_ms);

    } catch (const std::exception& e) {
        LOG_ERROR(Frontend, "GameListWorker exception: {}", e.what());
    } catch (...) {
        LOG_ERROR(Frontend, "GameListWorker unknown exception");
    }

    RecordEvent([this](GameListModel* model) { model->DonePopulating(watch_list); });
    processing_completed.Set();
}
