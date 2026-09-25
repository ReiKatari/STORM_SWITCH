// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <QDialog>
#include <QTableWidget>

namespace Core {
class System;
}

class QLabel;
class QPushButton;

struct NandProfileItem {
    QString name;
    QString path;
    bool is_active{false};
    bool is_default{false};
    qint64 size_bytes{0};
    int save_count{0};
};

std::string GetActiveNandProfileName();
void SetActiveNandProfileName(const std::string& name);

class NandManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit NandManagerDialog(QWidget* parent, Core::System& system);
    ~NandManagerDialog() override;

private slots:
    void OnProfileSelected();
    void OnSetActive();
    void OnCreateProfile();
    void OnCloneProfile();
    void OnDeleteProfile();
    void OnOpenFolder();
    void OnExportBackup();
    void OnImportBackup();

private:
    void RefreshProfiles();
    void CalculateProfileDetails(NandProfileItem& item);

    Core::System& system;

    QTableWidget* table{nullptr};
    QPushButton* btn_set_active{nullptr};
    QPushButton* btn_create{nullptr};
    QPushButton* btn_clone{nullptr};
    QPushButton* btn_delete{nullptr};
    QPushButton* btn_open_folder{nullptr};
    QPushButton* btn_export{nullptr};
    QPushButton* btn_import{nullptr};

    QLabel* lbl_active_name{nullptr};
    QLabel* lbl_details{nullptr};

    std::vector<NandProfileItem> profiles;
};
