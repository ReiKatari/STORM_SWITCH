// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/storm_games_world_dialog.h"
#include "storm_switch/storm_catalog_cache.h"

#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QMouseEvent>
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

static int ExtractDlcCount(const QString& title, u64 title_id) {
    static const QRegularExpression dlc_regex(
        QStringLiteral(R"((?:\+|\(|\[|\b)(\d+)\s*(?:D\b|DLC\b))"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = dlc_regex.match(title);
    if (match.hasMatch()) {
        bool ok = false;
        const int count = match.captured(1).toInt(&ok);
        if (ok && count > 0) {
            return count;
        }
    }

    if (title_id != 0) {
        TitleDB::TitleDatabase::Instance().EnsureLoaded();
        const int db_count = TitleDB::TitleDatabase::Instance().GetDlcCount(title_id);
        if (db_count > 0) {
            return db_count;
        }
    }
    return 0;
}

class StormWorldDlcListDialog : public QDialog {
public:
    StormWorldDlcListDialog(QWidget* parent, const StormWorldGame& game)
        : QDialog(parent) {
        const QString disp_title = game.final_title.isEmpty() ? game.title : game.final_title;
        setWindowTitle(tr("STORM SWITCH — Дополнения: %1").arg(disp_title));
        resize(740, 500);
        setMinimumSize(620, 380);

        setStyleSheet(QStringLiteral(
            "QDialog {"
            "    background-color: #0c1017;"
            "    color: #e2e8f0;"
            "    font-family: 'Segoe UI';"
            "}"
            "QFrame#HeaderBox {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #16202e, stop:1 #0e141d);"
            "    border: 1px solid #1e293b;"
            "    border-radius: 10px;"
            "    padding: 10px;"
            "}"
            "QTreeWidget {"
            "    background-color: #080b10;"
            "    border: 1px solid #1e293b;"
            "    border-radius: 8px;"
            "    color: #f1f5f9;"
            "    alternate-background-color: #0d121a;"
            "    outline: none;"
            "}"
            "QTreeWidget::item {"
            "    height: 32px;"
            "    padding: 4px;"
            "}"
            "QTreeWidget::item:hover {"
            "    background: rgba(0, 210, 255, 0.12);"
            "}"
            "QTreeWidget::item:selected {"
            "    background: rgba(0, 240, 255, 0.22);"
            "    color: #00F0FF;"
            "}"
            "QHeaderView::section {"
            "    background-color: #111722;"
            "    color: #00F0FF;"
            "    font-weight: bold;"
            "    padding: 6px;"
            "    border: none;"
            "    border-bottom: 1px solid #1e293b;"
            "}"
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00B4D8, stop:1 #00F0FF);"
            "    border: none;"
            "    border-radius: 6px;"
            "    color: #050a12;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "    padding: 8px 26px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00C6EB, stop:1 #33F5FF);"
            "}"
        ));

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(12);

        // Header Box
        auto* header_box = new QFrame(this);
        header_box->setObjectName(QStringLiteral("HeaderBox"));
        auto* header_layout = new QHBoxLayout(header_box);
        header_layout->setContentsMargins(14, 10, 14, 10);
        header_layout->setSpacing(14);

        auto* icon_label = new QLabel(QStringLiteral("📦"), header_box);
        icon_label->setStyleSheet(QStringLiteral("font-size: 28px; background: transparent;"));
        header_layout->addWidget(icon_label);

        auto* header_text_layout = new QVBoxLayout();
        header_text_layout->setSpacing(3);

        auto* title_lbl = new QLabel(disp_title, header_box);
        title_lbl->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: #FFFFFF;"));
        title_lbl->setWordWrap(true);
        header_text_layout->addWidget(title_lbl);

        auto* meta_lbl = new QLabel(
            tr("Title ID: %1  •  Всего дополнений (DLC): %2")
                .arg(game.serial_id.isEmpty() ? tr("—") : game.serial_id)
                .arg(game.dlc_count),
            header_box);
        meta_lbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #00F0FF; font-family: monospace; font-weight: bold;"));
        header_text_layout->addWidget(meta_lbl);

        header_layout->addLayout(header_text_layout, 1);
        layout->addWidget(header_box);

        // Tree
        auto* tree = new QTreeWidget(this);
        tree->setHeaderLabels({
            tr("№"),
            tr("Title ID"),
            tr("Название дополнения"),
            tr("Версия"),
            tr("Статус")
        });
        tree->setRootIsDecorated(false);
        tree->setAlternatingRowColors(true);

        tree->headerItem()->setTextAlignment(0, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(1, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
        tree->headerItem()->setTextAlignment(3, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(4, Qt::AlignCenter);

        tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
        tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

        bool ok = false;
        const u64 tid = game.serial_id.toULongLong(&ok, 16);
        std::vector<TitleDB::Entry> dlcs;

        if (!game.dlcs.isEmpty()) {
            for (int i = 0; i < game.dlcs.size(); ++i) {
                const auto& d = game.dlcs[i];
                auto* item = new QTreeWidgetItem(tree);
                item->setText(0, QString::number(i + 1));
                item->setTextAlignment(0, Qt::AlignCenter);
                item->setText(1, d.id.isEmpty() ? (ok ? QStringLiteral("%1").arg(tid + 0x1000ULL + static_cast<u64>(i + 1), 16, 16, QLatin1Char('0')).toUpper() : QStringLiteral("—")) : d.id.toUpper());
                item->setText(2, d.name.isEmpty() ? tr("Официальное дополнение (DLC #%1)").arg(i + 1) : d.name);
                item->setText(3, d.version.isEmpty() ? (game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version) : d.version);
                item->setText(4, tr("Доступно в пакете"));

                item->setTextAlignment(1, Qt::AlignCenter);
                item->setTextAlignment(3, Qt::AlignCenter);
                item->setTextAlignment(4, Qt::AlignCenter);
                item->setForeground(1, QBrush(QColor(QStringLiteral("#00D2FF"))));
                item->setForeground(2, QBrush(QColor(QStringLiteral("#FFFFFF"))));
                item->setForeground(4, QBrush(QColor(QStringLiteral("#00FF66"))));
            }
        } else {
            if (ok && tid != 0) {
                TitleDB::TitleDatabase::Instance().WaitLoaded(std::chrono::milliseconds(1500));
                dlcs = TitleDB::TitleDatabase::Instance().GetDlcs(tid);
            }

            const int total_items = std::max(static_cast<int>(dlcs.size()), game.dlc_count);
            if (total_items > 0) {
                for (int i = 0; i < total_items; ++i) {
                    auto* item = new QTreeWidgetItem(tree);
                    item->setText(0, QString::number(i + 1));
                    item->setTextAlignment(0, Qt::AlignCenter);

                    if (i < static_cast<int>(dlcs.size())) {
                        const auto& d = dlcs[i];
                        item->setText(1, QString::fromStdString(d.id).toUpper());
                        item->setText(2, QString::fromStdString(d.name.empty() ? ("DLC Pack " + std::to_string(i + 1)) : d.name));
                        item->setText(3, QString::fromStdString(d.version.empty() ? "1.0.0" : d.version));
                        item->setText(4, tr("Доступно в пакете"));
                    } else {
                        const u64 dlc_tid = ok ? (tid + 0x1000ULL + static_cast<u64>(i)) : 0ULL;
                        item->setText(1, dlc_tid != 0 ? QStringLiteral("%1").arg(dlc_tid, 16, 16, QLatin1Char('0')).toUpper() : QStringLiteral("—"));
                        item->setText(2, tr("Официальный контент (DLC #%1)").arg(i + 1));
                        item->setText(3, game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version);
                        item->setText(4, tr("Включено в релиз"));
                    }

                    item->setTextAlignment(1, Qt::AlignCenter);
                    item->setTextAlignment(3, Qt::AlignCenter);
                    item->setTextAlignment(4, Qt::AlignCenter);
                    item->setForeground(1, QBrush(QColor(QStringLiteral("#00D2FF"))));
                    item->setForeground(2, QBrush(QColor(QStringLiteral("#FFFFFF"))));
                    item->setForeground(4, QBrush(QColor(QStringLiteral("#00FF66"))));
                }
            } else {
                auto* item = new QTreeWidgetItem(tree);
                item->setText(0, QStringLiteral("—"));
                item->setText(1, QStringLiteral("—"));
                item->setText(2, tr("Для данной игры официальные дополнения не обнаружены"));
                item->setText(3, QStringLiteral("—"));
                item->setText(4, tr("Нет данных"));
                item->setForeground(2, QBrush(QColor(QStringLiteral("#94a3b8"))));
            }
        }

        layout->addWidget(tree, 1);

        // Close Button
        auto* btn_layout = new QHBoxLayout();
        btn_layout->addStretch(1);
        auto* close_btn = new QPushButton(tr("Закрыть"), this);
        close_btn->setCursor(Qt::PointingHandCursor);
        connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
        btn_layout->addWidget(close_btn);
        layout->addLayout(btn_layout);
    }
};

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

    const QString my_key = !game.serial_id.trimmed().isEmpty() ? game.serial_id.trimmed().toUpper()
        : (game.final_title.isEmpty() ? game.title.trimmed().toLower() : game.final_title.trimmed().toLower());

    int same_group_count = 0;
    for (const auto& other : all_games) {
        const QString other_key = !other.serial_id.trimmed().isEmpty() ? other.serial_id.trimmed().toUpper()
            : (other.final_title.isEmpty() ? other.title.trimmed().toLower() : other.final_title.trimmed().toLower());
        if (other_key == my_key) {
            same_group_count++;
        }
    }

    const QString full_game_name = (game.final_title + QLatin1Char(' ') + game.title).toLower();
    const bool game_is_rus = full_game_name.contains(QStringLiteral("rus"));
    const bool game_is_mod = full_game_name.contains(QStringLiteral("mod"));

    for (const auto& file : files) {
        const QString lower_file = file.toLower();
        if (!lower_file.endsWith(QStringLiteral(".nsp")) &&
            !lower_file.endsWith(QStringLiteral(".xci")) &&
            !lower_file.endsWith(QStringLiteral(".nsz"))) {
            continue;
        }

        const QFileInfo fi(dir.filePath(file));
        if (fi.size() < 1024 * 1024) continue;

        // 1. Direct title matches
        if (!title1.isEmpty() && lower_file.contains(title1)) {
            return true;
        }
        if (!title2.isEmpty() && lower_file.contains(title2)) {
            return true;
        }

        // 2. Title ID match
        if (!tid.isEmpty() && lower_file.contains(tid)) {
            if (same_group_count <= 1) {
                return true;
            }

            // Multiple versions in catalog: must match language/mod and version traits
            const bool file_has_rus = lower_file.contains(QStringLiteral("rus"));
            const bool file_has_mod = lower_file.contains(QStringLiteral("mod"));
            if (game_is_rus == file_has_rus && game_is_mod == file_has_mod) {
                if (!game.version.isEmpty() && game.version != QStringLiteral("1.0.0")) {
                    if (lower_file.contains(game.version.toLower())) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
        }
    }

    return false;
}

void StormGamesWorldDialog::SetupUI() {
    setWindowTitle(tr("🌐 STORM SWITCH — Каталог и менеджер игр STORM GAMES WORLD"));
    resize(1380, 800);
    setMinimumSize(1160, 680);

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
        tr("Дополнения"),
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
    games_tree->headerItem()->setTextAlignment(6, Qt::AlignCenter);

    games_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    games_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    games_tree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    connect(games_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        if (column == 3) {
            ShowDlcListForCurrentGame();
        }
    });
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

    internal_version_badge = new QLabel(tr("Сборка: —"), details_panel);
    internal_version_badge->setStyleSheet(QStringLiteral("background: rgba(168, 85, 247, 0.15); border: 1px solid #A855F7; border-radius: 4px; padding: 2px 8px; color: #C084FC; font-size: 11px; font-weight: bold; font-family: monospace;"));
    badges_layout->addWidget(internal_version_badge);

    size_badge = new QLabel(tr("Размер: —"), details_panel);
    size_badge->setStyleSheet(QStringLiteral("background: rgba(0, 255, 102, 0.15); border: 1px solid #00FF66; border-radius: 4px; padding: 2px 8px; color: #00FF66; font-size: 11px; font-weight: bold;"));
    badges_layout->addWidget(size_badge);

    dlc_badge = new QLabel(tr("Дополнения: —"), details_panel);
    dlc_badge->setCursor(Qt::PointingHandCursor);
    dlc_badge->setStyleSheet(QStringLiteral("background: rgba(0, 240, 255, 0.15); border: 1px solid #00F0FF; border-radius: 4px; padding: 2px 8px; color: #00F0FF; font-size: 11px; font-weight: bold;"));
    dlc_badge->installEventFilter(this);
    badges_layout->addWidget(dlc_badge);

    lang_badge = new QLabel(tr("Язык: —"), details_panel);
    lang_badge->setStyleSheet(QStringLiteral("background: rgba(245, 158, 11, 0.15); border: 1px solid #F59E0B; border-radius: 4px; padding: 2px 8px; color: #F59E0B; font-size: 11px; font-weight: bold;"));
    badges_layout->addWidget(lang_badge);

    badges_layout->addStretch(1);
    meta_layout->addLayout(badges_layout);

    // Version selector dropdown
    auto* ver_select_layout = new QHBoxLayout();
    ver_select_layout->setSpacing(6);
    auto* ver_title = new QLabel(tr("Доступная версия:"), details_panel);
    ver_title->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: bold;"));
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
    download_status_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 12px; font-weight: 500;"));
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
    splitter->setSizes({740, 640});

    root_layout->addWidget(splitter, 1);

    // --- Bottom Status Bar ---
    auto* bottom_bar = new QHBoxLayout();
    status_label = new QLabel(tr("Подключение к порталу stormgamesworld.ru..."), this);
    status_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 12px;"));
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
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.7 (Windows x64)"));

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
        const QString size_str = obj[QStringLiteral("size")].toString().trimmed();

        // Strict requirement: Nintendo Switch CONSOLES only, and ONLY games with existing files!
        if (platform == QStringLiteral("Nintendo Switch") &&
            platform_type == QStringLiteral("CONSOLES") &&
            file_exists && has_file &&
            !size_str.isEmpty() && size_str != QStringLiteral("—")) {

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

            bool ok_tid = false;
            const u64 num_tid = g.serial_id.toULongLong(&ok_tid, 16);
            g.dlc_count = ExtractDlcCount(g.title, ok_tid ? num_tid : 0ULL);
            if (g.dlc_count == 0 && !g.final_title.isEmpty()) {
                g.dlc_count = ExtractDlcCount(g.final_title, ok_tid ? num_tid : 0ULL);
            }

            const QJsonArray reg_arr = obj[QStringLiteral("regions")].toArray();
            for (const auto& r : reg_arr) g.regions.append(r.toString());

            const QJsonArray lang_arr = obj[QStringLiteral("textLangs")].toArray();
            for (const auto& l : lang_arr) g.text_langs.append(l.toString());

            all_games.push_back(g);
        }
    }

    // Save catalog to cache for update notifications across main screen and cards
    StormCatalogCache::Instance().SaveCache(raw_data);

    // Group games by key to determine recommendations
    QMap<QString, QVector<int>> game_groups;
    for (int i = 0; i < static_cast<int>(all_games.size()); ++i) {
        const auto& g = all_games[i];
        const QString key = !g.serial_id.trimmed().isEmpty() ? g.serial_id.trimmed().toUpper()
            : (g.final_title.isEmpty() ? g.title.trimmed().toLower() : g.final_title.trimmed().toLower());
        game_groups[key].push_back(i);
    }

    auto calc_priority = [](const StormWorldGame& g) -> int {
        const QString t = (g.final_title + QLatin1Char(' ') + g.title).toUpper();
        if (t.contains(QStringLiteral("MOD - RUS")) || t.contains(QStringLiteral("MOD - M. RUS")) ||
            t.contains(QStringLiteral("MOD-RUS")) || t.contains(QStringLiteral("MOD - M.RUS"))) {
            return 300;
        }
        if (t.contains(QStringLiteral("[RUS]")) || t.contains(QStringLiteral("(RUS)")) ||
            t.contains(QStringLiteral(" RUS ")) || t.endsWith(QStringLiteral(" RUS"))) {
            return 200;
        }
        for (const auto& l : g.text_langs) {
            const QString lu = l.toUpper();
            if (lu == QStringLiteral("RUS") || lu.contains(QStringLiteral("RUSSIAN")) || lu.contains(QStringLiteral("РУССКИЙ"))) {
                return 200;
            }
        }
        return 100;
    };

    for (auto it = game_groups.begin(); it != game_groups.end(); ++it) {
        const auto& indices = it.value();
        // If only 1 version exists: do NOT display Recommended badge
        if (indices.size() <= 1) {
            for (int idx : indices) {
                all_games[idx].is_recommended = false;
            }
            continue;
        }

        // Multiple versions exist: pick strictly ONE recommended version based on priority:
        // Priority: MOD - RUS / MOD - M. RUS (300) > RUS (200) > ENG / other (100)
        // Tie-breaker: highest version -> highest ID
        int best_idx = -1;
        int best_priority = -1;
        QString best_version;
        int best_id = -1;

        for (int idx : indices) {
            all_games[idx].is_recommended = false;
            const auto& g = all_games[idx];
            const int prio = calc_priority(g);

            bool is_better = false;
            if (best_idx == -1) {
                is_better = true;
            } else if (prio > best_priority) {
                is_better = true;
            } else if (prio == best_priority) {
                const int cmp = StormCatalogCache::CompareVersions(g.version, best_version);
                if (cmp > 0) {
                    is_better = true;
                } else if (cmp == 0 && g.id > best_id) {
                    is_better = true;
                }
            }

            if (is_better) {
                best_idx = idx;
                best_priority = prio;
                best_version = g.version;
                best_id = g.id;
            }
        }

        if (best_idx >= 0) {
            all_games[best_idx].is_recommended = true;
        }
    }

    // Sort all_games so recommended version appears at the top within its group, followed by higher versions
    std::stable_sort(all_games.begin(), all_games.end(), [&calc_priority](const StormWorldGame& a, const StormWorldGame& b) {
        const QString key_a = !a.serial_id.trimmed().isEmpty() ? a.serial_id.trimmed().toUpper() : (a.final_title.isEmpty() ? a.title.trimmed().toLower() : a.final_title.trimmed().toLower());
        const QString key_b = !b.serial_id.trimmed().isEmpty() ? b.serial_id.trimmed().toUpper() : (b.final_title.isEmpty() ? b.title.trimmed().toLower() : b.final_title.trimmed().toLower());
        if (key_a == key_b) {
            if (a.is_recommended != b.is_recommended) {
                return a.is_recommended;
            }
            const int prio_a = calc_priority(a);
            const int prio_b = calc_priority(b);
            if (prio_a != prio_b) {
                return prio_a > prio_b;
            }
            return StormCatalogCache::CompareVersions(a.version, b.version) > 0;
        }
        return false;
    });

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
        item->setForeground(0, QBrush(QColor(QStringLiteral("#FFFFFF"))));

        if (g.is_recommended) {
            item->setText(1, QStringLiteral("⭐ %1 [Рекомендуемая]").arg(g.version.isEmpty() ? QStringLiteral("1.0.0") : g.version));
            item->setForeground(1, QBrush(QColor(QStringLiteral("#00FF66"))));
            item->setToolTip(1, tr("Рекомендуемая новейшая версия игры"));
        } else {
            item->setText(1, g.version.isEmpty() ? tr("1.0.0") : g.version);
            item->setForeground(1, QBrush(QColor(QStringLiteral("#FFFFFF"))));
        }

        item->setText(2, g.size.isEmpty() ? tr("—") : g.size);
        item->setForeground(2, QBrush(QColor(QStringLiteral("#FFFFFF"))));

        if (g.dlc_count > 0) {
            item->setText(3, tr("📦 %1 DLC").arg(g.dlc_count));
            item->setForeground(3, QBrush(QColor(QStringLiteral("#00F0FF"))));
            item->setToolTip(3, tr("Нажмите для просмотра списка дополнений"));
        } else {
            item->setText(3, QStringLiteral("—"));
            item->setForeground(3, QBrush(QColor(QStringLiteral("#FFFFFF"))));
        }

        item->setText(4, langs);
        item->setForeground(4, QBrush(QColor(QStringLiteral("#FFFFFF"))));

        item->setText(5, g.serial_id.isEmpty() ? tr("—") : g.serial_id);
        item->setForeground(5, QBrush(QColor(QStringLiteral("#00F0FF"))));

        // Center align columns 1 to 6
        item->setTextAlignment(1, Qt::AlignCenter);
        item->setTextAlignment(2, Qt::AlignCenter);
        item->setTextAlignment(3, Qt::AlignCenter);
        item->setTextAlignment(4, Qt::AlignCenter);
        item->setTextAlignment(5, Qt::AlignCenter);
        item->setTextAlignment(6, Qt::AlignCenter);

        // Check download status
        if (IsGameDownloaded(g, current_dir)) {
            item->setText(6, tr("✅ Скачано"));
            item->setForeground(6, QBrush(QColor(QStringLiteral("#00FF66"))));
        } else if (is_downloading && current_download_game_id == g.id) {
            item->setText(6, tr("⏳ Загрузка"));
            item->setForeground(6, QBrush(QColor(QStringLiteral("#FFA500"))));
        } else {
            item->setText(6, tr("⚪ Доступно"));
            item->setForeground(6, QBrush(QColor(QStringLiteral("#FFFFFF"))));
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

static QString ExtractInternalVersion(const QString& title, const QString& version_str) {
    // 1. Look for explicit [v655360] or (655360) or [655360] in title
    static const QRegularExpression re_title(QStringLiteral(R"([\[\(vV]?(\d{5,8})[\]\)]?)"));
    QRegularExpressionMatchIterator it = re_title.globalMatch(title);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString num = match.captured(1);
        if (num.length() >= 5 && num.length() <= 8) {
            return num;
        }
    }

    // 2. Compute from version_str e.g. "1.0.10" or "v1.2.3"
    QString clean_v = version_str;
    if (clean_v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        clean_v.remove(0, 1);
    }
    const QStringList parts = clean_v.trimmed().split(QLatin1Char('.'));
    if (parts.size() >= 3) {
        bool ok1 = false, ok2 = false, ok3 = false;
        const int major = parts[0].toInt(&ok1);
        const int minor = parts[1].toInt(&ok2);
        const int patch = parts[2].toInt(&ok3);
        if (ok1 && ok2 && ok3) {
            quint32 internal_ver = ((quint32)std::max(0, major - 1) * 655360) +
                                   ((quint32)minor * 655360) +
                                   ((quint32)patch * 65536);
            if (internal_ver == 0 && (major > 1 || minor > 0 || patch > 0)) {
                internal_ver = static_cast<quint32>(patch) * 65536;
            }
            return QString::number(internal_ver);
        }
    } else if (parts.size() == 2) {
        bool ok1 = false, ok2 = false;
        const int major = parts[0].toInt(&ok1);
        const int minor = parts[1].toInt(&ok2);
        if (ok1 && ok2) {
            const quint32 internal_ver = ((quint32)std::max(0, major - 1) * 655360) + ((quint32)minor * 65536);
            return QString::number(internal_ver);
        }
    }
    return QStringLiteral("0");
}

void StormGamesWorldDialog::DisplayGameDetails(const StormWorldGame& game) {
    title_label->setText(game.final_title.isEmpty() ? game.title : game.final_title);
    tid_label->setText(tr("Title ID: %1").arg(game.serial_id.isEmpty() ? tr("Не указан") : game.serial_id));
    if (game.is_recommended) {
        version_badge->setText(tr("Версия: %1 ★ Рекомендуемая").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version));
        version_badge->setStyleSheet(QStringLiteral("background: rgba(0, 255, 102, 0.18); border: 1px solid #00FF66; border-radius: 4px; padding: 2px 8px; color: #00FF66; font-size: 11px; font-weight: bold;"));
    } else {
        version_badge->setText(tr("Версия: %1").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version));
        version_badge->setStyleSheet(QStringLiteral("background: rgba(0, 210, 255, 0.15); border: 1px solid #00D2FF; border-radius: 4px; padding: 2px 8px; color: #00F0FF; font-size: 11px; font-weight: bold;"));
    }
    const QString int_ver = ExtractInternalVersion(game.title, game.version);
    internal_version_badge->setText(tr("Сборка: %1").arg(int_ver));
    size_badge->setText(tr("Размер: %1").arg(game.size.isEmpty() ? tr("Неизвестно") : game.size));

    if (game.dlc_count > 0) {
        dlc_badge->setText(tr("📦 Дополнений: %1").arg(game.dlc_count));
        dlc_badge->setStyleSheet(QStringLiteral(
            "background: rgba(0, 240, 255, 0.15); border: 1px solid #00F0FF; "
            "border-radius: 4px; padding: 2px 8px; color: #00F0FF; font-size: 11px; font-weight: bold;"));
        dlc_badge->setToolTip(tr("Нажмите для просмотра списка дополнений"));
    } else {
        dlc_badge->setText(tr("Дополнений: 0"));
        dlc_badge->setStyleSheet(QStringLiteral(
            "background: rgba(255, 255, 255, 0.08); border: 1px solid #475569; "
            "border-radius: 4px; padding: 2px 8px; color: #FFFFFF; font-size: 11px; font-weight: bold;"));
        dlc_badge->setToolTip(QString());
    }

    lang_badge->setText(tr("Язык: %1").arg(game.text_langs.isEmpty() ? tr("Multi") : game.text_langs.join(QStringLiteral(", "))));

    version_combo->clear();
    const QString ext_tag = game.real_extension.isEmpty() ? QStringLiteral("NSP") : game.real_extension.mid(1).toUpper();
    const QString ver_item_text = game.is_recommended
        ? tr("Основная версия (%1) [%2] — Рекомендуемая").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(ext_tag)
        : tr("Основная версия (%1) [%2]").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(ext_tag);
    version_combo->addItem(ver_item_text);

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

bool StormGamesWorldDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == dlc_badge && event->type() == QEvent::MouseButtonRelease) {
        ShowDlcListForCurrentGame();
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void StormGamesWorldDialog::ShowDlcListForCurrentGame() {
    if (selected_game_index < 0 || selected_game_index >= static_cast<int>(filtered_games.size())) {
        return;
    }
    const auto& game = filtered_games[selected_game_index];
    StormWorldDlcListDialog dlg(this, game);
    dlg.exec();
}

void StormGamesWorldDialog::FetchGameDetails(int game_id) {
    if (details_reply) {
        details_reply->abort();
        details_reply->deleteLater();
    }

    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games?id=%1").arg(game_id)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.0.7 (Windows x64)"));
    details_reply = network_mgr.get(req);
    connect(details_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnGameDetailsReplyFinished);
}

void StormGamesWorldDialog::OnGameDetailsReplyFinished() {
    if (!details_reply) return;
    if (details_reply->error() == QNetworkReply::NoError) {
        const QByteArray raw_data = details_reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw_data);
        QJsonObject obj;
        if (doc.isObject()) {
            obj = doc.object();
        } else if (doc.isArray() && !doc.array().isEmpty()) {
            obj = doc.array().first().toObject();
        }

        if (!obj.isEmpty()) {
            const QString desc = obj[QStringLiteral("description")].toString();
            if (!desc.isEmpty()) {
                description_browser->setHtml(QStringLiteral("<p style='line-height: 1.4; color: #E0E8F0;'>%1</p>").arg(desc));
            }
            const qint64 bytes = obj[QStringLiteral("fileSizeBytes")].toVariant().toLongLong();
            if (bytes > 0 && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].file_size_bytes = bytes;
            }

            const QJsonArray dlcs_arr = obj[QStringLiteral("dlcs")].toArray();
            if (!dlcs_arr.isEmpty() && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].dlcs.clear();
                for (const auto& d_val : dlcs_arr) {
                    const QJsonObject d_obj = d_val.toObject();
                    StormWorldDlc dlc;
                    dlc.id = d_obj[QStringLiteral("id")].toString();
                    dlc.name = d_obj[QStringLiteral("name")].toString().trimmed();
                    dlc.description = d_obj[QStringLiteral("description")].toString().trimmed();
                    if (!dlc.name.isEmpty()) {
                        filtered_games[selected_game_index].dlcs.push_back(dlc);
                    }
                }
                if (!filtered_games[selected_game_index].dlcs.isEmpty()) {
                    filtered_games[selected_game_index].dlc_count = filtered_games[selected_game_index].dlcs.size();
                    dlc_badge->setText(tr("📦 Дополнений: %1").arg(filtered_games[selected_game_index].dlc_count));
                }
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
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.4.0 (Windows x64)"));
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
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.4.0 (Windows x64)"));
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("Connection", "keep-alive");
    req.setRawHeader("Accept-Encoding", "identity");

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
    download_reply->setReadBufferSize(16 * 1024 * 1024);
    connect(download_reply, &QNetworkReply::readyRead, this, &StormGamesWorldDialog::OnDownloadDataReady);
    connect(download_reply, &QNetworkReply::downloadProgress, this, &StormGamesWorldDialog::OnDownloadProgress);
    connect(download_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnDownloadReplyFinished);
}

void StormGamesWorldDialog::OnDownloadDataReady() {
    if (download_reply && output_file && output_file->isOpen()) {
        output_file->write(download_reply->readAll());
    }
}

static QString FormatRussianNumber(double value, int precision = 1) {
    QLocale ru(QLocale::Russian, QLocale::Russia);
    ru.setNumberOptions(QLocale::DefaultNumberOptions);
    QString str = ru.toString(value, 'f', precision);
    str.replace(QChar(0x00A0), QLatin1Char(' '));
    str.replace(QChar(0x202F), QLatin1Char(' '));
    return str;
}

void StormGamesWorldDialog::OnDownloadProgress(qint64 received, qint64 total) {
    int pct = 0;
    if (total > 0) {
        pct = static_cast<int>((received * 100) / total);
        progress_bar->setValue(pct);
    }

    const qint64 elapsed = download_timer.elapsed();
    if (elapsed - last_speed_time > 500) {
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
        const double rem_mb = std::max(0.0, tot_mb - rec_mb);
        const int eta_sec = (current_speed_mbps > 0.05) ? static_cast<int>(rem_mb / current_speed_mbps) : 0;
        const int eta_min = eta_sec / 60;
        const int eta_rem = eta_sec % 60;

        const QString line1 = tr("Скачано: %1 МБ из %2 МБ (%3%)")
            .arg(FormatRussianNumber(rec_mb, 1))
            .arg(FormatRussianNumber(tot_mb, 1))
            .arg(pct);
        const QString line2 = tr("Скорость: %1 МБ/с • Осталось: %2 мин %3 сек")
            .arg(FormatRussianNumber(current_speed_mbps, 1))
            .arg(eta_min)
            .arg(eta_rem);
        status = QStringLiteral("%1\n%2").arg(line1, line2);
    } else {
        const QString line1 = tr("Скачано: %1 МБ").arg(FormatRussianNumber(rec_mb, 1));
        const QString line2 = tr("Скорость: %1 МБ/с").arg(FormatRussianNumber(current_speed_mbps, 1));
        status = QStringLiteral("%1\n%2").arg(line1, line2);
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
