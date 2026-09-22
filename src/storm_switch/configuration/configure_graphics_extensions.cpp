// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <qnamespace.h>
#include "common/settings.h"
#include "core/core.h"
#include "qt_common/config/shared_translation.h"
#include "ui_configure_graphics_extensions.h"
#include "storm_switch/configuration/configuration_shared.h"
#include "storm_switch/configuration/configure_graphics_extensions.h"
#include "storm_switch/configuration/shared_widget.h"
#include "qt_common/config/uisettings.h"

static QString StormLang(const QString& ru, const QString& en,
                         const QString& de = QString(), const QString& fr = QString(),
                         const QString& zh = QString(), const QString& ja = QString()) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("ru", 0) == 0) return ru;
    if (lang.rfind("de", 0) == 0 && !de.isEmpty()) return de;
    if (lang.rfind("fr", 0) == 0 && !fr.isEmpty()) return fr;
    if (lang.rfind("zh", 0) == 0 && !zh.isEmpty()) return zh;
    if (lang.rfind("ja", 0) == 0 && !ja.isEmpty()) return ja;
    return en;
}

ConfigureGraphicsExtensions::ConfigureGraphicsExtensions(
    const Core::System& system_, std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group_,
    const ConfigurationShared::Builder& builder, QWidget* parent)
    : Tab(group_, parent), ui{std::make_unique<Ui::ConfigureGraphicsExtensions>()},
      system{system_} {

    ui->setupUi(this);

    Setup(builder);

    SetConfiguration();

    RetranslateUI();
}

ConfigureGraphicsExtensions::~ConfigureGraphicsExtensions() = default;

void ConfigureGraphicsExtensions::SetConfiguration() {}

void ConfigureGraphicsExtensions::Setup(const ConfigurationShared::Builder& builder) {
    auto& layout = *ui->populate_target->layout();

    std::map<u32, QWidget*> hold{}; // A map will sort the data for us

    for (auto setting :
         Settings::values.linkage.by_category[Settings::Category::RendererExtensions]) {
        ConfigurationShared::Widget* widget = [&]() {
            if (setting->Id() == Settings::values.sample_shading.Id()) {
                // TODO(crueter): should support this natively perhaps?
                return builder.BuildWidget(
                    setting, apply_funcs, ConfigurationShared::RequestType::Slider, true, 1.0f,
                    nullptr, tr("%", "Sample Shading percentage (e.g. 50%)"));
            } else {
                return builder.BuildWidget(setting, apply_funcs);
            }
        }();

        if (widget == nullptr) {
            continue;
        }
        if (!widget->Valid()) {
            widget->deleteLater();
            continue;
        }

        hold.emplace(setting->Id(), widget);

#ifdef __APPLE__
        if (setting->Id() == Settings::values.dyna_state.Id()) {
            widget->setEnabled(false);
            widget->setToolTip(tr("Extended Dynamic State is disabled on macOS due to MoltenVK "
                                  "compatibility issues that cause black screens."));
        }
#endif
    }

    for (const auto& [id, widget] : hold) {
        layout.addWidget(widget);
    }

    auto& hacks = *ui->hacks_target->layout();
    std::map<u32, QWidget*> hacks_hold{}; // A map will sort the data for us

    for (auto setting : Settings::values.linkage.by_category[Settings::Category::RendererHacks]) {
        auto* widget = builder.BuildWidget(setting, apply_funcs);

        if (widget == nullptr) {
            continue;
        }

        if (!widget->Valid()) {
            widget->deleteLater();
            continue;
        }

        hacks_hold.emplace(setting->Id(), widget);
    }

    for (const auto& [id, widget] : hacks_hold) {
        hacks.addWidget(widget);
    }
}

void ConfigureGraphicsExtensions::ApplyConfiguration() {
    const bool is_powered_on = system.IsPoweredOn();
    for (const auto& func : apply_funcs) {
        func(is_powered_on);
    }
}

void ConfigureGraphicsExtensions::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureGraphicsExtensions::RetranslateUI() {
    ui->retranslateUi(this);
    setAccessibleName(StormLang(
        QStringLiteral("Дополнительно"),
        QStringLiteral("Extensions"),
        QStringLiteral("Erweiterungen"),
        QStringLiteral("Extensions"),
        QStringLiteral("扩展"),
        QStringLiteral("拡張機能")
    ));
    ui->hacks->setTitle(StormLang(
        QStringLiteral("Хаки"),
        QStringLiteral("Hacks"),
        QStringLiteral("Hacks"),
        QStringLiteral("Hacks"),
        QStringLiteral("Hack"),
        QStringLiteral("ハック")
    ));
    ui->label->setText(StormLang(
        QStringLiteral("Изменение этих параметров может вызвать проблемы. Только для опытных пользователей!"),
        QStringLiteral("Modifying these settings can cause problems. For advanced users only!"),
        QStringLiteral("Das Ändern dieser Einstellungen kann Probleme verursachen. Nur für fortgeschrittene Benutzer!"),
        QStringLiteral("La modification de ces paramètres peut causer des problèmes. Réservé aux utilisateurs avancés !"),
        QStringLiteral("修改这些设置可能会导致问题。仅供高级用户使用！"),
        QStringLiteral("これらの設定を変更すると問題が発生する可能性があります。上級ユーザー専用です！")
    ));
    ui->groupBox_1->setTitle(StormLang(
        QStringLiteral("Расширения Vulkan"),
        QStringLiteral("Vulkan Extensions"),
        QStringLiteral("Vulkan-Erweiterungen"),
        QStringLiteral("Extensions Vulkan"),
        QStringLiteral("Vulkan 扩展"),
        QStringLiteral("Vulkan 拡張機能")
    ));
}
