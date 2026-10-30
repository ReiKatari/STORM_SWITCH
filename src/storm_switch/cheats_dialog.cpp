// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include "storm_switch/cheats_dialog.h"
#include "storm_switch/configuration/configure_per_game_cheats.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

CheatsDialog::CheatsDialog(QWidget* parent, Core::System& system, u64 title_id, const QString& file_name)
    : QDialog(parent) {
    setWindowTitle(tr("⚡ Менеджер чит-кодов"));
    resize(1160, 700);
    setMinimumSize(680, 460);
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0b0e14; color: #FFFFFF; }"
        "QLabel { color: #FFFFFF; font-size: 12px; }"
        "QPushButton, QDialogButtonBox QPushButton, QToolButton { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #25354e, stop:0.48 #1e293b, stop:0.52 #182232, stop:1 #0f172a); border-top: 1px solid rgba(255, 255, 255, 0.34); border-left: 1px solid rgba(255, 255, 255, 0.16); border-right: 1px solid rgba(255, 255, 255, 0.16); border-bottom: 3px solid rgba(0, 0, 0, 0.58); border-radius: 8px; padding: 6px 16px; color: #FFFFFF; font-weight: 600; min-height: 22px; }" "QPushButton:hover, QDialogButtonBox QPushButton:hover, QToolButton:hover { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #334664, stop:0.48 #26364d, stop:0.52 #1e2b3e, stop:1 #131c2c); border-top: 1px solid rgba(255, 255, 255, 0.55); border-left: 1px solid rgba(0, 210, 255, 0.45); border-right: 1px solid rgba(0, 210, 255, 0.45); border-bottom: 3px solid rgba(0, 0, 0, 0.70); color: #00D2FF; }" "QPushButton:pressed, QDialogButtonBox QPushButton:pressed, QToolButton:pressed { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0d1522, stop:0.5 #131d2e, stop:1 #1a283e); border-top: 2px solid rgba(0, 0, 0, 0.55); border-bottom: 1px solid rgba(255, 255, 255, 0.14); padding-top: 8px; padding-bottom: 4px; color: #FFFFFF; }"
        "QPushButton:hover { background-color: #243048; border-color: #00d2ff; color: #ffffff; }"
    ));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    cheats_widget = new ConfigurePerGameCheats(system, title_id, file_name, this);
    layout->addWidget(cheats_widget, 1);

    auto* button_layout = new QHBoxLayout();
    auto* close_button = new QPushButton(tr("Закрыть"), this);
    close_button->setStyleSheet(QStringLiteral("background-color: #00f2fe; color: #000000; font-weight: bold; padding: 6px 20px; border-radius: 5px;"));
    button_layout->addStretch(1);
    button_layout->addWidget(close_button);
    layout->addLayout(button_layout);

    connect(close_button, &QPushButton::clicked, this, [this]() {
        if (cheats_widget) {
            cheats_widget->ApplyConfiguration();
        }
        accept();
    });
    StormUI::StructureEngine::ModernizeForm(this);
    StormUI::StructureEngine::InstallEscDismiss(this);
}

CheatsDialog::~CheatsDialog() = default;
