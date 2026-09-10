// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/storm_games_world_dialog.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QComboBox>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "qt_common/titledb.h"

static QString SanitizeFileName(QString name) {
    static const QString forbidden = QStringLiteral("<>:\"/\\|?*");
    for (const auto& ch : forbidden) {
        name.replace(ch, QLatin1Char('_'));
    }
    return name.trimmed();
}

static QString SanitizeLibretroName(QString name) {
    static const QString forbidden = QStringLiteral("&*/:`<>?\\|\"");
    for (const auto& ch : forbidden) {
        name.replace(ch, QLatin1Char('_'));
    }
    return name.trimmed();
}

StormGamesWorldDialog::StormGamesWorldDialog(QWidget* parent)
    : QDialog(parent) {
    SetupUI();
    ApplyStormStyles();
    OnFetchCatalog();
}

StormGamesWorldDialog::~StormGamesWorldDialog() {
    if (download_reply) {
        download_reply->abort();
        download_reply->deleteLater();
    }
    if (head_reply) {
        head_reply->abort();
        head_reply->deleteLater();
    }
    if (cover_reply) {
        cover_reply->abort();
        cover_reply->deleteLater();
    }
    if (details_reply) {
        details_reply->abort();
        details_reply->deleteLater();
    }
    if (output_file && output_file->isOpen()) {
        output_file->close();
    }
}

QString StormGamesWorldDialog::GetDefaultDownloadDir() const {
    QSettings settings;
    QString dir = settings.value(QStringLiteral("StormGamesWorld/download_dir")).toString();
    if (dir.isEmpty() || !QDir(dir).exists()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/STORM_SWITCH_GAMES");
        QDir().mkpath(dir);
    }
    return dir;
}

void StormGamesWorldDialog::SaveDownloadDir(const QString& dir) {
    QSettings settings;
    settings.setValue(QStringLiteral("StormGamesWorld/download_dir"), dir);
}

bool StormGamesWorldDialog::IsGameDownloaded(const StormWorldGame& game, const QString& dir_path) const {
    if (dir_path.isEmpty() || !QDir(dir_path).exists()) return false;

    QDir dir(dir_path);
    const QStringList files = dir.entryList(QDir::Files);

    const QString title1 = SanitizeFileName(game.final_title.isEmpty() ? game.title : game.final_title).toLower();
    const QString title2 = SanitizeFileName(game.title).toLower();
    const QString tid = game.serial_id.toLower();

    for (const auto& file : files) {
        const QString lower_file = file.toLower();
        if (!lower_file.endsWith(QStringLiteral(".nsp")) &&
            !lower_file.endsWith(QStringLiteral(".xci")) &&
            !lower_file.endsWith(QStringLiteral(".nsz"))) {
            continue;
        }

        const QFileInfo fi(dir.filePath(file));
        if (fi.size() < 1024 * 1024) continue;

        if (!tid.isEmpty() && lower_file.contains(tid)) {
            return true;
        }
        if (!title1.isEmpty() && lower_file.contains(title1)) {
            return true;
        }
        if (!title2.isEmpty() && lower_file.contains(title2)) {
            return true;
        }
    }

    return false;
}

void StormGamesWorldDialog::SetupUI() {
    setWindowTitle(tr("🌐 STORM SWITCH — Каталог и менеджер игр STORM GAMES WORLD"));
    resize(1240, 760);
    setMinimumSize(1080, 680);

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(12, 12, 12, 12);
    root_layout->setSpacing(10);

    // --- Top Bar: Search and Refresh ---
    auto* top_bar = new QHBoxLayout();
    top_bar->setSpacing(10);

    search_edit = new QLineEdit(this);
    search_edit->setPlaceholderText(tr("🔍 Поиск доступных игр по названию или Title ID..."));
    search_edit->setClearButtonEnabled(true);
    top_bar->addWidget(search_edit, 1);

    refresh_btn = new QPushButton(tr("🔄 Обновить каталог"), this);
    refresh_btn->setCursor(Qt::PointingHandCursor);
    top_bar->addWidget(refresh_btn);

    root_layout->addLayout(top_bar);

    // --- Download Directory Bar ---
    auto* dir_bar = new QHBoxLayout();
    dir_bar->setSpacing(8);

    auto* dir_label = new QLabel(tr("📁 Папка для сохранения:"), this);
    dir_label->setStyleSheet(QStringLiteral("font-weight: bold; color: #00F0FF;"));
    dir_bar->addWidget(dir_label);

    download_dir_edit = new QLineEdit(GetDefaultDownloadDir(), this);
    download_dir_edit->setReadOnly(true);
    dir_bar->addWidget(download_dir_edit, 1);

    browse_dir_btn = new QPushButton(tr("Обзор..."), this);
    browse_dir_btn->setCursor(Qt::PointingHandCursor);
    dir_bar->addWidget(browse_dir_btn);

    open_folder_btn = new QPushButton(tr("Открыть папку"), this);
    open_folder_btn->setCursor(Qt::PointingHandCursor);
    dir_bar->addWidget(open_folder_btn);

    root_layout->addLayout(dir_bar);

    // --- Main Splitter (Left: Games Tree, Right: Game Card) ---
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // Left Panel: Games Tree
    games_tree = new QTreeWidget(splitter);
    games_tree->setHeaderLabels({
        tr("Название игры"),
        tr("Версия"),
        tr("Размер"),
        tr("Язык"),
        tr("Title ID"),
        tr("Статус")
    });
    games_tree->setRootIsDecorated(false);
    games_tree->setAlternatingRowColors(true);
    games_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    games_tree->headerItem()->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    games_tree->headerItem()->setTextAlignment(1, Qt::AlignCenter);
    games_tree->headerItem()->setTextAlignment(2, Qt::AlignCenter);
    games_tree->headerItem()->setTextAlignment(3, Qt::AlignCenter);
    games_tree->headerItem()->setTextAlignment(4, Qt::AlignCenter);
    games_tree->headerItem()->setTextAlignment(5, Qt::AlignCenter);

    games_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    games_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    splitter->addWidget(games_tree);

    // Right Panel: Game Details & Download Controls
    auto* details_panel = new QFrame(splitter);
    details_panel->setObjectName(QStringLiteral("details_panel"));
    auto* details_layout = new QVBoxLayout(details_panel);
    details_layout->setContentsMargins(14, 14, 14, 14);
    details_layout->setSpacing(10);

    // Header Card (Cover + Titles + Badges)
    auto* card_header = new QHBoxLayout();
    card_header->setSpacing(14);

    cover_label = new QLabel(details_panel);
    cover_label->setFixedSize(160, 160);
    cover_label->setAlignment(Qt::AlignCenter);
    cover_label->setStyleSheet(QStringLiteral(
        "background: rgba(10, 15, 25, 0.85);"
        "border: none;"
        "border-radius: 10px;"
        "color: #7090B0;"
        "font-size: 11px;"
    ));
    card_header->addWidget(cover_label);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setSpacing(6);

    title_label = new QLabel(tr("Выберите игру из списка"), details_panel);
    title_label->setWordWrap(true);
    title_label->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: #FFFFFF;"));
    meta_layout->addWidget(title_label);

    tid_label = new QLabel(tr("Title ID: —"), details_panel);
    tid_label->setStyleSheet(QStringLiteral("font-family: monospace; color: #00F0FF; font-size: 12px;"));
    meta_layout->addWidget(tid_label);

    auto* badges_layout = new QHBoxLayout();
    badges_layout->setSpacing(6);

    version_badge = new QLabel(tr("Версия: —"), details_panel);
    version_badge->setStyleSheet(QStringLiteral("background: rgba(0, 210, 255, 0.15); border: 1px solid #00D2FF; border-radius: 4px; padding: 2px 8px; color: #00F0FF; font-size: 11px; font-weight: bold;"));
    badges_layout->addWidget(version_badge);

    size_badge = new QLabel(tr("Размер: —"), details_panel);
    size_badge->setStyleSheet(QStringLiteral("background: rgba(0, 255, 102, 0.15); border: 1px solid #00FF66; border-radius: 4px; padding: 2px 8px; color: #00FF66; font-size: 11px; font-weight: bold;"));
    badges_layout->addWidget(size_badge);

    lang_badge = new QLabel(tr("Язык: —"), details_panel);
    lang_badge->setStyleSheet(QStringLiteral("background: rgba(168, 85, 247, 0.15); border: 1px solid #A855F7; border-radius: 4px; padding: 2px 8px; color: #A855F7; font-size: 11px; font-weight: bold;"));
    badges_layout->addWidget(lang_badge);

    badges_layout->addStretch(1);
    meta_layout->addLayout(badges_layout);

    // Version selector dropdown
    auto* ver_select_layout = new QHBoxLayout();
    ver_select_layout->setSpacing(6);
    auto* ver_title = new QLabel(tr("Доступная версия:"), details_panel);
    ver_title->setStyleSheet(QStringLiteral("color: #B0C4DE; font-weight: bold;"));
    ver_select_layout->addWidget(ver_title);

    version_combo = new QComboBox(details_panel);
    version_combo->setMinimumWidth(220);
    ver_select_layout->addWidget(version_combo, 1);
    meta_layout->addLayout(ver_select_layout);

    meta_layout->addStretch(1);
    card_header->addLayout(meta_layout, 1);

    details_layout->addLayout(card_header);

    // Description text browser
    auto* desc_title = new QLabel(tr("Описание и состав релиза:"), details_panel);
    desc_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #FFFFFF; font-size: 13px;"));
    details_layout->addWidget(desc_title);

    description_browser = new QTextBrowser(details_panel);
    description_browser->setOpenExternalLinks(true);
    description_browser->setPlaceholderText(tr("Выберите игру, чтобы увидеть описание сюжета и состав пакета..."));
    details_layout->addWidget(description_browser, 1);

    // Download Box
    auto* download_box = new QGroupBox(tr("Менеджер загрузки"), details_panel);
    download_box->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; color: #00F0FF; border: 1px solid rgba(0, 240, 255, 0.3); border-radius: 8px; margin-top: 10px; padding-top: 10px; }"));
    auto* dl_box_layout = new QVBoxLayout(download_box);
    dl_box_layout->setSpacing(8);

    progress_bar = new QProgressBar(download_box);
    progress_bar->setRange(0, 100);
    progress_bar->setValue(0);
    progress_bar->setTextVisible(true);
    progress_bar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: rgba(0,0,0,0.5); border: 1px solid #00D2FF; border-radius: 5px; text-align: center; color: #FFFFFF; font-weight: bold; height: 22px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0072ff, stop:1 #00f2fe); border-radius: 4px; }"
    ));
    dl_box_layout->addWidget(progress_bar);

    download_status_label = new QLabel(tr("Готов к скачиванию"), download_box);
    download_status_label->setStyleSheet(QStringLiteral("color: #B0C4DE; font-size: 12px;"));
    dl_box_layout->addWidget(download_status_label);

    auto* dl_buttons = new QHBoxLayout();
    dl_buttons->setSpacing(10);

    download_btn = new QPushButton(tr("⬇️ Скачать игру"), download_box);
    download_btn->setCursor(Qt::PointingHandCursor);
    download_btn->setEnabled(false);
    download_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00D2FF, stop:1 #0072FF); color: #000000; font-weight: bold; padding: 8px 22px; border-radius: 6px; font-size: 13px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #33E0FF, stop:1 #338BFF); }"
        "QPushButton:pressed { background: #0056b3; color: #FFFFFF; }"
        "QPushButton:disabled { background: #2A3040; color: #607080; border: none; }"
    ));
    dl_buttons->addWidget(download_btn, 1);

    cancel_btn = new QPushButton(tr("Отмена"), download_box);
    cancel_btn->setCursor(Qt::PointingHandCursor);
    cancel_btn->setEnabled(false);
    cancel_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(255, 50, 50, 0.2); color: #FF6666; border: 1px solid #FF4444; padding: 8px 18px; border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(255, 50, 50, 0.4); }"
        "QPushButton:disabled { background: transparent; color: #505050; border: 1px solid #303030; }"
    ));
    dl_buttons->addWidget(cancel_btn);

    dl_box_layout->addLayout(dl_buttons);
    details_layout->addWidget(download_box);

    splitter->addWidget(details_panel);
    splitter->setStretchFactor(0, 5);
    splitter->setStretchFactor(1, 4);

    root_layout->addWidget(splitter, 1);

    // --- Bottom Status Bar ---
    auto* bottom_bar = new QHBoxLayout();
    status_label = new QLabel(tr("Подключение к порталу stormgamesworld.ru..."), this);
    status_label->setStyleSheet(QStringLiteral("color: #7090B0; font-size: 12px;"));
    bottom_bar->addWidget(status_label, 1);

    auto* close_btn = new QPushButton(tr("Закрыть"), this);
    close_btn->setStyleSheet(QStringLiteral("padding: 6px 18px; border-radius: 5px; font-weight: bold;"));
    bottom_bar->addWidget(close_btn);

    root_layout->addLayout(bottom_bar);

    // Connections
    connect(search_edit, &QLineEdit::textChanged, this, &StormGamesWorldDialog::OnSearchFilterChanged);
    connect(refresh_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnFetchCatalog);
    connect(browse_dir_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnBrowseDownloadFolder);
    connect(open_folder_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnOpenDownloadFolder);
    connect(games_tree, &QTreeWidget::itemSelectionChanged, this, &StormGamesWorldDialog::OnGameSelectionChanged);
    connect(download_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnStartDownload);
    connect(cancel_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnCancelDownload);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
}

void StormGamesWorldDialog::ApplyStormStyles() {
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0A0E17; color: #FFFFFF; font-family: 'Segoe UI', sans-serif; }"
        "QLineEdit { background: rgba(255, 255, 255, 0.05); border: 1px solid #20354E; border-radius: 6px; padding: 7px 12px; color: #FFFFFF; font-size: 12px; }"
        "QLineEdit:focus { border: 1px solid #00D2FF; background: rgba(0, 210, 255, 0.08); }"
        "QTreeWidget { background-color: #070A10; border: 1px solid #1E2838; border-radius: 8px; color: #E0E8F0; alternate-background-color: #0F1420; selection-background-color: rgba(0, 210, 255, 0.25); selection-color: #FFFFFF; }"
        "QTreeWidget::item { height: 32px; padding: 4px; border-right: 1px solid rgba(0, 210, 255, 0.15); }"
        "QTreeWidget::item:hover { background-color: rgba(0, 210, 255, 0.12); }"
        "QTreeWidget::item:selected { background-color: rgba(0, 210, 255, 0.3); }"
        "QHeaderView::section { background-color: #121824; color: #00F0FF; font-weight: bold; border: none; border-right: 1px solid #20354E; border-bottom: 1px solid #20354E; padding: 7px 8px; }"
        "QTextBrowser { background-color: #070A10; border: 1px solid #1E2838; border-radius: 8px; color: #C8D8E8; font-size: 12px; padding: 8px; }"
        "QComboBox { background: rgba(255, 255, 255, 0.05); border: 1px solid #20354E; border-radius: 6px; color: #FFFFFF; padding: 5px 10px; font-weight: bold; }"
        "QComboBox QAbstractItemView { background-color: #141B26; color: #FFFFFF; selection-background-color: #00D2FF; selection-color: #000000; border: 1px solid #20354E; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1A2434, stop:1 #131A26); border: 1px solid #2A3B52; color: #E0E8F0; padding: 7px 16px; border-radius: 6px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #25344C, stop:1 #1A2638); border-color: #00D2FF; color: #FFFFFF; }"
        "QPushButton:pressed { background: #00D2FF; color: #000000; border-color: #38BDF8; }"
        "QScrollBar:vertical { background: #0A0E17; width: 12px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #1E2D42; min-height: 24px; border-radius: 6px; }"
        "QScrollBar::handle:vertical:hover { background: #00D2FF; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ));

    // Soft drop shadow on download button
    auto* shadow = new QGraphicsDropShadowEffect(download_btn);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 210, 255, 140));
    shadow->setOffset(0, 3);
    download_btn->setGraphicsEffect(shadow);
}

void StormGamesWorldDialog::OnFetchCatalog() {
    status_label->setText(tr("Загрузка каталога с stormgamesworld.ru..."));
    refresh_btn->setEnabled(false);

    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games/index")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.5 (Windows x64)"));

    if (catalog_reply) {
        catalog_reply->abort();
        catalog_reply->deleteLater();
    }
    catalog_reply = network_mgr.get(req);
    connect(catalog_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnCatalogReplyFinished);
}

void StormGamesWorldDialog::OnCatalogReplyFinished() {
    refresh_btn->setEnabled(true);
    if (!catalog_reply) return;

    if (catalog_reply->error() != QNetworkReply::NoError) {
        status_label->setText(tr("Ошибка подключения: %1").arg(catalog_reply->errorString()));
        catalog_reply->deleteLater();
        catalog_reply = nullptr;
        return;
    }

    const QByteArray raw_data = catalog_reply->readAll();
    catalog_reply->deleteLater();
    catalog_reply = nullptr;

    const QJsonDocument doc = QJsonDocument::fromJson(raw_data);
    if (!doc.isArray()) {
        status_label->setText(tr("Некорректный формат ответа сервера"));
        return;
    }

    all_games.clear();
    const QJsonArray arr = doc.array();

    for (const auto& val : arr) {
        if (!val.isObject()) continue;
        const QJsonObject obj = val.toObject();

        const QString platform = obj[QStringLiteral("platformName")].toString();
        const QString platform_type = obj[QStringLiteral("platformTypeName")].toString();
        const bool file_exists = obj[QStringLiteral("fileExists")].toBool();
        const bool has_file = obj[QStringLiteral("hasFile")].toBool();

        // Strict requirement: Nintendo Switch CONSOLES only, and ONLY games with existing files!
        if (platform == QStringLiteral("Nintendo Switch") &&
            platform_type == QStringLiteral("CONSOLES") &&
            (file_exists || has_file)) {

            StormWorldGame g;
            g.id = obj[QStringLiteral("id")].toInt();
            g.title = obj[QStringLiteral("title")].toString();
            g.final_title = obj[QStringLiteral("finalTitle")].toString();
            g.version = obj[QStringLiteral("version")].toString();
            g.serial_id = obj[QStringLiteral("serialId")].toString();
            g.size = obj[QStringLiteral("size")].toString();
            g.cover_url = obj[QStringLiteral("cover")].toString();
            g.file_exists = file_exists;
            g.has_file = has_file;

            const QJsonArray reg_arr = obj[QStringLiteral("regions")].toArray();
            for (const auto& r : reg_arr) g.regions.append(r.toString());

            const QJsonArray lang_arr = obj[QStringLiteral("textLangs")].toArray();
            for (const auto& l : lang_arr) g.text_langs.append(l.toString());

            all_games.push_back(g);
        }
    }

    PopulateGameList(search_edit->text());
    status_label->setText(tr("Каталог обновлен. Доступно для загрузки игр Nintendo Switch: %1").arg(all_games.size()));
}

void StormGamesWorldDialog::PopulateGameList(const QString& filter) {
    games_tree->clear();
    filtered_games.clear();

    const QString lower_filter = filter.trimmed().toLower();
    const QString current_dir = download_dir_edit ? download_dir_edit->text() : QString();

    for (const auto& g : all_games) {
        if (!lower_filter.isEmpty()) {
            const bool match_title = g.title.toLower().contains(lower_filter);
            const bool match_final = g.final_title.toLower().contains(lower_filter);
            const bool match_tid = g.serial_id.toLower().contains(lower_filter);
            if (!match_title && !match_final && !match_tid) {
                continue;
            }
        }

        filtered_games.push_back(g);

        const QString disp_title = g.final_title.isEmpty() ? g.title : g.final_title;
        const QString langs = g.text_langs.isEmpty() ? tr("Multi") : g.text_langs.join(QStringLiteral(", "));

        auto* item = new QTreeWidgetItem(games_tree);
        item->setText(0, disp_title);
        item->setText(1, g.version.isEmpty() ? tr("1.0.0") : g.version);
        item->setText(2, g.size.isEmpty() ? tr("—") : g.size);
        item->setText(3, langs);
        item->setText(4, g.serial_id.isEmpty() ? tr("—") : g.serial_id);

        // Center align columns 1 to 5
        item->setTextAlignment(1, Qt::AlignCenter);
        item->setTextAlignment(2, Qt::AlignCenter);
        item->setTextAlignment(3, Qt::AlignCenter);
        item->setTextAlignment(4, Qt::AlignCenter);
        item->setTextAlignment(5, Qt::AlignCenter);

        // Check download status
        if (IsGameDownloaded(g, current_dir)) {
            item->setText(5, tr("✅ Скачано"));
            item->setForeground(5, QBrush(QColor(QStringLiteral("#00FF66"))));
        } else if (is_downloading && current_download_game_id == g.id) {
            item->setText(5, tr("⏳ Загрузка"));
            item->setForeground(5, QBrush(QColor(QStringLiteral("#FFA500"))));
        } else {
            item->setText(5, tr("⚪ Доступно"));
            item->setForeground(5, QBrush(QColor(QStringLiteral("#7090B0"))));
        }

        item->setData(0, Qt::UserRole, static_cast<int>(filtered_games.size() - 1));
    }

    if (!filtered_games.empty()) {
        games_tree->setCurrentItem(games_tree->topLevelItem(0));
    } else {
        selected_game_index = -1;
        download_btn->setEnabled(false);
        title_label->setText(tr("Игры не найдены"));
        tid_label->setText(tr("Title ID: —"));
        description_browser->clear();
    }
}

void StormGamesWorldDialog::OnSearchFilterChanged(const QString& query) {
    PopulateGameList(query);
}

void StormGamesWorldDialog::OnGameSelectionChanged() {
    const auto items = games_tree->selectedItems();
    if (items.isEmpty()) {
        selected_game_index = -1;
        download_btn->setEnabled(false);
        return;
    }

    const int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < static_cast<int>(filtered_games.size())) {
        selected_game_index = idx;
        const auto& game = filtered_games[idx];
        DisplayGameDetails(game);
        FetchGameDetails(game.id);
        FetchRealExtension(game.id);
        LoadCover(game);
        download_btn->setEnabled(!is_downloading);
    }
}

void StormGamesWorldDialog::DisplayGameDetails(const StormWorldGame& game) {
    title_label->setText(game.final_title.isEmpty() ? game.title : game.final_title);
    tid_label->setText(tr("Title ID: %1").arg(game.serial_id.isEmpty() ? tr("Не указан") : game.serial_id));
    version_badge->setText(tr("Версия: %1").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version));
    size_badge->setText(tr("Размер: %1").arg(game.size.isEmpty() ? tr("Неизвестно") : game.size));
    lang_badge->setText(tr("Язык: %1").arg(game.text_langs.isEmpty() ? tr("Multi") : game.text_langs.join(QStringLiteral(", "))));

    version_combo->clear();
    const QString ext_tag = game.real_extension.isEmpty() ? QStringLiteral("NSP") : game.real_extension.mid(1).toUpper();
    version_combo->addItem(tr("Основная версия (%1) [%2]").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(ext_tag));

    // Fallback description from TitleDB if site description is not yet fetched
    if (!game.serial_id.isEmpty()) {
        bool ok = false;
        const u64 tid = game.serial_id.toULongLong(&ok, 16);
        if (ok) {
            const auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid);
            if (entry && !entry->description.empty()) {
                description_browser->setHtml(QStringLiteral("<p style='line-height: 1.4; color: #E0E8F0;'>%1</p>")
                    .arg(QString::fromStdString(entry->description)));
                return;
            }
        }
    }

    description_browser->setHtml(QStringLiteral("<p style='color: #8899A6;'>%1<br><br><b>Платформа:</b> Nintendo Switch (CONSOLES)<br><b>Идентификатор:</b> %2<br><b>Размер пакета:</b> %3</p>")
        .arg(tr("Информация о пакете и файлах получена с сервера stormgamesworld.ru."))
        .arg(game.serial_id)
        .arg(game.size));
}

void StormGamesWorldDialog::FetchGameDetails(int game_id) {
    if (details_reply) {
        details_reply->abort();
        details_reply->deleteLater();
    }

    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games?id=%1").arg(game_id)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.5 (Windows x64)"));
    details_reply = network_mgr.get(req);
    connect(details_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnGameDetailsReplyFinished);
}

void StormGamesWorldDialog::OnGameDetailsReplyFinished() {
    if (!details_reply) return;
    if (details_reply->error() == QNetworkReply::NoError) {
        const QByteArray raw_data = details_reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw_data);
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            const QString desc = obj[QStringLiteral("description")].toString();
            if (!desc.isEmpty()) {
                description_browser->setHtml(QStringLiteral("<p style='line-height: 1.4; color: #E0E8F0;'>%1</p>").arg(desc));
            }
            const qint64 bytes = obj[QStringLiteral("fileSizeBytes")].toVariant().toLongLong();
            if (bytes > 0 && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].file_size_bytes = bytes;
            }
        }
    }
    details_reply->deleteLater();
    details_reply = nullptr;
}

void StormGamesWorldDialog::FetchRealExtension(int game_id) {
    if (head_reply) {
        head_reply->abort();
        head_reply->deleteLater();
    }

    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games/%1/download").arg(game_id)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.5 (Windows x64)"));
    head_reply = network_mgr.head(req);
    connect(head_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnHeadReplyFinished);
}

void StormGamesWorldDialog::OnHeadReplyFinished() {
    if (!head_reply) return;

    if (head_reply->error() == QNetworkReply::NoError) {
        const QString disp = QString::fromUtf8(head_reply->rawHeader("Content-Disposition"));
        QString real_ext = QStringLiteral(".nsp");
        QString real_tag = QStringLiteral("NSP");

        if (disp.contains(QLatin1String(".nsz"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".nsz");
            real_tag = QStringLiteral("NSZ");
        } else if (disp.contains(QLatin1String(".xci"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".xci");
            real_tag = QStringLiteral("XCI");
        } else if (disp.contains(QLatin1String(".nsp"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".nsp");
            real_tag = QStringLiteral("NSP");
        }

        if (selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
            filtered_games[selected_game_index].real_extension = real_ext;
            const auto& game = filtered_games[selected_game_index];
            version_combo->clear();
            version_combo->addItem(tr("Основная версия (%1) [%2]").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(real_tag));
        }
    }

    head_reply->deleteLater();
    head_reply = nullptr;
}

void StormGamesWorldDialog::LoadCover(const StormWorldGame& game) {
    if (cover_reply) {
        cover_reply->abort();
        cover_reply->deleteLater();
        cover_reply = nullptr;
    }

    // Default stylized icon placeholder
    QPixmap placeholder(160, 160);
    placeholder.fill(QColor(14, 20, 32));
    QPainter p(&placeholder);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(0, 210, 255, 180));
    p.drawRoundedRect(1, 1, 158, 158, 8, 8);
    p.setPen(QColor(255, 255, 255, 220));
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(10);
    p.setFont(f);
    p.drawText(QRect(6, 35, 148, 80), Qt::AlignCenter | Qt::TextWordWrap, game.final_title.isEmpty() ? game.title : game.final_title);
    p.setPen(QColor(0, 240, 255, 160));
    f.setPointSize(8);
    p.setFont(f);
    p.drawText(QRect(6, 125, 148, 25), Qt::AlignCenter, game.serial_id);
    p.end();

    cover_label->setPixmap(placeholder);

    // Cache directory: covers/switch
    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "covers" / "switch";
    const QString cache_dir_q = QString::fromStdString(Common::FS::PathToUTF8String(cache_dir));
    QDir().mkpath(cache_dir_q);

    const QString tid = game.serial_id.toUpper();
    current_cover_cache_file = QDir(cache_dir_q).filePath(tid.isEmpty() ? QStringLiteral("game_%1.jpg").arg(game.id) : QStringLiteral("%1.jpg").arg(tid));

    // 1. Check local cache
    if (QFile::exists(current_cover_cache_file)) {
        QFileInfo fi(current_cover_cache_file);
        if (fi.size() > 500) {
            QPixmap cached_pix;
            if (cached_pix.load(current_cover_cache_file)) {
                cover_label->setPixmap(cached_pix.scaled(160, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                return;
            }
        }
    }

    // 2. Build multi-source candidate URLs matching STORM SWITCH BOX CoverCacheService
    current_cover_candidates.clear();
    current_cover_candidate_index = 0;

    // A. TitleDB Official Nintendo eShop Icon
    if (!tid.isEmpty()) {
        bool ok = false;
        const u64 tid_val = tid.toULongLong(&ok, 16);
        if (ok) {
            const auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val);
            if (entry && !entry->icon_url.empty()) {
                current_cover_candidates.append(QString::fromStdString(entry->icon_url));
            }
        }
    }

    // B. Storm Games World Cover URL
    if (!game.cover_url.isEmpty()) {
        current_cover_candidates.append(game.cover_url.startsWith(QLatin1String("http"))
            ? game.cover_url
            : QStringLiteral("https://stormgamesworld.ru") + game.cover_url);
    }

    // C. LibRetro Switch Boxart CDN
    const QString raw_title = game.final_title.isEmpty() ? game.title : game.final_title;
    const QString clean_title = SanitizeLibretroName(raw_title);
    if (!clean_title.isEmpty()) {
        const QString esc_title = QString::fromUtf8(QUrl::toPercentEncoding(clean_title));
        current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1.png").arg(esc_title));

        // Base title without parenthesis
        static const QRegularExpression reg(QStringLiteral("\\s*\\([^)]*\\)"));
        const QString base_title = clean_title.split(reg).first().trimmed();
        if (!base_title.isEmpty() && base_title != clean_title) {
            const QString esc_base = QString::fromUtf8(QUrl::toPercentEncoding(base_title));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(USA).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(World).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(Europe).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1.png").arg(esc_base));
        }
    }

    // D. Tinfoil & GameTDB CDNs
    if (!tid.isEmpty()) {
        current_cover_candidates.append(QStringLiteral("https://tinfoil.media/repo/db/icons/%1.jpg").arg(tid));
        current_cover_candidates.append(QStringLiteral("https://art.gametdb.com/switch/coverM/US/%1.jpg").arg(tid));
    }

    TryNextCoverCandidate();
}

void StormGamesWorldDialog::TryNextCoverCandidate() {
    if (current_cover_candidate_index >= current_cover_candidates.size()) {
        return;
    }

    const QString url_str = current_cover_candidates[current_cover_candidate_index++];
    QNetworkRequest req{QUrl(url_str)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/122.0.0.0 Safari/537.36"));

    cover_reply = network_mgr.get(req);
    connect(cover_reply, &QNetworkReply::finished, this, [this]() {
        if (!cover_reply) return;

        if (cover_reply->error() == QNetworkReply::NoError) {
            const QByteArray data = cover_reply->readAll();
            if (data.size() > 300) {
                QPixmap pix;
                if (pix.loadFromData(data)) {
                    QFile out(current_cover_cache_file);
                    if (out.open(QIODevice::WriteOnly)) {
                        out.write(data);
                        out.close();
                    }

                    cover_label->setPixmap(pix.scaled(160, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                    cover_reply->deleteLater();
                    cover_reply = nullptr;
                    return;
                }
            }
        }

        cover_reply->deleteLater();
        cover_reply = nullptr;

        TryNextCoverCandidate();
    });
}

void StormGamesWorldDialog::OnBrowseDownloadFolder() {
    const QString current = download_dir_edit->text();
    const QString selected = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите папку для сохранения игр"),
        current.isEmpty() ? GetDefaultDownloadDir() : current,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!selected.isEmpty()) {
        download_dir_edit->setText(selected);
        SaveDownloadDir(selected);
        PopulateGameList(search_edit->text());
    }
}

void StormGamesWorldDialog::OnOpenDownloadFolder() {
    const QString dir = download_dir_edit->text();
    if (QDir(dir).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(GetDefaultDownloadDir()));
    }
}

void StormGamesWorldDialog::OnStartDownload() {
    if (selected_game_index < 0 || selected_game_index >= static_cast<int>(filtered_games.size())) {
        return;
    }

    const auto& game = filtered_games[selected_game_index];
    const QString dir_path = download_dir_edit->text().trimmed();
    QDir dir(dir_path);
    if (!dir.exists()) {
        dir.mkpath(dir_path);
    }

    QString base_filename = SanitizeFileName(game.final_title.isEmpty() ? game.title : game.final_title);
    if (!base_filename.endsWith(QStringLiteral(".nsp"), Qt::CaseInsensitive) &&
        !base_filename.endsWith(QStringLiteral(".xci"), Qt::CaseInsensitive) &&
        !base_filename.endsWith(QStringLiteral(".nsz"), Qt::CaseInsensitive)) {
        base_filename += (game.real_extension.isEmpty() ? QStringLiteral(".nsp") : game.real_extension);
    }

    current_download_path = dir.filePath(base_filename);

    output_file = std::make_unique<QFile>(current_download_path);
    if (!output_file->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Ошибка записи файла"),
            tr("Не удалось создать файл для сохранения:\n%1").arg(current_download_path));
        return;
    }

    const QUrl download_url(QStringLiteral("https://stormgamesworld.ru/api/games/%1/download").arg(game.id));
    QNetworkRequest req(download_url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.5 (Windows x64)"));

    is_downloading = true;
    current_download_game_id = game.id;
    download_btn->setEnabled(false);
    cancel_btn->setEnabled(true);
    refresh_btn->setEnabled(false);
    progress_bar->setValue(0);
    download_status_label->setText(tr("Подключение к серверу загрузки..."));

    PopulateGameList(search_edit->text());

    download_timer.start();
    last_received_bytes = 0;
    last_speed_time = 0;

    download_reply = network_mgr.get(req);
    connect(download_reply, &QNetworkReply::readyRead, this, &StormGamesWorldDialog::OnDownloadDataReady);
    connect(download_reply, &QNetworkReply::downloadProgress, this, &StormGamesWorldDialog::OnDownloadProgress);
    connect(download_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnDownloadReplyFinished);
}

void StormGamesWorldDialog::OnDownloadDataReady() {
    if (download_reply && output_file && output_file->isOpen()) {
        output_file->write(download_reply->readAll());
    }
}

void StormGamesWorldDialog::OnDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        const int pct = static_cast<int>((received * 100) / total);
        progress_bar->setValue(pct);
    }

    const qint64 elapsed = download_timer.elapsed();
    if (elapsed - last_speed_time > 600) {
        const qint64 bytes_delta = received - last_received_bytes;
        const qint64 time_delta = elapsed - last_speed_time;
        if (time_delta > 0) {
            current_speed_mbps = (static_cast<double>(bytes_delta) / (1024.0 * 1024.0)) / (static_cast<double>(time_delta) / 1000.0);
        }
        last_received_bytes = received;
        last_speed_time = elapsed;
    }

    const double rec_mb = static_cast<double>(received) / (1024.0 * 1024.0);
    const double tot_mb = total > 0 ? (static_cast<double>(total) / (1024.0 * 1024.0)) : 0.0;

    QString status;
    if (total > 0) {
        const double rem_mb = tot_mb - rec_mb;
        const int eta_sec = (current_speed_mbps > 0.05) ? static_cast<int>(rem_mb / current_speed_mbps) : 0;
        status = tr("Скачано: %1 МБ из %2 МБ • Скорость: %3 МБ/с • Ост: %4 мин %5 сек")
            .arg(rec_mb, 0, 'f', 1)
            .arg(tot_mb, 0, 'f', 1)
            .arg(current_speed_mbps, 0, 'f', 2)
            .arg(eta_sec / 60)
            .arg(eta_sec % 60);
    } else {
        status = tr("Скачано: %1 МБ • Скорость: %2 МБ/с")
            .arg(rec_mb, 0, 'f', 1)
            .arg(current_speed_mbps, 0, 'f', 2);
    }

    download_status_label->setText(status);
}

void StormGamesWorldDialog::OnCancelDownload() {
    if (download_reply) {
        download_reply->abort();
    }
}

void StormGamesWorldDialog::OnDownloadReplyFinished() {
    is_downloading = false;
    current_download_game_id = -1;
    cancel_btn->setEnabled(false);
    download_btn->setEnabled(true);
    refresh_btn->setEnabled(true);

    if (output_file) {
        output_file->flush();
        output_file->close();
    }

    if (!download_reply) return;

    if (download_reply->error() == QNetworkReply::OperationCanceledError) {
        download_status_label->setText(tr("Загрузка отменена пользователем"));
        if (output_file) {
            output_file->remove();
        }
    } else if (download_reply->error() != QNetworkReply::NoError) {
        download_status_label->setText(tr("Ошибка загрузки: %1").arg(download_reply->errorString()));
        QMessageBox::critical(this, tr("Сбой скачивания"),
            tr("Произошла ошибка при загрузке игры с сервера:\n%1").arg(download_reply->errorString()));
    } else {
        progress_bar->setValue(100);
        download_status_label->setText(tr("✅ Загрузка успешно завершена! Файл сохранен в: %1").arg(current_download_path));

        emit GameDownloaded(current_download_path);

        QMessageBox::information(this, tr("Загрузка завершена"),
            tr("Игра успешно скачана в папку:\n%1\n\nВы можете запустить её прямо сейчас или открыть папку с файлом.")
            .arg(current_download_path));
    }

    PopulateGameList(search_edit->text());

    download_reply->deleteLater();
    download_reply = nullptr;
}
