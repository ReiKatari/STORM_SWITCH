// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QKeyEvent>
#include <QResizeEvent>
#include <QScroller>
#include <QScrollerProperties>

#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/model.h"
#include "storm_switch/game/common.h"
#include "storm_switch/game/game_card.h"
#include "storm_switch/game/game_grid.h"
#include "storm_switch/storm_catalog_cache.h"

GameGrid::GameGrid(QWidget* parent) : QListView{parent} {
    m_gameCard = new GameCard(this);
    setItemDelegate(m_gameCard);

    setViewMode(QListView::ListMode);
    setResizeMode(QListView::Fixed);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // ScrollBarAsNeeded per plan item 19
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);

    setSpacing(10);
    setWordWrap(true);
    setTextElideMode(Qt::ElideNone);
    setFlow(QListView::LeftToRight);
    setWrapping(true);

    setLayoutMode(QListView::SinglePass);
    setBatchSize(50);

    // Enable hover tracking for responsive highlights
    setMouseTracking(true);
    if (viewport()) {
        viewport()->setMouseTracking(true);
    }

    connect(&StormCatalogCache::Instance(), &StormCatalogCache::CacheLoaded, this, [this]() {
        if (viewport()) {
            viewport()->update();
        }
    }, Qt::QueuedConnection);
}

void GameGrid::SetModel(GameListModel* model) {
    QListView::setModel(model);
    UpdateIconSize();
}

void GameGrid::ApplyFilter(const QString& edit_filter_text, GameListModel* model) {
    if (!model) return;

    setUpdatesEnabled(false);
    const int row_count = model->rowCount();
    int visible_count = 0;
    int total_count = 0;

    for (int i = 0; i < row_count; ++i) {
        QStandardItem* item = model->item(i, 0);
        if (!item)
            continue;

        const auto type = item->data(GameListItem::TypeRole).value<GameListItemType>();
        if (type == GameListItemType::Game || !item->data(GameListItemPath::FullPathRole).toString().isEmpty()) {
            total_count++;
            if (Yuzu::FilterMatches(edit_filter_text, item)) {
                setRowHidden(i, false);
                visible_count++;
            } else {
                setRowHidden(i, true);
            }
        } else if (item->rowCount() > 0) {
            // Hierarchical directory item: count matching children
            bool folder_has_match = false;
            for (int j = 0; j < item->rowCount(); ++j) {
                auto* child = item->child(j, 0);
                if (!child) continue;
                total_count++;
                if (Yuzu::FilterMatches(edit_filter_text, child)) {
                    visible_count++;
                    folder_has_match = true;
                }
            }
            setRowHidden(i, !folder_has_match);
        } else {
            // Add directory or placeholder
            setRowHidden(i, true);
        }
    }
    setUpdatesEnabled(true);

    emit FilterResultReady(visible_count, total_count);
}

void GameGrid::UpdateIconSize() {
    const u32 icon_size = UISettings::values.game_icon_size.GetValue();

    int heightMargin = 20;
    int widthMargin = 24;

    if (UISettings::values.show_game_name) {
        switch (icon_size) {
        case 64:
            heightMargin = 50;
            widthMargin = 24;
            break;
        case 128:
            heightMargin = 58;
            widthMargin = 24;
            break;
        case 256:
        default:
            heightMargin = 64;
            widthMargin = 24;
            break;
        }
    }

    const int sb_width = style() ? style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this) : 14;
    const int view_width = std::max(100, this->width() - sb_width - 8);

    const int min_item_width = icon_size + widthMargin;
    const int columns = std::max(1, (view_width - 16) / min_item_width);
    const int stretched_width = std::max(min_item_width, (view_width - 16) / columns);

    QSize grid_size(stretched_width, icon_size + heightMargin);
    if (gridSize() != grid_size) {
        setUpdatesEnabled(false);
        setGridSize(grid_size);
        m_gameCard->setSize(grid_size, grid_size, 0, columns);
        setUpdatesEnabled(true);
    }
}

QModelIndex GameGrid::indexAt(const QPoint& point) const {
    QModelIndex index = QListView::indexAt(point);
    if (!index.isValid())
        return {};
    if (m_gameCard && !m_gameCard->hitTest(point, index, this, visualRect(index)))
        return {};
    return index;
}

void GameGrid::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        const auto current = currentIndex();
        if (current.isValid()) {
            emit activated(current);
            event->accept();
            return;
        }
    }
    QListView::keyPressEvent(event);
}

void GameGrid::resizeEvent(QResizeEvent* event) {
    QListView::resizeEvent(event);
    UpdateIconSize();
}
