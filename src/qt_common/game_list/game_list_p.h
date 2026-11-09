// SPDX-FileCopyrightText: Copyright 2026 STORM EDEN Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>
#include <QStandardItem>
#include <QString>

#include "common/common_types.h"
#include "storm_switch/common/storm_design_engine.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/string_util.h"
#include "frontend_common/play_time_manager.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/qt_common.h"

enum class GameListItemType {
    Game = QStandardItem::UserType + 1,
    CustomDir = QStandardItem::UserType + 2,
    SdmcDir = QStandardItem::UserType + 3,
    UserNandDir = QStandardItem::UserType + 4,
    SysNandDir = QStandardItem::UserType + 5,
    AddDir = QStandardItem::UserType + 6,
    Favorites = QStandardItem::UserType + 7,
};

Q_DECLARE_METATYPE(GameListItemType);

[[maybe_unused]] static inline QImage GetDefaultImage(u32 size) {
    if (size == 0) size = 32;
    static std::unordered_map<u32, QImage> s_default_image_cache;
    static std::mutex s_default_image_mutex;
    std::lock_guard<std::mutex> lock(s_default_image_mutex);
    auto it = s_default_image_cache.find(size);
    if (it != s_default_image_cache.end()) {
        return it->second;
    }
    QImage icon(size, size, QImage::Format_ARGB32_Premultiplied);
    icon.fill(Qt::transparent);
    QPainter p(&icon);
    p.setRenderHint(QPainter::Antialiasing);

    // Cartridge / Card Base
    QLinearGradient bg(0, 0, size, size);
    bg.setColorAt(0.0, QColor(24, 28, 44));
    bg.setColorAt(1.0, QColor(10, 12, 20));

    p.setPen(QPen(QColor(0, 242, 254), 1.5));
    p.setBrush(bg);
    p.drawRoundedRect(2, 2, size - 4, size - 4, 4, 4);

    // Joy-Con Red & Blue top accents
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 60, 90)); // Neon Red
    p.drawRoundedRect(4, 4, (size - 8) / 2, size / 3, 2, 2);

    p.setBrush(QColor(0, 220, 255)); // Neon Blue
    p.drawRoundedRect(4 + (size - 8) / 2 + 1, 4, (size - 8) / 2 - 1, size / 3, 2, 2);

    // Subtle controller circles
    p.setPen(QPen(QColor(255, 255, 255, 220), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(size / 4, size / 2, size / 5, size / 5);
    p.drawEllipse(size * 7 / 12, size * 7 / 12, size / 5, size / 5);
    p.end();

    s_default_image_cache.emplace(size, icon);
    return icon;
}

[[maybe_unused]] static inline QPixmap GetDefaultIcon(u32 size) {
    return QPixmap::fromImage(GetDefaultImage(size));
}

[[maybe_unused]] static inline QPixmap ThemeIcon(const char* name) {
    const int size = std::max(20, static_cast<int>(UISettings::values.folder_icon_size.GetValue()));
    const std::string key = std::string(name ? name : "") + ":" + std::to_string(size);
    static std::unordered_map<std::string, QPixmap> s_theme_icon_cache;
    static std::mutex s_theme_icon_mutex;
    std::lock_guard<std::mutex> lock(s_theme_icon_mutex);
    auto it = s_theme_icon_cache.find(key);
    if (it != s_theme_icon_cache.end()) {
        return it->second;
    }

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    if (std::string(name) == "folder" || std::string(name) == "bad_folder") {
        // Vibrant solid golden-yellow 3D folder with cyan tab & crisp outline
        QLinearGradient grad(0, 0, 0, size);
        grad.setColorAt(0.0, QColor(255, 238, 0));
        grad.setColorAt(1.0, QColor(255, 175, 0));

        // Back tab (Cyan)
        p.setPen(QPen(QColor(0, 240, 255), 1.5));
        p.setBrush(QColor(0, 240, 255));
        p.drawRoundedRect(1, 1, size / 2, 6, 2, 2);

        // Front folder body (Solid Yellow/Orange Gradient)
        p.setPen(QPen(QColor(0, 0, 0), 1.5));
        p.setBrush(grad);
        p.drawRoundedRect(1, 4, size - 2, size - 5, 3, 3);

        // Top glossy highlight
        p.setPen(QPen(QColor(255, 255, 255, 220), 1));
        p.drawLine(3, 6, size - 4, 6);
    } else if (std::string(name) == "star") {
        p.setPen(QPen(QColor(0, 0, 0), 1));
        p.setBrush(QColor(255, 215, 0));
        p.drawEllipse(2, 2, size - 4, size - 4);
    } else if (std::string(name) == "list-add") {
        p.setPen(QPen(QColor(0, 240, 255), 1.5));
        p.setBrush(QColor(18, 22, 36));
        p.drawRoundedRect(2, 2, size - 4, size - 4, 3, 3);
        p.setPen(QPen(QColor(0, 240, 255), 2));
        p.drawLine(size / 2, 5, size / 2, size - 5);
        p.drawLine(5, size / 2, size - 5, size / 2);
    } else {
        QIcon icon = QIcon::fromTheme(QLatin1String(name));
        if (!icon.isNull()) {
            p.end();
            QPixmap res = icon.pixmap(size, size);
            s_theme_icon_cache.emplace(key, res);
            return res;
        }
        p.setPen(QPen(QColor(0, 240, 255), 1.5));
        p.setBrush(QColor(0, 240, 255));
        p.drawRoundedRect(2, 2, size - 4, size - 4, 3, 3);
    }
    p.end();
    s_theme_icon_cache.emplace(key, pix);
    return pix;
}

class GameListItem : public QStandardItem {

public:
    // used to access type from item index
    static constexpr int TypeRole = Qt::UserRole + 1;
    static constexpr int SortRole = Qt::UserRole + 2;
    GameListItem() = default;
    explicit GameListItem(const QString& string) : QStandardItem(string) {
        setData(string, SortRole);
    }
};

/**
 * A specialization of GameListItem for path values.
 * This class ensures that for every full path value it holds, a correct string representation
 * of just the filename (with no extension) will be displayed to the user.
 * If this class receives valid title metadata, it will also display game icons and titles.
 */
class GameListItemPath : public GameListItem {
    mutable QImage m_image;
    mutable QPixmap m_pixmap;
    mutable int m_cached_icon_size{0};

public:
    static constexpr int TitleRole = SortRole + 1;
    static constexpr int FullPathRole = SortRole + 2;
    static constexpr int ProgramIdRole = SortRole + 3;
    static constexpr int FileTypeRole = SortRole + 4;
    static constexpr int VersionRole = SortRole + 5;
    static constexpr int ScaledPixmapRole = SortRole + 6;
    static constexpr int HasUpdateBadgeRole = SortRole + 7;
    static constexpr int AddonCountRole = SortRole + 8;
    static constexpr int HasCustomConfigRole = SortRole + 9;
    static constexpr int PlatformRole = SortRole + 10;
    static constexpr int PlayTimeRole = SortRole + 11;
    static constexpr int FileSizeBytesRole = SortRole + 12;
    static constexpr int IconPathRole = SortRole + 13;
    static constexpr int InternalVersionRole = SortRole + 14;
    static constexpr int AddonSummaryRole = SortRole + 15;
    static constexpr int RawIconRole = SortRole + 16;
    static constexpr int IconRole = SortRole + 16;

    GameListItemPath() = default;
    GameListItemPath(const QString& game_path, const std::vector<u8>& picture_data,
                     const QString& game_name, const QString& game_type, u64 program_id,
                     u64 play_time, const QString& patch_versions, u64 size_bytes = 0,
                     const QString& base_version = QStringLiteral("1.0.0")) {
        setData(type(), TypeRole);
        setData(game_path, FullPathRole);
        setData(game_name, TitleRole);
        setData(qulonglong(program_id), ProgramIdRole);
        setData(game_type, FileTypeRole);

        const auto readable_play_time =
            play_time > 0 ? QString::fromStdString(
                                PlayTime::PlayTimeManager::GetReadablePlayTime(play_time))
                          : QObject::tr("Не запускалась");

        QString version_num = base_version.trimmed().isEmpty() ? QStringLiteral("1.0.0") : base_version.trimmed();
        int addon_count = 0;
        const QStringList lines = patch_versions.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        static const QRegularExpression update_regex{QStringLiteral(R"(Update\s*\(v?([0-9a-zA-Z\.\-]+)\))"), QRegularExpression::CaseInsensitiveOption};

        for (const QString& line : lines) {
            const auto match = update_regex.match(line);
            if (match.hasMatch() && match.hasCaptured(1)) {
                version_num = match.captured(1);
            } else if (line.contains(QStringLiteral("Update"), Qt::CaseInsensitive)) {
                QString clean = line;
                clean.remove(QStringLiteral("Update"), Qt::CaseInsensitive);
                clean.remove(QLatin1Char('('));
                clean.remove(QLatin1Char(')'));
                clean = clean.trimmed();
                if (!clean.isEmpty()) {
                    version_num = clean;
                }
            } else {
                QString clean = line.trimmed();
                clean.replace(QStringLiteral("DLC "), QStringLiteral(""), Qt::CaseInsensitive);
                clean.replace(QStringLiteral("DLC"), QStringLiteral(""), Qt::CaseInsensitive);
                clean.remove(QLatin1Char('('));
                clean.remove(QLatin1Char(')'));
                clean = clean.trimmed();
                if (!clean.isEmpty()) {
                    const auto parts = clean.split(QLatin1Char(','), Qt::SkipEmptyParts);
                    addon_count += parts.isEmpty() ? 1 : parts.size();
                } else {
                    addon_count++;
                }
            }
        }

        if (addon_count == 0) {
            static const QRegularExpression fn_dlc_regex{QStringLiteral(R"(\+([0-9]+)D\b)"), QRegularExpression::CaseInsensitiveOption};
            const auto dm = fn_dlc_regex.match(game_path);
            if (dm.hasMatch() && dm.hasCaptured(1)) {
                addon_count = dm.captured(1).toInt();
            }
        }

        if (!base_version.isEmpty() && base_version != QStringLiteral("1.0.0")) {
            version_num = base_version;
        } else if (version_num == QStringLiteral("1.0.0") || version_num.isEmpty()) {
            static const QRegularExpression fn_ver_regex{QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))")};
            const auto m = fn_ver_regex.match(game_path);
            if (m.hasMatch() && m.hasCaptured(1)) {
                version_num = m.captured(1);
            }
        }

        version_num = StormFormat::CleanVersion(version_num);

        u64 internal_ver = 0;
        static const QRegularExpression fn_pair_sub_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
        const auto pm = fn_pair_sub_regex.match(game_path);
        if (pm.hasMatch() && !pm.captured(2).isEmpty()) {
            internal_ver = pm.captured(2).toULongLong();
        }
        if (internal_ver == 0) {
            static const QRegularExpression fn_vnum_sub_regex{QStringLiteral(R"(\[v([0-9]+)\])")};
            const auto vm = fn_vnum_sub_regex.match(game_path);
            if (vm.hasMatch()) {
                internal_ver = vm.captured(1).toULongLong();
            }
        }

        QString addon_summary;
        static const QRegularExpression fn_tag_sub_regex{QStringLiteral(R"(\((1G(?:\+[0-9]+[UMD])+)\))"), QRegularExpression::CaseInsensitiveOption};
        const auto am = fn_tag_sub_regex.match(game_path);
        if (am.hasMatch() && am.hasCaptured(1)) {
            addon_summary = am.captured(1).toUpper();
        } else if (addon_count > 0) {
            addon_summary = StormFormat::FormatAddons(addon_count);
        }

        setData(version_num, VersionRole);
        setData(addon_count, AddonCountRole);
        setData(qulonglong(internal_ver), InternalVersionRole);
        setData(addon_summary, AddonSummaryRole);
        setData(qulonglong(play_time), PlayTimeRole);
        setData(qulonglong(size_bytes), FileSizeBytesRole);
        if (!picture_data.empty()) {
            setData(QByteArray(reinterpret_cast<const char*>(picture_data.data()), static_cast<int>(picture_data.size())), RawIconRole);
        }

        // Check custom config (item 21) with one-time directory cache (0 disk I/O per game)
        const std::string config_name = program_id == 0
            ? Common::FS::PathToUTF8String(std::filesystem::path(Common::FS::ToU8String(game_path.toStdString())).filename()).append(".ini")
            : fmt::format("{:016X}.ini", program_id);
        static const auto custom_config_names = []() {
            std::unordered_set<std::string> set;
            const auto custom_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
            std::error_code ec;
            if (std::filesystem::exists(custom_dir, ec)) {
                for (const auto& entry : std::filesystem::directory_iterator(custom_dir, std::filesystem::directory_options::skip_permission_denied, ec)) {
                    std::error_code fec;
                    if (entry.is_regular_file(fec)) {
                        set.insert(entry.path().filename().string());
                    }
                }
            }
            return set;
        }();
        const bool has_custom_config = custom_config_names.contains(config_name);
        setData(has_custom_config, HasCustomConfigRole);

        // Check platform (Switch 1 vs Switch 2)
        const bool is_switch2 = ((program_id >> 48) == 0x0400ULL) ||
                                ((program_id >> 40) == 0x040000ULL) ||
                                ((program_id & 0x000F000000000000ULL) != 0ULL) ||
                                game_type.compare(QStringLiteral("NUP"), Qt::CaseInsensitive) == 0 ||
                                game_path.endsWith(QStringLiteral(".nup"), Qt::CaseInsensitive) ||
                                game_name.contains(QStringLiteral("Switch 2"), Qt::CaseInsensitive) ||
                                game_path.contains(QStringLiteral("switch2"), Qt::CaseInsensitive) ||
                                game_path.contains(QStringLiteral("nx2"), Qt::CaseInsensitive);
        setData(is_switch2 ? 1 : 0, PlatformRole);

        // Path to cached icon (0 synchronous disk seeks during construction; loaded lazily in data())
        const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
        const auto icon_file = cache_dir / "icons" / fmt::format("{:016X}.png", program_id);
        setData(QString::fromStdString(Common::FS::PathToUTF8String(icon_file)), IconPathRole);

        if (!picture_data.empty()) {
            m_image.loadFromData(picture_data.data(), static_cast<u32>(picture_data.size()));
        }
    }

    void setData(const QVariant& value, int role = Qt::UserRole + 1) override {
        if (role == ScaledPixmapRole || role == Qt::DecorationRole) {
            if (value.canConvert<QPixmap>()) {
                m_pixmap = value.value<QPixmap>();
                m_cached_icon_size = m_pixmap.width();
                emit emitDataChanged();
                return;
            }
        }
        GameListItem::setData(value, role);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    QVariant data(int role) const override {
        const u32 target_size = UISettings::values.game_icon_size.GetValue();
        if (role == ScaledPixmapRole) {
            if (!m_pixmap.isNull() && (m_cached_icon_size == 0 || m_cached_icon_size >= static_cast<int>(target_size))) {
                return m_pixmap;
            }
        }

        if (role == Qt::DecorationRole || role == ScaledPixmapRole) {
            if (m_pixmap.isNull() || (m_cached_icon_size > 0 && m_cached_icon_size != static_cast<int>(target_size))) {
                if (m_image.isNull()) {
                    const QByteArray raw = data(RawIconRole).toByteArray();
                    if (!raw.isEmpty()) {
                        m_image.loadFromData(raw);
                    } else {
                        const QString icon_path = data(IconPathRole).toString();
                        if (!icon_path.isEmpty() && QFile::exists(icon_path)) {
                            m_image.load(icon_path);
                        } else {
                            const u64 pid = data(ProgramIdRole).toULongLong();
                            if (pid != 0) {
                                const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
                                const auto jpeg_file = cache_dir / fmt::format("{:016X}.jpeg", pid);
                                const auto jpeg_path = QString::fromStdString(Common::FS::PathToUTF8String(jpeg_file));
                                if (QFile::exists(jpeg_path)) {
                                    m_image.load(jpeg_path);
                                }
                            }
                        }
                    }
                }
                if (!m_image.isNull()) {
                    const int icon_sz = static_cast<int>(target_size);
                    if (m_image.width() != icon_sz || m_image.height() != icon_sz) {
                        m_pixmap = QPixmap::fromImage(m_image.scaled(icon_sz, icon_sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    } else {
                        m_pixmap = QPixmap::fromImage(m_image);
                    }
                    m_cached_icon_size = icon_sz;
                }
            }
            if (!m_pixmap.isNull()) {
                return m_pixmap;
            }
            return GetDefaultImage(target_size);
        }

        if (role == Qt::ToolTipRole) {
            return GetLazyToolTip();
        }

        if (role == Qt::DisplayRole || role == SortRole) {
            std::string filename;
            Common::SplitPath(data(FullPathRole).toString().toStdString(), nullptr, &filename,
                              nullptr);

            QString display_title = data(TitleRole).toString().trimmed();
            if (display_title.isEmpty()) {
                display_title = QString::fromStdString(filename);
            }

            const std::array<QString, 4> row_data{{
                QString::fromStdString(filename),
                data(FileTypeRole).toString(),
                StormFormat::FormatTitleId(data(ProgramIdRole).toULongLong()),
                display_title,
            }};

            const auto& row1 = row_data.at(UISettings::values.row_1_text_id.GetValue());
            // don't show row 2 on grid view
            switch (UISettings::values.game_list_mode.GetValue()) {

            case Settings::GameListMode::TreeView: {
                const int row2_id = UISettings::values.row_2_text_id.GetValue();

                if (role == SortRole) {
                    return row1.toLower();
                }

                // None
                if (row2_id == 4) {
                    return row1;
                }

                const auto& row2 = row_data.at(row2_id);

                if (row1 == row2) {
                    return row1;
                }

                return QStringLiteral("%1\n    %2").arg(row1, row2);
            }
            case Settings::GameListMode::GridView:
            case Settings::GameListMode::CarouselView:
                return row1;
            default:
                break;
            }
        }

        return GameListItem::data(role);
    }

private:
    QString GetLazyToolTip() const {
        const u64 program_id = data(ProgramIdRole).toULongLong();
        QString game_name = data(Qt::DisplayRole).toString().split(QLatin1Char('\n')).first().trimmed();
        if (game_name.isEmpty()) {
            game_name = data(TitleRole).toString().trimmed();
        }
        if (game_name.isEmpty()) {
            std::string filename;
            Common::SplitPath(data(FullPathRole).toString().toStdString(), nullptr, &filename, nullptr);
            game_name = QString::fromStdString(filename);
        }
        const QString game_type = data(FileTypeRole).toString();
        const QString game_path = data(FullPathRole).toString();
        const QString version_num = data(VersionRole).toString();
        const int addon_count = data(AddonCountRole).toInt();
        const u64 size_bytes = data(FileSizeBytesRole).toULongLong();
        const u64 play_time = data(PlayTimeRole).toULongLong();

        const auto readable_play_time =
            play_time > 0 ? QString::fromStdString(PlayTime::PlayTimeManager::GetReadablePlayTime(play_time))
                          : QObject::tr("Не запускалась");

        const QString addons_info = StormFormat::FormatAddons(addon_count);
        const QString formatted_title_id = StormFormat::FormatTitleId(program_id);
        const QString formatted_size = size_bytes > 0 ? StormFormat::ReadableByteSize(size_bytes) : QStringLiteral("-");

        QString icon_base64;
        if (!m_image.isNull()) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            m_image.save(&buffer, "PNG", 80);
            icon_base64 = QString::fromLatin1(bArray.toBase64());
        }

        const QString custom_accent = QString::fromStdString(UISettings::values.accent_color.GetValue());
        const QString tooltip_accent = custom_accent.isEmpty() ? QStringLiteral("#8B5CF6") : custom_accent;

        return QObject::tr(
            R"(<table style="border-collapse: collapse; width: 640px;">)"
            R"(<tr>)"
            R"(<td style="vertical-align: middle; width: 110px; padding-right: 14px; text-align: center;">)"
            R"(<img src="data:image/png;base64,%1" width="96" height="96" style="border-radius: 8px;" />)"
            R"(</td>)"
            R"(<td style="vertical-align: middle;">)"
            R"(<div style="font-size: 11.5pt; font-weight: bold; color: #ffee00; line-height: 1.25; margin-bottom: 2px;">%2</div>)"
            R"(<div style="font-size: 8.5pt; font-weight: bold; color: %10; margin-bottom: 6px;">[%3]</div>)"
            R"(<table style="font-size: 8.5pt; width: 100%%; border-collapse: collapse;">)"
            R"(<tr><td style="color: #8fa2b8; padding: 1px 6px 1px 0; width: 130px;"><b>ID приложения:</b></td><td style="color: %10; font-family: monospace;">%4</td></tr>)"
            R"(<tr><td style="color: #8fa2b8; padding: 1px 6px 1px 0;"><b>Версия:</b></td><td><b style="color: #00ffaa;">%5</b></td></tr>)"
            R"(<tr><td style="color: #8fa2b8; padding: 1px 6px 1px 0;"><b>Размер:</b></td><td style="color: #ffffff;">%6</td></tr>)"
            R"(<tr><td style="color: #8fa2b8; padding: 1px 6px 1px 0;"><b>Время игры:</b></td><td style="color: #ffffff;">%7</td></tr>)"
            R"(<tr><td style="color: #8fa2b8; padding: 1px 6px 1px 0;"><b>Дополнения:</b></td><td style="color: #ffffff;">%8</td></tr>)"
            R"(</table>)"
            R"(</td>)"
            R"(</tr>)"
            R"(<tr>)"
            R"(<td colspan="2" style="padding-top: 6px; border-top: 1px solid rgba(255, 255, 255, 0.12); font-size: 8pt; color: #8fa2b8; word-break: break-all;">)"
            R"(<b>Путь к файлу:</b> <span style="color: #cbd5e1; font-family: monospace;">%9</span>)"
            R"(</td>)"
            R"(</tr>)"
            R"(</table>)"
        ).arg(icon_base64, game_name.toHtmlEscaped(), game_type.toHtmlEscaped(), formatted_title_id, version_num, formatted_size, readable_play_time, addons_info, game_path.toHtmlEscaped(), tooltip_accent);
    }
};

class GameListItemCompat : public GameListItem {
    Q_DECLARE_TR_FUNCTIONS(GameListItemCompat)
public:
    static constexpr int CompatNumberRole = SortRole;
    GameListItemCompat() = default;
    explicit GameListItemCompat(const QString& compatibility) {
        setData(type(), TypeRole);

        struct CompatStatus {
            QString color;
            const char* text;
            const char* tooltip;
        };
        // clang-format off
        const auto ingame_status =
                       CompatStatus{QStringLiteral("#f2d624"), QT_TR_NOOP("Ingame"),     QT_TR_NOOP("Game starts, but crashes or major glitches prevent it from being completed.")};
        static const std::map<QString, CompatStatus> status_data = {
            {QStringLiteral("0"),  {QStringLiteral("#5c93ed"), QT_TR_NOOP("Perfect"),    QT_TR_NOOP("Game can be played without issues.")}},
            {QStringLiteral("1"),  {QStringLiteral("#47d35c"), QT_TR_NOOP("Playable"),   QT_TR_NOOP("Game functions with minor graphical or audio glitches and is playable from start to finish.")}},
            {QStringLiteral("2"),  ingame_status},
            {QStringLiteral("3"),  ingame_status}, // Fallback for the removed "Okay" category
            {QStringLiteral("4"),  {QStringLiteral("#FF0000"), QT_TR_NOOP("Intro/Menu"), QT_TR_NOOP("Game loads, but is unable to progress past the Start Screen.")}},
            {QStringLiteral("5"),  {QStringLiteral("#828282"), QT_TR_NOOP("Won't Boot"), QT_TR_NOOP("The game crashes when attempting to startup.")}},
            {QStringLiteral("99"), {QStringLiteral("#000000"), QT_TR_NOOP("Not Tested"), QT_TR_NOOP("The game has not yet been tested.")}},
        };
        // clang-format on

        auto iterator = status_data.find(compatibility);
        if (iterator == status_data.end()) {
            LOG_WARNING(Frontend, "Invalid compatibility number {}", compatibility.toStdString());
            return;
        }
        const CompatStatus& status = iterator->second;
        setData(compatibility, CompatNumberRole);
        setText(tr(status.text));
        setToolTip(tr(status.tooltip));
        setData(QtCommon::CreateCirclePixmapFromColor(status.color), Qt::DecorationRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    bool operator<(const QStandardItem& other) const override {
        return data(CompatNumberRole).value<QString>() <
               other.data(CompatNumberRole).value<QString>();
    }
};

class GameListItemCentered : public GameListItem {
public:
    GameListItemCentered() = default;
    explicit GameListItemCentered(const QString& string) : GameListItem(string) {}

    QVariant data(int role) const override {
        if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }
        return GameListItem::data(role);
    }
};

class GameListItemSize : public GameListItem {
public:
    static constexpr int SizeRole = SortRole;

    GameListItemSize() = default;
    explicit GameListItemSize(const qulonglong size_bytes) {
        setData(type(), TypeRole);
        setData(size_bytes, SizeRole);
    }

    void setData(const QVariant& value, int role) override {
        if (role == SizeRole) {
            qulonglong size_bytes = value.toULongLong();
            GameListItem::setData(StormFormat::ReadableByteSize(size_bytes), Qt::DisplayRole);
            GameListItem::setData(value, SizeRole);
        } else {
            GameListItem::setData(value, role);
        }
    }

    QVariant data(int role) const override {
        if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }
        return GameListItem::data(role);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    bool operator<(const QStandardItem& other) const override {
        return data(SizeRole).toULongLong() < other.data(SizeRole).toULongLong();
    }
};

class GameListItemPlayTime : public GameListItem {
public:
    static constexpr int PlayTimeRole = SortRole;

    GameListItemPlayTime() = default;
    explicit GameListItemPlayTime(const qulonglong time_seconds) {
        setData(time_seconds, PlayTimeRole);
    }

    void setData(const QVariant& value, int role) override {
        qulonglong time_seconds = value.toULongLong();
        GameListItem::setData(
            QString::fromStdString(PlayTime::PlayTimeManager::GetReadablePlayTime(time_seconds)),
            Qt::DisplayRole);
        GameListItem::setData(value, PlayTimeRole);
    }

    QVariant data(int role) const override {
        if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }
        return GameListItem::data(role);
    }

    bool operator<(const QStandardItem& other) const override {
        return data(PlayTimeRole).toULongLong() < other.data(PlayTimeRole).toULongLong();
    }
};

class GameListDir : public GameListItem {
public:
    static constexpr int GameDirRole = Qt::UserRole + 2;

    explicit GameListDir(UISettings::GameDir& directory,
                         GameListItemType dir_type_ = GameListItemType::CustomDir)
        : dir_type{dir_type_} {
        setData(type(), TypeRole);

        UISettings::GameDir* game_dir = &directory;
        setData(QVariant(UISettings::values.game_dirs.indexOf(directory)), GameDirRole);

        const char* icon_name = nullptr;

        switch (dir_type) {
        case GameListItemType::SdmcDir:
            icon_name = "sd_card";
            setData(QObject::tr("Установленные на SD игры"), Qt::DisplayRole);
            break;
        case GameListItemType::UserNandDir:
            icon_name = "chip";
            setData(QObject::tr("Установленные в NAND игры"), Qt::DisplayRole);
            break;
        case GameListItemType::SysNandDir:
            icon_name = "chip";
            setData(QObject::tr("Системные программы"), Qt::DisplayRole);
            break;
        case GameListItemType::CustomDir: {
            const QString path = QString::fromStdString(game_dir->path);
            icon_name = QFileInfo::exists(path) ? "folder" : "bad_folder";
            setData(path, Qt::DisplayRole);
            break;
        }
        default:
            break;
        }

        if (icon_name != nullptr)
            setData(ThemeIcon(icon_name), Qt::DecorationRole);
    }

    int type() const override {
        return static_cast<int>(dir_type);
    }

    QVariant data(int role) const override {
        if (role == Qt::FontRole) {
            QFont font;
            font.setFamily(QStringLiteral("Segoe UI"));
            font.setBold(true);
            font.setPointSizeF(9.5);
            return font;
        }
        return GameListItem::data(role);
    }

    bool operator<(const QStandardItem& other) const override {
        return false;
    }

private:
    GameListItemType dir_type;
};

class GameListAddDir : public GameListItem {
public:
    explicit GameListAddDir() {
        setData(type(), TypeRole);

        setData(ThemeIcon("list-add"), Qt::DecorationRole);
        setData(QObject::tr("Добавить новую папку с играми"), Qt::DisplayRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::AddDir);
    }

    QVariant data(int role) const override {
        if (role == Qt::FontRole) {
            QFont font;
            font.setFamily(QStringLiteral("Segoe UI"));
            font.setBold(true);
            font.setPointSizeF(9.5);
            return font;
        }
        return GameListItem::data(role);
    }

    bool operator<(const QStandardItem& other) const override {
        return false;
    }
};

class GameListFavorites : public GameListItem {
public:
    explicit GameListFavorites() {
        setData(type(), TypeRole);

        setData(ThemeIcon("star"), Qt::DecorationRole);
        setData(QObject::tr("Избранное"), Qt::DisplayRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Favorites);
    }

    QVariant data(int role) const override {
        if (role == Qt::FontRole) {
            QFont font;
            font.setFamily(QStringLiteral("Segoe UI"));
            font.setBold(true);
            font.setPointSizeF(9.5);
            return font;
        }
        return GameListItem::data(role);
    }

    bool operator<(const QStandardItem& other) const override {
        return false;
    }
};

inline QString FormatGameName(const std::string& physical_name) {
    const QString physical_name_as_qstring = QString::fromStdString(physical_name);
    const QFileInfo file_info(physical_name_as_qstring);
    if (physical_name.ends_with("/main") || physical_name.ends_with("\\main")) {
        return file_info.dir().path();
    }
    return physical_name_as_qstring;
}

inline QList<QStandardItem*> MakeCachedGameListEntry(
    const std::string& path, const std::string& name, const std::size_t size,
    const std::vector<u8>& icon, const QString& file_type_string, u64 program_id,
    const PlayTime::PlayTimeManager& play_time_manager,
    const QString& patch_versions, const QString& file_version, const QString& addons_text) {
    const u64 play_time = play_time_manager.GetPlayTime(program_id);
    return QList<QStandardItem*>{
        new GameListItemPath(FormatGameName(path), icon, QString::fromStdString(name),
                             file_type_string, program_id, play_time, patch_versions, size, file_version),
        new GameListItemCentered(file_type_string),
        new GameListItemSize(size),
        new GameListItemPlayTime(play_time),
        new GameListItem(addons_text),
    };
}
