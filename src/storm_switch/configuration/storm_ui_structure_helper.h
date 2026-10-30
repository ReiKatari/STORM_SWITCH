// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <vector>
#include <QDialog>
#include <QEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMargins>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QTableView>
#include <QTreeView>
#include <QWidget>

namespace StormUI {

/// Visual options for volumetric structured cards
struct CardStyleOptions {
    QString accent_color = QStringLiteral("#00F0FF");
    QString card_bg = QStringLiteral("rgba(15, 23, 42, 0.65)");
    QString border_color = QStringLiteral("rgba(255, 255, 255, 0.08)");
    int border_radius = 8;
    int top_margin = 24;
    int inner_padding = 14;
    int row_spacing = 9;
};

/// Keyboard filter: clean dialog dismissal upon pressing ESC (Law 16)
class EscDismissFilter : public QObject {
    Q_OBJECT
public:
    explicit EscDismissFilter(QDialog* target_dialog);
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
private:
    QDialog* m_dialog;
};

/// Main Structure & Grouping Engine (STORM AURORA FLUID & 23 Laws)
class StructureEngine {
public:
    /// Recursively modernizes a form: volumetric QGroupBox cards, 3D capsule inputs,
    /// safe scrollbar margins (Law 12 >= 18px), table column dividers, and word wrapping.
    static void ModernizeForm(QWidget* container, const CardStyleOptions& options = CardStyleOptions());

    /// Styles a QGroupBox into a volumetric sculpted section card
    static void StyleAsVolumetricCard(QGroupBox* box, const CardStyleOptions& options = CardStyleOptions());

    /// Factory to create a volumetric card programmatically
    static QGroupBox* CreateVolumetricCard(const QString& title, const QString& icon_glyph = QString(),
                                           QWidget* parent = nullptr,
                                           const CardStyleOptions& options = CardStyleOptions());

    /// Creates a section header widget with title, description, and optional badge
    static QWidget* CreateSectionHeader(const QString& title, const QString& subtitle,
                                         const QString& badge_text = QString(),
                                         QWidget* parent = nullptr);

    /// Formats tables & tree views: visible 1px vertical column dividers, alternating rows
    static void ModernizeDataView(QWidget* table_or_tree);

    /// Installs universal ESC dismissal filter on a dialog (Law 16)
    static void InstallEscDismiss(QDialog* dialog);

    /// Guarantees scrollbar clearance >= 18px on the right and >= 16px bottom (Law 12)
    static void EnforceScrollMargins(QScrollArea* scroll_area, int min_margin_px = 18);
};

/// Live Multilingual Filter Engine for Settings Dialogs
class LiveFilterEngine : public QObject {
    Q_OBJECT
public:
    explicit LiveFilterEngine(QLineEdit* search_edit, QListWidget* selector_list,
                              QWidget* pages_host, QObject* parent = nullptr);

    void RebuildIndex();

public slots:
    void OnSearchTextChanged(const QString& query);
    void ClearSearch();

signals:
    void SearchMatchStats(int total_matches, int matching_tabs_count);

private:
    struct SearchItem {
        QWidget* widget = nullptr;
        QGroupBox* parent_group = nullptr;
        int tab_index = -1;
        QString text_blob;
    };

    QLineEdit* m_search_edit;
    QListWidget* m_selector_list;
    QWidget* m_pages_host;
    std::vector<SearchItem> m_indexed_items;
    QMap<int, QString> m_original_tab_names;
};

/// Per-Game Override Manager for tracking and resetting custom overrides
class PerGameOverrideManager : public QObject {
    Q_OBJECT
public:
    explicit PerGameOverrideManager(QWidget* per_game_dialog, QTabWidget* tab_widget,
                                    QListWidget* selector_list = nullptr,
                                    QObject* parent = nullptr);

    static QWidget* CreateGameHeaderCard(quint64 title_id, const QString& title,
                                         const QString& version, const QString& play_time,
                                         const QString& file_path, const QPixmap& icon,
                                         QWidget* parent = nullptr);

    void RefreshOverrideBadges();
    void ResetCurrentTabOverrides();
    void ResetAllGameOverrides();

signals:
    void OverridesChanged(int total_overridden_settings);

private:
    QWidget* m_dialog;
    QTabWidget* m_tab_widget;
    QListWidget* m_selector_list;
};

} // namespace StormUI

// Convenience namespace alias
namespace StormUi = StormUI;
