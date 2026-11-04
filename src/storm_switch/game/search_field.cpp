// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "game/game_list.h"
#include "game/search_field.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QToolButton>

#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_theme.h"

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
    layout_filter->setContentsMargins(8, 4, 8, 4);
    layout_filter->setSpacing(8);

    label_filter = new QLabel(this);
    edit_filter = new QLineEdit(this);
    edit_filter->clear();
    edit_filter->installEventFilter(key_release_eater);
    edit_filter->setClearButtonEnabled(true);
    connect(edit_filter, &QLineEdit::textChanged, parent, &GameList::OnTextChanged);

    label_filter_result = new QLabel(this);
    label_filter_result->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 8.5pt; font-weight: 500;"));

    // Visible sort selector
    sort_combobox = new QComboBox(this);
    connect(sort_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        emit SortModeChanged(idx);
    });

    // View mode toggle
    view_mode_toggle = new QToolButton(this);
    view_mode_toggle->setCursor(Qt::PointingHandCursor);
    connect(view_mode_toggle, &QToolButton::clicked, this, &GameListSearchField::ToggleViewModeRequested);

    button_filter_close = new QToolButton(this);
    button_filter_close->setObjectName(QStringLiteral("FilterCloseButton"));
    button_filter_close->setText(QStringLiteral("✕"));
    button_filter_close->setCursor(Qt::PointingHandCursor);
    button_filter_close->setStyleSheet(
        QStringLiteral("QToolButton#FilterCloseButton:hover { color: #FFFFFF; border: 1px solid #EF4444; background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #EF4444, stop:1 #B91C1C); }"));
    connect(button_filter_close, &QToolButton::clicked, parent, &GameList::OnFilterCloseClicked);

    layout_filter->addWidget(label_filter);
    layout_filter->addWidget(edit_filter, 1);
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
            view_mode_toggle->setText(tr("Список"));
            view_mode_toggle->setToolTip(tr("Переключить в режим списка"));
        } else {
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
    label_filter->setText(tr("Фильтр:"));
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
