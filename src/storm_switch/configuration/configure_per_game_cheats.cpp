// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/configuration/configure_per_game_cheats.h"
#include "storm_switch/common/storm_theme.h"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/logging.h"
#include "common/settings.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/vfs/vfs.h"
#include "core/loader/loader.h"
#include "core/memory/cheat_engine.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"

namespace {

enum class CheatIconType {
    Search,
    Download,
    Plus,
    Folder,
    Check,
    Cross,
    Lightning,
};

QIcon MakeCheatIcon(CheatIconType type, const QColor& color, int size = 16) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case CheatIconType::Search:
        p.drawEllipse(QPointF(6.5, 6.5), 4.0, 4.0);
        p.drawLine(QPointF(9.5, 9.5), QPointF(13.5, 13.5));
        break;
    case CheatIconType::Download:
        p.drawLine(QPointF(8.0, 2.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(4.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(11.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(3.0, 13.5), QPointF(13.0, 13.5));
        break;
    case CheatIconType::Plus:
        p.drawLine(QPointF(8.0, 3.0), QPointF(8.0, 13.0));
        p.drawLine(QPointF(3.0, 8.0), QPointF(13.0, 8.0));
        break;
    case CheatIconType::Folder: {
        QPainterPath path;
        path.moveTo(2.5, 5.0);
        path.lineTo(6.0, 5.0);
        path.lineTo(7.5, 6.5);
        path.lineTo(13.5, 6.5);
        path.lineTo(13.5, 12.5);
        path.lineTo(2.5, 12.5);
        path.closeSubpath();
        p.drawPath(path);
        break;
    }
    case CheatIconType::Check:
        p.drawLine(QPointF(3.0, 8.0), QPointF(6.5, 11.5));
        p.drawLine(QPointF(6.5, 11.5), QPointF(13.0, 4.5));
        break;
    case CheatIconType::Cross:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    case CheatIconType::Lightning: {
        QPainterPath path;
        path.moveTo(8.5, 2.0);
        path.lineTo(4.5, 8.5);
        path.lineTo(8.0, 8.5);
        path.lineTo(7.0, 14.0);
        path.lineTo(12.0, 7.5);
        path.lineTo(8.5, 7.5);
        path.closeSubpath();
        p.setBrush(color);
        p.drawPath(path);
        break;
    }
    }
    return QIcon(pix);
}

bool IsBogusCheatText(const QString& content) {
    return content.contains(QStringLiteral("52800001")) &&
           content.contains(QStringLiteral("5280270F")) &&
           (content.contains(QStringLiteral("Infinite Health")) || content.contains(QStringLiteral("Бесконечное здоровье")));
}

} // namespace

ConfigurePerGameCheats::ConfigurePerGameCheats(Core::System& system_, u64 title_id_, const QString& file_name_, QWidget* parent)
    : QWidget(parent), system(system_), title_id(title_id_), file_name(file_name_) {

    if (system.IsPoweredOn() && system.GetApplicationProcessProgramID() == title_id) {
        build_id = system.GetApplicationProcessBuildID();
        build_id_str = QString::fromStdString(Common::HexToString(build_id)).left(16).toUpper();
    } else {
        const auto dir_path = GetCheatsDirectoryPath();
        QDir cdir(dir_path);
        const auto files = cdir.entryList(QStringList() << QStringLiteral("*.txt"), QDir::Files);
        for (const auto& fname : files) {
            QString base = fname.left(fname.lastIndexOf(QLatin1Char('.'))).trimmed().toUpper();
            if (base.length() == 16 && !base.startsWith(QStringLiteral("CHEAT"))) {
                build_id_str = base;
                break;
            }
        }
        if (build_id_str.isEmpty()) {
            build_id_str = QString(QStringLiteral("%1")).arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
        }
    }

    const auto pal = StormTheme::GetThemePalette();

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(8);

    // 1. Compact toolbar
    auto* toolbar_layout = new QHBoxLayout();
    toolbar_layout->setSpacing(6);

    search_field = new QLineEdit(this);
    search_field->setPlaceholderText(StormLang(
        QStringLiteral("Поиск по названию чита..."),
        QStringLiteral("Search cheat name...")
    ));
    search_field->setClearButtonEnabled(true);
    search_field->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background-color: %1;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  color: %3;"
        "  font-size: 9.5pt;"
        "  min-height: 28px;"
        "}"
        "QLineEdit:focus {"
        "  border: none;"
        "  background-color: %5;"
        "}"
    ).arg(pal.inputBg, pal.border, pal.textPrimary, pal.accent, pal.surface));
    toolbar_layout->addWidget(search_field, 1);

    download_button = new QPushButton(StormLang(QStringLiteral("Скачать читы"), QStringLiteral("Download Cheats")), this);
    download_button->setIcon(MakeCheatIcon(CheatIconType::Download, QColor(pal.accent)));
    download_button->setToolTip(StormLang(
        QStringLiteral("Скачать проверенные чит-коды из онлайн-баз Atmosphere"),
        QStringLiteral("Download verified cheats from Atmosphere online databases")
    ));
    download_button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-weight: 600;"
        "  font-size: 9pt;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "  border: none;"
        "  color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %5;"
        "}"
    ).arg(pal.buttonBg, pal.accent, pal.buttonBorder, pal.accent, pal.surface));
    toolbar_layout->addWidget(download_button);

    add_button = new QPushButton(StormLang(QStringLiteral("Добавить чит"), QStringLiteral("Add Custom Cheat")), this);
    add_button->setIcon(MakeCheatIcon(CheatIconType::Plus, QColor(pal.textPrimary)));
    add_button->setToolTip(StormLang(
        QStringLiteral("Добавить собственный чит-код в формате Atmosphere"),
        QStringLiteral("Add custom Atmosphere cheat code")
    ));
    add_button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-weight: 600;"
        "  font-size: 9pt;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "  border: none;"
        "}"
    ).arg(pal.buttonBg, pal.textPrimary, pal.buttonBorder, pal.itemHover, pal.accent));
    toolbar_layout->addWidget(add_button);

    open_folder_button = new QPushButton(StormLang(QStringLiteral("Открыть папку"), QStringLiteral("Open Folder")), this);
    open_folder_button->setIcon(MakeCheatIcon(CheatIconType::Folder, QColor(pal.textSecondary)));
    open_folder_button->setToolTip(StormLang(
        QStringLiteral("Открыть папку читов игры в проводнике"),
        QStringLiteral("Open cheats folder in file explorer")
    ));
    open_folder_button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-weight: 600;"
        "  font-size: 9pt;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "  border: none;"
        "  color: %6;"
        "}"
    ).arg(pal.buttonBg, pal.textSecondary, pal.buttonBorder, pal.itemHover, pal.accent, pal.textPrimary));
    toolbar_layout->addWidget(open_folder_button);

    main_layout->addLayout(toolbar_layout);

    // 2. 3-column QTreeWidget (34 px rows, styled Aurora checkboxes, unclipped status column minWidth 156 px)
    tree_widget = new QTreeWidget(this);
    tree_widget->setHeaderLabels({
        StormLang(QStringLiteral("НАЗВАНИЕ ЧИТ-КОДА"), QStringLiteral("CHEAT NAME")),
        StormLang(QStringLiteral("СТАТУС"), QStringLiteral("STATUS")),
        StormLang(QStringLiteral("ВЕРСИЯ (BUILD ID)"), QStringLiteral("VERSION (BUILD ID)"))
    });

    tree_widget->header()->setDefaultAlignment(Qt::AlignCenter);
    tree_widget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_widget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    tree_widget->header()->resizeSection(1, 160);
    tree_widget->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    tree_widget->header()->resizeSection(2, 144);
    tree_widget->setAlternatingRowColors(true);
    tree_widget->setRootIsDecorated(false);

    tree_widget->setStyleSheet(QStringLiteral(
        "QTreeWidget {"
        "  background-color: %1;"
        "  alternate-background-color: %2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  color: %4;"
        "  outline: none;"
        "}"
        "QTreeWidget::item {"
        "  height: 34px;"
        "  padding: 2px 8px;"
        "  border-bottom: 1px solid %5;"
        "}"
        "QTreeWidget::item:hover {"
        "  background-color: %6;"
        "}"
        "QTreeWidget::item:selected {"
        "  background-color: %7;"
        "  color: #FFFFFF;"
        "}"
        "QTreeWidget::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border: none;"
        "  border-radius: 4px;"
        "  background-color: %9;"
        "}"
        "QTreeWidget::indicator:hover {"
        "  border: none;"
        "}"
        "QTreeWidget::indicator:checked {"
        "  background-color: %10;"
        "  border: none;"
        "  image: none;"
        "}"
        "QHeaderView::section {"
        "  background-color: %11;"
        "  color: %12;"
        "  border: none;"
        "  border-right: 1px solid %3;"
        "  border-bottom: 1px solid %3;"
        "  font-weight: 700;"
        "  font-size: 8.5pt;"
        "  padding: 6px 10px;"
        "}"
    ).arg(
        pal.card,
        pal.cardAlt,
        pal.border,
        pal.textPrimary,
        pal.border,
        pal.itemHover,
        pal.itemSelected,
        pal.borderHover,
        pal.inputBg,
        pal.accent,
        pal.surface,
        pal.textSecondary
    ));
    main_layout->addWidget(tree_widget, 1);

    // Empty state centered message overlay
    empty_overlay_label = new QLabel(this);
    empty_overlay_label->setAlignment(Qt::AlignCenter);
    empty_overlay_label->setWordWrap(true);
    empty_overlay_label->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 10pt; font-style: italic; padding: 24px;"
    ).arg(pal.textSecondary));
    empty_overlay_label->setText(StormLang(
        QStringLiteral("Чит-коды для этой версии игры не найдены.\nНажмите «Скачать читы» для автоматической загрузки или добавьте собственный чит-код."),
        QStringLiteral("No cheat codes found for this game version.\nClick \"Download Cheats\" or add custom cheat code.")
    ));
    empty_overlay_label->setVisible(false);
    main_layout->addWidget(empty_overlay_label);

    // 3. Selection bottom bar
    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->setSpacing(8);

    select_all_button = new QPushButton(StormLang(QStringLiteral("Включить все"), QStringLiteral("Enable All")), this);
    select_all_button->setIcon(MakeCheatIcon(CheatIconType::Check, QColor(pal.accent)));
    select_all_button->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 5px; padding: 5px 12px; font-weight: 600; font-size: 8.5pt; min-height: 24px; }"
        "QPushButton:hover { background-color: %4; border: none; color: #FFFFFF; }"
    ).arg(pal.buttonBg, pal.textPrimary, pal.buttonBorder, pal.itemHover, pal.accent));
    bottom_layout->addWidget(select_all_button);

    deselect_all_button = new QPushButton(StormLang(QStringLiteral("Отключить все"), QStringLiteral("Disable All")), this);
    deselect_all_button->setIcon(MakeCheatIcon(CheatIconType::Cross, QColor(pal.textSecondary)));
    deselect_all_button->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 5px; padding: 5px 12px; font-weight: 600; font-size: 8.5pt; min-height: 24px; }"
        "QPushButton:hover { background-color: %4; border: none; color: #FFFFFF; }"
    ).arg(pal.buttonBg, pal.textSecondary, pal.buttonBorder, pal.itemHover, pal.accent));
    bottom_layout->addWidget(deselect_all_button);

    filter_enabled_btn = new QPushButton(StormLang(QStringLiteral("Только включённые"), QStringLiteral("Only Enabled")), this);
    filter_enabled_btn->setCheckable(true);
    filter_enabled_btn->setIcon(MakeCheatIcon(CheatIconType::Lightning, QColor(pal.accent)));
    filter_enabled_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 5px; padding: 5px 12px; font-weight: 600; font-size: 8.5pt; min-height: 24px; }"
        "QPushButton:checked { background-color: %4; color: #FFFFFF; border: none; }"
        "QPushButton:hover { border: none; }"
    ).arg(pal.buttonBg, pal.textPrimary, pal.buttonBorder, pal.accent, pal.accentHover));
    bottom_layout->addWidget(filter_enabled_btn);

    connect(filter_enabled_btn, &QPushButton::toggled, this, [this](bool only_enabled) {
        if (!tree_widget) return;
        for (int i = 0; i < tree_widget->topLevelItemCount(); ++i) {
            if (auto* it = tree_widget->topLevelItem(i)) {
                it->setHidden(only_enabled && it->checkState(0) != Qt::Checked);
            }
        }
    });

    bottom_layout->addStretch(1);

    status_label = new QLabel(this);
    status_label->setStyleSheet(QStringLiteral("color: %1; font-size: 9pt;").arg(pal.textSecondary));
    bottom_layout->addWidget(status_label);

    main_layout->addLayout(bottom_layout);

    // Connections
    connect(tree_widget, &QTreeWidget::itemChanged, this, &ConfigurePerGameCheats::OnItemChanged);
    connect(tree_widget, &QTreeWidget::itemDoubleClicked, this, [](QTreeWidgetItem* item, int) {
        if (!item) return;
        item->setCheckState(0, item->checkState(0) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    });
    connect(download_button, &QPushButton::clicked, this, &ConfigurePerGameCheats::OnDownloadOnlineCheats);
    connect(add_button, &QPushButton::clicked, this, &ConfigurePerGameCheats::OnAddCustomCheat);
    connect(open_folder_button, &QPushButton::clicked, this, &ConfigurePerGameCheats::OnOpenCheatsFolder);
    connect(select_all_button, &QPushButton::clicked, this, &ConfigurePerGameCheats::OnSelectAll);
    connect(deselect_all_button, &QPushButton::clicked, this, &ConfigurePerGameCheats::OnDeselectAll);
    connect(search_field, &QLineEdit::textChanged, this, &ConfigurePerGameCheats::OnFilterTextChanged);

    LoadCheats();
}

ConfigurePerGameCheats::~ConfigurePerGameCheats() = default;

void ConfigurePerGameCheats::RetranslateUI() {}

int ConfigurePerGameCheats::GetActiveCheatsCount() const {
    int count = 0;
    for (const auto& ci : cheat_items) {
        if (ci.enabled) ++count;
    }
    return count;
}

QString ConfigurePerGameCheats::GetGameTitle() const {
    QString title;
    if (system.IsPoweredOn() && system.GetApplicationProcessProgramID() == title_id) {
        // Can be resolved dynamically if powered on
    }
    if (title.isEmpty() && !file_name.isEmpty()) {
        const auto vfs = system.GetFilesystem();
        if (vfs) {
            const auto file = vfs->OpenFile(file_name.toStdString(), FileSys::OpenMode::Read);
            if (file) {
                auto loader = Loader::GetLoader(system, file);
                if (loader) {
                    std::string loaded_title;
                    if (loader->ReadTitle(loaded_title) == Loader::ResultStatus::Success && !loaded_title.empty()) {
                        title = QString::fromStdString(loaded_title);
                    }
                }
            }
        }
    }
    if (title.isEmpty() && !file_name.isEmpty()) {
        QFileInfo fi(file_name);
        title = fi.completeBaseName();
    }
    if (title.isEmpty()) {
        title = StormLang(QStringLiteral("Игра Nintendo Switch"), QStringLiteral("Nintendo Switch Game"));
    }
    return title;
}

QString ConfigurePerGameCheats::GetCheatsDirectoryPath() const {
    const auto cheats_base = Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "cheats";
    const auto title_dir = cheats_base / fmt::format("{:016X}", title_id);
    return QString::fromStdString(Common::FS::PathToUTF8String(title_dir));
}

QString ConfigurePerGameCheats::GetCheatsFilePath() const {
    const auto dir_path = GetCheatsDirectoryPath();
    const auto clean_build_id = build_id_str.trimmed().toUpper();
    return QDir(dir_path).filePath(QStringLiteral("%1.txt").arg(clean_build_id));
}

void ConfigurePerGameCheats::LoadCheats() {
    cheat_items.clear();
    const auto dir_path = GetCheatsDirectoryPath();
    QDir dir(dir_path);
    dir.mkpath(dir_path);

    const auto& disabled = Settings::values.disabled_addons[title_id];
    const bool has_explicit_enabled_tags = std::any_of(disabled.cbegin(), disabled.cend(), [](const std::string& s) {
        return s.starts_with("__ENABLED__:");
    });

    const QStringList txt_files = dir.entryList(QStringList() << QStringLiteral("*.txt"), QDir::Files);
    for (const auto& fname : txt_files) {
        const QString full_path = dir.filePath(fname);
        QFile file(full_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        const QString all_content = QString::fromUtf8(file.readAll());
        file.close();

        if (IsBogusCheatText(all_content)) {
            QFile::remove(full_path);
            continue;
        }

        QString bid_label = fname.left(fname.lastIndexOf(QLatin1Char('.'))).toUpper();
        if (bid_label == QStringLiteral("CHEATS") || bid_label == QString(QStringLiteral("%1")).arg(title_id, 16, 16, QLatin1Char('0')).toUpper()) {
            bid_label = StormLang(QStringLiteral("Все версии"), QStringLiteral("All Versions"));
        }

        QTextStream stream(all_content.toUtf8());
        QString current_cheat_name;
        QString current_cheat_code;

        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QStringLiteral("#")) || line.startsWith(QStringLiteral("//")) || line.startsWith(QStringLiteral(";"))) continue;

            if (line.startsWith(QStringLiteral("[")) && line.contains(QStringLiteral("]"))) {
                if (!current_cheat_name.isEmpty() && !current_cheat_code.isEmpty()) {
                    bool is_enabled = false;
                    if (has_explicit_enabled_tags) {
                        is_enabled = std::find(disabled.cbegin(), disabled.cend(), "__ENABLED__:" + current_cheat_name.toStdString()) != disabled.cend();
                    } else {
                        is_enabled = false;
                    }

                    bool exists = std::any_of(cheat_items.begin(), cheat_items.end(), [&](const CheatItem& ci) {
                        return ci.name == current_cheat_name;
                    });
                    if (!exists) {
                        cheat_items.push_back({current_cheat_name, current_cheat_code.trimmed(), bid_label, is_enabled, false});
                    }
                }
                int close_idx = line.indexOf(QLatin1Char(']'));
                current_cheat_name = line.mid(1, close_idx - 1).trimmed();
                current_cheat_code.clear();
            } else {
                current_cheat_code += line + QStringLiteral("\n");
            }
        }

        if (!current_cheat_name.isEmpty() && !current_cheat_code.isEmpty()) {
            bool is_enabled = false;
            if (has_explicit_enabled_tags) {
                is_enabled = std::find(disabled.cbegin(), disabled.cend(), "__ENABLED__:" + current_cheat_name.toStdString()) != disabled.cend();
            } else {
                is_enabled = false;
            }

            bool exists = std::any_of(cheat_items.begin(), cheat_items.end(), [&](const CheatItem& ci) {
                return ci.name == current_cheat_name;
            });
            if (!exists) {
                cheat_items.push_back({current_cheat_name, current_cheat_code.trimmed(), bid_label, is_enabled, false});
            }
        }
    }

    PopulateCheatTree();
}

void ConfigurePerGameCheats::PopulateCheatTree() {
    tree_widget->blockSignals(true);
    tree_widget->clear();

    const auto pal = StormTheme::GetThemePalette();
    const QString search_query = search_field->text().trimmed();
    int enabled_count = 0;
    const bool is_game_running = system.IsPoweredOn() && (system.GetApplicationProcessProgramID() == title_id);

    int displayed_count = 0;
    for (int i = 0; i < cheat_items.size(); i++) {
        const auto& item = cheat_items[i];
        if (!search_query.isEmpty() && !item.name.contains(search_query, Qt::CaseInsensitive)) {
            continue;
        }

        if (item.enabled) {
            enabled_count++;
        }
        displayed_count++;

        auto* tree_item = new QTreeWidgetItem(tree_widget);
        tree_item->setText(0, item.name.toUpper());
        tree_item->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        tree_item->setCheckState(0, item.enabled ? Qt::Checked : Qt::Unchecked);
        tree_item->setData(0, Qt::UserRole, i);

        // Tooltip with full instructions
        const QString tip = QStringLiteral("%1\n\n%2").arg(item.name, item.code.trimmed());
        tree_item->setToolTip(0, tip);
        tree_item->setToolTip(1, tip);
        tree_item->setToolTip(2, tip);

        if (is_game_running && item.enabled) {
            tree_item->setText(1, StormLang(QStringLiteral("АКТИВЕН В ИГРЕ"), QStringLiteral("ACTIVE IN-GAME")));
            tree_item->setForeground(1, QColor(pal.accent));
        } else if (item.enabled) {
            tree_item->setText(1, StormLang(QStringLiteral("ВКЛЮЧЁН"), QStringLiteral("ENABLED")));
            tree_item->setForeground(1, QColor(QStringLiteral("#34D399")));
        } else {
            tree_item->setText(1, StormLang(QStringLiteral("ВЫКЛЮЧЕН"), QStringLiteral("DISABLED")));
            tree_item->setForeground(1, QColor(pal.textSecondary));
        }
        tree_item->setTextAlignment(1, Qt::AlignCenter);

        tree_item->setText(2, item.build_id.isEmpty() ? StormLang(QStringLiteral("ВСЕ ВЕРСИИ"), QStringLiteral("ALL VERSIONS")) : item.build_id.toUpper());
        tree_item->setForeground(2, QColor(pal.textSecondary));
        tree_item->setTextAlignment(2, Qt::AlignCenter);
    }

    if (cheat_items.isEmpty()) {
        empty_overlay_label->setVisible(true);
        tree_widget->setVisible(false);
        status_label->setText(StormLang(
            QStringLiteral("Чит-коды не найдены."),
            QStringLiteral("No cheats found.")
        ));
    } else {
        empty_overlay_label->setVisible(false);
        tree_widget->setVisible(true);
        status_label->setText(StormLang(
            QStringLiteral("Всего читов: <b>%1</b> &nbsp;|&nbsp; Включено: <b style='color:%2;'>%3</b> &nbsp;|&nbsp; Статус: <b>%4</b>")
                .arg(cheat_items.size())
                .arg(pal.accent)
                .arg(enabled_count)
                .arg(is_game_running ? StormLang(QStringLiteral("Применяются в реальном времени"), QStringLiteral("Applied in real time"))
                                     : StormLang(QStringLiteral("Готовы к запуску"), QStringLiteral("Ready to run"))),
            QStringLiteral("Total cheats: <b>%1</b> &nbsp;|&nbsp; Enabled: <b style='color:%2;'>%3</b> &nbsp;|&nbsp; Status: <b>%4</b>")
                .arg(cheat_items.size())
                .arg(pal.accent)
                .arg(enabled_count)
                .arg(is_game_running ? StormLang(QStringLiteral("Применяются в реальном времени"), QStringLiteral("Applied in real time"))
                                     : StormLang(QStringLiteral("Готовы к запуску"), QStringLiteral("Ready to run")))
        ));
    }
    tree_widget->blockSignals(false);
}

void ConfigurePerGameCheats::OnItemChanged(QTreeWidgetItem* item, int column) {
    if (column != 0 || !item) return;

    int idx = item->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < cheat_items.size()) {
        const bool is_checked = (item->checkState(0) == Qt::Checked);
        cheat_items[idx].enabled = is_checked;
        const auto pal = StormTheme::GetThemePalette();
        const bool is_game_running = system.IsPoweredOn() && (system.GetApplicationProcessProgramID() == title_id);
        if (is_game_running && is_checked) {
            item->setText(1, StormLang(QStringLiteral("АКТИВЕН В ИГРЕ"), QStringLiteral("ACTIVE IN-GAME")));
            item->setForeground(1, QColor(pal.accent));
        } else if (is_checked) {
            item->setText(1, StormLang(QStringLiteral("ВКЛЮЧЁН"), QStringLiteral("ENABLED")));
            item->setForeground(1, QColor(QStringLiteral("#34D399")));
        } else {
            item->setText(1, StormLang(QStringLiteral("ВЫКЛЮЧЕН"), QStringLiteral("DISABLED")));
            item->setForeground(1, QColor(pal.textSecondary));
        }
    }

    int enabled_count = 0;
    for (const auto& it : cheat_items) {
        if (it.enabled) enabled_count++;
    }
    const auto pal = StormTheme::GetThemePalette();
    const bool is_game_running = system.IsPoweredOn() && (system.GetApplicationProcessProgramID() == title_id);
    status_label->setText(StormLang(
        QStringLiteral("Всего читов: <b>%1</b> &nbsp;|&nbsp; Включено: <b style='color:%2;'>%3</b> &nbsp;|&nbsp; Статус: <b>%4</b>")
            .arg(cheat_items.size())
            .arg(pal.accent)
            .arg(enabled_count)
            .arg(is_game_running ? StormLang(QStringLiteral("Применяются в реальном времени"), QStringLiteral("Applied in real time"))
                                 : StormLang(QStringLiteral("Готовы к запуску"), QStringLiteral("Ready to run"))),
        QStringLiteral("Total cheats: <b>%1</b> &nbsp;|&nbsp; Enabled: <b style='color:%2;'>%3</b> &nbsp;|&nbsp; Status: <b>%4</b>")
            .arg(cheat_items.size())
            .arg(pal.accent)
            .arg(enabled_count)
            .arg(is_game_running ? StormLang(QStringLiteral("Применяются в реальном времени"), QStringLiteral("Applied in real time"))
                                 : StormLang(QStringLiteral("Готовы к запуску"), QStringLiteral("Ready to run")))
    ));

    ApplyConfiguration();
    emit CheatsChanged();
}

void ConfigurePerGameCheats::OnFilterTextChanged(const QString& /*text*/) {
    PopulateCheatTree();
}

void ConfigurePerGameCheats::OnSelectAll() {
    for (auto& item : cheat_items) {
        item.enabled = true;
    }
    PopulateCheatTree();
    ApplyConfiguration();
    emit CheatsChanged();
}

void ConfigurePerGameCheats::OnDeselectAll() {
    for (auto& item : cheat_items) {
        item.enabled = false;
    }
    PopulateCheatTree();
    ApplyConfiguration();
    emit CheatsChanged();
}

void ConfigurePerGameCheats::OnOpenCheatsFolder() {
    const auto dir_path = GetCheatsDirectoryPath();
    QDir().mkpath(dir_path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
}

void ConfigurePerGameCheats::OnDownloadOnlineCheats() {
    const auto tid_hex = QString(QStringLiteral("%1")).arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    const auto bid_hex = build_id_str.trimmed().toUpper();
    const auto bid_lower = bid_hex.toLower();

    download_button->setEnabled(false);
    download_button->setText(StormLang(QStringLiteral("Загрузка..."), QStringLiteral("Downloading...")));

    QNetworkAccessManager manager;
    auto fetch_http = [&](const QString& url_str, int timeout_ms = 7000) -> std::pair<int, QByteArray> {
        QNetworkRequest request{QUrl(url_str)};
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH_Emulator/17.1.0"));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        QNetworkReply* reply = manager.get(request);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        timer.start(timeout_ms);
        loop.exec();

        int status_code = 0;
        QByteArray data;
        if (reply->error() == QNetworkReply::NoError) {
            status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            data = reply->readAll();
        }
        reply->deleteLater();
        return {status_code, data};
    };

    const auto dir_path = GetCheatsDirectoryPath();
    QDir().mkpath(dir_path);

    auto save_clean_bid_file = [&](const QString& raw_bid, const QString& cheat_body) {
        QString clean_bid = raw_bid.trimmed().toUpper();
        if (clean_bid.length() != 16 && bid_hex.length() == 16) {
            clean_bid = bid_hex;
        }
        if (clean_bid.length() != 16 || cheat_body.trimmed().isEmpty() || IsBogusCheatText(cheat_body)) {
            return;
        }
        QFile bid_file(QDir(dir_path).filePath(QStringLiteral("%1.txt").arg(clean_bid)));
        if (bid_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&bid_file);
            stream << cheat_body.trimmed() << QStringLiteral("\n\n");
            bid_file.close();
        }
    };

    // DB 1: HamletDuFromage switch-cheats-db via jsDelivr CDN
    {
        const QString url = QStringLiteral("https://cdn.jsdelivr.net/gh/HamletDuFromage/switch-cheats-db@master/cheats/%1.json").arg(tid_hex);
        auto [code, resp_data] = fetch_http(url, 7000);
        if (code == 200 && !resp_data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(resp_data);
            if (doc.isObject()) {
                const QJsonObject root = doc.object();
                for (auto it = root.begin(); it != root.end(); ++it) {
                    const QString cur_bid = it.key().trimmed().toUpper();
                    const QJsonObject bid_cheats = it.value().toObject();
                    QString bid_file_content;
                    for (auto c_it = bid_cheats.begin(); c_it != bid_cheats.end(); ++c_it) {
                        const QString body = c_it.value().toString().trimmed();
                        if (!body.isEmpty() && !IsBogusCheatText(body)) {
                            bid_file_content += body + QStringLiteral("\n\n");
                        }
                    }
                    if (!bid_file_content.isEmpty()) {
                        save_clean_bid_file(cur_bid, bid_file_content);
                    }
                }
            }
        }
    }

    // DB 2: Tinfoil Media API
    {
        const QString url = QStringLiteral("https://tinfoil.media/api/cheats/%1").arg(tid_hex);
        auto [code, resp_data] = fetch_http(url, 7000);
        if (code == 200 && !resp_data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(resp_data);
            if (doc.isArray()) {
                QMap<QString, QString> bid_map;
                for (const auto& item_val : doc.array()) {
                    const QJsonObject item = item_val.toObject();
                    QString c_name = item.value(QStringLiteral("name")).toString().trimmed();
                    QString c_bid = item.value(QStringLiteral("build_id")).toString().trimmed().toUpper();
                    QString c_source = item.value(QStringLiteral("source")).toString().trimmed();
                    if (c_name.isEmpty() || c_source.isEmpty()) continue;
                    if (c_bid.isEmpty() && bid_hex.length() == 16) c_bid = bid_hex;
                    if (c_bid.length() != 16) continue;
                    if (!c_name.startsWith(QLatin1Char('['))) c_name = QStringLiteral("[%1]").arg(c_name);
                    if (!c_name.endsWith(QLatin1Char(']'))) c_name = QStringLiteral("%1]").arg(c_name);
                    bid_map[c_bid] += QStringLiteral("%1\n%2\n\n").arg(c_name, c_source);
                }
                for (auto it = bid_map.begin(); it != bid_map.end(); ++it) {
                    save_clean_bid_file(it.key(), it.value());
                }
            }
        }
    }

    // DB 3: ChanseyIsTheBest NX-60FPS-RES-GFX-Cheats (BuildID uppercase .txt)
    if (bid_hex.length() == 16) {
        const QString url = QStringLiteral("https://cdn.jsdelivr.net/gh/ChanseyIsTheBest/NX-60FPS-RES-GFX-Cheats@main/titles/%1/cheats/%2.txt")
                                .arg(tid_hex, bid_hex);
        auto [code, resp_data] = fetch_http(url, 6000);
        if (code == 200 && !resp_data.isEmpty()) {
            const QString text = QString::fromUtf8(resp_data);
            if (!text.trimmed().isEmpty() && !IsBogusCheatText(text)) {
                save_clean_bid_file(bid_hex, text);
            }
        }
    }

    // DB 4: ibnux switch-cheat master (BuildID lowercase .txt)
    if (bid_hex.length() == 16) {
        const QString url = QStringLiteral("https://cdn.jsdelivr.net/gh/ibnux/switch-cheat@master/atmosphere/titles/%1/cheats/%2.txt")
                                .arg(tid_hex, bid_lower);
        auto [code, resp_data] = fetch_http(url, 6000);
        if (code == 200 && !resp_data.isEmpty()) {
            const QString text = QString::fromUtf8(resp_data);
            if (!text.trimmed().isEmpty() && !IsBogusCheatText(text)) {
                save_clean_bid_file(bid_hex, text);
            }
        }
    }

    // DB 5: ibnux switch-cheat directory listing API fallback
    {
        const QString url = QStringLiteral("https://api.github.com/repos/ibnux/switch-cheat/contents/atmosphere/titles/%1/cheats").arg(tid_hex);
        auto [code, resp_data] = fetch_http(url, 6000);
        if (code == 200 && !resp_data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(resp_data);
            if (doc.isArray()) {
                for (const auto& item_val : doc.array()) {
                    const QJsonObject file_obj = item_val.toObject();
                    const QString entry_name = file_obj.value(QStringLiteral("name")).toString().trimmed();
                    const QString download_url = file_obj.value(QStringLiteral("download_url")).toString().trimmed();
                    if (entry_name.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive) && !download_url.isEmpty()) {
                        const QString file_bid = entry_name.left(entry_name.lastIndexOf(QLatin1Char('.'))).trimmed().toUpper();
                        if (file_bid.length() == 16) {
                            const QString local_path = QDir(dir_path).filePath(QStringLiteral("%1.txt").arg(file_bid));
                            if (!QFile::exists(local_path)) {
                                auto [f_code, f_data] = fetch_http(download_url, 6000);
                                if (f_code == 200 && !f_data.isEmpty()) {
                                    const QString text = QString::fromUtf8(f_data);
                                    if (!text.trimmed().isEmpty() && !IsBogusCheatText(text)) {
                                        save_clean_bid_file(file_bid, text);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    download_button->setEnabled(true);
    download_button->setText(StormLang(QStringLiteral("Скачать читы"), QStringLiteral("Download Cheats")));

    LoadCheats();
    emit CheatsChanged();

    if (!cheat_items.isEmpty()) {
        QMessageBox::information(this, StormLang(QStringLiteral("Читы загружены"), QStringLiteral("Cheats Downloaded")),
                                 StormLang(
                                     QStringLiteral("Успешно загружена база чит-кодов из проверенных источников!\nНайдено уникальных читов: %1").arg(cheat_items.size()),
                                     QStringLiteral("Successfully loaded cheat database from verified sources!\nFound unique cheats: %1").arg(cheat_items.size())
                                 ));
    } else {
        QMessageBox::information(this, StormLang(QStringLiteral("Чит-коды"), QStringLiteral("Cheats")),
                                 StormLang(
                                     QStringLiteral("Для данной версии/сборки игры читы в базах пока не найдены."),
                                     QStringLiteral("No cheats found in online databases for this version/build yet.")
                                 ));
    }
}

void ConfigurePerGameCheats::OnAddCustomCheat() {
    const auto pal = StormTheme::GetThemePalette();

    QDialog dialog(this);
    dialog.setWindowTitle(StormLang(QStringLiteral("Добавить чит-код"), QStringLiteral("Add Custom Cheat")));
    dialog.resize(500, 360);
    dialog.setStyleSheet(StormDesign::BuildDialogStylesheet(pal));

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    auto* name_lbl = new QLabel(StormLang(QStringLiteral("Название чита:"), QStringLiteral("Cheat Name:")), &dialog);
    name_lbl->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(pal.textPrimary));
    layout->addWidget(name_lbl);

    auto* name_edit = new QLineEdit(&dialog);
    name_edit->setPlaceholderText(StormLang(
        QStringLiteral("Например: Бесконечные жизни или 999 монет"),
        QStringLiteral("Example: Infinite Health or Max Gold")
    ));
    layout->addWidget(name_edit);

    auto* code_lbl = new QLabel(StormLang(QStringLiteral("Код чита (формат Atmosphere):"), QStringLiteral("Cheat Code (Atmosphere format):")), &dialog);
    code_lbl->setStyleSheet(QStringLiteral("font-weight: bold; color: %1;").arg(pal.textPrimary));
    layout->addWidget(code_lbl);

    auto* code_edit = new QTextEdit(&dialog);
    code_edit->setPlaceholderText(QStringLiteral("04000000 01234567 00000001\n580F0000 01234568\n..."));
    code_edit->setFontFamily(QStringLiteral("Courier New"));
    layout->addWidget(code_edit, 1);

    auto* btn_box = new QHBoxLayout();
    btn_box->setSpacing(8);

    auto* cancel_btn = new QPushButton(StormLang(QStringLiteral("Отмена"), QStringLiteral("Cancel")), &dialog);
    auto* ok_btn = new QPushButton(StormLang(QStringLiteral("Сохранить чит"), QStringLiteral("Save Cheat")), &dialog);
    ok_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: #FFFFFF; font-weight: bold; padding: 6px 18px; border-radius: 5px; min-height: 24px; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(pal.accent, pal.accentHover));

    btn_box->addStretch(1);
    btn_box->addWidget(cancel_btn);
    btn_box->addWidget(ok_btn);
    layout->addLayout(btn_box);

    connect(ok_btn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString cheat_name = name_edit->text().trimmed();
        QString cheat_code = code_edit->toPlainText().trimmed();
        if (cheat_name.isEmpty() || cheat_code.isEmpty()) {
            QMessageBox::warning(this, StormLang(QStringLiteral("Ошибка"), QStringLiteral("Error")),
                                 StormLang(QStringLiteral("Название и код чита не могут быть пустыми!"),
                                           QStringLiteral("Cheat name and code cannot be empty!")));
            return;
        }

        cheat_items.push_back({cheat_name, cheat_code, StormLang(QStringLiteral("Пользовательский"), QStringLiteral("Custom")), false, true});
        SaveCheatsToFile();
        PopulateCheatTree();
        ApplyConfiguration();
        emit CheatsChanged();
    }
}

void ConfigurePerGameCheats::SaveCheatsToFile() {
    const auto dir_path = GetCheatsDirectoryPath();
    QDir().mkpath(dir_path);
    const auto file_path = GetCheatsFilePath();

    QFile file(file_path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        for (const auto& item : cheat_items) {
            stream << QString(QStringLiteral("[%1]\n")).arg(item.name);
            stream << item.code.trimmed() << QStringLiteral("\n\n");
        }
        file.close();
    }
}

void ConfigurePerGameCheats::ApplyConfiguration() {
    SaveCheatsToFile();

    std::vector<std::string> new_disabled;
    const auto& current_disabled = Settings::values.disabled_addons[title_id];
    for (const auto& entry : current_disabled) {
        if (!entry.starts_with("__ENABLED__:")) {
            bool is_cheat_name = std::any_of(cheat_items.cbegin(), cheat_items.cend(), [&](const CheatItem& ci) {
                return ci.name.toStdString() == entry;
            });
            if (!is_cheat_name) {
                new_disabled.push_back(entry);
            }
        }
    }

    std::vector<Core::Memory::CheatEntry> active_cheat_entries;
    const Core::Memory::TextCheatParser parser;

    for (const auto& item : cheat_items) {
        if (!item.enabled) {
            new_disabled.push_back(item.name.toStdString());
        } else {
            new_disabled.push_back("__ENABLED__:" + item.name.toStdString());
            std::string formatted = fmt::format("[{}]\n{}\n", item.name.toStdString(), item.code.toStdString());
            auto parsed = parser.Parse(formatted);
            for (auto& p : parsed) {
                active_cheat_entries.push_back(std::move(p));
            }
        }
    }

    Settings::values.disabled_addons[title_id] = std::move(new_disabled);

    if (system.IsPoweredOn()) {
        system.ReloadCheatList(active_cheat_entries);
        LOG_INFO(Common, "Dynamically reloaded {} active cheat entries during gameplay for title_id=0x{:016X}",
                 active_cheat_entries.size(), title_id);
    }
}
