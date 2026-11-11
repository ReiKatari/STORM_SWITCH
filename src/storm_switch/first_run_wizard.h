// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWizard>
#include <QString>
#include <memory>

namespace Core {
class System;
}

namespace InputCommon {
class InputSubsystem;
}

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;

class FirstRunWizard : public QWizard {
    Q_OBJECT

public:
    explicit FirstRunWizard(QWidget* parent, Core::System& system_, InputCommon::InputSubsystem& input_subsystem_);
    ~FirstRunWizard() override;

    void accept() override;
    void reject() override;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void SetupPages();
    void RetranslateUI();

    Core::System& system;
    InputCommon::InputSubsystem& input_subsystem;

    // Page 1: Language & Theme
    QWizardPage* m_page_lang{nullptr};
    QComboBox* m_lang_combo{nullptr};
    QComboBox* m_theme_combo{nullptr};

    // Page 2: Decryption Keys
    QWizardPage* m_page_keys{nullptr};
    QLabel* m_keys_status_label{nullptr};
    QPushButton* m_import_prod_keys_btn{nullptr};
    QPushButton* m_import_title_keys_btn{nullptr};

    // Page 3: Firmware
    QWizardPage* m_page_firmware{nullptr};
    QLabel* m_firmware_status_label{nullptr};
    QPushButton* m_install_firmware_zip_btn{nullptr};
    QPushButton* m_install_firmware_folder_btn{nullptr};

    // Page 4: Game Directories
    QWizardPage* m_page_gamedirs{nullptr};
    QListWidget* m_gamedirs_list{nullptr};
    QPushButton* m_add_gamedir_btn{nullptr};
    QPushButton* m_remove_gamedir_btn{nullptr};

    void UpdateKeysStatus();
    void UpdateFirmwareStatus();
    void RefreshGameDirsList();
    void CompleteWizard();
};
