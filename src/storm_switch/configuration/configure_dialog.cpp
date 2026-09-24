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
        "    color: #F8FAFC;"
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

    adjustSize();
    ui->selectorList->setCurrentRow(0);

    // Selects the leftmost button on the bottom bar (Cancel as of writing)
    ui->buttonBox->setFocus();

    ConfigurationShared::RegisterReloadCallback(reinterpret_cast<uintptr_t>(this), [this]() {
        if (graphics_tab) {
            graphics_tab->SetConfiguration();
        }
    });

    RetranslateUI();
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
