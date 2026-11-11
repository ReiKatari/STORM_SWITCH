#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/first_run_wizard.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/core.h"
#include "frontend_common/content_manager.h"
#include "frontend_common/firmware_manager.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/content.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"
#include "storm_switch/libqt_common.h"

FirstRunWizard::FirstRunWizard(QWidget* parent, Core::System& system_,
                               InputCommon::InputSubsystem& input_subsystem_)
    : QWizard(parent), system(system_), input_subsystem(input_subsystem_) {
    setWindowTitle(StormLang(
        QStringLiteral("STORM SWITCH - Мастер первоначальной настройки"),
        QStringLiteral("STORM SWITCH - Initial Setup Wizard"),
        QStringLiteral("STORM SWITCH - Ersteinrichtungsassistent"),
        QStringLiteral("STORM SWITCH - Assistant de configuration initiale"),
        QStringLiteral("STORM SWITCH - 初始设置向导"),
        QStringLiteral("STORM SWITCH - 初期セットアップウィザード")
    ));

    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::HaveHelpButton, false);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    resize(760, 520);
    setMinimumSize(680, 460);

    setProperty("_stormDialogThemed", true);
    const auto frw_pal = StormTheme::GetThemePalette();
    setStyleSheet(StormDesign::BuildDialogStylesheet(frw_pal) + StormCancelButtonQssRule());

    // ESC dismisses
    StormUI::StructureEngine::InstallEscDismiss(this);
    setAcceptDrops(true);

    // Custom button texts
    setButtonText(QWizard::NextButton, StormLang(QStringLiteral("Далее →"), QStringLiteral("Next →")));
    setButtonText(QWizard::BackButton, StormLang(QStringLiteral("← Назад"), QStringLiteral("← Back")));
    setButtonText(QWizard::FinishButton, StormLang(QStringLiteral("Завершить и начать"), QStringLiteral("Finish & Start")));
    setButtonText(QWizard::CancelButton, StormLang(QStringLiteral("Пропустить"), QStringLiteral("Skip")));

    SetupPages();
}

FirstRunWizard::~FirstRunWizard() = default;

void FirstRunWizard::SetupPages() {
    const auto frw_pal = StormTheme::GetThemePalette();
    // -------------------------------------------------------------------------
    // Page 1: Language & Theme
    // -------------------------------------------------------------------------
    m_page_lang = new QWizardPage(this);
    m_page_lang->setTitle(StormLang(
        QStringLiteral("Добро пожаловать в STORM SWITCH!"),
        QStringLiteral("Welcome to STORM SWITCH!")));
    m_page_lang->setSubTitle(StormLang(
        QStringLiteral("Выберите предпочитаемый язык интерфейса и визуальную тему оформления эмулятора:"),
        QStringLiteral("Select your preferred interface language and visual theme:")));

    auto* lay1 = new QVBoxLayout(m_page_lang);
    lay1->setContentsMargins(24, 20, 24, 20);
    lay1->setSpacing(14);

    auto* lang_lbl = new QLabel(StormLang(QStringLiteral("Язык интерфейса:"), QStringLiteral("Interface Language:")), m_page_lang);
    lang_lbl->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 10pt;"));
    lay1->addWidget(lang_lbl);

    m_lang_combo = new QComboBox(m_page_lang);
    m_lang_combo->setObjectName(QStringLiteral("WizardLangCombo"));
    m_lang_combo->addItem(QStringLiteral("Русский (Russian)"), QStringLiteral("ru"));
    m_lang_combo->addItem(QStringLiteral("English (US)"), QStringLiteral("en"));
    m_lang_combo->addItem(QStringLiteral("Deutsch (German)"), QStringLiteral("de"));
    m_lang_combo->addItem(QStringLiteral("Français (French)"), QStringLiteral("fr"));
    m_lang_combo->addItem(QStringLiteral("Español (Spanish)"), QStringLiteral("es"));
    m_lang_combo->addItem(QStringLiteral("简体中文 (Simplified Chinese)"), QStringLiteral("zh"));
    m_lang_combo->addItem(QStringLiteral("日本語 (Japanese)"), QStringLiteral("ja"));
    m_lang_combo->addItem(QStringLiteral("한국어 (Korean)"), QStringLiteral("ko"));
    m_lang_combo->addItem(QStringLiteral("Italiano (Italian)"), QStringLiteral("it"));
    m_lang_combo->addItem(QStringLiteral("Português (Portuguese)"), QStringLiteral("pt"));
    m_lang_combo->addItem(QStringLiteral("العربية (Arabic)"), QStringLiteral("ar"));

    const std::string cur_lang = UISettings::values.language.GetValue();
    int lang_idx = m_lang_combo->findData(QString::fromStdString(cur_lang));
    if (lang_idx >= 0) {
        m_lang_combo->setCurrentIndex(lang_idx);
    }
    connect(m_lang_combo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        const QString l = m_lang_combo->itemData(idx).toString();
        UISettings::values.language = l.toStdString();
    });
    lay1->addWidget(m_lang_combo);

    auto* theme_lbl = new QLabel(StormLang(QStringLiteral("Тема оформления STORM:"), QStringLiteral("Visual Theme:")), m_page_lang);
    theme_lbl->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 10pt; margin-top: 12px;"));
    lay1->addWidget(theme_lbl);

    m_theme_combo = new QComboBox(m_page_lang);
    m_theme_combo->setObjectName(QStringLiteral("WizardThemeCombo"));
    m_theme_combo->addItem(QStringLiteral("STORM Dark (Кибер-сланец / Неон)"), QStringLiteral("storm_dark"));
    m_theme_combo->addItem(QStringLiteral("STORM Midnight (Глубокая полуночная)"), QStringLiteral("storm_midnight"));
    m_theme_combo->addItem(QStringLiteral("STORM Day (Светлая высококонтрастная)"), QStringLiteral("storm_day"));
    m_theme_combo->addItem(QStringLiteral("STORM Cyberpunk (Неоновый киберпанк)"), QStringLiteral("storm_cyberpunk"));
    m_theme_combo->addItem(QStringLiteral("STORM Matrix (Цифровая матрица)"), QStringLiteral("storm_matrix"));
    m_theme_combo->addItem(QStringLiteral("STORM Elemental (Стихийная)"), QStringLiteral("storm_elemental"));
    m_theme_combo->addItem(QStringLiteral("STORM Flagship (Флагманская)"), QStringLiteral("storm_flagship"));

    const std::string cur_theme = UISettings::values.theme;
    int theme_idx = m_theme_combo->findData(QString::fromStdString(cur_theme));
    if (theme_idx >= 0) {
        m_theme_combo->setCurrentIndex(theme_idx);
    }
    connect(m_theme_combo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        const QString th = m_theme_combo->itemData(idx).toString();
        UISettings::values.theme = th.toStdString();
    });
    lay1->addWidget(m_theme_combo);
    lay1->addStretch();
    addPage(m_page_lang);

    // -------------------------------------------------------------------------
    // Page 2: Decryption Keys
    // -------------------------------------------------------------------------
    m_page_keys = new QWizardPage(this);
    m_page_keys->setTitle(StormLang(
        QStringLiteral("Ключи дешифрования (prod.keys / title.keys)"),
        QStringLiteral("Decryption Keys (prod.keys / title.keys)")));
    m_page_keys->setSubTitle(StormLang(
        QStringLiteral("Для расшифровки и запуска игр необходимы ключи Nintendo Switch."),
        QStringLiteral("Decryption keys dumped from your Nintendo Switch console are required to launch games.")));

    auto* lay2 = new QVBoxLayout(m_page_keys);
    lay2->setContentsMargins(24, 20, 24, 20);
    lay2->setSpacing(14);

    m_keys_status_label = new QLabel(m_page_keys);
    m_keys_status_label->setWordWrap(true);
    UpdateKeysStatus();
    lay2->addWidget(m_keys_status_label);

    auto* keys_btn_box = new QWidget(m_page_keys);
    auto* keys_btn_lay = new QHBoxLayout(keys_btn_box);
    keys_btn_lay->setContentsMargins(0, 0, 0, 0);
    keys_btn_lay->setSpacing(10);

    m_import_prod_keys_btn = new QPushButton(StormLang(
        QStringLiteral("Выбрать файл prod.keys..."),
        QStringLiteral("Browse for prod.keys...")), keys_btn_box);
    m_import_prod_keys_btn->setCursor(Qt::PointingHandCursor);
    connect(m_import_prod_keys_btn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
            StormLang(QStringLiteral("Выберите файл prod.keys"), QStringLiteral("Select prod.keys file")),
            QString(), QStringLiteral("Key Files (*.keys);;All Files (*.*)"));
        if (!path.isEmpty()) {
            FirmwareManager::InstallKeys(path.toStdString(), "keys");
            UpdateKeysStatus();
        }
    });
    keys_btn_lay->addWidget(m_import_prod_keys_btn);

    m_import_title_keys_btn = new QPushButton(StormLang(
        QStringLiteral("Выбрать файл title.keys..."),
        QStringLiteral("Browse for title.keys...")), keys_btn_box);
    m_import_title_keys_btn->setCursor(Qt::PointingHandCursor);
    connect(m_import_title_keys_btn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
            StormLang(QStringLiteral("Выберите файл title.keys"), QStringLiteral("Select title.keys file")),
            QString(), QStringLiteral("Key Files (*.keys);;All Files (*.*)"));
        if (!path.isEmpty()) {
            FirmwareManager::InstallKeys(path.toStdString(), "keys");
            UpdateKeysStatus();
        }
    });
    keys_btn_lay->addWidget(m_import_title_keys_btn);
    lay2->addWidget(keys_btn_box);

    auto* keys_hint = new QLabel(StormLang(
        QStringLiteral("Если у вас пока нет файлов ключей, вы можете пропустить этот шаг и добавить их позже через меню «Инструменты»."),
        QStringLiteral("If you don't have keys right now, you can skip this step and install them later via the Tools menu.")), m_page_keys);
    keys_hint->setStyleSheet(QStringLiteral("color: %1; font-size: 8.5pt;").arg(frw_pal.textSecondary));
    keys_hint->setWordWrap(true);
    lay2->addWidget(keys_hint);
    lay2->addStretch();
    addPage(m_page_keys);

    // -------------------------------------------------------------------------
    // Page 3: Switch Firmware
    // -------------------------------------------------------------------------
    m_page_firmware = new QWizardPage(this);
    m_page_firmware->setTitle(StormLang(
        QStringLiteral("Системная прошивка Switch"),
        QStringLiteral("Switch System Firmware")));
    m_page_firmware->setSubTitle(StormLang(
        QStringLiteral("Прошивка требуется для системных шрифтов, аватаров Mii и корректной работы апплетов."),
        QStringLiteral("System firmware is recommended for system fonts, Mii applets, and full game compatibility.")));

    auto* lay3 = new QVBoxLayout(m_page_firmware);
    lay3->setContentsMargins(24, 20, 24, 20);
    lay3->setSpacing(14);

    m_firmware_status_label = new QLabel(m_page_firmware);
    m_firmware_status_label->setWordWrap(true);
    UpdateFirmwareStatus();
    lay3->addWidget(m_firmware_status_label);

    auto* fw_btn_box = new QWidget(m_page_firmware);
    auto* fw_btn_lay = new QHBoxLayout(fw_btn_box);
    fw_btn_lay->setContentsMargins(0, 0, 0, 0);
    fw_btn_lay->setSpacing(10);

    m_install_firmware_zip_btn = new QPushButton(StormLang(
        QStringLiteral("Установить из архива ZIP..."),
        QStringLiteral("Install from ZIP archive...")), fw_btn_box);
    m_install_firmware_zip_btn->setCursor(Qt::PointingHandCursor);
    connect(m_install_firmware_zip_btn, &QPushButton::clicked, this, [this]() {
        QtCommon::Content::InstallFirmwareZip();
        UpdateFirmwareStatus();
    });
    fw_btn_lay->addWidget(m_install_firmware_zip_btn);

    m_install_firmware_folder_btn = new QPushButton(StormLang(
        QStringLiteral("Установить из папки..."),
        QStringLiteral("Install from Folder...")), fw_btn_box);
    m_install_firmware_folder_btn->setCursor(Qt::PointingHandCursor);
    connect(m_install_firmware_folder_btn, &QPushButton::clicked, this, [this]() {
        QtCommon::Content::InstallFirmware();
        UpdateFirmwareStatus();
    });
    fw_btn_lay->addWidget(m_install_firmware_folder_btn);
    lay3->addWidget(fw_btn_box);

    auto* fw_hint = new QLabel(StormLang(
        QStringLiteral("Установка прошивки не является строго обязательной для всех игр. Вы можете пропустить этот шаг."),
        QStringLiteral("Firmware installation is optional for many titles. You can skip this step and proceed.")), m_page_firmware);
    fw_hint->setStyleSheet(QStringLiteral("color: %1; font-size: 8.5pt;").arg(frw_pal.textSecondary));
    fw_hint->setWordWrap(true);
    lay3->addWidget(fw_hint);
    lay3->addStretch();
    addPage(m_page_firmware);

    // -------------------------------------------------------------------------
    // Page 4: Game Directories
    // -------------------------------------------------------------------------
    m_page_gamedirs = new QWizardPage(this);
    m_page_gamedirs->setTitle(StormLang(
        QStringLiteral("Каталог игр"),
        QStringLiteral("Game Library Directories")));
    m_page_gamedirs->setSubTitle(StormLang(
        QStringLiteral("Укажите одну или несколько папок, где хранятся дампы ваших игр (.nsp, .xci):"),
        QStringLiteral("Specify one or more folders where your dumped games (.nsp, .xci) are stored:")));

    auto* lay4 = new QVBoxLayout(m_page_gamedirs);
    lay4->setContentsMargins(24, 20, 24, 20);
    lay4->setSpacing(12);

    m_gamedirs_list = new QListWidget(m_page_gamedirs);
    m_gamedirs_list->setObjectName(QStringLiteral("WizardGameDirsList"));
    m_gamedirs_list->setProperty("stormList", true);
    RefreshGameDirsList();
    lay4->addWidget(m_gamedirs_list);

    auto* gd_btn_box = new QWidget(m_page_gamedirs);
    auto* gd_btn_lay = new QHBoxLayout(gd_btn_box);
    gd_btn_lay->setContentsMargins(0, 0, 0, 0);
    gd_btn_lay->setSpacing(10);

    m_add_gamedir_btn = new QPushButton(StormLang(
        QStringLiteral("Добавить папку с играми..."),
        QStringLiteral("Add Game Folder...")), gd_btn_box);
    m_add_gamedir_btn->setCursor(Qt::PointingHandCursor);
    connect(m_add_gamedir_btn, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this,
            StormLang(QStringLiteral("Выберите папку с играми"), QStringLiteral("Select Game Folder")));
        if (!dir.isEmpty()) {
            const std::string std_dir = dir.toStdString();
            bool exists = false;
            for (const auto& existing : UISettings::values.game_dirs) {
                if (existing.path == std_dir) { exists = true; break; }
            }
            if (!exists) {
                UISettings::GameDir gd;
                gd.path = std_dir;
                gd.deep_scan = true;
                UISettings::values.game_dirs.append(gd);
            }
            RefreshGameDirsList();
        }
    });
    gd_btn_lay->addWidget(m_add_gamedir_btn);

    m_remove_gamedir_btn = new QPushButton(StormLang(
        QStringLiteral("Удалить выбранную"),
        QStringLiteral("Remove Selected")), gd_btn_box);
    m_remove_gamedir_btn->setCursor(Qt::PointingHandCursor);
    connect(m_remove_gamedir_btn, &QPushButton::clicked, this, [this]() {
        const int row = m_gamedirs_list->currentRow();
        if (row >= 0 && row < UISettings::values.game_dirs.size()) {
            UISettings::values.game_dirs.removeAt(row);
            RefreshGameDirsList();
        }
    });
    gd_btn_lay->addWidget(m_remove_gamedir_btn);
    lay4->addWidget(gd_btn_box);
    addPage(m_page_gamedirs);
}

void FirstRunWizard::UpdateKeysStatus() {
    if (!m_keys_status_label) return;
    const bool present = ContentManager::AreKeysPresent();
    if (present) {
        m_keys_status_label->setText(StormLang(
            QStringLiteral("<b>Статус:</b> <span style='color: #10B981;'>Ключи дешифрования успешно обнаружены и готовы к работе!</span>"),
            QStringLiteral("<b>Status:</b> <span style='color: #10B981;'>Decryption keys are present and ready!</span>")));
    } else {
        m_keys_status_label->setText(StormLang(
            QStringLiteral("<b>Статус:</b> <span style='color: #F59E0B;'>Файл prod.keys пока не найден. Нажмите кнопку ниже, чтобы указать его.</span>"),
            QStringLiteral("<b>Status:</b> <span style='color: #F59E0B;'>Keys not found yet. Click the button below to browse.</span>")));
    }
}

void FirstRunWizard::UpdateFirmwareStatus() {
    if (!m_firmware_status_label) return;
    const bool present = FirmwareManager::CheckFirmwarePresence(system);
    if (present) {
        const auto [ver, res] = FirmwareManager::GetFirmwareVersion(system);
        const QString ver_str = QString::fromUtf8(ver.display_version.data());
        m_firmware_status_label->setText(StormLang(
            QStringLiteral("<b>Статус:</b> <span style='color: #10B981;'>Прошивка установлена: %1</span>").arg(ver_str.isEmpty() ? QStringLiteral("OK") : ver_str),
            QStringLiteral("<b>Status:</b> <span style='color: #10B981;'>Firmware installed: %1</span>").arg(ver_str.isEmpty() ? QStringLiteral("OK") : ver_str)));
    } else {
        m_firmware_status_label->setText(StormLang(
            QStringLiteral("<b>Статус:</b> <span style='color: #F59E0B;'>Прошивка Switch не установлена.</span>"),
            QStringLiteral("<b>Status:</b> <span style='color: #F59E0B;'>Switch firmware is not installed.</span>")));
    }
}

void FirstRunWizard::RefreshGameDirsList() {
    if (!m_gamedirs_list) return;
    m_gamedirs_list->clear();
    for (const auto& gd : UISettings::values.game_dirs) {
        m_gamedirs_list->addItem(QString::fromStdString(gd.path));
    }
}

void FirstRunWizard::CompleteWizard() {
    UISettings::values.first_start.SetValue(false);
    UISettings::values.first_start = false;
}

void FirstRunWizard::accept() {
    CompleteWizard();
    QWizard::accept();
}

void FirstRunWizard::reject() {
    // Law 24: ESC dismissal should close cleanly without marking setup as complete
    QWizard::reject();
}

void FirstRunWizard::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWizard::dragEnterEvent(event);
    }
}

void FirstRunWizard::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasUrls()) {
        QWizard::dropEvent(event);
        return;
    }
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString local_path = url.toLocalFile();
        if (local_path.isEmpty()) continue;
        const QFileInfo fi(local_path);
        if (fi.isFile()) {
            const QString suf = fi.suffix().toLower();
            if (suf == QStringLiteral("keys") || fi.fileName().contains(QStringLiteral("keys"), Qt::CaseInsensitive)) {
                FirmwareManager::InstallKeys(local_path.toStdString(), "keys");
                UpdateKeysStatus();
            } else if (suf == QStringLiteral("zip")) {
                const QString tmp = QtCommon::Content::UnzipFirmwareToTmp(local_path);
                if (!tmp.isEmpty()) {
                    QtCommon::Content::InstallFirmware(tmp, false);
                } else {
                    QtCommon::Content::InstallKeysFromZip(local_path);
                    UpdateKeysStatus();
                }
                UpdateFirmwareStatus();
            }
        } else if (fi.isDir()) {
            QDir d(local_path);
            if (!d.entryList(QStringList() << QStringLiteral("*.nca"), QDir::Files).isEmpty()) {
                QtCommon::Content::InstallFirmware(local_path, false);
                UpdateFirmwareStatus();
            } else {
                const std::string std_dir = local_path.toStdString();
                bool exists = false;
                for (const auto& existing : UISettings::values.game_dirs) {
                    if (existing.path == std_dir) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    UISettings::GameDir gd;
                    gd.path = std_dir;
                    gd.deep_scan = true;
                    UISettings::values.game_dirs.append(gd);
                }
                RefreshGameDirsList();
            }
        }
    }
    event->acceptProposedAction();
}
