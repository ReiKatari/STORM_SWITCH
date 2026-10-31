// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <QString>
#include "qt_common/config/uisettings.h"

namespace StormTheme {

struct Palette {
    std::string theme_id;
    bool is_light = false;
    QString bg;             // Window / Dialog background
    QString surface;        // Secondary container / Tab / Header background
    QString card;           // Card / Section background
    QString cardAlt;        // Alternate card / item background
    QString border;         // Subtle border (1px solid)
    QString borderHover;    // Hover border
    QString accent;         // Main accent (cyan, purple, emerald, etc.)
    QString accentHover;    // Lighter / brighter accent on hover
    QString textPrimary;    // Heading / primary text
    QString textSecondary;  // Muted secondary text
    QString inputBg;        // Input / editor / table background
    QString itemHover;      // Row / tab hover
    QString itemSelected;   // Row / tab selected
    QString buttonBg;       // Standard button background
    QString buttonBorder;   // Standard button border
};

inline Palette GetThemePalette(const std::string& explicit_theme = {}) {
    std::string theme = explicit_theme;
    if (theme.empty()) {
        theme = UISettings::values.theme;
    }
    if (theme.empty()) {
        theme = "storm_midnight";
    }

    Palette pal;
    pal.theme_id = theme;

    if (theme == "storm_day" || theme == "day" || theme == "light") {
        pal.is_light = true;
        pal.bg = QStringLiteral("#F8FAFC");
        pal.surface = QStringLiteral("#FFFFFF");
        pal.card = QStringLiteral("#F1F5F9");
        pal.cardAlt = QStringLiteral("#E2E8F0");
        pal.border = QStringLiteral("#CBD5E1");
        pal.borderHover = QStringLiteral("#94A3B8");
        pal.accent = QStringLiteral("#0284C7");
        pal.accentHover = QStringLiteral("#0369A1");
        pal.textPrimary = QStringLiteral("#0F172A");
        pal.textSecondary = QStringLiteral("#475569");
        pal.inputBg = QStringLiteral("#FFFFFF");
        pal.itemHover = QStringLiteral("#E2E8F0");
        pal.itemSelected = QStringLiteral("#BAE6FD");
        pal.buttonBg = QStringLiteral("#F1F5F9");
        pal.buttonBorder = QStringLiteral("#CBD5E1");
    } else if (theme == "storm_cyberpunk" || theme == "cyberpunk") {
        pal.bg = QStringLiteral("#0D081E");
        pal.surface = QStringLiteral("#170F33");
        pal.card = QStringLiteral("#22164A");
        pal.cardAlt = QStringLiteral("#2D1D61");
        pal.border = QStringLiteral("#3D2485");
        pal.borderHover = QStringLiteral("#00F0FF");
        pal.accent = QStringLiteral("#00F0FF");
        pal.accentHover = QStringLiteral("#38BDF8");
        pal.textPrimary = QStringLiteral("#FDF2F8");
        pal.textSecondary = QStringLiteral("#F472B6");
        pal.inputBg = QStringLiteral("#0A0618");
        pal.itemHover = QStringLiteral("#22164A");
        pal.itemSelected = QStringLiteral("#3D2485");
        pal.buttonBg = QStringLiteral("#1F1342");
        pal.buttonBorder = QStringLiteral("#3D2485");
    } else if (theme == "storm_warhammer40k" || theme == "warhammer40k") {
        pal.bg = QStringLiteral("#120808");
        pal.surface = QStringLiteral("#1F0E0E");
        pal.card = QStringLiteral("#2E1515");
        pal.cardAlt = QStringLiteral("#3D1C1C");
        pal.border = QStringLiteral("#4D2121");
        pal.borderHover = QStringLiteral("#EF4444");
        pal.accent = QStringLiteral("#EF4444");
        pal.accentHover = QStringLiteral("#F87171");
        pal.textPrimary = QStringLiteral("#FEF2F2");
        pal.textSecondary = QStringLiteral("#FCA5A5");
        pal.inputBg = QStringLiteral("#0E0606");
        pal.itemHover = QStringLiteral("#2E1515");
        pal.itemSelected = QStringLiteral("#4D2121");
        pal.buttonBg = QStringLiteral("#261111");
        pal.buttonBorder = QStringLiteral("#4D2121");
    } else if (theme == "storm_gothic" || theme == "gothic") {
        pal.bg = QStringLiteral("#0A0A0E");
        pal.surface = QStringLiteral("#14141C");
        pal.card = QStringLiteral("#1E1E2A");
        pal.cardAlt = QStringLiteral("#292938");
        pal.border = QStringLiteral("#333347");
        pal.borderHover = QStringLiteral("#DC2626");
        pal.accent = QStringLiteral("#DC2626");
        pal.accentHover = QStringLiteral("#EF4444");
        pal.textPrimary = QStringLiteral("#F8FAFC");
        pal.textSecondary = QStringLiteral("#94A3B8");
        pal.inputBg = QStringLiteral("#08080C");
        pal.itemHover = QStringLiteral("#1E1E2A");
        pal.itemSelected = QStringLiteral("#333347");
        pal.buttonBg = QStringLiteral("#191924");
        pal.buttonBorder = QStringLiteral("#333347");
    } else if (theme == "storm_matrix" || theme == "matrix") {
        pal.bg = QStringLiteral("#061009");
        pal.surface = QStringLiteral("#0C1F12");
        pal.card = QStringLiteral("#132E1B");
        pal.cardAlt = QStringLiteral("#1A3E24");
        pal.border = QStringLiteral("#1E472A");
        pal.borderHover = QStringLiteral("#10B981");
        pal.accent = QStringLiteral("#10B981");
        pal.accentHover = QStringLiteral("#34D399");
        pal.textPrimary = QStringLiteral("#ECFDF5");
        pal.textSecondary = QStringLiteral("#6EE7B7");
        pal.inputBg = QStringLiteral("#040B06");
        pal.itemHover = QStringLiteral("#132E1B");
        pal.itemSelected = QStringLiteral("#1E472A");
        pal.buttonBg = QStringLiteral("#0E2415");
        pal.buttonBorder = QStringLiteral("#1E472A");
    } else if (theme == "storm_elemental" || theme == "elemental") {
        pal.bg = QStringLiteral("#0A131A");
        pal.surface = QStringLiteral("#12202B");
        pal.card = QStringLiteral("#1B2F3E");
        pal.cardAlt = QStringLiteral("#243E52");
        pal.border = QStringLiteral("#2A475E");
        pal.borderHover = QStringLiteral("#06B6D4");
        pal.accent = QStringLiteral("#06B6D4");
        pal.accentHover = QStringLiteral("#22D3EE");
        pal.textPrimary = QStringLiteral("#ECFEFF");
        pal.textSecondary = QStringLiteral("#67E8F9");
        pal.inputBg = QStringLiteral("#070E14");
        pal.itemHover = QStringLiteral("#1B2F3E");
        pal.itemSelected = QStringLiteral("#2A475E");
        pal.buttonBg = QStringLiteral("#152533");
        pal.buttonBorder = QStringLiteral("#2A475E");
    } else if (theme == "storm_fantasy" || theme == "fantasy") {
        pal.bg = QStringLiteral("#130A1F");
        pal.surface = QStringLiteral("#1E1030");
        pal.card = QStringLiteral("#2D1847");
        pal.cardAlt = QStringLiteral("#3B205D");
        pal.border = QStringLiteral("#46256E");
        pal.borderHover = QStringLiteral("#A855F7");
        pal.accent = QStringLiteral("#A855F7");
        pal.accentHover = QStringLiteral("#C084FC");
        pal.textPrimary = QStringLiteral("#FAF5FF");
        pal.textSecondary = QStringLiteral("#D8B4FE");
        pal.inputBg = QStringLiteral("#0E0717");
        pal.itemHover = QStringLiteral("#2D1847");
        pal.itemSelected = QStringLiteral("#46256E");
        pal.buttonBg = QStringLiteral("#231338");
        pal.buttonBorder = QStringLiteral("#46256E");
    } else if (theme == "storm_flagship" || theme == "flagship") {
        pal.bg = QStringLiteral("#0F1221");
        pal.surface = QStringLiteral("#181D33");
        pal.card = QStringLiteral("#222947");
        pal.cardAlt = QStringLiteral("#2C355B");
        pal.border = QStringLiteral("#343E6B");
        pal.borderHover = QStringLiteral("#6366F1");
        pal.accent = QStringLiteral("#6366F1");
        pal.accentHover = QStringLiteral("#818CF8");
        pal.textPrimary = QStringLiteral("#EEF2FF");
        pal.textSecondary = QStringLiteral("#A5B4FC");
        pal.inputBg = QStringLiteral("#0B0E1A");
        pal.itemHover = QStringLiteral("#222947");
        pal.itemSelected = QStringLiteral("#343E6B");
        pal.buttonBg = QStringLiteral("#1C223D");
        pal.buttonBorder = QStringLiteral("#343E6B");
    } else if (theme == "storm_night" || theme == "night") {
        pal.bg = QStringLiteral("#080E1A");
        pal.surface = QStringLiteral("#0F1A2E");
        pal.card = QStringLiteral("#172745");
        pal.cardAlt = QStringLiteral("#1F3359");
        pal.border = QStringLiteral("#243D6B");
        pal.borderHover = QStringLiteral("#3B82F6");
        pal.accent = QStringLiteral("#3B82F6");
        pal.accentHover = QStringLiteral("#60A5FA");
        pal.textPrimary = QStringLiteral("#EFF6FF");
        pal.textSecondary = QStringLiteral("#93C5FD");
        pal.inputBg = QStringLiteral("#060A13");
        pal.itemHover = QStringLiteral("#172745");
        pal.itemSelected = QStringLiteral("#243D6B");
        pal.buttonBg = QStringLiteral("#121F38");
        pal.buttonBorder = QStringLiteral("#243D6B");
    } else if (theme == "storm_dark" || theme == "dark") {
        pal.bg = QStringLiteral("#0D1117");
        pal.surface = QStringLiteral("#161B22");
        pal.card = QStringLiteral("#21262D");
        pal.cardAlt = QStringLiteral("#30363D");
        pal.border = QStringLiteral("#30363D");
        pal.borderHover = QStringLiteral("#38BDF8");
        pal.accent = QStringLiteral("#38BDF8");
        pal.accentHover = QStringLiteral("#7DD3FC");
        pal.textPrimary = QStringLiteral("#F0F6FC");
        pal.textSecondary = QStringLiteral("#8B949E");
        pal.inputBg = QStringLiteral("#0D1117");
        pal.itemHover = QStringLiteral("#21262D");
        pal.itemSelected = QStringLiteral("#30363D");
        pal.buttonBg = QStringLiteral("#1C2128");
        pal.buttonBorder = QStringLiteral("#30363D");
    } else {
        // Default: storm_midnight
        pal.bg = QStringLiteral("#0B0E17");
        pal.surface = QStringLiteral("#111726");
        pal.card = QStringLiteral("#162035");
        pal.cardAlt = QStringLiteral("#1E2B47");
        pal.border = QStringLiteral("#24324D");
        pal.borderHover = QStringLiteral("#8B5CF6");
        pal.accent = QStringLiteral("#8B5CF6");
        pal.accentHover = QStringLiteral("#A78BFA");
        pal.textPrimary = QStringLiteral("#FFFFFF");
        pal.textSecondary = QStringLiteral("#94A3B8");
        pal.inputBg = QStringLiteral("#0E1422");
        pal.itemHover = QStringLiteral("#162035");
        pal.itemSelected = QStringLiteral("#24324D");
        pal.buttonBg = QStringLiteral("#182136");
        pal.buttonBorder = QStringLiteral("#24324D");
    }

    const std::string custom_accent = UISettings::values.accent_color.GetValue();
    if (!custom_accent.empty()) {
        pal.accent = QString::fromStdString(custom_accent);
        pal.accentHover = pal.accent;
    }

    return pal;
}

inline QString GetDialogStyle(const Palette& pal = GetThemePalette()) {
    return QStringLiteral(
        "QDialog {"
        "    background-color: %1;"
        "    color: %2;"
        "    font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLabel {"
        "    color: %2;"
        "}"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "    background-color: %3;"
        "    color: %2;"
        "    border: 1px solid %4;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    min-height: 28px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "    border: 1px solid %5;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %4;"
        "    border-radius: 6px;"
        "    selection-background-color: %6;"
        "    selection-color: %2;"
        "    padding: 4px;"
        "}"
        "QTableWidget, QTableView, QTreeWidget, QTreeView, QListWidget {"
        "    background-color: %3;"
        "    color: %2;"
        "    border: 1px solid %4;"
        "    border-radius: 6px;"
        "    gridline-color: %4;"
        "    selection-background-color: %6;"
        "    selection-color: %2;"
        "}"
        "QHeaderView::section {"
        "    background-color: %7;"
        "    color: %2;"
        "    border: none;"
        "    border-right: 1px solid %4;"
        "    border-bottom: 1px solid %4;"
        "    padding: 6px 10px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox {"
        "    background-color: %8;"
        "    border: 1px solid %4;"
        "    border-radius: 8px;"
        "    margin-top: 24px;"
        "    padding: 14px;"
        "    font-weight: bold;"
        "    color: %2;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 12px;"
        "    padding: 0 6px;"
        "    color: %5;"
        "}"
        "QPushButton {"
        "    background-color: %9;"
        "    color: %2;"
        "    border: 1px solid %10;"
        "    border-radius: 6px;"
        "    padding: 6px 14px;"
        "    font-weight: 600;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %6;"
        "    border: 1px solid %5;"
        "    color: %2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %7;"
        "    border: 1px solid %5;"
        "    padding: 6px 14px;"
        "    color: %2;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %3;"
        "    color: %11;"
        "    border: 1px solid %4;"
        "}"
        "QScrollBar:vertical {"
        "    border: none;"
        "    background-color: transparent;"
        "    width: 8px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: %4;"
        "    min-height: 24px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: %5;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    border: 1px solid #EF4444;"
        "    background-color: rgba(239, 68, 68, 0.18);"
        "    border-radius: 4px;"
        "    width: 15px;"
        "    height: 15px;"
        "}"
        "QCheckBox::indicator:unchecked:hover {"
        "    border: 1px solid #F87171;"
        "    background-color: rgba(239, 68, 68, 0.28);"
        "}"
        "QCheckBox::indicator:checked {"
        "    border: 1px solid #10B981;"
        "    background-color: #10B981;"
        "    border-radius: 4px;"
        "    width: 15px;"
        "    height: 15px;"
        "}"
        "QCheckBox::indicator:checked:hover {"
        "    border: 1px solid #34D399;"
        "    background-color: #34D399;"
        "}"
    ).arg(
        pal.bg,            // %1
        pal.textPrimary,   // %2
        pal.inputBg,       // %3
        pal.border,        // %4
        pal.accent,        // %5
        pal.itemHover,     // %6
        pal.surface,       // %7
        pal.card,          // %8
        pal.buttonBg,      // %9
        pal.buttonBorder,  // %10
        pal.textSecondary  // %11
    );
}

inline QString GetFacetedButtonStyle(const Palette& pal = GetThemePalette()) {
    return QStringLiteral(
        "QPushButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 6px;"
        "    padding: 6px 14px;"
        "    font-weight: 600;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %4;"
        "    border: 1px solid %5;"
        "    color: %2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %6;"
        "    border: 1px solid %5;"
        "    padding: 6px 14px;"
        "    color: %2;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %7;"
        "    color: %8;"
        "    border: 1px solid %3;"
        "}"
    ).arg(
        pal.buttonBg,
        pal.textPrimary,
        pal.buttonBorder,
        pal.itemHover,
        pal.accent,
        pal.surface,
        pal.inputBg,
        pal.textSecondary
    );
}

} // namespace StormTheme
