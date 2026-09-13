// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "online_tools_dialog.h"

#include <algorithm>
#include <filesystem>
#include <tuple>
#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "core/core.h"
#include "core/file_sys/vfs/vfs.h"
#include "qt_common/abstract/frontend.h"
#include "qt_common/qt_common.h"
#include "qt_common/util/content.h"

namespace {

class ToolItemWidget : public QWidget {
public:
    ToolItemWidget(const ReleaseAsset& asset, OnlineToolType type, QWidget* parent = nullptr)
        : QWidget(parent) {
        auto* main_layout = new QHBoxLayout(this);
        main_layout->setContentsMargins(4, 3, 4, 3);

        auto* frame = new QFrame(this);
        frame->setObjectName(QStringLiteral("CardFrame"));
        auto* card_layout = new QHBoxLayout(frame);
        card_layout->setContentsMargins(14, 8, 14, 8);
        card_layout->setSpacing(14);

        // Icon
        auto* icon_label = new QLabel(frame);
        icon_label->setText(type == OnlineToolType::Firmware ? QStringLiteral("📦") : QStringLiteral("🔑"));
        icon_label->setStyleSheet(QStringLiteral("font-size: 24px; background: transparent;"));
        card_layout->addWidget(icon_label);

        // Texts
        auto* text_layout = new QVBoxLayout();
        text_layout->setContentsMargins(0, 0, 0, 0);
        text_layout->setSpacing(4);

        auto* title_label = new QLabel(asset.display_title, frame);
        title_label->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI'; font-size: 14px; font-weight: bold; color: #ffffff; background: transparent;"));
        text_layout->addWidget(title_label);

        auto* sub_label = new QLabel(
            QStringLiteral("Файл: %1 • Размер: %2").arg(asset.name, asset.display_size), frame);
        sub_label->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI'; font-size: 11px; color: #94a3b8; background: transparent;"));
        text_layout->addWidget(sub_label);

        card_layout->addLayout(text_layout, 1);

        // Recommended Badge
        if (asset.is_recommended) {
            auto* badge = new QLabel(tr("🌟 РЕКОМЕНДУЕТСЯ"), frame);
            badge->setStyleSheet(QStringLiteral(
                "background-color: rgba(0, 240, 255, 0.16); "
                "border: 1px solid #00F0FF; "
                "border-radius: 10px; "
                "color: #00F0FF; "
                "font-family: 'Segoe UI'; "
                "font-size: 11px; "
                "font-weight: bold; "
                "padding: 4px 10px;"));
            card_layout->addWidget(badge);
        }

        main_layout->addWidget(frame);
    }
};

} // anonymous namespace

OnlineToolsDialog::OnlineToolsDialog(QWidget* parent, Core::System& system, OnlineToolType type)
    : QDialog(parent), m_system(system), m_type(type) {
    m_nam = new QNetworkAccessManager(this);
    SetupUI();
    PopulateFallbackCatalog();
    UpdateListView();
    RefreshCatalog();
}

OnlineToolsDialog::~OnlineToolsDialog() {
    if (m_current_reply) {
        m_current_reply->abort();
        m_current_reply->deleteLater();
        m_current_reply = nullptr;
    }
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}

void OnlineToolsDialog::SetupUI() {
    resize(640, 560);
    setMinimumSize(560, 480);
    setWindowTitle(m_type == OnlineToolType::Firmware
                       ? tr("STORM SWITCH — Онлайн-установка прошивки")
                       : tr("STORM SWITCH — Онлайн-установка ключей"));

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: #0e1117;"
        "    color: #e2e8f0;"
        "    font-family: 'Segoe UI';"
        "}"
        "QFrame#CardFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #181d27, stop:1 #11141c);"
        "    border: 1px solid #283347;"
        "    border-radius: 8px;"
        "}"
        "QListWidget {"
        "    background-color: #0a0d13;"
        "    border: 1px solid #1f2737;"
        "    border-radius: 8px;"
        "    outline: none;"
        "    padding: 4px;"
        "}"
        "QListWidget::item {"
        "    background: transparent;"
        "    border: none;"
        "    margin: 2px 0px;"
        "}"
        "QListWidget::item:hover QFrame#CardFrame {"
        "    border: 1px solid #00D2FF;"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #202736, stop:1 #151924);"
        "}"
        "QListWidget::item:selected QFrame#CardFrame {"
        "    border: 2px solid #00F0FF;"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1c2b3e, stop:1 #131d2b);"
        "}"
        "QProgressBar {"
        "    background-color: #141824;"
        "    border: 1px solid #263145;"
        "    border-radius: 6px;"
        "    height: 22px;"
        "    text-align: center;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0099ff, stop:1 #00f0ff);"
        "    border-radius: 5px;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #222b3d, stop:1 #161c28);"
        "    border: 1px solid #334155;"
        "    border-radius: 6px;"
        "    color: #f1f5f9;"
        "    padding: 7px 18px;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    border: 1px solid #00D2FF;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d384d, stop:1 #1e2637);"
        "}"
        "QPushButton:pressed {"
        "    background-color: #111520;"
        "}"
        "QPushButton#HeroButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00B4D8, stop:1 #00F0FF);"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: #050a12;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "    padding: 8px 22px;"
        "}"
        "QPushButton#HeroButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00C6EB, stop:1 #33F5FF);"
        "}"
        "QPushButton#HeroButton:disabled {"
        "    background: #1e293b;"
        "    color: #64748b;"
        "}"));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 18, 20, 18);
    main_layout->setSpacing(14);

    // Header Card
    auto* header_frame = new QFrame(this);
    header_frame->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #161c26, stop:1 #0f131a); "
        "border: 1px solid #232b3b; border-radius: 10px; padding: 10px;"));
    auto* header_layout = new QHBoxLayout(header_frame);
    header_layout->setContentsMargins(12, 8, 12, 8);
    header_layout->setSpacing(14);

    m_header_icon = new QLabel(header_frame);
    m_header_icon->setText(m_type == OnlineToolType::Firmware ? QStringLiteral("🌐") : QStringLiteral("🔑"));
    m_header_icon->setStyleSheet(QStringLiteral("font-size: 34px;"));
    header_layout->addWidget(m_header_icon);

    auto* header_text_layout = new QVBoxLayout();
    header_text_layout->setSpacing(3);

    m_header_title = new QLabel(m_type == OnlineToolType::Firmware
                                    ? tr("Онлайн-установка прошивки Nintendo Switch")
                                    : tr("Онлайн-установка ключей дешифрования"),
                                header_frame);
    m_header_title->setStyleSheet(
        QStringLiteral("font-size: 16px; font-weight: bold; color: #00F0FF;"));
    header_text_layout->addWidget(m_header_title);

    m_header_desc = new QLabel(
        m_type == OnlineToolType::Firmware
            ? tr("Выберите версию прошивки для автоматической загрузки и установки в System NAND без сохранения архива на диск.")
            : tr("Выберите версию ключей (prod.keys и title.keys) для загрузки и распаковки в каталог keys с мгновенной активацией."),
        header_frame);
    m_header_desc->setWordWrap(true);
    m_header_desc->setStyleSheet(QStringLiteral("font-size: 11px; color: #94a3b8;"));
    header_text_layout->addWidget(m_header_desc);

    header_layout->addLayout(header_text_layout, 1);
    main_layout->addWidget(header_frame);

    // List of Releases
    m_list_widget = new QListWidget(this);
    m_list_widget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(m_list_widget, &QListWidget::itemSelectionChanged, this,
            &OnlineToolsDialog::OnItemSelectionChanged);
    main_layout->addWidget(m_list_widget, 1);

    // Progress Section
    auto* progress_layout = new QVBoxLayout();
    progress_layout->setSpacing(6);

    m_status_label = new QLabel(tr("Готово к установке. Выберите версию из списка выше."), this);
    m_status_label->setStyleSheet(QStringLiteral("font-size: 12px; color: #cbd5e1;"));
    progress_layout->addWidget(m_status_label);

    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setTextVisible(true);
    m_progress_bar->setVisible(false);
    progress_layout->addWidget(m_progress_bar);

    main_layout->addLayout(progress_layout);

    // Bottom Action Buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(10);

    m_btn_refresh = new QPushButton(tr("🔄 Обновить список"), this);
    connect(m_btn_refresh, &QPushButton::clicked, this, &OnlineToolsDialog::RefreshCatalog);
    btn_layout->addWidget(m_btn_refresh);

    btn_layout->addStretch();

    m_btn_cancel = new QPushButton(tr("Отмена"), this);
    connect(m_btn_cancel, &QPushButton::clicked, this, &OnlineToolsDialog::CancelOperation);
    btn_layout->addWidget(m_btn_cancel);

    m_btn_install = new QPushButton(m_type == OnlineToolType::Firmware
                                        ? tr("📥 Скачать и установить прошивку")
                                        : tr("📥 Скачать и установить ключи"),
                                    this);
    m_btn_install->setObjectName(QStringLiteral("HeroButton"));
    connect(m_btn_install, &QPushButton::clicked, this, &OnlineToolsDialog::StartDownload);
    btn_layout->addWidget(m_btn_install);

    main_layout->addLayout(btn_layout);

    connect(this, &OnlineToolsDialog::InstallationCompleted, this,
            &OnlineToolsDialog::HandleInstallationCompleted);
}

void OnlineToolsDialog::PopulateFallbackCatalog() {
    m_assets.clear();

    if (m_type == OnlineToolType::Firmware) {
        const std::vector<std::pair<QString, qint64>> default_firmwares = {
            {QStringLiteral("20.0.1"), 356052129},
            {QStringLiteral("20.0.0"), 356051105},
            {QStringLiteral("19.0.1.Rebootless.Update"), 338087260},
            {QStringLiteral("19.0.1"), 338082652},
            {QStringLiteral("19.0.0"), 338076508},
        };

        for (const auto& [ver, sz] : default_firmwares) {
            ReleaseAsset asset;
            asset.version = ver;
            asset.name = ver + QStringLiteral(".zip");
            asset.size = sz;
            asset.download_url =
                QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH_TOOLS/releases/download/STORM_SWITCH_TOOLS_FIRMWARES/") +
                asset.name;
            asset.display_title = tr("Прошивка %1").arg(ver);
            asset.display_size = FormatBytes(sz);
            m_assets.push_back(asset);
        }
    } else {
        const std::vector<std::pair<QString, qint64>> default_keys = {
            {QStringLiteral("23.0.0"), 12177}, {QStringLiteral("22.5.0"), 7423},
            {QStringLiteral("22.1.0"), 7407},  {QStringLiteral("22.0.0"), 7407},
            {QStringLiteral("21.2.0"), 11343}, {QStringLiteral("21.1.0"), 11343},
            {QStringLiteral("21.0.1"), 11343}, {QStringLiteral("21.0.0"), 11343},
            {QStringLiteral("20.5.0"), 6976},  {QStringLiteral("20.4.0"), 6976},
            {QStringLiteral("20.3.0"), 6130},  {QStringLiteral("20.2.0"), 6130},
            {QStringLiteral("20.1.5"), 6130},  {QStringLiteral("20.1.1"), 6130},
            {QStringLiteral("20.1.0"), 6130},  {QStringLiteral("20.0.1"), 6130},
            {QStringLiteral("20.0.0"), 6123},  {QStringLiteral("19.0.1"), 8149},
        };

        for (const auto& [ver, sz] : default_keys) {
            ReleaseAsset asset;
            asset.version = ver;
            asset.name = ver + QStringLiteral(".zip");
            asset.size = sz;
            asset.download_url =
                QStringLiteral("https://github.com/ReiKatari/STORM_SWITCH_TOOLS/releases/download/STORM_SWITCH_TOOLS_KEYS/") +
                asset.name;
            asset.display_title = tr("Ключи дешифрования %1").arg(ver);
            asset.display_size = FormatBytes(sz);
            m_assets.push_back(asset);
        }
    }

    // Sort descending by version
    std::sort(m_assets.begin(), m_assets.end(), [](const ReleaseAsset& a, const ReleaseAsset& b) {
        return CompareVersions(a.version, b.version) > 0;
    });

    if (!m_assets.empty()) {
        m_assets.front().is_recommended = true;
    }
}

void OnlineToolsDialog::UpdateListView() {
    m_list_widget->clear();

    for (const auto& asset : m_assets) {
        auto* item = new QListWidgetItem(m_list_widget);
        auto* widget = new ToolItemWidget(asset, m_type, m_list_widget);
        item->setSizeHint(QSize(0, 78));
        item->setData(Qt::UserRole, asset.version);
        m_list_widget->addItem(item);
        m_list_widget->setItemWidget(item, widget);
    }

    if (m_list_widget->count() > 0) {
        m_list_widget->setCurrentRow(0);
    }
}

void OnlineToolsDialog::RefreshCatalog() {
    m_btn_refresh->setEnabled(false);
    m_status_label->setText(tr("Проверка обновлений каталога в GitHub..."));

    const QString tag_name = (m_type == OnlineToolType::Firmware)
                                 ? QStringLiteral("STORM_SWITCH_TOOLS_FIRMWARES")
                                 : QStringLiteral("STORM_SWITCH_TOOLS_KEYS");

    const QUrl api_url(QStringLiteral("https://api.github.com/repos/ReiKatari/STORM_SWITCH_TOOLS/releases/tags/%1").arg(tag_name));

    QNetworkRequest request(api_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_btn_refresh->setEnabled(true);

        if (reply->error() != QNetworkReply::NoError) {
            LOG_WARNING(Frontend, "GitHub API query error: {}", reply->errorString().toStdString());
            m_status_label->setText(tr("Используется встроенный каталог (GitHub API недоступен)."));
            return;
        }

        const QByteArray response_data = reply->readAll();
        QJsonParseError parse_error{};
        const QJsonDocument doc = QJsonDocument::fromJson(response_data, &parse_error);
        if (parse_error.error != QJsonParseError::NoError || !doc.isObject()) {
            LOG_WARNING(Frontend, "Failed to parse GitHub API JSON response");
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonArray assets_array = root.value(QStringLiteral("assets")).toArray();
        if (assets_array.isEmpty()) {
            return;
        }

        std::vector<ReleaseAsset> new_assets;
        for (const auto& asset_val : assets_array) {
            const QJsonObject asset_obj = asset_val.toObject();
            const QString name = asset_obj.value(QStringLiteral("name")).toString();
            if (!name.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
                continue;
            }

            QString ver = name;
            ver.chop(4);

            ReleaseAsset asset;
            asset.version = ver;
            asset.name = name;
            asset.size = asset_obj.value(QStringLiteral("size")).toVariant().toLongLong();
            asset.download_url = asset_obj.value(QStringLiteral("browser_download_url")).toString();
            asset.display_title = (m_type == OnlineToolType::Firmware)
                                      ? tr("Прошивка %1").arg(ver)
                                      : tr("Ключи дешифрования %1").arg(ver);
            asset.display_size = FormatBytes(asset.size);
            new_assets.push_back(asset);
        }

        if (!new_assets.empty()) {
            std::sort(new_assets.begin(), new_assets.end(),
                      [](const ReleaseAsset& a, const ReleaseAsset& b) {
                          return CompareVersions(a.version, b.version) > 0;
                      });
            new_assets.front().is_recommended = true;

            m_assets = std::move(new_assets);
            UpdateListView();
            m_status_label->setText(tr("Каталог успешно обновлён из репозитория."));
        }
    });
}

void OnlineToolsDialog::OnItemSelectionChanged() {
    const int row = m_list_widget->currentRow();
    if (row >= 0 && row < static_cast<int>(m_assets.size())) {
        m_selected_asset = m_assets[row];
        m_btn_install->setEnabled(!m_is_downloading && !m_is_installing);
        if (!m_is_downloading && !m_is_installing) {
            m_status_label->setText(
                tr("Выбрана: %1 (%2)").arg(m_selected_asset.display_title, m_selected_asset.display_size));
        }
    }
}

void OnlineToolsDialog::StartDownload() {
    const int row = m_list_widget->currentRow();
    if (row < 0 || row >= static_cast<int>(m_assets.size())) {
        return;
    }
    m_selected_asset = m_assets[row];

    namespace fs = std::filesystem;
    const fs::path download_dir = fs::temp_directory_path() / "storm_eden" / "online_downloads";
    std::error_code ec;
    fs::create_directories(download_dir, ec);

    const QString filename = (m_type == OnlineToolType::Firmware)
                                 ? QStringLiteral("firmware_stream.zip")
                                 : QStringLiteral("keys_stream.zip");
    m_temp_file_path = QString::fromStdString((download_dir / filename.toStdString()).string());

    m_file = std::make_unique<QFile>(m_temp_file_path);
    qint64 existing_size = 0;
    if (m_file->exists()) {
        existing_size = m_file->size();
    }

    QNetworkRequest request(QUrl(m_selected_asset.download_url));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH-Installer"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    // Support resume (докачка) if partially downloaded file exists
    if (existing_size > 0 && m_selected_asset.size > 0 && existing_size < m_selected_asset.size) {
        request.setRawHeader(QByteArrayLiteral("Range"),
                             QStringLiteral("bytes=%1-").arg(existing_size).toUtf8());
        m_downloaded_offset = existing_size;
    } else {
        m_downloaded_offset = 0;
    }

    m_is_downloading = true;
    m_btn_install->setEnabled(false);
    m_btn_refresh->setEnabled(false);
    m_list_widget->setEnabled(false);
    m_progress_bar->setVisible(true);
    m_progress_bar->setValue(0);
    m_status_label->setText(tr("Подключение к серверу..."));

    m_speed_timer.start();
    m_last_bytes_measured = 0;
    m_last_time_measured = 0;
    m_current_speed_mbps = 0.0;

    m_current_reply = m_nam->get(request);

    connect(m_current_reply, &QNetworkReply::readyRead, this,
            &OnlineToolsDialog::OnDownloadReadyRead);
    connect(m_current_reply, &QNetworkReply::downloadProgress, this,
            &OnlineToolsDialog::OnDownloadProgress);
    connect(m_current_reply, &QNetworkReply::finished, this,
            &OnlineToolsDialog::OnDownloadFinished);
}

void OnlineToolsDialog::OnDownloadReadyRead() {
    if (!m_current_reply || !m_file) {
        return;
    }

    if (!m_file->isOpen()) {
        const auto status_code =
            m_current_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // If HTTP 206 Partial Content was returned, resume append; otherwise truncate
        const auto open_mode = (status_code == 206)
                                   ? (QIODevice::WriteOnly | QIODevice::Append)
                                   : (QIODevice::WriteOnly | QIODevice::Truncate);

        if (status_code != 206) {
            m_downloaded_offset = 0;
        }

        if (!m_file->open(open_mode)) {
            LOG_ERROR(Frontend, "Failed to open temporary file {} for writing",
                      m_temp_file_path.toStdString());
            CancelOperation();
            QMessageBox::critical(this, tr("Ошибка записи"),
                                  tr("Не удалось открыть временный файл для записи."));
            return;
        }
    }

    m_file->write(m_current_reply->readAll());
}

void OnlineToolsDialog::OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    const qint64 total_current = m_downloaded_offset + bytesReceived;
    const qint64 total_expected =
        (bytesTotal > 0) ? (m_downloaded_offset + bytesTotal) : m_selected_asset.size;

    // Measure speed every 400ms
    const qint64 elapsed = m_speed_timer.elapsed();
    if (elapsed - m_last_time_measured >= 400) {
        const qint64 bytes_diff = total_current - m_last_bytes_measured;
        const double time_sec = (elapsed - m_last_time_measured) / 1000.0;
        if (time_sec > 0.0) {
            m_current_speed_mbps = (bytes_diff / (1024.0 * 1024.0)) / time_sec;
        }
        m_last_bytes_measured = total_current;
        m_last_time_measured = elapsed;
    }

    if (total_expected > 0) {
        const int percent = static_cast<int>((total_current * 100) / total_expected);
        m_progress_bar->setValue((std::min)(100, percent));

        QString speed_str = (m_current_speed_mbps > 0.0)
                                ? QStringLiteral(" • %1 МБ/с").arg(QString::number(m_current_speed_mbps, 'f', 1))
                                : QString();

        QString eta_str;
        if (m_current_speed_mbps > 0.05 && total_current < total_expected) {
            const double remaining_mb = (total_expected - total_current) / (1024.0 * 1024.0);
            const int eta_sec = static_cast<int>(remaining_mb / m_current_speed_mbps);
            if (eta_sec < 60) {
                eta_str = QStringLiteral(" • ~%1 сек.").arg(eta_sec);
            } else {
                eta_str = QStringLiteral(" • ~%1 мин. %2 сек.").arg(eta_sec / 60).arg(eta_sec % 60);
            }
        }

        m_status_label->setText(
            tr("Загрузка: %1 из %2 (%3%)%4%5")
                .arg(FormatBytes(total_current), FormatBytes(total_expected),
                     QString::number(percent), speed_str, eta_str));
    }
}

void OnlineToolsDialog::OnDownloadFinished() {
    if (!m_current_reply) {
        return;
    }

    const auto error = m_current_reply->error();
    m_current_reply->deleteLater();
    m_current_reply = nullptr;

    if (m_file && m_file->isOpen()) {
        m_file->close();
    }

    if (error != QNetworkReply::NoError) {
        m_is_downloading = false;
        m_btn_install->setEnabled(true);
        m_btn_refresh->setEnabled(true);
        m_list_widget->setEnabled(true);
        m_status_label->setText(tr("Ошибка загрузки. Попробуйте снова или выберите другую версию."));
        return;
    }

    m_is_downloading = false;
    m_is_installing = true;
    m_progress_bar->setValue(100);
    m_status_label->setText(tr("Распаковка и установка компонентов..."));
    QApplication::processEvents();

    ProcessInstallation();
}

void OnlineToolsDialog::ProcessInstallation() {
    namespace fs = std::filesystem;

    if (m_type == OnlineToolType::Firmware) {
        const QString qCacheDir = QtCommon::Content::UnzipFirmwareToTmp(m_temp_file_path);

        if (!qCacheDir.isEmpty()) {
            QtCommon::Content::InstallFirmware(qCacheDir, true);

            std::error_code ec;
            fs::remove_all(fs::temp_directory_path() / "storm_eden" / "firmware", ec);
            fs::remove(m_temp_file_path.toStdString(), ec);

            m_system.GetFileSystemController().CreateFactories(*QtCommon::vfs);
            emit InstallationCompleted(true, tr("Прошивка успешно установлена в System NAND!"));
        } else {
            std::error_code ec;
            fs::remove(m_temp_file_path.toStdString(), ec);
            emit InstallationCompleted(false, tr("Ошибка распаковки архива прошивки."));
        }
    } else {
        const bool ok = QtCommon::Content::InstallKeysFromZip(m_temp_file_path);

        std::error_code ec;
        fs::remove(m_temp_file_path.toStdString(), ec);

        if (ok) {
            emit InstallationCompleted(true, tr("Ключи дешифрования успешно установлены!"));
        } else {
            emit InstallationCompleted(false, tr("Не удалось обнаружить или скопировать ключи prod.keys."));
        }
    }
}

void OnlineToolsDialog::HandleInstallationCompleted(bool success, const QString& message) {
    m_is_installing = false;
    m_list_widget->setEnabled(true);
    m_btn_refresh->setEnabled(true);
    m_btn_install->setEnabled(true);

    if (success) {
        m_status_label->setText(QStringLiteral("✅ ") + message);
        QtCommon::Frontend::Information(this, tr("Установка завершена"), message);
        accept();
    } else {
        m_status_label->setText(QStringLiteral("❌ ") + message);
        QtCommon::Frontend::Critical(this, tr("Ошибка установки"), message);
    }
}

void OnlineToolsDialog::CancelOperation() {
    if (m_is_downloading) {
        if (m_current_reply) {
            m_current_reply->abort();
            m_current_reply->deleteLater();
            m_current_reply = nullptr;
        }
        if (m_file && m_file->isOpen()) {
            m_file->close();
        }
        m_is_downloading = false;
        m_btn_install->setEnabled(true);
        m_btn_refresh->setEnabled(true);
        m_list_widget->setEnabled(true);
        m_status_label->setText(tr("Загрузка отменена."));
        return;
    }
    reject();
}

QString OnlineToolsDialog::FormatBytes(qint64 bytes) {
    if (bytes < 1024) {
        return QStringLiteral("%1 Б").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 КБ").arg(QString::number(bytes / 1024.0, 'f', 1));
    } else if (bytes < 1024LL * 1024 * 1024) {
        return QStringLiteral("%1 МБ").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
    } else {
        return QStringLiteral("%1 ГБ").arg(QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2));
    }
}

int OnlineToolsDialog::CompareVersions(const QString& va, const QString& vb) {
    auto parse_ver = [](QString str) -> std::tuple<int, int, int, int, int> {
        if (str.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            str.chop(4);
        }
        const bool is_rebootless = str.contains(QStringLiteral("Rebootless"), Qt::CaseInsensitive);

        QString clean;
        for (const QChar c : str) {
            if (c.isDigit() || c == QLatin1Char('.')) {
                clean.append(c);
            } else {
                clean.append(QLatin1Char('.'));
            }
        }
        const auto parts = clean.split(QLatin1Char('.'), Qt::SkipEmptyParts);
        const int p0 = parts.size() > 0 ? parts[0].toInt() : 0;
        const int p1 = parts.size() > 1 ? parts[1].toInt() : 0;
        const int p2 = parts.size() > 2 ? parts[2].toInt() : 0;
        const int p3 = parts.size() > 3 ? parts[3].toInt() : 0;
        const int p4 = is_rebootless ? 1 : 0;
        return {p0, p1, p2, p3, p4};
    };

    const auto ta = parse_ver(va);
    const auto tb = parse_ver(vb);
    if (ta > tb) {
        return 1;
    }
    if (ta < tb) {
        return -1;
    }
    return 0;
}
