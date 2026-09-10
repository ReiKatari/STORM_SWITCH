// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTranslator>
#include "common/fs/path_util.h"
#include "qt_common/qt_string_lookup.h"
#include "user_data_migration.h"
#include "storm_switch/migration_dialog.h"

// Needs to be included at the end due to https://bugreports.qt.io/browse/QTBUG-73263
#include <filesystem>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGuiApplication>
#include <QProgressDialog>
#include <QRadioButton>
#include <QThread>

UserDataMigrator::UserDataMigrator(QMainWindow* main_window) {
    // NOTE: Logging is not initialized yet, do not produce logs here.

    // Check migration if config directory does not exist
    // TODO: ProfileManager messes with us a bit here, and force-creates the
    // /nand/system/save/8000000000000010/su/avators/profiles.dat file. Find a way to reorder
    // operations and have it create after this guy runs.
    if (!std::filesystem::is_directory(Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir))) {
        ShowMigrationPrompt(main_window);
    }
}

#include <QPainter>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

static void ShowStyledMessageDialog(QWidget* parent, const QString& title, const QString& message, bool is_info = true) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(500);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: #0A0E17;"
        "    color: #F0F6FC;"
        "    border: 1px solid rgba(0, 210, 255, 0.45);"
        "    border-radius: 12px;"
        "}"
        "QLabel {"
        "    color: #F0F6FC;"
        "    font-size: 13px;"
        "    line-height: 1.5;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284C7, stop:1 #00D2FF);"
        "    color: #FFFFFF;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    border: 1px solid #38BDF8;"
        "    border-radius: 6px;"
        "    padding: 8px 24px;"
        "    min-width: 90px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0369A1, stop:1 #38BDF8);"
        "}"
    ));

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto* topLayout = new QHBoxLayout();
    auto* iconLabel = new QLabel(&dialog);
    QPixmap iconPix(36, 36);
    iconPix.fill(Qt::transparent);
    {
        QPainter p(&iconPix);
        p.setRenderHint(QPainter::Antialiasing);
        if (is_info) {
            p.setBrush(QColor(2, 132, 199));
            p.setPen(QPen(QColor(0, 210, 255), 1.5));
            p.drawEllipse(2, 2, 32, 32);
            QFont f = p.font();
            f.setPixelSize(20);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::white);
            p.drawText(QRect(0, 0, 36, 36), Qt::AlignCenter, QStringLiteral("i"));
        } else {
            p.setBrush(QColor(16, 185, 129));
            p.setPen(QPen(QColor(52, 211, 153), 1.5));
            p.drawEllipse(2, 2, 32, 32);
            p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(10, 18, 16, 24);
            p.drawLine(16, 24, 26, 12);
        }
    }
    iconLabel->setPixmap(iconPix);
    iconLabel->setFixedSize(36, 36);

    auto* textLabel = new QLabel(message, &dialog);
    textLabel->setWordWrap(true);

    topLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    topLayout->addSpacing(14);
    topLayout->addWidget(textLabel, 1);
    layout->addLayout(topLayout);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* okBtn = new QPushButton(QObject::tr("OK"), &dialog);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);

    dialog.exec();
}

void UserDataMigrator::ShowMigrationPrompt(QMainWindow* main_window) {
    namespace fs = std::filesystem;
    using namespace QtCommon::StringLookup;

    MigrationDialog migration_prompt;
    migration_prompt.setWindowTitle(QObject::tr("Миграция данных STORM SWITCH"));

    // mutually exclusive
    QButtonGroup* group = new QButtonGroup(&migration_prompt);

    // MACRO MADNESS

#define BUTTON(clazz, name, text, tooltip, checkState)                                             \
    clazz* name = new clazz(&migration_prompt);                                                    \
    name->setText(text);                                                                           \
    name->setToolTip(Lookup(tooltip));                                                             \
    name->setChecked(checkState);                                                                  \
    migration_prompt.addBox(name);

    BUTTON(QCheckBox, clear_shaders, QObject::tr("Очистить кэш шейдеров"), MigrationTooltipClearShader,
           true)

    u32 id = 0;

#define RADIO(name, text, tooltip, checkState)                                                     \
    BUTTON(QRadioButton, name, text, tooltip, checkState)                                          \
    group->addButton(name, ++id);

    RADIO(keep_old, QObject::tr("Сохранить старые данные"), MigrationTooltipKeepOld, true)
    RADIO(clear_old, QObject::tr("Удалить старые данные"), MigrationTooltipClearOld, false)
    RADIO(link_old, QObject::tr("Создать символическую ссылку на каталог"), MigrationTooltipLinkOld, false)

#undef RADIO
#undef BUTTON

    std::vector<Emulator> found{};

    for (const Emulator& emu : legacy_emus)
        if (fs::is_directory(emu.get_user_dir()))
            found.emplace_back(emu);

    if (found.empty()) {
        return;
    }

    // makes my life easier
    qRegisterMetaType<Emulator>();

    QString prompt_text = Lookup(MigrationPromptPrefix);

    // natural language processing is a nightmare
    for (const Emulator& emu : found) {
        prompt_text = prompt_text % QStringLiteral("\n ") % emu.name();

        QAbstractButton* button = migration_prompt.addButton(emu.name());

        // This is cursed, but it's actually the most efficient way by a mile
        button->setProperty("emulator", QVariant::fromValue(emu));
    }

    prompt_text.append(QObject::tr("\n\n"));
    prompt_text = prompt_text % QStringLiteral("\n\n") % Lookup(MigrationPrompt);

    migration_prompt.setText(prompt_text);
    migration_prompt.addButton(QObject::tr("Нет"), true);

    migration_prompt.exec();

    QAbstractButton* button = migration_prompt.clickedButton();

    if (!button || button->text() == QObject::tr("Нет") || button->text() == QStringLiteral("No") || button->text() == QStringLiteral("Отмена")) {
        return ShowMigrationCancelledMessage(main_window);
    }

    MigrationWorker::MigrationStrategy strategy =
        static_cast<MigrationWorker::MigrationStrategy>(group->checkedId());

    selected_emu = button->property("emulator").value<Emulator>();

    MigrateUserData(main_window, clear_shaders->isChecked(), strategy);
}

void UserDataMigrator::ShowMigrationCancelledMessage(QMainWindow* main_window) {
    ShowStyledMessageDialog(
        main_window,
        QObject::tr("Миграция данных STORM SWITCH"),
        QObject::tr("Вы можете вручную повторить этот запрос в любое время, удалив новую директорию конфигурации:\n\n%1")
            .arg(QString::fromStdString(Common::FS::GetEdenPathString(Common::FS::EdenPath::ConfigDir))),
        true);
}

void UserDataMigrator::MigrateUserData(QMainWindow* main_window, const bool clear_shader_cache,
                                       const MigrationWorker::MigrationStrategy strategy) {
    // Create a dialog to let the user know it's migrating
    QProgressDialog* progress = new QProgressDialog(main_window);
    progress->setWindowTitle(QObject::tr("Миграция данных STORM SWITCH"));
    progress->setLabelText(QObject::tr("Выполняется миграция данных, это может занять некоторое время..."));
    progress->setRange(0, 0);
    progress->setCancelButton(nullptr);
    progress->setWindowModality(Qt::WindowModality::ApplicationModal);
    progress->setStyleSheet(QStringLiteral(
        "QProgressDialog {"
        "    background-color: #0A0E17;"
        "    color: #F0F6FC;"
        "    border: 1px solid rgba(0, 210, 255, 0.45);"
        "    border-radius: 12px;"
        "}"
        "QLabel {"
        "    color: #F0F6FC;"
        "    font-size: 13px;"
        "}"
        "QProgressBar {"
        "    border: 1px solid rgba(0, 210, 255, 0.3);"
        "    border-radius: 6px;"
        "    background-color: #111827;"
        "    height: 18px;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00D2FF, stop:1 #0284C7);"
        "    border-radius: 5px;"
        "}"
    ));

    QThread* thread = new QThread(main_window);
    MigrationWorker* worker = new MigrationWorker(selected_emu, clear_shader_cache, strategy);
    worker->moveToThread(thread);

    thread->connect(thread, &QThread::started, worker, &MigrationWorker::process);

    thread->connect(worker, &MigrationWorker::finished, progress,
                    [=, this](const QString& success_text, const std::string& path) {
                        progress->close();
                        ShowStyledMessageDialog(
                            main_window,
                            QObject::tr("Миграция данных STORM SWITCH"),
                            QObject::tr("Миграция данных успешно завершена!\n%1").arg(success_text),
                            false);

                        migrated = true;
                        thread->quit();
                    });

    thread->connect(worker, &MigrationWorker::finished, worker, &QObject::deleteLater);
    thread->connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
    progress->exec();
}
