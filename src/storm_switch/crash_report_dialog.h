// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <QDialog>
#include "common/common_types.h"

class QLabel;
class QPushButton;
class QTextEdit;

struct CrashReportInfo {
    u64 title_id{0};
    QString game_name;
    QString error_code;
    QString error_description;
    QString register_dump;
    QString json_dump_path;
    QString recommended_action;
};

class CrashReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit CrashReportDialog(QWidget* parent, const CrashReportInfo& info);
    ~CrashReportDialog() override;

    bool ShouldRestart() const { return m_restart_requested; }

private slots:
    void OnCopyReport();
    void OnOpenDumpFolder();
    void OnRestart();

private:
    void SetupUI(const CrashReportInfo& info);

    CrashReportInfo m_info;
    bool m_restart_requested{false};

    QTextEdit* m_register_view{nullptr};
    QPushButton* m_btn_copy{nullptr};
    QPushButton* m_btn_open_folder{nullptr};
    QPushButton* m_btn_restart{nullptr};
    QPushButton* m_btn_close{nullptr};
};
