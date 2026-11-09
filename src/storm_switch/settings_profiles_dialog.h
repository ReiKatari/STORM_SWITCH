// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <QDialog>
#include <QString>

namespace Core {
class System;
}

class QLineEdit;
class QVBoxLayout;
class QScrollArea;
class QWidget;
class QPushButton;
class QLabel;

struct StormSettingsProfile {
    QString id;
    QString name;
    QString description;
    QString file_path;
    bool is_preset{false};
    qint64 timestamp{0};

    // Summary pills for UI display
    QString res_pill;
    QString gpu_pill;
    QString shaders_pill;
    QString aa_pill;
    QString cpu_pill;
    QString vsync_pill;
};

class SettingsProfilesDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsProfilesDialog(QWidget* parent, Core::System* system = nullptr);
    ~SettingsProfilesDialog() override;

private:
    void SetupUI();
    void ApplyTheme();
    void InitBuiltinPresets();
    void LoadCustomProfiles();
    void RebuildCards();
    void ApplyPreset(int index);
    void ApplyCustomProfile(const StormSettingsProfile& profile);
    void SaveCurrentAsProfile();
    void ImportProfile();
    void ExportProfile(const StormSettingsProfile& profile);
    void RenameProfile(const StormSettingsProfile& profile);
    void DeleteProfile(const StormSettingsProfile& profile);

    Core::System* m_system{nullptr};
    std::vector<StormSettingsProfile> m_builtin_presets;
    std::vector<StormSettingsProfile> m_custom_profiles;

    int m_filter{0}; // 0 = Все, 1 = Базовые пресеты STORM, 2 = Пользовательские
    QString m_search_query;

    QLineEdit* m_search_edit{nullptr};
    QLabel* m_subtitle_label{nullptr};
    QWidget* m_cards_container{nullptr};
    QVBoxLayout* m_cards_layout{nullptr};
    QPushButton* m_tab_all{nullptr};
    QPushButton* m_tab_builtin{nullptr};
    QPushButton* m_tab_custom{nullptr};
};
