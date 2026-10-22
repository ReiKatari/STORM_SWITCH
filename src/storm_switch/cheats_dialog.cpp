// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include "storm_switch/cheats_dialog.h"
#include "storm_switch/configuration/configure_per_game_cheats.h"

CheatsDialog::CheatsDialog(QWidget* parent, Core::System& system, u64 title_id, const QString& file_name)
    : QDialog(parent) {
    setWindowTitle(tr("⚡ Менеджер чит-кодов"));
    resize(1160, 700);
    setMinimumSize(680, 460);
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0b0e14; color: #e2e8f0; }"
        "QLabel { color: #cbd5e1; font-size: 12px; }"
        "QPushButton { background-color: #1a2232; color: #e2e8f0; border: 1px solid #2e3c56; border-radius: 6px; padding: 6px 14px; font-weight: bold; }"
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
}

CheatsDialog::~CheatsDialog() = default;
