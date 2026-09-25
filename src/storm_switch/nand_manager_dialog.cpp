// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "core/core.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/nand_manager_dialog.h"

std::string GetActiveNandProfileName() {
    const auto active_file = Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles" / "active_profile.txt";
    std::ifstream in(active_file);
    if (in.is_open()) {
        std::string name;
        std::getline(in, name);
        if (!name.empty()) {
            return name;
        }
    }
    return "Default";
}

void SetActiveNandProfileName(const std::string& name) {
    const auto prof_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles";
    std::error_code ec;
    std::filesystem::create_directories(prof_dir, ec);
    std::ofstream out(prof_dir / "active_profile.txt", std::ios::trunc);
    if (out.is_open()) {
        out << name << std::endl;
    }
}

NandManagerDialog::NandManagerDialog(QWidget* parent, Core::System& system_)
    : QDialog(parent), system{system_} {
    setWindowTitle(tr("💾 STORM SWITCH — Менеджер профилей NAND"));
    resize(920, 560);
    setMinimumSize(800, 480);

    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0B0F19; color: #E2E8F0; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
        "QTableWidget { background-color: #0A0E17; border: 1px solid #1E293B; border-radius: 6px; gridline-color: #1E293B; selection-background-color: #1E293B; selection-color: #00D2FF; }"
        "QHeaderView::section { background-color: #0F172A; color: #94A3B8; padding: 6px; font-weight: bold; border: 1px solid #1E293B; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A); "
        "border: 1px solid #334155; border-radius: 6px; color: #F1F5F9; padding: 7px 16px; font-weight: 500; }"
        "QPushButton:hover { border: 1px solid #00D2FF; color: #00D2FF; background: #1E293B; }"
        "QPushButton:pressed { background: #0284C7; color: #FFFFFF; }"
        "QPushButton:disabled { background: #0A0E17; border: 1px solid #1E293B; color: #475569; }"
    ));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(14, 14, 14, 14);
    main_layout->setSpacing(10);

    // Header info banner
    auto* banner = new QLabel(
        tr("💡 <b>Менеджер виртуальных профилей NAND</b> позволяет разделять системную память Switch, "
           "установленные игры, прошивки и сохранения по независимым изолированным контейнерам.<br>"
           "Переключение профилей применяется мгновенно при перезапуске эмуляции."),
        this);
    banner->setStyleSheet(QStringLiteral("background: rgba(14, 165, 233, 0.12); border: 1px solid #0284C7; border-radius: 6px; padding: 10px; color: #BAE6FD;"));
    main_layout->addWidget(banner);

    // Active profile indicator
    lbl_active_name = new QLabel(this);
    lbl_active_name->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: bold; color: #34D399; padding: 4px 0;"));
    main_layout->addWidget(lbl_active_name);

    auto* content_layout = new QHBoxLayout();
    content_layout->setSpacing(12);

    // Table of profiles
    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({
        tr("Профиль NAND"),
        tr("Статус"),
        tr("Занято на диске"),
        tr("Сохранений игр")
    });
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);

    connect(table, &QTableWidget::itemSelectionChanged, this, &NandManagerDialog::OnProfileSelected);
    content_layout->addWidget(table, 1);

    // Side Actions Layout
    auto* side_layout = new QVBoxLayout();
    side_layout->setSpacing(8);

    btn_set_active = new QPushButton(tr("✅ Сделать активным"), this);
    btn_set_active->setStyleSheet(QStringLiteral("background: #0284C7; color: #FFFFFF; font-weight: bold; border-color: #00D2FF;"));
    connect(btn_set_active, &QPushButton::clicked, this, &NandManagerDialog::OnSetActive);
    side_layout->addWidget(btn_set_active);

    btn_create = new QPushButton(tr("➕ Создать профиль..."), this);
    connect(btn_create, &QPushButton::clicked, this, &NandManagerDialog::OnCreateProfile);
    side_layout->addWidget(btn_create);

    btn_clone = new QPushButton(tr("📋 Клонировать профиль..."), this);
    connect(btn_clone, &QPushButton::clicked, this, &NandManagerDialog::OnCloneProfile);
    side_layout->addWidget(btn_clone);

    btn_delete = new QPushButton(tr("🗑️ Удалить профиль"), this);
    btn_delete->setStyleSheet(QStringLiteral("color: #EF4444;"));
    connect(btn_delete, &QPushButton::clicked, this, &NandManagerDialog::OnDeleteProfile);
    side_layout->addWidget(btn_delete);

    side_layout->addSpacing(10);

    btn_open_folder = new QPushButton(tr("📂 Открыть папку"), this);
    connect(btn_open_folder, &QPushButton::clicked, this, &NandManagerDialog::OnOpenFolder);
    side_layout->addWidget(btn_open_folder);

    btn_export = new QPushButton(tr("📦 Экспорт копии..."), this);
    connect(btn_export, &QPushButton::clicked, this, &NandManagerDialog::OnExportBackup);
    side_layout->addWidget(btn_export);

    btn_import = new QPushButton(tr("📥 Импорт копии..."), this);
    connect(btn_import, &QPushButton::clicked, this, &NandManagerDialog::OnImportBackup);
    side_layout->addWidget(btn_import);

    side_layout->addStretch(1);
    content_layout->addLayout(side_layout);

    main_layout->addLayout(content_layout);

    // Details label
    lbl_details = new QLabel(this);
    lbl_details->setStyleSheet(QStringLiteral("font-size: 11px; color: #94A3B8;"));
    main_layout->addWidget(lbl_details);

    // Bottom close button
    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->addStretch(1);
    auto* close_btn = new QPushButton(tr("Закрыть"), this);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
    bottom_layout->addWidget(close_btn);
    main_layout->addLayout(bottom_layout);

    RefreshProfiles();
}

NandManagerDialog::~NandManagerDialog() = default;

void NandManagerDialog::CalculateProfileDetails(NandProfileItem& item) {
    qint64 total_size = 0;
    int saves = 0;

    QDir dir(item.path);
    if (dir.exists()) {
        QDirIterator it(item.path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            total_size += it.fileInfo().size();
        }

        QDir save_dir(dir.filePath(QStringLiteral("user/save/0000000000000000")));
        if (save_dir.exists()) {
            saves = save_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
        }
    }

    item.size_bytes = total_size;
    item.save_count = saves;
}

void NandManagerDialog::RefreshProfiles() {
    profiles.clear();
    table->blockSignals(true);
    table->setRowCount(0);

    const QString active_name = QString::fromStdString(GetActiveNandProfileName());
    lbl_active_name->setText(tr("Текущий активный профиль: <b>%1</b>").arg(active_name.isEmpty() ? QStringLiteral("Default") : active_name));

    // 1. Default NAND Profile
    const QString default_nand_path = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand"));

    NandProfileItem default_item{};
    default_item.name = QStringLiteral("Default (Основной)");
    default_item.path = default_nand_path;
    default_item.is_default = true;
    default_item.is_active = (active_name.isEmpty() || active_name == QStringLiteral("Default"));
    CalculateProfileDetails(default_item);
    profiles.push_back(default_item);

    // 2. Custom NAND Profiles
    const QString profiles_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles"));
    QDir pdir(profiles_dir);
    if (pdir.exists()) {
        const auto dirs = pdir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto& d : dirs) {
            NandProfileItem item{};
            item.name = d.fileName();
            item.path = d.absoluteFilePath();
            item.is_default = false;
            item.is_active = (active_name == item.name);
            CalculateProfileDetails(item);
            profiles.push_back(item);
        }
    }

    // Populate table
    for (size_t i = 0; i < profiles.size(); ++i) {
        const auto& p = profiles[i];
        int row = table->rowCount();
        table->insertRow(row);

        // Name
        auto* name_item = new QTableWidgetItem(p.name);
        name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
        name_item->setFont(QFont(font().family(), font().pointSize(), p.is_active ? QFont::Bold : QFont::Normal));
        if (p.is_active) name_item->setForeground(QColor(0, 210, 255));
        table->setItem(row, 0, name_item);

        // Status
        auto* status_item = new QTableWidgetItem(p.is_active ? tr("🟢 Активен") : tr("⚪ Доступен"));
        status_item->setTextAlignment(Qt::AlignCenter);
        status_item->setFlags(status_item->flags() & ~Qt::ItemIsEditable);
        if (p.is_active) status_item->setForeground(QColor(52, 211, 153));
        table->setItem(row, 1, status_item);

        // Size
        double mb = static_cast<double>(p.size_bytes) / (1024.0 * 1024.0);
        QString size_str = (mb >= 1024.0)
            ? tr("%1 ГБ").arg(mb / 1024.0, 0, 'f', 2)
            : tr("%1 МБ").arg(mb, 0, 'f', 1);
        auto* size_item = new QTableWidgetItem(size_str);
        size_item->setTextAlignment(Qt::AlignCenter);
        size_item->setFlags(size_item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 2, size_item);

        // Saves count
        auto* saves_item = new QTableWidgetItem(tr("%1 игр").arg(p.save_count));
        saves_item->setTextAlignment(Qt::AlignCenter);
        saves_item->setFlags(saves_item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 3, saves_item);
    }

    table->blockSignals(false);
    OnProfileSelected();
}

void NandManagerDialog::OnProfileSelected() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) {
        btn_set_active->setEnabled(false);
        btn_delete->setEnabled(false);
        btn_clone->setEnabled(false);
        btn_open_folder->setEnabled(false);
        btn_export->setEnabled(false);
        lbl_details->clear();
        return;
    }

    const auto& p = profiles[row];
    btn_set_active->setEnabled(!p.is_active);
    btn_delete->setEnabled(!p.is_default && !p.is_active);
    btn_clone->setEnabled(true);
    btn_open_folder->setEnabled(true);
    btn_export->setEnabled(true);

    lbl_details->setText(tr("Путь к профилю: <code>%1</code>").arg(p.path));
}

void NandManagerDialog::OnSetActive() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) return;

    if (system.IsPoweredOn()) {
        QMessageBox::warning(
            this, tr("Эмуляция активна"),
            tr("Для переключения активного профиля NAND необходимо предварительно остановить эмуляцию игры."));
        return;
    }

    const auto& p = profiles[row];
    const std::string new_active = p.is_default ? "Default" : p.name.toStdString();
    SetActiveNandProfileName(new_active);

    Common::FS::SetEdenPath(Common::FS::EdenPath::NANDDir, p.path.toStdString());
    QMessageBox::information(
        this, tr("Профиль переключен"),
        tr("Профиль NAND «%1» успешно активирован!").arg(p.name));

    RefreshProfiles();
}

void NandManagerDialog::OnCreateProfile() {
    bool ok = false;
    QString name = QInputDialog::getText(
        this, tr("Создать профиль NAND"),
        tr("Введите название нового профиля (например: Clean, Modded, FW18):"),
        QLineEdit::Normal, QString{}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    name = name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_")).trimmed();
    if (name.isEmpty() || name == QStringLiteral("Default")) {
        QMessageBox::warning(this, tr("Некорректное имя"), tr("Задайте другое имя профиля."));
        return;
    }

    const QString profiles_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles"));
    const QString target_path = QDir(profiles_dir).filePath(name);

    if (QDir(target_path).exists()) {
        QMessageBox::warning(this, tr("Профиль уже существует"), tr("Профиль с таким именем уже создан."));
        return;
    }

    // Create standard directory tree
    QDir().mkpath(target_path + QStringLiteral("/system/save"));
    QDir().mkpath(target_path + QStringLiteral("/system/Contents/registered"));
    QDir().mkpath(target_path + QStringLiteral("/user/save/0000000000000000"));
    QDir().mkpath(target_path + QStringLiteral("/user/Contents/registered"));

    QMessageBox::information(this, tr("Профиль создан"), tr("Новый чистый профиль «%1» успешно создан!").arg(name));
    RefreshProfiles();
}

void NandManagerDialog::OnCloneProfile() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) return;
    const auto& src = profiles[row];

    bool ok = false;
    QString name = QInputDialog::getText(
        this, tr("Клонировать профиль"),
        tr("Введите название для копии профиля «%1»:").arg(src.name),
        QLineEdit::Normal, src.name + QStringLiteral("_Clone"), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    name = name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_")).trimmed();
    const QString profiles_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles"));
    const QString target_path = QDir(profiles_dir).filePath(name);

    if (QDir(target_path).exists()) {
        QMessageBox::warning(this, tr("Профиль уже существует"), tr("Профиль с таким именем уже существует."));
        return;
    }

    std::error_code ec;
    std::filesystem::copy(
        std::filesystem::path(src.path.toStdString()),
        std::filesystem::path(target_path.toStdString()),
        std::filesystem::copy_options::recursive, ec);

    if (!ec) {
        QMessageBox::information(this, tr("Клонирование завершено"), tr("Профиль «%1» успешно склонирован!").arg(name));
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось скопировать данные: %1").arg(QString::fromStdString(ec.message())));
    }

    RefreshProfiles();
}

void NandManagerDialog::OnDeleteProfile() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) return;
    const auto& p = profiles[row];

    if (p.is_default || p.is_active) {
        QMessageBox::warning(this, tr("Запрещено"), tr("Нельзя удалить основной или активный профиль NAND."));
        return;
    }

    auto ans = QMessageBox::question(
        this, tr("Удаление профиля"),
        tr("Вы действительно хотите БЕЗВОЗВРАТНО удалить профиль «%1» и все его сохранения/установленные игры?").arg(p.name),
        QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(p.path.toStdString()), ec);
        RefreshProfiles();
    }
}

void NandManagerDialog::OnOpenFolder() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) return;
    const auto& p = profiles[row];
    QDesktopServices::openUrl(QUrl::fromLocalFile(p.path));
}

void NandManagerDialog::OnExportBackup() {
    int row = table->currentRow();
    if (row < 0 || row >= static_cast<int>(profiles.size())) return;
    const auto& p = profiles[row];

    const QString dest = QFileDialog::getExistingDirectory(this, tr("Выберите папку для сохранения копии профиля"));
    if (dest.isEmpty()) return;

    const QString out_dir = QDir(dest).filePath(QStringLiteral("NAND_Backup_%1").arg(p.name));
    std::error_code ec;
    std::filesystem::copy(
        std::filesystem::path(p.path.toStdString()),
        std::filesystem::path(out_dir.toStdString()),
        std::filesystem::copy_options::recursive, ec);

    if (!ec) {
        QMessageBox::information(this, tr("Экспорт завершен"), tr("Резервная копия профиля успешно создана в:\n%1").arg(out_dir));
    } else {
        QMessageBox::critical(this, tr("Ошибка экспорта"), tr("Не удалось экспортировать: %1").arg(QString::fromStdString(ec.message())));
    }
}

void NandManagerDialog::OnImportBackup() {
    const QString src = QFileDialog::getExistingDirectory(this, tr("Выберите папку с резервной копией NAND"));
    if (src.isEmpty()) return;

    bool ok = false;
    QString name = QInputDialog::getText(
        this, tr("Импорт профиля"),
        tr("Введите имя для импортируемого профиля:"),
        QLineEdit::Normal, QFileInfo(src).fileName(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    name = name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_")).trimmed();
    const QString profiles_dir = QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "nand_profiles"));
    const QString target_path = QDir(profiles_dir).filePath(name);

    if (QDir(target_path).exists()) {
        QMessageBox::warning(this, tr("Профиль существует"), tr("Профиль с таким именем уже существует."));
        return;
    }

    std::error_code ec;
    std::filesystem::copy(
        std::filesystem::path(src.toStdString()),
        std::filesystem::path(target_path.toStdString()),
        std::filesystem::copy_options::recursive, ec);

    if (!ec) {
        QMessageBox::information(this, tr("Импорт завершен"), tr("Профиль «%1» успешно импортирован!").arg(name));
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось скопировать данные: %1").arg(QString::fromStdString(ec.message())));
    }

    RefreshProfiles();
}
