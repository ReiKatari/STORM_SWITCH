#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/configuration/configure_per_game_amiibo.h"

#include <filesystem>
#include <fstream>
#include <random>

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/fs_paths.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "core/core.h"


#include <QPainter>
#include <QPainterPath>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

enum class AmiiboIconType {
    Search,
    Star,
    Clock,
    Refresh,
    Controller,
    Disconnect,
    Download,
    Folder,
    Check,
    Cross,
};

QIcon MakeAmiiboIcon(AmiiboIconType type, const QColor& color, int size = 16) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case AmiiboIconType::Search:
        p.drawEllipse(QPointF(6.5, 6.5), 4.0, 4.0);
        p.drawLine(QPointF(9.5, 9.5), QPointF(13.5, 13.5));
        break;
    case AmiiboIconType::Star: {
        QPainterPath star;
        const qreal scx = 8.0, scy = 8.0, r_out = 6.2, r_in = 2.8;
        for (int i = 0; i < 5; ++i) {
            qreal a_out = -M_PI_2 + i * 2.0 * M_PI / 5.0;
            qreal a_in = a_out + M_PI / 5.0;
            if (i == 0) star.moveTo(scx + r_out * std::cos(a_out), scy + r_out * std::sin(a_out));
            else star.lineTo(scx + r_out * std::cos(a_out), scy + r_out * std::sin(a_out));
            star.lineTo(scx + r_in * std::cos(a_in), scy + r_in * std::sin(a_in));
        }
        star.closeSubpath();
        p.setBrush(color);
        p.drawPath(star);
        break;
    }
    case AmiiboIconType::Clock:
        p.drawEllipse(QPointF(8.0, 8.0), 5.5, 5.5);
        p.drawLine(QPointF(8.0, 4.5), QPointF(8.0, 8.0));
        p.drawLine(QPointF(8.0, 8.0), QPointF(11.0, 8.0));
        break;
    case AmiiboIconType::Refresh: {
        p.drawArc(QRectF(3.0, 3.0, 10.0, 10.0), 30 * 16, 270 * 16);
        p.drawLine(QPointF(11.0, 4.5), QPointF(13.0, 6.5));
        p.drawLine(QPointF(13.0, 6.5), QPointF(10.5, 9.0));
        break;
    }
    case AmiiboIconType::Controller:
        p.drawRoundedRect(QRectF(2.5, 4.5, 11.0, 7.0), 2.5, 2.5);
        p.drawPoint(QPointF(5.5, 8.0));
        p.drawPoint(QPointF(10.5, 7.0));
        p.drawPoint(QPointF(10.5, 9.0));
        break;
    case AmiiboIconType::Disconnect:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    case AmiiboIconType::Download:
        p.drawLine(QPointF(8.0, 2.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(4.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(11.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(3.0, 13.5), QPointF(13.0, 13.5));
        break;
    case AmiiboIconType::Folder: {
        QPainterPath path;
        path.moveTo(2.5, 5.0);
        path.lineTo(6.0, 5.0);
        path.lineTo(7.5, 6.5);
        path.lineTo(13.5, 6.5);
        path.lineTo(13.5, 12.5);
        path.lineTo(2.5, 12.5);
        path.closeSubpath();
        p.drawPath(path);
        break;
    }
    case AmiiboIconType::Check:
        p.drawLine(QPointF(3.0, 8.0), QPointF(6.5, 11.5));
        p.drawLine(QPointF(6.5, 11.5), QPointF(13.0, 4.5));
        break;
    case AmiiboIconType::Cross:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    }
    return QIcon(pix);
}

QPixmap MakeAmiiboPedestalPlaceholder(int w, int h, const StormTheme::Palette& pal) {
    QPixmap pix(w, h);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor accent(pal.accent);
    const QColor muted(pal.textSecondary);

    p.setPen(QPen(QColor(pal.border), 1.0));
    p.setBrush(QColor(pal.surface));
    p.drawRoundedRect(QRectF(2, 2, w - 4, h - 4), 8.0, 8.0);

    const qreal base_cx = w / 2.0;
    const qreal base_y = h - 28.0;

    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawEllipse(QPointF(base_cx, base_y + 10.0), 38.0, 9.0);

    // Pedestal base
    QLinearGradient gradBase(0, base_y - 4, 0, base_y + 8);
    gradBase.setColorAt(0.0, accent.darker(150));
    gradBase.setColorAt(0.5, accent);
    gradBase.setColorAt(1.0, accent.darker(180));
    p.setPen(QPen(accent.lighter(130), 1.2));
    p.setBrush(gradBase);
    p.drawRoundedRect(QRectF(base_cx - 36, base_y - 2, 72, 10), 4, 4);

    p.setPen(QPen(accent.lighter(160), 1.5));
    p.setBrush(QColor(pal.cardAlt));
    p.drawEllipse(QPointF(base_cx, base_y - 4), 32.0, 8.0);

    // NFC concentric wave rings
    p.setPen(QPen(accent, 1.0, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(base_cx, base_y - 4), 22.0, 5.5);
    p.drawEllipse(QPointF(base_cx, base_y - 4), 12.0, 3.0);

    // Figurite silhouette
    QPainterPath fig;
    fig.addEllipse(QPointF(base_cx, 38.0), 12.0, 12.0);
    fig.moveTo(base_cx - 16.0, 54.0);
    fig.cubicTo(base_cx - 20.0, 72.0, base_cx - 14.0, 92.0, base_cx - 8.0, base_y - 5.0);
    fig.lineTo(base_cx + 8.0, base_y - 5.0);
    fig.cubicTo(base_cx + 14.0, 92.0, base_cx + 20.0, 72.0, base_cx + 16.0, 54.0);
    fig.closeSubpath();

    QLinearGradient figGrad(0, 26, 0, base_y - 5);
    figGrad.setColorAt(0.0, QColor(accent.red(), accent.green(), accent.blue(), 180));
    figGrad.setColorAt(0.7, QColor(muted.red(), muted.green(), muted.blue(), 120));
    figGrad.setColorAt(1.0, QColor(accent.red(), accent.green(), accent.blue(), 40));

    p.setPen(QPen(accent, 1.2));
    p.setBrush(figGrad);
    p.drawPath(fig);

    // Center star crest
    p.setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 1.0));
    p.setBrush(QColor(QStringLiteral("#FFFFFF")));
    QPainterPath star;
    const qreal scx = base_cx;
    const qreal scy = 64.0;
    const qreal r_out = 6.0;
    const qreal r_in = 2.8;
    for (int i = 0; i < 5; ++i) {
        qreal a_out = -M_PI_2 + i * 2.0 * M_PI / 5.0;
        qreal a_in = a_out + M_PI / 5.0;
        if (i == 0) star.moveTo(scx + r_out * std::cos(a_out), scy + r_out * std::sin(a_out));
        else star.lineTo(scx + r_out * std::cos(a_out), scy + r_out * std::sin(a_out));
        star.lineTo(scx + r_in * std::cos(a_in), scy + r_in * std::sin(a_in));
    }
    star.closeSubpath();
    p.drawPath(star);

    return pix;
}

} // namespace

ConfigurePerGameAmiibo::ConfigurePerGameAmiibo(Core::System& system, u64 title_id,
                                               const QString& file_name, QWidget* parent)
    : QWidget(parent), m_system(system), m_title_id(title_id), m_file_name(file_name),
      m_network_mgr(new QNetworkAccessManager(this)) {
    SetupUi();
    LoadAmiiboDatabase();
}

ConfigurePerGameAmiibo::~ConfigurePerGameAmiibo() = default;

void ConfigurePerGameAmiibo::SetupUi() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(8);

    // Top Filter Bar
    auto* top_filter_box = new QWidget(this);
    auto* filter_layout = new QHBoxLayout(top_filter_box);
    filter_layout->setContentsMargins(0, 0, 0, 0);
    filter_layout->setSpacing(8);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText(StormLang(QStringLiteral("Поиск Amiibo для этой игры..."), QStringLiteral("Search Amiibo for this game...")));
    m_search_edit->setClearButtonEnabled(true);
    connect(m_search_edit, &QLineEdit::textChanged, this, &ConfigurePerGameAmiibo::OnSearchFilterChanged);
    filter_layout->addWidget(m_search_edit, 3);

    m_filter_scope_combo = new QComboBox(this);
    m_filter_scope_combo->addItems({StormLang(QStringLiteral("Рекомендуемые для игры"), QStringLiteral("Recommended for game")), StormLang(QStringLiteral("Все доступные Amiibo"), QStringLiteral("All available Amiibo"))});
    connect(m_filter_scope_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigurePerGameAmiibo::OnSearchFilterChanged);
    filter_layout->addWidget(m_filter_scope_combo, 2);

    m_series_combo = new QComboBox(this);
    m_series_combo->addItem(tr("Все серии"));
    connect(m_series_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigurePerGameAmiibo::OnSearchFilterChanged);
    filter_layout->addWidget(m_series_combo, 2);

    const auto pal = StormTheme::GetThemePalette();
    m_refresh_btn = new QPushButton(StormLang(QStringLiteral("Обновить"), QStringLiteral("Refresh")), this);
    m_refresh_btn->setIcon(MakeAmiiboIcon(AmiiboIconType::Refresh, QColor(pal.textSecondary)));
    connect(m_refresh_btn, &QPushButton::clicked, this, &ConfigurePerGameAmiibo::OnRefreshClicked);
    filter_layout->addWidget(m_refresh_btn, 0);

    main_layout->addWidget(top_filter_box);

    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setFixedHeight(8);
    m_progress_bar->setTextVisible(false);
    m_progress_bar->setRange(0, 0);
    m_progress_bar->setTextVisible(false);
    m_progress_bar->setFixedHeight(4);
    m_progress_bar->setVisible(false);
    main_layout->addWidget(m_progress_bar);

    // Splitter with List (left) and Inspector (right)
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);

    // Left List
    auto* left_container = new QWidget(this);
    auto* left_layout = new QVBoxLayout(left_container);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(4);

    auto* list_hint = new QLabel(tr("Отметьте галочками нужные Amiibo для автоматической загрузки и привязки к игре:"), this);
    list_hint->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 9pt;"));
    left_layout->addWidget(list_hint);

    m_amiibo_list = new QListWidget(this);
    m_amiibo_list->setStyleSheet(QStringLiteral("QListWidget::item { height: 34px; padding: 4px 8px; border-radius: 4px; }"));
    connect(m_amiibo_list, &QListWidget::currentItemChanged, this, &ConfigurePerGameAmiibo::OnItemSelected);
    connect(m_amiibo_list, &QListWidget::itemChanged, this, &ConfigurePerGameAmiibo::OnItemCheckChanged);
    left_layout->addWidget(m_amiibo_list);

    splitter->addWidget(left_container);

    // Right Details
    auto* right_container = new QWidget(this);
    auto* right_layout = new QVBoxLayout(right_container);
    right_layout->setContentsMargins(8, 0, 0, 0);
    right_layout->setSpacing(8);

    auto* details_group = new QGroupBox(tr("Информация и статус Amiibo"), this);
    auto* details_layout = new QVBoxLayout(details_group);
    details_layout->setContentsMargins(10, 14, 10, 10);
    details_layout->setSpacing(6);

    m_image_label = new QLabel(this);
    m_image_label->setFixedSize(140, 140);
    m_image_label->setPixmap(MakeAmiiboPedestalPlaceholder(140, 140, pal));
    m_image_label->setMinimumSize(160, 160);
    m_image_label->setMaximumHeight(200);
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_label->setStyleSheet(QStringLiteral("background-color: #0c1018; border: 1px solid rgba(255,255,255,0.08); border-radius: 8px;"));
    m_image_label->setText(tr("Выберите фигурку"));
    details_layout->addWidget(m_image_label);

    m_name_label = new QLabel(tr("Имя: —"), this);
    const auto amiibo_pal = StormTheme::GetThemePalette();
    m_name_label->setStyleSheet(QStringLiteral("font-size: 11pt; font-weight: bold; color: %1;").arg(amiibo_pal.accent));
    details_layout->addWidget(m_name_label);

    m_series_label = new QLabel(tr("Серия: —"), this);
    details_layout->addWidget(m_series_label);

    m_type_label = new QLabel(tr("Тип: —"), this);
    details_layout->addWidget(m_type_label);

    m_status_badge = new QLabel(tr("Статус: Ожидание"), this);
    m_status_badge->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: bold; padding: 2px 6px; background-color: #1a2336; border-radius: 3px;"));
    details_layout->addWidget(m_status_badge);

    // Elevated 3D Reward Card
    m_reward_card = new QWidget(this);
    m_reward_card->setStyleSheet(QStringLiteral(
        "QWidget#AmiiboRewardCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "}"
    ).arg(amiibo_pal.surface, amiibo_pal.bg, amiibo_pal.accent));
    m_reward_card->setObjectName(QStringLiteral("AmiiboRewardCard"));
    auto* card_layout = new QVBoxLayout(m_reward_card);
    card_layout->setContentsMargins(10, 8, 10, 8);
    card_layout->setSpacing(4);

    auto* card_header = new QHBoxLayout();
    auto* card_title = new QLabel(StormLang(QStringLiteral("Награда в этой игре:"), QStringLiteral("Reward in this game:")), m_reward_card);
    card_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #ffca28; font-size: 9.5pt;"));
    card_header->addWidget(card_title);

    m_reward_category_badge = new QLabel(tr("УНИВЕРСАЛЬНЫЙ БОНУС"), m_reward_card);
    m_reward_category_badge->setStyleSheet(QStringLiteral(
        "background-color: %1; color: %2; border: 1px solid %2; border-radius: 4px; padding: 2px 6px; font-weight: bold; font-size: 8pt;"
    ).arg(amiibo_pal.card, amiibo_pal.accent));
    card_header->addWidget(m_reward_category_badge, 0, Qt::AlignRight);
    card_layout->addLayout(card_header);

    m_reward_name_label = new QLabel(tr("Выберите фигурку для просмотра наград"), m_reward_card);
    m_reward_name_label->setStyleSheet(QStringLiteral("font-size: 10pt; font-weight: bold; color: %1;").arg(amiibo_pal.accent));
    m_reward_name_label->setWordWrap(true);
    card_layout->addWidget(m_reward_name_label);

    m_reward_desc_label = new QLabel(m_reward_card);
    m_reward_desc_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 8.5pt; line-height: 1.3;"));
    m_reward_desc_label->setWordWrap(true);
    card_layout->addWidget(m_reward_desc_label);

    details_layout->addWidget(m_reward_card);

    auto* games_label = new QLabel(StormLang(QStringLiteral("Эффект и совместимость в этой игре:"), QStringLiteral("Effect and compatibility in this game:")), this);
    games_label->setStyleSheet(QStringLiteral("font-weight: bold; color: #ffca28; margin-top: 4px;"));
    details_layout->addWidget(games_label);

    m_games_text = new QTextEdit(this);
    m_games_text->setReadOnly(true);
    m_games_text->setPlaceholderText(tr("Описание совместимости"));
    details_layout->addWidget(m_games_text, 1);

    auto* btn_layout = new QHBoxLayout();
    m_install_btn = new QPushButton(StormLang(QStringLiteral("Включить / Установить"), QStringLiteral("Enable / Install")), this);
    m_install_btn->setIcon(MakeAmiiboIcon(AmiiboIconType::Controller, QColor(QStringLiteral("#FFFFFF"))));
    m_install_btn->setStyleSheet(QStringLiteral("background-color: #004d40; border-color: #00e676; color: #ffffff; padding: 6px 12px; font-weight: bold;"));
    m_install_btn->setEnabled(false);
    connect(m_install_btn, &QPushButton::clicked, this, [this]() {
        auto* cur = m_amiibo_list->currentItem();
        if (cur) {
            bool is_checked = cur->checkState() == Qt::Checked;
            cur->setCheckState(is_checked ? Qt::Unchecked : Qt::Checked);
        }
    });
    btn_layout->addWidget(m_install_btn);

    m_open_folder_btn = new QPushButton(StormLang(QStringLiteral("Папка Amiibo игры"), QStringLiteral("Game Amiibo Folder")), this);
    m_open_folder_btn->setIcon(MakeAmiiboIcon(AmiiboIconType::Folder, QColor(pal.textSecondary)));
    connect(m_open_folder_btn, &QPushButton::clicked, this, &ConfigurePerGameAmiibo::OnOpenAmiiboFolderClicked);
    btn_layout->addWidget(m_open_folder_btn);

    details_layout->addLayout(btn_layout);
    right_layout->addWidget(details_group);
    splitter->addWidget(right_container);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    main_layout->addWidget(splitter);
}

QString ConfigurePerGameAmiibo::GetGameAmiiboFolder() const {
    const auto amiibo_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::AmiiboDir);
    QString clean_title = QStringLiteral("%1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper();
    if (!m_file_name.isEmpty()) {
        QFileInfo fi(m_file_name);
        QString base = fi.completeBaseName();
        base.replace(QRegularExpression(QStringLiteral(R"(\[[^\]]*\]|\([^\)]*\))")), QString());
        base = base.trimmed();
        if (!base.isEmpty()) {
            clean_title = base;
        }
    }
    clean_title.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    auto game_folder = amiibo_dir / clean_title.toStdString();
    Common::FS::CreateDirs(game_folder);
    return QString::fromStdString(game_folder.string());
}

bool ConfigurePerGameAmiibo::IsAmiiboInstalled(const AmiiboEntry& entry) const {
    QString folder = GetGameAmiiboFolder();
    QString clean_name = entry.name;
    clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    QString path1 = folder + QStringLiteral("/") + clean_name + QStringLiteral(".bin");

    return QFile::exists(path1);
}

void ConfigurePerGameAmiibo::LoadAmiiboDatabase() {
    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
    auto cache_file = cache_dir / "amiibo_database.json";

    if (std::filesystem::exists(cache_file)) {
        QFile file(QString::fromStdString(cache_file.string()));
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray file_bytes = file.readAll();
            file.close();
            ParseDatabaseJson(file_bytes);
            if (!m_all_amiibos.empty()) {
                PopulateSeriesFilter();
                ApplyFilters();
                return;
            }
        }
    }

    OnRefreshClicked();
}

void ConfigurePerGameAmiibo::OnRefreshClicked() {
    m_progress_bar->setVisible(true);

    const QStringList urls = {
        QStringLiteral("https://www.amiiboapi.com/api/amiibo/"),
        QStringLiteral("https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/database/amiibo.json"),
        QStringLiteral("https://raw.githubusercontent.com/N3evin/AmiiboAPI/master/database/amiibo.json"),
    };

    auto tryFetchUrl = [this, urls](auto&& self, int index) -> void {
        if (index >= urls.size()) {
            m_progress_bar->setVisible(false);
            return;
        }

        QUrl url(urls[index]);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 STORM-SWITCH/4.8.8"));
        req.setRawHeader("Accept", "application/json, text/plain, */*");

        auto* reply = m_network_mgr->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, index, self]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray db_raw_data = reply->readAll();
                const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
                Common::FS::CreateDirs(cache_dir);
                auto cache_file = cache_dir / "amiibo_database.json";
                QFile file(QString::fromStdString(cache_file.string()));
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(db_raw_data);
                    file.close();
                }
                ParseDatabaseJson(db_raw_data);
                PopulateSeriesFilter();
                ApplyFilters();
                m_progress_bar->setVisible(false);
            } else {
                self(self, index + 1);
            }
            reply->deleteLater();
        });
    };

    tryFetchUrl(tryFetchUrl, 0);
}

void ConfigurePerGameAmiibo::ParseDatabaseJson(const QByteArray& json_data) {
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(json_data, &err);
    if (err.error != QJsonParseError::NoError) return;

    std::vector<AmiiboEntry> parsed_list;

    auto populateSwitchGames = [](AmiiboEntry& entry) {
        if (!entry.switch_games.isEmpty()) return;
        if (entry.game_series.contains(QStringLiteral("Zelda"), Qt::CaseInsensitive) ||
            entry.amiibo_series.contains(QStringLiteral("Zelda"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• The Legend of Zelda: Tears of the Kingdom: Эксклюзивная ткань параплана, оружие, ресурсы"));
            entry.switch_games.append(tr("• The Legend of Zelda: Breath of the Wild: Доспехи, оружие, Эпона, Волк Линк, сундуки"));
            entry.switch_games.append(tr("• The Legend of Zelda: Echoes of Wisdom: Уникальные костюмы, аксессуары и ресурсы"));
            entry.switch_games.append(tr("• Super Smash Bros. Ultimate: Обучаемый боец FP (Figure Player)"));
        } else if (entry.game_series.contains(QStringLiteral("Mario"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Mario"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Super Mario Odyssey: Уникальные костюмы для Марио и подсказки Лун энергии"));
            entry.switch_games.append(tr("• Super Mario 3D World + Bowser's Fury: Костюм Белого Тануки Неуязвимости, суперзвезды"));
            entry.switch_games.append(tr("• Super Smash Bros. Ultimate: Боец FP с прокачкой 1-50 ур."));
            entry.switch_games.append(tr("• Mario Kart 8 Deluxe: Гоночный костюм Mii"));
        } else if (entry.game_series.contains(QStringLiteral("Splatoon"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Splatoon"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Splatoon 3 / 2: Эксклюзивные наборы экипировки и фотосессии"));
        } else if (entry.game_series.contains(QStringLiteral("Metroid"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Metroid"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Metroid Dread: Дополнительный контейнер энергии (Energy Tank) и пополнение ракет"));
        } else if (entry.game_series.contains(QStringLiteral("Monster Hunter"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Monster Hunter"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Monster Hunter Rise: Многослойная броня и ежедневная лотерея"));
        } else if (entry.game_series.contains(QStringLiteral("Animal Crossing"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Animal Crossing"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Animal Crossing: New Horizons: Плакаты, кемпинг жителя, фотосессия"));
        } else if (entry.game_series.contains(QStringLiteral("Fire Emblem"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Fire Emblem"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Fire Emblem Engage: Музыкальные треки и билеты на наряды"));
        } else if (entry.game_series.contains(QStringLiteral("Xenoblade"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Xenoblade"), Qt::CaseInsensitive)) {
            entry.switch_games.append(tr("• Xenoblade Chronicles 3: Облик Меча Монадо и полезные расходники"));
        } else {
            entry.switch_games.append(tr("• Super Smash Bros. Ultimate: Обучаемый боец FP или получение бонусов/духов"));
            entry.switch_games.append(tr("• Mario Kart 8 Deluxe: Гоночный костюм Mii"));
            entry.switch_games.append(tr("• Универсальная поддержка: Совместимо со всеми Switch играми с поддержкой Amiibo"));
        }
    };

    auto parseObject = [&populateSwitchGames](const QJsonObject& obj) -> AmiiboEntry {
        AmiiboEntry entry;
        entry.character = obj.value(QStringLiteral("character")).toString();
        entry.name = obj.value(QStringLiteral("name")).toString();
        entry.game_series = obj.value(QStringLiteral("gameSeries")).toString();
        entry.amiibo_series = obj.value(QStringLiteral("amiiboSeries")).toString();
        entry.type = obj.value(QStringLiteral("type")).toString();
        entry.head = obj.value(QStringLiteral("head")).toString();
        entry.tail = obj.value(QStringLiteral("tail")).toString();
        entry.image_url = obj.value(QStringLiteral("image")).toString();

        QJsonArray switch_games = obj.value(QStringLiteral("gamesSwitch")).toArray();
        for (const auto& g : switch_games) {
            QJsonObject g_obj = g.toObject();
            QString g_name = g_obj.value(QStringLiteral("gameName")).toString();
            QJsonArray usages = g_obj.value(QStringLiteral("amiiboUsage")).toArray();
            QString usage_str;
            for (const auto& u : usages) {
                usage_str += u.toObject().value(QStringLiteral("Usage")).toString() + QStringLiteral("; ");
            }
            if (!usage_str.isEmpty()) {
                entry.switch_games.append(QStringLiteral("• %1: %2").arg(g_name, usage_str));
            } else {
                entry.switch_games.append(QStringLiteral("• %1").arg(g_name));
            }
        }
        populateSwitchGames(entry);
        return entry;
    };

    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("amiibo")) && root.value(QStringLiteral("amiibo")).isArray()) {
            QJsonArray arr = root.value(QStringLiteral("amiibo")).toArray();
            for (const auto& val : arr) {
                if (val.isObject()) {
                    AmiiboEntry e = parseObject(val.toObject());
                    if (e.image_url.isEmpty() && !e.head.isEmpty() && !e.tail.isEmpty()) {
                        e.image_url = QStringLiteral("https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/images/icon_%1-%2.png")
                                          .arg(e.head.toLower(), e.tail.toLower());
                    }
                    parsed_list.push_back(std::move(e));
                }
            }
        } else if (root.contains(QStringLiteral("amiibos")) && root.value(QStringLiteral("amiibos")).isObject()) {
            QJsonObject dict = root.value(QStringLiteral("amiibos")).toObject();
            QJsonObject amiibo_series_map = root.value(QStringLiteral("amiibo_series")).toObject();
            QJsonObject game_series_map = root.value(QStringLiteral("game_series")).toObject();
            QJsonObject types_map = root.value(QStringLiteral("types")).toObject();
            QJsonObject characters_map = root.value(QStringLiteral("characters")).toObject();

            for (auto it = dict.begin(); it != dict.end(); ++it) {
                if (it.value().isObject()) {
                    AmiiboEntry e = parseObject(it.value().toObject());
                    QString key = it.key();
                    if (key.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) key = key.mid(2);
                    if (key.length() >= 16) {
                        e.head = key.left(8);
                        e.tail = key.mid(8, 8);
                    }

                    if (e.amiibo_series.isEmpty() && e.tail.length() >= 4) {
                        QString series_id = QStringLiteral("0x") + e.tail.mid(2, 2).toLower();
                        if (amiibo_series_map.contains(series_id)) {
                            e.amiibo_series = amiibo_series_map.value(series_id).toString();
                        }
                    }
                    if (e.amiibo_series.isEmpty()) e.amiibo_series = QStringLiteral("Others");

                    if (e.type.isEmpty() && e.tail.length() >= 8) {
                        QString type_id = QStringLiteral("0x") + e.tail.mid(6, 2).toLower();
                        if (types_map.contains(type_id)) {
                            e.type = types_map.value(type_id).toString();
                        }
                    }
                    if (e.type.isEmpty()) e.type = QStringLiteral("Figure");

                    if (e.game_series.isEmpty() && e.head.length() >= 3) {
                        QString g_id = QStringLiteral("0x") + e.head.left(3).toLower();
                        if (game_series_map.contains(g_id)) {
                            e.game_series = game_series_map.value(g_id).toString();
                        }
                    }
                    if (e.game_series.isEmpty()) e.game_series = QStringLiteral("Nintendo");

                    if (e.character.isEmpty() && e.head.length() >= 4) {
                        QString c_id = QStringLiteral("0x") + e.head.left(4).toLower();
                        if (characters_map.contains(c_id)) {
                            e.character = characters_map.value(c_id).toString();
                        }
                    }
                    if (e.character.isEmpty()) e.character = e.name;

                    if (e.image_url.isEmpty() && !e.head.isEmpty() && !e.tail.isEmpty()) {
                        e.image_url = QStringLiteral("https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/images/icon_%1-%2.png")
                                          .arg(e.head.toLower(), e.tail.toLower());
                    }

                    populateSwitchGames(e);
                    parsed_list.push_back(std::move(e));
                }
            }
        }
    }

    if (!parsed_list.empty()) {
        m_all_amiibos = std::move(parsed_list);
    }
}

void ConfigurePerGameAmiibo::PopulateSeriesFilter() {
    QString current_series = m_series_combo->currentText();
    m_series_combo->blockSignals(true);
    m_series_combo->clear();
    m_series_combo->addItem(tr("Все серии"));

    QSet<QString> series_set;
    for (const auto& a : m_all_amiibos) {
        if (!a.amiibo_series.isEmpty()) {
            series_set.insert(a.amiibo_series);
        }
    }
    QStringList series_list = series_set.values();
    series_list.sort();
    for (const auto& s : series_list) {
        m_series_combo->addItem(s);
    }

    int idx = m_series_combo->findText(current_series);
    if (idx >= 0) m_series_combo->setCurrentIndex(idx);
    m_series_combo->blockSignals(false);
}

void ConfigurePerGameAmiibo::OnSearchFilterChanged() {
    ApplyFilters();
}

void ConfigurePerGameAmiibo::ApplyFilters() {
    m_is_updating_ui = true;
    m_amiibo_list->clear();
    m_filtered_indices.clear();

    const QString search_text = m_search_edit->text().trimmed().toLower();
    const QString series_filter = m_series_combo->currentText();
    const bool recommended_only = (m_filter_scope_combo->currentIndex() == 0);

    // Determine keywords for current game
    QString title_str = QStringLiteral("%1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper();
    QString game_keyword;
    if (!m_file_name.isEmpty()) {
        QFileInfo fi(m_file_name);
        game_keyword = fi.completeBaseName().toLower();
    }

    for (size_t i = 0; i < m_all_amiibos.size(); ++i) {
        const auto& a = m_all_amiibos[i];

        if (m_series_combo->currentIndex() > 0 && a.amiibo_series != series_filter) {
            continue;
        }

        if (recommended_only) {
            bool matches_game = false;
            if (game_keyword.contains(QStringLiteral("zelda")) &&
                (a.game_series.contains(QStringLiteral("zelda"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("zelda"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("mario")) &&
                       (a.game_series.contains(QStringLiteral("mario"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("mario"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("smash")) || game_keyword.contains(QStringLiteral("ssbu"))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("splatoon")) &&
                       (a.game_series.contains(QStringLiteral("splatoon"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("splatoon"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("metroid")) &&
                       (a.game_series.contains(QStringLiteral("metroid"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("metroid"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("pokemon")) &&
                       (a.game_series.contains(QStringLiteral("pokemon"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("pokemon"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("kirby")) &&
                       (a.game_series.contains(QStringLiteral("kirby"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("kirby"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("fire emblem")) &&
                       (a.game_series.contains(QStringLiteral("fire emblem"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("fire emblem"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("xenoblade")) &&
                       (a.game_series.contains(QStringLiteral("xenoblade"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("xenoblade"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("monster hunter")) &&
                       (a.game_series.contains(QStringLiteral("monster hunter"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("monster hunter"), Qt::CaseInsensitive))) {
                matches_game = true;
            } else if (game_keyword.contains(QStringLiteral("animal crossing")) &&
                       (a.game_series.contains(QStringLiteral("animal crossing"), Qt::CaseInsensitive) || a.amiibo_series.contains(QStringLiteral("animal crossing"), Qt::CaseInsensitive))) {
                matches_game = true;
            }

            if (!matches_game) {
                // If game is unknown, match Smash or Mario or Zelda as top Switch amiibos
                if (a.amiibo_series.contains(QStringLiteral("Smash"), Qt::CaseInsensitive) ||
                    a.amiibo_series.contains(QStringLiteral("Zelda"), Qt::CaseInsensitive) ||
                    a.amiibo_series.contains(QStringLiteral("Mario"), Qt::CaseInsensitive)) {
                    matches_game = true;
                }
            }

            if (!matches_game) continue;
        }

        if (!search_text.isEmpty()) {
            bool matches = a.name.toLower().contains(search_text) ||
                           a.character.toLower().contains(search_text) ||
                           a.game_series.toLower().contains(search_text) ||
                           a.amiibo_series.toLower().contains(search_text);
            if (!matches) continue;
        }

        m_filtered_indices.push_back(static_cast<int>(i));

        bool installed = IsAmiiboInstalled(a);
        auto* item = new QListWidgetItem(QStringLiteral("%1 [%2] %3")
                                             .arg(a.name, a.amiibo_series, installed ? StormLang(QStringLiteral(" [Установлен]"), QStringLiteral(" [Installed]")) : QString()));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(installed ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, static_cast<int>(i));
        m_amiibo_list->addItem(item);
    }

    m_is_updating_ui = false;

    if (m_amiibo_list->count() > 0) {
        m_amiibo_list->setCurrentRow(0);
    } else {
        m_name_label->setText(tr("Ничего не найдено"));
        m_series_label->setText(QString());
        m_type_label->setText(QString());
        m_status_badge->setText(tr("Статус: Нет результатов"));
        m_games_text->clear();
        m_image_label->setText(tr("Нет данных"));
        m_install_btn->setEnabled(false);
        if (m_reward_card) {
            m_reward_card->setVisible(false);
        }
    }
}

void ConfigurePerGameAmiibo::OnItemSelected(QListWidgetItem* current, QListWidgetItem* previous) {
    if (!current) return;
    int idx = current->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx < static_cast<int>(m_all_amiibos.size())) {
        DisplayAmiiboDetails(m_all_amiibos[idx]);
    }
}

void ConfigurePerGameAmiibo::OnItemCheckChanged(QListWidgetItem* item) {
    if (m_is_updating_ui || !item) return;

    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= static_cast<int>(m_all_amiibos.size())) return;

    const auto& entry = m_all_amiibos[idx];
    if (item->checkState() == Qt::Checked) {
        QString saved = GenerateAndSaveAmiiboBin(entry);
        item->setText(QStringLiteral("%1 [%2] [Установлен]").arg(entry.name, entry.amiibo_series));
        DisplayAmiiboDetails(entry);
    } else {
        QString folder = GetGameAmiiboFolder();
        QString clean_name = entry.name;
        clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
        QString path = folder + QStringLiteral("/") + clean_name + QStringLiteral(".bin");
        QFile::remove(path);
        item->setText(QStringLiteral("%1 [%2]").arg(entry.name, entry.amiibo_series));
        DisplayAmiiboDetails(entry);
    }
}

void ConfigurePerGameAmiibo::DisplayAmiiboDetails(const AmiiboEntry& entry) {
    m_name_label->setText(tr("Имя: %1 (%2)").arg(entry.name, entry.character));
    m_series_label->setText(tr("Серия Amiibo: %1 | Вселенная: %2").arg(entry.amiibo_series, entry.game_series));
    m_type_label->setText(tr("Тип: %1 | ID: %2 %3").arg(entry.type, entry.head, entry.tail));

    bool installed = IsAmiiboInstalled(entry);
    if (installed) {
        m_status_badge->setText(StormLang(QStringLiteral("Статус: Включено и установлено для этой игры"), QStringLiteral("Status: Enabled and installed for this game")));
        m_status_badge->setStyleSheet(QStringLiteral("color: #00e676; font-weight: bold; padding: 3px 8px; background-color: #004d40; border-radius: 4px;"));
        m_install_btn->setText(StormLang(QStringLiteral("Отключить для игры"), QStringLiteral("Disable for game")));
        m_install_btn->setIcon(MakeAmiiboIcon(AmiiboIconType::Disconnect, QColor(StormTheme::GetThemePalette().textSecondary)));
    } else {
        m_status_badge->setText(StormLang(QStringLiteral("Статус: Нажмите галочку для скачивания и включения"), QStringLiteral("Status: Check box to download and enable")));
        const auto amiibo_pal2 = StormTheme::GetThemePalette();
        m_status_badge->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; padding: 3px 8px; background-color: %2; border-radius: 4px;").arg(amiibo_pal2.accent, amiibo_pal2.surface));
        m_install_btn->setText(StormLang(QStringLiteral("Скачать и включить"), QStringLiteral("Download and enable")));
        m_install_btn->setIcon(MakeAmiiboIcon(AmiiboIconType::Download, QColor(QStringLiteral("#FFFFFF"))));
    }

    const QString game = m_game_name.isEmpty() ? m_file_name : m_game_name;
    const auto reward = AmiiboBrowserDialog::GetRewardForGame(entry, game);
    if (m_reward_card && m_reward_category_badge && m_reward_name_label && m_reward_desc_label) {
        m_reward_category_badge->setText(reward.category.toUpper());
        m_reward_name_label->setText(reward.item_name);
        m_reward_desc_label->setText(reward.description);
        m_reward_card->setVisible(true);
    }

    if (entry.switch_games.isEmpty()) {
        m_games_text->setPlainText(tr("Совместимо с Nintendo Switch."));
    } else {
        m_games_text->setPlainText(entry.switch_games.join(QStringLiteral("\n\n")));
    }

    FetchImage(entry.image_url, m_image_label);
    m_install_btn->setEnabled(true);
}

void ConfigurePerGameAmiibo::FetchImage(const QString& image_url, QLabel* target_label) {
    if (image_url.isEmpty()) {
        target_label->setText(tr("Изображение отсутствует"));
        return;
    }

    if (m_image_cache.contains(image_url)) {
        target_label->setPixmap(m_image_cache[image_url].scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "amiibo_images";
    Common::FS::CreateDirs(cache_dir);
    QString filename = QUrl(image_url).fileName();
    if (filename.isEmpty()) filename = QStringLiteral("image.png");
    auto local_img_path = cache_dir / filename.toStdString();

    if (std::filesystem::exists(local_img_path)) {
        QPixmap pixmap(QString::fromStdString(local_img_path.string()));
        if (!pixmap.isNull()) {
            m_image_cache[image_url] = pixmap;
            target_label->setPixmap(pixmap.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    target_label->setText(tr("Загрузка артворка..."));

    QStringList mirror_urls;
    mirror_urls << image_url;
    QString fastly_url = image_url;
    fastly_url.replace(QStringLiteral("cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master"), QStringLiteral("fastly.jsdelivr.net/gh/N3evin/AmiiboAPI@master"));
    fastly_url.replace(QStringLiteral("raw.githubusercontent.com/N3evin/AmiiboAPI/master"), QStringLiteral("fastly.jsdelivr.net/gh/N3evin/AmiiboAPI@master"));
    mirror_urls << fastly_url;

    QString gh_url = image_url;
    gh_url.replace(QStringLiteral("cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master"), QStringLiteral("raw.githubusercontent.com/N3evin/AmiiboAPI/master"));
    mirror_urls << gh_url;

    QString proxy_url = QStringLiteral("https://ghproxy.net/") + gh_url;
    mirror_urls << proxy_url;
    mirror_urls.removeDuplicates();

    auto downloadWithFallback = [this, target_label, local_img_path, image_url, mirror_urls](int mirror_index, auto&& self) -> void {
        if (mirror_index >= mirror_urls.size()) {
            target_label->setText(tr("Изображение отсутствует"));
            return;
        }

        QUrl url(mirror_urls[mirror_index]);
        QNetworkRequest req(url);
        req.setTransferTimeout(2500);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 STORM-SWITCH/4.8.8"));
        req.setRawHeader("Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");

        auto* reply = m_network_mgr->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, image_url, local_img_path, target_label, mirror_index, mirror_urls, self]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray img_data = reply->readAll();
                QPixmap pixmap;
                if (pixmap.loadFromData(img_data)) {
                    QFile file(QString::fromStdString(local_img_path.string()));
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(img_data);
                        file.close();
                    }
                    m_image_cache[image_url] = pixmap;
                    target_label->setPixmap(pixmap.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    reply->deleteLater();
                    return;
                }
            }
            reply->deleteLater();
            self(mirror_index + 1, self);
        });
    };

    downloadWithFallback(0, downloadWithFallback);
}

QString ConfigurePerGameAmiibo::GenerateAndSaveAmiiboBin(const AmiiboEntry& entry) {
    QString folder = GetGameAmiiboFolder();
    QString clean_name = entry.name;
    if (clean_name.isEmpty()) clean_name = QStringLiteral("Amiibo");
    clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));

    QString bin_path_str = folder + QStringLiteral("/") + clean_name + QStringLiteral(".bin");
    std::filesystem::path bin_path = bin_path_str.toStdString();

    std::vector<u8> bin_data(540, 0);

    // UID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<u32> dis(0x10, 0xEF);
    bin_data[0] = static_cast<u8>(dis(gen));
    bin_data[1] = static_cast<u8>(dis(gen));
    bin_data[2] = static_cast<u8>(dis(gen));
    bin_data[3] = 0x88;
    bin_data[4] = static_cast<u8>(dis(gen));
    bin_data[5] = static_cast<u8>(dis(gen));
    bin_data[6] = static_cast<u8>(dis(gen));
    bin_data[7] = static_cast<u8>(dis(gen));

    // Nickname
    QString name_to_write = entry.name.left(10);
    for (int i = 0; i < name_to_write.length() && i < 10; ++i) {
        ushort unicode = name_to_write.at(i).unicode();
        bin_data[0x38 + i * 2] = static_cast<u8>((unicode >> 8) & 0xFF);
        bin_data[0x38 + i * 2 + 1] = static_cast<u8>(unicode & 0xFF);
    }

    // Model ID
    bool ok_head = false, ok_tail = false;
    u32 head_val = entry.head.toUInt(&ok_head, 16);
    u32 tail_val = entry.tail.toUInt(&ok_tail, 16);

    bin_data[0x54] = static_cast<u8>((head_val >> 24) & 0xFF);
    bin_data[0x55] = static_cast<u8>((head_val >> 16) & 0xFF);
    bin_data[0x56] = static_cast<u8>((head_val >> 8) & 0xFF);
    bin_data[0x57] = static_cast<u8>(head_val & 0xFF);

    bin_data[0x58] = static_cast<u8>((tail_val >> 24) & 0xFF);
    bin_data[0x59] = static_cast<u8>((tail_val >> 16) & 0xFF);
    bin_data[0x5A] = static_cast<u8>((tail_val >> 8) & 0xFF);
    bin_data[0x5B] = static_cast<u8>(tail_val & 0xFF);

    // Save to disk
    std::ofstream out(bin_path, std::ios::binary);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(bin_data.data()), bin_data.size());
        out.close();
    }

    return bin_path_str;
}

void ConfigurePerGameAmiibo::OnOpenAmiiboFolderClicked() {
    QString folder = GetGameAmiiboFolder();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void ConfigurePerGameAmiibo::ApplyConfiguration() {
    // Explicitly synchronize current checkbox states with game folder
    QString folder = GetGameAmiiboFolder();
    for (int i = 0; i < m_amiibo_list->count(); ++i) {
        auto* item = m_amiibo_list->item(i);
        if (!item) continue;
        int idx = item->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= static_cast<int>(m_all_amiibos.size())) continue;
        const auto& entry = m_all_amiibos[idx];

        QString clean_name = entry.name;
        if (clean_name.isEmpty()) clean_name = QStringLiteral("Amiibo");
        clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
        QString bin_path = folder + QStringLiteral("/") + clean_name + QStringLiteral(".bin");

        if (item->checkState() == Qt::Checked) {
            if (!QFile::exists(bin_path)) {
                GenerateAndSaveAmiiboBin(entry);
            }
        } else {
            if (QFile::exists(bin_path)) {
                QFile::remove(bin_path);
            }
        }
    }
}

void ConfigurePerGameAmiibo::SetGameInfo(u64 title_id, const QString& game_name) {
    m_title_id = title_id;
    m_game_name = game_name;
    if (m_amiibo_list && m_amiibo_list->currentItem()) {
        OnItemSelected(m_amiibo_list->currentItem(), nullptr);
    }
}
