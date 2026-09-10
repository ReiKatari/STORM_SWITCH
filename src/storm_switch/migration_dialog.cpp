// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "migration_dialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

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
}

MigrationDialog::~MigrationDialog() {
    m_boxes->deleteLater();
    m_buttons->deleteLater();
}

void MigrationDialog::ApplyStormStyles() {
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: #0A0E17;"
        "    color: #F0F6FC;"
        "    border: 1px solid rgba(0, 210, 255, 0.45);"
        "    border-radius: 12px;"
        "}"
        "QLabel {"
        "    color: #F0F6FC;"
        "    font-size: 13px;"
        "    line-height: 1.45;"
        "}"
        "QCheckBox {"
        "    color: #E2E8F0;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    spacing: 10px;"
        "    padding: 3px 0;"
        "}"
        "QCheckBox::indicator {"
        "    width: 20px;"
        "    height: 20px;"
        "    border-radius: 4px;"
        "    border: 1.5px solid rgba(0, 210, 255, 0.6);"
        "    background: #111827;"
        "}"
        "QCheckBox::indicator:hover {"
        "    border-color: #00D2FF;"
        "    background: #1F2937;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00D2FF, stop:1 #0284C7);"
        "    border: 1.5px solid #38BDF8;"
        "}"
        "QRadioButton {"
        "    color: #E2E8F0;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    spacing: 10px;"
        "    padding: 3px 0;"
        "}"
        "QRadioButton::indicator {"
        "    width: 20px;"
        "    height: 20px;"
        "    border-radius: 10px;"
        "    border: 1.5px solid rgba(0, 210, 255, 0.6);"
        "    background: #111827;"
        "}"
        "QRadioButton::indicator:hover {"
        "    border-color: #00D2FF;"
        "    background: #1F2937;"
        "}"
        "QRadioButton::indicator:checked {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #00D2FF, stop:0.55 #00D2FF, stop:0.65 #111827, stop:1 #111827);"
        "    border: 1.5px solid #38BDF8;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:1 #0F172A);"
        "    color: #F0F6FC;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    border: 1px solid rgba(0, 210, 255, 0.4);"
        "    border-radius: 6px;"
        "    padding: 8px 18px;"
        "    min-width: 90px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284C7, stop:1 #00D2FF);"
        "    border-color: #38BDF8;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background: #0369A1;"
        "    border-color: #00D2FF;"
        "}"
        "QPushButton#RejectButton {"
        "    background: rgba(46, 16, 20, 0.75);"
        "    color: #FCA5A5;"
        "    border: 1px solid rgba(239, 68, 68, 0.4);"
        "}"
        "QPushButton#RejectButton:hover {"
        "    background: rgba(239, 68, 68, 0.85);"
        "    color: #FFFFFF;"
        "    border-color: #EF4444;"
        "}"
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
