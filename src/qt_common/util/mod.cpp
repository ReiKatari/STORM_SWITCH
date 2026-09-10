// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <JlCompress.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/string_util.h"
#include "frontend_common/mod_manager.h"
#include "mod.h"
#include "qt_common/abstract/frontend.h"

namespace QtCommon::Mod {
QStringList GetModFolders(const QString& root, const QString& fallbackName) {
    namespace fs = std::filesystem;

    const auto std_root = root.toStdString();

    auto paths = FrontendCommon::GetModFolder(std_root);

    // multi mod zip
    if (paths.size() > 1) {
        // We just have to assume it's properly formed here.
        // If not, you're out of luck.
        QStringList qpaths;
        for (const fs::path& path : paths) {
            qpaths << QString::fromStdString(path.string());
        }

        return qpaths;
    }
    // either frontend didn't detect any romfs/exefs, or is a single-mod zip
    else {
        fs::path std_path;
        if (!paths.empty())
            std_path = paths[0];

        QString default_name;

        // If this is an atmosphere-packed mod, the default name will end up as the game's title ID.
        // So in this case ignore it and use the zip name instead
        if (!paths.empty() && std_path.string().find("atmosphere") == std::string::npos)
            default_name = QString::fromStdString(std_path.filename().string());
        else if (!fallbackName.isEmpty())
            default_name = fallbackName;
        else
            default_name = root.split(QLatin1Char('/')).last();

        QString name = QtCommon::Frontend::GetTextInput(
            tr("Mod Name"), tr("What should this mod be called?"), default_name);

        if (name.isEmpty())
            return {};

        // if std_path is empty, frontend_common could not determine mod type and/or name.
        // so we have to prompt the user and set up the structure ourselves
        if (paths.empty()) {
            // TODO: Carboxyl impl.
            const QStringList choices = {
                tr("RomFS"),
                tr("ExeFS/Patch"),
                tr("Cheat"),
            };

            int choice = QtCommon::Frontend::Choice(
                tr("Mod Type"),
                tr("Could not detect mod type automatically. Please manually "
                   "specify the type of mod you downloaded.\n\nMost mods are RomFS mods, but "
                   "patches "
                   "(.pchtxt) are typically ExeFS mods."),
                choices);

            std::string to_make;

            switch (choice) {
            case 0:
                to_make = "romfs";
                break;
            case 1:
                to_make = "exefs";
                break;
            case 2:
                to_make = "cheats";
                break;
            default:
                return {};
            }

            // now make a temp directory...
            const auto mod_dir = fs::temp_directory_path() / "storm_eden" / "mod" / name.toStdString();
            const auto tmp = mod_dir / to_make;
            std::error_code ec;
            fs::remove_all(mod_dir, ec);
            if (!fs::create_directories(tmp, ec)) {
                LOG_ERROR(Frontend, "Failed to create temporary directory {}", tmp.string());
                return {};
            }

            std_path = mod_dir;

            // ... and copy everything from the root to the temp dir
            for (const auto& entry : fs::directory_iterator(root.toStdString())) {
                const auto target = tmp / entry.path().filename();

                fs::copy(entry.path(), target,
                         fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
        } else {
            // Rename the existing mod folder.
            const auto new_path = std_path.parent_path() / name.toStdString();
            if (new_path != std_path) {
                fs::remove_all(new_path);
                fs::rename(std_path, new_path);
            }
            std_path = new_path;
        }

        return {QString::fromStdString(std_path.string())};
    }
}

const QString ExtractMod(const QString& path) {
    namespace fs = std::filesystem;
    fs::path tmp{fs::temp_directory_path() / "storm_switch" / "unzip_mod"};
    std::error_code ec;
    fs::remove_all(tmp, ec);
    if (!fs::create_directories(tmp, ec)) {
        QtCommon::Frontend::Critical(tr("Mod Extract Failed"),
                                     tr("Failed to create temporary directory %1")
                                         .arg(QString::fromStdString(tmp.string())));
        return QString();
    }

    const QString qCacheDir = QString::fromStdString(tmp.string());

    // 1. Try 7-Zip (supports .7z, .rar, .zip, .tar, .xz, .gz, etc.)
    QString seven_zip_path;
    const QString app_dir = QCoreApplication::applicationDirPath();
    if (QFile::exists(app_dir + QStringLiteral("/7z.exe"))) {
        seven_zip_path = app_dir + QStringLiteral("/7z.exe");
    } else if (QFile::exists(app_dir + QStringLiteral("/7za.exe"))) {
        seven_zip_path = app_dir + QStringLiteral("/7za.exe");
    } else if (QFile::exists(QStringLiteral("C:/Program Files/7-Zip/7z.exe"))) {
        seven_zip_path = QStringLiteral("C:/Program Files/7-Zip/7z.exe");
    } else if (QFile::exists(QStringLiteral("C:/Program Files (x86)/7-Zip/7z.exe"))) {
        seven_zip_path = QStringLiteral("C:/Program Files (x86)/7-Zip/7z.exe");
    } else {
        const QString found_7z = QStandardPaths::findExecutable(QStringLiteral("7z"));
        if (!found_7z.isEmpty()) {
            seven_zip_path = found_7z;
        } else {
            const QString found_7za = QStandardPaths::findExecutable(QStringLiteral("7za"));
            if (!found_7za.isEmpty()) {
                seven_zip_path = found_7za;
            }
        }
    }

    if (!seven_zip_path.isEmpty()) {
        QProcess proc;
        proc.start(seven_zip_path, {QStringLiteral("x"), path, QStringLiteral("-o%1").arg(qCacheDir), QStringLiteral("-y"), QStringLiteral("-aoa")});
        if (proc.waitForFinished(120000) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
            if (!fs::is_empty(tmp, ec)) {
                return qCacheDir;
            }
        }
    }

    // 2. Try tar.exe (Windows 10/11 built-in bsdtar with libarchive)
    QString tar_path;
    if (QFile::exists(QStringLiteral("C:/Windows/System32/tar.exe"))) {
        tar_path = QStringLiteral("C:/Windows/System32/tar.exe");
    } else {
        const QString found_tar = QStandardPaths::findExecutable(QStringLiteral("tar"));
        if (!found_tar.isEmpty()) {
            tar_path = found_tar;
        }
    }

    if (!tar_path.isEmpty()) {
        QProcess proc;
        proc.start(tar_path, {QStringLiteral("-xf"), path, QStringLiteral("-C"), qCacheDir});
        if (proc.waitForFinished(120000) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
            if (!fs::is_empty(tmp, ec)) {
                return qCacheDir;
            }
        }
    }

    // 3. Fallback: QuaZip for standard .zip archives
    QFile zip{path};
    QStringList result = JlCompress::extractDir(&zip, qCacheDir);
    if (!result.isEmpty() && !fs::is_empty(tmp, ec)) {
        return qCacheDir;
    }

    QtCommon::Frontend::Critical(tr("Mod Extract Failed"),
                                 tr("Failed to extract mod archive %1. Unsupported format or corrupted archive.").arg(path));
    return QString();
}

static std::filesystem::path UnwrapSingleFolder(const std::filesystem::path& source) {
    namespace fs = std::filesystem;
    fs::path current = source;
    std::error_code ec;

    while (true) {
        std::vector<fs::path> dirs;
        std::vector<fs::path> files;

        for (const auto& entry : fs::directory_iterator(current, ec)) {
            if (entry.is_directory(ec)) {
                dirs.push_back(entry.path());
            } else if (entry.is_regular_file(ec)) {
                files.push_back(entry.path());
            }
        }

        if (dirs.size() == 1 && files.empty()) {
            const std::string name = Common::ToLower(dirs[0].filename().string());
            if (name == "romfs" || name == "romfslite" || name == "romfs_ext" ||
                name == "exefs" || name == "cheats") {
                break;
            }
            current = dirs[0];
        } else {
            break;
        }
    }

    return current;
}

static std::filesystem::path FindContentsEffectiveRoot(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
        if (entry.is_directory(ec)) {
            const std::string name = Common::ToLower(entry.path().filename().string());
            if (name == "contents") {
                for (const auto& title_entry : fs::directory_iterator(entry.path(), ec)) {
                    if (title_entry.is_directory(ec)) {
                        return title_entry.path();
                    }
                }
            }
        }
    }
    return root;
}

bool OrganizeModStructure(const std::filesystem::path& source_dir, const std::filesystem::path& target_dir) {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(source_dir, ec) || !fs::is_directory(source_dir, ec)) {
        return false;
    }

    // Clean existing target mod directory if any
    fs::remove_all(target_dir, ec);
    fs::create_directories(target_dir, ec);

    // Step 1: Unwrap single wrapper folders
    fs::path effective_root = UnwrapSingleFolder(source_dir);

    // Step 2: Check for atmosphere/contents/<TitleID> or contents/<TitleID>
    effective_root = FindContentsEffectiveRoot(effective_root);

    bool organized = false;
    fs::path found_romfs;
    fs::path found_exefs;
    fs::path found_cheats;

    for (const auto& entry : fs::recursive_directory_iterator(effective_root, ec)) {
        if (entry.is_directory(ec)) {
            const std::string name = Common::ToLower(entry.path().filename().string());
            if (name == "romfs" || name == "romfslite" || name == "romfs_ext") {
                if (found_romfs.empty()) {
                    found_romfs = entry.path();
                }
            } else if (name == "exefs") {
                if (found_exefs.empty()) {
                    found_exefs = entry.path();
                }
            } else if (name == "cheats") {
                if (found_cheats.empty()) {
                    found_cheats = entry.path();
                }
            }
        }
    }

    if (!found_romfs.empty()) {
        const auto dst_romfs = target_dir / "romfs";
        fs::create_directories(dst_romfs, ec);
        fs::copy(found_romfs, dst_romfs,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        organized = true;
    }

    if (!found_exefs.empty()) {
        const auto dst_exefs = target_dir / "exefs";
        fs::create_directories(dst_exefs, ec);
        fs::copy(found_exefs, dst_exefs,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        organized = true;
    }

    if (!found_cheats.empty()) {
        const auto dst_cheats = target_dir / "cheats";
        fs::create_directories(dst_cheats, ec);
        fs::copy(found_cheats, dst_cheats,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        organized = true;
    }

    // Step 3: Check for .ips and .pchtxt patches anywhere in tree
    for (const auto& entry : fs::recursive_directory_iterator(effective_root, ec)) {
        if (entry.is_regular_file(ec)) {
            const std::string ext = Common::ToLower(entry.path().extension().string());
            if (ext == ".ips" || ext == ".pchtxt") {
                const auto dst_exefs = target_dir / "exefs";
                fs::create_directories(dst_exefs, ec);
                fs::copy_file(entry.path(), dst_exefs / entry.path().filename(),
                              fs::copy_options::overwrite_existing, ec);
                organized = true;
            }
        }
    }

    // Step 4: Check for cheat txt files (16 hex character filename like 0100726014352000.txt)
    for (const auto& entry : fs::recursive_directory_iterator(effective_root, ec)) {
        if (entry.is_regular_file(ec)) {
            const std::string ext = Common::ToLower(entry.path().extension().string());
            if (ext == ".txt") {
                const std::string stem = entry.path().stem().string();
                const bool is_16_hex = (stem.length() == 16) && std::all_of(stem.begin(), stem.end(), [](char c) {
                    return std::isxdigit(static_cast<unsigned char>(c));
                });
                if (is_16_hex) {
                    const auto dst_cheats = target_dir / "cheats";
                    fs::create_directories(dst_cheats, ec);
                    fs::copy_file(entry.path(), dst_cheats / entry.path().filename(),
                                  fs::copy_options::overwrite_existing, ec);
                    organized = true;
                }
            }
        }
    }

    // Step 5: If nothing standard was detected, bare files/folders belong in romfs!
    if (!organized) {
        const auto dst_romfs = target_dir / "romfs";
        fs::create_directories(dst_romfs, ec);
        for (const auto& entry : fs::directory_iterator(effective_root, ec)) {
            const std::string fname = Common::ToLower(entry.path().filename().string());
            const bool is_meta = fname.starts_with("readme") || fname.starts_with("license") ||
                                 fname.ends_with(".md") || fname.ends_with(".url") || fname.ends_with(".lnk");
            if (!is_meta) {
                if (entry.is_directory(ec)) {
                    fs::copy(entry.path(), dst_romfs / entry.path().filename(),
                             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
                    organized = true;
                } else if (entry.is_regular_file(ec)) {
                    fs::copy_file(entry.path(), dst_romfs / entry.path().filename(),
                                  fs::copy_options::overwrite_existing, ec);
                    organized = true;
                }
            }
        }
    }

    // Verify target directory has at least one of romfs, exefs, cheats with content
    bool has_content = false;
    for (const char* sub : {"romfs", "exefs", "cheats"}) {
        const auto p = target_dir / sub;
        if (fs::exists(p, ec) && fs::is_directory(p, ec) && !fs::is_empty(p, ec)) {
            has_content = true;
            break;
        }
    }

    return organized && has_content;
}

} // namespace QtCommon::Mod
