// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <QTabBar>
#include "ui_configure_debug_tab.h"
#include "storm_switch/configuration/configure_cpu_debug.h"
#include "storm_switch/configuration/configure_debug.h"
#include "storm_switch/configuration/configure_debug_tab.h"

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

ConfigureDebugTab::ConfigureDebugTab(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureDebugTab>()},
      debug_tab{std::make_unique<ConfigureDebug>(system_, this)},
      cpu_debug_tab{std::make_unique<ConfigureCpuDebug>(system_, this)} {
    ui->setupUi(this);

    QFont tab_font = font();
    tab_font.setBold(true);
    ui->tabWidget->tabBar()->setFont(tab_font);

    ui->tabWidget->addTab(debug_tab.get(), StormLang(QStringLiteral("Отладка"), QStringLiteral("Debug"), QStringLiteral("Debug"), QStringLiteral("Débogage"), QStringLiteral("调试"), QStringLiteral("デバッグ")));
    ui->tabWidget->addTab(cpu_debug_tab.get(), StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU")));

    SetConfiguration();
}

ConfigureDebugTab::~ConfigureDebugTab() = default;

void ConfigureDebugTab::ApplyConfiguration() {
    debug_tab->ApplyConfiguration();
    cpu_debug_tab->ApplyConfiguration();
}

void ConfigureDebugTab::SetCurrentIndex(int index) {
    ui->tabWidget->setCurrentIndex(index);
}

void ConfigureDebugTab::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureDebugTab::RetranslateUI() {
    ui->retranslateUi(this);
    ui->tabWidget->setTabText(0, StormLang(QStringLiteral("Отладка"), QStringLiteral("Debug"), QStringLiteral("Debug"), QStringLiteral("Débogage"), QStringLiteral("调试"), QStringLiteral("デバッグ")));
    ui->tabWidget->setTabText(1, StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU"), QStringLiteral("CPU")));
}

void ConfigureDebugTab::SetConfiguration() {}
