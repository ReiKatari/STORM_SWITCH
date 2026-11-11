// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

#include "game_card.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/game/game_cache.h"
#include "storm_switch/storm_catalog_cache.h"

GameCard::GameCard(QObject* parent) : QStyledItemDelegate{parent} {
    setObjectName(QStringLiteral("GameCard"));
}

void GameCard::paint(QPainter* painter, const QStyleOptionViewItem& option,
                     const QModelIndex& index) const {
    if (!index.isValid())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    constexpr int cardCornerRadius = 12;
    const QRect cardRect = getCardRect(option, index);

    const auto pal = StormTheme::GetThemePalette();
    QColor backgroundColor = QColor(pal.card);
    QColor borderColor = QColor(pal.border);
    QColor textColor = QColor(pal.textPrimary);

    const bool is_selected = (option.state & QStyle::State_Selected);
    const bool is_hovered = (option.state & QStyle::State_MouseOver);

    if (is_selected) {
        backgroundColor = QColor(pal.itemSelected);
        borderColor = QColor(pal.accent);
        textColor = QColor(pal.textPrimary);
    } else if (is_hovered) {
        backgroundColor = QColor(pal.itemHover);
        borderColor = QColor(pal.accentHover.isEmpty() ? pal.accent : pal.accentHover);
    }

    // 1. Card container background & border
    painter->setBrush(backgroundColor);
    painter->setPen(QPen(borderColor, is_selected ? 2 : 1));
    painter->drawRoundedRect(cardRect, cardCornerRadius, cardCornerRadius);

    const u32 icon_size = UISettings::values.game_icon_size.GetValue();
    const u64 program_id = index.data(GameListItemPath::ProgramIdRole).toULongLong();

    // 2. Cover pixmap
    QPixmap icon_pixmap = index.data(GameListItemPath::ScaledPixmapRole).value<QPixmap>();

    const qreal dpr = (painter->device() != nullptr) ? painter->device()->devicePixelRatioF() : 1.0;
    const int cover_w = static_cast<int>(icon_size);
    const int cover_h = static_cast<int>(icon_size);
    const int target_px = static_cast<int>(std::round(cover_w * dpr));

    // High-resolution fallback: if cached pixmap is missing or smaller than target_px, load full 256x256 source
    if ((icon_pixmap.isNull() || icon_pixmap.width() < target_px) && program_id != 0) {
        const QByteArray raw_icon = index.data(GameListItemPath::RawIconRole).toByteArray();
        QImage sharp_img;
        if (!raw_icon.isEmpty()) {
            sharp_img.loadFromData(raw_icon);
        }
        if (sharp_img.isNull()) {
            const QString icon_path = index.data(GameListItemPath::IconPathRole).toString();
            if (!icon_path.isEmpty() && QFile::exists(icon_path)) {
                sharp_img.load(icon_path);
            }
        }
        if (sharp_img.isNull()) {
            const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
            const auto jpeg_file = cache_dir / fmt::format("{:016X}.jpeg", program_id);
            const QString jp = QString::fromStdString(Common::FS::PathToUTF8String(jpeg_file));
            if (QFile::exists(jp)) {
                sharp_img.load(jp);
            }
        }
        if (!sharp_img.isNull()) {
            QImage scaled_img = sharp_img.scaled(target_px, target_px, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            icon_pixmap = QPixmap::fromImage(scaled_img);
            icon_pixmap.setDevicePixelRatio(dpr);
            if (auto* m = const_cast<QAbstractItemModel*>(index.model())) {
                m->setData(index, icon_pixmap, GameListItemPath::ScaledPixmapRole);
            }
        }
    }

    // Lazy async cover loading if still not loaded
    if (icon_pixmap.isNull() && program_id != 0) {
        const QString icon_path = index.data(GameListItemPath::IconPathRole).toString();
        GameCoverLoader::Instance().RequestCover(
            program_id, icon_path, static_cast<int>(icon_size),
            QPersistentModelIndex(index), const_cast<QAbstractItemModel*>(index.model()));
    }

    if (icon_pixmap.isNull()) {
        icon_pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
    }

    const QRect iconRect = {
        cardRect.left() + (cardRect.width() - cover_w) / 2,
        cardRect.top() + cardMargin,
        cover_w,
        cover_h
    };

    if (!icon_pixmap.isNull()) {
        painter->save();
        QPainterPath clip_path;
        clip_path.addRoundedRect(iconRect, 8, 8);
        painter->setClipPath(clip_path);
        painter->drawPixmap(iconRect, icon_pixmap);
        painter->restore();
    } else {
        // Sleek placeholder when cover is loading
        painter->save();
        QPainterPath clip_path;
        clip_path.addRoundedRect(iconRect, 8, 8);
        painter->setClipPath(clip_path);
        painter->setBrush(QColor(pal.surface));
        painter->setPen(Qt::NoPen);
        painter->drawRect(iconRect);

        painter->setPen(QColor(pal.textSecondary));
        QFont ph_font = option.font;
        ph_font.setBold(true);
        ph_font.setPixelSize(12);
        painter->setFont(ph_font);
        painter->drawText(iconRect, Qt::AlignCenter, QStringLiteral("STORM"));
        painter->restore();
    }

    // 3. BADGES
    // Version badge (bottom-left): e.g. "1.0.0 · 131072" or "1.0.0"
    QString ver_badge = index.data(GameListItemPath::VersionRole).toString().trimmed();
    while (ver_badge.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        ver_badge.remove(0, 1);
    }
    ver_badge = ver_badge.trimmed();
    if (ver_badge.isEmpty() || ver_badge == QStringLiteral("0")) {
        ver_badge = QStringLiteral("1.0.0");
    }
    const QString clean_ver = ver_badge;
    const u64 int_ver = index.data(GameListItemPath::InternalVersionRole).toULongLong();
    if (int_ver > 0) {
        ver_badge.append(QStringLiteral(" · %1").arg(int_ver));
    }
    DrawVersionBadge(painter, iconRect, ver_badge);

    // "Own Profile" Gear Badge (top-left)
    const bool has_custom_config = index.data(GameListItemPath::HasCustomConfigRole).toBool();
    if (has_custom_config) {
        DrawGearBadge(painter, iconRect);
    }

    // Update Available Badge (top-right)
    bool has_update = index.data(GameListItemPath::HasUpdateBadgeRole).toBool();
    if (!has_update && program_id != 0) {
        has_update = StormCatalogCache::Instance().HasUpdate(program_id, clean_ver);
    }
    if (has_update) {
        DrawUpdateBadge(painter, iconRect);
    }

    // Addon Summary Badge (bottom-right): e.g. "1G+1U", "+1 UPD", "+2 DLC"
    QString addon_text = index.data(GameListItemPath::AddonSummaryRole).toString().trimmed();
    if (addon_text.isEmpty()) {
        const int addon_count = index.data(GameListItemPath::AddonCountRole).toInt();
        if (addon_count > 0) {
            addon_text = QStringLiteral("+%1 DLC").arg(addon_count);
        }
    }
    if (!addon_text.isEmpty()) {
        DrawDlcBadge(painter, iconRect, addon_text);
    }

    // Switch 2 Platform Badge (top-left above or bottom-left)
    const int platform = index.data(GameListItemPath::PlatformRole).toInt();
    if (platform == 1) {
        DrawSwitch2Badge(painter, iconRect);
    }

    // 4. Game Title below cover - two cleanly wrapped & elided lines, zero clipped 3rd line
    if (UISettings::values.show_game_name.GetValue()) {
        QRect textRect = cardRect;
        textRect.setTop(iconRect.bottom() + 4);
        textRect.adjust(2, 0, -2, -2);

        QString full_title = index.data(Qt::DisplayRole).toString().split(QLatin1Char('\n')).first().trimmed();

        painter->setPen(textColor);
        QFont font = option.font;
        font.setBold(true);
        font.setPixelSize(std::max(11.0, std::sqrt(static_cast<double>(icon_size))));
        painter->setFont(font);

        QFontMetrics fm(font);
        const int line_h = fm.lineSpacing();
        const int max_w = std::max(20, textRect.width());

        QString line1, line2;
        if (fm.horizontalAdvance(full_title) <= max_w) {
            line1 = full_title;
        } else {
            const QStringList words = full_title.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            int w_idx = 0;
            while (w_idx < words.size()) {
                const QString cand = line1.isEmpty() ? words[w_idx] : (line1 + QLatin1Char(' ') + words[w_idx]);
                if (fm.horizontalAdvance(cand) <= max_w) {
                    line1 = cand;
                    w_idx++;
                } else {
                    break;
                }
            }
            if (line1.isEmpty() && w_idx < words.size()) {
                line1 = fm.elidedText(words[w_idx], Qt::ElideRight, max_w);
                w_idx++;
            }
            QString remainder;
            for (int i = w_idx; i < words.size(); ++i) {
                if (!remainder.isEmpty()) remainder += QLatin1Char(' ');
                remainder += words[i];
            }
            if (!remainder.isEmpty()) {
                line2 = fm.elidedText(remainder, Qt::ElideRight, max_w);
            }
        }

        if (!line1.isEmpty()) {
            const QRect r1(textRect.left(), textRect.top(), textRect.width(), line_h);
            painter->drawText(r1, Qt::AlignHCenter | Qt::AlignVCenter, line1);
        }
        if (!line2.isEmpty()) {
            const QRect r2(textRect.left(), textRect.top() + line_h, textRect.width(), line_h);
            painter->drawText(r2, Qt::AlignHCenter | Qt::AlignVCenter, line2);
        }
    }

    painter->restore();
}

void GameCard::DrawVersionBadge(QPainter* painter, const QRect& iconRect, const QString& version) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString text = version.trimmed();
    while (text.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        text.remove(0, 1);
    }
    text = text.trimmed();
    QFont badge_font = painter->font();
    badge_font.setBold(true);
    badge_font.setPixelSize(9);
    painter->setFont(badge_font);

    QFontMetrics fm(badge_font);
    const int text_w = fm.horizontalAdvance(text);
    const int badge_w = text_w + 10;
    const int badge_h = 16;

    const int badge_x = iconRect.left() + 4;
    const int badge_y = iconRect.bottom() - badge_h - 4;
    const QRect rect(badge_x, badge_y, badge_w, badge_h);

    painter->setPen(QPen(QColor(255, 255, 255, 60), 1));
    painter->setBrush(QColor(15, 23, 42, 220));
    painter->drawRoundedRect(rect, 4, 4);

    painter->setPen(QColor(52, 211, 153)); // Soft emerald
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

void GameCard::DrawGearBadge(QPainter* painter, const QRect& iconRect) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString text = tr("ПРОФИЛЬ");
    QFont badge_font = painter->font();
    badge_font.setBold(true);
    badge_font.setPixelSize(8);
    painter->setFont(badge_font);

    QFontMetrics fm(badge_font);
    const int text_w = fm.horizontalAdvance(text);
    const int badge_w = text_w + 10;
    const int badge_h = 16;
    const int badge_x = iconRect.left() + 4;
    const int badge_y = iconRect.top() + 4;
    const QRect rect(badge_x, badge_y, badge_w, badge_h);

    const auto pal = StormTheme::GetThemePalette();
    QColor accent_col = QColor(pal.accent);

    // Vector pill with theme accent color indicating per-game custom profile
    painter->setPen(QPen(accent_col, 1));
    painter->setBrush(QColor(15, 23, 42, 220));
    painter->drawRoundedRect(rect, 4, 4);

    painter->setPen(accent_col);
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

void GameCard::DrawUpdateBadge(QPainter* painter, const QRect& iconRect) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString badge_text = tr("Обновление");
    QFont badge_font = painter->font();
    badge_font.setBold(true);
    badge_font.setPixelSize(9);
    painter->setFont(badge_font);

    QFontMetrics fm(badge_font);
    const int text_w = fm.horizontalAdvance(badge_text);
    const int badge_w = text_w + 12;
    const int badge_h = 18;

    const int badge_x = iconRect.right() - badge_w - 4;
    const int badge_y = iconRect.top() + 4;
    const QRect badge_rect(badge_x, badge_y, badge_w, badge_h);

    // Glowing cyan-blue gradient pill
    QLinearGradient grad(badge_rect.topLeft(), badge_rect.bottomRight());
    grad.setColorAt(0.0, QColor(0, 210, 255, 240));
    grad.setColorAt(1.0, QColor(0, 114, 255, 240));

    painter->setPen(QPen(QColor(255, 255, 255, 220), 1));
    painter->setBrush(grad);
    painter->drawRoundedRect(badge_rect, 9, 9);

    painter->setPen(QColor(0, 0, 0));
    painter->drawText(badge_rect, Qt::AlignCenter, badge_text);
    painter->restore();
}

void GameCard::DrawDlcBadge(QPainter* painter, const QRect& iconRect, int dlc_count) const {
    DrawDlcBadge(painter, iconRect, QStringLiteral("+%1 DLC").arg(dlc_count));
}

void GameCard::DrawDlcBadge(QPainter* painter, const QRect& iconRect, const QString& text) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont badge_font = painter->font();
    badge_font.setBold(true);
    badge_font.setPixelSize(9);
    painter->setFont(badge_font);

    QFontMetrics fm(badge_font);
    const int text_w = fm.horizontalAdvance(text);
    const int badge_w = text_w + 10;
    const int badge_h = 16;

    const int badge_x = iconRect.right() - badge_w - 4;
    const int badge_y = iconRect.bottom() - badge_h - 4;
    const QRect rect(badge_x, badge_y, badge_w, badge_h);

    painter->setPen(QPen(QColor(255, 255, 255, 60), 1));
    painter->setBrush(QColor(15, 23, 42, 220));
    painter->drawRoundedRect(rect, 4, 4);

    painter->setPen(QColor(192, 132, 252)); // Soft purple
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

void GameCard::DrawSwitch2Badge(QPainter* painter, const QRect& iconRect) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString text = QStringLiteral("SWITCH 2");
    QFont badge_font = painter->font();
    badge_font.setBold(true);
    badge_font.setPixelSize(8);
    painter->setFont(badge_font);

    QFontMetrics fm(badge_font);
    const int text_w = fm.horizontalAdvance(text);
    const int badge_w = text_w + 8;
    const int badge_h = 15;

    const int badge_x = iconRect.left() + 4;
    const int badge_y = iconRect.bottom() - badge_h - 22; // stacked above version
    const QRect rect(badge_x, badge_y, badge_w, badge_h);

    QLinearGradient grad(rect.topLeft(), rect.bottomRight());
    grad.setColorAt(0.0, QColor(239, 68, 68, 230));  // Neon red
    grad.setColorAt(1.0, QColor(245, 158, 11, 230)); // Amber

    painter->setPen(QPen(QColor(255, 255, 255, 200), 1));
    painter->setBrush(grad);
    painter->drawRoundedRect(rect, 3, 3);

    painter->setPen(QColor(255, 255, 255));
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

QRect GameCard::getCardRect(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const int cell_width = option.rect.width();
    const int cell_height = option.rect.height();
    const int card_w = std::max(20, cell_width - 8);
    const int card_h = std::max(20, cell_height - 6);
    const int left = option.rect.left() + (cell_width - card_w) / 2;
    const int top = option.rect.top() + (cell_height - card_h) / 2;
    return QRect(left, top, card_w, card_h);
}

bool GameCard::hitTest(const QPoint& point, const QModelIndex& index, const QWidget* widget,
                       const QRect& cellRect) const {
    QStyleOptionViewItem option;
    option.initFrom(widget);
    option.rect = cellRect;
    return getCardRect(option, index).contains(point);
}

QSize GameCard::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return m_size;
}

void GameCard::setSize(const QSize& newSize, const QSize& contentSize, const int padding,
                       const int columns) {
    m_size = newSize;
    m_contentSize = contentSize;
    m_padding = padding;
    m_columns = columns;
}
