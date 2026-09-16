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
#include "common/cpu_features.h"
#include "common/memory_detect.h"
#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#endif
#include "common/logging.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "core/core.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
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

static QString StormLang(const QString& ru, const QString& en,
                         const QString& de = QString(), const QString& fr = QString(),
                         const QString& zh = QString(), const QString& ja = QString(),
                         const QString& ar = QString(), const QString& es = QString()) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("ar", 0) == 0 && !ar.isEmpty()) return ar;
    if (lang.rfind("es", 0) == 0 && !es.isEmpty()) return es;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;
    return en;
}

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

    m_auto_settings_btn = ui->buttonBox->addButton(tr("⚡ Auto Settings"), QDialogButtonBox::ActionRole);
    m_auto_settings_btn->setObjectName(QStringLiteral("AutoSettingsButton"));
    m_auto_settings_btn->setCursor(Qt::PointingHandCursor);
    m_auto_settings_btn->setStyleSheet(QStringLiteral(
        "QPushButton#AutoSettingsButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00D2FF, stop:1 #0284C7);"
        "    color: #050B14;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 6px 16px;"
        "    border-radius: 6px;"
        "    border: 1px solid #00F0FF;"
        "}"
        "QPushButton#AutoSettingsButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #38BDF8, stop:1 #00D2FF);"
        "    color: #FFFFFF;"
        "}"
        "QPushButton#AutoSettingsButton:pressed {"
        "    background: #0284C7;"
        "}"
    ));
    auto* shadow = new QGraphicsDropShadowEffect(m_auto_settings_btn);
    shadow->setBlurRadius(10);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 210, 255, 120));
    m_auto_settings_btn->setGraphicsEffect(shadow);
    connect(m_auto_settings_btn, &QPushButton::clicked, this, &ConfigureDialog::OnAutoSettingsClicked);

    adjustSize();
    ui->selectorList->setCurrentRow(0);

    // Selects the leftmost button on the bottom bar (Cancel as of writing)
    ui->buttonBox->setFocus();

    ConfigurationShared::RegisterReloadCallback(reinterpret_cast<uintptr_t>(this), [this]() {
        if (graphics_tab) {
            graphics_tab->SetConfiguration();
        }
    });
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
    if (graphics_tab) {
        graphics_tab->SetConfiguration();
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

    if (m_auto_settings_btn) {
        m_auto_settings_btn->setText(StormLang(
            QStringLiteral("⚡ Авто-настройки"),
            QStringLiteral("⚡ Auto Settings"),
            QStringLiteral("⚡ Automatische Einstellungen"),
            QStringLiteral("⚡ Paramètres automatiques"),
            QStringLiteral("⚡ 自动设置"),
            QStringLiteral("⚡ 自動設定")
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
        ui->tabWidget->addTab(tab, GetLocalizedTabText(tab->accessibleName()));
    }
}

void ConfigureDialog::OnAutoSettingsClicked() {
    DetectHardwareAndApplyAutoSettings();
}

void ConfigureDialog::DetectHardwareAndApplyAutoSettings() {
    // 1. CPU detection
    QString cpu_name;
#ifdef ARCHITECTURE_x86_64
    if (std::strlen(Common::g_cpu_caps.cpu_string) > 0) {
        cpu_name = QString::fromUtf8(Common::g_cpu_caps.cpu_string).trimmed();
    }
#endif
    const int thread_count = static_cast<int>(std::thread::hardware_concurrency());
    const int core_count = Common::GetProcessorCount().value_or(thread_count > 2 ? thread_count / 2 : thread_count);
    if (cpu_name.isEmpty()) {
        cpu_name = tr("Процессор (%1 ядер, %2 потоков)").arg(core_count).arg(thread_count);
    }

    // 2. RAM detection
    u64 total_ram_bytes = Common::GetMemInfo().TotalPhysicalMemory;
    u64 avail_ram_bytes = 0;
#ifdef _WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        total_ram_bytes = mem_status.ullTotalPhys;
        avail_ram_bytes = mem_status.ullAvailPhys;
    }
#endif
    double total_ram_gb = static_cast<double>(total_ram_bytes) / (1024.0 * 1024.0 * 1024.0);
    double avail_ram_gb = static_cast<double>(avail_ram_bytes) / (1024.0 * 1024.0 * 1024.0);

    // 3. GPU and VRAM detection
    QString gpu_name = tr("Не определено");
    u64 vram_bytes = 0;
    if (!vk_records.empty()) {
        int dev_idx = Settings::values.vulkan_device.GetValue();
        if (dev_idx >= 0 && static_cast<size_t>(dev_idx) < vk_records.size()) {
            gpu_name = QString::fromStdString(vk_records[dev_idx].name);
        } else {
            gpu_name = QString::fromStdString(vk_records[0].name);
        }
    }

#ifdef _WIN32
    HMODULE hDxgi = LoadLibraryA("dxgi.dll");
    if (hDxgi) {
        typedef HRESULT (WINAPI *pfnCreateDXGIFactory1)(REFIID, void**);
        auto pCreate = reinterpret_cast<pfnCreateDXGIFactory1>(GetProcAddress(hDxgi, "CreateDXGIFactory1"));
        if (pCreate) {
            IDXGIFactory1* pFactory = nullptr;
            if (SUCCEEDED(pCreate(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory)))) {
                IDXGIAdapter1* pAdapter = nullptr;
                for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
                    DXGI_ADAPTER_DESC1 desc;
                    if (SUCCEEDED(pAdapter->GetDesc1(&desc))) {
                        QString adapter_name = QString::fromWCharArray(desc.Description);
                        if (gpu_name.contains(adapter_name, Qt::CaseInsensitive) || adapter_name.contains(gpu_name, Qt::CaseInsensitive) || i == 0) {
                            if (desc.DedicatedVideoMemory > vram_bytes) {
                                vram_bytes = desc.DedicatedVideoMemory;
                                if (gpu_name == tr("Не определено")) {
                                    gpu_name = adapter_name;
                                }
                            }
                        }
                    }
                    pAdapter->Release();
                }
                pFactory->Release();
            }
        }
        FreeLibrary(hDxgi);
    }
#endif
    double vram_gb = static_cast<double>(vram_bytes) / (1024.0 * 1024.0 * 1024.0);

    // 4. Screen detection
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screen_res = screen ? screen->size() : QSize(1920, 1080);
    int refresh_rate = screen ? static_cast<int>(std::round(screen->refreshRate())) : 60;

    // 5. Power status
    QString power_text = tr("Питание от сети");
    bool on_battery = false;
#ifdef _WIN32
    SYSTEM_POWER_STATUS power_status;
    if (GetSystemPowerStatus(&power_status)) {
        if (power_status.ACLineStatus == 0) {
            on_battery = true;
            power_text = tr("Работа от батареи (%1%)").arg(static_cast<int>(power_status.BatteryLifePercent));
        } else if (power_status.BatteryFlag & 128) {
            power_text = tr("Стационарный ПК (сеть 220V)");
        } else {
            power_text = tr("Ноутбук от сети (зарядка %1%)").arg(static_cast<int>(power_status.BatteryLifePercent));
        }
    }
#endif

    // 6. Optimal profile tier determination
    enum class ProfileTier {
        Eco,
        Balanced,
        Enthusiast
    };

    ProfileTier tier = ProfileTier::Balanced;
    const bool is_integrated = gpu_name.contains(QStringLiteral("Intel"), Qt::CaseInsensitive) ||
                               (gpu_name.contains(QStringLiteral("Vega"), Qt::CaseInsensitive) && vram_gb < 2.0) ||
                               (gpu_name.contains(QStringLiteral("Radeon Graphics"), Qt::CaseInsensitive) && vram_gb < 2.0);

    if (on_battery || is_integrated || core_count <= 4 || total_ram_gb < 7.5) {
        tier = ProfileTier::Eco;
    } else if (vram_gb >= 7.5 || (vram_gb >= 5.5 && (gpu_name.contains(QStringLiteral("RTX"), Qt::CaseInsensitive) || gpu_name.contains(QStringLiteral("RX"), Qt::CaseInsensitive)))) {
        if (core_count >= 6 && total_ram_gb >= 15.0) {
            tier = ProfileTier::Enthusiast;
        } else {
            tier = ProfileTier::Balanced;
        }
    } else {
        tier = ProfileTier::Balanced;
    }

    QString tier_name;
    QStringList applied_list;

    if (tier == ProfileTier::Enthusiast) {
        tier_name = tr("Максимальное качество (Enthusiast)");

        Settings::values.resolution_setup.SetValue(Settings::ResolutionSetup::Res1X);
        Settings::values.gpu_accuracy.SetValue(Settings::GpuAccuracy::Low);
        Settings::values.astc_recompression.SetValue(Settings::AstcRecompression::Uncompressed);
        Settings::values.accelerate_astc.SetValue(Settings::AstcDecodeMode::Hybrid);
        Settings::values.nvdec_emulation.SetValue(Settings::NvdecEmulation::Hybrid);
        Settings::values.use_asynchronous_shaders.SetValue(true);
        Settings::values.use_asynchronous_gpu_emulation.SetValue(true);
        Settings::values.async_presentation.SetValue(true);
        Settings::values.use_reactive_flushing.SetValue(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.enable_gpu_buffer_readback.SetValue(false);
        Settings::values.gpu_clock.SetValue(Settings::GpuClock::Normal);
        Settings::values.eco_thermal_mode.SetValue(true);
        Settings::values.eco_frame_pacing.SetValue(true);
        Settings::values.smart_shader_throttle.SetValue(true);
        Settings::values.cpu_affinity_pinning.SetValue(true);
        Settings::values.use_vulkan_driver_pipeline_cache.SetValue(true);
        Settings::values.vram_garbage_collection.SetValue(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.optimize_spirv_output.SetValue(1);
        Settings::values.enable_frame_skipping.SetValue(true);
        Settings::values.max_anisotropy.SetValue(Settings::AnisotropyMode::X16);
        Settings::values.anti_aliasing.SetValue(Settings::AntiAliasing::Smaa);
        Settings::values.scaling_filter.SetValue(Settings::ScalingFilter::Fsr);
        Settings::values.fsr_sharpening_slider.SetValue(85);
        Settings::values.cpu_accuracy.SetValue(Settings::CpuAccuracy::Auto);
        Settings::values.cpuopt_fastmem.SetValue(true);
        Settings::values.cpuopt_ignore_memory_aborts.SetValue(true);
        Settings::values.use_docked_mode.SetValue(Settings::ConsoleMode::Docked);

        applied_list << tr("Разрешение рендеринга: 1X (720p/1080p) (нативное разрешение Switch для оптимального баланса скорости и стабильности)");
        applied_list << tr("Точность ГПУ: Быстрая (Low) (высокая скорость рендеринга без микрозадержек видеокарты)");
        applied_list << tr("Пересжатие текстур ASTC: Без сжатия (оригинальное бескомпромиссное качество текстур)");
        applied_list << tr("Декодирование ASTC: Гибридный (оптимальная аппаратная и программная распаковка)");
        applied_list << tr("Сглаживание: SMAA (высококачественное субпиксельное сглаживание без замыливания)");
        applied_list << tr("Масштабирование: AMD FSR (пространственный апскейлинг с резкостью 85% для идеальной картинки)");
        applied_list << tr("Анизотропная фильтрация: 16x (максимальная четкость текстур на наклонных поверхностях)");
        applied_list << tr("Режим консоли: В док-станции (максимальные тактовые частоты ЦП/ГПУ и повышенное разрешение)");
    } else if (tier == ProfileTier::Eco) {
        tier_name = tr("Энергосбережение и портативность (Eco)");

        Settings::values.resolution_setup.SetValue(on_battery ? Settings::ResolutionSetup::Res1_2X : Settings::ResolutionSetup::Res3_4X);
        Settings::values.gpu_accuracy.SetValue(Settings::GpuAccuracy::Low);
        Settings::values.astc_recompression.SetValue(Settings::AstcRecompression::Bc3);
        Settings::values.accelerate_astc.SetValue(Settings::AstcDecodeMode::Hybrid);
        Settings::values.nvdec_emulation.SetValue(Settings::NvdecEmulation::Hybrid);
        Settings::values.use_asynchronous_shaders.SetValue(true);
        Settings::values.use_asynchronous_gpu_emulation.SetValue(true);
        Settings::values.async_presentation.SetValue(true);
        Settings::values.use_reactive_flushing.SetValue(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.gpu_clock.SetValue(Settings::GpuClock::Normal);
        Settings::values.eco_thermal_mode.SetValue(true);
        Settings::values.eco_frame_pacing.SetValue(true);
        Settings::values.smart_shader_throttle.SetValue(true);
        Settings::values.cpu_affinity_pinning.SetValue(true);
        Settings::values.use_vulkan_driver_pipeline_cache.SetValue(true);
        Settings::values.vram_garbage_collection.SetValue(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.optimize_spirv_output.SetValue(1);
        Settings::values.enable_frame_skipping.SetValue(true);
        Settings::values.max_anisotropy.SetValue(Settings::AnisotropyMode::Default);
        Settings::values.anti_aliasing.SetValue(Settings::AntiAliasing::None);
        Settings::values.scaling_filter.SetValue(Settings::ScalingFilter::Bilinear);
        Settings::values.cpu_accuracy.SetValue(Settings::CpuAccuracy::Auto);
        Settings::values.cpuopt_fastmem.SetValue(true);
        Settings::values.cpuopt_ignore_memory_aborts.SetValue(true);
        Settings::values.use_docked_mode.SetValue(Settings::ConsoleMode::Handheld);

        applied_list << (on_battery ?
            tr("Разрешение рендеринга: 0.5X (360p/540p) (снижение разрешения для экономии заряда батареи)") :
            tr("Разрешение рендеринга: 0.75X (540p/810p) (пониженное разрешение для слабых ГПУ и снижения нагрева)"));
        applied_list << tr("Точность ГПУ: Быстрая (Low) (высокая скорость рендеринга и максимальная разгрузка видеокарты)");
        applied_list << tr("Пересжатие текстур ASTC: BC3 (аппаратное пересжатие с альфа-каналом снижает расход видеопамяти)");
        applied_list << tr("Декодирование ASTC: Гибридный (оптимальная аппаратная и программная распаковка)");
        applied_list << tr("Эко-выравнивание кадров: Включено (устранение микролагов и холостой нагрузки ЦП)");
        applied_list << tr("Масштабирование: Билинейное (минимальная нагрузка на вычислительные блоки ГПУ)");
        applied_list << tr("Режим консоли: В портативном режиме (сниженное энергопотребление и продление работы от батареи)");
    } else {
        tier_name = tr("Сбалансированный (Balanced 60 FPS)");

        Settings::values.resolution_setup.SetValue(Settings::ResolutionSetup::Res1X);
        Settings::values.gpu_accuracy.SetValue(Settings::GpuAccuracy::Low);
        Settings::values.astc_recompression.SetValue(Settings::AstcRecompression::Uncompressed);
        Settings::values.accelerate_astc.SetValue(Settings::AstcDecodeMode::Hybrid);
        Settings::values.nvdec_emulation.SetValue(Settings::NvdecEmulation::Hybrid);
        Settings::values.use_asynchronous_shaders.SetValue(true);
        Settings::values.use_asynchronous_gpu_emulation.SetValue(true);
        Settings::values.async_presentation.SetValue(true);
        Settings::values.use_reactive_flushing.SetValue(false);
        Settings::values.sync_memory_operations.SetValue(false);
        Settings::values.gpu_clock.SetValue(Settings::GpuClock::Normal);
        Settings::values.eco_thermal_mode.SetValue(true);
        Settings::values.eco_frame_pacing.SetValue(true);
        Settings::values.smart_shader_throttle.SetValue(true);
        Settings::values.cpu_affinity_pinning.SetValue(true);
        Settings::values.use_vulkan_driver_pipeline_cache.SetValue(true);
        Settings::values.vram_garbage_collection.SetValue(false);
        Settings::values.early_release_fences.SetValue(false);
        Settings::values.optimize_spirv_output.SetValue(1);
        Settings::values.enable_frame_skipping.SetValue(true);
        Settings::values.max_anisotropy.SetValue(Settings::AnisotropyMode::Automatic);
        Settings::values.anti_aliasing.SetValue(Settings::AntiAliasing::Fxaa);
        Settings::values.scaling_filter.SetValue(Settings::ScalingFilter::Fsr);
        Settings::values.fsr_sharpening_slider.SetValue(80);
        Settings::values.cpu_accuracy.SetValue(Settings::CpuAccuracy::Auto);
        Settings::values.cpuopt_fastmem.SetValue(true);
        Settings::values.cpuopt_ignore_memory_aborts.SetValue(true);
        Settings::values.use_docked_mode.SetValue(Settings::ConsoleMode::Docked);

        applied_list << tr("Разрешение рендеринга: 1X (720p/1080p) (нативное разрешение Switch для оптимального баланса скорости и качества)");
        applied_list << tr("Точность ГПУ: Быстрая (Low) (высокая скорость рендеринга без избыточной нагрузки на видеокарту)");
        applied_list << tr("Пересжатие текстур ASTC: Без сжатия (оригинальное качество текстур без артефактов)");
        applied_list << tr("Декодирование ASTC: ЦП асинхронно (фоновое декодирование процессором без задержек ГПУ)");
        applied_list << tr("Сглаживание: FXAA (быстрое сглаживание краев геометрии с минимальным расходом ресурсов)");
        applied_list << tr("Масштабирование: AMD FSR (пространственный апскейлинг с резкостью 80% для четкости)");
        applied_list << tr("Анизотропная фильтрация: Автоматически (адаптивная фильтрация текстур)");
        applied_list << tr("Режим консоли: В док-станции (стандартный режим полной производительности)");
    }

    applied_list << tr("Эмуляция Host MMU (fastmem): Включено (прямой маппинг виртуальной памяти для максимального FPS)");
    applied_list << tr("Точность ЦП: Авто (максимальная скорость и совместимость JIT-компилятора Dynarmic)");
    applied_list << tr("Асинхронная компиляция шейдеров: Включено (фоновая сборка конвейеров исключает внутриигровые микрофризы)");
    applied_list << tr("Асинхронный вывод: Включено (устраняет дедлоки потока Vulkan и зацикливание видеоряда)");
    applied_list << tr("Игнорирование прерываний памяти: Включено (защита от падений и аварийных вылетов при обращениях за границы буфера)");

    ReloadAllTabs();

    // Display stylized modal summary dialog
    QDialog reportDialog(this);
    reportDialog.setWindowTitle(tr("⚡ Авто-настройки системы"));
    reportDialog.setMinimumWidth(560);
    reportDialog.setModal(true);
    reportDialog.setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background: #0B111A;"
        "    color: #F0F6FC;"
        "    border: 1px solid rgba(0, 210, 255, 0.35);"
        "    border-radius: 10px;"
        "}"
        "QLabel { color: #E2E8F0; font-family: 'Segoe UI', sans-serif; }"
    ));

    auto* dlg_layout = new QVBoxLayout(&reportDialog);
    dlg_layout->setContentsMargins(18, 16, 18, 16);
    dlg_layout->setSpacing(12);

    auto* headerCard = new QFrame(&reportDialog);
    headerCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 210, 255, 0.15), stop:1 rgba(2, 132, 199, 0.05));"
        "    border: 1px solid rgba(0, 210, 255, 0.35);"
        "    border-radius: 8px;"
        "}"
    ));
    auto* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(12, 10, 12, 10);
    headerLayout->setSpacing(10);

    auto* iconLabel = new QLabel(QStringLiteral("⚡"), headerCard);
    iconLabel->setStyleSheet(QStringLiteral("font-size: 24px; background: transparent; border: none;"));
    headerLayout->addWidget(iconLabel);

    auto* titleLayout = new QVBoxLayout();
    auto* titleLabel = new QLabel(tr("<b>Авто-настройки успешно рассчитаны и применены</b>"), headerCard);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #FFFFFF; background: transparent; border: none;"));
    auto* subtitleLabel = new QLabel(tr("Профиль: <b style='color: #00D2FF;'>%1</b>").arg(tier_name), headerCard);
    subtitleLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #94A3B8; background: transparent; border: none;"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    headerLayout->addLayout(titleLayout, 1);
    dlg_layout->addWidget(headerCard);

    // Hardware Card
    auto* hwCard = new QFrame(&reportDialog);
    hwCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(15, 23, 42, 0.65);"
        "    border: 1px solid rgba(148, 163, 184, 0.20);"
        "    border-radius: 8px;"
        "}"
    ));
    auto* hwLayout = new QVBoxLayout(hwCard);
    hwLayout->setContentsMargins(12, 10, 12, 10);
    hwLayout->setSpacing(4);

    auto* hwTitle = new QLabel(tr("🖥️ <b>Характеристики обнаруженного оборудования:</b>"), hwCard);
    hwTitle->setStyleSheet(QStringLiteral("color: #38BDF8; font-size: 12px; background: transparent; border: none;"));
    hwLayout->addWidget(hwTitle);

    QString vram_str = vram_bytes > 0 ? QStringLiteral(" (%1 ГБ VRAM)").arg(vram_gb, 0, 'f', 1) : QString();
    QString hw_text = QStringLiteral(
        "• <b>ЦП:</b> %1 (%2 ядер / %3 потоков)<br>"
        "• <b>ГПУ:</b> %4%5<br>"
        "• <b>ОЗУ:</b> %6 ГБ (доступно: %7 ГБ)<br>"
        "• <b>Экран:</b> %8x%9 @ %10 Гц<br>"
        "• <b>Питание:</b> %11"
    ).arg(cpu_name)
     .arg(core_count)
     .arg(thread_count)
     .arg(gpu_name)
     .arg(vram_str)
     .arg(total_ram_gb, 0, 'f', 1)
     .arg(avail_ram_gb, 0, 'f', 1)
     .arg(screen_res.width())
     .arg(screen_res.height())
     .arg(refresh_rate)
     .arg(power_text);

    auto* hwLabel = new QLabel(hw_text, hwCard);
    hwLabel->setTextFormat(Qt::RichText);
    hwLabel->setStyleSheet(QStringLiteral("color: #CBD5E1; font-size: 11.5px; line-height: 1.4; background: transparent; border: none;"));
    hwLayout->addWidget(hwLabel);
    dlg_layout->addWidget(hwCard);

    // Settings Card
    auto* settingsCard = new QFrame(&reportDialog);
    settingsCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(0, 210, 255, 0.05);"
        "    border: 1px solid rgba(0, 210, 255, 0.25);"
        "    border-radius: 8px;"
        "}"
    ));
    auto* sLayout = new QVBoxLayout(settingsCard);
    sLayout->setContentsMargins(12, 10, 12, 10);
    sLayout->setSpacing(4);

    auto* sTitle = new QLabel(tr("⚙️ <b>Параметры авто-настроек (производительность оборудования):</b>"), settingsCard);
    sTitle->setStyleSheet(QStringLiteral("color: #00D2FF; font-size: 12px; background: transparent; border: none;"));
    sLayout->addWidget(sTitle);

    QString s_text;
    for (const auto& item : applied_list) {
        s_text += QStringLiteral("✓ %1<br>").arg(item);
    }
    auto* sLabel = new QLabel(s_text, settingsCard);
    sLabel->setTextFormat(Qt::RichText);
    sLabel->setStyleSheet(QStringLiteral("color: #E2E8F0; font-size: 11.5px; line-height: 1.4; background: transparent; border: none;"));
    sLayout->addWidget(sLabel);
    dlg_layout->addWidget(settingsCard);

    auto* okBtn = new QPushButton(tr("Готово"), &reportDialog);
    okBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00D2FF, stop:1 #0284C7);"
        "    color: #050B14;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "    padding: 8px 24px;"
        "    border-radius: 6px;"
        "    border: 1px solid #00F0FF;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #38BDF8, stop:1 #00D2FF);"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background: #0284C7;"
        "}"
    ));
    connect(okBtn, &QPushButton::clicked, &reportDialog, &QDialog::accept);

    auto* btn_layout = new QHBoxLayout();
    btn_layout->setAlignment(Qt::AlignCenter);
    btn_layout->addWidget(okBtn);
    dlg_layout->addLayout(btn_layout);

    reportDialog.exec();
}
