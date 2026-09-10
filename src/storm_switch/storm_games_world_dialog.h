// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;

struct StormWorldGame {
    int id{0};
    QString title;
    QString final_title;
    QString version;
    QString serial_id;
    QString size;
    qint64 file_size_bytes{0};
    QString cover_url;
    QString description;
    QStringList regions;
    QStringList text_langs;
    bool file_exists{false};
    bool has_file{false};
    QString real_extension{QStringLiteral(".nsp")};
};

class StormGamesWorldDialog : public QDialog {
    Q_OBJECT

public:
    explicit StormGamesWorldDialog(QWidget* parent = nullptr);
    ~StormGamesWorldDialog() override;

signals:
    void GameDownloaded(const QString& file_path);

private slots:
    void OnFetchCatalog();
    void OnCatalogReplyFinished();
    void OnGameDetailsReplyFinished();
    void OnHeadReplyFinished();
    void OnGameSelectionChanged();
    void OnSearchFilterChanged(const QString& query);
    void OnBrowseDownloadFolder();
    void OnStartDownload();
    void OnCancelDownload();
    void OnDownloadDataReady();
    void OnDownloadProgress(qint64 received, qint64 total);
    void OnDownloadReplyFinished();
    void OnOpenDownloadFolder();

private:
    void SetupUI();
    void ApplyStormStyles();
    void PopulateGameList(const QString& filter = QString());
    void DisplayGameDetails(const StormWorldGame& game);
    void FetchGameDetails(int game_id);
    void FetchRealExtension(int game_id);
    void LoadCover(const StormWorldGame& game);
    void TryNextCoverCandidate();
    bool IsGameDownloaded(const StormWorldGame& game, const QString& dir_path) const;
    QString GetDefaultDownloadDir() const;
    void SaveDownloadDir(const QString& dir);

    QNetworkAccessManager network_mgr;
    QNetworkReply* catalog_reply{nullptr};
    QNetworkReply* details_reply{nullptr};
    QNetworkReply* head_reply{nullptr};
    QNetworkReply* download_reply{nullptr};
    QNetworkReply* cover_reply{nullptr};

    std::vector<StormWorldGame> all_games;
    std::vector<StormWorldGame> filtered_games;
    int selected_game_index{-1};
    int current_download_game_id{-1};

    // Cover resolution
    QStringList current_cover_candidates;
    int current_cover_candidate_index{0};
    QString current_cover_cache_file;

    // Download state
    std::unique_ptr<QFile> output_file;
    QString current_download_path;
    QElapsedTimer download_timer;
    qint64 last_received_bytes{0};
    qint64 last_speed_time{0};
    double current_speed_mbps{0.0};
    bool is_downloading{false};

    // UI Widgets
    QLineEdit* search_edit{nullptr};
    QPushButton* refresh_btn{nullptr};
    QTreeWidget* games_tree{nullptr};
    QLabel* status_label{nullptr};

    // Directory Selector
    QLineEdit* download_dir_edit{nullptr};
    QPushButton* browse_dir_btn{nullptr};

    // Details Panel
    QLabel* cover_label{nullptr};
    QLabel* title_label{nullptr};
    QLabel* tid_label{nullptr};
    QLabel* version_badge{nullptr};
    QLabel* internal_version_badge{nullptr};
    QLabel* size_badge{nullptr};
    QLabel* lang_badge{nullptr};
    QComboBox* version_combo{nullptr};
    QTextBrowser* description_browser{nullptr};

    // Download Controls
    QProgressBar* progress_bar{nullptr};
    QLabel* download_status_label{nullptr};
    QPushButton* download_btn{nullptr};
    QPushButton* cancel_btn{nullptr};
    QPushButton* open_folder_btn{nullptr};
};
