// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QAbstractItemView>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QIcon>
#include <QPainterPath>
#include <cmath>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QScroller>
#include <QScrollerProperties>
#include <QToolButton>
#include <QVariantAnimation>
#include <qlayoutitem.h>

#include "common/cityhash.h"
#include "common/common_types.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "core/core.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/hle/service/game_fix_database.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/model.h"
#include "qt_common/qt_common.h"
#include "qt_common/util/game.h"
#include "storm_switch/game/carousel.h"
#include "storm_switch/game/game_grid.h"
#include "storm_switch/game/game_list.h"
#include "storm_switch/storm_catalog_cache.h"
#include "storm_switch/game/game_tree.h"
#include "storm_switch/game/search_field.h"
#include "storm_switch/main_window.h"
#include "storm_switch/util/controller_navigation.h"

namespace {

QModelIndex FindGameIndex(QStandardItemModel* model, const QString& target_path, u64 target_tid) {
    if (!model) return {};
    auto* root = model->invisibleRootItem();
    if (!root) return {};

    for (int i = 0; i < root->rowCount(); ++i) {
        auto* item = root->child(i, 0);
        if (!item) continue;
        if (item->data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
            u64 tid = item->data(GameListItemPath::ProgramIdRole).toULongLong();
            QString path = item->data(GameListItemPath::FullPathRole).toString();
            if ((!target_path.isEmpty() && path == target_path) || (target_tid != 0 && tid == target_tid)) {
                return item->index();
            }
        }
        for (int j = 0; j < item->rowCount(); ++j) {
            auto* child = item->child(j, 0);
            if (!child) continue;
            if (child->data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
                u64 tid = child->data(GameListItemPath::ProgramIdRole).toULongLong();
                QString path = child->data(GameListItemPath::FullPathRole).toString();
                if ((!target_path.isEmpty() && path == target_path) || (target_tid != 0 && tid == target_tid)) {
                    return child->index();
                }
            }
        }
    }
    return {};
}

QModelIndex FindFirstGameIndex(QStandardItemModel* model) {
    if (!model) return {};
    auto* root = model->invisibleRootItem();
    if (!root) return {};

    for (int i = 0; i < root->rowCount(); ++i) {
        auto* item = root->child(i, 0);
        if (!item) continue;
        if (item->data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
            return item->index();
        }
        for (int j = 0; j < item->rowCount(); ++j) {
            auto* child = item->child(j, 0);
            if (!child) continue;
            if (child->data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
                return child->index();
            }
        }
    }
    if (root->rowCount() > 0 && root->child(0, 0)) {
        return root->child(0, 0)->index();
    }
    return {};
}

} // namespace

GameList::GameList(FileSys::VirtualFilesystem vfs_, FileSys::ManualContentProvider* provider_,
                   PlayTime::PlayTimeManager& play_time_manager_, Core::System& system_,
                   MainWindow* parent)
    : QWidget{parent}, vfs{std::move(vfs_)}, provider{provider_},
      play_time_manager{play_time_manager_}, system{system_} {

    this->main_window = parent;
    layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    item_model = new GameListModel(vfs, provider, play_time_manager, system, this);

    SetupViews();

    search_field = new GameListSearchField(this);
    layout->addWidget(search_field);

    controller_navigation = new ControllerNavigation(system.HIDCore(), this);

    SetupScrollAnimation();

    connect(tree_view, &QTreeView::activated, this, &GameList::ValidateEntry);
    connect(tree_view, &QTreeView::customContextMenuRequested, this, &GameList::PopupContextMenu);

    connect(grid_view, &QListView::activated, this, &GameList::ValidateEntry);
    connect(grid_view, &QListView::customContextMenuRequested, this, &GameList::PopupContextMenu);

    connect(carousel_view, &QListView::activated, this, &GameList::ValidateEntry);
    connect(carousel_view, &QListView::customContextMenuRequested, this, &GameList::PopupContextMenu);

    auto connect_selection = [this](QAbstractItemView* view) {
        if (!view) return;
        connect(view, &QAbstractItemView::clicked, this, [this](const QModelIndex& index) {
            if (!index.isValid()) return;
            const auto selected = index.sibling(index.row(), 0);
            if (selected.data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
                u64 tid = selected.data(GameListItemPath::ProgramIdRole).toULongLong();
                QString path = selected.data(GameListItemPath::FullPathRole).toString();
                emit GameSelected(tid, path);
            }
        });
    };
    connect_selection(tree_view);
    connect_selection(grid_view);
    connect_selection(carousel_view);

    connect(controller_navigation, &ControllerNavigation::TriggerKeyboardEvent, this,
            [this](Qt::Key key) {
                if (system.IsPoweredOn()) {
                    return;
                }
                if (!this->isActiveWindow()) {
                    return;
                }
                QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
                QCoreApplication::postEvent(m_currentView, event);
            });

    connect(item_model, &GameListModel::PopulatingCompleted, this,
            &GameList::OnPopulatingCompleted);

    connect(item_model, &GameListModel::ShowList, this, &GameList::ShowList);
    connect(item_model, &GameListModel::SaveConfig, this, &GameList::SaveConfig);
    connect(item_model, &GameListModel::PopulatingStarted, this, &GameList::OnPopulate);

    connect(item_model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex& parent, int first, int last) {
                if (m_isTreeMode && tree_view) {
                    if (!parent.isValid()) {
                        for (int r = first; r <= last; ++r) {
                            const auto child_idx = item_model->index(r, 0, parent);
                            const int dir_index = child_idx.data(GameListDir::GameDirRole).toInt();
                            bool expand_dir = true;
                            if (dir_index >= 0 && dir_index < UISettings::values.game_dirs.size()) {
                                expand_dir = UISettings::values.game_dirs[dir_index].expanded;
                            }
                            if (expand_dir) {
                                tree_view->expand(child_idx);
                            }
                        }
                    } else {
                        const int dir_index = parent.data(GameListDir::GameDirRole).toInt();
                        bool expand_dir = true;
                        if (dir_index >= 0 && dir_index < UISettings::values.game_dirs.size()) {
                            expand_dir = UISettings::values.game_dirs[dir_index].expanded;
                        }
                        if (expand_dir && !tree_view->isExpanded(parent)) {
                            tree_view->expand(parent);
                        }
                    }
                }

                if (search_field && search_field->filterText().isEmpty()) {
                    int total = 0;
                    for (int i = 0; i < item_model->rowCount(); ++i) {
                        const auto* item = item_model->item(i, 0);
                        if (!item) continue;
                        if (item->rowCount() > 0) {
                            total += item->rowCount();
                        } else if (item->data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game ||
                                   !item->data(GameListItemPath::FullPathRole).toString().isEmpty()) {
                            total++;
                        }
                    }
                    if (total > 0) {
                        search_field->setFilterResult(total, total);
                    }
                }
            });

    connect(tree_view, &GameTree::FilterResultReady, search_field,
            [this](int visible, int total) { search_field->setFilterResult(visible, total); });
    connect(grid_view, &GameGrid::FilterResultReady, search_field,
            [this](int visible, int total) { search_field->setFilterResult(visible, total); });
    connect(search_field, &GameListSearchField::SortModeChanged, this, &GameList::OnSortChanged);
    connect(search_field, &GameListSearchField::ToggleViewModeRequested, this, &GameList::OnToggleViewMode);
    connect(item_model, &GameListModel::StatusMessage, this, &GameList::StatusMessage);
    connect(grid_view, &QListView::doubleClicked, this, &GameList::ValidateEntry);

    qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");

    ResetViewMode();
}

GameList::~GameList() {
    SaveInterfaceLayout();
    UnloadController();
}

void GameList::SetupViews() {
    tree_view = new GameTree(this);
    grid_view = new GameGrid(this);
    carousel_view = new GameCarousel(this);

    tree_view->SetModel(item_model);
    grid_view->SetModel(item_model);
    carousel_view->SetModel(item_model);

    connect(&StormCatalogCache::Instance(), &StormCatalogCache::CacheLoaded, this, [this]() {
        if (tree_view && tree_view->viewport()) {
            tree_view->viewport()->update();
        }
        if (grid_view && grid_view->viewport()) {
            grid_view->viewport()->update();
        }
        if (carousel_view && carousel_view->viewport()) {
            carousel_view->viewport()->update();
        }
    }, Qt::QueuedConnection);

    auto connect_selection = [this](QAbstractItemView* view) {
        if (!view) return;
        if (auto* sm = view->selectionModel()) {
            connect(sm, &QItemSelectionModel::selectionChanged, this, [this]() {
                auto [tid, path] = GetSelectedGameInfo();
                if (!path.isEmpty() || tid != 0) {
                    QSettings s;
                    s.setValue(QStringLiteral("gamelist/last_selected_path"), path);
                    s.setValue(QStringLiteral("gamelist/last_selected_tid"), static_cast<qulonglong>(tid));
                }
            });
        }
    };
    connect_selection(tree_view);
    connect_selection(grid_view);
    connect_selection(carousel_view);
}

QString GameList::GetLastFilterResultItem() const {
    return tree_view->GetLastFilterResultItem();
}

void GameList::ClearFilter() {
    search_field->clear();
}

void GameList::SetFilterFocus() {
    if (item_model->rowCount() > 0) {
        search_field->setFocus();
    }
}

void GameList::SetFilterVisible(bool visibility) {
    search_field->setVisible(visibility);
}

bool GameList::IsEmpty() const {
    return item_model->IsEmpty();
}

void GameList::OnPopulate() {
    if (m_currentView) {
        m_currentView->setEnabled(true);
    }

    switch (game_list_mode) {
    case Settings::GameListMode::TreeView:
        tree_view->UpdateColumnVisibility(item_model);
        break;
    case Settings::GameListMode::GridView:
        grid_view->UpdateIconSize();
        break;
    case Settings::GameListMode::CarouselView:
        carousel_view->UpdateIconSize();
        break;
    }
}

void GameList::PopulateAsync(QVector<UISettings::GameDir>& game_dirs) {
    if (tree_view) {
        tree_view->setSortingEnabled(false);
    }
    item_model->PopulateAsync(game_dirs);
}

void GameList::CancelPopulate() {
    if (item_model) {
        item_model->CancelPopulate();
    }
}

void GameList::SaveInterfaceLayout() {
    tree_view->SaveInterfaceLayout();

    // Law 21: Persist selection and scroll position
    QSettings settings;
    auto [tid, path] = GetSelectedGameInfo();
    if (!path.isEmpty() || tid != 0) {
        settings.setValue(QStringLiteral("gamelist/last_selected_path"), path);
        settings.setValue(QStringLiteral("gamelist/last_selected_tid"), static_cast<qulonglong>(tid));
    }
    if (tree_view && tree_view->verticalScrollBar()) {
        settings.setValue(QStringLiteral("gamelist/last_scroll_pos"), tree_view->verticalScrollBar()->value());
    }
}

void GameList::LoadInterfaceLayout() {
    tree_view->LoadInterfaceLayout();
}

QStandardItemModel* GameList::GetModel() const {
    return item_model;
}

void GameList::UnloadController() {
    controller_navigation->UnloadController();
}

void GameList::ResetViewMode() {
    auto mode = UISettings::values.game_list_mode.GetValue();
    if (mode == Settings::GameListMode::CarouselView) {
        mode = Settings::GameListMode::TreeView;
        UISettings::values.game_list_mode.SetValue(mode);
    }
    game_list_mode = mode;

    const bool was_initialized = (m_currentView != nullptr);
    if (m_currentView)
        layout->removeWidget(m_currentView);

    bool newTreeMode = false;

    switch (mode) {
    case Settings::GameListMode::TreeView:
        m_currentView = tree_view;
        newTreeMode = true;

        break;
    case Settings::GameListMode::GridView:
        m_currentView = grid_view;
        newTreeMode = false;

        break;
    case Settings::GameListMode::CarouselView:
        m_currentView = carousel_view;
        newTreeMode = false;

        break;
    default:
        m_currentView = tree_view;
        newTreeMode = true;
        break;
    }

    tree_view->setVisible(false);
    grid_view->setVisible(false);
    carousel_view->setVisible(false);

    tree_view->setEnabled(false);
    grid_view->setEnabled(false);
    carousel_view->setEnabled(false);

    m_currentView->setVisible(true);
    m_currentView->setEnabled(true);
    layout->insertWidget(0, m_currentView);
    if (search_field) {
        search_field->UpdateViewMode(mode == Settings::GameListMode::GridView);
    }

    auto view = m_currentView->viewport();
    view->installEventFilter(this);

    view->grabGesture(Qt::SwipeGesture);
    view->grabGesture(Qt::PanGesture);

    QScroller::grabGesture(view, QScroller::LeftMouseButtonGesture);

    auto scroller = QScroller::scroller(view);
    QScrollerProperties props;
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                          QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                          QScrollerProperties::OvershootAlwaysOff);
    scroller->setScrollerProperties(props);

    if (m_isTreeMode != newTreeMode) {
        m_isTreeMode = newTreeMode;
        item_model->SetFlat(!m_isTreeMode);
        if (was_initialized && !UISettings::values.game_dirs.empty()) {
            item_model->PopulateAsync(UISettings::values.game_dirs);
        }
    }
}

void GameList::OnSortChanged(int mode) {
    if (item_model) {
        item_model->SortGames(mode);
    }
}

void GameList::OnToggleViewMode() {
    auto current_mode = UISettings::values.game_list_mode.GetValue();
    if (current_mode == Settings::GameListMode::GridView) {
        current_mode = Settings::GameListMode::TreeView;
    } else {
        current_mode = Settings::GameListMode::GridView;
    }
    UISettings::values.game_list_mode.SetValue(current_mode);
    ResetViewMode();
    if (search_field) {
        search_field->UpdateViewMode(current_mode == Settings::GameListMode::GridView);
    }
}

void GameList::OnTextChanged(const QString& new_text) {
    const QString edit_filter_text = new_text.toLower();

    if (m_isTreeMode) {
        tree_view->ApplyFilter(edit_filter_text, item_model);
    } else {
        grid_view->ApplyFilter(edit_filter_text, item_model);
    }
}

void GameList::OnFilterCloseClicked() {
    main_window->filterBarSetChecked(false);
}

void GameList::OnPopulatingCompleted(const QStringList& watch_list) {
    if (m_isTreeMode && tree_view) {
        tree_view->setSortingEnabled(true);
    }
    emit ShowList(!item_model->IsEmpty());

    if (!m_isTreeMode && grid_view) {
        grid_view->ApplyFilter(search_field ? search_field->filterText() : QString{}, item_model);
    }

    // favorites row
    if (m_isTreeMode) {
        tree_view->setRowHidden(0, item_model->invisibleRootItem()->index(),
                                UISettings::values.favorited_ids.size() == 0);
        tree_view->setExpanded(item_model->invisibleRootItem()->child(0)->index(),
                               UISettings::values.favorites_expanded.GetValue());
    }

    // restore collapsed/expanded flags
    if (m_isTreeMode) {
        const auto* root = item_model->invisibleRootItem();
        for (int i = 1; i < root->rowCount() - 1; ++i) {
            const auto* dir_item = root->child(i);
            if (!dir_item) continue;
            const auto type = dir_item->data(GameListItem::TypeRole).value<GameListItemType>();
            if (type == GameListItemType::CustomDir || type == GameListItemType::SdmcDir ||
                type == GameListItemType::UserNandDir || type == GameListItemType::SysNandDir) {
                const int dir_index = dir_item->data(GameListDir::GameDirRole).toInt();
                bool expand_dir = true;
                if (dir_index >= 0 && dir_index < UISettings::values.game_dirs.size()) {
                    expand_dir = UISettings::values.game_dirs[dir_index].expanded;
                }
                tree_view->setExpanded(dir_item->index(), expand_dir);
                if (expand_dir) {
                    tree_view->expand(dir_item->index());
                }
            }
        }
    }

    // Watcher updates
    auto* watcher = item_model->GetWatcher();
    if (watcher) {
        const QSignalBlocker blocker(watcher);
        const auto current_watch_list = watcher->directories();

        constexpr qsizetype LIMIT_WATCH_DIRECTORIES = 5000;
        QStringList to_remove, to_add;

        // remove any paths not in the new watch list
        for (const auto& path : std::as_const(current_watch_list)) {
            if (!watch_list.contains(path)) {
                to_remove.emplaceBack(path);
            }
        }
        if (!to_remove.isEmpty()) {
            watcher->removePaths(to_remove);
        }

        // add any paths not in the old watch list
        for (const auto& path : std::as_const(watch_list)) {
            if (!current_watch_list.contains(path)) {
                to_add.emplaceBack(path);
            }
        }
        if (!to_add.isEmpty()) {
            const qsizetype len = (std::min)(to_add.size(), LIMIT_WATCH_DIRECTORIES);
            watcher->addPaths(to_add.mid(0, len));
        }
    }

    m_currentView->setEnabled(true);

    int children_total = 0;
    for (int i = 1; i < item_model->rowCount() - 1; ++i) {
        children_total += item_model->item(i, 0)->rowCount();
    }

    search_field->setFilterResult(children_total, children_total);
    if (children_total > 0) {
        search_field->setFocus();
    }

    // TODO: carousel/grid impl.
    item_model->sort(tree_view->header()->sortIndicatorSection(),
                     tree_view->header()->sortIndicatorOrder());

    // Law 21: Scroll & Selection Persistence + 1st-Game Auto-Select in 1st Second
    QSettings settings;
    QString last_path = settings.value(QStringLiteral("gamelist/last_selected_path")).toString();
    u64 last_tid = settings.value(QStringLiteral("gamelist/last_selected_tid"), 0).toULongLong();
    int last_scroll = settings.value(QStringLiteral("gamelist/last_scroll_pos"), 0).toInt();

    QModelIndex target_idx = FindGameIndex(item_model, last_path, last_tid);
    if (!target_idx.isValid()) {
        target_idx = FindFirstGameIndex(item_model);
    }

    if (target_idx.isValid() && m_currentView && m_currentView->selectionModel()) {
        m_currentView->selectionModel()->setCurrentIndex(
            target_idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_currentView->setCurrentIndex(target_idx);
        m_currentView->scrollTo(target_idx, QAbstractItemView::EnsureVisible);
        const auto sel = target_idx.sibling(target_idx.row(), 0);
        if (sel.data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
            u64 tid = sel.data(GameListItemPath::ProgramIdRole).toULongLong();
            QString path = sel.data(GameListItemPath::FullPathRole).toString();
            emit GameSelected(tid, path);
        }
    }

    if (tree_view && tree_view->verticalScrollBar() && last_scroll > 0) {
        tree_view->verticalScrollBar()->setValue(last_scroll);
    }

    emit PopulatingCompleted();
}

void GameList::RefreshGameDirectory() {
    item_model->RefreshGameDirectory();
}

void GameList::RefreshExternalContent() {
    item_model->RefreshExternalContent();
}

void GameList::UpdateIconSizes() {
    switch (game_list_mode) {
    case Settings::GameListMode::GridView:
        grid_view->UpdateIconSize();
        break;
    case Settings::GameListMode::CarouselView:
        carousel_view->UpdateIconSize();
        break;
    case Settings::GameListMode::TreeView:
        break;
    }
}

void GameList::ValidateEntry(const QModelIndex& item) {
    if (system.IsPoweredOn()) {
        return;
    }
    const auto selected = item.sibling(item.row(), 0);

    switch (selected.data(GameListItem::TypeRole).value<GameListItemType>()) {
    case GameListItemType::Game: {
        const QString file_path = selected.data(GameListItemPath::FullPathRole).toString();
        if (file_path.isEmpty())
            return;
        const QFileInfo file_info(file_path);
        if (!file_info.exists())
            return;

        if (file_info.isDir()) {
            const QDir dir{file_path};
            const QStringList matching_main = dir.entryList({QStringLiteral("main")}, QDir::Files);
            if (matching_main.size() == 1) {
                emit GameChosen(dir.path() + QDir::separator() + matching_main[0]);
            }
            return;
        }

        const auto title_id = selected.data(GameListItemPath::ProgramIdRole).toULongLong();

        search_field->clear();
        emit GameChosen(file_path, title_id);
        break;
    }
    case GameListItemType::AddDir:
        emit AddDirectory();
        break;
    default:
        break;
    }
}

void GameList::ToggleFavorite(u64 program_id) {
    item_model->ToggleFavorite(program_id);

    if (UISettings::values.favorited_ids.contains(program_id)) {
        tree_view->setRowHidden(0, item_model->invisibleRootItem()->index(),
                                !search_field->filterText().isEmpty());
        item_model->sort(tree_view->header()->sortIndicatorSection(),
                         tree_view->header()->sortIndicatorOrder());
    } else {
        if (UISettings::values.favorited_ids.size() == 0) {
            tree_view->setRowHidden(0, item_model->invisibleRootItem()->index(), true);
        }
    }
}

void GameList::PopupContextMenu(const QPoint& menu_location) {
    const auto gl_lang = [](const QString& ru, const QString& en, const QString& zh,
                            const QString& ja, const QString& ko, const QString& de,
                            const QString& fr, const QString& es, const QString& ar) -> QString {
        std::string lang = UISettings::values.language.GetValue();
        if (lang.empty()) {
            lang = QLocale::system().name().toStdString();
        }
        if (lang.rfind("ru", 0) == 0) return ru;
        if (lang.rfind("zh", 0) == 0) return zh;
        if (lang.rfind("ja", 0) == 0) return ja;
        if (lang.rfind("ko", 0) == 0) return ko;
        if (lang.rfind("de", 0) == 0) return de;
        if (lang.rfind("fr", 0) == 0) return fr;
        if (lang.rfind("es", 0) == 0) return es;
        if (lang.rfind("ar", 0) == 0) return ar;
        return en;
    };

    QModelIndex item = m_currentView->indexAt(menu_location);
    if (!item.isValid()) {
        if (m_isTreeMode)
            return;

        QMenu blank_menu;
        std::string lang = UISettings::values.language.GetValue();
        if (lang.empty()) {
            lang = QLocale::system().name().toStdString();
        }
        const QString add_dir_text = (lang.rfind("ru", 0) == 0)
                                         ? QStringLiteral("Добавить папку с играми...")
                                         : tr("&Add New Game Directory");
        QAction* addGameDirAction = blank_menu.addAction(add_dir_text);

        connect(addGameDirAction, &QAction::triggered, this, &GameList::AddDirectory);
        blank_menu.exec(m_currentView->viewport()->mapToGlobal(menu_location));
        return;
    }

    const auto selected = item.sibling(item.row(), 0);
    if (m_currentView && m_currentView->selectionModel()) {
        m_currentView->selectionModel()->setCurrentIndex(selected, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    QMenu context_menu;
    switch (selected.data(GameListItem::TypeRole).value<GameListItemType>()) {
    case GameListItemType::Game: {
        const u64 pid = selected.data(GameListItemPath::ProgramIdRole).toULongLong();
        const QString gpath = selected.data(GameListItemPath::FullPathRole).toString();
        emit GameSelected(pid, gpath);
        AddGamePopup(context_menu, pid, gpath.toStdString());
        break;
    }
    case GameListItemType::CustomDir:
        AddPermDirPopup(context_menu, selected);
        AddCustomDirPopup(context_menu, selected);
        break;
    case GameListItemType::SdmcDir:
    case GameListItemType::UserNandDir:
    case GameListItemType::SysNandDir:
        AddPermDirPopup(context_menu, selected);
        break;
    case GameListItemType::Favorites:
        AddFavoritesPopup(context_menu);
        break;
    default:
        break;
    }
    context_menu.exec(m_currentView->viewport()->mapToGlobal(menu_location));
}

void GameList::AddGamePopup(QMenu& context_menu, u64 program_id, const std::string& path) {
    std::string cur_lang = UISettings::values.language.GetValue();
    if (cur_lang.empty()) {
        cur_lang = QLocale::system().name().toStdString();
    }
    const bool is_ru = (cur_lang.rfind("ru", 0) == 0);
    const auto item_tr = [&](const QString& prefix, const char* en_source, const QString& ru_text) -> QString {
        if (is_ru) {
            return prefix + ru_text;
        }
        return prefix + tr(en_source);
    };
    const auto storm_tr = [&](const QString& ru, const QString& en, const QString& zh,
                              const QString& ja, const QString& ko, const QString& de,
                              const QString& fr, const QString& es, const QString& ar) -> QString {
        if (is_ru) return ru;
        if (cur_lang.rfind("zh", 0) == 0) return zh;
        if (cur_lang.rfind("ja", 0) == 0) return ja;
        if (cur_lang.rfind("ko", 0) == 0) return ko;
        if (cur_lang.rfind("de", 0) == 0) return de;
        if (cur_lang.rfind("fr", 0) == 0) return fr;
        if (cur_lang.rfind("es", 0) == 0) return es;
        if (cur_lang.rfind("ar", 0) == 0) return ar;
        if (cur_lang.rfind("pt", 0) == 0) return en;
        return en;
    };

    enum class ContextVectorIcon {
        Star,
        Play,
        Fast,
        Save,
        Sync,
        Puzzle,
        Bolt,
        Link,
        Trash,
        Clock,
        Package,
        Search,
        Copy,
        Sparkle,
        Shield,
    };

    const auto make_vector_icon = [](ContextVectorIcon type) -> QIcon {
        QPixmap pixmap(18, 18);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        const QColor accent(0, 240, 255);
        QPen pen(accent, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        switch (type) {
        case ContextVectorIcon::Star: {
            QPainterPath path;
            const qreal cx = 9.0, cy = 9.0;
            const qreal r_out = 6.5, r_in = 2.8;
            for (int i = 0; i < 10; ++i) {
                const qreal angle = -1.57079632679 + i * (3.14159265359 / 5.0);
                const qreal r = (i % 2 == 0) ? r_out : r_in;
                const qreal x = cx + r * std::cos(angle);
                const qreal y = cy + r * std::sin(angle);
                if (i == 0) path.moveTo(x, y);
                else path.lineTo(x, y);
            }
            path.closeSubpath();
            painter.setBrush(QColor(0, 240, 255, 60));
            painter.drawPath(path);
            break;
        }
        case ContextVectorIcon::Play: {
            QPainterPath path;
            path.moveTo(6.0, 4.0);
            path.lineTo(13.5, 9.0);
            path.lineTo(6.0, 14.0);
            path.closeSubpath();
            painter.setBrush(QColor(0, 240, 255, 120));
            painter.drawPath(path);
            break;
        }
        case ContextVectorIcon::Fast: {
            QPainterPath p1, p2;
            p1.moveTo(4.5, 5.0); p1.lineTo(9.5, 9.0); p1.lineTo(4.5, 13.0); p1.closeSubpath();
            p2.moveTo(9.5, 5.0); p2.lineTo(14.5, 9.0); p2.lineTo(9.5, 13.0); p2.closeSubpath();
            painter.setBrush(QColor(0, 240, 255, 120));
            painter.drawPath(p1);
            painter.drawPath(p2);
            break;
        }
        case ContextVectorIcon::Save: {
            painter.drawRoundedRect(QRectF(3.5, 3.5, 11.0, 11.0), 1.5, 1.5);
            painter.fillRect(QRectF(5.5, 3.5, 7.0, 3.5), accent);
            painter.fillRect(QRectF(6.0, 9.5, 6.0, 4.5), QColor(0, 240, 255, 80));
            break;
        }
        case ContextVectorIcon::Sync: {
            painter.drawArc(QRectF(3.5, 3.5, 11.0, 11.0), 45 * 16, 270 * 16);
            QPainterPath arrow;
            arrow.moveTo(13.0, 6.0);
            arrow.lineTo(15.5, 8.5);
            arrow.lineTo(11.5, 9.5);
            arrow.closeSubpath();
            painter.setBrush(accent);
            painter.drawPath(arrow);
            break;
        }
        case ContextVectorIcon::Puzzle: {
            painter.drawRoundedRect(QRectF(4.0, 4.0, 10.0, 10.0), 2.0, 2.0);
            painter.drawEllipse(QPointF(9.0, 4.0), 2.0, 2.0);
            painter.drawEllipse(QPointF(14.0, 9.0), 2.0, 2.0);
            break;
        }
        case ContextVectorIcon::Bolt: {
            QPainterPath path;
            path.moveTo(10.0, 3.0);
            path.lineTo(5.5, 9.5);
            path.lineTo(9.0, 9.5);
            path.lineTo(8.0, 15.0);
            path.lineTo(13.0, 8.0);
            path.lineTo(9.8, 8.0);
            path.closeSubpath();
            painter.setBrush(QColor(0, 240, 255, 120));
            painter.drawPath(path);
            break;
        }
        case ContextVectorIcon::Link: {
            painter.drawRoundedRect(QRectF(3.5, 6.5, 6.5, 5.0), 2.5, 2.5);
            painter.drawRoundedRect(QRectF(8.0, 6.5, 6.5, 5.0), 2.5, 2.5);
            painter.drawLine(QPointF(7.0, 9.0), QPointF(11.0, 9.0));
            break;
        }
        case ContextVectorIcon::Trash: {
            painter.drawLine(QPointF(4.0, 5.5), QPointF(14.0, 5.5));
            painter.drawRoundedRect(QRectF(5.0, 5.5, 8.0, 9.0), 1.0, 1.0);
            painter.drawLine(QPointF(7.5, 7.5), QPointF(7.5, 12.5));
            painter.drawLine(QPointF(10.5, 7.5), QPointF(10.5, 12.5));
            painter.drawArc(QRectF(7.0, 3.0, 4.0, 3.0), 0, 180 * 16);
            break;
        }
        case ContextVectorIcon::Clock: {
            painter.drawEllipse(QPointF(9.0, 9.0), 5.5, 5.5);
            painter.drawLine(QPointF(9.0, 9.0), QPointF(9.0, 5.5));
            painter.drawLine(QPointF(9.0, 9.0), QPointF(12.0, 9.0));
            break;
        }
        case ContextVectorIcon::Package: {
            painter.drawRect(QRectF(4.0, 4.0, 10.0, 10.0));
            painter.drawLine(QPointF(4.0, 9.0), QPointF(14.0, 9.0));
            painter.drawLine(QPointF(9.0, 4.0), QPointF(9.0, 14.0));
            break;
        }
        case ContextVectorIcon::Search: {
            painter.drawEllipse(QPointF(7.5, 7.5), 4.0, 4.0);
            painter.drawLine(QPointF(10.5, 10.5), QPointF(14.5, 14.5));
            break;
        }
        case ContextVectorIcon::Copy: {
            painter.drawRoundedRect(QRectF(6.0, 3.5, 8.0, 9.5), 1.0, 1.0);
            painter.drawRoundedRect(QRectF(4.0, 5.5, 8.0, 9.5), 1.0, 1.0);
            break;
        }
        case ContextVectorIcon::Sparkle: {
            QPainterPath path;
            const qreal cx = 9.0, cy = 9.0;
            path.moveTo(cx, cy - 6.5);
            path.quadTo(cx, cy, cx + 6.5, cy);
            path.quadTo(cx, cy, cx, cy + 6.5);
            path.quadTo(cx, cy, cx - 6.5, cy);
            path.quadTo(cx, cy, cx, cy - 6.5);
            painter.setBrush(QColor(0, 240, 255, 120));
            painter.drawPath(path);
            break;
        }
        case ContextVectorIcon::Shield: {
            QPainterPath path;
            path.moveTo(9.0, 3.5);
            path.lineTo(14.0, 5.5);
            path.lineTo(14.0, 9.5);
            path.quadTo(14.0, 14.0, 9.0, 15.5);
            path.quadTo(4.0, 14.0, 4.0, 9.5);
            path.lineTo(4.0, 5.5);
            path.closeSubpath();
            painter.setBrush(QColor(0, 240, 255, 70));
            painter.drawPath(path);
            break;
        }
        }

        return QIcon(pixmap);
    };

    QAction* properties = context_menu.addAction(item_tr(QString(), "Configure Game", QStringLiteral("Настройки игры...")));
    {
        QPixmap pixmap(18, 18);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(0, 240, 255), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawEllipse(QPointF(9, 9), 5.5, 5.5);
        painter.drawEllipse(QPointF(9, 9), 2.5, 2.5);
        for (int i = 0; i < 8; ++i) {
            qreal a = i * 3.14159265 / 4.0;
            painter.drawLine(QPointF(9 + 5.5 * cos(a), 9 + 5.5 * sin(a)),
                             QPointF(9 + 8.0 * cos(a), 9 + 8.0 * sin(a)));
        }
        properties->setIcon(QIcon(pixmap));
    }
    const bool is_fav = UISettings::values.favorited_ids.contains(program_id);
    QAction* favorite = context_menu.addAction(
        is_fav ? item_tr(QString(), "Favorite", QStringLiteral("В избранном (убрать)"))
               : item_tr(QString(), "Favorite", QStringLiteral("Добавить в избранное")));
    favorite->setIcon(make_vector_icon(ContextVectorIcon::Star));
    context_menu.addSeparator();
    QAction* start_game = context_menu.addAction(item_tr(QString(), "Start Game", QStringLiteral("Запустить игру")));
    start_game->setIcon(make_vector_icon(ContextVectorIcon::Play));
    QAction* start_game_global =
        context_menu.addAction(item_tr(QString(), "Start Game without Custom Configuration", QStringLiteral("Запустить со стандартными настройками")));
    start_game_global->setIcon(make_vector_icon(ContextVectorIcon::Fast));
    context_menu.addSeparator();
    QAction* open_save_location = context_menu.addAction(item_tr(QString(), "Open Save Data Location", QStringLiteral("Открыть папку сохранений")));
    open_save_location->setIcon(make_vector_icon(ContextVectorIcon::Save));
    QAction* sync_save = context_menu.addAction(storm_tr(
        QStringLiteral("Синхронизация сохранения (STORM SAVE SYNC)..."),
        QStringLiteral("Save Data Sync (STORM SAVE SYNC)..."),
        QStringLiteral("存档同步 (STORM SAVE SYNC)..."),
        QStringLiteral("セーブデータ同期 (STORM SAVE SYNC)..."),
        QStringLiteral("세이브 데이터 동기화 (STORM SAVE SYNC)..."),
        QStringLiteral("Speicherdaten-Synchronisation (STORM SAVE SYNC)..."),
        QStringLiteral("Synchronisation des sauvegardes (STORM SAVE SYNC)..."),
        QStringLiteral("Sincronización de partidas (STORM SAVE SYNC)..."),
        QStringLiteral("مزامنة بيانات الحفظ (STORM SAVE SYNC)...")));
    sync_save->setIcon(make_vector_icon(ContextVectorIcon::Sync));
    QAction* open_mod_location = context_menu.addAction(item_tr(QString(), "Open Mod Data Location", QStringLiteral("Открыть папку модов")));
    open_mod_location->setIcon(make_vector_icon(ContextVectorIcon::Puzzle));
    QAction* open_transferable_shader_cache =
        context_menu.addAction(item_tr(QString(), "Open Transferable Pipeline Cache", QStringLiteral("Открыть кэш шейдеров")));
    open_transferable_shader_cache->setIcon(make_vector_icon(ContextVectorIcon::Bolt));
    QAction* ryujinx = context_menu.addAction(item_tr(QString(), "Link to Ryujinx", QStringLiteral("Связать с Ryujinx")));
    ryujinx->setIcon(make_vector_icon(ContextVectorIcon::Link));
    context_menu.addSeparator();
    QMenu* remove_menu = context_menu.addMenu(item_tr(QString(), "Remove", QStringLiteral("Удалить")));
    remove_menu->setIcon(make_vector_icon(ContextVectorIcon::Trash));
    QAction* remove_update = remove_menu->addAction(item_tr(QString(), "Remove Installed Update", QStringLiteral("Удалить установленное обновление")));
    QAction* remove_dlc = remove_menu->addAction(item_tr(QString(), "Remove All Installed DLC", QStringLiteral("Удалить все установленные DLC")));
    QAction* remove_custom_config = remove_menu->addAction(item_tr(QString(), "Remove Custom Configuration", QStringLiteral("Удалить индивидуальные настройки")));
    QAction* remove_cache_storage = remove_menu->addAction(item_tr(QString(), "Remove Cache Storage", QStringLiteral("Очистить хранилище кэша")));
    QAction* remove_gl_shader_cache = remove_menu->addAction(item_tr(QString(), "Remove OpenGL Pipeline Cache", QStringLiteral("Удалить кэш шейдеров OpenGL")));
    QAction* remove_vk_shader_cache = remove_menu->addAction(item_tr(QString(), "Remove Vulkan Pipeline Cache", QStringLiteral("Удалить кэш шейдеров Vulkan")));
    remove_menu->addSeparator();
    QAction* remove_shader_cache = remove_menu->addAction(item_tr(QString(), "Remove All Pipeline Caches", QStringLiteral("Удалить все кэши шейдеров")));
    QAction* remove_all_content = remove_menu->addAction(item_tr(QString(), "Remove All Installed Contents", QStringLiteral("Удалить весь установленный контент")));
    QMenu* play_time_menu = context_menu.addMenu(item_tr(QString(), "Manage Play Time", QStringLiteral("Управление временем игры")));
    play_time_menu->setIcon(make_vector_icon(ContextVectorIcon::Clock));
    QAction* set_play_time = play_time_menu->addAction(item_tr(QString(), "Edit Play Time Data", QStringLiteral("Изменить время игры...")));
    QAction* remove_play_time_data = play_time_menu->addAction(item_tr(QString(), "Remove Play Time Data", QStringLiteral("Сбросить время игры")));
    QMenu* dump_romfs_menu = context_menu.addMenu(item_tr(QString(), "Dump RomFS", QStringLiteral("Дамп RomFS")));
    dump_romfs_menu->setIcon(make_vector_icon(ContextVectorIcon::Package));
    QAction* dump_romfs = dump_romfs_menu->addAction(item_tr(QString(), "Dump RomFS", QStringLiteral("Дамп RomFS")));
    QAction* dump_romfs_sdmc = dump_romfs_menu->addAction(item_tr(QString(), "Dump RomFS to SDMC", QStringLiteral("Дамп RomFS на SD-карту")));
    QAction* verify_integrity = context_menu.addAction(item_tr(QString(), "Verify Integrity", QStringLiteral("Проверить целостность")));
    verify_integrity->setIcon(make_vector_icon(ContextVectorIcon::Search));
    QAction* copy_tid = context_menu.addAction(item_tr(QString(), "Copy Title ID to Clipboard", QStringLiteral("Скопировать Title ID в буфер обмена")));
    copy_tid->setIcon(make_vector_icon(ContextVectorIcon::Copy));
#if !defined(__APPLE__)
    QMenu* shortcut_menu = context_menu.addMenu(item_tr(QString(), "Create Shortcut", QStringLiteral("Создать ярлык")));
    shortcut_menu->setIcon(make_vector_icon(ContextVectorIcon::Link));
    QAction* create_desktop_shortcut = shortcut_menu->addAction(item_tr(QString(), "Add to Desktop", QStringLiteral("На рабочий стол")));
    QAction* create_applications_menu_shortcut =
        shortcut_menu->addAction(item_tr(QString(), "Add to Applications Menu", QStringLiteral("В меню приложений")));
#endif
    context_menu.addSeparator();
    QAction* mod_manager_action = context_menu.addAction(storm_tr(
        QStringLiteral("Менеджер модов..."),
        QStringLiteral("Mod Manager..."),
        QStringLiteral("模组管理器..."),
        QStringLiteral("Mod マネージャー..."),
        QStringLiteral("모드 매니저..."),
        QStringLiteral("Mod-Manager..."),
        QStringLiteral("Gestionnaire de mods..."),
        QStringLiteral("Gestor de mods..."),
        QStringLiteral("مدير التعديلات...")));
    mod_manager_action->setIcon(make_vector_icon(ContextVectorIcon::Puzzle));
    QAction* cheats_action = context_menu.addAction(storm_tr(
        QStringLiteral("Чит-коды..."),
        QStringLiteral("Cheats..."),
        QStringLiteral("金手指 / 作弊码..."),
        QStringLiteral("チートコード..."),
        QStringLiteral("치트 코드..."),
        QStringLiteral("Cheats..."),
        QStringLiteral("Codes de triche..."),
        QStringLiteral("Trucos..."),
        QStringLiteral("رموز الغش...")));
    cheats_action->setIcon(make_vector_icon(ContextVectorIcon::Sparkle));
    QAction* game_fix_action = nullptr;
    if (Core::GameFixDatabase::GetProfileByTitleOrPath(program_id, path) != nullptr) {
        game_fix_action = context_menu.addAction(storm_tr(
            QStringLiteral("Авто-исправление игры..."),
            QStringLiteral("Game Auto-Fix..."),
            QStringLiteral("游戏自动修复..."),
            QStringLiteral("ゲーム自動修復..."),
            QStringLiteral("게임 자동 수정..."),
            QStringLiteral("Spiel-Auto-Korrektur..."),
            QStringLiteral("Correction automatique du jeu..."),
            QStringLiteral("Corrección automática del juego..."),
            QStringLiteral("الإصلاح التلقائي للعبة...")));
        game_fix_action->setIcon(make_vector_icon(ContextVectorIcon::Shield));
    }
    // properties action moved to top of context menu per Item 21
    QAction* reset_game_settings = context_menu.addAction(storm_tr(
        QStringLiteral("Сбросить настройки игры на стандартные"),
        QStringLiteral("Reset Per-Game Settings to Defaults"),
        QStringLiteral("重置游戏独立设置为默认值"),
        QStringLiteral("ゲーム個別設定をデフォルトにリセット"),
        QStringLiteral("게임별 설정을 기본값으로 초기화"),
        QStringLiteral("Spielspezifische Einstellungen zurücksetzen"),
        QStringLiteral("Réinitialiser les paramètres du jeu par défaut"),
        QStringLiteral("Restablecer configuración del juego a valores predeterminados"),
        QStringLiteral("إعادة تعيين إعدادات اللعبة إلى الافتراضية")));
    reset_game_settings->setIcon(make_vector_icon(ContextVectorIcon::Sync));

    favorite->setVisible(program_id != 0);
    reset_game_settings->setVisible(program_id != 0);
    mod_manager_action->setVisible(program_id != 0);
    cheats_action->setVisible(program_id != 0);
    open_save_location->setVisible(program_id != 0);
    sync_save->setVisible(program_id != 0);
    open_mod_location->setVisible(program_id != 0);
    open_transferable_shader_cache->setVisible(program_id != 0);
    remove_update->setVisible(program_id != 0);
    remove_dlc->setVisible(program_id != 0);
    remove_gl_shader_cache->setVisible(program_id != 0);
    remove_vk_shader_cache->setVisible(program_id != 0);
    remove_shader_cache->setVisible(program_id != 0);
    remove_all_content->setVisible(program_id != 0);

    connect(mod_manager_action, &QAction::triggered, this, [this, program_id, path]() {
        emit OpenModManagerRequested(program_id, QString::fromStdString(path));
    });
    connect(cheats_action, &QAction::triggered, this, [this, program_id, path]() {
        emit OpenCheatsRequested(program_id, QString::fromStdString(path));
    });
    if (game_fix_action) {
        connect(game_fix_action, &QAction::triggered, this, [this, program_id, path]() {
            emit OpenGameFixRequested(program_id, QString::fromStdString(path));
        });
    }
    connect(reset_game_settings, &QAction::triggered, this, [this, program_id, path, storm_tr]() {
        const auto file_path_hash = Common::CityHash64(path.data(), path.size());
        const auto specific_config = fmt::format("{:016X}_{:016X}", program_id, file_path_hash);
        const auto legacy_config = fmt::format("{:016X}", program_id);
        const auto custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
        std::error_code ec;
        std::filesystem::remove(custom_path / (specific_config + ".ini"), ec);
        std::filesystem::remove(custom_path / (legacy_config + ".ini"), ec);
        QMessageBox::information(
            this,
            storm_tr(QStringLiteral("Сброс настроек"), QStringLiteral("Reset Settings"),
                     QStringLiteral("重置设置"), QStringLiteral("設定のリセット"),
                     QStringLiteral("설정 초기화"), QStringLiteral("Einstellungen zurücksetzen"),
                     QStringLiteral("Réinitialiser les paramètres"), QStringLiteral("Restablecer configuración"),
                     QStringLiteral("إعادة ضبط الإعدادات")),
            storm_tr(QStringLiteral("Персональные настройки игры сброшены на стандартные.\nИгра будет ориентироваться на глобальные настройки эмулятора."),
                     QStringLiteral("Per-game custom settings have been reset to defaults.\nThe game will now use the global emulator settings."),
                     QStringLiteral("游戏的自定义设置已恢复为默认值。\n游戏现在将使用模拟器的全局设置。"),
                     QStringLiteral("ゲーム個別の設定がデフォルトにリセットされました。\n今後はエミュレータの全体設定が使用されます。"),
                     QStringLiteral("게임의 사용자 지정 설정이 기본값으로 초기화되었습니다.\n이제 에뮬레이터의 전역 설정을 사용합니다."),
                     QStringLiteral("Die spielspezifischen Einstellungen wurden zurückgesetzt.\nDas Spiel verwendet nun die globalen Emulator-Einstellungen."),
                     QStringLiteral("Les paramètres personnalisés du jeu ont été réinitialisés.\nLe jeu utilisera désormais les paramètres globaux de l'émulateur."),
                     QStringLiteral("La configuración personalizada del juego se ha restablecido.\nEl juego usará ahora la configuración global del emulador."),
                     QStringLiteral("تمت إعادة تعيين الإعدادات المخصصة للعبة إلى الوضع الافتراضي.\nستستخدم اللعبة الآن الإعدادات العامة للمحاكي.")));
    });
    connect(favorite, &QAction::triggered, this,
            [this, program_id]() { ToggleFavorite(program_id); });
    connect(open_save_location, &QAction::triggered, this, [this, program_id, path]() {
        emit OpenFolderRequested(program_id, GameListOpenTarget::SaveData, path);
    });
    connect(sync_save, &QAction::triggered, this, [this, program_id]() {
        emit OpenSaveSyncRequested(program_id);
    });
    connect(start_game, &QAction::triggered, this,
            [this, path]() { emit BootGame(QString::fromStdString(path), StartGameType::Normal); });
    connect(start_game_global, &QAction::triggered, this,
            [this, path]() { emit BootGame(QString::fromStdString(path), StartGameType::Global); });
    connect(open_mod_location, &QAction::triggered, this, [this, program_id, path]() {
        emit OpenFolderRequested(program_id, GameListOpenTarget::ModData, path);
    });
    connect(open_transferable_shader_cache, &QAction::triggered, this,
            [this, program_id]() { emit OpenTransferableShaderCacheRequested(program_id); });
    connect(remove_all_content, &QAction::triggered, this, [this, program_id]() {
        emit RemoveInstalledEntryRequested(program_id, QtCommon::Game::InstalledEntryType::Game);
    });
    connect(remove_update, &QAction::triggered, this, [this, program_id]() {
        emit RemoveInstalledEntryRequested(program_id, QtCommon::Game::InstalledEntryType::Update);
    });
    connect(remove_dlc, &QAction::triggered, this, [this, program_id]() {
        emit RemoveInstalledEntryRequested(program_id,
                                           QtCommon::Game::InstalledEntryType::AddOnContent);
    });
    connect(remove_gl_shader_cache, &QAction::triggered, this, [this, program_id, path]() {
        emit RemoveFileRequested(program_id, QtCommon::Game::GameListRemoveTarget::GlShaderCache,
                                 path);
    });
    connect(remove_vk_shader_cache, &QAction::triggered, this, [this, program_id, path]() {
        emit RemoveFileRequested(program_id, QtCommon::Game::GameListRemoveTarget::VkShaderCache,
                                 path);
    });
    connect(remove_shader_cache, &QAction::triggered, this, [this, program_id, path]() {
        emit RemoveFileRequested(program_id, QtCommon::Game::GameListRemoveTarget::AllShaderCache,
                                 path);
    });
    connect(remove_custom_config, &QAction::triggered, this, [this, program_id, path]() {
        emit RemoveFileRequested(program_id,
                                 QtCommon::Game::GameListRemoveTarget::CustomConfiguration, path);
    });
    connect(set_play_time, &QAction::triggered, this,
            [this, program_id]() { emit SetPlayTimeRequested(program_id); });
    connect(remove_play_time_data, &QAction::triggered, this,
            [this, program_id]() { emit RemovePlayTimeRequested(program_id); });
    connect(remove_cache_storage, &QAction::triggered, this, [this, program_id, path] {
        emit RemoveFileRequested(program_id, QtCommon::Game::GameListRemoveTarget::CacheStorage,
                                 path);
    });
    connect(dump_romfs, &QAction::triggered, this, [this, program_id, path]() {
        emit DumpRomFSRequested(program_id, path, DumpRomFSTarget::Normal);
    });
    connect(dump_romfs_sdmc, &QAction::triggered, this, [this, program_id, path]() {
        emit DumpRomFSRequested(program_id, path, DumpRomFSTarget::SDMC);
    });
    connect(verify_integrity, &QAction::triggered, this,
            [this, path]() { emit VerifyIntegrityRequested(path); });
    connect(copy_tid, &QAction::triggered, this,
            [this, program_id]() { emit CopyTIDRequested(program_id); });
#if !defined(__APPLE__)
    connect(create_desktop_shortcut, &QAction::triggered, this, [this, program_id, path]() {
        emit CreateShortcut(program_id, path, QtCommon::Game::ShortcutTarget::Desktop);
    });
    connect(create_applications_menu_shortcut, &QAction::triggered, this,
            [this, program_id, path]() {
                emit CreateShortcut(program_id, path, QtCommon::Game::ShortcutTarget::Applications);
            });
#endif
    connect(properties, &QAction::triggered, this,
            [this, path]() { emit OpenPerGameGeneralRequested(path); });

    connect(ryujinx, &QAction::triggered, this,
            [this, program_id]() { emit LinkToRyujinxRequested(program_id); });
};

void GameList::AddCustomDirPopup(QMenu& context_menu, QModelIndex selected) {
    UISettings::GameDir& game_dir =
        UISettings::values.game_dirs[selected.data(GameListDir::GameDirRole).toInt()];

    QAction* deep_scan = context_menu.addAction(tr("Scan Subfolders"));
    QAction* delete_dir = context_menu.addAction(tr("Remove Game Directory"));

    deep_scan->setCheckable(true);
    deep_scan->setChecked(game_dir.deep_scan);

    connect(deep_scan, &QAction::triggered, this, [this, &game_dir] {
        game_dir.deep_scan = !game_dir.deep_scan;
        PopulateAsync(UISettings::values.game_dirs);
    });
    connect(delete_dir, &QAction::triggered, this, [this, &game_dir, selected] {
        UISettings::values.game_dirs.removeOne(game_dir);
        item_model->invisibleRootItem()->removeRow(selected.row());
        OnTextChanged(search_field->filterText());
    });
}

void GameList::AddPermDirPopup(QMenu& context_menu, QModelIndex selected) {
    const int game_dir_index = selected.data(GameListDir::GameDirRole).toInt();

    QAction* move_up = context_menu.addAction(tr("\u25B2 Move Up"));
    QAction* move_down = context_menu.addAction(tr("\u25bc Move Down"));
    QAction* open_directory_location = context_menu.addAction(tr("Open Directory Location"));

    const int row = selected.row();

    move_up->setEnabled(row > 1);
    move_down->setEnabled(row < item_model->rowCount() - 2);

    connect(move_up, &QAction::triggered, this, [this, selected, row, game_dir_index] {
        const int other_index = selected.sibling(row - 1, 0).data(GameListDir::GameDirRole).toInt();
        std::swap(UISettings::values.game_dirs[game_dir_index],
                  UISettings::values.game_dirs[other_index]);
        item_model->setData(selected, QVariant(other_index), GameListDir::GameDirRole);
        item_model->setData(selected.sibling(row - 1, 0), QVariant(game_dir_index),
                            GameListDir::GameDirRole);
        QList<QStandardItem*> items = item_model->takeRow(row);
        item_model->invisibleRootItem()->insertRow(row - 1, items);
        tree_view->setExpanded(selected.sibling(row - 1, 0),
                               UISettings::values.game_dirs[other_index].expanded);
    });

    connect(move_down, &QAction::triggered, this, [this, selected, row, game_dir_index] {
        const int other_index = selected.sibling(row + 1, 0).data(GameListDir::GameDirRole).toInt();
        std::swap(UISettings::values.game_dirs[game_dir_index],
                  UISettings::values.game_dirs[other_index]);
        item_model->setData(selected, QVariant(other_index), GameListDir::GameDirRole);
        item_model->setData(selected.sibling(row + 1, 0), QVariant(game_dir_index),
                            GameListDir::GameDirRole);
        const QList<QStandardItem*> items = item_model->takeRow(row);
        item_model->invisibleRootItem()->insertRow(row + 1, items);
        tree_view->setExpanded(selected.sibling(row + 1, 0),
                               UISettings::values.game_dirs[other_index].expanded);
    });

    connect(open_directory_location, &QAction::triggered, this, [this, game_dir_index] {
        emit OpenDirectory(
            QString::fromStdString(UISettings::values.game_dirs[game_dir_index].path));
    });
}

void GameList::AddFavoritesPopup(QMenu& context_menu) {
    QAction* clear = context_menu.addAction(tr("Clear"));

    connect(clear, &QAction::triggered, this, [this] {
        UISettings::values.favorited_ids.clear();
        item_model->invisibleRootItem()->child(0)->removeRows(
            0, item_model->invisibleRootItem()->child(0)->rowCount());
        tree_view->setRowHidden(0, item_model->invisibleRootItem()->index(), true);
    });
}

void GameList::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void GameList::RetranslateUI() {
    if (item_model) {
        item_model->RetranslateUI();
        for (int col = 0; col < item_model->columnCount(); ++col) {
            auto* header_item = item_model->horizontalHeaderItem(col);
            if (header_item) {
                header_item->setText(item_model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
            }
        }
    }
    if (tree_view && tree_view->header()) {
        tree_view->header()->update();
    }
}

void GameList::SetupScrollAnimation() {
    auto setup = [this](QVariantAnimation* anim, QScrollBar* bar) {
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setDuration(200);
        connect(anim, &QVariantAnimation::valueChanged, this,
                [bar](const QVariant& value) { bar->setValue(value.toInt()); });
    };

    vertical_scroll = new QVariantAnimation(this);
    horizontal_scroll = new QVariantAnimation(this);

    setup(vertical_scroll, tree_view->verticalScrollBar());
    setup(horizontal_scroll, tree_view->horizontalScrollBar());

    setup(vertical_scroll, grid_view->verticalScrollBar());
    setup(horizontal_scroll, grid_view->horizontalScrollBar());
}

bool GameList::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_currentView->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        bool horizontal = wheelEvent->modifiers() & Qt::ShiftModifier;

        int deltaX = wheelEvent->angleDelta().x();
        int deltaY = wheelEvent->angleDelta().y();

        if (horizontal && deltaY != 0 && deltaX == 0) {
            deltaX = deltaY;
            deltaY = 0;
        }

        if (deltaY != 0) {
            if (vertical_scroll->state() == QAbstractAnimation::Stopped)
                vertical_scroll_target = m_currentView->verticalScrollBar()->value();

            vertical_scroll_target -= deltaY;
            vertical_scroll_target =
                qBound(0, vertical_scroll_target, m_currentView->verticalScrollBar()->maximum());

            vertical_scroll->stop();
            vertical_scroll->setStartValue(m_currentView->verticalScrollBar()->value());
            vertical_scroll->setEndValue(vertical_scroll_target);
            vertical_scroll->start();
        }

        if (deltaX != 0) {
            if (horizontal_scroll->state() == QAbstractAnimation::Stopped)
                horizontal_scroll_target = m_currentView->horizontalScrollBar()->value();

            horizontal_scroll_target -= deltaX;
            horizontal_scroll_target = qBound(0, horizontal_scroll_target,
                                              m_currentView->horizontalScrollBar()->maximum());

            horizontal_scroll->stop();
            horizontal_scroll->setStartValue(m_currentView->horizontalScrollBar()->value());
            horizontal_scroll->setEndValue(horizontal_scroll_target);
            horizontal_scroll->start();
        }

        return true;
    }

    if (obj == m_currentView->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        QModelIndex index = m_currentView->indexAt(mouseEvent->pos());
        if (!index.isValid()) {
            m_currentView->selectionModel()->clearSelection();
            m_currentView->setCurrentIndex(QModelIndex());
        }
    }



    if (obj == carousel_view->viewport() && event->type() == QEvent::Resize) {
        carousel_view->UpdateIconSize();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

GameListPlaceholder::GameListPlaceholder(MainWindow* parent) : QWidget{parent} {
    layout = new QVBoxLayout;
    image = new QLabel;
    text = new QLabel;
    layout->setAlignment(Qt::AlignCenter);
    image->setPixmap(QIcon::fromTheme(QStringLiteral("plus_folder")).pixmap(200));

    QFont font = text->font();
    font.setPointSize(18);
    font.setBold(true);
    text->setFont(font);
    text->setAlignment(Qt::AlignHCenter);
    image->setAlignment(Qt::AlignHCenter);

    RetranslateUI();

    layout->addWidget(image);
    layout->addWidget(text);
    setLayout(layout);
}

GameListPlaceholder::~GameListPlaceholder() = default;

void GameListPlaceholder::mouseDoubleClickEvent(QMouseEvent* event) {
    emit GameListPlaceholder::AddDirectory();
}

void GameListPlaceholder::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void GameListPlaceholder::RetranslateUI() {
    text->setText(tr("Double-click to add a new folder to the game list"));
}

std::pair<u64, QString> GameList::GetSelectedGameInfo() const {
    if (!m_currentView) {
        return {0, QString{}};
    }
    if (m_currentView->selectionModel()) {
        const auto indexes = m_currentView->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            const auto selected = idx.sibling(idx.row(), 0);
            if (selected.data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
                u64 tid = selected.data(GameListItemPath::ProgramIdRole).toULongLong();
                QString path = selected.data(GameListItemPath::FullPathRole).toString();
                return {tid, path};
            }
        }
    }
    const auto curr = m_currentView->currentIndex();
    if (curr.isValid()) {
        const auto selected = curr.sibling(curr.row(), 0);
        if (selected.data(GameListItem::TypeRole).value<GameListItemType>() == GameListItemType::Game) {
            u64 tid = selected.data(GameListItemPath::ProgramIdRole).toULongLong();
            QString path = selected.data(GameListItemPath::FullPathRole).toString();
            return {tid, path};
        }
    }
    return {0, QString{}};
}
