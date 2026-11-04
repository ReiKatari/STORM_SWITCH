// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QLocale>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThreadPool>

#include "common/logging.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/qt_common.h"
#include "qt_common/util/game.h"

#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/model.h"
#include "qt_common/game_list/worker.h"

GameListModel::GameListModel(std::shared_ptr<FileSys::VfsFilesystem> vfs_,
                             FileSys::ManualContentProvider* provider_,
                             const PlayTime::PlayTimeManager& play_time_manager_,
                             Core::System& system_, QObject* parent)
    : QStandardItemModel{parent}, vfs{std::move(vfs_)}, provider{provider_},
      play_time_manager{play_time_manager_}, system{system_} {
    watcher = new QFileSystemWatcher(this);
    external_watcher = new QFileSystemWatcher(this);

    connect(watcher, &QFileSystemWatcher::directoryChanged, this,
            &GameListModel::RefreshGameDirectory);
    connect(external_watcher, &QFileSystemWatcher::directoryChanged, this,
            &GameListModel::RefreshExternalContent);

    refresh_timer = new QTimer(this);
    refresh_timer->setSingleShot(true);
    connect(refresh_timer, &QTimer::timeout, this, &GameListModel::Repopulate);

    ResetExternalWatcher();

    insertColumns(0, COLUMN_COUNT);
    RetranslateUI();

    setSortRole(GameListItemPath::SortRole);
}

GameListModel::~GameListModel() = default;

bool GameListModel::LoadFromCache(const QVector<UISettings::GameDir>& game_dirs) {
    if (!UISettings::values.cache_game_list) {
        return false;
    }
    const auto cache_path = Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list" / "file_metadata_cache.json");
    QFile file(QString::fromStdString(cache_path));
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    QJsonParseError parse_err{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parse_err);
    if (parse_err.error != QJsonParseError::NoError || !doc.isObject()) {
        LOG_WARNING(Frontend, "Game list cache corrupted ({}), falling back to full scan",
                    parse_err.errorString().toStdString());
        return false;
    }

    const auto root = doc.object();
    if (root.isEmpty()) {
        return false;
    }

    int loaded_count = 0;
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QString qpath = it.key();
        const std::string path = qpath.toStdString();
        const auto obj = it.value().toObject();

        const bool is_bootable = obj.value(QStringLiteral("bootable")).toBool(true);
        if (!is_bootable) {
            continue;
        }

        const u64 program_id = obj.value(QStringLiteral("id")).toString().toULongLong(nullptr, 16);
        if (program_id == 0 || (program_id & 0xFFF) != 0) {
            continue;
        }

        const u64 file_size = static_cast<u64>(obj.value(QStringLiteral("size")).toVariant().toULongLong());
        const std::string name = obj.value(QStringLiteral("name")).toString().toStdString();
        const QString file_type = obj.value(QStringLiteral("type")).toString();
        const QString patches = obj.value(QStringLiteral("patches")).toString();
        const QString version = obj.value(QStringLiteral("version")).toString();
        const QString addons = obj.value(QStringLiteral("addons")).toString();

        const auto entry = MakeCachedGameListEntry(path, name, file_size, {}, file_type,
                                                   program_id, play_time_manager, patches, version, addons);
        invisibleRootItem()->appendRow(entry);
        loaded_count++;
    }

    if (loaded_count > 0) {
        emit ShowList(true);
        emit StatusMessage(tr("Обновление списка игр..."));
        return true;
    }
    return false;
}

void GameListModel::SortGames(int sort_mode) {
    switch (sort_mode) {
    case 0: // Name A-Z
        setSortRole(GameListItemPath::TitleRole);
        sort(0, Qt::AscendingOrder);
        break;
    case 1: // Name Z-A
        setSortRole(GameListItemPath::TitleRole);
        sort(0, Qt::DescendingOrder);
        break;
    case 2: // Play Time Descending
        setSortRole(GameListItemPath::PlayTimeRole);
        sort(0, Qt::DescendingOrder);
        break;
    case 3: // File Size Descending
        setSortRole(GameListItemPath::FileSizeBytesRole);
        sort(0, Qt::DescendingOrder);
        break;
    case 4: // Platform Descending (Switch 2 first)
        setSortRole(GameListItemPath::PlatformRole);
        sort(0, Qt::DescendingOrder);
        break;
    default:
        setSortRole(GameListItemPath::SortRole);
        sort(0, Qt::AscendingOrder);
        break;
    }
}

void GameListModel::PopulateAsync(QVector<UISettings::GameDir>& game_dirs) {
    if (refresh_timer) {
        refresh_timer->stop();
    }
    emit PopulatingStarted();

    current_worker.reset();
    removeRows(0, rowCount());

    // 1. Fast Cache Hydration (<= 0.2s): show game cards immediately!
    LoadFromCache(game_dirs);

    // 2. Incremental Background Rescan
    current_worker = std::make_unique<GameListWorker>(vfs, provider, game_dirs,
                                                      play_time_manager, system);

    connect(current_worker.get(), &GameListWorker::DataAvailable, this, &GameListModel::WorkerEvent,
            Qt::QueuedConnection);

    QThreadPool::globalInstance()->start(current_worker.get());
}

void GameListModel::CancelPopulate() {
    if (refresh_timer) {
        refresh_timer->stop();
    }
    if (current_worker) {
        current_worker->Cancel();
    }
}

void GameListModel::WorkerEvent() {
    current_worker->ProcessEvents(this);
}

void GameListModel::AddDirEntry(GameListDir* entry_items) {
    if (m_flat) {
        return;
    }
    invisibleRootItem()->appendRow(entry_items);
}

void GameListModel::AddEntry(const QList<QStandardItem*>& entry_items, GameListDir* parent) {
    if (entry_items.isEmpty()) return;
    const QString new_path = entry_items.first()->data(GameListItemPath::FullPathRole).toString();

    // Check if entry already exists in model (e.g. from fast cache hydration)
    if (!new_path.isEmpty()) {
        auto* root = invisibleRootItem();
        for (int i = 0; i < root->rowCount(); ++i) {
            auto* existing = root->child(i, 0);
            if (existing && existing->data(GameListItemPath::FullPathRole).toString() == new_path) {
                return;
            }
        }
    }

    if (m_flat) {
        invisibleRootItem()->appendRow(entry_items);
    } else {
        parent->appendRow(entry_items);
    }
    emit ShowList(true);
}

void GameListModel::DonePopulating(const QStringList& watch_list) {
    emit ShowList(!IsEmpty());

    if (!m_flat) {
        invisibleRootItem()->appendRow(new GameListAddDir());
        invisibleRootItem()->insertRow(0, new GameListFavorites());

        for (const auto id : std::as_const(UISettings::values.favorited_ids)) {
            AddFavorite(id);
        }
    }

    emit StatusMessage(QString{});
    emit PopulatingCompleted(watch_list);
}

bool GameListModel::IsEmpty() const {
    for (int i = 0; i < rowCount(); i++) {
        const QStandardItem* child = invisibleRootItem()->child(i);
        const auto type = static_cast<GameListItemType>(child->type());

        if (!child->hasChildren() &&
            (type == GameListItemType::SdmcDir || type == GameListItemType::UserNandDir ||
             type == GameListItemType::SysNandDir)) {
            invisibleRootItem()->removeRow(child->row());
            i--;
        }
    }

    return !invisibleRootItem()->hasChildren();
}

void GameListModel::ToggleFavorite(u64 program_id) {
    if (!UISettings::values.favorited_ids.contains(program_id)) {
        UISettings::values.favorited_ids.append(program_id);
        AddFavorite(program_id);
    } else {
        UISettings::values.favorited_ids.removeOne(program_id);
        RemoveFavorite(program_id);
    }
    emit SaveConfig();
}

void GameListModel::AddFavorite(u64 program_id) {
    auto* favorites_row = item(0);

    for (int i = 1; i < rowCount() - 1; i++) {
        const auto* folder = item(i);
        for (int j = 0; j < folder->rowCount(); j++) {
            if (folder->child(j)->data(GameListItemPath::ProgramIdRole).toULongLong() ==
                program_id) {
                QList<QStandardItem*> list;
                for (int k = 0; k < COLUMN_COUNT; k++) {
                    list.append(folder->child(j, k)->clone());
                }
                list[0]->setData(folder->child(j)->data(GameListItem::SortRole),
                                 GameListItem::SortRole);
                list[0]->setText(folder->child(j)->data(Qt::DisplayRole).toString());

                favorites_row->appendRow(list);
                return;
            }
        }
    }
}

void GameListModel::RemoveFavorite(u64 program_id) {
    auto* favorites_row = item(0);

    for (int i = 0; i < favorites_row->rowCount(); i++) {
        const auto* game = favorites_row->child(i);
        if (game->data(GameListItemPath::ProgramIdRole).toULongLong() == program_id) {
            favorites_row->removeRow(i);
            return;
        }
    }
}

void GameListModel::Repopulate() {
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        LOG_INFO(Frontend, "Game emulation is active. Postponing game list reload to prevent I/O and VFS conflicts.");
        return;
    }
    LOG_INFO(Frontend, "Executing debounced game list reload.");
    current_worker.reset();
    QtCommon::system->GetFileSystemController().CreateFactories(*QtCommon::vfs);
    PopulateAsync(UISettings::values.game_dirs);
}

void GameListModel::RefreshGameDirectory() {
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        return;
    }
    ResetExternalWatcher();
    if (!UISettings::values.game_dirs.empty()) {
        if (!refresh_timer->isActive()) {
            LOG_INFO(Frontend, "Change detected in the games directory. Debouncing reload (2.5s)...");
        }
        refresh_timer->start(2500);
    }
}

void GameListModel::RefreshExternalContent() {
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        return;
    }
    if (!UISettings::values.game_dirs.empty()) {
        QtCommon::Game::ResetMetadata(false);
        if (!refresh_timer->isActive()) {
            LOG_INFO(Frontend, "External content changed. Debouncing reload (2.5s)...");
        }
        refresh_timer->start(2500);
    }
}

void GameListModel::ResetExternalWatcher() {
    auto watch_dirs = external_watcher->directories();
    if (!watch_dirs.isEmpty()) {
        external_watcher->removePaths(watch_dirs);
    }

    for (const std::string& dir : Settings::values.external_content_dirs) {
        external_watcher->addPath(QString::fromStdString(dir));
    }
}

namespace {

QString GetColumnHeaderTitle(GameListModel::Column col) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }

    if (lang.rfind("ru", 0) == 0 || lang.rfind("be", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("НАЗВАНИЕ");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("ТИП ФАЙЛА");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("РАЗМЕР");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("ВРЕМЯ ИГРЫ");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("ДОПОЛНЕНИЯ");
        default: return {};
        }
    }
    if (lang.rfind("de", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("NAME");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("DATEITYP");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("GRÖSSE");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("SPIELZEIT");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("ADD-ONS");
        default: return {};
        }
    }
    if (lang.rfind("fr", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("NOM");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("TYPE DE FICHIER");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("TAILLE");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("TEMPS DE JEU");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("EXTENSIONS");
        default: return {};
        }
    }
    if (lang.rfind("es", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("NOMBRE");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("TIPO DE ARCHIVO");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("TAMAÑO");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("TIEMPO DE JUEGO");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("COMPLEMENTOS");
        default: return {};
        }
    }
    if (lang.rfind("it", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("NOME");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("TIPO DI FILE");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("DIMENSIONE");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("TEMPO DI GIOCO");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("COMPONENTI AGGIUNTIVI");
        default: return {};
        }
    }
    if (lang.rfind("pt", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("NOME");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("TIPO DE ARQUIVO");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("TAMANHO");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("TEMPO DE JOGO");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("COMPLEMENTOS");
        default: return {};
        }
    }
    if (lang.rfind("zh", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("名称");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("文件类型");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("大小");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("游玩时间");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("附加内容");
        default: return {};
        }
    }
    if (lang.rfind("ja", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("タイトル");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("ファイル形式");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("サイズ");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("プレイ時間");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("追加コンテンツ");
        default: return {};
        }
    }
    if (lang.rfind("ko", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("이름");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("파일 형식");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("크기");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("플레이 시간");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("추가 콘텐츠");
        default: return {};
        }
    }
    if (lang.rfind("hi", 0) == 0) {
        switch (col) {
        case GameListModel::COLUMN_NAME: return QStringLiteral("नाम");
        case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("फ़ाइल प्रकार");
        case GameListModel::COLUMN_SIZE: return QStringLiteral("आकार");
        case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("खेल का समय");
        case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("अतिरिक्त सामग्री");
        default: return {};
        }
    }

    // Default: English
    switch (col) {
    case GameListModel::COLUMN_NAME: return QStringLiteral("NAME");
    case GameListModel::COLUMN_FILE_TYPE: return QStringLiteral("FILE TYPE");
    case GameListModel::COLUMN_SIZE: return QStringLiteral("SIZE");
    case GameListModel::COLUMN_PLAY_TIME: return QStringLiteral("PLAY TIME");
    case GameListModel::COLUMN_ADD_ONS: return QStringLiteral("ADD-ONS");
    default: return {};
    }
}

} // namespace

void GameListModel::RetranslateUI() {
    setHeaderData(COLUMN_NAME, Qt::Horizontal, GetColumnHeaderTitle(COLUMN_NAME));
    setHeaderData(COLUMN_FILE_TYPE, Qt::Horizontal, GetColumnHeaderTitle(COLUMN_FILE_TYPE));
    setHeaderData(COLUMN_SIZE, Qt::Horizontal, GetColumnHeaderTitle(COLUMN_SIZE));
    setHeaderData(COLUMN_PLAY_TIME, Qt::Horizontal, GetColumnHeaderTitle(COLUMN_PLAY_TIME));
    setHeaderData(COLUMN_ADD_ONS, Qt::Horizontal, GetColumnHeaderTitle(COLUMN_ADD_ONS));

    setHeaderData(COLUMN_NAME, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    setHeaderData(COLUMN_FILE_TYPE, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    setHeaderData(COLUMN_SIZE, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    setHeaderData(COLUMN_PLAY_TIME, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    setHeaderData(COLUMN_ADD_ONS, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
}

QFileSystemWatcher* GameListModel::GetWatcher() const {
    return watcher;
}

void GameListModel::SetFlat(bool flat) {
    m_flat = flat;
}
