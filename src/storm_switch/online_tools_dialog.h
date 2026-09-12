// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <QDialog>
#include <QElapsedTimer>
#include <QList>
#include <QString>

class QLabel;
class QPushButton;
class QProgressBar;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;
class QFile;

namespace Core {
class System;
}

enum class OnlineToolType {
    Firmware,
    Keys,
};

struct ReleaseAsset {
    QString version;
    QString name;
    qint64 size{0};
    QString download_url;
    QString display_title;
    QString display_size;
    bool is_recommended{false};
};

class OnlineToolsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OnlineToolsDialog(QWidget* parent, Core::System& system, OnlineToolType type);
    ~OnlineToolsDialog() override;

signals:
    void InstallationCompleted(bool success, const QString& message);

private slots:
    void RefreshCatalog();
    void StartDownload();
    void CancelOperation();
    void OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void OnDownloadReadyRead();
    void OnDownloadFinished();
    void OnItemSelectionChanged();
    void HandleInstallationCompleted(bool success, const QString& message);

private:
    void SetupUI();
    void PopulateFallbackCatalog();
    void UpdateListView();
    void ProcessInstallation();
    static QString FormatBytes(qint64 bytes);
    static int CompareVersions(const QString& a, const QString& b);

    Core::System& m_system;
    OnlineToolType m_type;
    std::vector<ReleaseAsset> m_assets;
    ReleaseAsset m_selected_asset;

    // UI elements
    QLabel* m_header_icon{nullptr};
    QLabel* m_header_title{nullptr};
    QLabel* m_header_desc{nullptr};
    QListWidget* m_list_widget{nullptr};
    QProgressBar* m_progress_bar{nullptr};
    QLabel* m_status_label{nullptr};
    QPushButton* m_btn_refresh{nullptr};
    QPushButton* m_btn_cancel{nullptr};
    QPushButton* m_btn_install{nullptr};

    // Networking & download state
    QNetworkAccessManager* m_nam{nullptr};
    QNetworkReply* m_current_reply{nullptr};
    std::unique_ptr<QFile> m_file;
    QString m_temp_file_path;
    qint64 m_downloaded_offset{0};
    qint64 m_total_bytes{0};
    qint64 m_current_downloaded{0};
    QElapsedTimer m_speed_timer;
    qint64 m_last_bytes_measured{0};
    qint64 m_last_time_measured{0};
    double m_current_speed_mbps{0.0};
    bool m_is_downloading{false};
    bool m_is_installing{false};
};
