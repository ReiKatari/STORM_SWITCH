// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_save_sync_dialog.h"

#include <chrono>
#include <fstream>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "core/core.h"
#include "qt_common/titledb.h"
#include "qt_common/util/compress.h"

// ---------------------------------------------------------------------------
// StormSaveConflictDialog implementation
// ---------------------------------------------------------------------------

StormSaveConflictDialog::StormSaveConflictDialog(QWidget* parent, const StormSaveItem& item,
                                                 const QString& local_device_name,
                                                 const QString& remote_device_name)
    : QDialog(parent) {
    setWindowTitle(tr("STORM SAVE SYNC вЂ” Р Р°Р·СЂРµС€РµРЅРёРµ РєРѕРЅС„Р»РёРєС‚Р° СЃРѕС…СЂР°РЅРµРЅРёР№"));
    resize(720, 420);
    setMinimumSize(640, 360);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: #0A0E17;"
        "    color: #FFFFFF;"
        "    font-family: 'Segoe UI', sans-serif;"
        "}"
        "QFrame#CardLocal, QFrame#CardRemote {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #141C2B, stop:1 #0D131F);"
        "    border: 1px solid #1F2D42;"
        "    border-radius: 8px;"
        "    padding: 12px;"
        "}"
        "QLabel#CardTitle {"
        "    color: #00F0FF;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "}"
        "QLabel#CardDetail {"
        "    color: #E2E8F0;"
        "    font-size: 12px;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1A2434, stop:1 #131A26);"
        "    border: 1px solid #2A3B52;"
        "    border-radius: 6px;"
        "    color: #E0E8F0;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #25344C, stop:1 #1A2638);"
        "    border-color: #00D2FF;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background: #00D2FF;"
        "    color: #000000;"
        "}"
        "QPushButton#HeroBtn {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0099CC, stop:1 #006699);"
        "    border: 1px solid #00D2FF;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton#HeroBtn:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00BFFF, stop:1 #0088CC);"
        "}"
    ));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(14);
    main_layout->setContentsMargins(18, 18, 18, 18);

    auto* header_lbl = new QLabel(tr("вљЎ РћР±РЅР°СЂСѓР¶РµРЅ РєРѕРЅС„Р»РёРєС‚ РІРµСЂСЃРёР№ СЃРѕС…СЂР°РЅРµРЅРёСЏ"), this);
    header_lbl->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: #00F0FF;"));
    main_layout->addWidget(header_lbl);

    auto* desc_lbl = new QLabel(
        tr("Р”Р»СЏ РёРіСЂС‹ %1 [%2] РѕР±РЅР°СЂСѓР¶РµРЅС‹ СЂР°Р·РЅС‹Рµ РІРµСЂСЃРёРё СЃРѕС…СЂР°РЅРµРЅРёСЏ РЅР° С‚РµРєСѓС‰РµРј Рё СѓРґР°Р»С‘РЅРЅРѕРј СѓСЃС‚СЂРѕР№СЃС‚РІР°С….\n"
           "РЎСЂР°РІРЅРёС‚Рµ РґР°РЅРЅС‹Рµ СЃРѕР·РґР°РЅРёСЏ Рё РІС‹Р±РµСЂРёС‚Рµ РґРµР№СЃС‚РІРёРµ.")
            .arg(item.title_name.isEmpty() ? item.title_id : item.title_name, item.title_id),
        this);
    desc_lbl->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 12px;"));
    desc_lbl->setWordWrap(true);
    main_layout->addWidget(desc_lbl);

    auto* cards_layout = new QHBoxLayout();
    cards_layout->setSpacing(14);

    // Local Card
    auto* local_card = new QFrame(this);
    local_card->setObjectName(QStringLiteral("CardLocal"));
    auto* local_layout = new QVBoxLayout(local_card);
    local_layout->setSpacing(8);

    auto* local_title = new QLabel(
        tr("РўРµРєСѓС‰РµРµ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ (%1)").arg(local_device_name.isEmpty() ? tr("РџРљ") : local_device_name),
        local_card);
    local_title->setObjectName(QStringLiteral("CardTitle"));
    local_layout->addWidget(local_title);

    auto* local_date = new QLabel(
        tr("рџ“… Р”Р°С‚Р° Рё РІСЂРµРјСЏ: %1").arg(item.local_date_str.isEmpty() ? tr("РќРµС‚ РґР°РЅРЅС‹С…") : item.local_date_str),
        local_card);
    local_date->setObjectName(QStringLiteral("CardDetail"));
    local_layout->addWidget(local_date);

    auto* local_size = new QLabel(
        tr("рџ“¦ Р Р°Р·РјРµСЂ РґР°РЅРЅС‹С…: %1").arg(StormSaveSyncDialog::FormatSize(item.local_size_bytes)),
        local_card);
    local_size->setObjectName(QStringLiteral("CardDetail"));
    local_layout->addWidget(local_size);

    auto* local_files = new QLabel(
        tr("рџ“„ Р¤Р°Р№Р»РѕРІ: %1").arg(item.local_file_count),
        local_card);
    local_files->setObjectName(QStringLiteral("CardDetail"));
    local_layout->addWidget(local_files);
    local_layout->addStretch();
    cards_layout->addWidget(local_card);

    // Remote Card
    auto* remote_card = new QFrame(this);
    remote_card->setObjectName(QStringLiteral("CardRemote"));
    auto* remote_layout = new QVBoxLayout(remote_card);
    remote_layout->setSpacing(8);

    auto* remote_title = new QLabel(
        tr("РЈРґР°Р»С‘РЅРЅРѕРµ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ (%1)").arg(remote_device_name.isEmpty() ? tr("РўРµР»РµС„РѕРЅ / РЈР·РµР»") : remote_device_name),
        remote_card);
    remote_title->setObjectName(QStringLiteral("CardTitle"));
    remote_layout->addWidget(remote_title);

    auto* remote_date = new QLabel(
        tr("рџ“… Р”Р°С‚Р° Рё РІСЂРµРјСЏ: %1").arg(item.remote_date_str.isEmpty() ? tr("РќРµС‚ РґР°РЅРЅС‹С…") : item.remote_date_str),
        remote_card);
    remote_date->setObjectName(QStringLiteral("CardDetail"));
    remote_layout->addWidget(remote_date);

    auto* remote_size = new QLabel(
        tr("рџ“¦ Р Р°Р·РјРµСЂ РґР°РЅРЅС‹С…: %1").arg(StormSaveSyncDialog::FormatSize(item.remote_size_bytes)),
        remote_card);
    remote_size->setObjectName(QStringLiteral("CardDetail"));
    remote_layout->addWidget(remote_size);

    auto* remote_files = new QLabel(
        tr("рџ“„ Р¤Р°Р№Р»РѕРІ: %1").arg(item.remote_file_count),
        remote_card);
    remote_files->setObjectName(QStringLiteral("CardDetail"));
    remote_layout->addWidget(remote_files);
    remote_layout->addStretch();
    cards_layout->addWidget(remote_card);

    main_layout->addLayout(cards_layout);

    // Action buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(10);

    auto* btn_replace_local = new QPushButton(tr("Р—Р°РјРµРЅРёС‚СЊ С‚РµРєСѓС‰РµРµ"), this);
    btn_replace_local->setToolTip(tr("РЎРєР°С‡Р°С‚СЊ СЃРѕС…СЂР°РЅРµРЅРёРµ СЃ СѓРґР°Р»С‘РЅРЅРѕРіРѕ СѓСЃС‚СЂРѕР№СЃС‚РІР° Рё РїРµСЂРµР·Р°РїРёСЃР°С‚СЊ Р»РѕРєР°Р»СЊРЅРѕРµ"));
    connect(btn_replace_local, &QPushButton::clicked, this, [this]() {
        m_action = StormConflictAction::ReplaceCurrent;
        accept();
    });

    auto* btn_replace_remote = new QPushButton(tr("Р—Р°РјРµРЅРёС‚СЊ РЅР° СѓРґР°Р»С‘РЅРЅРѕРј"), this);
    btn_replace_remote->setToolTip(tr("РћС‚РїСЂР°РІРёС‚СЊ Р»РѕРєР°Р»СЊРЅРѕРµ СЃРѕС…СЂР°РЅРµРЅРёРµ РЅР° СѓРґР°Р»С‘РЅРЅРѕРµ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ"));
    connect(btn_replace_remote, &QPushButton::clicked, this, [this]() {
        m_action = StormConflictAction::ReplaceRemote;
        accept();
    });

    auto* btn_keep_both = new QPushButton(tr("РЎРѕС…СЂР°РЅРёС‚СЊ РѕР±Р° РІР°СЂРёР°РЅС‚Р°"), this);
    btn_keep_both->setObjectName(QStringLiteral("HeroBtn"));
    btn_keep_both->setToolTip(tr("РЎРѕР·РґР°С‚СЊ СЂРµР·РµСЂРІРЅСѓСЋ РєРѕРїРёСЋ СЃРѕ С€С‚Р°РјРїРѕРј РґР°С‚С‹ Рё РІСЂРµРјРµРЅРё РЅР° РѕР±РѕРёС… СѓСЃС‚СЂРѕР№СЃС‚РІР°С…"));
    connect(btn_keep_both, &QPushButton::clicked, this, [this]() {
        m_action = StormConflictAction::KeepBoth;
        accept();
    });

    auto* btn_cancel = new QPushButton(tr("РћС‚РјРµРЅР°"), this);
    connect(btn_cancel, &QPushButton::clicked, this, [this]() {
        m_action = StormConflictAction::Cancel;
        reject();
    });

    // 3D Drop shadows
    auto add_shadow = [](QPushButton* btn, const QColor& color) {
        auto* s = new QGraphicsDropShadowEffect(btn);
        s->setBlurRadius(12);
        s->setColor(color);
        s->setOffset(0, 2);
        btn->setGraphicsEffect(s);
    };

    add_shadow(btn_replace_local, QColor(0, 210, 255, 90));
    add_shadow(btn_replace_remote, QColor(59, 130, 246, 90));
    add_shadow(btn_keep_both, QColor(0, 240, 255, 140));

    btn_layout->addWidget(btn_replace_local);
    btn_layout->addWidget(btn_replace_remote);
    btn_layout->addWidget(btn_keep_both);
    btn_layout->addStretch();
    btn_layout->addWidget(btn_cancel);

    main_layout->addLayout(btn_layout);
}

// ---------------------------------------------------------------------------
// StormSaveSyncDialog implementation
// ---------------------------------------------------------------------------

StormSaveSyncDialog::StormSaveSyncDialog(QWidget* parent, u64 target_program_id)
    : QDialog(parent), m_target_program_id(target_program_id) {
    setWindowTitle(tr("STORM SAVE SYNC вЂ” РљСЂРѕСЃСЃРїР»Р°С‚С„РѕСЂРјРµРЅРЅР°СЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ СЃРѕС…СЂР°РЅРµРЅРёР№"));
    resize(940, 620);
    setMinimumSize(800, 500);

    m_network_mgr = new QNetworkAccessManager(this);

    SetupUI();
    ApplyStormStyles();
    StartHostServer();
    ScanLocalSaves();
    PopulateTable();

    // Auto-discover on start
    QTimer::singleShot(250, this, &StormSaveSyncDialog::BroadcastDiscovery);
}

StormSaveSyncDialog::~StormSaveSyncDialog() {
    StopHostServer();
}

QString StormSaveSyncDialog::FormatDateTime(const QDateTime& dt) {
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return dt.toString(QStringLiteral("dd.MM.yyyy HH:mm"));
}

QString StormSaveSyncDialog::FormatSize(qint64 bytes) {
    if (bytes <= 0) return QStringLiteral("0 Р‘");
    if (bytes < 1024) return QStringLiteral("%1 Р‘").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 РљР‘").arg(QString::number(bytes / 1024.0, 'f', 1));
    return QStringLiteral("%1 РњР‘").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 2));
}

QString StormSaveSyncDialog::GenerateConnectionKey(const QHostAddress& ip, quint16 port) {
    quint32 ipv4 = ip.toIPv4Address();
    return QString::asprintf("STORM-%08X-%04X", ipv4, port);
}

bool StormSaveSyncDialog::ParseConnectionKey(const QString& raw_key, QString& out_ip, quint16& out_port) {
    const QString key = raw_key.trimmed();
    if (key.isEmpty()) return false;

    static const QRegularExpression storm_pattern(
        QStringLiteral(R"(^STORM-([0-9A-Fa-f]{8})-([0-9A-Fa-f]{4})$)"),
        QRegularExpression::CaseInsensitiveOption);
    auto match = storm_pattern.match(key);
    if (match.hasMatch()) {
        bool ok_ip = false;
        bool ok_port = false;
        quint32 ipv4 = match.captured(1).toUInt(&ok_ip, 16);
        quint16 p = match.captured(2).toUShort(&ok_port, 16);
        if (ok_ip && ok_port) {
            out_ip = QHostAddress(ipv4).toString();
            out_port = p;
            return true;
        }
    }

    if (key.contains(QLatin1Char(':'))) {
        const QStringList parts = key.split(QLatin1Char(':'));
        if (parts.size() == 2) {
            out_ip = parts[0].trimmed();
            bool ok = false;
            quint16 p = parts[1].trimmed().toUShort(&ok);
            if (ok && p > 0) {
                out_port = p;
                return true;
            }
        }
    }

    QHostAddress addr(key);
    if (!addr.isNull()) {
        out_ip = key;
        out_port = 28443;
        return true;
    }

    return false;
}

std::filesystem::path StormSaveSyncDialog::GetLocalSaveRootDir() const {
    const auto nand_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::NANDDir);
    return nand_dir / "user" / "save" / "0000000000000000";
}

std::filesystem::path StormSaveSyncDialog::GetActiveUserSaveDir() const {
    const auto root = GetLocalSaveRootDir();
    std::error_code ec;
    if (std::filesystem::exists(root, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (entry.is_directory()) {
                return entry.path();
            }
        }
    }
    const auto default_user_dir = root / "00000000000000010000000000000000";
    std::filesystem::create_directories(default_user_dir, ec);
    return default_user_dir;
}

std::filesystem::path StormSaveSyncDialog::GetLocalSaveDirForTitle(const QString& title_id) const {
    const auto root = GetLocalSaveRootDir();
    std::error_code ec;
    if (std::filesystem::exists(root, ec)) {
        for (const auto& user_entry : std::filesystem::directory_iterator(root, ec)) {
            if (user_entry.is_directory()) {
                const auto title_dir = user_entry.path() / title_id.toStdString();
                if (std::filesystem::exists(title_dir, ec)) {
                    return title_dir;
                }
            }
        }
    }
    return GetActiveUserSaveDir() / title_id.toStdString();
}

void StormSaveSyncDialog::SetupUI() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(12);
    main_layout->setContentsMargins(16, 16, 16, 16);

    // Header Card
    auto* header_card = new QFrame(this);
    header_card->setObjectName(QStringLiteral("HeaderCard"));
    auto* header_layout = new QHBoxLayout(header_card);
    header_layout->setContentsMargins(14, 12, 14, 12);

    auto* title_box = new QVBoxLayout();
    title_box->setSpacing(4);
    auto* title_lbl = new QLabel(tr("STORM SAVE SYNC"), header_card);
    title_lbl->setObjectName(QStringLiteral("AppTitle"));
    title_box->addWidget(title_lbl);

    auto* sub_lbl = new QLabel(
        tr("РљСЂРѕСЃСЃРїР»Р°С‚С„РѕСЂРјРµРЅРЅР°СЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ СЃРѕС…СЂР°РЅРµРЅРёР№ Switch РјРµР¶РґСѓ РџРљ Рё Android РїРѕ Р·Р°С‰РёС‰С‘РЅРЅРѕРјСѓ РєР»СЋС‡Сѓ"),
        header_card);
    sub_lbl->setObjectName(QStringLiteral("AppSubtitle"));
    title_box->addWidget(sub_lbl);
    header_layout->addLayout(title_box);
    header_layout->addStretch();

    // Local Key & Host Status box
    auto* key_box = new QVBoxLayout();
    key_box->setAlignment(Qt::AlignRight);
    key_box->setSpacing(4);

    auto* key_row = new QHBoxLayout();
    key_row->setSpacing(8);
    m_local_key_label = new QLabel(tr("РљР»СЋС‡: РёРЅРёС†РёР°Р»РёР·Р°С†РёСЏ..."), header_card);
    m_local_key_label->setStyleSheet(QStringLiteral("font-family: monospace; font-size: 13px; font-weight: bold; color: #00F0FF;"));
    key_row->addWidget(m_local_key_label);

    auto* copy_key_btn = new QPushButton(tr("рџ“‹ РЎРєРѕРїРёСЂРѕРІР°С‚СЊ"), header_card);
    copy_key_btn->setToolTip(tr("РЎРєРѕРїРёСЂРѕРІР°С‚СЊ РєР»СЋС‡ СЌС‚РѕРіРѕ СѓСЃС‚СЂРѕР№СЃС‚РІР° РІ Р±СѓС„РµСЂ РѕР±РјРµРЅР°"));
    connect(copy_key_btn, &QPushButton::clicked, this, &StormSaveSyncDialog::OnCopyKeyClicked);
    key_row->addWidget(copy_key_btn);
    key_box->addLayout(key_row);

    m_host_status_label = new QLabel(tr("рџџў РЎРµСЂРІРµСЂ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё Р°РєС‚РёРІРµРЅ"), header_card);
    m_host_status_label->setStyleSheet(QStringLiteral("font-size: 11px; color: #10B981;"));
    key_box->addWidget(m_host_status_label);

    header_layout->addLayout(key_box);
    main_layout->addWidget(header_card);

    // Connection Bar
    auto* conn_frame = new QFrame(this);
    conn_frame->setObjectName(QStringLiteral("ConnFrame"));
    auto* conn_layout = new QHBoxLayout(conn_frame);
    conn_layout->setContentsMargins(12, 8, 12, 8);
    conn_layout->setSpacing(10);

    auto* enter_lbl = new QLabel(tr("РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє СѓРґР°Р»С‘РЅРЅРѕРјСѓ СѓСЃС‚СЂРѕР№СЃС‚РІСѓ:"), conn_frame);
    enter_lbl->setStyleSheet(QStringLiteral("font-weight: bold; color: #E2E8F0;"));
    conn_layout->addWidget(enter_lbl);

    m_remote_key_edit = new QLineEdit(conn_frame);
    m_remote_key_edit->setPlaceholderText(tr("Р’РІРµРґРёС‚Рµ РєР»СЋС‡ (STORM-XXXXXXXX-YYYY) РёР»Рё IP:РїРѕСЂС‚..."));
    m_remote_key_edit->setMinimumWidth(260);
    conn_layout->addWidget(m_remote_key_edit);

    m_connect_btn = new QPushButton(tr("рџ”— РџРѕРґРєР»СЋС‡РёС‚СЊСЃСЏ"), conn_frame);
    connect(m_connect_btn, &QPushButton::clicked, this, &StormSaveSyncDialog::OnConnectClicked);
    conn_layout->addWidget(m_connect_btn);

    m_search_btn = new QPushButton(tr("рџ”Ќ РџРѕРёСЃРє СѓСЃС‚СЂРѕР№СЃС‚РІ"), conn_frame);
    m_search_btn->setToolTip(tr("РџРѕРёСЃРє СѓСЃС‚СЂРѕР№СЃС‚РІ STORM SWITCH РІ Р»РѕРєР°Р»СЊРЅРѕР№ СЃРµС‚Рё Wi-Fi"));
    connect(m_search_btn, &QPushButton::clicked, this, &StormSaveSyncDialog::OnSearchDevicesClicked);
    conn_layout->addWidget(m_search_btn);

    m_discovered_combo = new QComboBox(conn_frame);
    m_discovered_combo->addItem(tr("РћР±РЅР°СЂСѓР¶РµРЅРЅС‹Рµ СѓСЃС‚СЂРѕР№СЃС‚РІР° РІ СЃРµС‚Рё (0)"));
    m_discovered_combo->setMinimumWidth(220);
    connect(m_discovered_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StormSaveSyncDialog::OnDiscoveredDeviceSelected);
    conn_layout->addWidget(m_discovered_combo);

    main_layout->addWidget(conn_frame);

    // Connection Status Strip
    m_connection_status_label = new QLabel(
        tr("РЎС‚Р°С‚СѓСЃ: РЅРµ РїРѕРґРєР»СЋС‡РµРЅРѕ Рє СѓРґР°Р»С‘РЅРЅРѕРјСѓ СѓСЃС‚СЂРѕР№СЃС‚РІСѓ. Р’РІРµРґРёС‚Рµ РєР»СЋС‡ РёР»Рё РІС‹Р±РµСЂРёС‚Рµ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ РёР· СЃРїРёСЃРєР°."),
        this);
    m_connection_status_label->setStyleSheet(QStringLiteral("font-size: 11px; color: #94A3B8; padding-left: 4px;"));
    main_layout->addWidget(m_connection_status_label);

    // Saves Table
    m_saves_table = new QTableWidget(this);
    m_saves_table->setColumnCount(6);
    m_saves_table->setHorizontalHeaderLabels({
        tr("РРіСЂР°"),
        tr("Title ID"),
        tr("Р›РѕРєР°Р»СЊРЅРѕРµ СЃРѕС…СЂР°РЅРµРЅРёРµ"),
        tr("РЈРґР°Р»С‘РЅРЅРѕРµ СЃРѕС…СЂР°РЅРµРЅРёРµ"),
        tr("РЎС‚Р°С‚СѓСЃ"),
        tr("Р”РµР№СЃС‚РІРёРµ")
    });
    m_saves_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_saves_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_saves_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_saves_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_saves_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_saves_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_saves_table->verticalHeader()->setVisible(false);
    m_saves_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_saves_table->setAlternatingRowColors(true);
    connect(m_saves_table, &QTableWidget::cellDoubleClicked, this, &StormSaveSyncDialog::OnTableItemDoubleClicked);
    main_layout->addWidget(m_saves_table);

    // Progress Bar
    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setTextVisible(true);
    m_progress_bar->setVisible(false);
    main_layout->addWidget(m_progress_bar);

    // Bottom Bar
    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->setSpacing(10);

    m_status_label = new QLabel(tr("Р“РѕС‚РѕРІРѕ"), this);
    m_status_label->setStyleSheet(QStringLiteral("color: #94A3B8; font-size: 11px;"));
    bottom_layout->addWidget(m_status_label);
    bottom_layout->addStretch();

    m_refresh_btn = new QPushButton(tr("рџ”„ РћР±РЅРѕРІРёС‚СЊ"), this);
    connect(m_refresh_btn, &QPushButton::clicked, this, &StormSaveSyncDialog::OnRefreshClicked);
    bottom_layout->addWidget(m_refresh_btn);

    m_sync_all_btn = new QPushButton(tr("вљЎ РЎРёРЅС…СЂРѕРЅРёР·РёСЂРѕРІР°С‚СЊ РІСЃС‘"), this);
    m_sync_all_btn->setStyleSheet(QStringLiteral("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0099CC, stop:1 #006699); border: 1px solid #00D2FF; color: #FFFFFF; font-weight: bold;"));
    connect(m_sync_all_btn, &QPushButton::clicked, this, &StormSaveSyncDialog::OnSyncAllClicked);
    bottom_layout->addWidget(m_sync_all_btn);

    m_close_btn = new QPushButton(tr("Р—Р°РєСЂС‹С‚СЊ"), this);
    connect(m_close_btn, &QPushButton::clicked, this, &QDialog::accept);
    bottom_layout->addWidget(m_close_btn);

    main_layout->addLayout(bottom_layout);
}

void StormSaveSyncDialog::ApplyStormStyles() {
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "    background-color: #0A0E17;"
        "    color: #FFFFFF;"
        "    font-family: 'Segoe UI', sans-serif;"
        "}"
        "QFrame#HeaderCard {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #141C2B, stop:1 #0D131F);"
        "    border: 1px solid #1F2D42;"
        "    border-radius: 8px;"
        "}"
        "QLabel#AppTitle {"
        "    color: #00F0FF;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    letter-spacing: 1px;"
        "}"
        "QLabel#AppSubtitle {"
        "    color: #94A3B8;"
        "    font-size: 12px;"
        "}"
        "QFrame#ConnFrame {"
        "    background-color: #0F1624;"
        "    border: 1px solid #1E293B;"
        "    border-radius: 8px;"
        "}"
        "QLineEdit {"
        "    background: rgba(255, 255, 255, 0.05);"
        "    border: 1px solid #20354E;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "    color: #FFFFFF;"
        "    font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #00D2FF;"
        "    background: rgba(0, 210, 255, 0.08);"
        "}"
        "QComboBox {"
        "    background: rgba(255, 255, 255, 0.05);"
        "    border: 1px solid #20354E;"
        "    border-radius: 6px;"
        "    color: #FFFFFF;"
        "    padding: 5px 10px;"
        "    font-weight: bold;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #141B26;"
        "    color: #FFFFFF;"
        "    selection-background-color: #00D2FF;"
        "    selection-color: #000000;"
        "    border: 1px solid #20354E;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1A2434, stop:1 #131A26);"
        "    border: 1px solid #2A3B52;"
        "    color: #E0E8F0;"
        "    padding: 6px 14px;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #25344C, stop:1 #1A2638);"
        "    border-color: #00D2FF;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background: #00D2FF;"
        "    color: #000000;"
        "}"
        "QTableWidget {"
        "    background-color: #070A10;"
        "    border: 1px solid #1E2838;"
        "    border-radius: 8px;"
        "    color: #E0E8F0;"
        "    alternate-background-color: #0E131E;"
        "    selection-background-color: rgba(0, 210, 255, 0.25);"
        "    selection-color: #FFFFFF;"
        "    gridline-color: #16202F;"
        "}"
        "QTableWidget::item {"
        "    padding: 6px 8px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #121824;"
        "    color: #00F0FF;"
        "    font-weight: bold;"
        "    border: none;"
        "    border-right: 1px solid #20354E;"
        "    border-bottom: 1px solid #20354E;"
        "    padding: 7px 8px;"
        "}"
        "QProgressBar {"
        "    background-color: #121824;"
        "    border: 1px solid #20354E;"
        "    border-radius: 6px;"
        "    text-align: center;"
        "    color: #FFFFFF;"
        "    font-size: 11px;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00D2FF, stop:1 #00F0FF);"
        "    border-radius: 5px;"
        "}"
    ));
}

void StormSaveSyncDialog::StartHostServer() {
    m_tcp_server = new QTcpServer(this);
    connect(m_tcp_server, &QTcpServer::newConnection, this, &StormSaveSyncDialog::OnNewTcpConnection);

    // Determine primary local IPv4
    QHostAddress local_v4 = QHostAddress::LocalHost;
    for (const auto& iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const auto& entry : iface.addressEntries()) {
            const auto ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback()) {
                local_v4 = ip;
                break;
            }
        }
        if (local_v4 != QHostAddress::LocalHost) break;
    }

    m_local_ip = local_v4.toString();
    m_local_port = 28443;

    if (!m_tcp_server->listen(QHostAddress::AnyIPv4, m_local_port)) {
        // Try fallback port
        m_local_port = 28445;
        m_tcp_server->listen(QHostAddress::AnyIPv4, m_local_port);
    }

    m_local_key = GenerateConnectionKey(local_v4, m_local_port);
    m_local_key_label->setText(tr("РљР»СЋС‡: %1 (%2:%3)").arg(m_local_key, m_local_ip).arg(m_local_port));

    // Start UDP broadcast listener
    m_udp_socket = new QUdpSocket(this);
    m_udp_socket->bind(QHostAddress::AnyIPv4, 28444, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_udp_socket, &QUdpSocket::readyRead, this, &StormSaveSyncDialog::OnUdpSocketReadyRead);
}

void StormSaveSyncDialog::StopHostServer() {
    if (m_tcp_server) {
        m_tcp_server->close();
        delete m_tcp_server;
        m_tcp_server = nullptr;
    }
    if (m_udp_socket) {
        m_udp_socket->close();
        delete m_udp_socket;
        m_udp_socket = nullptr;
    }
}

void StormSaveSyncDialog::OnNewTcpConnection() {
    while (m_tcp_server && m_tcp_server->hasPendingConnections()) {
        auto* socket = m_tcp_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &StormSaveSyncDialog::OnTcpSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &StormSaveSyncDialog::OnTcpSocketDisconnected);
    }
}

void StormSaveSyncDialog::OnTcpSocketReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    const QByteArray socket_data = socket->readAll();
    const QString req_str = QString::fromUtf8(socket_data);

    // Parse HTTP request line
    const int first_line_end = req_str.indexOf(QStringLiteral("\r\n"));
    if (first_line_end == -1) return;
    const QString first_line = req_str.left(first_line_end);
    const QStringList parts = first_line.split(QLatin1Char(' '));
    if (parts.size() < 2) return;

    const QString method = parts[0];
    const QString path_and_query = parts[1];
    const QUrl url(QStringLiteral("http://localhost") + path_and_query);
    const QString path = url.path();
    const QUrlQuery query(url);

    auto send_response = [socket](int status_code, const QString& content_type, const QByteArray& body) {
        QByteArray resp;
        resp.append(QString::asprintf("HTTP/1.1 %d OK\r\n", status_code).toUtf8());
        resp.append(QString::asprintf("Content-Type: %s\r\n", content_type.toUtf8().constData()).toUtf8());
        resp.append(QString::asprintf("Content-Length: %lld\r\n", static_cast<long long>(body.size())).toUtf8());
        resp.append("Connection: close\r\n\r\n");
        resp.append(body);
        socket->write(resp);
        socket->flush();
        socket->disconnectFromHost();
    };

    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/status")) {
        QJsonObject obj;
        obj[QStringLiteral("status")] = QStringLiteral("ok");
        obj[QStringLiteral("device_name")] = QHostInfo::localHostName();
        obj[QStringLiteral("platform")] = QStringLiteral("windows");
        obj[QStringLiteral("version")] = QStringLiteral("8.5.0");
        send_response(200, QStringLiteral("application/json"), QJsonDocument(obj).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/saves")) {
        ScanLocalSaves();
        QJsonArray arr;
        for (const auto& item : m_items) {
            if (!item.has_local) continue;
            QJsonObject obj;
            obj[QStringLiteral("title_id")] = item.title_id;
            obj[QStringLiteral("title_name")] = item.title_name;
            obj[QStringLiteral("timestamp")] = item.local_timestamp;
            obj[QStringLiteral("date_str")] = item.local_date_str;
            obj[QStringLiteral("size_bytes")] = item.local_size_bytes;
            obj[QStringLiteral("file_count")] = item.local_file_count;
            arr.append(obj);
        }
        send_response(200, QStringLiteral("application/json"), QJsonDocument(arr).toJson(QJsonDocument::Compact));
        return;
    }

    if (method == QStringLiteral("GET") && path == QStringLiteral("/api/save/download")) {
        const QString title_id = query.queryItemValue(QStringLiteral("title_id"));
        if (title_id.isEmpty()) {
            send_response(400, QStringLiteral("text/plain"), "Missing title_id");
            return;
        }

        const auto save_dir = GetLocalSaveDirForTitle(title_id);
        std::error_code ec;
        if (!std::filesystem::exists(save_dir, ec)) {
            send_response(404, QStringLiteral("text/plain"), "Save not found");
            return;
        }

        const QString temp_zip = QDir::tempPath() + QStringLiteral("/storm_save_dl_%1.zip").arg(title_id);
        QFile::remove(temp_zip);
        const bool zip_ok = QtCommon::Compress::compressDir(temp_zip, QString::fromStdString(save_dir.string()));
        if (!zip_ok) {
            send_response(500, QStringLiteral("text/plain"), "Failed to compress save");
            return;
        }

        QFile zip_file(temp_zip);
        if (zip_file.open(QIODevice::ReadOnly)) {
            const QByteArray zip_data = zip_file.readAll();
            zip_file.close();
            QFile::remove(temp_zip);
            send_response(200, QStringLiteral("application/zip"), zip_data);
        } else {
            send_response(500, QStringLiteral("text/plain"), "Failed to read zip");
        }
        return;
    }

    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/save/upload")) {
        const QString title_id = query.queryItemValue(QStringLiteral("title_id"));
        if (title_id.isEmpty()) {
            send_response(400, QStringLiteral("text/plain"), "Missing title_id");
            return;
        }

        // Find header-body boundary
        const int body_idx = socket_data.indexOf("\r\n\r\n");
        if (body_idx == -1) {
            send_response(400, QStringLiteral("text/plain"), "Malformed body");
            return;
        }
        const QByteArray body_data = socket_data.mid(body_idx + 4);

        const QString temp_zip = QDir::tempPath() + QStringLiteral("/storm_save_ul_%1.zip").arg(title_id);
        QFile::remove(temp_zip);
        QFile zip_file(temp_zip);
        if (zip_file.open(QIODevice::WriteOnly)) {
            zip_file.write(body_data);
            zip_file.close();

            const auto target_dir = GetLocalSaveDirForTitle(title_id);
            std::error_code ec;
            std::filesystem::create_directories(target_dir, ec);

            const auto extracted = QtCommon::Compress::extractDir(temp_zip, QString::fromStdString(target_dir.string()));
            QFile::remove(temp_zip);

            if (!extracted.isEmpty()) {
                ScanLocalSaves();
                PopulateTable();
                send_response(200, QStringLiteral("application/json"), "{\"status\":\"ok\"}");
                return;
            }
        }
        send_response(500, QStringLiteral("text/plain"), "Failed to extract save");
        return;
    }

    if (method == QStringLiteral("POST") && path == QStringLiteral("/api/save/backup")) {
        const QString title_id = query.queryItemValue(QStringLiteral("title_id"));
        if (!title_id.isEmpty()) {
            BackupLocalSave(title_id);
            send_response(200, QStringLiteral("application/json"), "{\"status\":\"ok\"}");
            return;
        }
        send_response(400, QStringLiteral("text/plain"), "Missing title_id");
        return;
    }

    send_response(404, QStringLiteral("text/plain"), "Not Found");
}

void StormSaveSyncDialog::OnTcpSocketDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}

void StormSaveSyncDialog::OnUdpSocketReadyRead() {
    while (m_udp_socket && m_udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(m_udp_socket->pendingDatagramSize()));
        QHostAddress sender_ip;
        quint16 sender_port{0};
        m_udp_socket->readDatagram(datagram.data(), datagram.size(), &sender_ip, &sender_port);

        const QString msg = QString::fromUtf8(datagram).trimmed();
        if (msg == QStringLiteral("STORM_SYNC_DISCOVER")) {
            // Reply with our announcement
            const QString reply = QStringLiteral("STORM_SYNC_ANNOUNCE:%1:%2:Windows")
                                      .arg(m_local_key, QHostInfo::localHostName());
            m_udp_socket->writeDatagram(reply.toUtf8(), sender_ip, sender_port);
        } else if (msg.startsWith(QStringLiteral("STORM_SYNC_ANNOUNCE:"))) {
            const QStringList tokens = msg.split(QLatin1Char(':'));
            if (tokens.size() >= 3) {
                const QString remote_key = tokens[1];
                const QString dev_name = tokens[2];
                const QString platform = tokens.size() >= 4 ? tokens[3] : QStringLiteral("Node");

                const QString label = QStringLiteral("%1 (%2) вЂ” %3").arg(dev_name, platform, remote_key);
                if (!m_discovered_devices.contains(remote_key)) {
                    m_discovered_devices.insert(remote_key, dev_name);
                    m_discovered_combo->addItem(label, remote_key);
                    m_discovered_combo->setItemText(
                        0, tr("РћР±РЅР°СЂСѓР¶РµРЅРЅС‹Рµ СѓСЃС‚СЂРѕР№СЃС‚РІР° РІ СЃРµС‚Рё (%1)").arg(m_discovered_devices.size()));
                }
            }
        }
    }
}

void StormSaveSyncDialog::BroadcastDiscovery() {
    if (!m_udp_socket) return;
    const QByteArray ping = "STORM_SYNC_DISCOVER";
    m_udp_socket->writeDatagram(ping, QHostAddress::Broadcast, 28444);
    m_status_label->setText(tr("Р’С‹РїРѕР»РЅСЏРµС‚СЃСЏ РїРѕРёСЃРє СѓСЃС‚СЂРѕР№СЃС‚РІ РІ Р»РѕРєР°Р»СЊРЅРѕР№ СЃРµС‚Рё..."));
}

void StormSaveSyncDialog::OnSearchDevicesClicked() {
    BroadcastDiscovery();
}

void StormSaveSyncDialog::OnDiscoveredDeviceSelected(int index) {
    if (index <= 0) return;
    const QString key = m_discovered_combo->itemData(index).toString();
    if (!key.isEmpty()) {
        m_remote_key_edit->setText(key);
        OnConnectClicked();
    }
}

void StormSaveSyncDialog::OnCopyKeyClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard) {
        clipboard->setText(m_local_key);
        m_status_label->setText(tr("РљР»СЋС‡ СЃРєРѕРїРёСЂРѕРІР°РЅ РІ Р±СѓС„РµСЂ РѕР±РјРµРЅР°: %1").arg(m_local_key));
    }
}

void StormSaveSyncDialog::OnConnectClicked() {
    const QString input = m_remote_key_edit->text().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, tr("РџРѕРґРєР»СЋС‡РµРЅРёРµ"), tr("РџРѕР¶Р°Р»СѓР№СЃС‚Р°, РІРІРµРґРёС‚Рµ РєР»СЋС‡ РїРѕРґРєР»СЋС‡РµРЅРёСЏ РёР»Рё IP:РїРѕСЂС‚."));
        return;
    }

    QString ip;
    quint16 port = 28443;
    if (!ParseConnectionKey(input, ip, port)) {
        QMessageBox::warning(this, tr("РќРµРІРµСЂРЅС‹Р№ РєР»СЋС‡"),
                             tr("РќРµ СѓРґР°Р»РѕСЃСЊ СЂР°СЃРїРѕР·РЅР°С‚СЊ РєР»СЋС‡ РїРѕРґРєР»СЋС‡РµРЅРёСЏ. РџСЂРѕРІРµСЂСЊС‚Рµ С„РѕСЂРјР°С‚."));
        return;
    }

    m_connected_remote_ip = ip;
    m_connected_remote_port = port;

    m_connection_status_label->setText(
        tr("РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє %1:%2...").arg(m_connected_remote_ip).arg(m_connected_remote_port));

    // Test connectivity via /api/status
    const QUrl url(QStringLiteral("http://%1:%2/api/status").arg(ip).arg(port));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM-SWITCH-SYNC/8.5.0"));

    auto* reply = m_network_mgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                const auto obj = doc.object();
                m_remote_device_name = obj[QStringLiteral("device_name")].toString();
                const QString platform = obj[QStringLiteral("platform")].toString();
                m_is_connected = true;
                m_connection_status_label->setText(
                    tr("рџџў РџРѕРґРєР»СЋС‡РµРЅРѕ: %1 (%2) [%3:%4]")
                        .arg(m_remote_device_name, platform, m_connected_remote_ip)
                        .arg(m_connected_remote_port));
                m_status_label->setText(tr("РЈСЃРїРµС€РЅРѕ РїРѕРґРєР»СЋС‡РµРЅРѕ Рє СѓРґР°Р»С‘РЅРЅРѕРјСѓ СѓР·Р»Сѓ"));
                FetchRemoteSaves();
                return;
            }
        }
        m_is_connected = false;
        m_connection_status_label->setText(
            tr("вќЊ РћС€РёР±РєР° РїРѕРґРєР»СЋС‡РµРЅРёСЏ Рє %1:%2").arg(m_connected_remote_ip).arg(m_connected_remote_port));
        QMessageBox::critical(this, tr("РћС€РёР±РєР° РїРѕРґРєР»СЋС‡РµРЅРёСЏ"),
                              tr("РќРµ СѓРґР°Р»РѕСЃСЊ СЃРѕРµРґРёРЅРёС‚СЊСЃСЏ СЃ СѓРґР°Р»С‘РЅРЅС‹Рј СѓСЃС‚СЂРѕР№СЃС‚РІРѕРј (%1:%2).\n"
                                 "РЈР±РµРґРёС‚РµСЃСЊ, С‡С‚Рѕ РѕР±Р° СѓСЃС‚СЂРѕР№СЃС‚РІР° РЅР°С…РѕРґСЏС‚СЃСЏ РІ РѕРґРЅРѕР№ СЃРµС‚Рё Wi-Fi.")
                                  .arg(m_connected_remote_ip)
                                  .arg(m_connected_remote_port));
    });
}

void StormSaveSyncDialog::ScanLocalSaves() {
    const auto root = GetLocalSaveRootDir();
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return;
    }

    TitleDB::TitleDatabase::Instance().EnsureLoaded();

    for (const auto& user_entry : std::filesystem::directory_iterator(root, ec)) {
        if (!user_entry.is_directory()) continue;

        for (const auto& title_entry : std::filesystem::directory_iterator(user_entry.path(), ec)) {
            if (!title_entry.is_directory()) continue;

            const QString title_id = QString::fromStdString(title_entry.path().filename().string()).toUpper();
            if (title_id.size() != 16) continue;

            qint64 total_size = 0;
            int file_count = 0;
            auto latest_time = std::filesystem::file_time_type::min();

            for (const auto& file : std::filesystem::recursive_directory_iterator(title_entry.path(), ec)) {
                if (file.is_regular_file()) {
                    total_size += static_cast<qint64>(file.file_size());
                    file_count++;
                    const auto wt = file.last_write_time();
                    if (wt > latest_time) {
                        latest_time = wt;
                    }
                }
            }

            if (file_count == 0) continue;

            auto& item = m_items[title_id];
            item.title_id = title_id;
            item.has_local = true;
            item.local_size_bytes = total_size;
            item.local_file_count = file_count;

            // Timestamp conversion
            if (latest_time != std::filesystem::file_time_type::min()) {
                const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    latest_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                const auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
                const QDateTime dt = QDateTime::fromSecsSinceEpoch(time_t_val);
                item.local_timestamp = static_cast<qint64>(time_t_val);
                item.local_date_str = FormatDateTime(dt);
            }

            if (item.title_name.isEmpty()) {
                const auto entry = TitleDB::TitleDatabase::Instance().Lookup(title_id.toStdString());
                if (entry) {
                    item.title_name = QString::fromStdString(entry->name);
                } else {
                    item.title_name = title_id;
                }
            }
        }
    }

    UpdateComparisonList();
}

void StormSaveSyncDialog::FetchRemoteSaves() {
    if (!m_is_connected) return;

    const QUrl url(QStringLiteral("http://%1:%2/api/saves")
                       .arg(m_connected_remote_ip)
                       .arg(m_connected_remote_port));
    QNetworkRequest req(url);
    auto* reply = m_network_mgr->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isArray()) {
                const auto arr = doc.array();

                // Clear previous remote markers
                for (auto& it : m_items) {
                    it.has_remote = false;
                    it.remote_timestamp = 0;
                    it.remote_size_bytes = 0;
                    it.remote_file_count = 0;
                    it.remote_date_str.clear();
                }

                for (const auto& val : arr) {
                    const auto obj = val.toObject();
                    const QString title_id = obj[QStringLiteral("title_id")].toString().toUpper();
                    if (title_id.isEmpty()) continue;

                    auto& item = m_items[title_id];
                    item.title_id = title_id;
                    item.has_remote = true;
                    item.remote_timestamp = obj[QStringLiteral("timestamp")].toInteger();
                    item.remote_date_str = obj[QStringLiteral("date_str")].toString();
                    item.remote_size_bytes = obj[QStringLiteral("size_bytes")].toInteger();
                    item.remote_file_count = obj[QStringLiteral("file_count")].toInt();
                    if (item.title_name.isEmpty() || item.title_name == title_id) {
                        item.title_name = obj[QStringLiteral("title_name")].toString();
                    }
                }

                UpdateComparisonList();
                PopulateTable();
                m_status_label->setText(tr("РЎРїРёСЃРѕРє СѓРґР°Р»С‘РЅРЅС‹С… СЃРѕС…СЂР°РЅРµРЅРёР№ СѓСЃРїРµС€РЅРѕ РѕР±РЅРѕРІР»С‘РЅ"));
            }
        }
    });
}

void StormSaveSyncDialog::UpdateComparisonList() {
    for (auto& item : m_items) {
        if (item.has_local && item.has_remote) {
            const qint64 diff = std::abs(item.local_timestamp - item.remote_timestamp);
            if (diff <= 3 && item.local_size_bytes == item.remote_size_bytes) {
                item.status = StormSaveSyncStatus::Synchronized;
            } else {
                item.status = StormSaveSyncStatus::Conflict;
            }
        } else if (item.has_local) {
            item.status = StormSaveSyncStatus::LocalOnly;
        } else if (item.has_remote) {
            item.status = StormSaveSyncStatus::RemoteOnly;
        } else {
            item.status = StormSaveSyncStatus::Unknown;
        }
    }
}

void StormSaveSyncDialog::PopulateTable() {
    m_saves_table->setRowCount(0);

    int row = 0;
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        const auto& item = it.value();
        m_saves_table->insertRow(row);

        // Col 0: Name
        auto* name_item = new QTableWidgetItem(item.title_name);
        name_item->setData(Qt::UserRole, item.title_id);
        m_saves_table->setItem(row, 0, name_item);

        // Col 1: Title ID
        auto* tid_item = new QTableWidgetItem(item.title_id);
        tid_item->setTextAlignment(Qt::AlignCenter);
        m_saves_table->setItem(row, 1, tid_item);

        // Col 2: Local Info
        const QString local_str = item.has_local
                                      ? QStringLiteral("%1 (%2)").arg(item.local_date_str, FormatSize(item.local_size_bytes))
                                      : tr("РћС‚СЃСѓС‚СЃС‚РІСѓРµС‚");
        auto* local_item = new QTableWidgetItem(local_str);
        local_item->setTextAlignment(Qt::AlignCenter);
        m_saves_table->setItem(row, 2, local_item);

        // Col 3: Remote Info
        const QString remote_str = item.has_remote
                                       ? QStringLiteral("%1 (%2)").arg(item.remote_date_str, FormatSize(item.remote_size_bytes))
                                       : tr("РћС‚СЃСѓС‚СЃС‚РІСѓРµС‚");
        auto* remote_item = new QTableWidgetItem(remote_str);
        remote_item->setTextAlignment(Qt::AlignCenter);
        m_saves_table->setItem(row, 3, remote_item);

        // Col 4: Status
        QString status_text;
        QColor status_color;
        switch (item.status) {
        case StormSaveSyncStatus::Synchronized:
            status_text = tr("рџџў РЎРёРЅС…СЂРѕРЅРёР·РёСЂРѕРІР°РЅРѕ");
            status_color = QColor(16, 185, 129);
            break;
        case StormSaveSyncStatus::Conflict:
            status_text = tr("вљ пёЏ РљРѕРЅС„Р»РёРєС‚ СЃРѕС…СЂР°РЅРµРЅРёР№");
            status_color = QColor(245, 158, 11);
            break;
        case StormSaveSyncStatus::LocalOnly:
            status_text = tr("рџ“¤ РўРѕР»СЊРєРѕ Р»РѕРєР°Р»СЊРЅРѕ");
            status_color = QColor(59, 130, 246);
            break;
        case StormSaveSyncStatus::RemoteOnly:
            status_text = tr("рџ“Ґ РўРѕР»СЊРєРѕ РЅР° СѓРґР°Р»С‘РЅРЅРѕРј");
            status_color = QColor(139, 92, 246);
            break;
        default:
            status_text = tr("РќРµРёР·РІРµСЃС‚РЅРѕ");
            status_color = QColor(148, 163, 184);
            break;
        }
        auto* st_item = new QTableWidgetItem(status_text);
        st_item->setTextAlignment(Qt::AlignCenter);
        st_item->setForeground(status_color);
        m_saves_table->setItem(row, 4, st_item);

        // Col 5: Action Button
        auto* action_btn = new QPushButton(
            item.status == StormSaveSyncStatus::Conflict ? tr("Р Р°Р·СЂРµС€РёС‚СЊ РєРѕРЅС„Р»РёРєС‚") : tr("РЎРёРЅС…СЂРѕРЅРёР·РёСЂРѕРІР°С‚СЊ"),
            this);
        const QString title_id = item.title_id;
        connect(action_btn, &QPushButton::clicked, this, [this, title_id]() {
            if (m_items.contains(title_id)) {
                SyncItem(m_items[title_id]);
            }
        });
        m_saves_table->setCellWidget(row, 5, action_btn);

        row++;
    }
}

void StormSaveSyncDialog::OnTableItemDoubleClicked(int row, int column) {
    auto* item = m_saves_table->item(row, 0);
    if (!item) return;
    const QString title_id = item->data(Qt::UserRole).toString();
    if (m_items.contains(title_id)) {
        SyncItem(m_items[title_id]);
    }
}

void StormSaveSyncDialog::SyncItem(StormSaveItem& item) {
    if (!m_is_connected) {
        QMessageBox::warning(this, tr("РЎРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ"),
                             tr("Р”Р»СЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё РЅРµРѕР±С…РѕРґРёРјРѕ РїРѕРґРєР»СЋС‡РёС‚СЊСЃСЏ Рє СѓРґР°Р»С‘РЅРЅРѕРјСѓ СѓСЃС‚СЂРѕР№СЃС‚РІСѓ."));
        return;
    }

    if (item.status == StormSaveSyncStatus::Conflict) {
        StormSaveConflictDialog dialog(this, item, QHostInfo::localHostName(), m_remote_device_name);
        if (dialog.exec() == QDialog::Accepted) {
            SyncItemWithAction(item, dialog.GetAction());
        }
    } else if (item.status == StormSaveSyncStatus::LocalOnly) {
        UploadLocalSave(item.title_id, [this](bool ok) {
            if (ok) {
                m_status_label->setText(tr("РЎРѕС…СЂР°РЅРµРЅРёРµ СѓСЃРїРµС€РЅРѕ РїРµСЂРµРґР°РЅРѕ РЅР° СѓРґР°Р»С‘РЅРЅРѕРµ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ"));
                FetchRemoteSaves();
            }
        });
    } else if (item.status == StormSaveSyncStatus::RemoteOnly) {
        DownloadRemoteSave(item.title_id, [this](bool ok) {
            if (ok) {
                m_status_label->setText(tr("РЎРѕС…СЂР°РЅРµРЅРёРµ СѓСЃРїРµС€РЅРѕ РїРѕР»СѓС‡РµРЅРѕ СЃ СѓРґР°Р»С‘РЅРЅРѕРіРѕ СѓСЃС‚СЂРѕР№СЃС‚РІР°"));
                ScanLocalSaves();
                PopulateTable();
            }
        });
    } else if (item.status == StormSaveSyncStatus::Synchronized) {
        QMessageBox::information(this, tr("РЎРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ"),
                                 tr("РЎРѕС…СЂР°РЅРµРЅРёСЏ РґР»СЏ РґР°РЅРЅРѕР№ РёРіСЂС‹ СѓР¶Рµ РїРѕР»РЅРѕСЃС‚СЊСЋ РёРґРµРЅС‚РёС‡РЅС‹ РЅР° РѕР±РѕРёС… СѓСЃС‚СЂРѕР№СЃС‚РІР°С…."));
    }
}

void StormSaveSyncDialog::SyncItemWithAction(StormSaveItem& item, StormConflictAction action) {
    switch (action) {
    case StormConflictAction::ReplaceCurrent:
        DownloadRemoteSave(item.title_id, [this](bool ok) {
            if (ok) {
                m_status_label->setText(tr("РўРµРєСѓС‰РµРµ СЃРѕС…СЂР°РЅРµРЅРёРµ Р·Р°РјРµРЅРµРЅРѕ РЅР° СѓРґР°Р»С‘РЅРЅРѕРµ"));
                ScanLocalSaves();
                PopulateTable();
            }
        });
        break;
    case StormConflictAction::ReplaceRemote:
        UploadLocalSave(item.title_id, [this](bool ok) {
            if (ok) {
                m_status_label->setText(tr("РЈРґР°Р»С‘РЅРЅРѕРµ СЃРѕС…СЂР°РЅРµРЅРёРµ Р·Р°РјРµРЅРµРЅРѕ РЅР° С‚РµРєСѓС‰РµРµ"));
                FetchRemoteSaves();
            }
        });
        break;
    case StormConflictAction::KeepBoth:
        BackupLocalSave(item.title_id);
        BackupRemoteSave(item.title_id, [this, item](bool ok) {
            // After backup, download remote to synchronize active
            DownloadRemoteSave(item.title_id, [this](bool dl_ok) {
                if (dl_ok) {
                    m_status_label->setText(tr("РћР±Р° РІР°СЂРёР°РЅС‚Р° СЃРѕС…СЂР°РЅРµРЅС‹, СЂРµР·РµСЂРІРЅС‹Рµ РєРѕРїРёРё СЃРѕР·РґР°РЅС‹"));
                    ScanLocalSaves();
                    PopulateTable();
                }
            });
        });
        break;
    default:
        break;
    }
}

void StormSaveSyncDialog::DownloadRemoteSave(const QString& title_id, std::function<void(bool)> on_complete) {
    if (!m_is_connected) return;

    m_progress_bar->setVisible(true);
    m_progress_bar->setValue(30);

    const QUrl url(QStringLiteral("http://%1:%2/api/save/download?title_id=%3")
                       .arg(m_connected_remote_ip)
                       .arg(m_connected_remote_port)
                       .arg(title_id));
    QNetworkRequest req(url);
    auto* reply = m_network_mgr->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, title_id, on_complete]() {
        reply->deleteLater();
        m_progress_bar->setValue(70);

        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray data = reply->readAll();
            const QString temp_zip = QDir::tempPath() + QStringLiteral("/storm_dl_%1.zip").arg(title_id);
            QFile file(temp_zip);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();

                const auto target_dir = GetLocalSaveDirForTitle(title_id);
                std::error_code ec;
                std::filesystem::create_directories(target_dir, ec);

                const auto extracted = QtCommon::Compress::extractDir(temp_zip, QString::fromStdString(target_dir.string()));
                QFile::remove(temp_zip);
                m_progress_bar->setValue(100);
                m_progress_bar->setVisible(false);

                if (!extracted.isEmpty()) {
                    if (on_complete) on_complete(true);
                    return;
                }
            }
        }
        m_progress_bar->setVisible(false);
        QMessageBox::critical(this, tr("РћС€РёР±РєР° СЃРєР°С‡РёРІР°РЅРёСЏ"),
                              tr("РќРµ СѓРґР°Р»РѕСЃСЊ СЃРєР°С‡Р°С‚СЊ СЃРѕС…СЂР°РЅРµРЅРёРµ РґР»СЏ Title ID: %1").arg(title_id));
        if (on_complete) on_complete(false);
    });
}

void StormSaveSyncDialog::UploadLocalSave(const QString& title_id, std::function<void(bool)> on_complete) {
    if (!m_is_connected) return;

    const auto save_dir = GetLocalSaveDirForTitle(title_id);
    std::error_code ec;
    if (!std::filesystem::exists(save_dir, ec)) {
        if (on_complete) on_complete(false);
        return;
    }

    m_progress_bar->setVisible(true);
    m_progress_bar->setValue(30);

    const QString temp_zip = QDir::tempPath() + QStringLiteral("/storm_ul_%1.zip").arg(title_id);
    QFile::remove(temp_zip);
    if (!QtCommon::Compress::compressDir(temp_zip, QString::fromStdString(save_dir.string()))) {
        m_progress_bar->setVisible(false);
        if (on_complete) on_complete(false);
        return;
    }

    QFile file(temp_zip);
    if (!file.open(QIODevice::ReadOnly)) {
        m_progress_bar->setVisible(false);
        if (on_complete) on_complete(false);
        return;
    }
    const QByteArray zip_data = file.readAll();
    file.close();
    QFile::remove(temp_zip);

    m_progress_bar->setValue(60);

    const QUrl url(QStringLiteral("http://%1:%2/api/save/upload?title_id=%3")
                       .arg(m_connected_remote_ip)
                       .arg(m_connected_remote_port)
                       .arg(title_id));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/zip"));

    auto* reply = m_network_mgr->post(req, zip_data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, on_complete]() {
        reply->deleteLater();
        m_progress_bar->setValue(100);
        m_progress_bar->setVisible(false);
        const bool success = (reply->error() == QNetworkReply::NoError);
        if (on_complete) on_complete(success);
    });
}

void StormSaveSyncDialog::BackupLocalSave(const QString& title_id) {
    const auto save_dir = GetLocalSaveDirForTitle(title_id);
    std::error_code ec;
    if (!std::filesystem::exists(save_dir, ec)) return;

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy_HH-mm"));
    const auto backup_dir = save_dir.parent_path() / (title_id.toStdString() + "_backup_" + stamp.toStdString());
    std::filesystem::copy(save_dir, backup_dir,
                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
                          ec);
}

void StormSaveSyncDialog::BackupRemoteSave(const QString& title_id, std::function<void(bool)> on_complete) {
    if (!m_is_connected) {
        if (on_complete) on_complete(false);
        return;
    }

    const QUrl url(QStringLiteral("http://%1:%2/api/save/backup?title_id=%3")
                       .arg(m_connected_remote_ip)
                       .arg(m_connected_remote_port)
                       .arg(title_id));
    QNetworkRequest req(url);
    auto* reply = m_network_mgr->post(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [reply, on_complete]() {
        reply->deleteLater();
        const bool success = (reply->error() == QNetworkReply::NoError);
        if (on_complete) on_complete(success);
    });
}

void StormSaveSyncDialog::OnSyncAllClicked() {
    if (!m_is_connected) {
        QMessageBox::warning(this, tr("РЎРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ"),
                             tr("Р”Р»СЏ СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёРё РЅРµРѕР±С…РѕРґРёРјРѕ РїРѕРґРєР»СЋС‡РёС‚СЊСЃСЏ Рє СѓРґР°Р»С‘РЅРЅРѕРјСѓ СѓСЃС‚СЂРѕР№СЃС‚РІСѓ."));
        return;
    }

    for (auto& item : m_items) {
        if (item.status != StormSaveSyncStatus::Synchronized) {
            SyncItem(item);
        }
    }
}

void StormSaveSyncDialog::OnRefreshClicked() {
    ScanLocalSaves();
    if (m_is_connected) {
        FetchRemoteSaves();
    } else {
        PopulateTable();
    }
    BroadcastDiscovery();
}
