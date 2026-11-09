// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QDialog>
#include <QString>
#include "common/common_types.h"

namespace Core {
class System;
struct GameFixProfile;
}

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QVBoxLayout;

class GameFixDialog : public QDialog {
    Q_OBJECT

public:
    enum class Result {
        ApplyAndLaunch,
        LaunchWithoutChanges,
        Cancel,
    };

    enum class OptimizationMode {
        Fast,
        Balanced,
        Accurate,
        FactoryReset,
    };

    explicit GameFixDialog(QWidget* parent, u64 title_id_, const QString& game_path_,
                           bool force_show_, Core::System& system_,
                           const QPixmap& game_icon_ = QPixmap());
    ~GameFixDialog() override;

    Result Execute();

    bool ShouldApplyFixes() const { return m_apply_fixes; }
    bool ShouldSaveToIni() const { return m_save_to_ini; }
    bool IsDontAskChecked() const { return m_dont_ask_checked; }
    const std::vector<std::string>& GetExcludedKeys() const { return m_excluded_keys; }

private:
    void SetupUI();
    void DetectHardwareInfo();
    void SelectOptimizationMode(OptimizationMode mode);
    void PopulateProfileData();
    void SaveAsSettingsProfile();
    void ResetToDefaults();
    QString GetLocalizedSettingLabel(const std::string& raw_key) const;
    QString FormatSettingValue(const std::string& key, const std::string& val) const;

    u64 m_title_id{0};
    QString m_game_path;
    bool m_force_show{false};
    Core::System& m_system;
    QPixmap m_game_icon;

    const Core::GameFixProfile* m_profile{nullptr};

    bool m_apply_fixes{true};
    bool m_save_to_ini{true};
    bool m_dont_ask_checked{false};
    std::vector<std::string> m_excluded_keys;
    Result m_result{Result::Cancel};

    OptimizationMode m_current_mode{OptimizationMode::Balanced};

    // Detected system hardware
    QString m_cpu_info;
    QString m_ram_info;
    QString m_gpu_info;

    struct FixItem {
        QCheckBox* checkbox{nullptr};
        std::vector<std::string> keys;
        std::string raw_key;
        std::string val_fast;
        std::string val_balanced;
        std::string val_accurate;
        std::string val_reset;
    };
    std::vector<FixItem> m_fix_items;

    // Group 2 mode buttons
    QPushButton* m_mode_btn_fast{nullptr};
    QPushButton* m_mode_btn_balanced{nullptr};
    QPushButton* m_mode_btn_accurate{nullptr};
    QPushButton* m_mode_btn_reset{nullptr};

    // Category layouts in Group 3
    QVBoxLayout* m_cat_graphics_lay{nullptr};
    QVBoxLayout* m_cat_cpu_lay{nullptr};
    QVBoxLayout* m_cat_sync_lay{nullptr};

    QCheckBox* m_dont_ask_cb{nullptr};
    QTableWidget* m_changes_table{nullptr};
};
