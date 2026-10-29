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
#include "storm_switch/configuration/configure_per_game_addons.h"
#include "storm_switch/configuration/configure_gamebanana_mods.h"
#include "storm_switch/configuration/configure_per_game_amiibo.h"
#include "storm_switch/configuration/configure_per_game_cheats.h"
#include "storm_switch/configuration/configure_system.h"
#include "storm_switch/util/util.h"

#include "storm_switch/common/storm_lang.h"
ConfigurePerGame::ConfigurePerGame(QWidget* parent, u64 title_id_, const std::string& file_name,
                                   std::vector<VkDeviceInfo::Record>& vk_device_records,
                                   Core::System& system_)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigurePerGame>()), title_id{title_id_},
      system{system_},
      builder{std::make_unique<ConfigurationShared::Builder>(this, !system_.IsPoweredOn())},
      tab_group{std::make_shared<std::vector<ConfigurationShared::Tab*>>()} {
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

    connect(gamebanana_tab.get(), &ConfigureGameBananaMods::ModInstalled, this, [this]() {
        if (file) {
            addons_tab->LoadFromFile(file);
        }
    });

    ui->setupUi(this);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setClearButtonEnabled(true);
    m_search_edit->setPlaceholderText(StormLang(
        QStringLiteral("🔍 Быстрый поиск настройки в свойствах игры (например: 21:9, 60 FPS, LSFG, Vulkan, память)..."),
        QStringLiteral("🔍 Quick search per-game settings (e.g. 21:9, 60 FPS, LSFG, Vulkan, memory)..."),
        QStringLiteral("🔍 Spiel-Einstellungen schnell suchen (z. B. 21:9, 60 FPS, LSFG, Vulkan, Speicher)..."),
        QStringLiteral("🔍 Recherche rapide des paramètres du jeu (ex. 21:9, 60 FPS, LSFG, Vulkan, mémoire)..."),
        QStringLiteral("🔍 快速搜索此游戏设置（例如：21:9、60 FPS、LSFG、Vulkan、内存）..."),
        QStringLiteral("🔍 ゲーム別設定をクイック検索（例：21:9、60 FPS、LSFG、Vulkan、メモリ）..."),
        QStringLiteral("🔍 بحث سريع في إعدادات اللعبة (مثل: 21:9، 60 FPS، LSFG، Vulkan، الذاكرة)..."),
        QStringLiteral("🔍 Búsqueda rápida de ajustes del juego (ej.: 21:9, 60 FPS, LSFG, Vulkan, memoria)...")));
    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->insertWidget(0, m_search_edit);
    }
    connect(m_search_edit, &QLineEdit::textChanged, this, &ConfigurePerGame::OnSearchTextChanged);

        ui->tabWidget->addTab(addons_tab.get(), tr("Add-Ons"));
    ui->tabWidget->addTab(gamebanana_tab.get(), StormLang(QStringLiteral("Моды GameBanana"), QStringLiteral("GameBanana Mods"), QStringLiteral("GameBanana-Mods"), QStringLiteral("Mods GameBanana"), QStringLiteral("GameBanana 模组"), QStringLiteral("GameBanana Mod"), QStringLiteral("تعديلات GameBanana"), QStringLiteral("Mods de GameBanana")));
    ui->tabWidget->addTab(amiibo_tab.get(), tr("Amiibo"));
    ui->tabWidget->addTab(cheats_tab.get(), StormLang(QStringLiteral("Читы"), QStringLiteral("Cheats"), QStringLiteral("Cheats"), QStringLiteral("Triche"), QStringLiteral("作弊码"), QStringLiteral("チート"), QStringLiteral("الغش"), QStringLiteral("Trucos")));
    ui->tabWidget->addTab(system_tab.get(), tr("System"));
    ui->tabWidget->addTab(cpu_tab.get(), tr("CPU"));
    ui->tabWidget->addTab(graphics_tab.get(), tr("Graphics"));
    ui->tabWidget->addTab(graphics_advanced_tab.get(), tr("Adv. Graphics"));
    ui->tabWidget->addTab(graphics_extensions_tab.get(), tr("Ext. Graphics"));
    ui->tabWidget->addTab(audio_tab.get(), tr("Audio"));
    ui->tabWidget->addTab(input_tab.get(), tr("Input Profiles"));
    ui->tabWidget->addTab(network_tab.get(), tr("Network"));
    ui->tabWidget->addTab(applets_tab.get(), tr("Applets"));

    setFocusPolicy(Qt::ClickFocus);
    setWindowTitle(tr("Properties"));
    setSizeGripEnabled(true);
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

    ui->tabWidget->tabBar()->setUsesScrollButtons(false);
    ui->tabWidget->tabBar()->setExpanding(false);
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
        setMinimumSize(std::min(1240, max_w), std::min(620, max_h));
        setMaximumSize(max_w, max_h);

        int target_w = std::clamp(1280, std::min(1240, max_w), max_w);
        int target_h = std::min(840, max_h);
        if (QWidget* main_win = parentWidget() ? parentWidget()->window() : nullptr) {
            if (main_win->isVisible() && !main_win->isMinimized()) {
                target_w = std::clamp(std::max(1280, main_win->width() - 36), std::min(1240, max_w), max_w);
                if (avail.height() <= 920) {
                    target_h = std::clamp(std::min(main_win->height() - 16, avail.height() - 68),
                                          std::min(620, max_h), max_h);
                } else {
                    target_h = std::clamp(std::max(820, main_win->height()), std::min(680, max_h), max_h);
                }
            }
        } else if (avail.height() <= 920) {
            target_h = std::clamp(avail.height() - 68, std::min(620, max_h), max_h);
        }
        resize(target_w, target_h);
    } else {
        setMinimumSize(1240, 620);
        resize(1280, 840);
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
            StormLang(QStringLiteral("⚡ Профиль Авто-фикса"),
                      QStringLiteral("⚡ Auto-Fix Profile"),
                      QStringLiteral("⚡ Auto-Fix-Profil"),
                      QStringLiteral("⚡ Profil Auto-Fix"),
                      QStringLiteral("⚡ 自动修复配置"),
                      QStringLiteral("⚡ 自動修正プロファイル"),
                      QStringLiteral("⚡ ملف الإصلاح التلقائي"),
                      QStringLiteral("⚡ Perfil Auto-Fix")),
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
                StormLang(QStringLiteral("Профиль Авто-фикса применён"),
                          QStringLiteral("Auto-Fix Profile Applied"),
                          QStringLiteral("Auto-Fix-Profil angewendet"),
                          QStringLiteral("Profil Auto-Fix appliqué"),
                          QStringLiteral("已应用自动修复配置"),
                          QStringLiteral("自動修正プロファイルを適用しました"),
                          QStringLiteral("تم تطبيق ملف الإصلاح التلقائي"),
                          QStringLiteral("Perfil Auto-Fix aplicado")),
                StormLang(QStringLiteral("Рекомендованные настройки STORM GameFix для этой игры загружены в форму.\nНажмите «ОК» или «Применить», чтобы сохранить их."),
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

    setStyleSheet(QStringLiteral(
        "QDialog#ConfigurePerGame {"
        "    background-color: #0B0F19;"
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
        "    padding: 6px 11px;"
        "    font-size: 12px;"
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
        "QGroupBox#groupBox {"
        "    font-weight: bold;"
        "    color: #00F0FF;"
        "    border: 1px solid rgba(0, 210, 255, 0.25);"
        "    border-radius: 8px;"
        "    margin-top: 0px;"
        "    padding: 10px 10px 10px 10px;"
        "    background: rgba(15, 23, 42, 0.65);"
        "}"
        "QGraphicsView#icon_view {"
        "    background: #0B1325;"
        "    border: 1px solid rgba(0, 210, 255, 0.4);"
        "    border-radius: 10px;"
        "}"
        "QTextEdit#display_name, QTextEdit#display_filename {"
        "    background-color: #121826;"
        "    border: 1.5px solid #334155;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    color: #FFFFFF;"
        "    font-size: 12px;"
        "}"
        "QLineEdit#display_developer, QLineEdit#display_version, QLineEdit#display_format, QLineEdit#display_size {"
        "    background-color: #121826;"
        "    border: 1.5px solid #334155;"
        "    border-radius: 6px;"
        "    padding: 3px 8px;"
        "    color: #FFFFFF;"
        "    font-size: 11.5px;"
        "}"
        "QLineEdit#display_title_id {"
        "    background-color: #121826;"
        "    border: 1.5px solid #334155;"
        "    border-radius: 6px;"
        "    padding: 3px 8px;"
        "    color: #00F0FF;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 11.5px;"
        "    font-weight: 600;"
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
        "QComboBoxPrivateContainer, QComboBox QListView, QComboBox QAbstractItemView {"
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

    RetranslateUI();
    LoadConfiguration();
}

ConfigurePerGame::~ConfigurePerGame() = default;

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
            QStringLiteral("🔍 Быстрый поиск настройки в свойствах игры (например: 21:9, 60 FPS, LSFG, Vulkan, память)..."),
            QStringLiteral("🔍 Quick search per-game settings (e.g. 21:9, 60 FPS, LSFG, Vulkan, memory)..."),
            QStringLiteral("🔍 Spiel-Einstellungen schnell suchen (z. B. 21:9, 60 FPS, LSFG, Vulkan, Speicher)..."),
            QStringLiteral("🔍 Recherche rapide des paramètres du jeu (ex. 21:9, 60 FPS, LSFG, Vulkan, mémoire)..."),
            QStringLiteral("🔍 快速搜索此游戏设置（例如：21:9、60 FPS、LSFG、Vulkan、内存）..."),
            QStringLiteral("🔍 ゲーム別設定をクイック検索（例：21:9、60 FPS、LSFG、Vulkan、メモリ）..."),
            QStringLiteral("🔍 بحث سريع في إعدادات اللعبة (مثل: 21:9، 60 FPS، LSFG، Vulkan، الذاكرة)..."),
            QStringLiteral("🔍 Búsqueda rápida de ajustes del juego (ej.: 21:9, 60 FPS, LSFG, Vulkan, memoria)...")));
    }
    ui->label->setText(StormLang(QStringLiteral("Название"), QStringLiteral("Title")));
    ui->label_2->setText(StormLang(QStringLiteral("Разработчик:"), QStringLiteral("Developer:")));
    ui->label_3->setText(StormLang(QStringLiteral("Версия:"), QStringLiteral("Version:")));
    ui->label_4->setText(StormLang(QStringLiteral("ID приложения:"), QStringLiteral("Program ID:")));
    ui->label_5->setText(StormLang(QStringLiteral("Формат:"), QStringLiteral("Format:")));
    ui->label_6->setText(StormLang(QStringLiteral("Размер:"), QStringLiteral("Size:")));
    ui->label_7->setText(StormLang(QStringLiteral("Имя файла"), QStringLiteral("File Name")));

    if (ui->tabWidget && ui->tabWidget->count() >= 13) {
        ui->tabWidget->setTabText(0, tr("Add-Ons"));
        ui->tabWidget->setTabText(1, StormLang(QStringLiteral("Моды GameBanana"), QStringLiteral("GameBanana Mods"), QStringLiteral("GameBanana-Mods"), QStringLiteral("Mods GameBanana"), QStringLiteral("GameBanana 模组"), QStringLiteral("GameBanana Mod"), QStringLiteral("تعديلات GameBanana"), QStringLiteral("Mods de GameBanana")));
        ui->tabWidget->setTabText(2, tr("Amiibo"));
        ui->tabWidget->setTabText(3, StormLang(QStringLiteral("Читы"), QStringLiteral("Cheats"), QStringLiteral("Cheats"), QStringLiteral("Triche"), QStringLiteral("作弊码"), QStringLiteral("チート"), QStringLiteral("الغش"), QStringLiteral("Trucos")));
        ui->tabWidget->setTabText(4, tr("System"));
        ui->tabWidget->setTabText(5, tr("CPU"));
        ui->tabWidget->setTabText(6, tr("Graphics"));
        ui->tabWidget->setTabText(7, StormLang(QStringLiteral("Расш. графика"), QStringLiteral("Adv. Graphics"), QStringLiteral("Erw. Grafik"), QStringLiteral("Graphismes avancés"), QStringLiteral("高级图形"), QStringLiteral("拡張描画")));
        ui->tabWidget->setTabText(8, StormLang(QStringLiteral("Доп. графика"), QStringLiteral("Ext. Graphics"), QStringLiteral("Zus. Grafik"), QStringLiteral("Graphismes étendus"), QStringLiteral("扩展图形"), QStringLiteral("追加描画")));
        ui->tabWidget->setTabText(9, tr("Audio"));
        ui->tabWidget->setTabText(10, StormLang(QStringLiteral("Управление"), QStringLiteral("Controls"), QStringLiteral("Steuerung"), QStringLiteral("Commandes"), QStringLiteral("控制"), QStringLiteral("操作"), QStringLiteral("التحكم"), QStringLiteral("Controles")));
        ui->tabWidget->setTabText(11, StormLang(QStringLiteral("Сеть"), QStringLiteral("Network"), QStringLiteral("Netzwerk"), QStringLiteral("Réseau"), QStringLiteral("网络"), QStringLiteral("ネットワーク"), QStringLiteral("الشبكة"), QStringLiteral("Red")));
        ui->tabWidget->setTabText(12, StormLang(QStringLiteral("Апплеты"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("Applets"), QStringLiteral("小程序"), QStringLiteral("アプレット"), QStringLiteral("البريمجات"), QStringLiteral("Applets")));
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

    const std::array<QWidget*, 13> all_tabs = {
        addons_tab.get(), gamebanana_tab.get(), amiibo_tab.get(), cheats_tab.get(),
        system_tab.get(), cpu_tab.get(), graphics_tab.get(), graphics_advanced_tab.get(),
        graphics_extensions_tab.get(), audio_tab.get(), input_tab.get(), network_tab.get(),
        applets_tab.get()
    };

    auto widget_matches = [&](QWidget* w) -> bool {
        if (!has_query) {
            return true;
        }
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
        bool tab_has_match = false;
        const auto shared_widgets = tab->findChildren<ConfigurationShared::Widget*>();
        if (!shared_widgets.isEmpty()) {
            for (auto* sw : shared_widgets) {
                const bool match = widget_matches(sw);
                sw->setVisible(match);
                if (match) {
                    tab_has_match = true;
                }
            }
        } else if (widget_matches(tab)) {
            tab_has_match = true;
        }
        if (tab_has_match && !first_matching_tab) {
            first_matching_tab = tab;
        }
    }

    if (!has_query && graphics_tab) {
        graphics_tab->SetConfiguration();
    } else if (has_query && first_matching_tab) {
        const int idx = ui->tabWidget->indexOf(first_matching_tab);
        if (idx >= 0 && ui->tabWidget->currentIndex() != idx) {
            ui->tabWidget->setCurrentIndex(idx);
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

    // Extract paired version (e.g. "(1.5.1 - 262144 - ...)") from filename if present
    static const QRegularExpression fn_pair_ver_regex{QStringLiteral(R"(\(([0-9]+\.[0-9]+(?:\.[0-9]+)*)\s*-\s*([0-9]+))")};
    const auto fm = fn_pair_ver_regex.match(QString::fromStdString(file->GetName()));
    if (fm.hasMatch() && !fm.captured(1).isEmpty()) {
        ver_str = fm.captured(1);
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

    if (ver_str.isEmpty() || ver_str == QStringLiteral("1.0.0")) {
        static const QRegularExpression fn_vnum_regex{QStringLiteral(R"(\[v([0-9]+)\])")};
        const auto vm = fn_vnum_regex.match(QString::fromStdString(file->GetName()));
        if (vm.hasMatch()) {
            const u32 vnum = vm.captured(1).toUInt();
            if (vnum > 0) {
                ver_str = QString::number(vnum);
            }
        }
    }

    while (ver_str.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        ver_str.remove(0, 1);
    }
    ver_str = ver_str.trimmed();

    if (ver_str.isEmpty() || ver_str == QStringLiteral("0")) {
        ver_str = QStringLiteral("1.0.0");
    }

    addons_tab->SetGameVersion(ver_str);
    addons_tab->LoadFromFile(file);

    ui->display_name->setPlainText(title_text);
    ui->display_developer->setText(dev_text);
    ui->display_developer->setCursorPosition(0);
    ui->display_developer->home(false);
    ui->display_developer->setToolTip(dev_text);
    ui->display_version->setText(ver_str);
    ui->display_version->setCursorPosition(0);
    ui->display_version->home(false);

    if (gamebanana_tab) {
        gamebanana_tab->SetGameInfo(title_id, title_text);
    }
    if (amiibo_tab) {
        amiibo_tab->SetGameInfo(title_id, title_text);
    }

    if (control.second != nullptr) {
        scene->clear();

        QPixmap map;
        const auto bytes = control.second->ReadAllBytes();
        map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));

        const int icon_dim = 144;
        scene->setSceneRect(0, 0, icon_dim, icon_dim);
        ui->icon_view->setSceneRect(0, 0, icon_dim, icon_dim);
        scene->addPixmap(map.scaled(icon_dim, icon_dim,
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        std::vector<u8> bytes;
        if (loader->ReadIcon(bytes) == Loader::ResultStatus::Success) {
            scene->clear();

            QPixmap map;
            map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));

            const int icon_dim = 144;
            scene->setSceneRect(0, 0, icon_dim, icon_dim);
            ui->icon_view->setSceneRect(0, 0, icon_dim, icon_dim);
            scene->addPixmap(map.scaled(icon_dim, icon_dim,
                                        Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
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
}

void ConfigurePerGame::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}
