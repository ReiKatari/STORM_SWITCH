// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <QApplication>
#include <QHeaderView>
#include <QResizeEvent>
#include <QScroller>
#include <QScrollerProperties>
#include <QShowEvent>

#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/model.h"
#include "storm_switch/game/common.h"
#include "storm_switch/game/game_tree.h"

GameTree::GameTree(QWidget* parent) : QTreeView{parent} {
    setAlternatingRowColors(true);
    setSelectionMode(QHeaderView::SingleSelection);
    setSelectionBehavior(QHeaderView::SelectRows);
    setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSortingEnabled(true);
    setEditTriggers(QHeaderView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setIndentation(16);
    header()->setDefaultAlignment(Qt::AlignCenter);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(true);

    connect(header(), &QHeaderView::sectionResized, this,
            [this](int logicalIndex, int /*oldSize*/, int /*newSize*/) {
                if (!m_adjusting_columns && QApplication::mouseButtons() != Qt::NoButton &&
                    logicalIndex == GameListModel::COLUMN_NAME) {
                    m_user_resized_columns = true;
                }
            });

    connect(this, &QTreeView::expanded, this, &GameTree::OnItemExpanded);
    connect(this, &QTreeView::collapsed, this, &GameTree::OnItemExpanded);
}

void GameTree::SetModel(GameListModel* model) {
    QTreeView::setModel(model);
    LoadInterfaceLayout();
    header()->setStretchLastSection(true);
    UpdateColumnVisibility(model);
    AdjustColumnsToViewport();
    sortByColumn(GameListModel::COLUMN_NAME, Qt::AscendingOrder);
}

void GameTree::OnItemExpanded(const QModelIndex& item) {
    const auto type = item.data(GameListItem::TypeRole).value<GameListItemType>();
    const bool is_dir = type == GameListItemType::CustomDir || type == GameListItemType::SdmcDir ||
                        type == GameListItemType::UserNandDir ||
                        type == GameListItemType::SysNandDir;
    const bool is_fave = type == GameListItemType::Favorites;
    if (!is_dir && !is_fave) {
        return;
    }
    const bool is_expanded = isExpanded(item);
    if (is_fave) {
        UISettings::values.favorites_expanded = is_expanded;
        return;
    }
    const int item_dir_index = item.data(GameListDir::GameDirRole).toInt();
    UISettings::values.game_dirs[item_dir_index].expanded = is_expanded;
}

void GameTree::SaveInterfaceLayout() {
    UISettings::values.gamelist_header_state = header()->saveState();
}

void GameTree::LoadInterfaceLayout() {
    m_adjusting_columns = true;
    auto* hdr = header();
    hdr->setDefaultAlignment(Qt::AlignCenter);
    hdr->setStretchLastSection(true);

    if (UISettings::values.gamelist_header_state.isEmpty() ||
        !hdr->restoreState(UISettings::values.gamelist_header_state)) {
        hdr->resizeSection(GameListModel::COLUMN_NAME, 1250);
        hdr->resizeSection(GameListModel::COLUMN_FILE_TYPE, 135);
        hdr->resizeSection(GameListModel::COLUMN_SIZE, 135);
        hdr->resizeSection(GameListModel::COLUMN_PLAY_TIME, 140);
        hdr->resizeSection(GameListModel::COLUMN_ADD_ONS, 200);
    } else {
        if (hdr->sectionSize(GameListModel::COLUMN_FILE_TYPE) < 110 ||
            hdr->sectionSize(GameListModel::COLUMN_FILE_TYPE) >= 195) {
            hdr->resizeSection(GameListModel::COLUMN_FILE_TYPE, 135);
        }
        if (hdr->sectionSize(GameListModel::COLUMN_SIZE) < 110 ||
            hdr->sectionSize(GameListModel::COLUMN_SIZE) >= 195) {
            hdr->resizeSection(GameListModel::COLUMN_SIZE, 135);
        }
        if (hdr->sectionSize(GameListModel::COLUMN_PLAY_TIME) < 110 ||
            hdr->sectionSize(GameListModel::COLUMN_PLAY_TIME) >= 195) {
            hdr->resizeSection(GameListModel::COLUMN_PLAY_TIME, 140);
        }
        if (hdr->sectionSize(GameListModel::COLUMN_ADD_ONS) < 160 ||
            hdr->sectionSize(GameListModel::COLUMN_ADD_ONS) > 240) {
            hdr->resizeSection(GameListModel::COLUMN_ADD_ONS, 200);
        }
    }
    hdr->setDefaultAlignment(Qt::AlignCenter);
    hdr->setStretchLastSection(true);
    m_adjusting_columns = false;
    AdjustColumnsToViewport();
}

void GameTree::AdjustColumnsToViewport() {
    if (m_adjusting_columns) {
        return;
    }
    const int vw = viewport() ? viewport()->width() : 0;
    if (vw <= 480) {
        return;
    }

    m_adjusting_columns = true;
    auto* hdr = header();
    const int w_type = isColumnHidden(GameListModel::COLUMN_FILE_TYPE)
                           ? 0
                           : hdr->sectionSize(GameListModel::COLUMN_FILE_TYPE);
    const int w_size = isColumnHidden(GameListModel::COLUMN_SIZE)
                           ? 0
                           : hdr->sectionSize(GameListModel::COLUMN_SIZE);
    const int w_time = isColumnHidden(GameListModel::COLUMN_PLAY_TIME)
                           ? 0
                           : hdr->sectionSize(GameListModel::COLUMN_PLAY_TIME);
    const int w_addons = isColumnHidden(GameListModel::COLUMN_ADD_ONS) ? 0 : 200;
    const int right_cols = w_type + w_size + w_time + w_addons;

    const int cur_name = hdr->sectionSize(GameListModel::COLUMN_NAME);
    const int target_name = std::max(360, vw - right_cols);

    // By default (until the user manually drags the column divider), or if the total width
    // overflows the viewport / leaves ADD_ONS overly stretched (>240px), fit COLUMN_NAME
    // so ADD_ONS sits right against the right edge with zero horizontal scrolling.
    if (!m_user_resized_columns || (cur_name + right_cols > vw) || cur_name <= 600 ||
        hdr->sectionSize(GameListModel::COLUMN_ADD_ONS) > 240) {
        if (!isColumnHidden(GameListModel::COLUMN_ADD_ONS)) {
            hdr->resizeSection(GameListModel::COLUMN_ADD_ONS, w_addons);
        }
        hdr->resizeSection(GameListModel::COLUMN_NAME, target_name);
    }
    hdr->setStretchLastSection(true);
    m_adjusting_columns = false;
}

void GameTree::resizeEvent(QResizeEvent* event) {
    QTreeView::resizeEvent(event);
    AdjustColumnsToViewport();
}

void GameTree::showEvent(QShowEvent* event) {
    QTreeView::showEvent(event);
    AdjustColumnsToViewport();
}

void GameTree::UpdateColumnVisibility(GameListModel* model) {
    Q_UNUSED(model)
    setColumnHidden(GameListModel::COLUMN_ADD_ONS, !UISettings::values.show_add_ons);
    setColumnHidden(GameListModel::COLUMN_FILE_TYPE, !UISettings::values.show_types);
    setColumnHidden(GameListModel::COLUMN_SIZE, !UISettings::values.show_size);
    setColumnHidden(GameListModel::COLUMN_PLAY_TIME, !UISettings::values.show_play_time);
    header()->setStretchLastSection(true);
    AdjustColumnsToViewport();
}

QString GameTree::GetLastFilterResultItem() const {
    QString file_path;

    auto* model = qobject_cast<GameListModel*>(QTreeView::model());
    if (!model)
        return {};

    for (int i = 1; i < model->rowCount() - 1; ++i) {
        const QStandardItem* folder = model->item(i, 0);
        const QModelIndex folder_index = folder->index();
        const int children_count = folder->rowCount();

        for (int j = 0; j < children_count; ++j) {
            if (isRowHidden(j, folder_index)) {
                continue;
            }

            const QStandardItem* child = folder->child(j, 0);
            file_path = child->data(GameListItemPath::FullPathRole).toString();
        }
    }

    return file_path;
}

int GameTree::FilterClosedResultCount(GameListModel* model) {
    int children_total = 0;

    auto hide_favorites_row = UISettings::values.favorited_ids.size() == 0;
    setRowHidden(0, model->invisibleRootItem()->index(), hide_favorites_row);

    for (int i = 1; i < model->rowCount() - 1; ++i) {
        auto* folder = model->item(i, 0);
        const QModelIndex folder_index = folder->index();
        const int children_count = folder->rowCount();
        for (int j = 0; j < children_count; ++j) {
            ++children_total;
            setRowHidden(j, folder_index, false);
        }
    }

    return children_total;
}

void GameTree::ApplyFilter(const QString& edit_filter_text, GameListModel* model) {
    int children_total = 0;
    int result_count = 0;

    if (edit_filter_text.isEmpty()) {
        children_total = FilterClosedResultCount(model);
        emit FilterResultReady(children_total, children_total);
        return;
    }

    setRowHidden(0, model->invisibleRootItem()->index(), true);

    for (int i = 1; i < model->rowCount() - 1; ++i) {
        auto* folder = model->item(i, 0);
        const QModelIndex folder_index = folder->index();
        const int children_count = folder->rowCount();

        for (int j = 0; j < children_count; ++j) {
            ++children_total;

            const QStandardItem* child = folder->child(j, 0);

            if (Yuzu::FilterMatches(edit_filter_text, child)) {
                setRowHidden(j, folder_index, false);
                ++result_count;
            } else {
                setRowHidden(j, folder_index, true);
            }
        }
    }

    emit FilterResultReady(result_count, children_total);
}
