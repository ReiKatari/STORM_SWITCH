// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <QDialog>
#include <QStringList>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;

class LogViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit LogViewerDialog(QWidget* parent = nullptr);
    ~LogViewerDialog() override;

private slots:
    void OnFilterChanged(int id);
    void OnSearchTextChanged(const QString& text);
    void OnRefreshLog();
    void OnCopyLog();
    void OnExportLog();
    void OnOpenLogFolder();

private:
    enum FilterType {
        FilterAll = 0,
        FilterErrors = 1,
        FilterWarnings = 2,
        FilterInfo = 3,
    };

    void SetupUI();
    void ApplyStormStyles();
    void LoadLogFile();
    void ApplyFilterAndSearch();
    QString FindActiveLogPath() const;

    // UI Widgets
    QLabel* title_label{nullptr};
    QLabel* stats_label{nullptr};
    QLineEdit* search_edit{nullptr};
    QPushButton* clear_search_btn{nullptr};

    QRadioButton* radio_all{nullptr};
    QRadioButton* radio_errors{nullptr};
    QRadioButton* radio_warnings{nullptr};
    QRadioButton* radio_info{nullptr};
    QButtonGroup* filter_group{nullptr};

    QPlainTextEdit* log_view{nullptr};

    QPushButton* refresh_btn{nullptr};
    QPushButton* copy_btn{nullptr};
    QPushButton* export_btn{nullptr};
    QPushButton* open_folder_btn{nullptr};
    QPushButton* close_btn{nullptr};

    // State
    QString active_log_path;
    QStringList raw_lines;
    FilterType current_filter{FilterAll};
};
