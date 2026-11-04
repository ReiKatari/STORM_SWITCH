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

    // 2. Cover pixmap - zero computation in paint()!
    QPixmap icon_pixmap = index.data(GameListItemPath::ScaledPixmapRole).value<QPixmap>();

    // Lazy async cover loading if not yet loaded
    if (icon_pixmap.isNull() && program_id != 0) {
        const QString icon_path = index.data(GameListItemPath::IconPathRole).toString();
        GameCoverLoader::Instance().RequestCover(
            program_id, icon_path, static_cast<int>(icon_size),
            QPersistentModelIndex(index), const_cast<QAbstractItemModel*>(index.model()));
    }

    if (icon_pixmap.isNull()) {
        icon_pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
    }

    const int cover_w = static_cast<int>(icon_size);
    const int cover_h = static_cast<int>(icon_size);
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

    // 3. BADGES - ALL PRECOMPUTED IN MODEL ROLES (Item 20 & 21)
    // Version badge (top-left or bottom-left)
    const QString version_str = index.data(GameListItemPath::VersionRole).toString();
    if (!version_str.isEmpty() && version_str != QStringLiteral("1.0.0")) {
        DrawVersionBadge(painter, iconRect, version_str);
    }

    // "Own Profile" Gear Badge (Item 21)
    const bool has_custom_config = index.data(GameListItemPath::HasCustomConfigRole).toBool();
    if (has_custom_config) {
        DrawGearBadge(painter, iconRect);
    }

    // Update Available Badge (top-right)
    bool has_update = index.data(GameListItemPath::HasUpdateBadgeRole).toBool();
    if (!has_update && program_id != 0) {
        // Fast one-time check from in-memory catalog
        has_update = StormCatalogCache::Instance().HasUpdate(program_id, version_str);
    }
    if (has_update) {
        DrawUpdateBadge(painter, iconRect);
    }

    // DLC Count Badge (bottom-right)
    const int addon_count = index.data(GameListItemPath::AddonCountRole).toInt();
    if (addon_count > 0) {
        DrawDlcBadge(painter, iconRect, addon_count);
    }

    // Switch 2 Platform Badge (bottom-left)
    const int platform = index.data(GameListItemPath::PlatformRole).toInt();
    if (platform == 1) {
        DrawSwitch2Badge(painter, iconRect);
    }

    // 4. Game Title below cover
    if (UISettings::values.show_game_name.GetValue()) {
        QRect textRect = cardRect;
        textRect.setTop(iconRect.bottom() + cardMargin);
        textRect.adjust(cardMargin, 0, -cardMargin, -cardMargin);

        QString title = index.data(Qt::DisplayRole).toString().split(QLatin1Char('\n')).first();

        painter->setPen(textColor);
        QFont font = option.font;
        font.setBold(true);
        font.setPixelSize(std::max(11.0, std::sqrt(static_cast<double>(icon_size))));
        painter->setFont(font);

        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, title);
    }

    painter->restore();
}

void GameCard::DrawVersionBadge(QPainter* painter, const QRect& iconRect, const QString& version) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString text = version.startsWith(QLatin1Char('v')) ? version : (QStringLiteral("v") + version);
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
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString text = QStringLiteral("+%1 DLC").arg(dlc_count);
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
    const int card_width = cell_width - m_padding;

    int card_left, card_top, card_height;

    if (m_columns >= 1) {
        const int column = index.row() % m_columns;
        const int row_width = m_columns * cell_width;
        const int total_gap = row_width - cardMargin * 2 - m_columns * card_width;
        const int gap = (m_columns > 1) ? (total_gap / (m_columns - 1)) : 0;
        card_left =
            option.rect.left() - column * cell_width + cardMargin + column * (card_width + gap) + 4;

        card_top = option.rect.top() + cardMargin;
        card_height = option.rect.height() - cardMargin - 1;
    } else {
        card_left = option.rect.left() + cardMargin + 4;
        const int content_height = m_contentSize.height() - cardMargin;
        const int cell_height = option.rect.height();

        card_height = std::min(content_height, cell_height - cardMargin * 2) - 1;
        card_top = option.rect.top() + (cell_height - card_height) / 2;
    }

    return QRect(card_left, card_top, card_width - cardMargin, card_height);
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
