// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/cheats_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "core/core.h"
#include "core/file_sys/vfs/vfs.h"
#include "core/loader/loader.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/configure_per_game_cheats.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

namespace {

enum class VectorIcon {
    Copy,
    Check,
};

QIcon MakeVectorIcon(VectorIcon type, const QColor& color, int size = 16) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case VectorIcon::Copy:
        p.drawRoundedRect(QRectF(2.5, 5.5, 8.0, 8.0), 1.5, 1.5);
        p.drawLine(QPointF(5.5, 2.5), QPointF(13.5, 2.5));
        p.drawLine(QPointF(13.5, 2.5), QPointF(13.5, 10.5));
        p.drawLine(QPointF(10.5, 10.5), QPointF(13.5, 10.5));
        break;
    case VectorIcon::Check:
        p.drawLine(QPointF(3.0, 8.0), QPointF(6.5, 11.5));
        p.drawLine(QPointF(6.5, 11.5), QPointF(13.0, 4.5));
        break;
    }
    return QIcon(pix);
}

} // namespace

CheatsDialog::CheatsDialog(QWidget* parent, Core::System& system, u64 title_id, const QString& file_name)
    : QDialog(parent), m_system(system), m_title_id(title_id), m_file_name(file_name) {
    setWindowTitle(StormLang(
        QStringLiteral("Менеджер чит-кодов — STORM SWITCH"),
        QStringLiteral("Cheat Code Manager — STORM SWITCH")
    ));

    resize(880, 600);
    setMinimumSize(720, 500);

    const auto pal = StormTheme::GetThemePalette();
    setStyleSheet(StormDesign::BuildDialogStylesheet(pal));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(10);

    // Instantiate cheats widget first to resolve Build ID and counts
    cheats_widget = new ConfigurePerGameCheats(system, title_id, file_name, this);
    m_build_id_str = cheats_widget->GetBuildId();
    m_game_title = cheats_widget->GetGameTitle();

    // 1. Sleek Bento Game Header Card
    SetupGameHeader(this);
    main_layout->addWidget(findChild<QGroupBox*>(QStringLiteral("cheatsHeaderCard")));

    // 2. Main Cheats Configuration Widget
    main_layout->addWidget(cheats_widget, 1);

    // 3. Bottom Close Bar
    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->setContentsMargins(0, 4, 0, 0);

    auto* close_button = new QPushButton(StormLang(QStringLiteral("Закрыть"), QStringLiteral("Close")), this);
    close_button->setIcon(MakeVectorIcon(VectorIcon::Check, QColor(QStringLiteral("#FFFFFF"))));
    close_button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
        "  color: #FFFFFF;"
        "  border: 1px solid %1;"
        "  border-radius: 6px;"
        "  font-weight: 700;"
        "  font-size: 10pt;"
        "  padding: 7px 24px;"
        "  min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "  background: %1;"
        "  border: 1px solid #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "  background: %2;"
        "}"
    ).arg(pal.accent, pal.accentHover));

    bottom_layout->addStretch(1);
    bottom_layout->addWidget(close_button);
    main_layout->addLayout(bottom_layout);

    connect(close_button, &QPushButton::clicked, this, [this]() {
        if (cheats_widget) {
            cheats_widget->ApplyConfiguration();
        }
        accept();
    });

    connect(cheats_widget, &ConfigurePerGameCheats::CheatsChanged, this, &CheatsDialog::UpdateCheatCountBadge);
    UpdateCheatCountBadge();

    StormUI::StructureEngine::InstallEscDismiss(this);
}

CheatsDialog::~CheatsDialog() = default;

void CheatsDialog::SetupGameHeader(QWidget* parent_widget) {
    const auto pal = StormTheme::GetThemePalette();

    auto* header_card = new QGroupBox(parent_widget);
    header_card->setObjectName(QStringLiteral("cheatsHeaderCard"));
    header_card->setStyleSheet(QStringLiteral(
        "QGroupBox#cheatsHeaderCard {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  padding: 10px 14px;"
        "  margin: 0px;"
        "}"
    ).arg(pal.card, pal.border));

    auto* header_layout = new QVBoxLayout(header_card);
    header_layout->setContentsMargins(10, 8, 10, 8);
    header_layout->setSpacing(6);

    // Top row: Game Title + Active counter pill
    auto* top_row = new QHBoxLayout();
    top_row->setSpacing(10);

    auto* title_lbl = new QLabel(m_game_title, header_card);
    title_lbl->setStyleSheet(QStringLiteral(
        "font-size: 13pt; font-weight: 700; color: %1;"
    ).arg(pal.textPrimary));
    title_lbl->setWordWrap(false);
    top_row->addWidget(title_lbl, 1);

    m_counter_pill = new QLabel(header_card);
    m_counter_pill->setStyleSheet(QStringLiteral(
        "background-color: %1;"
        "color: %2;"
        "border: 1px solid %3;"
        "border-radius: 12px;"
        "padding: 4px 12px;"
        "font-size: 9pt;"
        "font-weight: 700;"
    ).arg(pal.surface, pal.accent, pal.accent));
    top_row->addWidget(m_counter_pill, 0, Qt::AlignRight | Qt::AlignVCenter);
    header_layout->addLayout(top_row);

    // Bottom row: Title ID + Build ID + Copy button
    auto* sub_row = new QHBoxLayout();
    sub_row->setSpacing(8);

    const QString tid_str = StormFormat::FormatTitleId(m_title_id);
    auto* tid_lbl = new QLabel(StormLang(
        QStringLiteral("ID приложения: %1").arg(tid_str),
        QStringLiteral("Title ID: %1").arg(tid_str)
    ), header_card);
    tid_lbl->setStyleSheet(QStringLiteral(
        "background-color: %1;"
        "color: %2;"
        "border: 1px solid %3;"
        "border-radius: 4px;"
        "padding: 3px 8px;"
        "font-family: monospace;"
        "font-size: 9pt;"
    ).arg(pal.inputBg, pal.textSecondary, pal.border));
    sub_row->addWidget(tid_lbl);

    auto* bid_lbl = new QLabel(StormLang(
        QStringLiteral("Build ID: %1").arg(m_build_id_str),
        QStringLiteral("Build ID: %1").arg(m_build_id_str)
    ), header_card);
    bid_lbl->setStyleSheet(QStringLiteral(
        "background-color: %1;"
        "color: %2;"
        "border: 1px solid %3;"
        "border-radius: 4px;"
        "padding: 3px 8px;"
        "font-family: monospace;"
        "font-size: 9pt;"
        "font-weight: 600;"
    ).arg(pal.inputBg, pal.accent, pal.border));
    sub_row->addWidget(bid_lbl);

    m_copy_bid_btn = new QPushButton(StormLang(
        QStringLiteral("Копировать Build ID"),
        QStringLiteral("Copy Build ID")
    ), header_card);
    m_copy_bid_btn->setIcon(MakeVectorIcon(VectorIcon::Copy, QColor(pal.accent)));
    m_copy_bid_btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 3px 10px;"
        "  font-size: 8.5pt;"
        "  font-weight: 600;"
        "  min-height: 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "  color: #FFFFFF;"
        "  border-color: %2;"
        "}"
    ).arg(pal.buttonBg, pal.accent, pal.border));

    connect(m_copy_bid_btn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_build_id_str);
        m_copy_bid_btn->setText(StormLang(QStringLiteral("Скопировано!"), QStringLiteral("Copied!")));
        QTimer::singleShot(1500, m_copy_bid_btn, [this]() {
            if (m_copy_bid_btn) {
                m_copy_bid_btn->setText(StormLang(QStringLiteral("Копировать Build ID"), QStringLiteral("Copy Build ID")));
            }
        });
    });
    sub_row->addWidget(m_copy_bid_btn);

    sub_row->addStretch(1);
    header_layout->addLayout(sub_row);
}

void CheatsDialog::UpdateCheatCountBadge() {
    if (!m_counter_pill || !cheats_widget) return;
    const int total = cheats_widget->GetTotalCheatsCount();
    const int active = cheats_widget->GetActiveCheatsCount();
    if (total == 0) {
        m_counter_pill->setText(StormLang(
            QStringLiteral("Чит-коды: 0"),
            QStringLiteral("Cheats: 0")
        ));
    } else {
        m_counter_pill->setText(StormLang(
            QStringLiteral("Активно читов: %1 из %2").arg(active).arg(total),
            QStringLiteral("Active cheats: %1 of %2").arg(active).arg(total)
        ));
    }
}
