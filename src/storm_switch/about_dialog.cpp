// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>
#include <fmt/ranges.h>
#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "storm_switch/about_dialog.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()} {
    static const std::string build_id = std::string{Common::g_build_id};
    static const std::string yuzu_build =
        fmt::format("{} {}", std::string{Common::g_build_name}, std::string{Common::g_build_version});

    const auto override_build =
        fmt::format(fmt::runtime(std::string(Common::g_title_bar_format_idle)), build_id);
    const auto yuzu_build_version = override_build.empty() ? yuzu_build : override_build;

    ui->setupUi(this);
    const QIcon app_logo = QApplication::windowIcon();
    if (!app_logo.isNull()) {
        ui->labelLogo->setPixmap(app_logo.pixmap(180, 180));
    }
    setWindowTitle(tr("О STORM SWITCH"));
    ui->labelBuildInfo->setText(
        ui->labelBuildInfo->text().arg(QString::fromStdString(yuzu_build_version),
                                       QString::fromUtf8(Common::g_build_date).left(10)));
    ui->labelAbout->setText(tr(
        "<html><head/><body><p><span style=\"font-size:10.5pt; color:#e0e6ed;\">"
        "STORM SWITCH — это передовой высокопроизводительный форк эмулятора Eden (Nintendo Switch), созданный для максимальной производительности, глубокой кастомизации и стабильности.<br/><br/>"
        "STORM SWITCH распространяется под лицензией GPLv3.0+ и включает расширенные режимы графики, ускоренный запуск игр, многофункциональный подвал управления и полную поддержку DLC и обновлений.<br/><br/>"
        "Данное программное обеспечение предназначено исключительно для запуска легально полученных копий игр."
        "</span></p></body></html>"));
    ui->labelLinks->setText(tr(
        "<html><head/><body><p><a href=\"https://eden-emulator.github.io/\"><span style=\"text-decoration: underline; color:#00f2fe;\">Eden Project</span></a> | "
        "<a href=\"https://git.eden-emu.dev\"><span style=\"text-decoration: underline; color:#00f2fe;\">Исходный код</span></a> | "
        "<a href=\"https://git.eden-emu.dev/eden-emu/eden/src/branch/master/LICENSE.txt\"><span style=\"text-decoration: underline; color:#00f2fe;\">Лицензия (GPLv3)</span></a></p></body></html>"));
    ui->labelLiability->setText(tr(
        "<html><head/><body><p><span style=\"font-size:7.5pt; color:#7b8fa3;\">"
        "&quot;Nintendo Switch&quot; является зарегистрированным товарным знаком Nintendo. STORM SWITCH не связан с Nintendo."
        "</span></p></body></html>"));
}

AboutDialog::~AboutDialog() = default;
