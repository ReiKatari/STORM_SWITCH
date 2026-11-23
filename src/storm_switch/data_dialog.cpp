// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "data_dialog.h"
#include "frontend_common/data_manager.h"
#include "qt_common/qt_string_lookup.h"
#include "qt_common/util/content.h"
#include "ui_data_dialog.h"
#include "util/util.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QPainter>
#include <QPainterPath>
#include <QProgressDialog>
#include <QTabBar>
#include "storm_switch/common/storm_lang.h"
#include <QtConcurrentRun>

#include <core/frontend/applets/profile_select.h>

#include <applets/qt_profile_select.h>
#include "storm_switch/configuration/storm_ui_structure_helper.h"

DataDialog::DataDialog(QWidget* parent) : QDialog(parent), ui(std::make_unique<Ui::DataDialog>()) {
    ui->setupUi(this);
    StormUI::StructureEngine::InstallEscDismiss(this);
    ui->pages->setUsesScrollButtons(false);
    if (auto* tb = ui->pages->tabBar()) {
        tb->setUsesScrollButtons(false);
        tb->setExpanding(true);
    }

    // TODO: Should we make this a single widget that pulls data from a model?
#define WIDGET(label, name)                                                                        \
    ui->pages->addTab(new DataWidget(FrontendCommon::DataManager::DataDir::name,                   \
                                     QtCommon::StringLookup::DataManager##name##Tooltip,           \
                                     QStringLiteral(#name), this),                                 \
                      label);

    WIDGET(tr("Shaders"), Shaders)
    WIDGET(tr("User NAND"), UserNand)
    WIDGET(tr("System NAND"), SysNand)
    WIDGET(tr("Mods"), Mods)
    WIDGET(tr("Saves"), Saves)

#undef WIDGET
}

DataDialog::~DataDialog() = default;

DataWidget::DataWidget(FrontendCommon::DataManager::DataDir data_dir,
                       QtCommon::StringLookup::StringKey tooltip, const QString& exportName,
                       QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::DataWidget>()), m_dir(data_dir),
      m_exportName(exportName) {
    ui->setupUi(this);

    ui->tooltip->setText(QtCommon::StringLookup::Lookup(tooltip));

    auto make_data_icon = [](int kind, const QColor& col) -> QIcon {
        QPixmap pm(36, 36);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(col, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (kind == 0) { // Folder
            QPainterPath path;
            path.moveTo(5, 10); path.lineTo(14, 10); path.lineTo(17, 13);
            path.lineTo(31, 13); path.lineTo(31, 28); path.lineTo(5, 28);
            path.closeSubpath();
            p.drawPath(path);
        } else if (kind == 1) { // Trash
            p.drawLine(7, 11, 29, 11);
            p.drawLine(14, 7, 22, 7);
            p.drawRoundedRect(QRectF(10, 11, 16, 18), 2, 2);
            p.drawLine(15, 16, 15, 24);
            p.drawLine(21, 16, 21, 24);
        } else if (kind == 2) { // Export (Upload)
            p.drawLine(18, 22, 18, 7);
            p.drawLine(12, 13, 18, 7);
            p.drawLine(24, 13, 18, 7);
            p.drawLine(7, 22, 7, 29); p.drawLine(7, 29, 29, 29); p.drawLine(29, 29, 29, 22);
        } else { // Import (Download)
            p.drawLine(18, 7, 18, 22);
            p.drawLine(12, 16, 18, 22);
            p.drawLine(24, 16, 18, 22);
            p.drawLine(7, 22, 7, 29); p.drawLine(7, 29, 29, 29); p.drawLine(29, 29, 29, 22);
        }
        return QIcon(pm);
    };

    ui->open->setIcon(make_data_icon(0, QColor(QStringLiteral("#38BDF8"))));
    ui->open->setText(StormLang(QStringLiteral("Открыть"), QStringLiteral("Open")));
    ui->clear->setIcon(make_data_icon(1, QColor(QStringLiteral("#F87171"))));
    ui->clear->setText(StormLang(QStringLiteral("Очистить"), QStringLiteral("Clear")));
    ui->upload->setIcon(make_data_icon(2, QColor(QStringLiteral("#34D399"))));
    ui->upload->setText(StormLang(QStringLiteral("Экспорт"), QStringLiteral("Export")));
    ui->download->setIcon(make_data_icon(3, QColor(QStringLiteral("#A78BFA"))));
    ui->download->setText(StormLang(QStringLiteral("Импорт"), QStringLiteral("Import")));

    connect(ui->clear, &QPushButton::clicked, this, &DataWidget::clear);
    connect(ui->open, &QPushButton::clicked, this, &DataWidget::open);
    connect(ui->upload, &QPushButton::clicked, this, &DataWidget::upload);
    connect(ui->download, &QPushButton::clicked, this, &DataWidget::download);

    scan();
}

void DataWidget::clear() {
    std::optional<std::string> user_id = selectProfile();
    if (!user_id)
        return;
    QtCommon::Content::ClearDataDir(m_dir, user_id.value());
    scan();
}

void DataWidget::open() {
    std::optional<std::string> user_id = selectProfile();
    if (!user_id)
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(
        FrontendCommon::DataManager::GetDataDirString(m_dir, user_id.value()))));
}

void DataWidget::upload() {
    std::optional<std::string> user_id = selectProfile();
    if (!user_id)
        return;
    QtCommon::Content::ExportDataDir(m_dir, user_id.value(), m_exportName);
}

void DataWidget::download() {
    std::optional<std::string> user_id = selectProfile();
    if (!user_id)
        return;
    QtCommon::Content::ImportDataDir(m_dir, user_id.value(), std::bind(&DataWidget::scan, this));
}

void DataWidget::scan() {
    ui->size->setText(tr("Calculating..."));

    QFutureWatcher<u64>* watcher = new QFutureWatcher<u64>(this);

    connect(watcher, &QFutureWatcher<u64>::finished, this, [=, this]() {
        u64 size = watcher->result();
        ui->size->setText(
            QString::fromStdString(FrontendCommon::DataManager::ReadableBytesSize(size)));
        watcher->deleteLater();
    });

    watcher->setFuture(
        QtConcurrent::run([this]() { return FrontendCommon::DataManager::DataDirSize(m_dir); }));
}

std::optional<std::string> DataWidget::selectProfile() {
    std::string user_id{};
    if (m_dir == FrontendCommon::DataManager::DataDir::Saves) {
        user_id = GetProfileIDString();
        if (user_id.empty())
            return std::nullopt;
    }

    return user_id;
}
