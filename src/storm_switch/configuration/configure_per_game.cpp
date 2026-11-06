#include "storm_switch/common/storm_design_engine.h"
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QPainterPath>
#include "frontend_common/play_time_manager.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"
#include "qt_common/config/setting_conflicts.h"
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fmt/ranges.h>

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QFontMetrics>
#include <QGroupBox>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QString>
#include <QTabBar>
#include <QTimer>

#include "common/cityhash.h"
#include "common/fs/fs_util.h"
#include "common/fs/path_util.h"
#include "common/string_util.h"
#include "common/settings_enums.h"
#include "common/settings_input.h"
#include "configuration/shared_widget.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/xts_archive.h"
#include "core/hle/service/game_fix_database.h"
#include "core/loader/loader.h"
#include "frontend_common/config.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/qt_common.h"
#include "qt_common/util/vk.h"
#include "ui_configure_per_game.h"
#include "storm_switch/configuration/configuration_shared.h"
#include "storm_switch/configuration/configure_applets.h"
#include "storm_switch/configuration/configure_audio.h"
#include "storm_switch/configuration/configure_cpu.h"
#include "storm_switch/configuration/configure_graphics.h"
#include "storm_switch/configuration/configure_graphics_advanced.h"
#include "storm_switch/configuration/configure_graphics_extensions.h"
#include "storm_switch/configuration/configure_input_per_game.h"
#include "storm_switch/configuration/configure_network.h"
#include "storm_switch/configuration/configure_per_game.h"
#include <QListWidget>
#include "storm_switch/configuration/storm_ui_structure_helper.h"
#include "qt_common/config/setting_conflicts.h"
#include "storm_switch/configuration/configure_per_game_addons.h"
#include "storm_switch/configuration/configure_gamebanana_mods.h"
#include "storm_switch/configuration/configure_per_game_amiibo.h"
#include "storm_switch/configuration/configure_per_game_cheats.h"
#include "storm_switch/configuration/configure_system.h"
#include "storm_switch/util/util.h"

#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
ConfigurePerGame::ConfigurePerGame(QWidget* parent, u64 title_id_, const std::string& file_name,
                                   std::vector<VkDeviceInfo::Record>& vk_device_records,
                                   Core::System& system_)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigurePerGame>()), title_id{title_id_},
      system{system_},
      builder{std::make_unique<ConfigurationShared::Builder>(this, !system_.IsPoweredOn())},
      tab_group{std::make_shared<std::vector<ConfigurationShared::Tab*>>()} {
    ConfigurationShared::SetCurrentPerGameTitleId(title_id_);
    const auto file_path = std::filesystem::path(Common::FS::ToU8String(file_name));
    const auto file_path_hash = Common::CityHash64(file_name.data(), file_name.size());
    const auto specific_config = fmt::format("{:016X}_{:016X}", title_id, file_path_hash);
    const auto legacy_config = title_id == 0 ? Common::FS::PathToUTF8String(file_path.filename())
                                             : fmt::format("{:016X}", title_id);

    std::filesystem::path custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
    std::string config_file_name = specific_config;
    if (!std::filesystem::exists(custom_path / (specific_config + ".ini")) &&
        std::filesystem::exists(custom_path / (legacy_config + ".ini"))) {
        std::error_code ec;
        std::filesystem::copy_file(custom_path / (legacy_config + ".ini"), custom_path / (specific_config + ".ini"), std::filesystem::copy_options::skip_existing, ec);
    }
    game_config = std::make_unique<QtConfig>(config_file_name, Config::ConfigType::PerGameConfig);
    addons_tab = std::make_unique<ConfigurePerGameAddons>(system_, this);
    gamebanana_tab = std::make_unique<ConfigureGameBananaMods>(system_, title_id_, QString::fromStdString(file_name), addons_tab.get(), this);
    amiibo_tab = std::make_unique<ConfigurePerGameAmiibo>(system_, title_id_, QString::fromStdString(file_name), this);
    cheats_tab = std::make_unique<ConfigurePerGameCheats>(system_, title_id_, QString::fromStdString(file_name), this);
    audio_tab = std::make_unique<ConfigureAudio>(system_, tab_group, *builder, this);
    cpu_tab = std::make_unique<ConfigureCpu>(system_, tab_group, *builder, this);
    graphics_advanced_tab =
        std::make_unique<ConfigureGraphicsAdvanced>(system_, tab_group, *builder, this);
    graphics_extensions_tab =
        std::make_unique<ConfigureGraphicsExtensions>(system_, tab_group, *builder, this);
    graphics_tab = std::make_unique<ConfigureGraphics>(
        system_, vk_device_records, [&]() { graphics_advanced_tab->ExposeComputeOption(); },
        [](Settings::AspectRatio, Settings::ResolutionSetup) {}, tab_group, *builder, this);
    input_tab = std::make_unique<ConfigureInputPerGame>(system_, game_config.get(), this);
    system_tab = std::make_unique<ConfigureSystem>(system_, tab_group, *builder, this);
    network_tab = std::make_unique<ConfigureNetwork>(system_, this);
    applets_tab = std::make_unique<ConfigureApplets>(system_, tab_group, *builder, this);

    for (QWidget* t : {static_cast<QWidget*>(addons_tab.get()), static_cast<QWidget*>(gamebanana_tab.get()),
                       static_cast<QWidget*>(amiibo_tab.get()), static_cast<QWidget*>(cheats_tab.get()),
                       static_cast<QWidget*>(audio_tab.get()), static_cast<QWidget*>(cpu_tab.get()),
                       static_cast<QWidget*>(graphics_advanced_tab.get()), static_cast<QWidget*>(graphics_extensions_tab.get()),
                       static_cast<QWidget*>(graphics_tab.get()), static_cast<QWidget*>(input_tab.get()),
                       static_cast<QWidget*>(system_tab.get()), static_cast<QWidget*>(network_tab.get()),
                       static_cast<QWidget*>(applets_tab.get())}) {
        if (t) {
            t->hide();
        }
    }

    connect(gamebanana_tab.get(), &ConfigureGameBananaMods::ModInstalled, this, [this]() {
        if (file) {
            addons_tab->LoadFromFile(file);
        }
    });

    ui->setupUi(this);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setObjectName(QStringLiteral("SettingsSearchEdit"));
    m_search_edit->setClearButtonEnabled(true);
    m_search_edit->setPlaceholderText(StormLang(
        QStringLiteral("Быстрый поиск настройки в свойствах игры (например: 21:9, 60 FPS, LSFG, Vulkan, память)..."),
        QStringLiteral("Quick search per-game settings (e.g. 21:9, 60 FPS, LSFG, Vulkan, memory)..."),
        QStringLiteral("Spiel-Einstellungen schnell suchen (z. B. 21:9, 60 FPS, LSFG, Vulkan, Speicher)..."),
        QStringLiteral("Recherche rapide des paramètres du jeu (ex. 21:9, 60 FPS, LSFG, Vulkan, mémoire)..."),
        QStringLiteral("快速搜索此游戏设置（例如：21:9、60 FPS、LSFG、Vulkan、内存）..."),
        QStringLiteral("ゲーム別設定をクイック検索（例：21:9、60 FPS、LSFG、Vulkan、メモリ）..."),
        QStringLiteral("بحث سريع في إعدادات اللعبة (مثل: 21:9، 60 FPS، LSFG، Vulkan، الذاكرة)..."),
        QStringLiteral("Búsqueda rápida de ajustes del juego (ej.: 21:9, 60 FPS, LSFG, Vulkan, memoria)...")));
    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->insertWidget(0, m_search_edit);
    }
    connect(m_search_edit, &QLineEdit::textChanged, this, &ConfigurePerGame::OnSearchTextChanged);
    m_search_edit->setProperty("stormSearchBox", true);
    ui->groupBox->setProperty("stormHeroCard", true);

    m_reset_hero_btn = new QPushButton(ui->groupBox);
    m_reset_hero_btn->setObjectName(QStringLiteral("ResetPerGameHeroButton"));
    m_reset_hero_btn->setText(StormLang(
        QStringLiteral("↺ Сбросить настройки игры"),
        QStringLiteral("↺ Reset Per-Game Overrides"),
        QStringLiteral("↺ Spieleinstellungen zurücksetzen"),
        QStringLiteral("↺ Réinitialiser les paramètres du jeu"),
        QStringLiteral("↺ 重置此游戏设置"),
        QStringLiteral("↺ ゲーム設定をリセット"),
        QStringLiteral("↺ استعادة إعدادات اللعبة"),
        QStringLiteral("↺ Restablecer ajustes del juego")));
    m_reset_hero_btn->setCursor(Qt::PointingHandCursor);
    m_reset_hero_btn->setToolTip(StormLang(
        QStringLiteral("Сбросить все индивидуальные настройки этой игры и вернуть глобальные настройки эмулятора"),
        QStringLiteral("Reset all custom settings for this game and revert to global emulator settings")));

    connect(m_reset_hero_btn, &QPushButton::clicked, this, [this, specific_config, legacy_config, custom_path]() {
        QMessageBox ask_box(this);
        ask_box.setWindowTitle(StormLang(QStringLiteral("Сброс настроек игры"), QStringLiteral("Reset Per-Game Settings")));
        ask_box.setText(StormLang(
            QStringLiteral("<b>Вы уверены, что хотите сбросить все индивидуальные настройки этой игры?</b><br><br>Игра будет использовать стандартные глобальные настройки эмулятора."),
            QStringLiteral("<b>Are you sure you want to reset all custom settings for this game?</b><br><br>The game will revert to global emulator settings.")));
        ask_box.setIcon(QMessageBox::Question);
        auto* ok_btn = ask_box.addButton(StormLang(QStringLiteral("Сбросить"), QStringLiteral("Reset")), QMessageBox::AcceptRole);
        ask_box.addButton(StormLang(QStringLiteral("Отмена"), QStringLiteral("Cancel")), QMessageBox::RejectRole);
        ask_box.exec();
        if (ask_box.clickedButton() == ok_btn) {
            std::error_code ec;
            std::filesystem::remove(custom_path / (specific_config + ".ini"), ec);
            std::filesystem::remove(custom_path / (legacy_config + ".ini"), ec);
            Settings::RestoreGlobalState(false);
            game_config = std::make_unique<QtConfig>(specific_config, Config::ConfigType::PerGameConfig);
            for (const auto tab : *tab_group) {
                tab->SetConfiguration();
            }
            ConfigurationShared::ReloadAllActiveWidgets();
            RefreshOverrideBadges();
            QMessageBox::information(this,
                StormLang(QStringLiteral("Настройки сброшены"), QStringLiteral("Settings Reset")),
                StormLang(QStringLiteral("Все индивидуальные настройки игры успешно сброшены к глобальным."),
                          QStringLiteral("All per-game custom overrides have been reset to global defaults.")));
        }
    });

    if (ui->verticalLayout) {
        ui->verticalLayout->addWidget(m_reset_hero_btn);
    }



    m_selector_list = new QListWidget(this);
    m_selector_list->setObjectName(QStringLiteral("PerGameSelectorList"));
    m_selector_list->setProperty("stormList", true);
    m_selector_list->setFixedWidth(200);
    ui->horizontalLayout->insertWidget(1, m_selector_list);

    // Hide legacy single tabWidget to prevent mutating clear()/addTab() glitches and overlapping
    ui->tabWidget->setVisible(false);
    ui->tabWidget->setEnabled(false);

    // Create stacked container with 6 dedicated category tab widgets (matching ConfigureDialog)
    m_category_stack = new QStackedWidget(this);
    m_category_stack->setObjectName(QStringLiteral("CategoryStack"));
    m_category_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->VerticalLayout->addWidget(m_category_stack);

    auto create_category_tab_widget = [this](const char* name) -> QTabWidget* {
        auto* tw = new QTabWidget(this);
        tw->setObjectName(QString::fromUtf8(name));
        tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tw->setMinimumSize(320, 200);
        tw->tabBar()->setUsesScrollButtons(false);
        tw->tabBar()->setExpanding(false);
        QFont bold_font = font();
        bold_font.setBold(true);
        tw->tabBar()->setFont(bold_font);
        m_category_stack->addWidget(tw);
        return tw;
    };

    m_general_tabs = create_category_tab_widget("GeneralTabWidget");
    m_system_tabs = create_category_tab_widget("SystemTabWidget");
    m_graphics_tabs = create_category_tab_widget("GraphicsTabWidget");
    m_audio_tabs = create_category_tab_widget("AudioTabWidget");
    m_input_tabs = create_category_tab_widget("InputTabWidget");
    m_network_tabs = create_category_tab_widget("NetworkTabWidget");

    // Category 0: General (Общие)
    m_general_tabs->addTab(addons_tab.get(), tr("Add-Ons"));
    m_general_tabs->addTab(gamebanana_tab.get(), StormLang(QStringLiteral("Моды GameBanana"), QStringLiteral("GameBanana Mods")));
    m_general_tabs->addTab(amiibo_tab.get(), tr("Amiibo"));
    m_general_tabs->addTab(cheats_tab.get(), StormLang(QStringLiteral("Читы"), QStringLiteral("Cheats")));
    m_general_tabs->addTab(applets_tab.get(), StormLang(QStringLiteral("Апплеты"), QStringLiteral("Applets")));

    // Category 1: System (Система)
    m_system_tabs->addTab(system_tab.get(), StormLang(QStringLiteral("Система"), QStringLiteral("System")));
    m_system_tabs->addTab(cpu_tab.get(), StormLang(QStringLiteral("Процессор"), QStringLiteral("CPU")));

    // Category 2: Graphics (Графика)
    m_graphics_tabs->addTab(graphics_tab.get(), StormLang(QStringLiteral("Основное"), QStringLiteral("Main")));
    m_graphics_tabs->addTab(graphics_advanced_tab.get(), StormLang(QStringLiteral("Расширенное"), QStringLiteral("Advanced")));
    m_graphics_tabs->addTab(graphics_extensions_tab.get(), StormLang(QStringLiteral("Хаки и расширения"), QStringLiteral("Hacks & Extensions")));

    // Category 3: Audio (Звук)
    m_audio_tabs->addTab(audio_tab.get(), StormLang(QStringLiteral("Звук"), QStringLiteral("Audio")));

    // Category 4: Controls (Управление)
    m_input_tabs->addTab(input_tab.get(), StormLang(QStringLiteral("Управление"), QStringLiteral("Controls")));

    // Category 5: Network (Сеть)
    m_network_tabs->addTab(network_tab.get(), StormLang(QStringLiteral("Сеть"), QStringLiteral("Network")));

    PopulateSelectionList();
    connect(m_selector_list, &QListWidget::currentRowChanged, this, &ConfigurePerGame::UpdateVisibleTabs);
    connect(m_selector_list, &QListWidget::itemSelectionChanged, this, &ConfigurePerGame::UpdateVisibleTabs);
    m_selector_list->setCurrentRow(0);
    UpdateVisibleTabs();

    m_live_filter_engine = new StormUI::LiveFilterEngine(m_search_edit, m_selector_list, m_category_stack, this);
    m_live_filter_engine->RebuildIndex();

    setFocusPolicy(Qt::ClickFocus);
    setWindowTitle(tr("Properties"));
    setSizeGripEnabled(true);
    for (auto* sa : findChildren<QScrollArea*>()) {
        sa->setWidgetResizable(true);
        sa->setFrameShape(QFrame::NoFrame);
        sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        sa->setMinimumSize(200, 160);
    }
    ui->groupBox->setMinimumWidth(290);
    ui->groupBox->setMaximumWidth(320);
    ui->icon_view->setFixedSize(144, 144);
    ui->verticalLayout->setSpacing(8);
    ui->name_layout->setSpacing(3);
    ui->gridLayout_2->setVerticalSpacing(6);
    ui->gridLayout_2->setHorizontalSpacing(8);
    ui->filename_layout->setSpacing(3);

    QScreen* scr = (parentWidget() && parentWidget()->screen()) ? parentWidget()->screen()
                   : (screen() ? screen() : QGuiApplication::primaryScreen());
    if (scr) {
        const QRect avail = scr->availableGeometry();
        const int max_w = std::max(680, avail.width() - 48);
        const int max_h = std::max(420, avail.height() - 56);
        setMinimumSize(std::min(1240, max_w), std::min(500, max_h));
        setMaximumSize(max_w, max_h);

        int target_w = std::clamp(1280, std::min(1240, max_w), max_w);
        int target_h = std::min(620, max_h);
        if (QWidget* main_win = parentWidget() ? parentWidget()->window() : nullptr) {
            if (main_win->isVisible() && !main_win->isMinimized()) {
                target_w = std::clamp(std::max(1280, main_win->width() - 36), std::min(1240, max_w), max_w);
                if (avail.height() <= 920) {
                    target_h = std::clamp(std::min(main_win->height() - 16, avail.height() - 68),
                                          std::min(500, max_h), std::min(620, max_h));
                } else {
                    target_h = std::clamp(main_win->height() - 80, std::min(500, max_h), std::min(620, max_h));
                }
            }
        } else if (avail.height() <= 920) {
            target_h = std::clamp(avail.height() - 68, std::min(500, max_h), std::min(620, max_h));
        }
        resize(target_w, target_h);
    } else {
        setMinimumSize(1240, 500);
        resize(1280, 620);
    }

    ui->display_name->setLineWrapMode(QTextEdit::WidgetWidth);
    ui->display_filename->setLineWrapMode(QTextEdit::WidgetWidth);
    ui->display_name->setAcceptRichText(false);
    ui->display_filename->setAcceptRichText(false);

    QFont bold_font = font();
    bold_font.setBold(true);
    ui->tabWidget->tabBar()->setFont(bold_font);
    ui->label->setFont(bold_font);
    ui->label_2->setFont(bold_font);
    ui->label_3->setFont(bold_font);
    ui->label_4->setFont(bold_font);
    ui->label_5->setFont(bold_font);
    ui->label_6->setFont(bold_font);
    ui->label_7->setFont(bold_font);

    addons_tab->SetTitleId(title_id);

    scene = new QGraphicsScene;
    ui->icon_view->setScene(scene);

    if (system.IsPoweredOn()) {
        QPushButton* apply_button = ui->buttonBox->addButton(QDialogButtonBox::Apply);
        connect(apply_button, &QAbstractButton::clicked, this,
                &ConfigurePerGame::HandleApplyButtonClicked);
    }

    QPushButton* reset_button = ui->buttonBox->addButton(QDialogButtonBox::RestoreDefaults);
    reset_button->setText(StormLang(
        QStringLiteral("↺ Сбросить настройки"),
        QStringLiteral("↺ Restore Defaults"),
        QStringLiteral("↺ Standardeinstellungen"),
        QStringLiteral("↺ Paramètres par défaut"),
        QStringLiteral("↺ 恢复默认设置"),
        QStringLiteral("↺ デフォルトに戻す"),
        QStringLiteral("↺ استعادة الإعدادات الافتراضية"),
        QStringLiteral("↺ Restaurar valores predeterminados")));
    connect(reset_button, &QAbstractButton::clicked, this, [this, specific_config, legacy_config, custom_path]() {
        std::error_code ec;
        std::filesystem::remove(custom_path / (specific_config + ".ini"), ec);
        std::filesystem::remove(custom_path / (legacy_config + ".ini"), ec);
        Settings::RestoreGlobalState(false);
        game_config = std::make_unique<QtConfig>(specific_config, Config::ConfigType::PerGameConfig);
        for (const auto tab : *tab_group) {
            tab->SetConfiguration();
        }
        ConfigurationShared::ReloadAllActiveWidgets();
        QMessageBox::information(
            this,
            StormLang(QStringLiteral("Сброс настроек"), QStringLiteral("Reset Settings"),
                      QStringLiteral("Einstellungen zurücksetzen"), QStringLiteral("Réinitialiser les paramètres"),
                      QStringLiteral("重置设置"), QStringLiteral("設定のリセット"),
                      QStringLiteral("إعادة ضبط الإعدادات"), QStringLiteral("Restablecer configuración")),
            StormLang(QStringLiteral("Персональные настройки игры сброшены на стандартные.\nИгра будет ориентироваться на глобальные настройки эмулятора."),
                      QStringLiteral("Per-game custom settings have been reset to defaults.\nThe game will now use the global emulator settings."),
                      QStringLiteral("Die spielspezifischen Einstellungen wurden zurückgesetzt.\nDas Spiel verwendet nun die globalen Emulator-Einstellungen."),
                      QStringLiteral("Les paramètres personnalisés du jeu ont été réinitialisés.\nLe jeu utilisera désormais les paramètres globaux de l'émulateur."),
                      QStringLiteral("游戏的自定义设置已恢复为默认值。\n游戏现在将使用模拟器的全局设置。"),
                      QStringLiteral("ゲーム個別の設定がデフォルトにリセットされました。\n今後はエミュレータの全体設定が使用されます。"),
                      QStringLiteral("تمت إعادة تعيين الإعدادات المخصصة للعبة إلى الوضع الافتراضي.\nستستخدم اللعبة الآن الإعدادات العامة للمحاكي."),
                      QStringLiteral("La configuración personalizada del juego se ha restablecido.\nEl juego usará ahora la configuración global del emulador.")));
    });

    if (const auto* fix_profile = Core::GameFixDatabase::GetProfileByTitleOrPath(title_id, file_name)) {
        QPushButton* autofix_button = ui->buttonBox->addButton(
            StormLang(QStringLiteral("Профиль авто-исправления"),
                      QStringLiteral("Auto-Fix Profile"),
                      QStringLiteral("Auto-Fix-Profil"),
                      QStringLiteral("Profil Auto-Fix"),
                      QStringLiteral("自动修复配置"),
                      QStringLiteral("自動修正プロファイル"),
                      QStringLiteral("ملف الإصلاح التلقائي"),
                      QStringLiteral("Perfil Auto-Fix")),
            QDialogButtonBox::ActionRole);
        autofix_button->setToolTip(QString::fromUtf8(fix_profile->fixes_ru.empty()
                                                         ? fix_profile->fixes_en.c_str()
                                                         : fix_profile->fixes_ru.c_str()));
        connect(autofix_button, &QAbstractButton::clicked, this, [this, file_name]() {
            Core::GameFixDatabase::ApplyProfileDirectly(title_id, file_name);
            for (const auto tab : *tab_group) {
                tab->SetConfiguration();
            }
            ConfigurationShared::ReloadAllActiveWidgets();
            QMessageBox::information(
                this,
                StormLang(QStringLiteral("Профиль авто-исправления применён"),
                          QStringLiteral("Auto-Fix Profile Applied"),
                          QStringLiteral("Auto-Fix-Profil angewendet"),
                          QStringLiteral("Profil Auto-Fix appliqué"),
                          QStringLiteral("已应用自动修复配置"),
                          QStringLiteral("自動修正プロファイルを適用しました"),
                          QStringLiteral("تم تطبيق ملف الإصلاح التلقائي"),
                          QStringLiteral("Perfil Auto-Fix aplicado")),
                StormLang(QStringLiteral("Рекомендуемые настройки STORM GameFix для этой игры загружены в форму.\nНажмите «ОК» или «Применить», чтобы сохранить их."),
                          QStringLiteral("Recommended STORM GameFix settings for this game have been loaded.\nClick OK or Apply to save them."),
                          QStringLiteral("Die empfohlenen STORM GameFix-Einstellungen wurden geladen.\nKlicken Sie auf OK oder Anwenden, um sie zu speichern."),
                          QStringLiteral("Les paramètres recommandés STORM GameFix ont été chargés.\nCliquez sur OK ou Appliquer pour enregistrer."),
                          QStringLiteral("已加载推荐的 STORM GameFix 游戏配置。\n点击“确定”或“应用”以保存。"),
                          QStringLiteral("推奨される STORM GameFix 設定が読み込まれました。\nOK または適用をクリックして保存してください。"),
                          QStringLiteral("تم تحميل إعدادات STORM GameFix الموصى بها لهذه اللعبة.\nاضغط على موافق أو تطبيق للحفظ."),
                          QStringLiteral("Se han cargado los ajustes recomendados de STORM GameFix.\nHaga clic en Aceptar o Aplicar para guardarlos.")));
        });
    }

    if (auto* cancel_btn = ui->buttonBox->button(QDialogButtonBox::Cancel)) {
        cancel_btn->setProperty("isCancelButton", true);
        cancel_btn->setText(StormCancelText());
        cancel_btn->setStyleSheet(StormCancelButtonStyle());
        cancel_btn->setCursor(Qt::PointingHandCursor);
    }

    for (auto* t : {static_cast<QWidget*>(addons_tab.get()), static_cast<QWidget*>(gamebanana_tab.get()),
                    static_cast<QWidget*>(amiibo_tab.get()), static_cast<QWidget*>(cheats_tab.get()),
                    static_cast<QWidget*>(system_tab.get()), static_cast<QWidget*>(cpu_tab.get()),
                    static_cast<QWidget*>(graphics_tab.get()), static_cast<QWidget*>(graphics_advanced_tab.get()),
                    static_cast<QWidget*>(graphics_extensions_tab.get()), static_cast<QWidget*>(audio_tab.get()),
                    static_cast<QWidget*>(input_tab.get()), static_cast<QWidget*>(network_tab.get()),
                    static_cast<QWidget*>(applets_tab.get())}) {
        if (t) {
            StormUI::StructureEngine::ModernizeForm(t);
        }
    }
    StormUI::StructureEngine::ModernizeForm(this);
    StormUI::StructureEngine::InstallEscDismiss(this);


    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    setStyleSheet(StormDesign::BuildConfigDialogStylesheet(pal, true) + StormCancelButtonQssRule());

    RetranslateUI();
    LoadConfiguration();

    SetupConflictBanner();
    CheckConflicts();
}

ConfigurePerGame::~ConfigurePerGame() {
    ConfigurationShared::SetCurrentPerGameTitleId(0);
}

void ConfigurePerGame::ApplyConfiguration() {
    for (const auto tab : *tab_group) {
        tab->ApplyConfiguration();
    }
    addons_tab->ApplyConfiguration();
    cheats_tab->ApplyConfiguration();
    input_tab->ApplyConfiguration();
    network_tab->ApplyConfiguration();
    applets_tab->ApplyConfiguration();

    if (Settings::IsDockedMode() && Settings::values.players.GetValue()[0].controller_type ==
                                        Settings::ControllerType::Handheld) {
        Settings::values.use_docked_mode.SetValue(Settings::ConsoleMode::Handheld);
        Settings::values.use_docked_mode.SetGlobal(true);
    }

    system.ApplySettings();
    Settings::LogSettings();

    game_config->SaveAllValues();
}

void ConfigurePerGame::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigurePerGame::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (ui && ui->display_developer) {
        ui->display_developer->setCursorPosition(0);
        ui->display_developer->home(false);
    }
}

void ConfigurePerGame::RetranslateUI() {
    ui->retranslateUi(this);
    if (m_search_edit) {
        m_search_edit->setPlaceholderText(StormLang(
            QStringLiteral("Быстрый поиск настройки в свойствах игры (например: 21:9, 60 FPS, LSFG, Vulkan, память)..."),
            QStringLiteral("Quick search per-game settings (e.g. 21:9, 60 FPS, LSFG, Vulkan, memory)..."),
            QStringLiteral("Spiel-Einstellungen schnell suchen (z. B. 21:9, 60 FPS, LSFG, Vulkan, Speicher)..."),
            QStringLiteral("Recherche rapide des paramètres du jeu (ex. 21:9, 60 FPS, LSFG, Vulkan, mémoire)..."),
            QStringLiteral("快速搜索此游戏设置（例如：21:9、60 FPS、LSFG、Vulkan、内存）..."),
            QStringLiteral("ゲーム別設定をクイック検索（例：21:9、60 FPS、LSFG、Vulkan、メモリ）..."),
            QStringLiteral("بحث سريع في إعدادات اللعبة (مثل: 21:9، 60 FPS، LSFG، Vulkan، الذاكرة)..."),
            QStringLiteral("Búsqueda rápida de ajustes del juego (ej.: 21:9, 60 FPS, LSFG, Vulkan, memoria)...")));
    }
    ui->label->setText(StormLang(QStringLiteral("Название"), QStringLiteral("Title")));
    ui->label_2->setText(StormLang(QStringLiteral("Разработчик:"), QStringLiteral("Developer:")));
    ui->label_3->setText(StormLang(QStringLiteral("Версия:"), QStringLiteral("Version:")));
    ui->label_4->setText(StormLang(QStringLiteral("ID приложения:"), QStringLiteral("Program ID:")));
    ui->label_5->setText(StormLang(QStringLiteral("Формат:"), QStringLiteral("Format:")));
    ui->label_6->setText(StormLang(QStringLiteral("Размер:"), QStringLiteral("Size:")));
    ui->label_7->setText(StormLang(QStringLiteral("Имя файла"), QStringLiteral("File Name")));

    if (m_general_tabs) {
        m_general_tabs->setTabText(0, tr("Add-Ons"));
        m_general_tabs->setTabText(1, StormLang(QStringLiteral("Моды GameBanana"), QStringLiteral("GameBanana Mods"), QStringLiteral("GameBanana-Mods"), QStringLiteral("Mods GameBanana"), QStringLiteral("GameBanana 模组"), QStringLiteral("GameBanana Mod"), QStringLiteral("تعديلات GameBanana"), QStringLiteral("Mods de GameBanana")));
        m_general_tabs->setTabText(2, tr("Amiibo"));
        m_general_tabs->setTabText(3, StormLang(QStringLiteral("Читы"), QStringLiteral("Cheats"), QStringLiteral("Cheats"), QStringLiteral("Triche"), QStringLiteral("作弊码"), QStringLiteral("チート"), QStringLiteral("الغش"), QStringLiteral("Trucos")));
        m_general_tabs->setTabText(4, StormLang(QStringLiteral("Апплеты"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("小程序"), QStringLiteral("アプレット"), QStringLiteral("البريمجات"), QStringLiteral("Applets")));
    }
    if (m_system_tabs) {
        m_system_tabs->setTabText(0, tr("System"));
        m_system_tabs->setTabText(1, tr("CPU"));
    }
    if (m_graphics_tabs) {
        m_graphics_tabs->setTabText(0, tr("Graphics"));
        m_graphics_tabs->setTabText(1, StormLang(QStringLiteral("Расширенная графика"), QStringLiteral("Advanced Graphics"), QStringLiteral("Erweiterte Grafik"), QStringLiteral("Graphismes avancés"), QStringLiteral("高级图形"), QStringLiteral("拡張描画")));
        m_graphics_tabs->setTabText(2, StormLang(QStringLiteral("Дополнительная графика"), QStringLiteral("Extended Graphics"), QStringLiteral("Zusätzliche Grafik"), QStringLiteral("Graphismes étendus"), QStringLiteral("扩展图形"), QStringLiteral("追加描画")));
    }
    if (m_audio_tabs) {
        m_audio_tabs->setTabText(0, tr("Audio"));
    }
    if (m_input_tabs) {
        m_input_tabs->setTabText(0, StormLang(QStringLiteral("Управление"), QStringLiteral("Controls"), QStringLiteral("Steuerung"), QStringLiteral("Commandes"), QStringLiteral("控制"), QStringLiteral("操作"), QStringLiteral("التحكم"), QStringLiteral("Controles")));
    }
    if (m_network_tabs) {
        m_network_tabs->setTabText(0, StormLang(QStringLiteral("Сеть"), QStringLiteral("Network"), QStringLiteral("Netzwerk"), QStringLiteral("Réseau"), QStringLiteral("网络"), QStringLiteral("ネットワーク"), QStringLiteral("الشبكة"), QStringLiteral("Red")));
    }

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

void ConfigurePerGame::HandleApplyButtonClicked() {
    UISettings::values.configuration_applied = true;
    ApplyConfiguration();
}

void ConfigurePerGame::OnSearchTextChanged(const QString& text) {
    const QString query = text.trimmed();
    const bool has_query = !query.isEmpty();

    const std::vector<QWidget*> all_tabs = {
        addons_tab.get(),
        gamebanana_tab.get(),
        amiibo_tab.get(),
        cheats_tab.get(),
        applets_tab.get(),
        system_tab.get(),
        cpu_tab.get(),
        graphics_tab.get(),
        graphics_advanced_tab.get(),
        graphics_extensions_tab.get(),
        audio_tab.get(),
        input_tab.get(),
        network_tab.get()
    };

    const std::vector<QTabWidget*> all_tw = {
        m_general_tabs, m_system_tabs, m_graphics_tabs,
        m_audio_tabs, m_input_tabs, m_network_tabs
    };

    if (!has_query) {
        RefreshOverrideBadges();
        for (QWidget* tab : all_tabs) {
            if (!tab) continue;
            for (auto* w : tab->findChildren<QWidget*>()) {
                w->setVisible(true);
            }
        }
        if (graphics_tab) {
            graphics_tab->SetConfiguration();
        }
        return;
    }

    auto widget_matches = [&](QWidget* w) -> bool {
        if (w->toolTip().contains(query, Qt::CaseInsensitive)) {
            return true;
        }
        for (const auto* lbl : w->findChildren<QLabel*>()) {
            if (lbl->text().contains(query, Qt::CaseInsensitive) ||
                lbl->toolTip().contains(query, Qt::CaseInsensitive)) {
                return true;
            }
        }
        for (const auto* cb : w->findChildren<QCheckBox*>()) {
            if (cb->text().contains(query, Qt::CaseInsensitive) ||
                cb->toolTip().contains(query, Qt::CaseInsensitive)) {
                return true;
            }
        }
        for (const auto* combo : w->findChildren<QComboBox*>()) {
            for (int i = 0; i < combo->count(); ++i) {
                if (combo->itemText(i).contains(query, Qt::CaseInsensitive)) {
                    return true;
                }
            }
        }
        return false;
    };

    QWidget* first_matching_tab = nullptr;
    for (QWidget* tab : all_tabs) {
        if (!tab) continue;
        int tab_matches = 0;
        const auto shared_widgets = tab->findChildren<ConfigurationShared::Widget*>();
        if (!shared_widgets.isEmpty()) {
            for (auto* sw : shared_widgets) {
                const bool match = widget_matches(sw);
                sw->setVisible(match);
                if (match) {
                    tab_matches++;
                }
            }
        } else {
            const auto group_boxes = tab->findChildren<QGroupBox*>();
            for (auto* gb : group_boxes) {
                const bool match = widget_matches(gb);
                gb->setVisible(match);
                if (match) {
                    tab_matches++;
                }
            }
        }

        for (auto* tw : all_tw) {
            if (!tw) continue;
            int idx = tw->indexOf(tab);
            if (idx >= 0) {
                QString title = tw->tabText(idx);
                static const QRegularExpression badge_re(QStringLiteral(R"(\s*\(\d+\))"));
                title.remove(badge_re);
                if (tab_matches > 0) {
                    tw->setTabText(idx, QStringLiteral("%1 (%2)").arg(title).arg(tab_matches));
                    if (!first_matching_tab) {
                        first_matching_tab = tab;
                    }
                } else {
                    tw->setTabText(idx, title);
                }
                break;
            }
        }
    }

    if (first_matching_tab && m_category_stack) {
        for (int cat_idx = 0; cat_idx < m_category_stack->count(); ++cat_idx) {
            auto* tw = qobject_cast<QTabWidget*>(m_category_stack->widget(cat_idx));
            if (!tw) continue;
            int t_idx = tw->indexOf(first_matching_tab);
            if (t_idx >= 0) {
                m_selector_list->setCurrentRow(cat_idx);
                tw->setCurrentIndex(t_idx);
                break;
            }
        }
    }
}

void ConfigurePerGame::LoadFromFile(FileSys::VirtualFile file_) {
    file = std::move(file_);
    LoadConfiguration();
}

void ConfigurePerGame::LoadConfiguration() {
    if (file == nullptr) {
        return;
    }

    ui->display_title_id->setText(
        QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char{'0'}).toUpper());
    ui->display_title_id->setCursorPosition(0);
    ui->display_title_id->home(false);

    const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                   system.GetContentProvider()};
    const auto control = pm.GetControlMetadata();
    const auto loader = Loader::GetLoader(system, file);

    QString title_text;
    QString dev_text;
    QString ver_str;

    FileSys::NACP file_nacp;
    if (loader != nullptr && loader->ReadControlData(file_nacp) == Loader::ResultStatus::Success) {
        title_text = QString::fromStdString(file_nacp.GetApplicationName());
        dev_text = QString::fromStdString(file_nacp.GetDeveloperName());
        const auto nacp_ver = file_nacp.GetVersionString();
        if (!nacp_ver.empty() && nacp_ver != "0") {
            ver_str = QString::fromStdString(nacp_ver);
        }
    }

    if (title_text.trimmed().isEmpty() && loader != nullptr) {
        std::string title;
        if (loader->ReadTitle(title) == Loader::ResultStatus::Success) {
            title_text = QString::fromStdString(title);
        }
    }

    if (title_text.trimmed().isEmpty()) {
        title_text = QString::fromStdString(file->GetName());
    }

    while (ver_str.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        ver_str.remove(0, 1);
    }
    ver_str = ver_str.trimmed();

    u64 internal_ver = 0;
    // If NACP returned a raw numeric internal version (e.g. "458752" without dots), treat it as internal_ver
    if (!ver_str.isEmpty() && !ver_str.contains(QLatin1Char('.'))) {
        bool ok_num = false;
        const u64 raw_num = ver_str.toULongLong(&ok_num);
        if (ok_num && raw_num > 0) {
            internal_ver = raw_num;
        }
        ver_str.clear();
    }

    // Extract paired version (e.g. "(1.5.1 - 262144 - ...)") from filename if present
    static const QRegularExpression fn_pair_ver_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
    const auto fm = fn_pair_ver_regex.match(QString::fromStdString(file->GetName()));
    if (fm.hasMatch() && !fm.captured(1).isEmpty()) {
        if (!fm.captured(2).isEmpty()) {
            const u64 f_int = fm.captured(2).toULongLong();
            if (f_int > 0) {
                internal_ver = f_int;
            }
        }
        if (fm.captured(1) != QStringLiteral("1.0.0") || ver_str.isEmpty() || ver_str == QStringLiteral("0")) {
            ver_str = fm.captured(1);
        }
    } else {
        static const QRegularExpression fn_ver_regex{QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))")};
        const auto m = fn_ver_regex.match(QString::fromStdString(file->GetName()));
        if (m.hasMatch() && m.hasCaptured(1)) {
            const QString parsed_ver = m.captured(1);
            if (!parsed_ver.isEmpty() && (ver_str.isEmpty() || ver_str == QStringLiteral("1.0.0"))) {
                ver_str = parsed_ver;
            }
        }
    }

    if (internal_ver == 0) {
        static const QRegularExpression fn_vnum_regex{QStringLiteral(R"(\[v([0-9]+)\])")};
        const auto vm = fn_vnum_regex.match(QString::fromStdString(file->GetName()));
        if (vm.hasMatch()) {
            internal_ver = vm.captured(1).toULongLong();
        }
    }

    // Convert Switch 65536-step internal version (e.g. 458752 -> 1.0.7) when display version is missing or placeholder 1.0.0
    if ((ver_str.isEmpty() || ver_str == QStringLiteral("0") || ver_str == QStringLiteral("1.0.0") || ver_str == QStringLiteral("1.0")) &&
        internal_ver > 0 && (internal_ver % 65536ULL) == 0 && (internal_ver / 65536ULL) <= 200) {
        ver_str = QStringLiteral("1.0.%1").arg(internal_ver / 65536ULL);
    }

    while (ver_str.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        ver_str.remove(0, 1);
    }
    ver_str = ver_str.trimmed();

    if (ver_str.isEmpty() || ver_str == QStringLiteral("0")) {
        ver_str = QStringLiteral("1.0.0");
    }

    QString full_ver_text = ver_str;
    if (internal_ver > 0 && full_ver_text != QString::number(internal_ver)) {
        full_ver_text.append(QStringLiteral(" (%1)").arg(internal_ver));
    }

    addons_tab->SetGameVersion(full_ver_text);
    addons_tab->LoadFromFile(file);

    ui->display_name->setPlainText(title_text);
    ui->display_developer->setText(dev_text);
    ui->display_developer->setCursorPosition(0);
    ui->display_developer->home(false);
    ui->display_developer->setToolTip(dev_text);

    ui->display_version->setText(full_ver_text);
    ui->display_version->setCursorPosition(0);
    ui->display_version->home(false);

    if (gamebanana_tab) {
        gamebanana_tab->SetGameInfo(title_id, title_text);
    }
    if (amiibo_tab) {
        amiibo_tab->SetGameInfo(title_id, title_text);
    }

    ui->icon_view->setFrameShape(QFrame::NoFrame);
    ui->icon_view->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    QPixmap map;
    if (control.second != nullptr) {
        const auto bytes = control.second->ReadAllBytes();
        map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));
    }
    if (map.isNull() && loader != nullptr) {
        std::vector<u8> bytes;
        if (loader->ReadIcon(bytes) == Loader::ResultStatus::Success) {
            map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));
        }
    }
    if (map.isNull()) {
        const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
        const auto icon_file = cache_dir / "icons" / fmt::format("{:016X}.png", title_id);
        const auto jpeg_file = cache_dir / fmt::format("{:016X}.jpeg", title_id);
        const QString icon_str = QString::fromStdString(Common::FS::PathToUTF8String(icon_file));
        const QString jpeg_str = QString::fromStdString(Common::FS::PathToUTF8String(jpeg_file));
        if (QFile::exists(icon_str)) {
            map.load(icon_str);
        } else if (QFile::exists(jpeg_str)) {
            map.load(jpeg_str);
        }
    }

    scene->clear();
    const int icon_dim = 144;
    scene->setSceneRect(0, 0, icon_dim, icon_dim);
    ui->icon_view->setSceneRect(0, 0, icon_dim, icon_dim);

    if (!map.isNull()) {
        const qreal dpr = devicePixelRatioF();
        const int px_dim = static_cast<int>(std::round(icon_dim * dpr));
        QPixmap rounded(px_dim, px_dim);
        rounded.fill(Qt::transparent);
        {
            QPainter painter(&rounded);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QPainterPath rpath;
            rpath.addRoundedRect(0, 0, px_dim, px_dim, static_cast<int>(12 * dpr), static_cast<int>(12 * dpr));
            painter.setClipPath(rpath);
            painter.drawPixmap(0, 0, px_dim, px_dim, map.scaled(px_dim, px_dim, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
        rounded.setDevicePixelRatio(dpr);
        scene->addPixmap(rounded);
    }

    ui->display_filename->setPlainText(QString::fromStdString(file->GetName()));

    QString format_str = QString::fromStdString(Loader::GetFileTypeString(loader ? loader->GetFileType() : Loader::FileType::Unknown));
    const auto ext = Common::ToLower(std::string(Common::FS::GetExtensionFromFilename(file->GetName())));
    if (ext == "nsz") {
        format_str = QStringLiteral("NSZ");
    } else if (ext == "xcz") {
        format_str = QStringLiteral("XCZ");
    }
    ui->display_format->setText(format_str);
    ui->display_format->setCursorPosition(0);
    ui->display_format->home(false);

    const auto valueText = QtCommon::ReadableByteSize(file->GetSize());
    ui->display_size->setText(valueText);
    ui->display_size->setCursorPosition(0);
    ui->display_size->home(false);

    PlayTime::PlayTimeManager ptm;
    const u64 pt_sec = ptm.GetPlayTime(title_id);
    const QString play_time_str = QString::fromStdString(PlayTime::PlayTimeManager::GetReadablePlayTime(pt_sec));
    if (!play_time_str.isEmpty() && ui->display_size) {
        ui->display_size->setToolTip(StormLang(
            QStringLiteral("Размер: %1 • Общее время в игре: %2").arg(valueText, play_time_str),
            QStringLiteral("Size: %1 • Total Playtime: %2").arg(valueText, play_time_str)
        ));
    }

    RefreshOverrideBadges();
}

void ConfigurePerGame::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void ConfigurePerGame::RefreshOverrideBadges() {
    const std::vector<QTabWidget*> all_tw = {
        m_general_tabs, m_system_tabs, m_graphics_tabs,
        m_audio_tabs, m_input_tabs, m_network_tabs
    };
    for (auto* tw : all_tw) {
        if (!tw) continue;
        for (int t = 0; t < tw->count(); ++t) {
            QWidget* tab = tw->widget(t);
            if (!tab) continue;
            int overrides = 0;
            const auto shared_widgets = tab->findChildren<ConfigurationShared::Widget*>();
            for (const auto* sw : shared_widgets) {
                if (sw && sw->restore_button && sw->restore_button->isEnabled()) {
                    overrides++;
                }
            }
            QString title = tw->tabText(t);
            static const QRegularExpression badge_re(QStringLiteral(R"(\s*\[•\s*\d+\]|\s*\(\d+\))"));
            title.remove(badge_re);
            if (overrides > 0) {
                tw->setTabText(t, QStringLiteral("%1 [• %2]").arg(title).arg(overrides));
            } else {
                tw->setTabText(t, title);
            }
        }
    }
}

void ConfigurePerGame::PopulateSelectionList() {
    const std::array<std::pair<QString, QList<QWidget*>>, 6> items{
        {{StormLang(QStringLiteral("Общие"), QStringLiteral("General"), QStringLiteral("Allgemein"), QStringLiteral("Général"), QStringLiteral("通用"), QStringLiteral("全般")),
          {addons_tab.get(), gamebanana_tab.get(), amiibo_tab.get(), cheats_tab.get(), applets_tab.get()}},
         {StormLang(QStringLiteral("Система"), QStringLiteral("System"), QStringLiteral("System"), QStringLiteral("Système"), QStringLiteral("系统"), QStringLiteral("システム")),
          {system_tab.get(), cpu_tab.get()}},
         {StormLang(QStringLiteral("Графика"), QStringLiteral("Graphics"), QStringLiteral("Grafik"), QStringLiteral("Graphismes"), QStringLiteral("图形"), QStringLiteral("グラフィックス")),
          {graphics_tab.get(), graphics_advanced_tab.get(), graphics_extensions_tab.get()}},
         {StormLang(QStringLiteral("Звук"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("Audio"), QStringLiteral("音频"), QStringLiteral("オーディオ")),
          {audio_tab.get()}},
         {StormLang(QStringLiteral("Управление"), QStringLiteral("Controls"), QStringLiteral("Steuerung"), QStringLiteral("Commandes"), QStringLiteral("控制"), QStringLiteral("操作")),
          {input_tab.get()}},
         {StormLang(QStringLiteral("Сеть"), QStringLiteral("Network"), QStringLiteral("Netzwerk"), QStringLiteral("Réseau"), QStringLiteral("网络"), QStringLiteral("ネットワーク")),
          {network_tab.get()}}},
    };

    if (!m_selector_list) return;

    QFont list_font = m_selector_list->font();
    list_font.setBold(true);
    m_selector_list->setFont(list_font);

    [[maybe_unused]] const QSignalBlocker blocker(m_selector_list);
    m_selector_list->clear();
    for (const auto& entry : items) {
        auto* const item = new QListWidgetItem(entry.first);
        item->setFont(list_font);
        item->setData(Qt::UserRole, QVariant::fromValue(entry.second));
        m_selector_list->addItem(item);
    }
}

void ConfigurePerGame::UpdateVisibleTabs() {
    if (!m_selector_list || !m_category_stack) {
        return;
    }
    const int row = m_selector_list->currentRow();
    if (row >= 0 && row < m_category_stack->count()) {
        m_category_stack->setCurrentIndex(row);
    }
}

void ConfigurePerGame::SetupConflictBanner() {
    m_conflict_banner = new QWidget(this);
    m_conflict_banner->setObjectName(QStringLiteral("ConflictBanner"));
    m_conflict_banner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_conflict_banner->setMaximumHeight(36);
    auto* banner_layout = new QHBoxLayout(m_conflict_banner);
    banner_layout->setContentsMargins(10, 4, 10, 4);
    banner_layout->setSpacing(8);
    m_conflict_label = new QLabel(m_conflict_banner);
    m_conflict_label->setStyleSheet(QStringLiteral("border: none; background: transparent; color: #fbbf24; font-weight: 600; font-size: 9pt; padding: 0px;"));
    m_conflict_fix_btn = new QPushButton(StormLang(QStringLiteral("Исправить"), QStringLiteral("Fix")), m_conflict_banner);
    m_conflict_fix_btn->setCursor(Qt::PointingHandCursor);
    m_conflict_fix_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #ffaa00; color: #1a1a1a; font-weight: bold; padding: 4px 12px; border-radius: 4px; border: none; font-size: 8.5pt; }"
        "QPushButton:hover { background: #ffbb22; }"));

    banner_layout->addWidget(m_conflict_label, 1);
    banner_layout->addWidget(m_conflict_fix_btn);

    m_conflict_banner->setStyleSheet(QStringLiteral(
        "QWidget#ConflictBanner { background-color: rgba(245, 158, 11, 0.14); border: 1px solid rgba(245, 158, 11, 0.55); border-radius: 6px; }"));
    m_conflict_banner->setVisible(false);

    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->addWidget(m_conflict_banner);
    }

    connect(m_conflict_fix_btn, &QPushButton::clicked, this, [this]() {
        if (m_conflict_current_fix) {
            m_conflict_current_fix();
            ConfigurationShared::ReloadAllActiveWidgets();
            CheckConflicts();
        }
    });
}

void ConfigurePerGame::CheckConflicts() {
    if (!m_conflict_banner || !m_conflict_label) return;
    const auto conflicts = ConfigurationShared::SettingConflicts::DetectConflicts(true, title_id);
    if (conflicts.empty()) {
        m_conflict_banner->setVisible(false);
        m_conflict_current_fix = nullptr;
    } else {
        const auto& c = conflicts.front();
        const bool is_ru = UISettings::values.language.GetValue().rfind("ru", 0) == 0;
        m_conflict_label->setText(QStringLiteral("<b>%1:</b> %2")
            .arg(c.LocalizedTitle(is_ru))
            .arg(c.LocalizedMessage(is_ru)));
        m_conflict_current_fix = c.apply_fix;
        m_conflict_banner->setVisible(true);
    }
}
