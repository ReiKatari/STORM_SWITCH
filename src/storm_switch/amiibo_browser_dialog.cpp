// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/amiibo_browser_dialog.h"

#include <algorithm>
#include <filesystem>
#include <random>
#include <QBoxLayout>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QUrl>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "core/core.h"

AmiiboBrowserDialog::AmiiboBrowserDialog(QWidget* parent, Core::System& system, const QString& initial_game_hint)
    : QDialog(parent), m_system(system), m_network_mgr(new QNetworkAccessManager(this)), m_initial_game_hint(initial_game_hint) {
    SetupUi();
    FetchAmiiboDatabase();
}

AmiiboBrowserDialog::~AmiiboBrowserDialog() = default;

void AmiiboBrowserDialog::SetupUi() {
    setWindowTitle(tr("Онлайн-база и менеджер Amiibo — STORM SWITCH"));
    resize(980, 680);
    setMinimumSize(850, 560);

    // Apply dark cyber styling
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #0b0e14;"
        "  color: #e2e8f0;"
        "}"
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 6px;"
        "  margin-top: 10px;"
        "  padding-top: 14px;"
        "  background-color: #111622;"
        "  color: #00f0ff;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 6px;"
        "  color: #00f0ff;"
        "}"
        "QLineEdit, QComboBox {"
        "  background-color: #161d2d;"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 4px;"
        "  padding: 5px 8px;"
        "  color: #ffffff;"
        "  font-size: 9pt;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "  border: 1px solid #00f0ff;"
        "  background-color: #1a2336;"
        "}"
        "QListWidget {"
        "  background-color: #111622;"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 6px;"
        "  color: #ffffff;"
        "  padding: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 4px;"
        "  margin-bottom: 2px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: rgba(0, 240, 255, 0.12);"
        "  color: #00f0ff;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(0, 240, 255, 0.25);"
        "  color: #ffffff;"
        "  border: 1px solid #00f0ff;"
        "}"
        "QPushButton {"
        "  background-color: #1a2336;"
        "  border: 1px solid rgba(0, 240, 255, 0.35);"
        "  border-radius: 5px;"
        "  color: #ffffff;"
        "  padding: 6px 14px;"
        "  font-weight: bold;"
        "  font-size: 8.5pt;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(0, 240, 255, 0.20);"
        "  border-color: #00f0ff;"
        "  color: #00f0ff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #00f0ff;"
        "  color: #000000;"
        "}"
        "QTextEdit {"
        "  background-color: #161d2d;"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 4px;"
        "  color: #cbd5e1;"
        "  font-size: 8.5pt;"
        "}"
        "QProgressBar {"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 3px;"
        "  text-align: center;"
        "  background-color: #111622;"
        "  color: #ffffff;"
        "  max-height: 14px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #00f0ff;"
        "  border-radius: 2px;"
        "}"
    ));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(8);

    // Top Filter Bar
    auto* top_filter_box = new QWidget(this);
    auto* filter_layout = new QHBoxLayout(top_filter_box);
    filter_layout->setContentsMargins(0, 0, 0, 0);
    filter_layout->setSpacing(8);

    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText(tr("🔍 Поиск по имени фигурки или игровой серии (напр. Zelda, Mario, Samus)..."));
    m_search_edit->setClearButtonEnabled(true);
    connect(m_search_edit, &QLineEdit::textChanged, this, &AmiiboBrowserDialog::OnSearchFilterChanged);
    filter_layout->addWidget(m_search_edit, 3);

    m_series_combo = new QComboBox(this);
    m_series_combo->addItem(tr("Все серии"));
    connect(m_series_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AmiiboBrowserDialog::OnSearchFilterChanged);
    filter_layout->addWidget(m_series_combo, 2);

    m_type_combo = new QComboBox(this);
    m_type_combo->addItems({tr("Все типы"), tr("Figure (Фигурка)"), tr("Card (Карта)"), tr("Yarn (Пряжа)")});
    connect(m_type_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AmiiboBrowserDialog::OnSearchFilterChanged);
    filter_layout->addWidget(m_type_combo, 1);

    m_refresh_btn = new QPushButton(tr("🔄 Обновить базу"), this);
    connect(m_refresh_btn, &QPushButton::clicked, this, &AmiiboBrowserDialog::OnRefreshClicked);
    filter_layout->addWidget(m_refresh_btn, 0);

    main_layout->addWidget(top_filter_box);

    // Splitter with List (left) and Inspector (right)
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);

    // Left container: Amiibo List + count
    auto* left_container = new QWidget(this);
    auto* left_layout = new QVBoxLayout(left_container);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(4);

    m_amiibo_list = new QListWidget(this);
    connect(m_amiibo_list, &QListWidget::currentItemChanged, this, &AmiiboBrowserDialog::OnItemSelected);
    left_layout->addWidget(m_amiibo_list);

    splitter->addWidget(left_container);

    // Right container: Inspector / Details
    auto* right_container = new QWidget(this);
    auto* right_layout = new QVBoxLayout(right_container);
    right_layout->setContentsMargins(8, 0, 0, 0);
    right_layout->setSpacing(8);

    auto* details_group = new QGroupBox(tr("Информация об Amiibo"), this);
    auto* details_layout = new QVBoxLayout(details_group);
    details_layout->setContentsMargins(10, 14, 10, 10);
    details_layout->setSpacing(6);

    // Image preview centered
    m_image_label = new QLabel(this);
    m_image_label->setMinimumSize(180, 180);
    m_image_label->setMaximumHeight(220);
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_label->setStyleSheet(QStringLiteral("background-color: #0c1018; border: 1px solid rgba(255,255,255,0.08); border-radius: 8px;"));
    m_image_label->setText(tr("Выберите Amiibo для просмотра"));
    details_layout->addWidget(m_image_label);

    m_name_label = new QLabel(tr("Имя: —"), this);
    m_name_label->setStyleSheet(QStringLiteral("font-size: 11pt; font-weight: bold; color: #00f0ff;"));
    details_layout->addWidget(m_name_label);

    m_series_label = new QLabel(tr("Серия Amiibo: —"), this);
    details_layout->addWidget(m_series_label);

    m_game_series_label = new QLabel(tr("Игровая вселенная: —"), this);
    details_layout->addWidget(m_game_series_label);

    m_type_label = new QLabel(tr("Тип: —"), this);
    details_layout->addWidget(m_type_label);

    m_id_label = new QLabel(tr("ID (Head/Tail): —"), this);
    m_id_label->setStyleSheet(QStringLiteral("color: #718096; font-family: monospace; font-size: 8pt;"));
    details_layout->addWidget(m_id_label);

    m_status_badge = new QLabel(tr("Статус: Ожидание"), this);
    m_status_badge->setStyleSheet(QStringLiteral("color: #a0aec0; font-weight: bold; padding: 2px 6px; background-color: #1a2336; border-radius: 3px;"));
    details_layout->addWidget(m_status_badge);

    // Target game selector
    auto* target_game_box = new QWidget(this);
    auto* tg_layout = new QHBoxLayout(target_game_box);
    tg_layout->setContentsMargins(0, 4, 0, 4);
    tg_layout->setSpacing(6);

    auto* tg_title = new QLabel(tr("🎮 Выбранная игра:"), this);
    tg_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #00f0ff;"));
    tg_layout->addWidget(tg_title);

    m_target_game_combo = new QComboBox(this);
    const QStringList target_games = {
        QStringLiteral("The Legend of Zelda: Tears of the Kingdom"),
        QStringLiteral("The Legend of Zelda: Breath of the Wild"),
        QStringLiteral("The Legend of Zelda: Echoes of Wisdom"),
        QStringLiteral("Super Mario Odyssey"),
        QStringLiteral("Super Smash Bros. Ultimate"),
        QStringLiteral("Metroid Dread"),
        QStringLiteral("Splatoon 3"),
        QStringLiteral("Monster Hunter Rise / Sunbreak"),
        QStringLiteral("Animal Crossing: New Horizons"),
        QStringLiteral("Xenoblade Chronicles 3"),
        QStringLiteral("Fire Emblem Engage"),
        QStringLiteral("Mario Kart 8 Deluxe"),
        QStringLiteral("Kirby and the Forgotten Land"),
        QStringLiteral("Hyrule Warriors: Age of Calamity"),
        QStringLiteral("Все игры (Универсальная поддержка)")
    };
    m_target_game_combo->addItems(target_games);
    connect(m_target_game_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AmiiboBrowserDialog::OnTargetGameChanged);
    tg_layout->addWidget(m_target_game_combo, 1);
    details_layout->addWidget(target_game_box);

    if (!m_initial_game_hint.isEmpty()) {
        const QString hint_lower = m_initial_game_hint.toLower();
        if (hint_lower.contains(QStringLiteral("tears")) || hint_lower.contains(QStringLiteral("totk")) || hint_lower.contains(QStringLiteral("0100f2c0115b6000"))) {
            m_target_game_combo->setCurrentIndex(0);
        } else if (hint_lower.contains(QStringLiteral("breath")) || hint_lower.contains(QStringLiteral("botw")) || hint_lower.contains(QStringLiteral("01007ef00011e000"))) {
            m_target_game_combo->setCurrentIndex(1);
        } else if (hint_lower.contains(QStringLiteral("echoes")) || hint_lower.contains(QStringLiteral("01008cf01baac000"))) {
            m_target_game_combo->setCurrentIndex(2);
        } else if (hint_lower.contains(QStringLiteral("odyssey")) || hint_lower.contains(QStringLiteral("0100000000010000"))) {
            m_target_game_combo->setCurrentIndex(3);
        } else if (hint_lower.contains(QStringLiteral("smash")) || hint_lower.contains(QStringLiteral("01006a800016e000"))) {
            m_target_game_combo->setCurrentIndex(4);
        } else if (hint_lower.contains(QStringLiteral("dread")) || hint_lower.contains(QStringLiteral("010093801237c000"))) {
            m_target_game_combo->setCurrentIndex(5);
        } else if (hint_lower.contains(QStringLiteral("splatoon")) || hint_lower.contains(QStringLiteral("0100c2500fc20000"))) {
            m_target_game_combo->setCurrentIndex(6);
        } else if (hint_lower.contains(QStringLiteral("monster hunter")) || hint_lower.contains(QStringLiteral("0100559011740000"))) {
            m_target_game_combo->setCurrentIndex(7);
        } else if (hint_lower.contains(QStringLiteral("animal crossing")) || hint_lower.contains(QStringLiteral("01006f8002326000"))) {
            m_target_game_combo->setCurrentIndex(8);
        } else if (hint_lower.contains(QStringLiteral("xenoblade")) || hint_lower.contains(QStringLiteral("010074f013262000"))) {
            m_target_game_combo->setCurrentIndex(9);
        } else if (hint_lower.contains(QStringLiteral("fire emblem")) || hint_lower.contains(QStringLiteral("0100a6301214e000"))) {
            m_target_game_combo->setCurrentIndex(10);
        } else if (hint_lower.contains(QStringLiteral("mario kart")) || hint_lower.contains(QStringLiteral("0100152000022000"))) {
            m_target_game_combo->setCurrentIndex(11);
        } else if (hint_lower.contains(QStringLiteral("kirby")) || hint_lower.contains(QStringLiteral("01004d300c5ae000"))) {
            m_target_game_combo->setCurrentIndex(12);
        }
    }

    // Elevated 3D Reward Card
    m_reward_card = new QWidget(this);
    m_reward_card->setStyleSheet(QStringLiteral(
        "QWidget#AmiiboRewardCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #121a29, stop:1 #0c121d);"
        "  border: 1px solid #00f0ff;"
        "  border-radius: 8px;"
        "}"
    ));
    m_reward_card->setObjectName(QStringLiteral("AmiiboRewardCard"));
    auto* card_layout = new QVBoxLayout(m_reward_card);
    card_layout->setContentsMargins(10, 8, 10, 8);
    card_layout->setSpacing(4);

    auto* card_header = new QHBoxLayout();
    auto* card_title = new QLabel(tr("🎁 Награда в этой игре:"), m_reward_card);
    card_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #ffca28; font-size: 9.5pt;"));
    card_header->addWidget(card_title);

    m_reward_category_badge = new QLabel(tr("УНИВЕРСАЛЬНЫЙ БОНУС"), m_reward_card);
    m_reward_category_badge->setStyleSheet(QStringLiteral(
        "background-color: #004d40; color: #00f0ff; border: 1px solid #00f0ff; border-radius: 4px; padding: 2px 6px; font-weight: bold; font-size: 8pt;"
    ));
    card_header->addWidget(m_reward_category_badge, 0, Qt::AlignRight);
    card_layout->addLayout(card_header);

    m_reward_name_label = new QLabel(tr("Выберите фигурку для просмотра наград"), m_reward_card);
    m_reward_name_label->setStyleSheet(QStringLiteral("font-size: 10pt; font-weight: bold; color: #00f0ff;"));
    m_reward_name_label->setWordWrap(true);
    card_layout->addWidget(m_reward_name_label);

    m_reward_desc_label = new QLabel(m_reward_card);
    m_reward_desc_label->setStyleSheet(QStringLiteral("color: #cbd5e1; font-size: 8.5pt; line-height: 1.3;"));
    m_reward_desc_label->setWordWrap(true);
    card_layout->addWidget(m_reward_desc_label);

    details_layout->addWidget(m_reward_card);

    // Games compatibility
    auto* games_label = new QLabel(tr("🎮 Поддерживаемые игры на Nintendo Switch:"), this);
    games_label->setStyleSheet(QStringLiteral("font-weight: bold; color: #ffca28; margin-top: 4px;"));
    details_layout->addWidget(games_label);

    m_games_text = new QTextEdit(this);
    m_games_text->setReadOnly(true);
    m_games_text->setPlaceholderText(tr("Список совместимых игр загружается или отсутствует"));
    details_layout->addWidget(m_games_text, 1);

    // Action buttons inside details
    auto* act_btn_layout = new QHBoxLayout();
    act_btn_layout->setSpacing(6);

    m_save_btn = new QPushButton(tr("💾 Сохранить Amiibo (.bin)"), this);
    m_save_btn->setCursor(Qt::PointingHandCursor);
    m_save_btn->setEnabled(false);
    m_save_btn->setStyleSheet(QStringLiteral("background-color: #004d40; border-color: #00e676; color: #ffffff;"));
    connect(m_save_btn, &QPushButton::clicked, this, &AmiiboBrowserDialog::OnSaveAmiiboClicked);
    act_btn_layout->addWidget(m_save_btn);

    m_load_btn = new QPushButton(tr("⚡ Загрузить в игру"), this);
    m_load_btn->setCursor(Qt::PointingHandCursor);
    m_load_btn->setEnabled(false);
    m_load_btn->setStyleSheet(QStringLiteral("background-color: #006064; border-color: #00e5ff; color: #ffffff;"));
    connect(m_load_btn, &QPushButton::clicked, this, &AmiiboBrowserDialog::OnLoadAmiiboClicked);
    act_btn_layout->addWidget(m_load_btn);

    m_disconnect_btn = new QPushButton(tr("❌ Отключить Amiibo"), this);
    m_disconnect_btn->setCursor(Qt::PointingHandCursor);
    m_disconnect_btn->setStyleSheet(QStringLiteral("background-color: #3b1111; border-color: #ff5252; color: #ff8a80;"));
    connect(m_disconnect_btn, &QPushButton::clicked, this, &AmiiboBrowserDialog::OnDisconnectAmiiboClicked);
    act_btn_layout->addWidget(m_disconnect_btn);

    details_layout->addLayout(act_btn_layout);

    right_layout->addWidget(details_group);
    splitter->addWidget(right_container);

    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 6);
    main_layout->addWidget(splitter, 1);

    // Bottom Status & Controls
    auto* bottom_bar = new QHBoxLayout();
    bottom_bar->setContentsMargins(0, 0, 0, 0);

    m_status_label = new QLabel(tr("Подключение к базе Amiibo..."), this);
    m_status_label->setStyleSheet(QStringLiteral("color: #718096; font-size: 8.5pt;"));
    bottom_bar->addWidget(m_status_label, 1);

    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 0); // Busy indicator initially
    m_progress_bar->setFixedWidth(140);
    bottom_bar->addWidget(m_progress_bar);

    m_open_folder_btn = new QPushButton(tr("📁 Открыть папку Amiibo"), this);
    connect(m_open_folder_btn, &QPushButton::clicked, this, &AmiiboBrowserDialog::OnOpenAmiiboFolderClicked);
    bottom_bar->addWidget(m_open_folder_btn);

    auto* close_btn = new QPushButton(tr("Закрыть"), this);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
    bottom_bar->addWidget(close_btn);

    main_layout->addLayout(bottom_bar);
}

void AmiiboBrowserDialog::FetchAmiiboDatabase() {
    m_status_label->setText(tr("Загрузка каталога Amiibo из сети..."));
    m_progress_bar->setVisible(true);
    m_progress_bar->setRange(0, 0);

    const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
    const auto cache_file = cache_dir / "amiibo_cache.json";

    // Try reading cache if available first to show immediately
    if (std::filesystem::exists(cache_file)) {
        QFile file(QString::fromStdString(cache_file.string()));
        if (file.open(QIODevice::ReadOnly)) {
            ParseDatabaseJson(file.readAll());
            file.close();
            if (!m_all_amiibos.empty()) {
                PopulateSeriesFilter();
                ApplyFilters();
                m_status_label->setText(tr("Загружено из кэша: %1 Amiibo. Обновление из сети...").arg(m_all_amiibos.size()));
            }
        }
    }

    const QStringList urls = {
        QStringLiteral("https://www.amiiboapi.com/api/amiibo/"),
        QStringLiteral("https://raw.githubusercontent.com/N3evin/AmiiboAPI/master/database/amiibo.json"),
        QStringLiteral("https://cdn.jsdelivr.net/gh/N3evin/AmiiboAPI@master/database/amiibo.json")
    };

    auto tryFetchUrl = [this, urls, cache_file](auto self, int url_index) -> void {
        if (url_index >= urls.size()) {
            m_progress_bar->setVisible(false);
            if (m_all_amiibos.empty()) {
                m_status_label->setText(tr("Не удалось загрузить каталог Amiibo из всех источников"));
            } else {
                m_status_label->setText(tr("Автономный режим. Доступно из кэша: %1 Amiibo").arg(m_all_amiibos.size()));
            }
            return;
        }

        QUrl url(urls[url_index]);
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36 STORM-EDEN/4.0.0");
        request.setRawHeader("Accept", "application/json, text/plain, */*");

        auto* reply = m_network_mgr->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, self, url_index, urls, cache_file]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                m_progress_bar->setVisible(false);
                ParseDatabaseJson(data);
                if (!m_all_amiibos.empty()) {
                    QFile file(QString::fromStdString(cache_file.string()));
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(data);
                        file.close();
                    }
                    PopulateSeriesFilter();
                    ApplyFilters();
                    m_status_label->setText(tr("Каталог успешно обновлен. Всего доступно: %1 Amiibo").arg(m_all_amiibos.size()));
                    reply->deleteLater();
                    return;
                }
            }
            reply->deleteLater();
            self(self, url_index + 1);
        });
    };

    tryFetchUrl(tryFetchUrl, 0);
}

void AmiiboBrowserDialog::ParseDatabaseJson(const QByteArray& json_data) {
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(json_data, &err);
    if (err.error != QJsonParseError::NoError) {
        return;
    }

    std::vector<AmiiboEntry> parsed_list;

    auto populateSwitchGames = [](AmiiboEntry& entry) {
        if (!entry.switch_games.isEmpty()) return;
        if (entry.game_series.contains(QStringLiteral("Zelda"), Qt::CaseInsensitive) ||
            entry.amiibo_series.contains(QStringLiteral("Zelda"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• The Legend of Zelda: Tears of the Kingdom: Эксклюзивная ткань для параплана, редкое оружие, материалы и снаряжение"));
            entry.switch_games.append(QStringLiteral("• The Legend of Zelda: Breath of the Wild: Специальное оружие, доспехи, призыв Эпоны / Волка Линка, сундуки"));
            entry.switch_games.append(QStringLiteral("• The Legend of Zelda: Echoes of Wisdom: Уникальные костюмы, аксессуары и ресурсы"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Обучаемый боец FP (Figure Player) с настраиваемыми духами"));
            entry.switch_games.append(QStringLiteral("• Mario Kart 8 Deluxe: Специальный гоночный костюм Mii"));
            entry.switch_games.append(QStringLiteral("• Hyrule Warriors: Age of Calamity: Высокоуровневое оружие и материалы"));
        } else if (entry.game_series.contains(QStringLiteral("Mario"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Mario"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Super Mario Odyssey: Уникальные костюмы для Марио и подсказки Лун энергии у дядюшки Amiibo"));
            entry.switch_games.append(QStringLiteral("• Super Mario 3D World + Bowser's Fury: Костюм Белого Тануки Неуязвимости, суперзвезды и бонусы"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Обучаемый боец FP (Figure Player) с прокачкой 1-50 ур."));
            entry.switch_games.append(QStringLiteral("• Mario Kart 8 Deluxe: Гоночный костюм Mii Mario/Luigi/Peach/Bowser"));
            entry.switch_games.append(QStringLiteral("• Mario Party Superstars / Jamboree: Дополнительные монеты и стикеры"));
        } else if (entry.game_series.contains(QStringLiteral("Splatoon"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Splatoon"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Splatoon 3: Эксклюзивные наборы экипировки (снаряжение), совместные фотосессии и сохраненные настройки"));
            entry.switch_games.append(QStringLiteral("• Splatoon 2: Эксклюзивное снаряжение и треки кальмаро-радио"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Боец FP Инклинг"));
        } else if (entry.game_series.contains(QStringLiteral("Metroid"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Metroid"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Metroid Dread: Дополнительный контейнер энергии (Energy Tank) и ежедневное пополнение ракет/здоровья"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Боец FP Самус / Темная Самус / Ридли"));
        } else if (entry.game_series.contains(QStringLiteral("Monster Hunter"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Monster Hunter"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Monster Hunter Rise / Sunbreak: Специальная многослойная броня (Layered Armor) и ежедневная лотерея Кагари"));
            entry.switch_games.append(QStringLiteral("• Monster Hunter Stories 2: Уникальные костюмы наездника"));
        } else if (entry.game_series.contains(QStringLiteral("Animal Crossing"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Animal Crossing"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Animal Crossing: New Horizons: Приглашение жителя на кемпинг, эксклюзивные плакаты, фотостудия острова Харви и кофейня наседки"));
        } else if (entry.game_series.contains(QStringLiteral("Kirby"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Kirby"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Kirby and the Forgotten Land: Дополнительные монеты Звезд, ускорение и лечебная еда"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Боец FP Кирби / Метанайт / Король Дидиди"));
        } else if (entry.game_series.contains(QStringLiteral("Fire Emblem"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Fire Emblem"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Fire Emblem Engage: Музыкальные треки из прошлых частей, билеты на наряды и релейные билеты"));
            entry.switch_games.append(QStringLiteral("• Fire Emblem: Three Houses: Музыка в беседке Amiibo и редкие предметы"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Боец FP"));
        } else if (entry.game_series.contains(QStringLiteral("Xenoblade"), Qt::CaseInsensitive) ||
                   entry.amiibo_series.contains(QStringLiteral("Xenoblade"), Qt::CaseInsensitive)) {
            entry.switch_games.append(QStringLiteral("• Xenoblade Chronicles 3: Облик Меча Монадо (Monado Skin) и полезные расходники"));
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Боец FP Шулк / Пайра / Мифра"));
        } else {
            entry.switch_games.append(QStringLiteral("• Super Smash Bros. Ultimate: Обучаемый боец FP или получение бонусов/духов"));
            entry.switch_games.append(QStringLiteral("• Mario Kart 8 Deluxe: Гоночный костюм Mii (для совместимых персонажей)"));
            entry.switch_games.append(QStringLiteral("• Универсальная поддержка: Совместимо со всеми играми Nintendo Switch, поддерживающими любые Amiibo (бонусы, ресурсы, монеты)"));
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

        QJsonObject rel = obj.value(QStringLiteral("release")).toObject();
        entry.release_na = rel.value(QStringLiteral("na")).toString();
        entry.release_jp = rel.value(QStringLiteral("jp")).toString();
        entry.release_eu = rel.value(QStringLiteral("eu")).toString();

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
            parsed_list.reserve(arr.size());
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

            parsed_list.reserve(dict.size());
            for (auto it = dict.begin(); it != dict.end(); ++it) {
                if (it.value().isObject()) {
                    AmiiboEntry e = parseObject(it.value().toObject());
                    QString key = it.key();
                    if (key.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
                        key = key.mid(2);
                    }
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
        } else if (root.contains(QStringLiteral("amiibos")) && root.value(QStringLiteral("amiibos")).isArray()) {
            QJsonArray arr = root.value(QStringLiteral("amiibos")).toArray();
            parsed_list.reserve(arr.size());
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
        }
    } else if (doc.isArray()) {
        QJsonArray arr = doc.array();
        parsed_list.reserve(arr.size());
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
    }

    if (!parsed_list.empty()) {
        m_all_amiibos = std::move(parsed_list);
    }
}

void AmiiboBrowserDialog::PopulateSeriesFilter() {
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
    if (idx >= 0) {
        m_series_combo->setCurrentIndex(idx);
    }
    m_series_combo->blockSignals(false);
}

void AmiiboBrowserDialog::OnSearchFilterChanged() {
    ApplyFilters();
}

void AmiiboBrowserDialog::ApplyFilters() {
    m_amiibo_list->clear();
    m_filtered_indices.clear();

    const QString search_text = m_search_edit->text().trimmed().toLower();
    const QString series_filter = m_series_combo->currentText();
    const int type_idx = m_type_combo->currentIndex();

    for (size_t i = 0; i < m_all_amiibos.size(); ++i) {
        const auto& a = m_all_amiibos[i];

        if (m_series_combo->currentIndex() > 0 && a.amiibo_series != series_filter) {
            continue;
        }

        if (type_idx == 1 && !a.type.contains(QStringLiteral("Figure"), Qt::CaseInsensitive)) continue;
        if (type_idx == 2 && !a.type.contains(QStringLiteral("Card"), Qt::CaseInsensitive)) continue;
        if (type_idx == 3 && !a.type.contains(QStringLiteral("Yarn"), Qt::CaseInsensitive)) continue;

        if (!search_text.isEmpty()) {
            bool matches = a.name.toLower().contains(search_text) ||
                           a.character.toLower().contains(search_text) ||
                           a.game_series.toLower().contains(search_text) ||
                           a.amiibo_series.toLower().contains(search_text);
            if (!matches) {
                continue;
            }
        }

        m_filtered_indices.push_back(static_cast<int>(i));

        auto* item = new QListWidgetItem(QStringLiteral("%1 [%2]").arg(a.name, a.amiibo_series));
        item->setData(Qt::UserRole, static_cast<int>(i));
        m_amiibo_list->addItem(item);
    }

    if (m_amiibo_list->count() > 0) {
        m_amiibo_list->setCurrentRow(0);
    } else {
        m_name_label->setText(tr("Ничего не найдено"));
        m_series_label->setText(QString());
        m_game_series_label->setText(QString());
        m_type_label->setText(QString());
        m_id_label->setText(QString());
        m_status_badge->setText(tr("Статус: Нет результатов"));
        m_games_text->clear();
        m_image_label->setText(tr("Нет данных"));
        m_save_btn->setEnabled(false);
        m_load_btn->setEnabled(false);
    }
}

void AmiiboBrowserDialog::OnTargetGameChanged(int index) {
    if (m_amiibo_list && m_amiibo_list->currentItem()) {
        int idx = m_amiibo_list->currentItem()->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < static_cast<int>(m_all_amiibos.size())) {
            UpdateRewardCard(m_all_amiibos[idx]);
        }
    }
}

void AmiiboBrowserDialog::OnItemSelected(QListWidgetItem* current, QListWidgetItem* previous) {
    if (!current) return;
    int idx = current->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx < static_cast<int>(m_all_amiibos.size())) {
        DisplayAmiiboDetails(m_all_amiibos[idx]);
    }
}

void AmiiboBrowserDialog::DisplayAmiiboDetails(const AmiiboEntry& entry) {
    m_name_label->setText(tr("Имя: %1 (%2)").arg(entry.name, entry.character));
    m_series_label->setText(tr("Серия Amiibo: %1").arg(entry.amiibo_series));
    m_game_series_label->setText(tr("Игровая вселенная: %1").arg(entry.game_series));
    m_type_label->setText(tr("Тип: %1").arg(entry.type));
    m_id_label->setText(tr("Model ID: Head %1 | Tail %2").arg(entry.head, entry.tail));

    // Check if file already exists locally
    const auto amiibo_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::AmiiboDir);
    QString clean_series = entry.amiibo_series;
    clean_series.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    QString clean_name = entry.name;
    clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    auto local_path = amiibo_dir / clean_series.toStdString() / (clean_name.toStdString() + ".bin");

    if (std::filesystem::exists(local_path)) {
        m_status_badge->setText(tr("Статус: ✅ Установлено локально (%1)").arg(clean_name + QStringLiteral(".bin")));
        m_status_badge->setStyleSheet(QStringLiteral("color: #00e676; font-weight: bold; padding: 2px 6px; background-color: #004d40; border-radius: 3px;"));
    } else {
        m_status_badge->setText(tr("Статус: 🌐 Доступно для загрузки"));
        m_status_badge->setStyleSheet(QStringLiteral("color: #00e5ff; font-weight: bold; padding: 2px 6px; background-color: #006064; border-radius: 3px;"));
    }

    // Update target game specific reward card
    UpdateRewardCard(entry);

    // Switch games list
    if (entry.switch_games.isEmpty()) {
        m_games_text->setPlainText(tr("Совместимо с универсальными играми Nintendo Switch (Super Smash Bros., Zelda, Mario Kart 8, и др.)."));
    } else {
        m_games_text->setPlainText(entry.switch_games.join(QStringLiteral("\n\n")));
    }

    // Fetch and display image
    FetchImage(entry.image_url, m_image_label);

    m_save_btn->setEnabled(true);
    m_load_btn->setEnabled(true);
}

void AmiiboBrowserDialog::UpdateRewardCard(const AmiiboEntry& entry) {
    if (!m_target_game_combo || !m_reward_category_badge || !m_reward_name_label || !m_reward_desc_label) return;
    const QString current_game = m_target_game_combo->currentText();
    const auto reward = GetRewardForGame(entry, current_game);

    m_reward_category_badge->setText(QStringLiteral("%1 %2").arg(reward.icon_emoji, reward.category.toUpper()));
    m_reward_name_label->setText(reward.item_name);
    m_reward_desc_label->setText(reward.description);
}

AmiiboRewardInfo AmiiboBrowserDialog::GetRewardForGame(const AmiiboEntry& entry, const QString& game_name) {
    const QString n = entry.name.toLower();
    const QString c = entry.character.toLower();
    const QString gs = entry.game_series.toLower();
    const QString as = entry.amiibo_series.toLower();
    const QString gn = game_name.toLower();

    // 1. Zelda: Tears of the Kingdom
    if (gn.contains(QStringLiteral("tears")) || gn.contains(QStringLiteral("totk"))) {
        if (n.contains(QStringLiteral("tears")) || (c.contains(QStringLiteral("link")) && n.contains(QStringLiteral("totk")))) {
            return {QObject::tr("Ткань параплана и оружие"), QStringLiteral("🪂"),
                    QObject::tr("Новая ткань чемпионов и Меч рыцаря"),
                    QObject::tr("Эксклюзивная ткань для параплана «Новая туника чемпионов», крепкий Меч рыцаря, целебные травы и мясо")};
        }
        if (n.contains(QStringLiteral("ocarina")) || n.contains(QStringLiteral("time"))) {
            return {QObject::tr("Оружие, броня и ткань"), QStringLiteral("🗡️"),
                    QObject::tr("Меч Большого Горона и Сет Времени"),
                    QObject::tr("Двуручный Меч Большого Горона, Шапка/Туника/Штаны Времени, Ткань маски Лони-Лони")};
        }
        if (n.contains(QStringLiteral("majora"))) {
            return {QObject::tr("Оружие, броня и ткань"), QStringLiteral("🗡️"),
                    QObject::tr("Двуручник и Сет Свирепого Божества"),
                    QObject::tr("Маска, Нагрудник и Поножи Свирепого Божества, Двуручник Свирепого Божества, Ткань маски Маджоры")};
        }
        if (n.contains(QStringLiteral("twilight")) || (c.contains(QStringLiteral("link")) && as.contains(QStringLiteral("smash")))) {
            return {QObject::tr("Скакун, броня и ткань"), QStringLiteral("🐺"),
                    QObject::tr("Легендарная кобыла Эпона и Сет Сумерек"),
                    QObject::tr("Призыв легендарной лошади Эпоны (максимальные характеристики), Шапка/Туника/Штаны Сумерек, Сумеречная ткань")};
        }
        if (n.contains(QStringLiteral("skyward"))) {
            return {QObject::tr("Оружие, броня и ткань"), QStringLiteral("🗡️"),
                    QObject::tr("Белый меч Небес и Сет Неба"),
                    QObject::tr("Белый меч Богини, Шапка/Туника/Штаны Неба, ткань Меча Небес")};
        }
        if (n.contains(QStringLiteral("awakening"))) {
            return {QObject::tr("Броня и ткань"), QStringLiteral("🛡️"),
                    QObject::tr("Сет Пробуждения и Ткань Яйца"),
                    QObject::tr("Мультяшная маска и туника Пробуждения, эксклюзивная ткань Яйца Ветрорыба, солдатские стрелы")};
        }
        if (n.contains(QStringLiteral("archer"))) {
            return {QObject::tr("Оружие и ткань"), QStringLiteral("🏹"),
                    QObject::tr("Ткань капюшона лучника и Королевский лук"),
                    QObject::tr("Эксклюзивная ткань лучника, Королевский лук, сырая дичь и редкая рыба")};
        }
        if (n.contains(QStringLiteral("rider"))) {
            return {QObject::tr("Ткань и снаряжение"), QStringLiteral("🪂"),
                    QObject::tr("Ткань капюшона всадника и Солдатский палаш"),
                    QObject::tr("Ткань всадника, оружие всадника, целебные грибы")};
        }
        if (c.contains(QStringLiteral("wolf link")) || n.contains(QStringLiteral("wolf"))) {
            return {QObject::tr("Охотничьи ресурсы"), QStringLiteral("📦"),
                    QObject::tr("Охотничий провиант Линка"),
                    QObject::tr("Огромный запас свежего мяса, птицы и дичи для кулинарии")};
        }
        if (c.contains(QStringLiteral("zelda"))) {
            return {QObject::tr("Ткань и оружие"), QStringLiteral("🪂"),
                    QObject::tr("Ткань принцессы Зельды и Королевский лук"),
                    QObject::tr("Эксклюзивная ткань принцессы Зельды, древний или королевский лук, драгоценные камни и травы")};
        }
        if (c.contains(QStringLiteral("ganondorf")) || n.contains(QStringLiteral("ganon"))) {
            return {QObject::tr("Оружие и ткань"), QStringLiteral("🗡️"),
                    QObject::tr("Меч и Ткань Демонического Короля"),
                    QObject::tr("Эксклюзивная ткань Демонического Короля, Меч Демонического Короля / Меч сумерек, запеченное мясо и самоцветы")};
        }
        if (c.contains(QStringLiteral("sheik"))) {
            return {QObject::tr("Броня и ткань"), QStringLiteral("🛡️"),
                    QObject::tr("Маска Шейха и Ткань клана Шеика"),
                    QObject::tr("Маска Шейха (бонус к скрытности), Ткань клана Шеика, ножи и щиты")};
        }
        if (c.contains(QStringLiteral("guardian"))) {
            return {QObject::tr("Ткань и механизмы"), QStringLiteral("🪂"),
                    QObject::tr("Древняя ткань Шеика и Древние стрелы"),
                    QObject::tr("Древняя ткань стража, металлические ящики, древние механизмы и древние клинки")};
        }
        if (c.contains(QStringLiteral("bokoblin"))) {
            return {QObject::tr("Ткань и оружие"), QStringLiteral("🪂"),
                    QObject::tr("Ткань Бокоблина и Боко-щиты"),
                    QObject::tr("Ткань Бокоблина, шипастый боко-щит, дубина и сырое мясо")};
        }
        if (c.contains(QStringLiteral("daruk")) || c.contains(QStringLiteral("mipha")) ||
            c.contains(QStringLiteral("revali")) || c.contains(QStringLiteral("urbosa"))) {
            return {QObject::tr("Божественный шлем и ткань"), QStringLiteral("🛡️"),
                    QObject::tr("Шлем Божественного чудища и Ткань чемпиона"),
                    QObject::tr("Уникальный Божественный шлем со стихийной защитой и персональная ткань чемпиона")};
        }
        if (gs.contains(QStringLiteral("zelda")) || as.contains(QStringLiteral("zelda"))) {
            return {QObject::tr("Оружие и ткань"), QStringLiteral("🗡️"),
                    QObject::tr("Тематическая ткань параплана и оружие"),
                    QObject::tr("Эксклюзивная ткань параплана, сундук с высокоуровневым оружием и самоцветами")};
        }
        return {QObject::tr("Универсальные ресурсы"), QStringLiteral("📦"),
                QObject::tr("Припасы путешественника"),
                QObject::tr("Сундук со случайным оружием, стрелы, целебные травы, яблоки, мясо и рыба")};
    }

    // 2. Zelda: Breath of the Wild
    if (gn.contains(QStringLiteral("breath")) || gn.contains(QStringLiteral("botw"))) {
        if (c.contains(QStringLiteral("wolf link")) || n.contains(QStringLiteral("wolf"))) {
            return {QObject::tr("Компаньон"), QStringLiteral("🐺"),
                    QObject::tr("Призыв Волка Линка (Wolf Link)"),
                    QObject::tr("Волк Линк появляется в мире и сражается на вашей стороне с 20 сердцами здоровья!")};
        }
        if (n.contains(QStringLiteral("twilight")) || (c.contains(QStringLiteral("link")) && as.contains(QStringLiteral("smash")))) {
            return {QObject::tr("Скакун и броня"), QStringLiteral("🐺"),
                    QObject::tr("Кобыла Эпона и Сет Сумерек"),
                    QObject::tr("Призыв легендарной лошади Эпоны с максимальными характеристиками и Набор Сумерек")};
        }
        if (n.contains(QStringLiteral("majora"))) {
            return {QObject::tr("Оружие и броня"), QStringLiteral("🗡️"),
                    QObject::tr("Двуручник и Сет Свирепого Божества"),
                    QObject::tr("Маска, Доспех и Поножи Свирепого Божества, Двуручник Свирепого Божества")};
        }
        if (n.contains(QStringLiteral("ocarina")) || n.contains(QStringLiteral("time"))) {
            return {QObject::tr("Оружие и броня"), QStringLiteral("🗡️"),
                    QObject::tr("Меч Большого Горона и Сет Времени"),
                    QObject::tr("Шапка/Туника/Штаны Времени, Меч Большого Горона")};
        }
        if (n.contains(QStringLiteral("archer"))) {
            return {QObject::tr("Оружие"), QStringLiteral("🏹"),
                    QObject::tr("Лук Путешественника и особые стрелы"),
                    QObject::tr("Древние, ледяные, огненные и электрические стрелы, редкое мясо")};
        }
        if (c.contains(QStringLiteral("zelda"))) {
            return {QObject::tr("Щит и самоцветы"), QStringLiteral("🛡️"),
                    QObject::tr("Щит Бригадира и Звездный осколок"),
                    QObject::tr("Щит Бригадира, редкие рубины, сапфиры, алмазы и целебные травы")};
        }
        if (c.contains(QStringLiteral("daruk")) || c.contains(QStringLiteral("mipha")) ||
            c.contains(QStringLiteral("revali")) || c.contains(QStringLiteral("urbosa"))) {
            return {QObject::tr("Броня"), QStringLiteral("🛡️"),
                    QObject::tr("Шлем Божественного чудища"),
                    QObject::tr("Шлемы Ва-Рудания, Ва-Рута, Ва-Медо или Ва-Наборис со стихийной защитой")};
        }
        if (gs.contains(QStringLiteral("zelda")) || as.contains(QStringLiteral("zelda"))) {
            return {QObject::tr("Оружие и сундук"), QStringLiteral("🗡️"),
                    QObject::tr("Уникальный сундук Хайрула"),
                    QObject::tr("Сундук с редким оружием, стрелами и драгоценными металлами")};
        }
        return {QObject::tr("Ресурсы"), QStringLiteral("📦"),
                QObject::tr("Припасы и сырье"),
                QObject::tr("Сундук с базовым оружием, мясо, грибы, травы и овощи")};
    }

    // 3. Super Mario Odyssey
    if (gn.contains(QStringLiteral("odyssey")) || gn.contains(QStringLiteral("mario odyssey"))) {
        if (n.contains(QStringLiteral("wedding")) && c.contains(QStringLiteral("mario"))) {
            return {QObject::tr("Костюм и усиление"), QStringLiteral("👕"),
                    QObject::tr("Свадебный смокинг Марио и Неуязвимость"),
                    QObject::tr("Свадебный цилиндр и смокинг; Неуязвимость Марио ко всем видам урона на 30 секунд!")};
        }
        if (n.contains(QStringLiteral("wedding")) && c.contains(QStringLiteral("bowser"))) {
            return {QObject::tr("Костюм и подсказки"), QStringLiteral("👕"),
                    QObject::tr("Свадебный смокинг Боузера и Локатор монет"),
                    QObject::tr("Цилиндр и смокинг Боузера; Подсветка фиолетовых региональных монет на карте мира")};
        }
        if (n.contains(QStringLiteral("wedding")) && c.contains(QStringLiteral("peach"))) {
            return {QObject::tr("Костюм и здоровье"), QStringLiteral("💖"),
                    QObject::tr("Свадебное платье Пич и Сердце Жизни"),
                    QObject::tr("Свадебная фата и платье Пич; Сердце Жизни (мгновенное увеличение до 6 сердец)")};
        }
        if (c.contains(QStringLiteral("luigi"))) {
            return {QObject::tr("Костюм"), QStringLiteral("👕"),
                    QObject::tr("Костюм Луиджи"),
                    QObject::tr("Зеленая кепка и синий рабочий комбинезон Луиджи")};
        }
        if (c.contains(QStringLiteral("wario"))) {
            return {QObject::tr("Костюм"), QStringLiteral("👕"),
                    QObject::tr("Костюм Варио"),
                    QObject::tr("Желтая кепка и фиолетовый костюм Варио")};
        }
        if (c.contains(QStringLiteral("waluigi"))) {
            return {QObject::tr("Костюм"), QStringLiteral("👕"),
                    QObject::tr("Костюм Валуиджи"),
                    QObject::tr("Фиолетовая кепка и темный костюм Валуиджи")};
        }
        if (c.contains(QStringLiteral("diddy"))) {
            return {QObject::tr("Костюм"), QStringLiteral("👕"),
                    QObject::tr("Костюм Дидди Конга"),
                    QObject::tr("Красная бейсболка и желтая майка Дидди Конга")};
        }
        if (n.contains(QStringLiteral("gold")) || n.contains(QStringLiteral("silver"))) {
            return {QObject::tr("Костюм"), QStringLiteral("✨"),
                    QObject::tr("Золотой / Серебряный костюм Марио"),
                    QObject::tr("Ослепительный золотой или серебряный смокинг и кепка")};
        }
        return {QObject::tr("Подсказка Луны"), QStringLiteral("🌙"),
                QObject::tr("Подсказка дядюшки Amiibo"),
                QObject::tr("Дядюшка Amiibo отмечает точное расположение скрытой Луны энергии на карте королевства")};
    }

    // 4. Metroid Dread
    if (gn.contains(QStringLiteral("dread")) || gn.contains(QStringLiteral("metroid"))) {
        if (n.contains(QStringLiteral("dread")) && c.contains(QStringLiteral("samus"))) {
            return {QObject::tr("Усиление здоровья"), QStringLiteral("🚀"),
                    QObject::tr("+1 Контейнер Энергии (Energy Tank)"),
                    QObject::tr("Постоянное увеличение максимума энергии на +1 Tank (+100 HP); Ежедневное пополнение 200 HP")};
        }
        if (c.contains(QStringLiteral("emmi")) || c.contains(QStringLiteral("e.m.m.i."))) {
            return {QObject::tr("Увеличение боезапаса"), QStringLiteral("🚀"),
                    QObject::tr("+10 Ракет (Missile Tank)"),
                    QObject::tr("Постоянное увеличение запаса ракет на +10 шт; Ежедневное полное пополнение ракет")};
        }
        if (c.contains(QStringLiteral("samus"))) {
            return {QObject::tr("Пополнение ракет"), QStringLiteral("🔋"),
                    QObject::tr("Ежедневное пополнение ракет"),
                    QObject::tr("Мгновенное пополнение запаса ракет Самус один раз в день")};
        }
        return {QObject::tr("Пополнение энергии"), QStringLiteral("🔋"),
                QObject::tr("Ежедневное пополнение энергии"),
                QObject::tr("Мгновенное пополнение энергетического запаса костюма один раз в день")};
    }

    // 5. Splatoon 3 / Splatoon 2
    if (gn.contains(QStringLiteral("splatoon"))) {
        if (c.contains(QStringLiteral("inkling girl"))) {
            return {QObject::tr("Эксклюзивный сет"), QStringLiteral("👕"),
                    QObject::tr("Школьная форма (School Uniform Set)"),
                    QObject::tr("Школьная заколка, униформа школы Инкополиса, туфли; совместные фотосессии")};
        }
        if (c.contains(QStringLiteral("inkling boy"))) {
            return {QObject::tr("Эксклюзивный сет"), QStringLiteral("👕"),
                    QObject::tr("Самурайский сет (Samurai Gear)"),
                    QObject::tr("Шлем самурая, доспех самурая, сандалии с носками; фото на аренах")};
        }
        if (c.contains(QStringLiteral("squid"))) {
            return {QObject::tr("Силовая броня"), QStringLiteral("🛡️"),
                    QObject::tr("Силовая броня (Power Armor Set)"),
                    QObject::tr("Силовой шлем, силовой экзоскелет, силовые ботинки")};
        }
        if (c.contains(QStringLiteral("shiver")) || c.contains(QStringLiteral("frye")) || c.contains(QStringLiteral("big man"))) {
            return {QObject::tr("Концертный сет"), QStringLiteral("✨"),
                    QObject::tr("Сценические наряды Deep Cut"),
                    QObject::tr("Эксклюзивный дизайнерский наряд Deep Cut и сохранение комплектов снаряжения")};
        }
        if (c.contains(QStringLiteral("callie")) || c.contains(QStringLiteral("marie")) ||
            c.contains(QStringLiteral("pearl")) || c.contains(QStringLiteral("marina"))) {
            return {QObject::tr("Концертный сет"), QStringLiteral("🎤"),
                    QObject::tr("Наряды кумиров Инкополиса"),
                    QObject::tr("Концертные наряды Squid Sisters и Off the Hook, эксклюзивные треки в лобби")};
        }
        return {QObject::tr("Снаряжение и фото"), QStringLiteral("👕"),
                QObject::tr("Уникальная экипировка персонажа"),
                QObject::tr("Эксклюзивный комплект снаряжения для боев за район и совместные фото в Плюхтонии")};
    }

    // 6. Monster Hunter Rise / Sunbreak
    if (gn.contains(QStringLiteral("monster hunter"))) {
        if (c.contains(QStringLiteral("magnamalo"))) {
            return {QObject::tr("Броня охотника"), QStringLiteral("🛡️"),
                    QObject::tr("Многослойный доспех Синистера (Hunter)"),
                    QObject::tr("Полный набор многослойного доспеха Синистера для охотника, билет в лотерею Кагари")};
        }
        if (c.contains(QStringLiteral("palamute"))) {
            return {QObject::tr("Броня паламута"), QStringLiteral("🐺"),
                    QObject::tr("Многослойный доспех Синистера (Palamute)"),
                    QObject::tr("Зловещий многослойный доспех Синистера для верного паламута, лотерея")};
        }
        if (c.contains(QStringLiteral("palico"))) {
            return {QObject::tr("Броня палико"), QStringLiteral("🐱"),
                    QObject::tr("Многослойный доспех Синистера (Palico)"),
                    QObject::tr("Зловещий многослойный доспех Синистера для котика палико, лотерея")};
        }
        if (c.contains(QStringLiteral("malzeno"))) {
            return {QObject::tr("Броня охотника"), QStringLiteral("🛡️"),
                    QObject::tr("Многослойный доспех Малзено (Formal Dragon)"),
                    QObject::tr("Аристократический вампирский доспех Малзено для охотника, билет лотереи")};
        }
        return {QObject::tr("Лотерея Кагари"), QStringLiteral("🎲"),
                QObject::tr("Билет в лотерею рынка Кагари"),
                QObject::tr("Ежедневный билет на лотерею ценных зелий, ловушек, порошков и талисманов")};
    }

    // 7. Xenoblade Chronicles 3
    if (gn.contains(QStringLiteral("xenoblade"))) {
        if (c.contains(QStringLiteral("shulk"))) {
            return {QObject::tr("Облик оружия"), QStringLiteral("🗡️"),
                    QObject::tr("Облик Меча Монадо (Monado Skin)"),
                    QObject::tr("Легендарный сияющий клинок Меч Монадо для бойца класса Мечник (Swordfighter)")};
        }
        if (c.contains(QStringLiteral("pyra"))) {
            return {QObject::tr("Облик меча"), QStringLiteral("🗡️"),
                    QObject::tr("Огненный клинок Пайры (Aegis Sword)"),
                    QObject::tr("Облик легендарного огненного меча Иджис для класса Мечник")};
        }
        if (c.contains(QStringLiteral("mythra"))) {
            return {QObject::tr("Облик меча"), QStringLiteral("🗡️"),
                    QObject::tr("Световой клинок Мифры (Aegis Sword)"),
                    QObject::tr("Облик легендарного светового меча Иджис для класса Мечник")};
        }
        if (c.contains(QStringLiteral("noah")) || c.contains(QStringLiteral("mio"))) {
            return {QObject::tr("Костюмы"), QStringLiteral("👕"),
                    QObject::tr("Костюмы Консулов N и M"),
                    QObject::tr("Эксклюзивные доспехи Консула N и Консула M для персонажей")};
        }
        return {QObject::tr("Расходные ресурсы"), QStringLiteral("📦"),
                QObject::tr("Пакет припасов колонии"),
                QObject::tr("Золото, эфирные цилиндры, материалы и коллекционные предметы Айониоса")};
    }

    // 8. Super Smash Bros. Ultimate
    if (gn.contains(QStringLiteral("smash"))) {
        return {QObject::tr("Обучаемый боец FP"), QStringLiteral("🥊"),
                QObject::tr("Боец Figure Player (1-50 ур.)"),
                QObject::tr("Создание обучаемого ИИ-бойца: учится вашим тактикам, прокачивается до 50 уровня, экипируется Духами")};
    }

    // 9. Mario Kart 8 Deluxe
    if (gn.contains(QStringLiteral("mario kart"))) {
        return {QObject::tr("Гоночный костюм"), QStringLiteral("🏎️"),
                QObject::tr("Тематический костюм Mii"),
                QObject::tr("Эксклюзивный гоночный комбинезон и шлем Mii в стиле фигурки Amiibo")};
    }

    // 10. Animal Crossing: New Horizons
    if (gn.contains(QStringLiteral("animal crossing"))) {
        return {QObject::tr("Гость и плакат"), QStringLiteral("🏕️"),
                QObject::tr("Визит на кемпинг острова"),
                QObject::tr("Приглашение персонажа погостить на острове в кемпинге, плакат жителя в банкомате Нука, фотостудия")};
    }

    // 11. Fire Emblem Engage
    if (gn.contains(QStringLiteral("fire emblem"))) {
        return {QObject::tr("Костюмы и музыка"), QStringLiteral("🎵"),
                QObject::tr("Билеты на наряды и классические треки"),
                QObject::tr("Билеты на наряды Эмблем в беседке Amiibo Сомниэля и музыкальные темы прошлых частей FE")};
    }

    // 12. Kirby and the Forgotten Land
    if (gn.contains(QStringLiteral("kirby"))) {
        return {QObject::tr("Усиления и монеты"), QStringLiteral("⭐"),
                QObject::tr("Звездные монеты и сытная еда"),
                QObject::tr("Пачка Звездных монет и предметы временного усиления атаки и скорости бега")};
    }

    // Default / All games fallback
    return {QObject::tr("Универсальный бонус"), QStringLiteral("📦"),
            QObject::tr("Награда путешественника"),
            QObject::tr("Сундук со случайными бонусами: монеты, редкие расходники, материалы для крафта и припасы")};
}

void AmiiboBrowserDialog::FetchImage(const QString& image_url, QLabel* target_label) {
    if (image_url.isEmpty()) {
        target_label->setText(tr("Изображение отсутствует"));
        return;
    }

    if (m_image_cache.contains(image_url)) {
        target_label->setPixmap(m_image_cache[image_url].scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    // Check disk cache
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
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 STORM-EDEN/4.0.2"));
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

QString AmiiboBrowserDialog::GenerateAndSaveAmiiboBin(const AmiiboEntry& entry) {
    const auto amiibo_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::AmiiboDir);
    QString clean_series = entry.amiibo_series;
    if (clean_series.isEmpty()) clean_series = QStringLiteral("General");
    clean_series.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));

    QString clean_name = entry.name;
    if (clean_name.isEmpty()) clean_name = QStringLiteral("Amiibo");
    clean_name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));

    const auto series_folder = amiibo_dir / clean_series.toStdString();
    Common::FS::CreateDirs(series_folder);

    const auto bin_path = series_folder / (clean_name.toStdString() + ".bin");

    // Generate standard 540-byte NTAG215 binary
    std::vector<u8> ntag(540, 0);

    // Randomize tag UID (0x00 to 0x08)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<u32> dis(1, 254);
    ntag[0] = 0x04; // NXP manufacturer prefix
    ntag[1] = static_cast<u8>(dis(gen));
    ntag[2] = static_cast<u8>(dis(gen));
    ntag[3] = ntag[0] ^ ntag[1] ^ ntag[2] ^ 0x88; // BCC0
    ntag[4] = static_cast<u8>(dis(gen));
    ntag[5] = static_cast<u8>(dis(gen));
    ntag[6] = static_cast<u8>(dis(gen));
    ntag[7] = static_cast<u8>(dis(gen));
    ntag[8] = ntag[4] ^ ntag[5] ^ ntag[6] ^ ntag[7]; // BCC1

    // NTAG215 Internal bytes
    ntag[9] = 0x48;
    ntag[10] = 0x0F;
    ntag[11] = 0xE0;
    ntag[12] = 0xF1;

    // Capability Container (CC)
    ntag[13] = 0x11;
    ntag[14] = 0x48;
    ntag[15] = 0x00;
    ntag[16] = 0x00;
    ntag[17] = 0xE1;
    ntag[18] = 0x10;
    ntag[19] = 0x3E;
    ntag[20] = 0x00;

    // Parse Amiibo Model ID (Head & Tail 32-bit hex)
    u32 head_val = entry.head.toUInt(nullptr, 16);
    u32 tail_val = entry.tail.toUInt(nullptr, 16);

    // Write Head (0x54 - 0x57) big endian
    ntag[0x54] = static_cast<u8>((head_val >> 24) & 0xFF);
    ntag[0x55] = static_cast<u8>((head_val >> 16) & 0xFF);
    ntag[0x56] = static_cast<u8>((head_val >> 8) & 0xFF);
    ntag[0x57] = static_cast<u8>(head_val & 0xFF);

    // Write Tail (0x58 - 0x5B) big endian
    ntag[0x58] = static_cast<u8>((tail_val >> 24) & 0xFF);
    ntag[0x59] = static_cast<u8>((tail_val >> 16) & 0xFF);
    ntag[0x5A] = static_cast<u8>((tail_val >> 8) & 0xFF);
    ntag[0x5B] = static_cast<u8>(tail_val & 0xFF);

    // Write Backup Head / Tail at 0x1DC and 0x1E0
    ntag[0x1DC] = ntag[0x54];
    ntag[0x1DD] = ntag[0x55];
    ntag[0x1DE] = ntag[0x56];
    ntag[0x1DF] = ntag[0x57];

    ntag[0x1E0] = ntag[0x58];
    ntag[0x1E1] = ntag[0x59];
    ntag[0x1E2] = ntag[0x5A];
    ntag[0x1E3] = ntag[0x5B];

    // Dynamic Lock Bytes and Config at 0x208
    ntag[0x208] = 0x01;
    ntag[0x209] = 0x00;
    ntag[0x20A] = 0x0F;
    ntag[0x20B] = 0xBD;
    ntag[0x20F] = 0x04;
    ntag[0x210] = 0x5F;
    ntag[0x218] = 0xFF;
    ntag[0x219] = 0xFF;
    ntag[0x21A] = 0xFF;
    ntag[0x21B] = 0xFF;

    // Write to disk
    Common::FS::IOFile file{bin_path, Common::FS::FileAccessMode::Write, Common::FS::FileType::BinaryFile};
    if (file.IsOpen()) {
        file.Write(ntag);
        file.Close();
    }

    return QString::fromStdString(bin_path.string());
}

void AmiiboBrowserDialog::OnSaveAmiiboClicked() {
    auto* cur = m_amiibo_list->currentItem();
    if (!cur) return;
    int idx = cur->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= static_cast<int>(m_all_amiibos.size())) return;

    QString saved_path = GenerateAndSaveAmiiboBin(m_all_amiibos[idx]);
    DisplayAmiiboDetails(m_all_amiibos[idx]);
    QMessageBox::information(this, tr("Amiibo сохранен"),
                             tr("Файл Amiibo успешно создан и сохранен в каталог:\n%1").arg(saved_path));
}

void AmiiboBrowserDialog::OnLoadAmiiboClicked() {
    auto* cur = m_amiibo_list->currentItem();
    if (!cur) return;
    int idx = cur->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= static_cast<int>(m_all_amiibos.size())) return;

    QString saved_path = GenerateAndSaveAmiiboBin(m_all_amiibos[idx]);
    emit AmiiboSelectedForLoading(saved_path);
    DisplayAmiiboDetails(m_all_amiibos[idx]);
    QMessageBox::information(this, tr("Amiibo загружен"),
                             tr("Amiibo «%1» успешно отправлен в виртуальный NFC-считыватель эмулятора!").arg(m_all_amiibos[idx].name));
}

void AmiiboBrowserDialog::OnDisconnectAmiiboClicked() {
    emit AmiiboRemoveRequested();
    QMessageBox::information(this, tr("Amiibo отключен"),
                             tr("Текущий подключенный Amiibo успешно убран с виртуального контроллера."));
}

void AmiiboBrowserDialog::OnOpenAmiiboFolderClicked() {
    const auto amiibo_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::AmiiboDir);
    Common::FS::CreateDirs(amiibo_dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(amiibo_dir.string())));
}

void AmiiboBrowserDialog::OnRefreshClicked() {
    FetchAmiiboDatabase();
}

void AmiiboBrowserDialog::OnListLoaded() {}
