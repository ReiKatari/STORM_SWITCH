// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <QDialog>
#include <QString>
#include "common/common_types.h"

namespace Core {
class System;
}

class ConfigurePerGameCheats;
class QLabel;
class QPushButton;

class CheatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheatsDialog(QWidget* parent, Core::System& system, u64 title_id, const QString& file_name);
    ~CheatsDialog() override;

private:
    void SetupGameHeader(QWidget* parent_widget);
    void UpdateCheatCountBadge();

    Core::System& m_system;
    u64 m_title_id{0};
    QString m_file_name;
    QString m_game_title;
    QString m_build_id_str;

    ConfigurePerGameCheats* cheats_widget{nullptr};
    QLabel* m_counter_pill{nullptr};
    QPushButton* m_copy_bid_btn{nullptr};
};
