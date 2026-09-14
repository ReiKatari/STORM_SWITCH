// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <filesystem>
#include "common/common_types.h"
#include <QDateTime>
#include <QDialog>
#include <QHostAddress>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;

enum class StormSaveSyncStatus {
    Synchronized,
    Conflict,
    LocalOnly,
    RemoteOnly,
    Unknown
};

enum class StormConflictAction {
    Cancel,
    ReplaceCurrent,
    ReplaceRemote,
    KeepBoth
};

struct StormSaveItem {
    QString title_id;
    QString title_name;

    qint64 local_timestamp{0};
    QString local_date_str;
    qint64 local_size_bytes{0};
    int local_file_count{0};
    bool has_local{false};

    qint64 remote_timestamp{0};
    QString remote_date_str;
    qint64 remote_size_bytes{0};
    int remote_file_count{0};
    bool has_remote{false};

    StormSaveSyncStatus status{StormSaveSyncStatus::Unknown};
};

class StormSaveConflictDialog : public QDialog {
    Q_OBJECT

public:
    explicit StormSaveConflictDialog(QWidget* parent, const StormSaveItem& item,
                                    const QString& local_device_name,
                                    const QString& remote_device_name);
    StormConflictAction GetAction() const { return m_action; }

private:
    StormConflictAction m_action{StormConflictAction::Cancel};
};

class StormSaveSyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit StormSaveSyncDialog(QWidget* parent = nullptr, u64 target_program_id = 0);
    ~StormSaveSyncDialog() override;

    static QString GenerateConnectionKey(const QHostAddress& ip, quint16 port);
    static bool ParseConnectionKey(const QString& key, QString& out_ip, quint16& out_port);
    static QString FormatDateTime(const QDateTime& dt);
    static QString FormatSize(qint64 bytes);

private slots:
    void OnNewTcpConnection();
    void OnTcpSocketReadyRead();
    void OnTcpSocketDisconnected();
    void OnUdpSocketReadyRead();
    void OnSearchDevicesClicked();
    void OnConnectClicked();
    void OnCopyKeyClicked();
    void OnSyncAllClicked();
    void OnRefreshClicked();
    void OnDiscoveredDeviceSelected(int index);
    void OnTableItemDoubleClicked(int row, int column);

private:
    void SetupUI();
    void ApplyStormStyles();
    void StartHostServer();
    void StopHostServer();
    void BroadcastDiscovery();
    void ScanLocalSaves();
    void FetchRemoteSaves();
    void UpdateComparisonList();
    void PopulateTable();
    void SyncItem(StormSaveItem& item);
    void SyncItemWithAction(StormSaveItem& item, StormConflictAction action);
    void DownloadRemoteSave(const QString& title_id, std::function<void(bool)> on_complete = nullptr);
    void UploadLocalSave(const QString& title_id, std::function<void(bool)> on_complete = nullptr);
    void BackupLocalSave(const QString& title_id);
    void BackupRemoteSave(const QString& title_id, std::function<void(bool)> on_complete = nullptr);

    // Helpers
    std::filesystem::path GetLocalSaveRootDir() const;
    std::filesystem::path GetLocalSaveDirForTitle(const QString& title_id) const;
    std::filesystem::path GetActiveUserSaveDir() const;

    u64 m_target_program_id{0};
    QTcpServer* m_tcp_server{nullptr};
    QUdpSocket* m_udp_socket{nullptr};
    QNetworkAccessManager* m_network_mgr{nullptr};

    QString m_local_key;
    QString m_local_ip;
    quint16 m_local_port{28443};
    QString m_connected_remote_ip;
    quint16 m_connected_remote_port{28443};
    QString m_remote_device_name;
    bool m_is_connected{false};

    QMap<QString, StormSaveItem> m_items;
    QMap<QString, QString> m_discovered_devices;

    // UI elements
    QLabel* m_local_key_label{nullptr};
    QLabel* m_host_status_label{nullptr};
    QLineEdit* m_remote_key_edit{nullptr};
    QPushButton* m_connect_btn{nullptr};
    QPushButton* m_search_btn{nullptr};
    QComboBox* m_discovered_combo{nullptr};
    QLabel* m_connection_status_label{nullptr};
    QTableWidget* m_saves_table{nullptr};
    QPushButton* m_sync_all_btn{nullptr};
    QPushButton* m_refresh_btn{nullptr};
    QPushButton* m_close_btn{nullptr};
    QProgressBar* m_progress_bar{nullptr};
    QLabel* m_status_label{nullptr};
};
