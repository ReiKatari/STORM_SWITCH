// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <QTabBar>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <QScrollArea>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFontMetrics>
#include <QLabel>
#include <QListWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QScrollBar>
#include "common/cpu_features.h"
#include "common/memory_detect.h"
#ifdef _WIN32
#undef LoadString
#endif
#include "common/logging.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "core/core.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
#undef LoadString
#include "ui_configure.h"
#include "storm_switch/configuration/configure_applets.h"
#include "storm_switch/configuration/configure_audio.h"
#include "storm_switch/configuration/configure_cpu.h"
#include "storm_switch/configuration/configure_debug_tab.h"
#include "storm_switch/configuration/configure_dialog.h"
#include "storm_switch/configuration/configure_filesystem.h"
#include "storm_switch/configuration/configure_general.h"
#include "storm_switch/configuration/configure_graphics.h"
#include "storm_switch/configuration/configure_graphics_advanced.h"
#include "storm_switch/configuration/configure_graphics_extensions.h"
#include "storm_switch/configuration/configure_hotkeys.h"
#include "storm_switch/configuration/configure_input.h"
#include "storm_switch/configuration/configure_input_player.h"
#include "storm_switch/configuration/configure_network.h"
#include "storm_switch/configuration/configure_profile_manager.h"
#include "storm_switch/configuration/configure_system.h"
#include "storm_switch/configuration/configure_ui.h"
#include "storm_switch/configuration/configure_web.h"
#include "storm_switch/configuration/shared_widget.h"

#include "storm_switch/common/storm_lang.h"

static QString GetLocalizedTabText(const QString& accessible_name) {
    if (accessible_name == QStringLiteral("General"))
        return StormLang(QStringLiteral("Общие"), QStringLiteral("General"), QStringLiteral("Allgemein"), QStringLiteral("Général"), QStringLiteral("通用"), QStringLiteral("全般"), QStringLiteral("عام"), QStringLiteral("General"));
    if (accessible_name == QStringLiteral("Hotkeys"))
        return StormLang(QStringLiteral("Горячие клавиши"), QStringLiteral("Hotkeys"), QStringLiteral("Tastenkürzel"), QStringLiteral("Raccourcis"), QStringLiteral("快捷键"), QStringLiteral("ショートカット"), QStringLiteral("مفاتيح الاختصار"), QStringLiteral("Atajos de teclado"));
    if (accessible_name == QStringLiteral("UI") || accessible_name == QStringLiteral("Game List"))
        return StormLang(QStringLiteral("Интерфейс"), QStringLiteral("Interface"), QStringLiteral("Benutzeroberfläche"), QStringLiteral("Interface"), QStringLiteral("界面"), QStringLiteral("インターフェース"), QStringLiteral("الواجهة"), QStringLiteral("Interfaz"));
    if (accessible_name == QStringLiteral("Web"))
        return StormLang(QStringLiteral("Веб"), QStringLiteral("Web"), QStringLiteral("Web"), QStringLiteral("Web"), QStringLiteral("网络服务"), QStringLiteral("Web"), QStringLiteral("الويب"), QStringLiteral("Web"));
    if (accessible_name == QStringLiteral("Debug"))
        return StormLang(QStringLiteral("Отладка"), QStringLiteral("Debug"), QStringLiteral("Debug"), QStringLiteral("Débogage"), QStringLiteral("调试"), QStringLiteral("デバッグ"), QStringLiteral("تصحيح الأخطاء"), QStringLiteral("Depuración"));
    if (accessible_name == QStringLiteral("System"))
        return StormLang(QStringLiteral("Система"), QStringLiteral("System"), QStringLiteral("System"), QStringLiteral("Système"), QStringLiteral("系统"), QStringLiteral("システム"), QStringLiteral("النظام"), QStringLiteral("Sistema"));
    if (accessible_name == QStringLiteral("Profiles"))
        return StormLang(QStringLiteral("Профили"), QStringLiteral("Profiles"), QStringLiteral("Profile"), QStringLiteral("Profils"), QStringLiteral("用户配置"), QStringLiteral("プロフィール"), QStringLiteral("الملفات الشخصية"), QStringLiteral("Perfiles"));
    if (accessible_name == QStringLiteral("Filesystem"))
        return StormLang(QStringLiteral("Файловая система"), QStringLiteral("Filesystem"), QStringLiteral("Dateisystem"), QStringLiteral("Système de fichiers"), QStringLiteral("文件系统"), QStringLiteral("ファイルシステム"), QStringLiteral("نظام الملفات"), QStringLiteral("Sistema de archivos"));
    if (accessible_name == QStringLiteral("Applets"))
        return StormLang(QStringLiteral("Апплеты"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("小程序"), QStringLiteral("アプレット"), QStringLiteral("البريمجات"), QStringLiteral("Applets"));
    if (accessible_name == QStringLiteral("CPU"))
        return StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("المعالج"), QStringLiteral("CPU"));
    if (accessible_name == QStringLiteral("Graphics"))
        return StormLang(QStringLiteral("Графика"), QStringLiteral("Graphics"), QStringLiteral("Grafik"), QStringLiteral("Graphismes"), QStringLiteral("图形"), QStringLiteral("グラフィックス"), QStringLiteral("الرسوميات"), QStringLiteral("Gráficos"));
    if (accessible_name == QStringLiteral("Advanced") || accessible_name == QStringLiteral("GraphicsAdvanced"))
        return StormLang(QStringLiteral("Продвинутые"), QStringLiteral("Advanced"), QStringLiteral("Erweitert"), QStringLiteral("Avancé"), QStringLiteral("高级"), QStringLiteral("高度な設定"), QStringLiteral("متقدم"), QStringLiteral("Avanzado"));
    if (accessible_name == QStringLiteral("Extensions") || accessible_name == QStringLiteral("GraphicsExtra"))
        return StormLang(QStringLiteral("Дополнительно"), QStringLiteral("Extensions"), QStringLiteral("Erweiterungen"), QStringLiteral("Extensions"), QStringLiteral("扩展"), QStringLiteral("拡張設定"), QStringLiteral("ملحقات"), QStringLiteral("Extensiones"));
    if (accessible_name == QStringLiteral("Audio"))
        return StormLang(QStringLiteral("Аудио"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("音频"), QStringLiteral("オーディオ"), QStringLiteral("الصوت"), QStringLiteral("Audio"));
    if (accessible_name == QStringLiteral("Network"))
        return StormLang(QStringLiteral("Сеть"), QStringLiteral("Network"), QStringLiteral("Netzwerk"), QStringLiteral("Réseau"), QStringLiteral("网络"), QStringLiteral("ネットワーク"), QStringLiteral("الشبكة"), QStringLiteral("Red"));
    if (accessible_name == QStringLiteral("Controls") || accessible_name == QStringLiteral("Input"))
        return StormLang(QStringLiteral("Управление"), QStringLiteral("Controls"), QStringLiteral("Steuerung"), QStringLiteral("Commandes"), QStringLiteral("控制"), QStringLiteral("操作"), QStringLiteral("التحكم"), QStringLiteral("Controles"));
    if (accessible_name == QStringLiteral("Input Profiles"))
        return StormLang(QStringLiteral("Профили ввода"), QStringLiteral("Input Profiles"), QStringLiteral("Eingabeprofile"), QStringLiteral("Profils d'entrée"), QStringLiteral("输入配置"), QStringLiteral("入力プロファイル"), QStringLiteral("ملفات التحكم"), QStringLiteral("Perfiles de entrada"));
    if (accessible_name == QStringLiteral("Add-Ons"))
        return StormLang(QStringLiteral("Дополнения"), QStringLiteral("Add-Ons"), QStringLiteral("Add-Ons"), QStringLiteral("Extensions"), QStringLiteral("附加组件"), QStringLiteral("アドオン"), QStringLiteral("الإضافات"), QStringLiteral("Complementos"));
    if (accessible_name == QStringLiteral("GameBanana Mods"))
        return StormLang(QStringLiteral("Моды GameBanana"), QStringLiteral("GameBanana Mods"), QStringLiteral("GameBanana-Mods"), QStringLiteral("Mods GameBanana"), QStringLiteral("GameBanana 模组"), QStringLiteral("GameBanana Mod"), QStringLiteral("تعديلات GameBanana"), QStringLiteral("Mods de GameBanana"));
    if (accessible_name == QStringLiteral("Amiibo"))
        return StormLang(QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"), QStringLiteral("Amiibo"));
    if (accessible_name == QStringLiteral("Cheats"))
        return StormLang(QStringLiteral("Читы"), QStringLiteral("Cheats"), QStringLiteral("Cheats"), QStringLiteral("Triche"), QStringLiteral("作弊码"), QStringLiteral("チート"), QStringLiteral("الغش"), QStringLiteral("Trucos"));
    return accessible_name;
}
#include "storm_switch/hotkeys.h"

ConfigureDialog::ConfigureDialog(QWidget* parent, HotkeyRegistry& registry_,
                                 InputCommon::InputSubsystem* input_subsystem,
                                 std::vector<VkDeviceInfo::Record>& vk_device_records,
                                 Core::System& system_, bool enable_web_config)
    : QDialog(parent), ui{std::make_unique<Ui::ConfigureDialog>()}, registry(registry_),
      system{system_},
      builder{std::make_unique<ConfigurationShared::Builder>(this, !system_.IsPoweredOn())},
      applets_tab{std::make_unique<ConfigureApplets>(system_, nullptr, *builder, this)},
      audio_tab{std::make_unique<ConfigureAudio>(system_, nullptr, *builder, this)},
      cpu_tab{std::make_unique<ConfigureCpu>(system_, nullptr, *builder, this)},
      debug_tab_tab{std::make_unique<ConfigureDebugTab>(system_, this)},
      filesystem_tab{std::make_unique<ConfigureFilesystem>(this)},
      general_tab{std::make_unique<ConfigureGeneral>(system_, nullptr, *builder, this)},
      graphics_advanced_tab{
          std::make_unique<ConfigureGraphicsAdvanced>(system_, nullptr, *builder, this)},
      graphics_extensions_tab{
          std::make_unique<ConfigureGraphicsExtensions>(system_, nullptr, *builder, this)},
      ui_tab{std::make_unique<ConfigureUi>(system_, this)},
      graphics_tab{std::make_unique<ConfigureGraphics>(
          system_, vk_device_records, [&]() { graphics_advanced_tab->ExposeComputeOption(); },
          [this](Settings::AspectRatio ratio, Settings::ResolutionSetup setup) {
              ui_tab->UpdateScreenshotInfo(ratio, setup);
          },
          nullptr, *builder, this)},
      hotkeys_tab{std::make_unique<ConfigureHotkeys>(system_.HIDCore(), this)},
      input_tab{std::make_unique<ConfigureInput>(system_, this)},
      network_tab{std::make_unique<ConfigureNetwork>(system_, this)},
      profile_tab{std::make_unique<ConfigureProfileManager>(system_, this)},
      system_tab{std::make_unique<ConfigureSystem>(system_, nullptr, *builder, this)},
      web_tab{std::make_unique<ConfigureWeb>(this)},
      vk_records{vk_device_records} {
    Settings::SetConfiguringGlobal(true);

    ui->setupUi(this);

    ui->tabWidget->addTab(applets_tab.get(), tr("Applets"));
    ui->tabWidget->addTab(audio_tab.get(), tr("Audio"));
    ui->tabWidget->addTab(cpu_tab.get(), tr("CPU"));
    ui->tabWidget->addTab(debug_tab_tab.get(), tr("Debug"));
    ui->tabWidget->addTab(filesystem_tab.get(), tr("Filesystem"));
    ui->tabWidget->addTab(general_tab.get(), tr("General"));
    ui->tabWidget->addTab(graphics_tab.get(), tr("Graphics"));
    ui->tabWidget->addTab(graphics_advanced_tab.get(), tr("GraphicsAdvanced"));
    ui->tabWidget->addTab(graphics_extensions_tab.get(), tr("GraphicsExtra"));
    ui->tabWidget->addTab(hotkeys_tab.get(), tr("Hotkeys"));
    ui->tabWidget->addTab(input_tab.get(), tr("Controls"));
    ui->tabWidget->addTab(profile_tab.get(), tr("Profiles"));
    ui->tabWidget->addTab(network_tab.get(), tr("Network"));
    ui->tabWidget->addTab(system_tab.get(), tr("System"));
    ui->tabWidget->addTab(ui_tab.get(), tr("Game List"));
    ui->tabWidget->addTab(web_tab.get(), tr("Web"));

    web_tab->SetWebServiceConfigEnabled(enable_web_config);
    hotkeys_tab->Populate(registry);

    input_tab->Initialize(input_subsystem);

    general_tab->SetResetCallback([&] { this->close(); });

    SetConfiguration();
    PopulateSelectionList();
    ui->selectorList->setFixedWidth(175);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setObjectName(QStringLiteral("SettingsSearchEdit"));
    m_search_edit->setClearButtonEnabled(true);
    m_search_edit->setPlaceholderText(StormLang(
        QStringLiteral("🔍 Быстрый поиск настройки (например: Vulkan, FSR, звук, память, кадры)..."),
        QStringLiteral("🔍 Quick search settings (e.g. Vulkan, FSR, audio, memory, frame)..."),
        QStringLiteral("🔍 Einstellungen schnell suchen (z. B. Vulkan, FSR, Audio, Speicher, FPS)..."),
        QStringLiteral("🔍 Recherche rapide des paramètres (ex. Vulkan, FSR, audio, mémoire, FPS)..."),
        QStringLiteral("🔍 快速搜索设置（例如：Vulkan、FSR、音频、内存、帧率）..."),
        QStringLiteral("🔍 設定をクイック検索（例：Vulkan、FSR、オーディオ、メモリ、フレーム）..."),
        QStringLiteral("🔍 بحث سريع في الإعدادات (مثل: Vulkan، FSR، الصوت، الذاكرة، الإطارات)..."),
        QStringLiteral("🔍 Búsqueda rápida de ajustes (ej.: Vulkan, FSR, audio, memoria, FPS)...")
    ));
    m_search_edit->setStyleSheet(QStringLiteral(
        "QLineEdit#SettingsSearchEdit {"
        "    background-color: #0F172A;"
        "    border: 1px solid rgba(0, 210, 255, 0.35);"
        "    border-radius: 7px;"
        "    padding: 6px 12px;"
        "    color: #FFFFFF;"
        "    font-size: 12.5px;"
        "    margin-bottom: 4px;"
        "}"
        "QLineEdit#SettingsSearchEdit:focus {"
        "    border: 1px solid #00F0FF;"
        "    background-color: #162032;"
        "}"
    ));
    ui->verticalLayout->insertWidget(0, m_search_edit);

    m_search_results_list = new QListWidget(this);
    m_search_results_list->setObjectName(QStringLiteral("SettingsSearchResultsList"));
    m_search_results_list->setVisible(false);
    m_search_results_list->setMaximumHeight(210);
    m_search_results_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_search_results_list->setCursor(Qt::PointingHandCursor);
    m_search_results_list->setStyleSheet(QStringLiteral(
        "QListWidget#SettingsSearchResultsList {"
        "    background-color: #0B1325;"
        "    border: 1px solid #00F0FF;"
        "    border-radius: 7px;"
        "    padding: 4px;"
        "    color: #FFFFFF;"
        "    font-size: 12px;"
        "    margin-bottom: 6px;"
        "    outline: none;"
        "}"
        "QListWidget#SettingsSearchResultsList::item {"
        "    padding: 6px 10px;"
        "    border-radius: 5px;"
        "    margin: 1px 0px;"
        "}"
        "QListWidget#SettingsSearchResultsList::item:hover {"
        "    background-color: rgba(0, 240, 255, 0.16);"
        "    color: #FFFFFF;"
        "}"
        "QListWidget#SettingsSearchResultsList::item:selected {"
        "    background-color: rgba(0, 240, 255, 0.26);"
        "    border-left: 3px solid #00F0FF;"
        "    color: #FFFFFF;"
        "    font-weight: 600;"
        "}"
    ));
    ui->verticalLayout->insertWidget(1, m_search_results_list);
    m_search_edit->installEventFilter(this);

    connect(m_search_edit, &QLineEdit::textChanged, this, &ConfigureDialog::OnSearchTextChanged);
    connect(m_search_results_list, &QListWidget::itemClicked, this, &ConfigureDialog::ActivateSearchResult);
    connect(m_search_results_list, &QListWidget::itemActivated, this, &ConfigureDialog::ActivateSearchResult);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index != -1) {
            debug_tab_tab->SetCurrentIndex(0);
        }
    });
    connect(ui_tab.get(), &ConfigureUi::LanguageChanged, this, &ConfigureDialog::OnLanguageChanged);
    connect(ui_tab.get(), &ConfigureUi::ThemeChanged, this, &ConfigureDialog::OnThemeChanged);
    connect(general_tab.get(), &ConfigureGeneral::ExternalContentDirsChanged, this,
            &ConfigureDialog::ExternalContentDirsChanged);
    connect(ui->selectorList, &QListWidget::itemSelectionChanged, this,
            &ConfigureDialog::UpdateVisibleTabs);

    if (system.IsPoweredOn()) {
        QPushButton* apply_button = ui->buttonBox->addButton(QDialogButtonBox::Apply);
        connect(apply_button, &QAbstractButton::clicked, this,
                &ConfigureDialog::HandleApplyButtonClicked);
    }

    m_restore_defaults_btn = ui->buttonBox->addButton(StormLang(
        QStringLiteral("↺ Сбросить настройки"),
        QStringLiteral("↺ Restore Defaults"),
        QStringLiteral("↺ Standardeinstellungen"),
        QStringLiteral("↺ Paramètres par défaut"),
        QStringLiteral("↺ 恢复默认设置"),
        QStringLiteral("↺ デフォルトに戻す")
    ), QDialogButtonBox::ActionRole);
    m_restore_defaults_btn->setObjectName(QStringLiteral("RestoreDefaultsButton"));
    m_restore_defaults_btn->setCursor(Qt::PointingHandCursor);
    m_restore_defaults_btn->setStyleSheet(QStringLiteral(
        "QPushButton#RestoreDefaultsButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1E293B, stop:1 #334155);"
        "    color: #FFFFFF;"
        "    font-weight: 600;"
        "    font-size: 12px;"
        "    padding: 6px 16px;"
        "    border-radius: 6px;"
        "    border: 1px solid #64748B;"
        "}"
        "QPushButton#RestoreDefaultsButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #EF4444, stop:1 #DC2626);"
        "    color: #FFFFFF;"
        "    border: 1px solid #F87171;"
        "}"
        "QPushButton#RestoreDefaultsButton:pressed {"
        "    background: #B91C1C;"
        "}"
    ));
    auto* shadow = new QGraphicsDropShadowEffect(m_restore_defaults_btn);
    shadow->setBlurRadius(8);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 160));
    m_restore_defaults_btn->setGraphicsEffect(shadow);
    connect(m_restore_defaults_btn, &QPushButton::clicked, this, &ConfigureDialog::OnRestoreDefaultsClicked);

    if (auto* cancel_btn = ui->buttonBox->button(QDialogButtonBox::Cancel)) {
        cancel_btn->setProperty("isCancelButton", true);
        cancel_btn->setText(StormCancelText());
        cancel_btn->setStyleSheet(StormCancelButtonStyle());
        cancel_btn->setCursor(Qt::PointingHandCursor);
    }

    setStyleSheet(QStringLiteral(
        "QDialog#ConfigureDialog {"
        "    background-color: #0B0F19;"
        "}"
        "QListWidget#selectorList {"
        "    background-color: #0F172A;"
        "    border: 1px solid #1E293B;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "    outline: none;"
        "}"
        "QListWidget#selectorList::item {"
        "    color: #FFFFFF;"
        "    background: transparent;"
        "    border-radius: 6px;"
        "    padding: 10px 14px;"
        "    font-size: 13px;"
        "    font-weight: 600;"
        "    margin-bottom: 4px;"
        "}"
        "QListWidget#selectorList::item:hover {"
        "    color: #FFFFFF;"
        "    background: rgba(0, 210, 255, 0.12);"
        "    border: 1px solid rgba(0, 210, 255, 0.35);"
        "}"
        "QListWidget#selectorList::item:selected {"
        "    color: #00F0FF;"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 210, 255, 0.22), stop:1 rgba(0, 150, 255, 0.1));"
        "    border: 1px solid #00D2FF;"
        "}"
        "QTabWidget::pane {"
        "    border: 1px solid #1E293B;"
        "    border-radius: 8px;"
        "    background-color: #0F172A;"
        "    top: -1px;"
        "}"
        "QTabBar::tab {"
        "    background: #0B0F19;"
        "    color: #FFFFFF;"
        "    border: 1px solid #1E293B;"
        "    border-bottom: none;"
        "    border-top-left-radius: 6px;"
        "    border-top-right-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-weight: 600;"
        "    margin-right: 2px;"
        "}"
        "QTabBar::tab:hover {"
        "    color: #FFFFFF;"
        "    background: #1E293B;"
        "}"
        "QTabBar::tab:selected {"
        "    color: #00F0FF;"
        "    background: #0F172A;"
        "    border-top: 2px solid #00D2FF;"
        "    border-bottom: 1px solid #0F172A;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    color: #00F0FF;"
        "    border: 1px solid rgba(0, 210, 255, 0.25);"
        "    border-radius: 8px;"
        "    margin-top: 9px;"
        "    padding: 16px 12px 12px 12px;"
        "    background: rgba(15, 23, 42, 0.5);"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    top: 0px;"
        "    left: 12px;"
        "    padding: 0 6px;"
        "    color: #00F0FF;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    background-color: #0F172A;"
        "    border-radius: 3px;"
        "}"
        "QLabel, QCheckBox, QRadioButton {"
        "    background: transparent;"
        "    background-color: transparent;"
        "}"
        "QWidget#StormSettingRow, QGroupBox QLabel, QGroupBox QCheckBox, QGroupBox QRadioButton {"
        "    background: transparent;"
        "    background-color: transparent;"
        "}"
        "QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget {"
        "    background: transparent;"
        "    background-color: transparent;"
        "}"
        "QScrollBar:vertical {"
        "    background: #0B0F19;"
        "    width: 10px;"
        "    margin: 2px 2px 2px 0px;"
        "    border-radius: 5px;"
        "    border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(0, 210, 255, 0.45);"
        "    min-height: 28px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #00F0FF;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "    background: none;"
        "    border: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "    background: transparent;"
        "}"
        "QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit, QDateTimeEdit {"
        "    background-color: #121826;"
        "    border: 1.5px solid #334155;"
        "    border-radius: 6px;"
        "    padding: 4px 10px;"
        "    min-height: 22px;"
        "    color: #FFFFFF;"
        "    font-size: 12px;"
        "    selection-background-color: #0284C7;"
        "    selection-color: #FFFFFF;"
        "}"
        "QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QLineEdit:hover, QDateTimeEdit:hover {"
        "    border: 1.5px solid #00D2FF;"
        "    background-color: #162032;"
        "}"
        "QComboBox:focus, QComboBox:on, QSpinBox:focus, QDoubleSpinBox:focus, QLineEdit:focus, QDateTimeEdit:focus {"
        "    border: 1.5px solid #00F0FF;"
        "    background-color: #182438;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: border;"
        "    subcontrol-position: top right;"
        "    width: 24px;"
        "    margin: 1px 1px 1px 0px;"
        "    border-left: 1px solid #334155;"
        "    border-top-right-radius: 5px;"
        "    border-bottom-right-radius: 5px;"
        "    background-color: rgba(30, 41, 59, 0.55);"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #0F172A;"
        "    border: 1px solid #334155;"
        "    border-radius: 6px;"
        "    selection-background-color: rgba(0, 210, 255, 0.25);"
        "    selection-color: #00F0FF;"
        "    color: #FFFFFF;"
        "    outline: none;"
        "    padding: 4px;"
        "}"
        "QSlider::groove:horizontal {"
        "    border: 1px solid #334155;"
        "    height: 6px;"
        "    background: #121826;"
        "    border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0099FF, stop:1 #00F0FF);"
        "    border-radius: 3px;"
        "}"
        "QSlider::thumb:horizontal {"
        "    background: #00F0FF;"
        "    border: 2px solid #FFFFFF;"
        "    width: 16px;"
        "    margin-top: -6px;"
        "    margin-bottom: -6px;"
        "    border-radius: 8px;"
        "}"
        "QSlider::thumb:horizontal:hover {"
        "    background: #FFFFFF;"
        "    border: 2px solid #00F0FF;"
        "}"
        "QCheckBox, QRadioButton {"
        "    color: #FFFFFF;"
        "    spacing: 8px;"
        "    font-size: 12px;"
        "    background: transparent;"
        "    background-color: transparent;"
        "}"
        "QCheckBox::indicator, QGroupBox::indicator, QRadioButton::indicator, QTreeView::indicator, QListView::indicator, QTableView::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid #475569;"
        "    border-radius: 4px;"
        "    background: #121826;"
        "    image: none;"
        "}"
        "QCheckBox::indicator:hover, QGroupBox::indicator:hover, QRadioButton::indicator:hover, QTreeView::indicator:hover, QListView::indicator:hover, QTableView::indicator:hover {"
        "    border: 1px solid #00D2FF;"
        "}"
        "QCheckBox::indicator:checked, QGroupBox::indicator:checked, QRadioButton::indicator:checked, QTreeView::indicator:checked, QListView::indicator:checked, QTableView::indicator:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00F0FF, stop:1 #0088CC);"
        "    border: 1px solid #00F0FF;"
        "    image: none;"
        "}"
        "QCheckBox::indicator:unchecked, QGroupBox::indicator:unchecked, QRadioButton::indicator:unchecked, QTreeView::indicator:unchecked, QListView::indicator:unchecked, QTableView::indicator:unchecked {"
        "    background: #121826;"
        "    border: 1px solid #475569;"
        "    image: none;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1E293B, stop:1 #334155);"
        "    color: #FFFFFF;"
        "    font-weight: 600;"
        "    font-size: 12px;"
        "    padding: 6px 16px;"
        "    border-radius: 6px;"
        "    border: 1px solid #475569;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0099CC, stop:1 #006699);"
        "    color: #FFFFFF;"
        "    border: 1px solid #00D2FF;"
        "}"
        "QPushButton:pressed {"
        "    background: #005580;"
        "}"
    ) + StormCancelButtonQssRule());

    ui->tabWidget->setMinimumSize(320, 200);
    ui->tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    for (auto* sa : findChildren<QScrollArea*>()) {
        sa->setWidgetResizable(true);
        sa->setFrameShape(QFrame::NoFrame);
        sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        sa->setMinimumSize(200, 160);
    }

    ui->selectorList->setCurrentRow(0);

    // Selects the leftmost button on the bottom bar (Cancel as of writing)
    ui->buttonBox->setFocus();

    ConfigurationShared::RegisterReloadCallback(reinterpret_cast<uintptr_t>(this), [this]() {
        if (graphics_tab) {
            graphics_tab->SetConfiguration();
        }
    });

    RetranslateUI();

    setSizeGripEnabled(true);
    QScreen* scr = (parentWidget() && parentWidget()->screen()) ? parentWidget()->screen()
                   : (screen() ? screen() : QGuiApplication::primaryScreen());
    if (scr) {
        const QRect avail = scr->availableGeometry();
        const int max_w = std::max(640, avail.width() - 48);
        const int max_h = std::max(420, avail.height() - 56);
        setMinimumSize(std::min(720, max_w), std::min(440, max_h));
        setMaximumSize(max_w, max_h);

        int target_w = std::min(1340, max_w);
        int target_h = std::min(610, max_h);
        if (QWidget* main_win = parentWidget() ? parentWidget()->window() : nullptr) {
            if (main_win->isVisible() && !main_win->isMinimized()) {
                target_w = std::clamp(main_win->width() - 36, std::min(1240, max_w), max_w);
                if (avail.height() <= 920) {
                    target_h = std::clamp(std::min(main_win->height() - 56, avail.height() - 88),
                                          std::min(460, max_h), std::min(610, max_h));
                } else {
                    target_h = std::clamp(main_win->height() - 80, std::min(540, max_h), std::min(620, max_h));
                }
            }
        } else if (avail.height() <= 920) {
            target_h = std::clamp(avail.height() - 88, std::min(460, max_h), std::min(600, max_h));
        }
        resize(target_w, target_h);
    } else {
        resize(1280, 610);
    }
}

ConfigureDialog::~ConfigureDialog() {
    ConfigurationShared::UnregisterReloadCallback(reinterpret_cast<uintptr_t>(this));
}

void ConfigureDialog::ReloadAllTabs() {
    static bool s_in_reload = false;
    if (s_in_reload) {
        return;
    }
    s_in_reload = true;
    ConfigurationShared::ReloadAllActiveWidgets();
    if (general_tab) {
        general_tab->SetConfiguration();
    }
    if (system_tab) {
        system_tab->SetConfiguration();
    }
    if (applets_tab) {
        applets_tab->SetConfiguration();
    }
    if (cpu_tab) {
        cpu_tab->SetConfiguration();
    }
    if (graphics_tab) {
        graphics_tab->SetConfiguration();
    }
    if (graphics_advanced_tab) {
        graphics_advanced_tab->SetConfiguration();
    }
    if (graphics_extensions_tab) {
        graphics_extensions_tab->SetConfiguration();
    }
    if (audio_tab) {
        audio_tab->SetConfiguration();
    }
    s_in_reload = false;
}

void ConfigureDialog::SetConfiguration() {
    ReloadAllTabs();
}

void ConfigureDialog::ApplyConfiguration() {
    general_tab->ApplyConfiguration();
    system_tab->ApplyConfiguration();
    profile_tab->ApplyConfiguration();
    filesystem_tab->ApplyConfiguration();
    applets_tab->ApplyConfiguration();
    ui_tab->ApplyConfiguration();
    cpu_tab->ApplyConfiguration();
    graphics_tab->ApplyConfiguration();
    graphics_advanced_tab->ApplyConfiguration();
    graphics_extensions_tab->ApplyConfiguration();
    audio_tab->ApplyConfiguration();
    input_tab->ApplyConfiguration();
    hotkeys_tab->ApplyConfiguration(registry);
    network_tab->ApplyConfiguration();
    debug_tab_tab->ApplyConfiguration();
    web_tab->ApplyConfiguration();
    system.ApplySettings();
    Settings::LogSettings();
}

void ConfigureDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureDialog::RetranslateUI() {
    const int old_row = ui->selectorList->currentRow();
    const int old_index = ui->tabWidget->currentIndex();

    ui->retranslateUi(this);

    if (m_search_edit) {
        m_search_edit->setPlaceholderText(StormLang(
            QStringLiteral("🔍 Быстрый поиск настройки (например: Vulkan, FSR, звук, память, кадры)..."),
            QStringLiteral("🔍 Quick search settings (e.g. Vulkan, FSR, audio, memory, frame)..."),
            QStringLiteral("🔍 Einstellungen schnell suchen (z. B. Vulkan, FSR, Audio, Speicher, FPS)..."),
            QStringLiteral("🔍 Recherche rapide des paramètres (ex. Vulkan, FSR, audio, mémoire, FPS)..."),
            QStringLiteral("🔍 快速搜索设置（例如：Vulkan、FSR、音频、内存、帧率）..."),
            QStringLiteral("🔍 設定をクイック検索（例：Vulkan、FSR、オーディオ、メモリ、フレーム）..."),
            QStringLiteral("🔍 بحث سريع في الإعدادات (مثل: Vulkan، FSR، الصوت، الذاكرة، الإطارات)..."),
            QStringLiteral("🔍 Búsqueda rápida de ajustes (ej.: Vulkan, FSR, audio, memoria, FPS)...")
        ));
    }

    if (m_restore_defaults_btn) {
        m_restore_defaults_btn->setText(StormLang(
            QStringLiteral("↺ Сбросить настройки"),
            QStringLiteral("↺ Restore Defaults"),
            QStringLiteral("↺ Standardeinstellungen"),
            QStringLiteral("↺ Paramètres par défaut"),
            QStringLiteral("↺ 恢复默认设置"),
            QStringLiteral("↺ デフォルトに戻す")
        ));
        m_restore_defaults_btn->setToolTip(StormLang(
            QStringLiteral("Сбросить все параметры эмуляции на стандартные значения"),
            QStringLiteral("Restore all emulation settings to default values"),
            QStringLiteral("Alle Emulationseinstellungen auf Standardwerte zurücksetzen"),
            QStringLiteral("Restaurer tous les paramètres d'émulation aux valeurs par défaut"),
            QStringLiteral("将所有模拟设置恢复为默认值"),
            QStringLiteral("すべてのエミュレーション設定をデフォルトに戻す")
        ));
    }

    if (ui->label) {
        ui->label->setText(StormLang(
            QStringLiteral("Некоторые параметры доступны только когда игра не запущена."),
            QStringLiteral("Some settings are only available when a game is not running."),
            QStringLiteral("Einige Einstellungen sind nur verfügbar, wenn kein Spiel ausgeführt wird."),
            QStringLiteral("Certains paramètres ne sont disponibles que lorsqu'aucun jeu n'est lancé."),
            QStringLiteral("某些设置仅在游戏未运行时可用。"),
            QStringLiteral("一部の設定はゲームが実行されていない場合にのみ利用可能です。")
        ));
    }

    if (builder) {
        builder->ReloadTranslations();
    }

    ConfigurationShared::RetranslateAllActiveWidgets();

    for (auto* tab : tab_group) {
        if (tab) {
            QEvent event(QEvent::LanguageChange);
            QCoreApplication::sendEvent(tab, &event);
        }
    }
    if (ui_tab) {
        ui_tab->RetranslateUI();
    }
    if (general_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(general_tab.get(), &event);
    }
    if (system_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(system_tab.get(), &event);
    }
    if (applets_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(applets_tab.get(), &event);
    }
    if (filesystem_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(filesystem_tab.get(), &event);
    }
    if (graphics_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(graphics_tab.get(), &event);
    }
    if (graphics_advanced_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(graphics_advanced_tab.get(), &event);
    }
    if (graphics_extensions_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(graphics_extensions_tab.get(), &event);
    }
    if (audio_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(audio_tab.get(), &event);
    }
    if (cpu_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(cpu_tab.get(), &event);
    }
    if (debug_tab_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(debug_tab_tab.get(), &event);
    }
    if (web_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(web_tab.get(), &event);
    }
    if (profile_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(profile_tab.get(), &event);
    }
    if (network_tab) {
        QEvent event(QEvent::LanguageChange);
        QCoreApplication::sendEvent(network_tab.get(), &event);
    }

    PopulateSelectionList();
    ui->selectorList->setCurrentRow(old_row);

    UpdateVisibleTabs();
    ui->tabWidget->setCurrentIndex(old_index);

    for (auto* gb : findChildren<QGroupBox*>()) {
        if (!gb->title().isEmpty()) {
            QFont f = gb->font();
            f.setBold(true);
            const QFontMetrics fm(f);
            const int extra = gb->isCheckable() ? 58 : 38;
            const int needed = fm.horizontalAdvance(gb->title()) + extra;
            if (gb->minimumWidth() < needed) {
                gb->setMinimumWidth(needed);
            }
        }
    }
}

void ConfigureDialog::HandleApplyButtonClicked() {
    UISettings::values.configuration_applied = true;
    ApplyConfiguration();
    emit ConfigurationApplied();
}

Q_DECLARE_METATYPE(QList<QWidget*>);

void ConfigureDialog::PopulateSelectionList() {
    const std::array<std::pair<QString, QList<QWidget*>>, 7> items{
        {{StormLang(QStringLiteral("Общие"), QStringLiteral("General"), QStringLiteral("Allgemein"), QStringLiteral("Général"), QStringLiteral("通用"), QStringLiteral("全般")),
          {general_tab.get(), hotkeys_tab.get(), ui_tab.get(), web_tab.get(), debug_tab_tab.get()}},
         {StormLang(QStringLiteral("Система"), QStringLiteral("System"), QStringLiteral("System"), QStringLiteral("Système"), QStringLiteral("系统"), QStringLiteral("システム")),
          {system_tab.get(), profile_tab.get(), filesystem_tab.get(),
           applets_tab.get()}},
         {StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU")), {cpu_tab.get()}},
         {StormLang(QStringLiteral("Графика"), QStringLiteral("Graphics"), QStringLiteral("Grafik"), QStringLiteral("Graphismes"), QStringLiteral("图形"), QStringLiteral("グラフィックス")),
          {graphics_tab.get(), graphics_advanced_tab.get(), graphics_extensions_tab.get()}},
         {StormLang(QStringLiteral("Аудио"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("音频"), QStringLiteral("オーディオ")), {audio_tab.get()}},
         {StormLang(QStringLiteral("Сеть"), QStringLiteral("Network"), QStringLiteral("Netzwerk"), QStringLiteral("Réseau"), QStringLiteral("网络"), QStringLiteral("ネットワーク")), {network_tab.get()}},
         {StormLang(QStringLiteral("Управление"), QStringLiteral("Controls"), QStringLiteral("Steuerung"), QStringLiteral("Commandes"), QStringLiteral("控制"), QStringLiteral("操作")), input_tab->GetSubTabs()}},
    };

    QFont tab_font = ui->tabWidget->tabBar()->font();
    tab_font.setBold(true);
    ui->tabWidget->tabBar()->setFont(tab_font);

    QFont list_font = ui->selectorList->font();
    list_font.setBold(true);
    ui->selectorList->setFont(list_font);

    [[maybe_unused]] const QSignalBlocker blocker(ui->selectorList);
    ui->selectorList->clear();
    for (const auto& entry : items) {
        auto* const item = new QListWidgetItem(entry.first);
        item->setFont(list_font);
        item->setData(Qt::UserRole, QVariant::fromValue(entry.second));

        ui->selectorList->addItem(item);
    }
}

void ConfigureDialog::OnLanguageChanged(const QString& locale) {
    if (m_is_changing_language) {
        return;
    }
    m_is_changing_language = true;
    emit LanguageChanged(locale);
    //  Reloading the game list is needed to force retranslation.
    UISettings::values.is_game_list_reload_pending = true;
    RetranslateUI();
    m_is_changing_language = false;
}

void ConfigureDialog::OnThemeChanged(const QString& theme) {
    emit ThemeChanged(theme);
}

void ConfigureDialog::UpdateVisibleTabs() {
    const auto items = ui->selectorList->selectedItems();
    if (items.isEmpty()) {
        return;
    }

    const QSize saved_size = size();
    [[maybe_unused]] const QSignalBlocker blocker(ui->tabWidget);

    const auto tabs = qvariant_cast<QList<QWidget*>>(items[0]->data(Qt::UserRole));

    // Check if current tabs match the requested list to avoid destructive clear() on active widgets
    bool tabs_match = (ui->tabWidget->count() == tabs.size());
    if (tabs_match) {
        for (int i = 0; i < tabs.size(); ++i) {
            if (ui->tabWidget->widget(i) != tabs[i]) {
                tabs_match = false;
                break;
            }
        }
    }

    if (tabs_match) {
        for (int i = 0; i < tabs.size(); ++i) {
            ui->tabWidget->setTabText(i, GetLocalizedTabText(tabs[i]->accessibleName()));
        }
        return;
    }

    ui->tabWidget->clear();

    for (auto* const tab : tabs) {
        LOG_DEBUG(Frontend, "{}", tab->accessibleName().toStdString());
        tab->setMinimumSize(200, 160);
        ui->tabWidget->addTab(tab, GetLocalizedTabText(tab->accessibleName()));
    }

    if (isVisible() && saved_size.isValid()) {
        resize(saved_size);
    }
}

void ConfigureDialog::OnRestoreDefaultsClicked() {
    QMessageBox ask_box(this);
    ask_box.setWindowTitle(StormLang(
        QStringLiteral("Сброс настроек"),
        QStringLiteral("Reset Settings"),
        QStringLiteral("Einstellungen zurücksetzen"),
        QStringLiteral("Réinitialiser les paramètres"),
        QStringLiteral("重置设置"),
        QStringLiteral("設定のリセット")
    ));
    ask_box.setText(StormLang(
        QStringLiteral("<b>Вы уверены, что хотите сбросить все параметры эмуляции на стандартные значения?</b><br><br>Параметры графики, процессора, звука и системы будут возвращены к исходным настройкам по умолчанию."),
        QStringLiteral("<b>Are you sure you want to restore all emulation settings to default values?</b><br><br>Graphics, CPU, audio, and system settings will be restored to clean factory defaults."),
        QStringLiteral("<b>Möchten Sie wirklich alle Emulationseinstellungen auf die Standardwerte zurücksetzen?</b><br><br>Grafik-, CPU-, Audio- und Systemeinstellungen werden auf die Standardwerte zurückgesetzt."),
        QStringLiteral("<b>Êtes-vous sûr de vouloir restaurer tous les paramètres d'émulation aux valeurs par défaut ?</b><br><br>Les paramètres graphiques, processeur, audio et système seront réinitialisés."),
        QStringLiteral("<b>您确定要将所有模拟设置恢复为默认值吗？</b><br><br>图形、CPU、音频和系统设置将恢复为出厂默认值。"),
        QStringLiteral("<b>すべてのエミュレーション設定をデフォルト値に戻してもよろしいですか？</b><br><br>グラフィックス、CPU、オーディオ、システム設定が初期設定に戻ります。")
    ));
    ask_box.setIcon(QMessageBox::Question);
    auto* yes_btn = ask_box.addButton(StormLang(
        QStringLiteral("Сбросить"),
        QStringLiteral("Reset"),
        QStringLiteral("Zurücksetzen"),
        QStringLiteral("Réinitialiser"),
        QStringLiteral("重置"),
        QStringLiteral("リセット")
    ), QMessageBox::YesRole);
    ask_box.addButton(StormLang(
        QStringLiteral("Отмена"),
        QStringLiteral("Cancel"),
        QStringLiteral("Abbrechen"),
        QStringLiteral("Annuler"),
        QStringLiteral("取消"),
        QStringLiteral("キャンセル")
    ), QMessageBox::NoRole);
    ask_box.setDefaultButton(yes_btn);
    ask_box.exec();

    if (ask_box.clickedButton() != yes_btn) {
        return;
    }

    static const std::vector<Settings::Category> emulation_categories = {
        Settings::Category::Audio,
        Settings::Category::Core,
        Settings::Category::Cpu,
        Settings::Category::CpuDebug,
        Settings::Category::CpuUnsafe,
        Settings::Category::Overlay,
        Settings::Category::Renderer,
        Settings::Category::RendererAdvanced,
        Settings::Category::RendererHacks,
        Settings::Category::RendererExtensions,
        Settings::Category::RendererDebug,
        Settings::Category::System,
        Settings::Category::SystemAudio,
        Settings::Category::Network,
        Settings::Category::Debugging,
        Settings::Category::DebuggingGraphics,
        Settings::Category::Services,
    };

#undef LoadString
    for (auto cat : emulation_categories) {
        auto it = Settings::values.linkage.by_category.find(cat);
        if (it != Settings::values.linkage.by_category.end()) {
            for (const auto& setting : it->second) {
                (setting->LoadString)(setting->DefaultToString());
                if (setting->Switchable()) {
                    setting->SetGlobal(true);
                }
            }
        }
    }

    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();

    ReloadAllTabs();

    QMessageBox info_box(this);
    info_box.setWindowTitle(StormLang(
        QStringLiteral("Настройки сброшены"),
        QStringLiteral("Settings Reset"),
        QStringLiteral("Einstellungen zurückgesetzt"),
        QStringLiteral("Paramètres réinitialisés"),
        QStringLiteral("设置已重置"),
        QStringLiteral("設定がリセットされました")
    ));
    info_box.setText(StormLang(
        QStringLiteral("Все параметры эмуляции успешно возвращены к значениям по умолчанию."),
        QStringLiteral("All emulation settings have been successfully restored to defaults."),
        QStringLiteral("Alle Emulationseinstellungen wurden erfolgreich auf die Standardwerte zurückgesetzt."),
        QStringLiteral("Tous les paramètres d'émulation ont été restaurés avec succès aux valeurs par défaut."),
        QStringLiteral("所有模拟设置已成功恢复为默认值。"),
        QStringLiteral("すべてのエミュレーション設定が正常にデフォルトに戻されました。")
    ));
    info_box.setIcon(QMessageBox::Information);
    info_box.exec();
}

bool ConfigureDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_search_edit && event->type() == QEvent::KeyPress) {
        auto* key_event = static_cast<QKeyEvent*>(event);
        if (m_search_results_list && m_search_results_list->isVisible() &&
            m_search_results_list->count() > 0) {
            if (key_event->key() == Qt::Key_Down) {
                int next_row = std::min(m_search_results_list->currentRow() + 1,
                                        m_search_results_list->count() - 1);
                m_search_results_list->setCurrentRow(next_row);
                ActivateSearchResult(m_search_results_list->item(next_row));
                return true;
            }
            if (key_event->key() == Qt::Key_Up) {
                int prev_row = std::max(m_search_results_list->currentRow() - 1, 0);
                m_search_results_list->setCurrentRow(prev_row);
                ActivateSearchResult(m_search_results_list->item(prev_row));
                return true;
            }
            if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
                auto* cur_item = m_search_results_list->currentItem();
                if (!cur_item && m_search_results_list->count() > 0) {
                    cur_item = m_search_results_list->item(0);
                }
                if (cur_item) {
                    ActivateSearchResult(cur_item);
                    m_search_results_list->setVisible(false);
                }
                return true;
            }
            if (key_event->key() == Qt::Key_Escape) {
                m_search_edit->clear();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void ConfigureDialog::ClearSearchHighlight() {
    if (m_highlighted_widget) {
        m_highlighted_widget->setStyleSheet(m_highlighted_prev_style);
        m_highlighted_widget = nullptr;
        m_highlighted_prev_style.clear();
    }
}

void ConfigureDialog::ActivateSearchResult(QListWidgetItem* item) {
    if (!item) {
        return;
    }
    const int cat_row = item->data(Qt::UserRole).toInt();
    const int tab_idx = item->data(Qt::UserRole + 1).toInt();
    auto* target_widget = reinterpret_cast<QWidget*>(item->data(Qt::UserRole + 2).value<quintptr>());

    if (cat_row >= 0 && cat_row < ui->selectorList->count()) {
        if (ui->selectorList->currentRow() != cat_row) {
            ui->selectorList->setCurrentRow(cat_row);
            UpdateVisibleTabs();
        }
        if (tab_idx >= 0 && tab_idx < ui->tabWidget->count()) {
            ui->tabWidget->setCurrentIndex(tab_idx);
        }
    }

    ClearSearchHighlight();
    if (target_widget) {
        target_widget->setVisible(true);
        m_highlighted_widget = target_widget;
        m_highlighted_prev_style = target_widget->styleSheet();
        target_widget->setStyleSheet(
            m_highlighted_prev_style +
            QStringLiteral(" QWidget { border: 1px solid #00F0FF; background-color: rgba(0, 240, 255, 0.12); border-radius: 6px; }"));

        QWidget* p = target_widget->parentWidget();
        while (p && p != this) {
            if (auto* scroll = qobject_cast<QScrollArea*>(p)) {
                QTimer::singleShot(0, this, [scroll, target_widget]() {
                    scroll->ensureWidgetVisible(target_widget, 24, 48);
                });
                break;
            }
            p = p->parentWidget();
        }
    }
}

void ConfigureDialog::OnSearchTextChanged(const QString& text) {
    const QString query = text.trimmed();
    const bool has_query = query.length() >= 2;
    const QStringList tokens = query.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    ClearSearchHighlight();

    auto matches_all_tokens = [&](const QString& haystack) -> bool {
        if (!has_query) {
            return true;
        }
        for (const QString& tok : tokens) {
            if (!haystack.contains(tok, Qt::CaseInsensitive)) {
                return false;
            }
        }
        return true;
    };

    auto extract_widget_summary = [](QWidget* w, QString& out_title, QString& out_value, QString& out_search_blob) {
        QStringList titles;
        QStringList values;
        QStringList blobs;

        if (!w->toolTip().isEmpty()) {
            blobs << w->toolTip();
        }
        if (auto* gb = qobject_cast<QGroupBox*>(w)) {
            if (!gb->title().isEmpty()) {
                titles << gb->title();
                blobs << gb->title();
            }
        }
        for (const auto* lbl : w->findChildren<QLabel*>()) {
            const QString t = lbl->text().trimmed();
            if (!t.isEmpty() && t.length() < 120) {
                titles << t;
                blobs << t;
            }
            if (!lbl->toolTip().isEmpty()) {
                blobs << lbl->toolTip();
            }
        }
        for (const auto* cb : w->findChildren<QCheckBox*>()) {
            const QString t = cb->text().trimmed();
            if (!t.isEmpty()) {
                titles << t;
                blobs << t;
            }
            values << (cb->isChecked() ? QStringLiteral("✓") : QStringLiteral("—"));
            if (!cb->toolTip().isEmpty()) {
                blobs << cb->toolTip();
            }
        }
        for (const auto* combo : w->findChildren<QComboBox*>()) {
            if (!combo->currentText().isEmpty()) {
                values << combo->currentText().trimmed();
            }
            for (int i = 0; i < combo->count(); ++i) {
                blobs << combo->itemText(i);
            }
        }
        titles.removeDuplicates();
        values.removeDuplicates();
        out_title = titles.isEmpty() ? w->accessibleName() : titles.first();
        out_value = values.join(QStringLiteral(", "));
        out_search_blob = blobs.join(QLatin1Char(' '));
    };

    struct SearchCandidate {
        int score;
        int cat_row;
        int tab_idx;
        QString display_text;
        QString tooltip;
        QWidget* widget;
    };
    std::vector<SearchCandidate> candidates;

    for (int row = 0; row < ui->selectorList->count(); ++row) {
        auto* cat_item = ui->selectorList->item(row);
        const QString cat_name = cat_item->text();
        const auto tabs = qvariant_cast<QList<QWidget*>>(cat_item->data(Qt::UserRole));

        for (int t_idx = 0; t_idx < tabs.size(); ++t_idx) {
            QWidget* tab = tabs[t_idx];
            if (!tab) {
                continue;
            }
            const QString tab_name = GetLocalizedTabText(tab->accessibleName());
            const auto shared_widgets = tab->findChildren<ConfigurationShared::Widget*>();

            if (!shared_widgets.isEmpty()) {
                for (auto* sw : shared_widgets) {
                    QString title, val, blob;
                    extract_widget_summary(sw, title, val, blob);
                    const QString full_blob = cat_name + QLatin1Char(' ') + tab_name + QLatin1Char(' ') + blob;
                    const bool match = matches_all_tokens(full_blob);
                    sw->setVisible(match);
                    if (has_query && match && !title.isEmpty()) {
                        int score = 10;
                        if (title.startsWith(query, Qt::CaseInsensitive)) {
                            score = 100;
                        } else if (title.contains(query, Qt::CaseInsensitive)) {
                            score = 70;
                        } else if (val.contains(query, Qt::CaseInsensitive)) {
                            score = 50;
                        }
                        const QString disp = val.isEmpty()
                            ? QStringLiteral("⚡ [%1 › %2]  %3").arg(cat_name, tab_name, title)
                            : QStringLiteral("⚡ [%1 › %2]  %3  —  %4").arg(cat_name, tab_name, title, val);
                        candidates.push_back({score, row, t_idx, disp, sw->toolTip(), sw});
                    }
                }
            } else if (has_query) {
                // Non-ConfigurationShared tabs (General, Audio, System, Controls, Hotkeys, Network, etc.)
                const auto group_boxes = tab->findChildren<QGroupBox*>();
                bool added_subwidget = false;
                for (auto* gb : group_boxes) {
                    QString title, val, blob;
                    extract_widget_summary(gb, title, val, blob);
                    const QString full_blob = cat_name + QLatin1Char(' ') + tab_name + QLatin1Char(' ') + blob;
                    if (matches_all_tokens(full_blob) && !title.isEmpty()) {
                        int score = title.startsWith(query, Qt::CaseInsensitive) ? 95
                                  : (title.contains(query, Qt::CaseInsensitive) ? 65 : 20);
                        const QString disp = val.isEmpty()
                            ? QStringLiteral("⚡ [%1 › %2]  %3").arg(cat_name, tab_name, title)
                            : QStringLiteral("⚡ [%1 › %2]  %3  —  %4").arg(cat_name, tab_name, title, val);
                        candidates.push_back({score, row, t_idx, disp, gb->toolTip(), gb});
                        added_subwidget = true;
                    }
                }
                if (!added_subwidget) {
                    QString title, val, blob;
                    extract_widget_summary(tab, title, val, blob);
                    const QString full_blob = cat_name + QLatin1Char(' ') + tab_name + QLatin1Char(' ') + blob;
                    if (matches_all_tokens(full_blob)) {
                        const QString disp = QStringLiteral("⚡ [%1 › %2]  %3").arg(cat_name, tab_name, title.isEmpty() ? tab_name : title);
                        candidates.push_back({15, row, t_idx, disp, tab->toolTip(), tab});
                    }
                }
            }
        }
    }

    if (!has_query) {
        m_search_results_list->clear();
        m_search_results_list->setVisible(false);
        if (graphics_tab) {
            graphics_tab->SetConfiguration();
        }
        return;
    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const SearchCandidate& a, const SearchCandidate& b) {
        return a.score > b.score;
    });

    m_search_results_list->clear();
    QSet<QString> seen_displays;
    const int max_results = 35;
    for (const auto& c : candidates) {
        if (seen_displays.contains(c.display_text)) {
            continue;
        }
        seen_displays.insert(c.display_text);
        auto* item = new QListWidgetItem(c.display_text, m_search_results_list);
        item->setData(Qt::UserRole, c.cat_row);
        item->setData(Qt::UserRole + 1, c.tab_idx);
        item->setData(Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(c.widget)));
        if (!c.tooltip.isEmpty()) {
            item->setToolTip(c.tooltip);
        }
        if (m_search_results_list->count() >= max_results) {
            break;
        }
    }

    if (m_search_results_list->count() > 0) {
        m_search_results_list->setVisible(true);
        m_search_results_list->setCurrentRow(0);
        const int row_h = std::max(28, m_search_results_list->sizeHintForRow(0));
        const int list_h = std::clamp(m_search_results_list->count() * row_h + 12, 42, 195);
        m_search_results_list->setFixedHeight(list_h);
        ActivateSearchResult(m_search_results_list->item(0));
    } else {
        auto* empty_item = new QListWidgetItem(
            StormLang(QStringLiteral("Ничего не найдено по запросу «%1»"),
                      QStringLiteral("No settings found for '%1'"),
                      QStringLiteral("Keine Einstellungen für „%1“ gefunden"),
                      QStringLiteral("Aucun paramètre trouvé pour « %1 »"),
                      QStringLiteral("未找到与“%1”相关的设置"),
                      QStringLiteral("「%1」に一致する設定が見つかりません")).arg(query),
            m_search_results_list);
        empty_item->setData(Qt::UserRole, -1);
        m_search_results_list->setFixedHeight(40);
        m_search_results_list->setVisible(true);
    }
}

