// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "storm_switch/crash_report_dialog.h"

CrashReportDialog::CrashReportDialog(QWidget* parent, const CrashReportInfo& info)
    : QDialog(parent), m_info{info} {
    setWindowTitle(tr("⚠️ STORM SWITCH — Критический сбой игры"));
    resize(780, 560);
    setMinimumSize(680, 480);

    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0B0F19; color: #E2E8F0; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
        "QTextEdit { background-color: #0A0E17; border: 1px solid #1E293B; border-radius: 6px; color: #38BDF8; font-family: 'Consolas', 'Courier New', monospace; font-size: 11px; padding: 8px; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A); "
        "border: 1px solid #334155; border-radius: 6px; color: #F1F5F9; padding: 7px 16px; font-weight: 500; }"
        "QPushButton:hover { border: 1px solid #00D2FF; color: #00D2FF; background: #1E293B; }"
        "QPushButton:pressed { background: #0284C7; color: #FFFFFF; }"
    ));

    SetupUI(info);
}

CrashReportDialog::~CrashReportDialog() = default;

void CrashReportDialog::SetupUI(const CrashReportInfo& info) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(14, 14, 14, 14);
    main_layout->setSpacing(10);

    // Header error banner
    auto* header_layout = new QHBoxLayout();
    auto* error_icon = new QLabel(QStringLiteral("⚠️"), this);
    error_icon->setStyleSheet(QStringLiteral("font-size: 28px;"));
    header_layout->addWidget(error_icon);

    auto* title_box = new QVBoxLayout();
    auto* title_lbl = new QLabel(tr("<b>Критический сбой процесса эмуляции</b>"), this);
    title_lbl->setStyleSheet(QStringLiteral("font-size: 15px; color: #EF4444; font-weight: bold;"));
    title_box->addWidget(title_lbl);

    QString game_desc = info.game_name.isEmpty()
        ? QStringLiteral("Title ID: 0x%1").arg(info.title_id, 16, 16, QLatin1Char('0')).toUpper()
        : QStringLiteral("%1 (0x%2)").arg(info.game_name).arg(info.title_id, 16, 16, QLatin1Char('0')).toUpper();
    auto* game_lbl = new QLabel(game_desc, this);
    game_lbl->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 12px;"));
    title_box->addWidget(game_lbl);

    header_layout->addLayout(title_box, 1);
    main_layout->addLayout(header_layout);

    // Error details box
    QString err_str = info.error_code.isEmpty() ? tr("Необработанное исключение гостевого ядра") : tr("Код ошибки: %1").arg(info.error_code);
    if (!info.error_description.isEmpty()) {
        err_str += QStringLiteral("\n") + info.error_description;
    }
    auto* err_box = new QLabel(err_str, this);
    err_box->setStyleSheet(QStringLiteral("background: rgba(239, 68, 68, 0.12); border: 1px solid #EF4444; border-radius: 6px; padding: 8px; color: #FCA5A5; font-weight: 500;"));
    main_layout->addWidget(err_box);

    // Recommendations banner
    QString rec = info.recommended_action;
    if (rec.isEmpty()) {
        rec = tr("• Проверьте совместимость установленных модов LayeredFS и отключите конфликтующие.<br>"
                 "• Переключите точность GPU Accuracy в режим High.<br>"
                 "• Включите «Авиарежим», если игра пытается выполнить сетевой запрос к отключенным серверам.<br>"
                 "• Очистите шейдерный кэш игры (ПКМ по игре в списке -> Очистить кэш шейдеров).");
    }
    auto* rec_box = new QLabel(tr("💡 <b>Рекомендации по устранению сбоя:</b><br>%1").arg(rec), this);
    rec_box->setStyleSheet(QStringLiteral("background: rgba(14, 165, 233, 0.12); border: 1px solid #0284C7; border-radius: 6px; padding: 8px; color: #BAE6FD;"));
    main_layout->addWidget(rec_box);

    // Registers and Callstack
    auto* reg_header = new QLabel(tr("Дамп регистров CPU ARM64 и стек вызовов:"), this);
    reg_header->setStyleSheet(QStringLiteral("font-weight: bold; color: #CBD5E1;"));
    main_layout->addWidget(reg_header);

    m_register_view = new QTextEdit(this);
    m_register_view->setReadOnly(true);
    m_register_view->setText(info.register_dump.isEmpty() ? tr("[Регистры сохранены в локальный JSON-файл]") : info.register_dump);
    main_layout->addWidget(m_register_view, 1);

    // Buttons layout
    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(8);

    m_btn_copy = new QPushButton(tr("📋 Скопировать отчет"), this);
    connect(m_btn_copy, &QPushButton::clicked, this, &CrashReportDialog::OnCopyReport);
    btn_layout->addWidget(m_btn_copy);

    m_btn_open_folder = new QPushButton(tr("📂 Папка отчетов"), this);
    connect(m_btn_open_folder, &QPushButton::clicked, this, &CrashReportDialog::OnOpenDumpFolder);
    btn_layout->addWidget(m_btn_open_folder);

    btn_layout->addStretch(1);

    m_btn_restart = new QPushButton(tr("🔄 Перезапустить игру"), this);
    m_btn_restart->setStyleSheet(QStringLiteral("background: #0284C7; color: #FFFFFF; font-weight: bold; border-color: #00D2FF;"));
    connect(m_btn_restart, &QPushButton::clicked, this, &CrashReportDialog::OnRestart);
    btn_layout->addWidget(m_btn_restart);

    m_btn_close = new QPushButton(tr("Закрыть"), this);
    connect(m_btn_close, &QPushButton::clicked, this, &QDialog::reject);
    btn_layout->addWidget(m_btn_close);

    main_layout->addLayout(btn_layout);
}

void CrashReportDialog::OnCopyReport() {
    QString report;
    report += QStringLiteral("=== STORM SWITCH CRASH REPORT ===\n");
    report += QStringLiteral("Title ID: 0x%1\n").arg(m_info.title_id, 16, 16, QLatin1Char('0')).toUpper();
    report += QStringLiteral("Game: %1\n").arg(m_info.game_name);
    report += QStringLiteral("Error Code: %1\n").arg(m_info.error_code);
    report += QStringLiteral("Description: %1\n\n").arg(m_info.error_description);
    report += m_info.register_dump;

    QGuiApplication::clipboard()->setText(report);
    m_btn_copy->setText(tr("✅ Отчет скопирован!"));
}

void CrashReportDialog::OnOpenDumpFolder() {
    QString folder = m_info.json_dump_path;
    if (folder.isEmpty() || !QFileInfo(folder).exists()) {
        const QString dump_root = QString::fromStdString(Common::FS::PathToUTF8String(
            Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "crash_reports"));
        const QString tid_str = QStringLiteral("%1").arg(m_info.title_id, 16, 16, QLatin1Char('0')).toUpper();
        folder = QDir(dump_root).filePath(tid_str);
        QDir().mkpath(folder);
    } else {
        folder = QFileInfo(folder).absolutePath();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void CrashReportDialog::OnRestart() {
    m_restart_requested = true;
    accept();
}
