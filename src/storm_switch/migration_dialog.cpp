// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migration_dialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

MigrationDialog::MigrationDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Миграция данных STORM SWITCH"));
    setMinimumWidth(540);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    m_text = new QLabel(this);
    m_text->setWordWrap(true);
    QFont font = m_text->font();
    font.setPointSize(10);
    m_text->setFont(font);

    m_boxes = new QVBoxLayout;
    m_boxes->setSpacing(8);

    m_buttons = new QHBoxLayout;
    m_buttons->setSpacing(12);

    layout->addWidget(m_text);
    layout->addLayout(m_boxes);
    layout->addSpacing(8);
    layout->addLayout(m_buttons);

    ApplyStormStyles();
    StormUI::StructureEngine::InstallEscDismiss(this);
}

MigrationDialog::~MigrationDialog() {
    m_boxes->deleteLater();
    m_buttons->deleteLater();
}

void MigrationDialog::ApplyStormStyles() {
    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 12px;"
        "    font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLabel {"
        "    color: %2;"
        "    font-size: 13px;"
        "    line-height: 1.45;"
        "}"
        "QCheckBox, QRadioButton {"
        "    color: %2;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    spacing: 10px;"
        "    padding: 3px 0;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    border: 1px solid #EF4444;"
        "    background-color: rgba(239, 68, 68, 0.18);"
        "    border-radius: 4px;"
        "    width: 16px;"
        "    height: 16px;"
        "}"
        "QCheckBox::indicator:checked {"
        "    border: 1px solid #10B981;"
        "    background-color: #10B981;"
        "    border-radius: 4px;"
        "    width: 16px;"
        "    height: 16px;"
        "}"
        "QRadioButton::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border-radius: 8px;"
        "    border: 1px solid %3;"
        "    background: %4;"
        "}"
        "QRadioButton::indicator:checked {"
        "    background: %5;"
        "    border: 1px solid %5;"
        "}"
        "QPushButton {"
        "    background-color: %6;"
        "    color: %2;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    border: 1px solid %3;"
        "    border-radius: 6px;"
        "    padding: 8px 18px;"
        "    min-width: 90px;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %3;"
        "    border-color: %5;"
        "    color: %2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %7;"
        "    border-color: %5;"
        "    padding: 8px 18px;"
        "    color: %2;"
        "}"
        "QPushButton#RejectButton {"
        "    background: rgba(239, 68, 68, 0.15);"
        "    color: #EF4444;"
        "    border: 1px solid #EF4444;"
        "}"
        "QPushButton#RejectButton:hover {"
        "    background: #EF4444;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton#RejectButton:pressed {"
        "    background: #DC2626;"
        "    color: #FFFFFF;"
        "    padding: 8px 18px;"
        "}"
    ).arg(
        pal.bg, pal.textPrimary, pal.border, pal.inputBg, pal.accent, pal.buttonBg, pal.surface
    ));
}

void MigrationDialog::setText(const QString& text) {
    m_text->setText(text);
}

void MigrationDialog::addBox(QWidget* box) {
    m_boxes->addWidget(box);
}

QAbstractButton* MigrationDialog::addButton(const QString& text, const bool reject) {
    QPushButton* button = new QPushButton(this);
    button->setText(text);
    if (reject) {
        button->setObjectName(QStringLiteral("RejectButton"));
    }
    m_buttons->addWidget(button, 1);

    connect(button, &QPushButton::clicked, this, [this, button, reject]() {
        m_clickedButton = button;

        if (reject) {
            this->reject();
        } else {
            this->accept();
        }
    });
    return button;
}

QAbstractButton* MigrationDialog::clickedButton() const {
    return m_clickedButton;
}
