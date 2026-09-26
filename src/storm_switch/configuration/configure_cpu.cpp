// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <typeinfo>
#include <vector>
#include <QComboBox>
#include "common/common_types.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "configuration/shared_widget.h"
#include "core/core.h"
#include "ui_configure_cpu.h"
#include "storm_switch/configuration/configuration_shared.h"
#include "storm_switch/configuration/configure_cpu.h"

ConfigureCpu::ConfigureCpu(const Core::System& system_,
                           std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group_,
                           const ConfigurationShared::Builder& builder, QWidget* parent)
    : Tab(group_, parent), ui{std::make_unique<Ui::ConfigureCpu>()}, system{system_},
      combobox_translations(builder.ComboboxTranslations()) {
    ui->setupUi(this);

    Setup(builder);

    SetConfiguration();

    connect(accuracy_combobox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);

    connect(backend_combobox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);

#ifdef HAS_NCE
    ui->backend_group->setVisible(true);
#endif
}

ConfigureCpu::~ConfigureCpu() = default;

void ConfigureCpu::SetConfiguration() {
    if (accuracy_combobox) {
        const auto val = static_cast<int>(Settings::values.cpu_accuracy.GetValue());
        if (val >= 0 && val < accuracy_combobox->count()) {
            const bool blocked = accuracy_combobox->blockSignals(true);
            accuracy_combobox->setCurrentIndex(val);
            accuracy_combobox->blockSignals(blocked);
        }
    }
    UpdateGroup();
}
void ConfigureCpu::Setup(const ConfigurationShared::Builder& builder) {
    auto* accuracy_layout = ui->widget_accuracy->layout();
    auto* backend_layout = ui->widget_backend->layout();
    auto* unsafe_layout = ui->unsafe_widget->layout();
    std::map<u32, QWidget*> unsafe_hold{};
    std::vector<QWidget*> general_inputs{};
    std::vector<QWidget*> general_checkboxes{};

    std::vector<Settings::BasicSetting*> settings;
    const auto push = [&](Settings::Category category) {
        for (const auto setting : Settings::values.linkage.by_category[category]) {
            settings.push_back(setting);
        }
    };

    push(Settings::Category::Cpu);
    push(Settings::Category::CpuUnsafe);

    for (const auto setting : settings) {
        auto* widget = builder.BuildWidget(setting, apply_funcs);

        if (widget == nullptr) {
            continue;
        }
        if (!widget->Valid()) {
            widget->deleteLater();
            continue;
        }

        if (setting->Id() == Settings::values.cpu_accuracy.Id()) {
            // Keep track of cpu_accuracy combobox to display/hide the unsafe settings
            accuracy_layout->addWidget(widget);
            accuracy_combobox = widget->combobox;
        } else if (setting->Id() == Settings::values.cpu_backend.Id()) {
            backend_layout->addWidget(widget);
            backend_combobox = widget->combobox;
        } else if (setting->Id() == Settings::values.cpu_ticks.Id() ||
                   setting->Id() == Settings::values.cpu_affinity_pinning.Id() ||
                   setting->Id() == Settings::values.cpu_clock.Id()) {
            if (widget->IsInputOrSelectionControl()) {
                general_inputs.push_back(widget);
            } else {
                general_checkboxes.push_back(widget);
            }
        } else {
            // Presently, all other settings here are unsafe checkboxes
            unsafe_hold.insert({setting->Id(), widget});
        }
    }

    for (auto* widget : general_inputs) {
        ui->general_layout->addWidget(widget);
    }
    for (auto* widget : general_checkboxes) {
        ui->general_layout->addWidget(widget);
    }

    for (const auto& [label, widget] : unsafe_hold) {
        auto* w = qobject_cast<ConfigurationShared::Widget*>(widget);
        if (w && w->IsInputOrSelectionControl()) {
            unsafe_layout->addWidget(widget);
        }
    }
    for (const auto& [label, widget] : unsafe_hold) {
        auto* w = qobject_cast<ConfigurationShared::Widget*>(widget);
        if (!w || !w->IsInputOrSelectionControl()) {
            unsafe_layout->addWidget(widget);
        }
    }

    UpdateGroup();
}

void ConfigureCpu::UpdateGroup() {
    const u32 accuracy = accuracy_combobox->currentIndex();
    const u32 backend = backend_combobox->currentIndex();
    // TODO(crueter): see if this works on NCE
    ui->unsafe_group->setVisible(accuracy == (u32)Settings::CpuAccuracy::Unsafe &&
                                 backend == (u32)Settings::CpuBackend::Dynarmic);
}

void ConfigureCpu::ApplyConfiguration() {
    const bool is_powered_on = system.IsPoweredOn();
    for (const auto& apply_func : apply_funcs) {
        apply_func(is_powered_on);
    }
    if (Settings::IsConfiguringGlobal() && accuracy_combobox) {
        const int idx = accuracy_combobox->currentIndex();
        if (idx >= 0 && idx <= 4) {
            Settings::values.cpu_accuracy.SetGlobal(true);
            Settings::values.cpu_accuracy.SetValue(static_cast<Settings::CpuAccuracy>(idx));
        }
    }
}

void ConfigureCpu::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureCpu::RetranslateUI() {
    ui->retranslateUi(this);
}
