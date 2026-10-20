// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMimeData>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTabBar>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <fmt/format.h>
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/hex_util.h"
#include "common/lz4_compression.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/submission_package.h"
#include "core/file_sys/xts_archive.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/nso.h"
#include "qt_common/abstract/frontend.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/mod.h"
#include "storm_switch/configuration/configure_gamebanana_mods.h"
#include "storm_switch/mod_manager_dialog.h"

ModManagerDialog::ModManagerDialog(QWidget* parent, Core::System& system_, u64 title_id_,
                                   const QString& game_path_, const QString& game_name_)
    : QDialog(parent), system{system_}, title_id{title_id_}, game_path{game_path_},
      game_name{game_name_} {
    const QString display_title = game_name.isEmpty()
        ? QStringLiteral("0x%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper()
        : game_name;
    setWindowTitle(tr("STORM SWITCH — Менеджер модов: %1").arg(display_title));
    resize(1160, 700);
    setMinimumSize(680, 460);
    setAcceptDrops(true);

    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0B0F19; color: #E2E8F0; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
        "QTabWidget::pane { border: 1px solid #1E293B; background: #0F172A; border-radius: 8px; }"
        "QTabBar::tab { background: #0A0E17; color: #94A3B8; padding: 8px 18px; border: 1px solid #1E293B; "
        "border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-weight: bold; }"
        "QTabBar::tab:selected { background: #1E293B; color: #00D2FF; border-bottom: 2px solid #00D2FF; }"
        "QTableWidget { background-color: #0A0E17; border: 1px solid #1E293B; border-radius: 6px; gridline-color: #1E293B; selection-background-color: #1E293B; selection-color: #00D2FF; }"
        "QHeaderView::section { background-color: #0F172A; color: #94A3B8; padding: 6px; font-weight: bold; border: 1px solid #1E293B; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A); "
        "border: 1px solid #334155; border-radius: 6px; color: #F1F5F9; padding: 7px 16px; font-weight: 500; }"
        "QPushButton:hover { border: 1px solid #00D2FF; color: #00D2FF; background: #1E293B; }"
        "QPushButton:pressed { background: #0284C7; color: #FFFFFF; }"
        "QPushButton:disabled { background: #0A0E17; border: 1px solid #1E293B; color: #475569; }"
    ));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(10);

    tab_widget = new QTabWidget(this);
    QFont tab_font = font();
    tab_font.setBold(true);
    tab_widget->tabBar()->setFont(tab_font);

    // Tab 1: Priorities & Installed Mods
    auto* priority_tab = new QWidget(this);
    SetupPriorityTab(priority_tab);
    tab_widget->addTab(priority_tab, tr("📁 Приоритеты и установленные моды (Drag-and-Drop)"));

    // Tab 2: GameBanana online mods
    gamebanana_tab = new ConfigureGameBananaMods(system, title_id, game_name, nullptr, this);
    connect(gamebanana_tab, &ConfigureGameBananaMods::ModInstalled, this, &ModManagerDialog::RefreshModsList);
    tab_widget->addTab(gamebanana_tab, tr("🍌 Онлайн каталог GameBanana (60 FPS / Графика / Текстуры)"));

    main_layout->addWidget(tab_widget, 1);

    // Bottom Action Buttons
    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->setSpacing(10);

    auto* open_folder_btn = new QPushButton(tr("📁 Открыть папку модов"), this);
    connect(open_folder_btn, &QPushButton::clicked, this, &ModManagerDialog::OnOpenModFolder);
    bottom_layout->addWidget(open_folder_btn);

    auto* gen_patch_btn = new QPushButton(tr("⚡ Генератор патчей 60/120 FPS и всех форматов экрана..."), this);
    gen_patch_btn->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0F766E, stop:1 #0284C7); "
        "color: #FFFFFF; font-weight: bold; border: 1px solid #38BDF8;"));
    connect(gen_patch_btn, &QPushButton::clicked, this, &ModManagerDialog::OnGenerateAutoPatch);
    bottom_layout->addWidget(gen_patch_btn);

    bottom_layout->addStretch(1);

    auto* cancel_btn = new QPushButton(tr("Отмена"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    bottom_layout->addWidget(cancel_btn);

    auto* apply_btn = new QPushButton(tr("💾 Сохранить приоритеты и применить"), this);
    apply_btn->setStyleSheet(QStringLiteral("background: #0284C7; color: #FFFFFF; font-weight: bold; border-color: #00D2FF;"));
    connect(apply_btn, &QPushButton::clicked, this, &ModManagerDialog::OnApplyAndClose);
    bottom_layout->addWidget(apply_btn);

    main_layout->addLayout(bottom_layout);

    RefreshModsList();
}

ModManagerDialog::~ModManagerDialog() = default;

void ModManagerDialog::SetupPriorityTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // Help banner
    auto* banner = new QLabel(
        tr("💡 <b>Совет:</b> Перетаскивайте папки или ZIP-архивы модов прямо в это окно для быстрой установки!<br>"
           "Используйте кнопки «▲ Выше» и «▼ Ниже» для настройки порядка загрузки LayeredFS (верхние моды перекрывают файлы нижних)."),
        tab);
    banner->setStyleSheet(QStringLiteral("background: rgba(14, 165, 233, 0.12); border: 1px solid #0284C7; border-radius: 6px; padding: 8px; color: #BAE6FD;"));
    layout->addWidget(banner);

    auto* content_layout = new QHBoxLayout();
    content_layout->setSpacing(10);

    // Table of mods
    mod_table = new QTableWidget(tab);
    mod_table->setColumnCount(5);
    mod_table->setHorizontalHeaderLabels({
        tr("Приоритет"),
        tr("Включен"),
        tr("Название мода"),
        tr("Тип"),
        tr("Конфликты файлов LayeredFS")
    });
    mod_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mod_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mod_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    mod_table->horizontalHeader()->resizeSection(2, 280);
    mod_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mod_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    mod_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    mod_table->setSelectionMode(QAbstractItemView::SingleSelection);
    mod_table->verticalHeader()->setVisible(false);

    connect(mod_table, &QTableWidget::itemChanged, this, &ModManagerDialog::OnItemChanged);

    content_layout->addWidget(mod_table, 1);

    // Side control buttons
    auto* btn_layout = new QVBoxLayout();
    btn_layout->setSpacing(8);

    btn_up = new QPushButton(tr("▲ Выше"), tab);
    btn_up->setStyleSheet(QStringLiteral("font-weight: bold; color: #00D2FF;"));
    connect(btn_up, &QPushButton::clicked, this, &ModManagerDialog::OnMoveUp);
    btn_layout->addWidget(btn_up);

    btn_down = new QPushButton(tr("▼ Ниже"), tab);
    btn_down->setStyleSheet(QStringLiteral("font-weight: bold; color: #00D2FF;"));
    connect(btn_down, &QPushButton::clicked, this, &ModManagerDialog::OnMoveDown);
    btn_layout->addWidget(btn_down);

    btn_layout->addSpacing(15);

    auto* btn_add_folder = new QPushButton(tr("➕ Добавить папку..."), tab);
    connect(btn_add_folder, &QPushButton::clicked, this, &ModManagerDialog::OnAddFolder);
    btn_layout->addWidget(btn_add_folder);

    auto* btn_add_zip = new QPushButton(tr("📦 Добавить ZIP..."), tab);
    connect(btn_add_zip, &QPushButton::clicked, this, &ModManagerDialog::OnAddZip);
    btn_layout->addWidget(btn_add_zip);

    auto* btn_auto_patch = new QPushButton(tr("⚡ Авто-патч 60 FPS / Экран..."), tab);
    btn_auto_patch->setStyleSheet(QStringLiteral("color: #38BDF8; font-weight: bold;"));
    connect(btn_auto_patch, &QPushButton::clicked, this, &ModManagerDialog::OnGenerateAutoPatch);
    btn_layout->addWidget(btn_auto_patch);

    btn_delete = new QPushButton(tr("🗑️ Удалить мод"), tab);
    btn_delete->setStyleSheet(QStringLiteral("color: #EF4444;"));
    connect(btn_delete, &QPushButton::clicked, this, &ModManagerDialog::OnDeleteMod);
    btn_layout->addWidget(btn_delete);

    btn_layout->addStretch(1);
    content_layout->addLayout(btn_layout);

    layout->addLayout(content_layout);

    conflict_summary_lbl = new QLabel(tab);
    conflict_summary_lbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #94A3B8;"));
    layout->addWidget(conflict_summary_lbl);
}

void ModManagerDialog::RefreshModsList() {
    mods_list.clear();
    mod_table->blockSignals(true);
    mod_table->setRowCount(0);

    const QString tid_str = QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    const QString load_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir)));
    const QString game_load_dir = QDir(load_dir).filePath(tid_str);

    // Read mod_priorities.json
    std::vector<std::string> saved_priorities;
    const QString prio_path = QDir(game_load_dir).filePath(QStringLiteral("mod_priorities.json"));
    if (QFile::exists(prio_path)) {
        std::ifstream f(prio_path.toStdString());
        if (f.is_open()) {
            try {
                auto j = nlohmann::json::parse(f);
                if (j.contains("priorities") && j["priorities"].is_array()) {
                    for (const auto& item : j["priorities"]) {
                        if (item.is_string()) {
                            saved_priorities.push_back(item.get<std::string>());
                        }
                    }
                }
            } catch (...) {}
        }
    }

    const auto& disabled = Settings::values.disabled_addons[title_id];

    // Scan mods in load/<TitleID>/
    QDir dir(game_load_dir);
    if (dir.exists()) {
        const auto subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& d : subdirs) {
            ModEntryInfo entry{};
            entry.name = d.fileName();
            entry.path = d.absoluteFilePath();
            entry.enabled = (std::find(disabled.begin(), disabled.end(), entry.name.toStdString()) == disabled.end());

            // Check type
            QDir mod_dir(entry.path);
            bool has_romfs = mod_dir.exists(QStringLiteral("romfs")) || mod_dir.exists(QStringLiteral("romfslite"));
            bool has_exefs = mod_dir.exists(QStringLiteral("exefs"));
            if (has_romfs && has_exefs) {
                entry.type = QStringLiteral("RomFS + ExeFS");
            } else if (has_romfs) {
                entry.type = QStringLiteral("LayeredFS (RomFS)");
            } else if (has_exefs) {
                entry.type = QStringLiteral("LayeredFS (ExeFS)");
            } else {
                entry.type = QStringLiteral("Патч / Мод");
            }

            // Gather relative files for conflict detection
            QDirIterator it(entry.path, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString abs_file = it.next();
                QString rel_file = mod_dir.relativeFilePath(abs_file);
                entry.relative_files.push_back(rel_file.toLower().toStdString());
            }

            mods_list.push_back(std::move(entry));
        }
    }

    // Sort according to saved priorities
    std::stable_sort(mods_list.begin(), mods_list.end(), [&saved_priorities](const ModEntryInfo& a, const ModEntryInfo& b) {
        const auto a_it = std::find(saved_priorities.begin(), saved_priorities.end(), a.name.toStdString());
        const auto b_it = std::find(saved_priorities.begin(), saved_priorities.end(), b.name.toStdString());
        if (a_it != saved_priorities.end() && b_it != saved_priorities.end()) {
            return (a_it - saved_priorities.begin()) < (b_it - saved_priorities.begin());
        }
        if (a_it != saved_priorities.end()) return true;
        if (b_it != saved_priorities.end()) return false;
        return a.name < b.name;
    });

    DetectConflicts();

    for (size_t i = 0; i < mods_list.size(); ++i) {
        const auto& m = mods_list[i];
        int row = mod_table->rowCount();
        mod_table->insertRow(row);

        // Priority
        QString prio_text = (i == 0) ? tr("1 [Высший]") : QString::number(i + 1);
        auto* item_prio = new QTableWidgetItem(prio_text);
        item_prio->setTextAlignment(Qt::AlignCenter);
        item_prio->setFlags(item_prio->flags() & ~Qt::ItemIsEditable);
        if (i == 0) item_prio->setForeground(QColor(0, 210, 255));
        mod_table->setItem(row, 0, item_prio);

        // Checkbox
        auto* item_check = new QTableWidgetItem();
        item_check->setCheckState(m.enabled ? Qt::Checked : Qt::Unchecked);
        item_check->setTextAlignment(Qt::AlignCenter);
        item_check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        mod_table->setItem(row, 1, item_check);

        // Name
        auto* item_name = new QTableWidgetItem(m.name);
        item_name->setFlags(item_name->flags() & ~Qt::ItemIsEditable);
        item_name->setFont(QFont(font().family(), font().pointSize(), QFont::DemiBold));
        mod_table->setItem(row, 2, item_name);

        // Type
        auto* item_type = new QTableWidgetItem(m.type);
        item_type->setTextAlignment(Qt::AlignCenter);
        item_type->setFlags(item_type->flags() & ~Qt::ItemIsEditable);
        item_type->setForeground(QColor(148, 163, 184));
        mod_table->setItem(row, 3, item_type);

        // Conflict
        auto* item_conf = new QTableWidgetItem(m.conflict_warning);
        item_conf->setFlags(item_conf->flags() & ~Qt::ItemIsEditable);
        if (m.conflict_warning.startsWith(QStringLiteral("⚠️"))) {
            item_conf->setForeground(QColor(245, 158, 11)); // Amber
        } else {
            item_conf->setForeground(QColor(52, 211, 153)); // Emerald
        }
        mod_table->setItem(row, 4, item_conf);
    }

    mod_table->blockSignals(false);
}

void ModManagerDialog::DetectConflicts() {
    int total_conflicts = 0;
    for (size_t i = 0; i < mods_list.size(); ++i) {
        mods_list[i].conflict_warning = tr("✅ Нет конфликтов");
        if (!mods_list[i].enabled) continue;

        for (size_t j = 0; j < mods_list.size(); ++j) {
            if (i == j || !mods_list[j].enabled) continue;

            for (const auto& file_a : mods_list[i].relative_files) {
                if (file_a == "mod_priorities.json") continue;
                if (std::find(mods_list[j].relative_files.begin(), mods_list[j].relative_files.end(), file_a) != mods_list[j].relative_files.end()) {
                    total_conflicts++;
                    if (i < j) {
                        mods_list[i].conflict_warning = tr("⚠️ Перекрывает файлы в «%1» (%2)").arg(mods_list[j].name).arg(QString::fromStdString(file_a));
                    } else {
                        mods_list[i].conflict_warning = tr("⚠️ Файлы перекрываются модом «%1» (низкий приоритет)").arg(mods_list[j].name);
                    }
                    break;
                }
            }
            if (mods_list[i].conflict_warning.startsWith(QStringLiteral("⚠️"))) break;
        }
    }

    if (total_conflicts > 0) {
        conflict_summary_lbl->setText(tr("Обнаружено пересечений файлов LayeredFS: %1. Моды, расположенные выше в списке, имеют наивысший приоритет.").arg(total_conflicts));
        conflict_summary_lbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #F59E0B;"));
    } else {
        conflict_summary_lbl->setText(tr("Пересечений файлов между активными модами не обнаружено. Все моды загружаются бесконфликтно."));
        conflict_summary_lbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #34D399;"));
    }
}

void ModManagerDialog::OnMoveUp() {
    int row = mod_table->currentRow();
    if (row <= 0 || row >= static_cast<int>(mods_list.size())) return;

    std::swap(mods_list[row], mods_list[row - 1]);
    RefreshModsList();
    mod_table->selectRow(row - 1);
}

void ModManagerDialog::OnMoveDown() {
    int row = mod_table->currentRow();
    if (row < 0 || row >= static_cast<int>(mods_list.size()) - 1) return;

    std::swap(mods_list[row], mods_list[row + 1]);
    RefreshModsList();
    mod_table->selectRow(row + 1);
}

void ModManagerDialog::OnItemChanged(QTableWidgetItem* item) {
    if (!item || item->column() != 1) return;
    int row = item->row();
    if (row >= 0 && row < static_cast<int>(mods_list.size())) {
        mods_list[row].enabled = (item->checkState() == Qt::Checked);
        DetectConflicts();
        for (size_t i = 0; i < mods_list.size(); ++i) {
            auto* conf_item = mod_table->item(static_cast<int>(i), 4);
            if (conf_item) {
                conf_item->setText(mods_list[i].conflict_warning);
                if (mods_list[i].conflict_warning.startsWith(QStringLiteral("⚠️"))) {
                    conf_item->setForeground(QColor(245, 158, 11));
                } else {
                    conf_item->setForeground(QColor(52, 211, 153));
                }
            }
        }
    }
}

void ModManagerDialog::OnAddFolder() {
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("Выберите папку мода"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    if (path.isEmpty()) return;

    InstallDroppedPath(path);
    RefreshModsList();
}

void ModManagerDialog::OnAddZip() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Выберите архив с модом"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
        tr("Архивы модов (*.zip *.7z *.rar *.tar.gz *.tar.xz);;Все файлы (*.*)"));
    if (path.isEmpty()) return;

    InstallDroppedPath(path);
    RefreshModsList();
}

void ModManagerDialog::InstallDroppedPath(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists()) return;

    const QString default_name = fi.isDir() ? fi.fileName() : fi.baseName();
    QString safe_name = default_name;
    safe_name = safe_name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_")).trimmed();
    if (safe_name.isEmpty()) safe_name = QStringLiteral("CustomMod");

    const auto target_load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) /
                                 fmt::format("{:016X}", title_id) / safe_name.toStdString();

    if (fi.isDir()) {
        const bool ok = QtCommon::Mod::OrganizeModStructure(
            std::filesystem::path(path.toStdString()), target_load_dir);
        if (ok) {
            QMessageBox::information(this, tr("Мод установлен"), tr("Мод «%1» успешно установлен!").arg(safe_name));
        } else {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось распознать структуру мода в папке %1").arg(path));
        }
    } else {
        const QString extracted = QtCommon::Mod::ExtractMod(path);
        if (!extracted.isEmpty()) {
            const bool ok = QtCommon::Mod::OrganizeModStructure(
                std::filesystem::path(extracted.toStdString()), target_load_dir);
            std::error_code ec;
            std::filesystem::remove_all(std::filesystem::path(extracted.toStdString()), ec);
            if (ok) {
                QMessageBox::information(this, tr("Мод установлен"), tr("Мод «%1» успешно извлечен и установлен!").arg(safe_name));
            } else {
                QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось структурировать распакованный мод из архива."));
            }
        }
    }
}

void ModManagerDialog::OnDeleteMod() {
    int row = mod_table->currentRow();
    if (row < 0 || row >= static_cast<int>(mods_list.size())) return;

    const auto& m = mods_list[row];
    auto ans = QMessageBox::question(
        this, tr("Удаление мода"),
        tr("Вы уверены, что хотите полностью удалить мод «%1» с диска?").arg(m.name),
        QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(m.path.toStdString()), ec);
        RefreshModsList();
    }
}

void ModManagerDialog::OnOpenModFolder() {
    const QString load_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir)));
    const QString tid_str = QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    const QString game_mod_dir = QDir(load_dir).filePath(tid_str);
    QDir().mkpath(game_mod_dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(game_mod_dir));
}

void ModManagerDialog::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ModManagerDialog::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    for (const auto& url : urls) {
        if (url.isLocalFile()) {
            InstallDroppedPath(url.toLocalFile());
        }
    }
    RefreshModsList();
}

void ModManagerDialog::SavePriorities() {
    const QString tid_str = QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    const QString load_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir)));
    const QString game_load_dir = QDir(load_dir).filePath(tid_str);
    QDir().mkpath(game_load_dir);

    nlohmann::json j;
    auto prio_array = nlohmann::json::array();
    for (const auto& m : mods_list) {
        prio_array.push_back(m.name.toStdString());
    }
    j["priorities"] = prio_array;

    const QString prio_file = QDir(game_load_dir).filePath(QStringLiteral("mod_priorities.json"));
    std::ofstream f(prio_file.toStdString(), std::ios::trunc);
    if (f.is_open()) {
        f << std::setw(2) << j << std::endl;
    }
}

void ModManagerDialog::OnApplyAndClose() {
    SavePriorities();

    auto& disabled = Settings::values.disabled_addons[title_id];
    for (const auto& m : mods_list) {
        const std::string name_std = m.name.toStdString();
        auto it = std::find(disabled.begin(), disabled.end(), name_std);
        if (!m.enabled && it == disabled.end()) {
            disabled.push_back(name_std);
        } else if (m.enabled && it != disabled.end()) {
            disabled.erase(it);
        }
    }

    UISettings::values.is_game_list_reload_pending.exchange(true);
    accept();
}

void ModManagerDialog::OnGenerateAutoPatch() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("⚡ Генератор патчей 60/120 FPS и всех форматов экрана"));
    dlg.setMinimumWidth(560);
    dlg.setStyleSheet(styleSheet() + QStringLiteral(
        "QGroupBox { border: 1px solid #1E293B; border-radius: 8px; margin-top: 10px; padding-top: 12px; color: #00D2FF; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QComboBox { background: #0F172A; border: 1px solid #334155; border-radius: 6px; padding: 6px 10px; color: #F1F5F9; }"
        "QCheckBox { color: #E2E8F0; spacing: 8px; }"
    ));

    auto* vbox = new QVBoxLayout(&dlg);
    vbox->setContentsMargins(14, 14, 14, 14);
    vbox->setSpacing(12);

    auto* desc = new QLabel(
        tr("Автоматическое создание ExeFS-мода (<b>.pchtxt</b>) и настройка 3D-камеры / разблокировки FPS для игры <b>%1</b>.")
            .arg(game_name.isEmpty() ? QStringLiteral("0x%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper() : game_name),
        &dlg);
    desc->setWordWrap(true);
    desc->setStyleSheet(QStringLiteral("background: rgba(14, 165, 233, 0.12); border: 1px solid #0284C7; border-radius: 6px; padding: 8px; color: #BAE6FD;"));
    vbox->addWidget(desc);

    // Aspect Ratio Group (All 17 variants)
    auto* aspect_group = new QGroupBox(tr("🖥️ Формат экрана и 3D-камера (Все доступные варианты)"), &dlg);
    auto* aspect_layout = new QVBoxLayout(aspect_group);
    auto* aspect_combo = new QComboBox(aspect_group);

    struct AspectOption {
        Settings::AspectRatio ratio;
        QString tag;
        QString label;
    };
    const std::vector<AspectOption> aspect_options = {
        {Settings::AspectRatio::R16_9,       QStringLiteral("16x9"),     tr("16:9 — Стандартный экран (FullHD / QHD / 4K)")},
        {Settings::AspectRatio::R16_10,      QStringLiteral("16x10"),    tr("16:10 — Steam Deck / Ноутбуки WUXGA")},
        {Settings::AspectRatio::R21_9,       QStringLiteral("21x9"),     tr("21:9 — Ультраширокий UltraWide (2560x1080 / 3440x1440)")},
        {Settings::AspectRatio::R32_9,       QStringLiteral("32x9"),     tr("32:9 — Супер-ультраширокий Super UltraWide (5120x1440)")},
        {Settings::AspectRatio::R4_3,        QStringLiteral("4x3"),      tr("4:3 — Классический экран")},
        {Settings::AspectRatio::R18_9,       QStringLiteral("18x9"),     tr("18:9 — Univisium (2:1)")},
        {Settings::AspectRatio::R19_5_9,     QStringLiteral("19_5x9"),   tr("19.5:9 — Современные смартфоны / OLED")},
        {Settings::AspectRatio::R20_9,       QStringLiteral("20x9"),     tr("20:9 — Широкий киноформат / Смартфоны")},
        {Settings::AspectRatio::R21_10,      QStringLiteral("21x10"),    tr("21:10 — Широкий 16:10")},
        {Settings::AspectRatio::R24_10,      QStringLiteral("24x10"),    tr("24:10 — UltraWide+ (3840x1600)")},
        {Settings::AspectRatio::R32_10,      QStringLiteral("32x10"),    tr("32:10 — Super UltraWide (3840x1200)")},
        {Settings::AspectRatio::R48_9,       QStringLiteral("48x9"),     tr("48:9 — Три монитора Surround (3x 16:9)")},
        {Settings::AspectRatio::R3_2,        QStringLiteral("3x2"),      tr("3:2 — Портативные дисплеи / Surface")},
        {Settings::AspectRatio::R5_4,        QStringLiteral("5x4"),      tr("5:4 — Мониторы SXGA (1280x1024)")},
        {Settings::AspectRatio::R1_1,        QStringLiteral("1x1"),      tr("1:1 — Квадратный формат")},
        {Settings::AspectRatio::Auto_Window, QStringLiteral("AutoWin"),  tr("Авто по размеру окна — Честный 3D-формат")},
        {Settings::AspectRatio::Stretch,     QStringLiteral("Stretch"),  tr("Растянуть во всё окно (Stretch)")},
    };

    const auto current_aspect = Settings::values.aspect_ratio.GetValue();
    int initial_aspect_idx = 2; // Default to 21:9 in patch generator if currently 16:9
    for (size_t i = 0; i < aspect_options.size(); ++i) {
        aspect_combo->addItem(aspect_options[i].label, static_cast<u32>(aspect_options[i].ratio));
        if (aspect_options[i].ratio == current_aspect && current_aspect != Settings::AspectRatio::R16_9) {
            initial_aspect_idx = static_cast<int>(i);
        }
    }
    aspect_combo->setCurrentIndex(initial_aspect_idx);
    aspect_layout->addWidget(aspect_combo);

    auto* chk_3d_frustum = new QCheckBox(
        tr("Патчить матрицу 3D-камеры в NSO (.rodata/.data) без растяжения моделей"), aspect_group);
    chk_3d_frustum->setChecked(true);
    aspect_layout->addWidget(chk_3d_frustum);
    vbox->addWidget(aspect_group);

    // FPS Unlock Group
    auto* fps_group = new QGroupBox(tr("🚀 Разблокировка частоты кадров (FPS Unlock & LSFG)"), &dlg);
    auto* fps_layout = new QVBoxLayout(fps_group);
    auto* fps_combo = new QComboBox(fps_group);
    fps_combo->addItem(tr("60 FPS — Разблокировка VSync и лимита 30 FPS"), 60);
    fps_combo->addItem(tr("90 FPS — Разблокировка 60 FPS + ускорение 150%"), 90);
    fps_combo->addItem(tr("120 FPS — Разблокировка 60 FPS + Генерация кадров LSFG x2"), 120);
    fps_combo->addItem(tr("240 FPS — Разблокировка 60 FPS + Генерация кадров LSFG x4"), 240);
    fps_combo->addItem(tr("Без изменения FPS (только патч формата экрана)"), 0);
    fps_combo->setCurrentIndex(0);
    fps_layout->addWidget(fps_combo);
    vbox->addWidget(fps_group);

    auto* btn_row = new QHBoxLayout();
    btn_row->addStretch(1);
    auto* cancel_b = new QPushButton(tr("Отмена"), &dlg);
    connect(cancel_b, &QPushButton::clicked, &dlg, &QDialog::reject);
    btn_row->addWidget(cancel_b);

    auto* create_b = new QPushButton(tr("⚡ Сгенерировать и активировать патч"), &dlg);
    create_b->setStyleSheet(QStringLiteral("background: #0284C7; color: #FFFFFF; font-weight: bold; border-color: #00D2FF;"));
    connect(create_b, &QPushButton::clicked, &dlg, &QDialog::accept);
    btn_row->addWidget(create_b);
    vbox->addLayout(btn_row);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const int sel_aspect_idx = std::clamp(aspect_combo->currentIndex(), 0, static_cast<int>(aspect_options.size()) - 1);
    const auto chosen_aspect = aspect_options[sel_aspect_idx].ratio;
    const QString aspect_tag = aspect_options[sel_aspect_idx].tag;
    const int target_fps = fps_combo->currentData().toInt();
    const bool patch_3d = chk_3d_frustum->isChecked();

    // Apply live runtime settings
    Settings::values.aspect_ratio.SetValue(chosen_aspect);
    Settings::values.auto_aspect_ratio_patch.SetValue(patch_3d);
    if (target_fps >= 60) {
        Settings::values.auto_60fps_unlock.SetValue(true);
        if (target_fps == 90) {
            Settings::values.use_speed_limit.SetValue(true);
            Settings::values.speed_limit.SetValue(150);
        } else if (target_fps == 120) {
            Settings::values.frame_gen.SetValue(true);
            Settings::values.frame_gen_multiplier.SetValue(2);
            Settings::values.frame_gen_target_rate.SetValue(120);
        } else if (target_fps == 240) {
            Settings::values.frame_gen.SetValue(true);
            Settings::values.frame_gen_multiplier.SetValue(4);
            Settings::values.frame_gen_target_rate.SetValue(240);
        }
    }

    // Try to locate main NSO to extract exact build_id and .rodata/.data 16:9 float offsets
    std::string build_id_hex;
    std::vector<u32> patched_nso_offsets;
    const float target_aspect_val = Layout::AspectRatioWidthOverHeight(chosen_aspect);
    const u32 target_bits = std::bit_cast<u32>(target_aspect_val);
    const u32 default_bits = std::bit_cast<u32>(16.0f / 9.0f);
    const u32 default_bits_alt = default_bits - 1;

    FileSys::VirtualFile main_nso = nullptr;
    try {
        const FileSys::PatchManager pm{title_id, system.GetFileSystemController(), system.GetContentProvider()};
        FileSys::VirtualDir exefs = nullptr;
        const auto update_nca = system.GetContentProvider().GetEntry(
            FileSys::GetUpdateTitleID(title_id), FileSys::ContentRecordType::Program);
        if (update_nca && update_nca->GetExeFS()) {
            exefs = update_nca->GetExeFS();
        }
        if (!exefs) {
            const auto base_nca = system.GetContentProvider().GetEntry(
                title_id, FileSys::ContentRecordType::Program);
            if (base_nca && base_nca->GetExeFS()) {
                exefs = base_nca->GetExeFS();
            }
        }
        if (!exefs && !game_path.isEmpty()) {
            const auto vfile = Core::GetGameFileFromPath(system.GetFilesystem(), game_path.toStdString());
            if (vfile) {
                FileSys::NSP nsp(vfile, title_id);
                if (nsp.GetStatus() == Loader::ResultStatus::Success && nsp.GetExeFS() != nullptr) {
                    exefs = nsp.GetExeFS();
                } else {
                    FileSys::XCI xci(vfile, title_id);
                    if (xci.GetStatus() == Loader::ResultStatus::Success && xci.GetProgramNCA() && xci.GetProgramNCA()->GetExeFS()) {
                        exefs = xci.GetProgramNCA()->GetExeFS();
                    }
                }
            }
        }
        if (exefs) {
            exefs = pm.PatchExeFS(exefs);
            if (exefs) {
                main_nso = exefs->GetFile("main");
            }
        }
    } catch (...) {}

    if (main_nso && main_nso->GetSize() >= sizeof(Loader::NSOHeader)) {
        Loader::NSOHeader nso_header{};
        if (main_nso->ReadObject(&nso_header) == sizeof(Loader::NSOHeader) &&
            nso_header.magic == Common::MakeMagic('N', 'S', 'O', '0')) {
            const auto bid_raw = Common::HexToString(nso_header.build_id);
            const auto last_nz = bid_raw.find_last_not_of('0');
            build_id_hex = (last_nz != std::string::npos) ? bid_raw.substr(0, last_nz + 1) : bid_raw.substr(0, 40);

            if (patch_3d && std::abs(target_aspect_val - (16.0f / 9.0f)) > 0.005f) {
                for (size_t seg_idx = 1; seg_idx <= 2 && seg_idx < nso_header.segments.size(); ++seg_idx) {
                    const u32 seg_size = nso_header.segments[seg_idx].size;
                    const u32 seg_loc = nso_header.segments[seg_idx].location;
                    if (seg_size < sizeof(u32) || seg_size > 128 * 1024 * 1024) {
                        continue;
                    }
                    std::vector<u8> seg_data(seg_size, 0);
                    if (nso_header.IsSegmentCompressed(seg_idx)) {
                        const u32 comp_size = nso_header.segments_compressed_size[seg_idx];
                        std::vector<u8> comp_buf(comp_size);
                        const size_t read_b = main_nso->Read(comp_buf.data(), comp_size, nso_header.segments[seg_idx].offset);
                        void(Common::Compression::DecompressDataLZ4(seg_data.data(), seg_size, comp_buf.data(), read_b));
                    } else {
                        main_nso->Read(seg_data.data(), seg_size, nso_header.segments[seg_idx].offset);
                    }
                    for (u32 pos = 0; pos + sizeof(u32) <= seg_size && patched_nso_offsets.size() < 64; pos += sizeof(u32)) {
                        u32 val = 0;
                        std::memcpy(&val, seg_data.data() + pos, sizeof(u32));
                        if (val == default_bits || val == default_bits_alt) {
                            patched_nso_offsets.push_back(seg_loc + pos);
                        }
                    }
                }
            }
        }
    }

    if (build_id_hex.empty() && system.IsPoweredOn() && system.GetApplicationProcessProgramID() == title_id) {
        const auto bid_raw = Common::HexToString(system.GetApplicationProcessBuildID());
        const auto last_nz = bid_raw.find_last_not_of('0');
        build_id_hex = (last_nz != std::string::npos) ? bid_raw.substr(0, last_nz + 1) : bid_raw.substr(0, 40);
    }
    if (build_id_hex.empty()) {
        build_id_hex = fmt::format("{:016X}000000000000000000000000", title_id);
    }

    // Create mod directory in load/<TitleID>/
    const QString tid_str = QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    const QString load_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir)));
    const QString mod_folder_name = (target_fps > 0)
        ? QStringLiteral("STORM_AutoPatch_%1_%2FPS").arg(aspect_tag).arg(target_fps)
        : QStringLiteral("STORM_AutoPatch_%1").arg(aspect_tag);
    const QString mod_root = QDir(QDir(load_dir).filePath(tid_str)).filePath(mod_folder_name);
    const QString exefs_dir = QDir(mod_root).filePath(QStringLiteral("exefs"));
    QDir().mkpath(exefs_dir);

    const QString pchtxt_path = QDir(exefs_dir).filePath(QStringLiteral("%1.pchtxt").arg(QString::fromStdString(build_id_hex).toUpper()));
    {
        std::ofstream pf(pchtxt_path.toStdString(), std::ios::trunc);
        if (pf.is_open()) {
            pf << "@nsobid-" << build_id_hex << "\n\n";
            pf << "@flag print_values\n";
            pf << "@flag offset_shift 0x100\n\n";
            pf << "// Generated by STORM SWITCH 12.7.1 Universal Aspect Ratio & FPS Auto-Patcher\n";
            pf << fmt::format("// Target Aspect Ratio: {} ({:.5f}), Target FPS: {}\n",
                              aspect_tag.toStdString(), target_aspect_val, target_fps > 0 ? target_fps : 60);
            pf << "@enabled\n";
            const u8 b0 = static_cast<u8>(target_bits & 0xFF);
            const u8 b1 = static_cast<u8>((target_bits >> 8) & 0xFF);
            const u8 b2 = static_cast<u8>((target_bits >> 16) & 0xFF);
            const u8 b3 = static_cast<u8>((target_bits >> 24) & 0xFF);
            for (const u32 off : patched_nso_offsets) {
                pf << fmt::format("{:08X} {:02X}{:02X}{:02X}{:02X}\n", off, b0, b1, b2, b3);
            }
            pf << "@stop\n";
        }
    }

    // Ensure the new mod is enabled
    auto& disabled = Settings::values.disabled_addons[title_id];
    disabled.erase(std::remove(disabled.begin(), disabled.end(), mod_folder_name.toStdString()), disabled.end());

    RefreshModsList();
    SavePriorities();

    QMessageBox::information(
        this,
        tr("⚡ Авто-патч успешно создан и активирован"),
        tr("Создан и включён мод <b>%1</b>!<br><br>"
           "• <b>Формат экрана:</b> %2 (коэффициент W/H: %3)<br>"
           "• <b>Найдено и пропатчено констант 3D-камеры в NSO:</b> %4 (+ активен динамический перехват в реальном времени)<br>"
           "• <b>Режим FPS:</b> %5<br>"
           "• <b>Файл патча:</b> <code>%6</code>")
            .arg(mod_folder_name,
                 aspect_options[sel_aspect_idx].label,
                 QString::number(target_aspect_val, 'f', 4),
                 QString::number(patched_nso_offsets.size()),
                 fps_combo->currentText(),
                 pchtxt_path));
}

