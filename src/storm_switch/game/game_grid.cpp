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

    setLayoutMode(QListView::Batched);
    setBatchSize(50);

    // Enable hover tracking for responsive highlights
    setMouseTracking(true);
    if (viewport()) {
        viewport()->setMouseTracking(true);
    }
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

        total_count++;
        if (Yuzu::FilterMatches(edit_filter_text, item)) {
            setRowHidden(i, false);
            visible_count++;
        } else {
            setRowHidden(i, true);
        }
    }
    setUpdatesEnabled(true);

    emit FilterResultReady(visible_count, total_count);
}

void GameGrid::UpdateIconSize() {
    const u32 icon_size = UISettings::values.game_icon_size.GetValue();

    int heightMargin = 0;
    int widthMargin = 80;

    if (UISettings::values.show_game_name) {
        switch (icon_size) {
        case 128:
            heightMargin = 72;
            break;
        case 0:
            widthMargin = 120;
            heightMargin = 120;
            break;
        case 64:
            heightMargin = 82;
            break;
        case 32:
        case 256:
            heightMargin = 88;
            break;
        }
    } else {
        widthMargin = 24;
        heightMargin = 24;
    }

    const int view_width = viewport()->width();

    const double spacing = 0.01;
    const int min_item_width = icon_size + widthMargin;

    int columns = std::max(1, (view_width - 16) / min_item_width);
    int stretched_width = ((view_width) - (spacing * (columns - 1))) / columns;

    QSize grid_size(stretched_width, icon_size + heightMargin);
    if (gridSize() != grid_size) {
        setUpdatesEnabled(false);

        setGridSize(grid_size);
        m_gameCard->setSize(grid_size, grid_size, stretched_width - min_item_width, columns);

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
