// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QStringList>
#include "common/net/net.h"

class QFrame;
class QLabel;
class QMouseEvent;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

struct StormUpdateInfo {
    QString current_version;
    QString new_version;
    QString release_name;
    QString release_notes;
    QString source_mirror;
    QString published_date;
    qint64 package_size_bytes{0};
    QStringList download_urls;
    QString release_page_url;
};

class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Checking,
        UpdateAvailable,
        UpToDate,
        Error,
    };

    explicit UpdateDialog(QWidget* parent = nullptr);
    explicit UpdateDialog(const StormUpdateInfo& info, QWidget* parent = nullptr);
    explicit UpdateDialog(const Common::Net::Release& release, QWidget* parent = nullptr);
    ~UpdateDialog() override;

    void SetChecking();
    void SetUpdateAvailable(const StormUpdateInfo& info);
    void SetUpToDate(const QString& current_version, const QString& source_mirror = {});
    void SetError(const QString& error_message, const QString& release_page_url = {});

    static QString GetSkippedVersion();
    static void SetSkippedVersion(const QString& version);
    static void ClearSkippedVersion();

    static QString CleanVersion(const QString& raw);
    static QString FormatIsoDateToRu(const QString& iso_date);
    static QString FormatEpochToRu(qint64 epoch_seconds);
    static QString FormatBytes(qint64 bytes);

signals:
    void RetryRequested();
    void InstallReadyToQuit();

protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void StartDownload();
    void CancelDownload();

private:
    void BuildUi();
    void ApplyThemeAndFont();
    void ActivatePage(int index);
    void CenterOverParent();
    void PopulateReleaseNotes(const QString& markdown);
    void SetDownloadUiActive(bool active);
    void UpdateHeaderAppearance(Mode mode);
    void DownloadFromMirrorIndex(int mirror_index);
    void FinalizeDownloadedPackage(const QString& temp_dir, const QString& package_path,
                                   const QString& source_url, int mirror_index);
    void AbortActiveDownload();

    Mode m_mode{Mode::Checking};
    StormUpdateInfo m_info{};

    // Window dragging state (frameless header drag)
    bool m_dragging{false};
    QPoint m_drag_offset{};

    // Core card & header widgets
    QFrame* m_card{nullptr};
    QWidget* m_header_widget{nullptr};
    QWidget* m_header_badge{nullptr};
    QLabel* m_title_label{nullptr};
    QLabel* m_subtitle_label{nullptr};
    QPushButton* m_close_btn{nullptr};
    QFrame* m_header_divider{nullptr};

    QStackedWidget* m_stack{nullptr};

    // Page 0: Checking
    QWidget* m_checking_spinner{nullptr};
    QLabel* m_checking_label{nullptr};

    // Page 1: UpdateAvailable
    QFrame* m_version_card{nullptr};
    QLabel* m_cur_ver_title{nullptr};
    QLabel* m_cur_ver_value{nullptr};
    QWidget* m_arrow_badge{nullptr};
    QFrame* m_new_ver_pill{nullptr};
    QLabel* m_new_ver_title{nullptr};
    QLabel* m_new_ver_value{nullptr};

    QWidget* m_chips_row{nullptr};
    QLabel* m_mirror_chip{nullptr};
    QLabel* m_size_chip{nullptr};
    QLabel* m_date_chip{nullptr};

    QLabel* m_notes_header{nullptr};
    QFrame* m_notes_frame{nullptr};
    QScrollArea* m_notes_scroll{nullptr};
    QWidget* m_notes_container{nullptr};
    QVBoxLayout* m_notes_layout{nullptr};

    QFrame* m_progress_card{nullptr};
    QLabel* m_progress_status{nullptr};
    QProgressBar* m_progress_bar{nullptr};
    QLabel* m_progress_bytes{nullptr};
    QLabel* m_progress_speed{nullptr};
    QLabel* m_progress_percent{nullptr};
    QPushButton* m_cancel_dl_btn{nullptr};

    QWidget* m_actions_container{nullptr};
    QPushButton* m_update_btn{nullptr};
    QPushButton* m_later_btn{nullptr};
    QPushButton* m_skip_btn{nullptr};
    QPushButton* m_release_link_btn{nullptr};

    // Page 2: UpToDate
    QLabel* m_uptodate_desc{nullptr};
    QLabel* m_uptodate_badge{nullptr};
    QLabel* m_uptodate_mirror{nullptr};
    QPushButton* m_uptodate_ok_btn{nullptr};

    // Page 3: Error
    QFrame* m_error_card{nullptr};
    QLabel* m_error_label{nullptr};
    QPushButton* m_error_retry_btn{nullptr};
    QPushButton* m_error_ok_btn{nullptr};
    QPushButton* m_error_link_btn{nullptr};

    // Download tracking
    QNetworkAccessManager* m_nam{nullptr};
    QPointer<QNetworkReply> m_active_reply;
    bool m_download_cancelled{false};
    bool m_installing{false};
};
