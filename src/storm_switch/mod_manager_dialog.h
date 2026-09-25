// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QDialog>
#include <QTableWidget>
#include "common/common_types.h"

namespace Core {
class System;
}

class ConfigureGameBananaMods;
class QLabel;
class QPushButton;
class QTabWidget;

struct ModEntryInfo {
    QString name;
    QString path;
    bool enabled{true};
    QString type;
    int priority{1};
    QString conflict_warning;
    std::vector<std::string> relative_files;
};

class ModManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ModManagerDialog(QWidget* parent, Core::System& system, u64 title_id,
                              const QString& game_path, const QString& game_name);
    ~ModManagerDialog() override;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void OnMoveUp();
    void OnMoveDown();
    void OnAddFolder();
    void OnAddZip();
    void OnDeleteMod();
    void OnOpenModFolder();
    void OnItemChanged(QTableWidgetItem* item);
    void OnApplyAndClose();

private:
    void SetupPriorityTab(QWidget* tab);
    void RefreshModsList();
    void DetectConflicts();
    void SavePriorities();
    void InstallDroppedPath(const QString& path);

    Core::System& system;
    u64 title_id{0};
    QString game_path;
    QString game_name;

    QTabWidget* tab_widget{nullptr};
    ConfigureGameBananaMods* gamebanana_tab{nullptr};

    QTableWidget* mod_table{nullptr};
    QPushButton* btn_up{nullptr};
    QPushButton* btn_down{nullptr};
    QPushButton* btn_delete{nullptr};
    QLabel* conflict_summary_lbl{nullptr};

    std::vector<ModEntryInfo> mods_list;
};
