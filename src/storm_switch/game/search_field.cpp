// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "game/game_list.h"
#include "game/search_field.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/common/storm_vector_icons.h"

GameListSearchField::KeyReleaseEater::KeyReleaseEater(GameList* gamelist_, QObject* parent)
    : QObject(parent), gamelist{gamelist_} {}

bool GameListSearchField::KeyReleaseEater::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    QString edit_filter_text = gamelist->search_field->edit_filter->text().toLower();

    if (edit_filter_text == edit_filter_text_old) {
        switch (keyEvent->key()) {
        case Qt::Key_Escape: {
            if (edit_filter_text_old.isEmpty()) {
                return QObject::eventFilter(obj, event);
            } else {
                gamelist->search_field->edit_filter->clear();
                edit_filter_text.clear();
            }
            break;
        }
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            if (gamelist->search_field->visible == 1) {
                const QString file_path = gamelist->GetLastFilterResultItem();
                gamelist->search_field->edit_filter->clear();
                edit_filter_text.clear();
                emit gamelist->GameChosen(file_path);
            } else {
                return QObject::eventFilter(obj, event);
            }
            break;
        }
        default:
            return QObject::eventFilter(obj, event);
        }
    }
    edit_filter_text_old = edit_filter_text;
    return QObject::eventFilter(obj, event);
}

void GameListSearchField::setFilterResult(int visible_, int total_) {
    visible = visible_;
    total = total_;

    label_filter_result->setText(tr("%1 из %2 игр").arg(visible).arg(total));
}

QString GameListSearchField::filterText() const {
    return edit_filter->text();
}

void GameListSearchField::clear() {
    edit_filter->clear();
}

void GameListSearchField::setFocus() {
    if (edit_filter->isVisible()) {
        edit_filter->setFocus();
    }
}

GameListSearchField::GameListSearchField(GameList* parent) : QWidget{parent} {
    auto* const key_release_eater = new KeyReleaseEater(parent, this);
    layout_filter = new QHBoxLayout;
    layout_filter->setContentsMargins(10, 6, 10, 6);
    layout_filter->setSpacing(10);

    // 1. Recessed 3D Search Capsule (single outer capsule, transparent inner QLineEdit, zero double border)
    search_capsule = new QWidget(this);
    search_capsule->setObjectName(QStringLiteral("SearchCapsuleWidget"));
    search_capsule->setCursor(Qt::IBeamCursor);

    auto* capsule_layout = new QHBoxLayout(search_capsule);
    capsule_layout->setContentsMargins(10, 3, 10, 3);
    capsule_layout->setSpacing(8);

    const auto pal = StormTheme::GetThemePalette();
    const QColor accent_col(pal.accent.isEmpty() ? QStringLiteral("#8B5CF6") : pal.accent);

    icon_search = new QLabel(search_capsule);
    icon_search->setObjectName(QStringLiteral("SearchCapsuleIcon"));
    icon_search->setPixmap(StormVectorIcons::CreateSearchIcon(16, 16, accent_col).pixmap(16, 16));
    icon_search->setStyleSheet(QStringLiteral("border: none; background: transparent; padding: 0px;"));
    capsule_layout->addWidget(icon_search);

    edit_filter = new QLineEdit(search_capsule);
    edit_filter->setObjectName(QStringLiteral("SearchCapsuleEdit"));
    edit_filter->setFrame(false);
    edit_filter->setClearButtonEnabled(false);
    edit_filter->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    edit_filter->installEventFilter(key_release_eater);
    edit_filter->setStyleSheet(QStringLiteral(
        "QLineEdit#SearchCapsuleEdit {"
        "  border: none !important;"
        "  outline: none !important;"
        "  background: transparent !important;"
        "  color: %1;"
        "  font-size: 13px;"
        "  padding: 0px 2px;"
        "}").arg(pal.textPrimary));
    connect(edit_filter, &QLineEdit::textChanged, parent, &GameList::OnTextChanged);
    capsule_layout->addWidget(edit_filter, 1);

    button_clear_text = new QToolButton(search_capsule);
    button_clear_text->setObjectName(QStringLiteral("SearchCapsuleClear"));
    button_clear_text->setIcon(StormVectorIcons::CreateClearCrossIcon(14, 14));
    button_clear_text->setCursor(Qt::PointingHandCursor);
    button_clear_text->setToolTip(tr("Очистить строку поиска"));
    button_clear_text->setVisible(false);
    button_clear_text->setStyleSheet(QStringLiteral(
        "QToolButton#SearchCapsuleClear {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 2px;"
        "  border-radius: 8px;"
        "}"
        "QToolButton#SearchCapsuleClear:hover {"
        "  background-color: rgba(239, 68, 68, 0.25);"
        "}"));
    connect(button_clear_text, &QToolButton::clicked, edit_filter, &QLineEdit::clear);
    connect(edit_filter, &QLineEdit::textChanged, this, [this](const QString& text) {
        button_clear_text->setVisible(!text.trimmed().isEmpty());
    });
    capsule_layout->addWidget(button_clear_text);

    // 2. Filter result badge
    label_filter_result = new QLabel(this);
    label_filter_result->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 8.5pt; font-weight: 600; padding: 0 4px;"));

    // 3. Sort selector combobox
    sort_combobox = new QComboBox(this);
    sort_combobox->setCursor(Qt::PointingHandCursor);
    connect(sort_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        emit SortModeChanged(idx);
    });

    // 4. View mode toggle (Grid / Tree)
    view_mode_toggle = new QToolButton(this);
    view_mode_toggle->setCursor(Qt::PointingHandCursor);
    connect(view_mode_toggle, &QToolButton::clicked, this, &GameListSearchField::ToggleViewModeRequested);

    // 5. Close search bar button (Vector icon, 0 emoji)
    button_filter_close = new QToolButton(this);
    button_filter_close->setObjectName(QStringLiteral("FilterCloseButton"));
    button_filter_close->setIcon(StormVectorIcons::CreateClearCrossIcon(14, 14));
    button_filter_close->setCursor(Qt::PointingHandCursor);
    button_filter_close->setToolTip(tr("Закрыть строку поиска"));
    button_filter_close->setStyleSheet(QStringLiteral(
        "QToolButton#FilterCloseButton {"
        "  border: 1px solid rgba(239, 68, 68, 0.35);"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  background: rgba(15, 23, 42, 0.50);"
        "}"
        "QToolButton#FilterCloseButton:hover {"
        "  border: 1px solid #EF4444;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #EF4444, stop:1 #B91C1C);"
        "}"));
    connect(button_filter_close, &QToolButton::clicked, parent, &GameList::OnFilterCloseClicked);

    layout_filter->addWidget(search_capsule, 1);
    layout_filter->addWidget(label_filter_result);
    layout_filter->addWidget(sort_combobox);
    layout_filter->addWidget(view_mode_toggle);
    layout_filter->addWidget(button_filter_close);
    setLayout(layout_filter);

    RetranslateUI();
}

void GameListSearchField::UpdateViewMode(bool is_grid) {
    if (view_mode_toggle) {
        if (is_grid) {
            view_mode_toggle->setIcon(StormVectorIcons::CreateTreeViewIcon(16, 16));
            view_mode_toggle->setText(tr("Дерево"));
            view_mode_toggle->setToolTip(tr("Переключить в режим дерева"));
        } else {
            view_mode_toggle->setIcon(StormVectorIcons::CreateGridViewIcon(16, 16));
            view_mode_toggle->setText(tr("Сетка"));
            view_mode_toggle->setToolTip(tr("Переключить в режим сетки"));
        }
    }
}

void GameListSearchField::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void GameListSearchField::RetranslateUI() {
    edit_filter->setPlaceholderText(tr("Поиск по названию или Title ID..."));

    if (total > 0) {
        label_filter_result->setText(tr("%1 из %2 игр").arg(visible).arg(total));
    } else {
        label_filter_result->clear();
    }

    if (sort_combobox) {
        const int cur_idx = sort_combobox->currentIndex();
        sort_combobox->blockSignals(true);
        sort_combobox->clear();
        sort_combobox->addItem(tr("Имя (А–Я)"), 0);
        sort_combobox->addItem(tr("Имя (Я–А)"), 1);
        sort_combobox->addItem(tr("Время игры"), 2);
        sort_combobox->addItem(tr("Размер файла"), 3);
        sort_combobox->addItem(tr("Switch 2 / 1"), 4);
        if (cur_idx >= 0 && cur_idx < sort_combobox->count()) {
            sort_combobox->setCurrentIndex(cur_idx);
        }
        sort_combobox->blockSignals(false);
    }

    UpdateViewMode(UISettings::values.game_list_mode.GetValue() == Settings::GameListMode::GridView);
}
