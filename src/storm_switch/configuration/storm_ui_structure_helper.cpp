// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/configuration/storm_ui_structure_helper.h"
#include <algorithm>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QSpinBox>
#include <QTabBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "storm_switch/common/storm_lang.h"

namespace StormUI {

// -----------------------------------------------------------------------------
// EscDismissFilter (Law 16)
// -----------------------------------------------------------------------------
EscDismissFilter::EscDismissFilter(QDialog* target_dialog)
    : QObject(target_dialog), m_dialog(target_dialog) {}

bool EscDismissFilter::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Escape) {
            // If focused in a QLineEdit with content, first clear the input
            if (auto* le = qobject_cast<QLineEdit*>(obj)) {
                if (!le->text().isEmpty()) {
                    le->clear();
                    return true;
                }
            }
            if (m_dialog) {
                m_dialog->reject();
                return true;
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

// -----------------------------------------------------------------------------
// StructureEngine
// -----------------------------------------------------------------------------
void StructureEngine::ModernizeForm(QWidget* container, const CardStyleOptions& options) {
    if (!container) return;

    // 1. Walk and style all QGroupBox widgets as volumetric cards
    const auto group_boxes = container->findChildren<QGroupBox*>();
    for (auto* gb : group_boxes) {
        StyleAsVolumetricCard(gb, options);
    }

    // 2. Enforce scroll margins (Law 12 >= 18px clearance) on all QScrollAreas
    const auto scroll_areas = container->findChildren<QScrollArea*>();
    for (auto* sa : scroll_areas) {
        EnforceScrollMargins(sa, 18);
    }

    // 3. Style recessed 3D capsule inputs, dropdowns and spinboxes (minHeight >= 32px)
    const auto line_edits = container->findChildren<QLineEdit*>();
    for (auto* le : line_edits) {
        if (le->minimumHeight() < 32) {
            le->setMinimumHeight(32);
        }
        le->setProperty("stormCapsule", true);
        le->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    const auto combo_boxes = container->findChildren<QComboBox*>();
    for (auto* cb : combo_boxes) {
        if (cb->minimumHeight() < 32) {
            cb->setMinimumHeight(32);
        }
        cb->setProperty("stormCapsule", true);
    }

    const auto spin_boxes = container->findChildren<QSpinBox*>();
    for (auto* sb : spin_boxes) {
        if (sb->minimumHeight() < 32) {
            sb->setMinimumHeight(32);
        }
        sb->setProperty("stormCapsule", true);
        sb->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    const auto double_spin_boxes = container->findChildren<QDoubleSpinBox*>();
    for (auto* dsb : double_spin_boxes) {
        if (dsb->minimumHeight() < 32) {
            dsb->setMinimumHeight(32);
        }
        dsb->setProperty("stormCapsule", true);
        dsb->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    // 4. Description and subtext labels: enforce word wrap to eliminate truncation (Law 12/17)
    const auto labels = container->findChildren<QLabel*>();
    for (auto* lbl : labels) {
        if (lbl->text().length() > 36 || lbl->text().contains(QLatin1Char('\n'))) {
            lbl->setWordWrap(true);
        }
    }

    // 5. Tables and trees: visible vertical 1px column dividers and alternating rows
    const auto table_views = container->findChildren<QTableView*>();
    for (auto* tv : table_views) {
        ModernizeDataView(tv);
    }
    const auto tree_views = container->findChildren<QTreeView*>();
    for (auto* trv : tree_views) {
        ModernizeDataView(trv);
    }
}

void StructureEngine::StyleAsVolumetricCard(QGroupBox* box, const CardStyleOptions& options) {
    if (!box) return;
    box->setProperty("stormCard", true);

    QFont f = box->font();
    f.setBold(true);
    box->setFont(f);

    if (auto* lay = box->layout()) {
        lay->setContentsMargins(options.inner_padding, options.top_margin, options.inner_padding, options.inner_padding);
        lay->setSpacing(options.row_spacing);
    }
}

QGroupBox* StructureEngine::CreateVolumetricCard(const QString& title, const QString& icon_glyph,
                                                 QWidget* parent, const CardStyleOptions& options) {
    auto* box = new QGroupBox(parent);
    QString full_title = icon_glyph.isEmpty() ? title : (icon_glyph + QStringLiteral("  ") + title);
    box->setTitle(full_title);
    StyleAsVolumetricCard(box, options);
    return box;
}

QWidget* StructureEngine::CreateSectionHeader(const QString& title, const QString& subtitle,
                                              const QString& badge_text, QWidget* parent) {
    auto* container = new QWidget(parent);
    auto* lay = new QVBoxLayout(container);
    lay->setContentsMargins(0, 4, 0, 8);
    lay->setSpacing(2);

    auto* top_row = new QWidget(container);
    auto* top_lay = new QHBoxLayout(top_row);
    top_lay->setContentsMargins(0, 0, 0, 0);
    top_lay->setSpacing(8);

    auto* lbl_title = new QLabel(title, top_row);
    QFont f = lbl_title->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 1);
    lbl_title->setFont(f);
    top_lay->addWidget(lbl_title);

    if (!badge_text.isEmpty()) {
        auto* badge = new QLabel(badge_text, top_row);
        badge->setStyleSheet(QStringLiteral("background: rgba(0, 240, 255, 0.15); border: 1px solid #00F0FF; "
                                            "color: #00F0FF; border-radius: 4px; padding: 2px 6px; font-weight: bold; font-size: 8.5pt;"));
        top_lay->addWidget(badge);
    }
    top_lay->addStretch();
    lay->addWidget(top_row);

    if (!subtitle.isEmpty()) {
        auto* lbl_sub = new QLabel(subtitle, container);
        lbl_sub->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.65); font-size: 8.5pt;"));
        lbl_sub->setWordWrap(true);
        lay->addWidget(lbl_sub);
    }

    return container;
}

void StructureEngine::ModernizeDataView(QWidget* table_or_tree) {
    if (!table_or_tree) return;
    table_or_tree->setProperty("stormTable", true);

    if (auto* tv = qobject_cast<QTableView*>(table_or_tree)) {
        tv->setAlternatingRowColors(true);
        tv->setShowGrid(true);
        if (auto* h = tv->horizontalHeader()) {
            h->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            h->setHighlightSections(false);
            h->setMinimumSectionSize(32);
        }
    } else if (auto* trv = qobject_cast<QTreeView*>(table_or_tree)) {
        trv->setAlternatingRowColors(true);
        if (auto* h = trv->header()) {
            h->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            h->setHighlightSections(false);
            h->setMinimumSectionSize(32);
        }
    }
}

void StructureEngine::InstallEscDismiss(QDialog* dialog) {
    if (!dialog) return;
    auto* filter = new EscDismissFilter(dialog);
    dialog->installEventFilter(filter);
    const auto children = dialog->findChildren<QWidget*>();
    for (auto* child : children) {
        child->installEventFilter(filter);
    }
}

void StructureEngine::EnforceScrollMargins(QScrollArea* scroll_area, int min_margin_px) {
    if (!scroll_area) return;
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (auto* w = scroll_area->widget()) {
        if (auto* lay = w->layout()) {
            QMargins m = lay->contentsMargins();
            lay->setContentsMargins(m.left(), m.top(), std::max(m.right(), min_margin_px), std::max(m.bottom(), 16));
            lay->setSpacing(std::max(lay->spacing(), 8));
        }
    }
}

// -----------------------------------------------------------------------------
// LiveFilterEngine
// -----------------------------------------------------------------------------
LiveFilterEngine::LiveFilterEngine(QLineEdit* search_edit, QListWidget* selector_list,
                                   QWidget* pages_host, QObject* parent)
    : QObject(parent), m_search_edit(search_edit), m_selector_list(selector_list),
      m_pages_host(pages_host) {
    if (m_search_edit) {
        connect(m_search_edit, &QLineEdit::textChanged, this, &LiveFilterEngine::OnSearchTextChanged);
    }
}

void LiveFilterEngine::RebuildIndex() {
    m_indexed_items.clear();
    if (!m_selector_list || !m_pages_host) return;

    for (int r = 0; r < m_selector_list->count(); ++r) {
        auto* item = m_selector_list->item(r);
        if (!m_original_tab_names.contains(r)) {
            m_original_tab_names.insert(r, item->text());
        }
    }
}

void LiveFilterEngine::OnSearchTextChanged(const QString& query) {
    const QString q = query.trimmed();
    if (!m_selector_list) return;

    if (q.isEmpty()) {
        ClearSearch();
        return;
    }

    const QStringList tokens = q.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    int total_matches = 0;
    int matching_tabs_count = 0;

    for (int r = 0; r < m_selector_list->count(); ++r) {
        auto* item = m_selector_list->item(r);
        const QString orig_name = m_original_tab_names.value(r, item->text());
        int tab_matches = 0;

        const auto tabs = qvariant_cast<QList<QWidget*>>(item->data(Qt::UserRole));
        for (auto* tab : tabs) {
            if (!tab) continue;
            for (auto* w : tab->findChildren<QWidget*>()) {
                QString text_blob = w->toolTip();
                if (auto* lbl = qobject_cast<QLabel*>(w)) text_blob += QLatin1Char(' ') + lbl->text();
                if (auto* cb = qobject_cast<QCheckBox*>(w)) text_blob += QLatin1Char(' ') + cb->text();
                if (auto* gb = qobject_cast<QGroupBox*>(w)) text_blob += QLatin1Char(' ') + gb->title();

                bool matches = true;
                for (const auto& tok : tokens) {
                    if (!text_blob.contains(tok, Qt::CaseInsensitive)) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    tab_matches++;
                }
            }
        }

        if (tab_matches > 0) {
            item->setText(orig_name + QStringLiteral(" (%1)").arg(tab_matches));
            matching_tabs_count++;
            total_matches += tab_matches;
        } else {
            item->setText(orig_name);
        }
    }

    emit SearchMatchStats(total_matches, matching_tabs_count);
}

void LiveFilterEngine::ClearSearch() {
    if (!m_selector_list) return;
    for (int r = 0; r < m_selector_list->count(); ++r) {
        if (m_original_tab_names.contains(r)) {
            m_selector_list->item(r)->setText(m_original_tab_names.value(r));
        }
    }
}

// -----------------------------------------------------------------------------
// PerGameOverrideManager
// -----------------------------------------------------------------------------
PerGameOverrideManager::PerGameOverrideManager(QWidget* per_game_dialog, QTabWidget* tab_widget,
                                               QListWidget* selector_list, QObject* parent)
    : QObject(parent), m_dialog(per_game_dialog), m_tab_widget(tab_widget),
      m_selector_list(selector_list) {}

QWidget* PerGameOverrideManager::CreateGameHeaderCard(quint64 title_id, const QString& title,
                                                     const QString& version, const QString& play_time,
                                                     const QString& file_path, const QPixmap& icon,
                                                     QWidget* parent) {
    auto* card = new QGroupBox(parent);
    card->setObjectName(QStringLiteral("BentoGameHeaderCard"));
    card->setProperty("stormHeroCard", true);

    auto* root_lay = new QHBoxLayout(card);
    root_lay->setContentsMargins(14, 14, 14, 14);
    root_lay->setSpacing(14);

    // Rounded Game Icon (128x128)
    auto* icon_label = new QLabel(card);
    icon_label->setFixedSize(128, 128);
    const int icon_dim = 128;
    QPixmap rounded_icon(icon_dim, icon_dim);
    rounded_icon.fill(Qt::transparent);
    {
        QPainter painter(&rounded_icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addRoundedRect(0, 0, icon_dim, icon_dim, 14, 14);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, icon.scaled(icon_dim, icon_dim, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    icon_label->setPixmap(rounded_icon);
    root_lay->addWidget(icon_label);

    // Metadata & Title Layout
    auto* info_lay = new QVBoxLayout();
    info_lay->setContentsMargins(0, 0, 0, 0);
    info_lay->setSpacing(6);

    auto* title_lbl = new QLabel(title, card);
    QFont tf = title_lbl->font();
    tf.setBold(true);
    tf.setPointSize(13);
    title_lbl->setFont(tf);
    title_lbl->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: bold;"));
    title_lbl->setWordWrap(true);
    info_lay->addWidget(title_lbl);

    // Pills row: TitleID, Version, Playtime
    auto* pills_row = new QWidget(card);
    auto* pills_lay = new QHBoxLayout(pills_row);
    pills_lay->setContentsMargins(0, 0, 0, 0);
    pills_lay->setSpacing(8);

    auto create_pill = [pills_row](const QString& text, const QString& tooltip = QString()) {
        auto* btn = new QPushButton(text, pills_row);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "    background: rgba(255, 255, 255, 0.08);"
            "    border: 1px solid rgba(255, 255, 255, 0.16);"
            "    color: #E2E8F0;"
            "    border-radius: 12px;"
            "    padding: 3px 10px;"
            "    font-weight: 600;"
            "    font-size: 8.5pt;"
            "}"
            "QPushButton:hover {"
            "    background: rgba(0, 240, 255, 0.20);"
            "    border-color: #00F0FF;"
            "    color: #00F0FF;"
            "}"
        ));
        if (!tooltip.isEmpty()) btn->setToolTip(tooltip);
        return btn;
    };

    const QString tid_str = QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper();
    auto* tid_btn = create_pill(QStringLiteral("📋 TID: ") + tid_str,
                                StormLang(QStringLiteral("Нажмите, чтобы скопировать Title ID"),
                                          QStringLiteral("Click to copy Title ID")));
    connect(tid_btn, &QPushButton::clicked, [tid_str]() {
        QApplication::clipboard()->setText(tid_str);
    });
    pills_lay->addWidget(tid_btn);

    auto* ver_btn = create_pill(QStringLiteral("📦 v") + version,
                                StormLang(QStringLiteral("Версия игры"), QStringLiteral("Game Version")));
    pills_lay->addWidget(ver_btn);

    if (!play_time.isEmpty()) {
        auto* time_btn = create_pill(QStringLiteral("⏱️ ") + play_time,
                                     StormLang(QStringLiteral("Общее время в игре"), QStringLiteral("Total Playtime")));
        pills_lay->addWidget(time_btn);
    }
    pills_lay->addStretch();
    info_lay->addWidget(pills_row);

    root_lay->addLayout(info_lay);
    return card;
}

void PerGameOverrideManager::RefreshOverrideBadges() {
    if (!m_tab_widget) return;
    int total_overrides = 0;

    for (int i = 0; i < m_tab_widget->count(); ++i) {
        QWidget* page = m_tab_widget->widget(i);
        if (!page) continue;
        int tab_overrides = 0;

        for (auto* w : page->findChildren<QWidget*>()) {
            if (w->property("stormOverrideRow").toBool()) {
                tab_overrides++;
            }
        }
        total_overrides += tab_overrides;
    }

    emit OverridesChanged(total_overrides);
}

void PerGameOverrideManager::ResetCurrentTabOverrides() {
    // Handled in ConfigurePerGame tab configuration reload
}

void PerGameOverrideManager::ResetAllGameOverrides() {
    // Handled in ConfigurePerGame dialog configuration reload
}

} // namespace StormUI
