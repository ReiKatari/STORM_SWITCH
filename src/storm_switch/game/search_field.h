// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class GameList;
class QHBoxLayout;
class QTreeView;
class QLabel;
class QLineEdit;
class QComboBox;
class QToolButton;

class GameListSearchField : public QWidget {
    Q_OBJECT

public:
    explicit GameListSearchField(GameList* parent = nullptr);

    QString filterText() const;
    void setFilterResult(int visible_, int total_);

    void clear();
    void setFocus();

    void UpdateViewMode(bool is_grid);

signals:
    void SortModeChanged(int mode);
    void ToggleViewModeRequested();

private:
    void changeEvent(QEvent*) override;
    void RetranslateUI();

    class KeyReleaseEater : public QObject {
    public:
        explicit KeyReleaseEater(GameList* gamelist_, QObject* parent = nullptr);

    private:
        GameList* gamelist = nullptr;
        QString edit_filter_text_old;

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
    };
    int visible = 0;
    int total = 0;

    QHBoxLayout* layout_filter = nullptr;
    QTreeView* tree_view = nullptr;
    QWidget* search_capsule = nullptr;
    QLabel* icon_search = nullptr;
    QLineEdit* edit_filter = nullptr;
    QToolButton* button_clear_text = nullptr;
    QLabel* label_filter_result = nullptr;
    QComboBox* sort_combobox = nullptr;
    QToolButton* view_mode_toggle = nullptr;
    QToolButton* button_filter_close = nullptr;
};
