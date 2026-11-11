#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include <QPainter>
#include <QPainterPath>
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "core/core.h"
#include "frontend_common/mod_manager.h"
#include "qt_common/util/mod.h"
#include "storm_switch/configuration/configure_gamebanana_mods.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/configuration/configure_per_game_addons.h"
#include "common/settings.h"
#include "qt_common/config/uisettings.h"

static QString FormatNumber(qint64 num) {
    if (num < 0) return QString::number(num);
    QString s = QString::number(num);
    int n = s.length() - 3;
    while (n > 0) {
        s.insert(n, QLatin1Char(' '));
        n -= 3;
    }
    return s;
}

static int ResolveGameBananaGameId(u64 title_id, const QString& game_name) {
    switch (title_id) {
    case 0x01007EF00011E000: return 5866;  // The Legend of Zelda: Breath of the Wild (Switch)
    case 0x0100F2C0115B6000: return 17654; // The Legend of Zelda: Tears of the Kingdom
    case 0x01006A800016E000: return 6686;  // Super Smash Bros. Ultimate
    case 0x0100000000010000: return 6150;  // Super Mario Odyssey
    case 0x0100152000022000: return 6507;  // Mario Kart 8 Deluxe
    case 0x01006F8002326000: return 8282;  // Animal Crossing: New Horizons
    case 0x0100A3D008C5C000: return 16871; // Pokemon Scarlet
    case 0x01008F6008C5E000: return 16871; // Pokemon Violet
    case 0x0100ABF008968000: return 7616;  // Pokemon Sword
    case 0x01008DB008C2C000: return 7616;  // Pokemon Shield
    case 0x01001F5010DFA000: return 12845; // Pokemon Legends: Arceus
    case 0x0100000011D90000: return 18365; // Pokemon Legends: Z-A
    case 0x0100000000014000: return 7018;  // Pokemon: Let's Go, Pikachu!
    case 0x0100000000015000: return 7018;  // Pokemon: Let's Go, Eevee!
    case 0x0100000011DE8000: return 12844; // Pokemon Brilliant Diamond / Shining Pearl
    case 0x01002DA013484000: return 11394; // Metroid Dread
    case 0x0100121014688000: return 14457; // Metroid Prime Remastered
    case 0x01000A10041EA000: return 16777; // Persona 5 Royal (Switch)
    case 0x010074F013262000: return 16781; // Xenoblade Chronicles 3
    case 0x0100E95004038000: return 6927;  // Xenoblade Chronicles 2
    case 0x0100FF500E34A000: return 8449;  // Xenoblade Chronicles: Definitive Edition
    case 0x0100C2500FC20000: return 15797; // Splatoon 3
    case 0x01003BC0000A0000: return 5956;  // Splatoon 2
    case 0x0100C9A00ECE6000: return 17462; // Fire Emblem Engage
    case 0x010055D009F78000: return 7460;  // Fire Emblem: Three Houses
    case 0x01004A4010FE8000: return 17088; // Bayonetta 3
    case 0x01004D300C5AE000: return 15206; // Kirby and the Forgotten Land
    case 0x010003200D166000: return 17497; // Sonic Frontiers (Switch)
    case 0x010015100B5B4000: return 17655; // Super Mario Bros. Wonder
    case 0x01009B90006DC000: return 6848;  // Super Mario Party
    case 0x0100D8701267E000: return 12797; // Mario Party Superstars
    case 0x01002B4019BCA000: return 18980; // Super Mario Party Jamboree
    case 0x01009AA000FAA000: return 6853;  // Luigi's Mansion 3
    case 0x010028600EBDA000: return 12836; // Super Mario 3D World + Bowser's Fury
    case 0x0100B04011742000: return 12836; // Super Mario 3D World + Bowser's Fury
    case 0x01004BC0000AA000: return 6554;  // Super Mario Maker 2
    case 0x01004A5013054000: return 18600; // The Legend of Zelda: Echoes of Wisdom
    case 0x01006BB00C6F0000: return 7433;  // The Legend of Zelda: Link's Awakening
    case 0x010065800002A000: return 6168;  // The Legend of Zelda: Skyward Sword HD
    default: break;
    }

    const QString lower = game_name.toLower();
    if (lower.contains(QStringLiteral("breath of the wild")) || lower.contains(QStringLiteral("botw"))) return 5866;
    if (lower.contains(QStringLiteral("tears of the kingdom")) || lower.contains(QStringLiteral("totk"))) return 17654;
    if (lower.contains(QStringLiteral("smash bros"))) return 6686;
    if (lower.contains(QStringLiteral("mario odyssey"))) return 6150;
    if (lower.contains(QStringLiteral("mario kart 8"))) return 6507;
    if (lower.contains(QStringLiteral("animal crossing"))) return 8282;
    if (lower.contains(QStringLiteral("metroid dread"))) return 11394;
    if (lower.contains(QStringLiteral("metroid prime"))) return 14457;
    if (lower.contains(QStringLiteral("splatoon 3"))) return 15797;
    if (lower.contains(QStringLiteral("splatoon 2"))) return 5956;
    if (lower.contains(QStringLiteral("persona 5"))) return 16777;
    if (lower.contains(QStringLiteral("xenoblade 3"))) return 16781;
    if (lower.contains(QStringLiteral("xenoblade 2"))) return 6927;
    if (lower.contains(QStringLiteral("sonic frontiers"))) return 17497;
    if (lower.contains(QStringLiteral("mario wonder"))) return 17655;
    if (lower.contains(QStringLiteral("echoes of wisdom"))) return 18600;
    return 0;
}


namespace {

enum class GbIconType {
    Search,
    Refresh,
    Download,
    Folder,
    Check,
    Cross,
    Trash,
    ChevronLeft,
    ChevronRight,
    FirstPage,
};

QIcon MakeGbIcon(GbIconType type, const QColor& color, int size = 16) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case GbIconType::Search:
        p.drawEllipse(QPointF(6.5, 6.5), 4.0, 4.0);
        p.drawLine(QPointF(9.5, 9.5), QPointF(13.5, 13.5));
        break;
    case GbIconType::Refresh: {
        p.drawArc(QRectF(3.0, 3.0, 10.0, 10.0), 30 * 16, 270 * 16);
        p.drawLine(QPointF(11.0, 4.5), QPointF(13.0, 6.5));
        p.drawLine(QPointF(13.0, 6.5), QPointF(10.5, 9.0));
        break;
    }
    case GbIconType::Download:
        p.drawLine(QPointF(8.0, 2.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(4.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(11.5, 7.5), QPointF(8.0, 10.5));
        p.drawLine(QPointF(3.0, 13.5), QPointF(13.0, 13.5));
        break;
    case GbIconType::Folder: {
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
    case GbIconType::Check:
        p.drawLine(QPointF(3.0, 8.0), QPointF(6.5, 11.5));
        p.drawLine(QPointF(6.5, 11.5), QPointF(13.0, 4.5));
        break;
    case GbIconType::Cross:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    case GbIconType::Trash: {
        p.drawLine(QPointF(3.5, 4.5), QPointF(12.5, 4.5));
        p.drawLine(QPointF(6.0, 2.5), QPointF(10.0, 2.5));
        QPainterPath body;
        body.moveTo(4.5, 4.5);
        body.lineTo(5.5, 13.5);
        body.lineTo(10.5, 13.5);
        body.lineTo(11.5, 4.5);
        p.drawPath(body);
        p.drawLine(QPointF(7.0, 6.5), QPointF(7.0, 11.5));
        p.drawLine(QPointF(9.0, 6.5), QPointF(9.0, 11.5));
        break;
    }
    case GbIconType::ChevronLeft:
        p.drawLine(QPointF(10.0, 4.0), QPointF(6.0, 8.0));
        p.drawLine(QPointF(6.0, 8.0), QPointF(10.0, 12.0));
        break;
    case GbIconType::ChevronRight:
        p.drawLine(QPointF(6.0, 4.0), QPointF(10.0, 8.0));
        p.drawLine(QPointF(10.0, 8.0), QPointF(6.0, 12.0));
        break;
    case GbIconType::FirstPage:
        p.drawLine(QPointF(4.5, 4.0), QPointF(4.5, 12.0));
        p.drawLine(QPointF(11.5, 4.0), QPointF(7.5, 8.0));
        p.drawLine(QPointF(7.5, 8.0), QPointF(11.5, 12.0));
        break;
    }
    return QIcon(pix);
}

QPixmap MakeScreenshotPlaceholder(int w, int h, const StormTheme::Palette& pal) {
    QPixmap pix(w, h);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor accent(pal.accent);
    const QColor border(pal.border);

    p.setPen(QPen(border, 1.0));
    p.setBrush(QColor(pal.surface));
    p.drawRoundedRect(QRectF(2, 2, w - 4, h - 4), 8.0, 8.0);

    p.setPen(QPen(accent, 1.5));
    p.setBrush(QColor(pal.cardAlt));
    const qreal cx = w / 2.0;
    const qreal cy = h / 2.0;
    p.drawRoundedRect(QRectF(cx - 32, cy - 20, 64, 40), 8.0, 8.0);

    p.drawLine(QPointF(cx - 18, cy - 6), QPointF(cx - 18, cy + 6));
    p.drawLine(QPointF(cx - 24, cy), QPointF(cx - 12, cy));
    p.drawEllipse(QPointF(cx + 18, cy - 4), 3.0, 3.0);
    p.drawEllipse(QPointF(cx + 18, cy + 4), 3.0, 3.0);

    return pix;
}

} // namespace

ConfigureGameBananaMods::ConfigureGameBananaMods(Core::System& system_, u64 title_id_,
                                                 const QString& game_name_,
                                                 ConfigurePerGameAddons* addons_tab_,
                                                 QWidget* parent)
    : QWidget(parent), title_id{title_id_}, game_name{game_name_}, system{system_},
      addons_tab{addons_tab_} {
    network_manager = new QNetworkAccessManager(this);
    setProperty("_stormDialogThemed", true);
    const auto pal = StormTheme::GetThemePalette();
    setStyleSheet(StormDesign::BuildDialogStylesheet(pal));

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->setSpacing(8);

    // 1. Header Banner
    game_header_label = new QLabel(this);
    game_header_label->setStyleSheet(QStringLiteral(
        "background-color: %1; color: %2; border: 1px solid %3; padding: 8px 12px; border-radius: 6px; font-weight: bold; font-size: 13px;").arg(pal.card, pal.textPrimary, pal.border));
    main_layout->addWidget(game_header_label);

    // 2. View Switcher Bar (Online Catalog vs Installed Mods)
    auto* switcher_layout = new QHBoxLayout;
    switcher_layout->setSpacing(8);

    tab_online_btn = new QPushButton(StormLang(QStringLiteral("Онлайн-каталог"), QStringLiteral("Online Catalog")), this);
    tab_online_btn->setCheckable(true);
    tab_online_btn->setChecked(true);
    tab_online_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #00f2fe; color: #000000; font-weight: bold; padding: 6px 18px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: #38bdf8; }"
    ));

    tab_installed_btn = new QPushButton(StormLang(QStringLiteral("Установленные"), QStringLiteral("Installed")), this);
    tab_installed_btn->setCheckable(true);
    tab_installed_btn->setChecked(false);
    tab_installed_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #1e293b; color: #FFFFFF; font-weight: bold; padding: 6px 18px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: #334155; }"
    ));

    switcher_layout->addWidget(tab_online_btn);
    switcher_layout->addWidget(tab_installed_btn);
    switcher_layout->addStretch(1);
    main_layout->addLayout(switcher_layout);

    view_stack = new QStackedWidget(this);

    // ==========================================
    // --- Page 0: Online Catalog ---
    // ==========================================
    online_page = new QWidget(this);
    auto* online_layout = new QVBoxLayout(online_page);
    online_layout->setContentsMargins(0, 0, 0, 0);
    online_layout->setSpacing(6);

    // Search & Controls Bar
    auto* controls_layout = new QHBoxLayout;
    controls_layout->setSpacing(6);

    search_input = new QLineEdit(this);
    search_input->setPlaceholderText(StormLang(QStringLiteral("Поиск модов на GameBanana..."), QStringLiteral("Search GameBanana mods...")));
    search_input->setClearButtonEnabled(true);
    controls_layout->addWidget(search_input, 1);

    search_btn = new QPushButton(StormLang(QStringLiteral("Найти"), QStringLiteral("Search")), this);
    search_btn->setIcon(MakeGbIcon(GbIconType::Search, QColor(pal.textPrimary)));
    search_btn->setStyleSheet(QStringLiteral("font-weight: bold; padding: 4px 12px;"));
    controls_layout->addWidget(search_btn);

    sort_combo = new QComboBox(this);
    sort_combo->addItem(tr("По скачиваниям (Популярные)"), QStringLiteral("Generic_MostDownloaded"));
    sort_combo->addItem(tr("По количеству лайков"), QStringLiteral("Generic_MostLiked"));
    sort_combo->addItem(tr("По просмотрам"), QStringLiteral("Generic_MostViewed"));
    sort_combo->addItem(tr("По дате (Новые)"), QStringLiteral("Generic_Latest"));
    sort_combo->addItem(tr("По алфавиту (А-Я)"), QStringLiteral("Generic_Alphabetical"));
    controls_layout->addWidget(sort_combo);

    refresh_btn = new QPushButton(StormLang(QStringLiteral("Обновить"), QStringLiteral("Refresh")), this);
    refresh_btn->setIcon(MakeGbIcon(GbIconType::Refresh, QColor(pal.textSecondary)));
    controls_layout->addWidget(refresh_btn);

    online_layout->addLayout(controls_layout);

    // Category filter pills
    auto* pills_layout = new QHBoxLayout();
    pills_layout->setSpacing(6);
    pills_layout->setContentsMargins(0, 2, 0, 4);

    const QStringList cat_names = {
        StormLang(QStringLiteral("Все"), QStringLiteral("All")),
        StormLang(QStringLiteral("Скины"), QStringLiteral("Skins")),
        StormLang(QStringLiteral("Текстуры"), QStringLiteral("Textures")),
        StormLang(QStringLiteral("Интерфейс"), QStringLiteral("UI")),
        StormLang(QStringLiteral("Звук"), QStringLiteral("Audio")),
        StormLang(QStringLiteral("60 FPS / Твики"), QStringLiteral("60 FPS / Tweaks")),
    };

    for (const auto& cname : cat_names) {
        auto* pbtn = new QPushButton(cname, online_page);
        pbtn->setCheckable(true);
        pbtn->setChecked(cname == cat_names[0]);
        pbtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 4px 12px;"
            "  font-size: 8.5pt;"
            "  font-weight: 600;"
            "  min-height: 20px;"
            "}"
            "QPushButton:checked {"
            "  background: %4;"
            "  color: #FFFFFF;"
            "  border: none;"
            "}"
            "QPushButton:hover {"
            "  border: none;"
            "}"
        ).arg(pal.surface, pal.textSecondary, pal.border, pal.accent));
        category_pill_buttons.push_back(pbtn);
        connect(pbtn, &QPushButton::clicked, this, [this, cname]() {
            OnCategoryPillClicked(cname);
        });
        pills_layout->addWidget(pbtn);
    }
    pills_layout->addStretch(1);
    online_layout->addLayout(pills_layout);

    // Main Splitter (Left: Mod List, Right: Mod Details & Install)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left Pane
    auto* left_pane = new QWidget(this);
    auto* left_layout = new QVBoxLayout(left_pane);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(4);

    mod_tree = new QTreeWidget(this);
    mod_tree->setStyleSheet(QStringLiteral("QTreeWidget::item { height: 34px; padding: 2px 6px; }"));
    mod_tree->setHeaderLabels({tr("НАЗВАНИЕ МОДА"), tr("АВТОР"), tr("ПРОСМОТРОВ"), tr("ЛАЙКОВ"), tr("КАТЕГОРИЯ")});
    mod_tree->setAlternatingRowColors(true);
    mod_tree->setRootIsDecorated(false);
    for (int i = 0; i < 5; ++i) {
        mod_tree->headerItem()->setTextAlignment(i, Qt::AlignCenter);
    }
    mod_tree->header()->setStretchLastSection(false);
    mod_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    mod_tree->header()->resizeSection(0, 340);
    mod_tree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    mod_tree->header()->resizeSection(1, 150);
    mod_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    mod_tree->header()->resizeSection(2, 120);
    mod_tree->header()->setSectionResizeMode(3, QHeaderView::Interactive);
    mod_tree->header()->resizeSection(3, 100);
    mod_tree->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    mod_tree->header()->resizeSection(4, 150);
    left_layout->addWidget(mod_tree, 1);

    splitter->addWidget(left_pane);

    // Right Details Pane
    details_widget = new QWidget(this);
    auto* details_layout = new QVBoxLayout(details_widget);
    details_layout->setContentsMargins(0, 0, 0, 0);
    details_layout->setSpacing(6);

    auto* details_top_bar = new QHBoxLayout;
    auto* details_header_label = new QLabel(StormLang(QStringLiteral("Инспектор и установка мода"), QStringLiteral("Mod Inspector & Installation")), this);
    details_header_label->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 10pt; color: %1;").arg(pal.accent));
    auto* close_details_btn = new QPushButton(StormLang(QStringLiteral("Скрыть"), QStringLiteral("Hide")), this);
    close_details_btn->setIcon(MakeGbIcon(GbIconType::Cross, QColor(pal.textSecondary)));
    close_details_btn->setStyleSheet(QStringLiteral("padding: 4px 12px; font-weight: bold; background-color: %1; color: %2; border-radius: 4px;").arg(pal.buttonBg, pal.textPrimary));
    connect(close_details_btn, &QPushButton::clicked, [this]() {
        if (details_widget) {
            details_widget->setVisible(false);
        }
        if (mod_tree) {
            mod_tree->clearSelection();
        }
    });
    details_top_bar->addWidget(details_header_label, 1);
    details_top_bar->addWidget(close_details_btn);
    details_layout->addLayout(details_top_bar);

    inspector_title_label = new QLabel(details_widget);
    inspector_title_label->setStyleSheet(QStringLiteral("font-size: 11pt; font-weight: 700; color: %1;").arg(pal.textPrimary));
    inspector_title_label->setWordWrap(true);
    details_layout->addWidget(inspector_title_label);

    // Metadata pills row
    auto* meta_row = new QHBoxLayout();
    meta_row->setSpacing(6);
    auto make_pill = [&](QLabel*& lbl) {
        lbl = new QLabel(details_widget);
        lbl->setStyleSheet(QStringLiteral(
            "background-color: %1; color: %2; border: none; border-radius: 4px; padding: 2px 8px; font-size: 8.5pt; font-weight: 600;"
        ).arg(pal.cardAlt, pal.textSecondary, pal.border));
        meta_row->addWidget(lbl);
    };
    make_pill(mod_author_pill);
    make_pill(mod_size_pill);
    make_pill(mod_downloads_pill);
    make_pill(mod_likes_pill);
    make_pill(mod_category_pill);
    meta_row->addStretch(1);
    details_layout->addLayout(meta_row);

    // Screenshot viewer
    auto* ss_container = new QWidget(details_widget);
    auto* ss_layout = new QVBoxLayout(ss_container);
    ss_layout->setContentsMargins(0, 4, 0, 4);
    ss_layout->setSpacing(4);

    screenshot_label = new QLabel(ss_container);
    screenshot_label->setFixedSize(280, 160);
    screenshot_label->setAlignment(Qt::AlignCenter);
    screenshot_label->setPixmap(MakeScreenshotPlaceholder(280, 160, pal));
    ss_layout->addWidget(screenshot_label, 0, Qt::AlignCenter);

    auto* ss_nav = new QHBoxLayout();
    ss_nav->setSpacing(8);
    prev_screenshot_btn = new QPushButton(ss_container);
    prev_screenshot_btn->setIcon(MakeGbIcon(GbIconType::ChevronLeft, QColor(pal.textSecondary)));
    prev_screenshot_btn->setFixedSize(28, 24);

    screenshot_counter_label = new QLabel(ss_container);
    screenshot_counter_label->setStyleSheet(QStringLiteral("font-size: 8.5pt; color: %1;").arg(pal.textSecondary));
    screenshot_counter_label->setAlignment(Qt::AlignCenter);

    next_screenshot_btn = new QPushButton(ss_container);
    next_screenshot_btn->setIcon(MakeGbIcon(GbIconType::ChevronRight, QColor(pal.textSecondary)));
    next_screenshot_btn->setFixedSize(28, 24);

    connect(prev_screenshot_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnPrevScreenshot);
    connect(next_screenshot_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnNextScreenshot);

    ss_nav->addStretch(1);
    ss_nav->addWidget(prev_screenshot_btn);
    ss_nav->addWidget(screenshot_counter_label);
    ss_nav->addWidget(next_screenshot_btn);
    ss_nav->addStretch(1);
    ss_layout->addLayout(ss_nav);

    details_layout->addWidget(ss_container);

    detail_browser = new QTextBrowser(this);
    detail_browser->setOpenLinks(false);
    detail_browser->setOpenExternalLinks(false);
    detail_browser->setPlaceholderText(tr("Выберите мод из списка слева для просмотра описания и файлов для установки."));
    connect(detail_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        QString url_str = url.toString();
        static const QRegularExpression mod_regex(
            QStringLiteral(R"((?:gamebanana\.com/(?:mods|gamefiles|skins|guis|sounds|wips)/|/mods/|mods/|#)(\d+))"),
            QRegularExpression::CaseInsensitiveOption);
        auto match = mod_regex.match(url_str);
        int target_mod_id = 0;
        if (match.hasMatch()) {
            target_mod_id = match.captured(1).toInt();
        } else {
            static const QRegularExpression digits_regex(QStringLiteral(R"(^\D*(\d{3,8})\D*$)"));
            auto digit_match = digits_regex.match(url_str);
            if (digit_match.hasMatch()) {
                target_mod_id = digit_match.captured(1).toInt();
            }
        }

        if (target_mod_id > 0) {
            for (int i = 0; i < mod_tree->topLevelItemCount(); ++i) {
                auto* item = mod_tree->topLevelItem(i);
                if (item && item->data(0, Qt::UserRole).toInt() == target_mod_id) {
                    mod_tree->setCurrentItem(item);
                    return;
                }
            }
            if (search_input) {
                search_input->setText(QString::number(target_mod_id));
                OnSearchClicked();
            }
        } else if (url_str.contains(QStringLiteral("gamebanana.com"), Qt::CaseInsensitive)) {
            QString path_query = url.path().section(QLatin1Char('/'), -1);
            if (!path_query.isEmpty() && search_input) {
                search_input->setText(path_query);
                OnSearchClicked();
            }
        } else if (url_str.startsWith(QStringLiteral("http://")) || url_str.startsWith(QStringLiteral("https://"))) {
            QDesktopServices::openUrl(url);
        }
    });
    details_layout->addWidget(detail_browser, 1);

    auto* files_label = new QLabel(StormLang(QStringLiteral("Доступные файлы мода:"), QStringLiteral("Available Mod Files:")), this);
    files_label->setStyleSheet(QStringLiteral("font-weight: bold;"));
    details_layout->addWidget(files_label);

    files_combo = new QComboBox(this);
    details_layout->addWidget(files_combo);

    progress_bar = new QProgressBar(this);
    progress_bar->setRange(0, 100);
    progress_bar->setValue(0);
    progress_bar->setTextVisible(true);
    progress_bar->setVisible(false);
    details_layout->addWidget(progress_bar);

    status_label = new QLabel(this);
    status_label->setWordWrap(true);
    status_label->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(pal.accent));
    details_layout->addWidget(status_label);

    auto* btn_layout = new QHBoxLayout;
    install_btn = new QPushButton(StormLang(QStringLiteral("Установить мод"), QStringLiteral("Install Mod")), this);
    install_btn->setIcon(MakeGbIcon(GbIconType::Download, QColor(QStringLiteral("#FFFFFF"))));
    install_btn->setStyleSheet(QStringLiteral("QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2); color: #FFFFFF; font-weight: bold; padding: 6px 18px; border: 1px solid %3; border-top: 1px solid rgba(255,255,255,0.35); border-radius: 6px; min-height: 32px; font-size: 9.5pt; } QPushButton:hover { background: %2; } QPushButton:pressed { background: %1; }").arg(pal.accent, pal.accentHover, pal.border));
    install_btn->setEnabled(false);
    btn_layout->addWidget(install_btn, 1);

    open_folder_btn = new QPushButton(StormLang(QStringLiteral("Папка модов игры"), QStringLiteral("Game Mods Folder")), this);
    open_folder_btn->setIcon(MakeGbIcon(GbIconType::Folder, QColor(pal.textSecondary)));
    btn_layout->addWidget(open_folder_btn);

    details_layout->addLayout(btn_layout);

    splitter->addWidget(details_widget);
    details_widget->setVisible(false);
    online_layout->addWidget(splitter, 1);

    // Bottom Pagination Bar
    auto* bottom_bar = new QHBoxLayout;
    bottom_bar->setContentsMargins(4, 2, 4, 2);

    page_label = new QLabel(tr("Страница 1"), this);
    page_label->setStyleSheet(QStringLiteral("font-weight: bold; padding: 0 4px; color: #38bdf8; font-size: 11.5px;"));
    bottom_bar->addWidget(page_label);
    bottom_bar->addStretch(1);

    first_page_btn = new QPushButton(StormLang(QStringLiteral("Первая"), QStringLiteral("First")), this);
    first_page_btn->setIcon(MakeGbIcon(GbIconType::FirstPage, QColor(pal.textSecondary)));
    prev_page_btn = new QPushButton(StormLang(QStringLiteral("Назад"), QStringLiteral("Back")), this);
    prev_page_btn->setIcon(MakeGbIcon(GbIconType::ChevronLeft, QColor(pal.textSecondary)));
    next_page_btn = new QPushButton(StormLang(QStringLiteral("Вперед"), QStringLiteral("Next")), this);
    next_page_btn->setIcon(MakeGbIcon(GbIconType::ChevronRight, QColor(pal.textSecondary)));

    first_page_btn->setStyleSheet(QStringLiteral("padding: 3px 10px; font-weight: bold;"));
    prev_page_btn->setStyleSheet(QStringLiteral("padding: 3px 10px; font-weight: bold;"));
    next_page_btn->setStyleSheet(QStringLiteral("padding: 3px 10px; font-weight: bold;"));

    first_page_btn->setEnabled(false);
    prev_page_btn->setEnabled(false);
    next_page_btn->setEnabled(false);

    bottom_bar->addWidget(first_page_btn);
    bottom_bar->addWidget(prev_page_btn);
    bottom_bar->addWidget(next_page_btn);
    bottom_bar->addStretch(1);

    online_layout->addLayout(bottom_bar);

    // ==========================================
    // --- Page 1: Installed Mods ---
    // ==========================================
    installed_page = new QWidget(this);
    auto* installed_layout = new QVBoxLayout(installed_page);
    installed_layout->setContentsMargins(0, 0, 0, 0);
    installed_layout->setSpacing(6);

    auto* installed_top_bar = new QHBoxLayout;
    installed_status_label = new QLabel(this);
    installed_status_label->setStyleSheet(QStringLiteral("color: #38bdf8; font-weight: bold; font-size: 12px;"));
    installed_top_bar->addWidget(installed_status_label, 1);

    auto* inst_search = new QLineEdit(this);
    inst_search->setPlaceholderText(StormLang(QStringLiteral("Фильтр установленных модов..."), QStringLiteral("Filter installed mods...")));
    inst_search->setClearButtonEnabled(true);
    inst_search->setMinimumWidth(200);
    connect(inst_search, &QLineEdit::textChanged, this, [this](const QString& q) {
        if (!installed_tree) return;
        const QString query = q.trimmed();
        for (int i = 0; i < installed_tree->topLevelItemCount(); ++i) {
            if (auto* it = installed_tree->topLevelItem(i)) {
                const bool match = query.isEmpty() ||
                    it->text(0).contains(query, Qt::CaseInsensitive) ||
                    it->text(3).contains(query, Qt::CaseInsensitive);
                it->setHidden(!match);
            }
        }
    });
    installed_top_bar->addWidget(inst_search);

    auto* enable_all_mods_btn = new QPushButton(StormLang(QStringLiteral("Включить все"), QStringLiteral("Enable All")), this);
    enable_all_mods_btn->setIcon(MakeGbIcon(GbIconType::Check, QColor(pal.accent)));
    connect(enable_all_mods_btn, &QPushButton::clicked, this, [this]() {
        if (!installed_tree) return;
        for (int i = 0; i < installed_tree->topLevelItemCount(); ++i) {
            if (auto* it = installed_tree->topLevelItem(i); it && !it->isHidden()) {
                it->setCheckState(0, Qt::Checked);
            }
        }
    });
    installed_top_bar->addWidget(enable_all_mods_btn);

    auto* disable_all_mods_btn = new QPushButton(StormLang(QStringLiteral("Выключить все"), QStringLiteral("Disable All")), this);
    disable_all_mods_btn->setIcon(MakeGbIcon(GbIconType::Cross, QColor(pal.textSecondary)));
    connect(disable_all_mods_btn, &QPushButton::clicked, this, [this]() {
        if (!installed_tree) return;
        for (int i = 0; i < installed_tree->topLevelItemCount(); ++i) {
            if (auto* it = installed_tree->topLevelItem(i); it && !it->isHidden()) {
                it->setCheckState(0, Qt::Unchecked);
            }
        }
    });
    installed_top_bar->addWidget(disable_all_mods_btn);

    refresh_installed_btn = new QPushButton(StormLang(QStringLiteral("Обновить"), QStringLiteral("Refresh")), this);
    refresh_installed_btn->setIcon(MakeGbIcon(GbIconType::Refresh, QColor(pal.textSecondary)));
    refresh_installed_btn->setStyleSheet(QStringLiteral("font-weight: bold; padding: 5px 12px;"));
    installed_top_bar->addWidget(refresh_installed_btn);

    open_installed_folder_btn = new QPushButton(StormLang(QStringLiteral("Папка модов"), QStringLiteral("Mods Folder")), this);
    open_installed_folder_btn->setIcon(MakeGbIcon(GbIconType::Folder, QColor(pal.textSecondary)));
    open_installed_folder_btn->setStyleSheet(QStringLiteral("padding: 5px 12px;"));
    installed_top_bar->addWidget(open_installed_folder_btn);

    delete_mod_btn = new QPushButton(StormLang(QStringLiteral("Удалить выбранный"), QStringLiteral("Delete Selected")), this);
    delete_mod_btn->setIcon(MakeGbIcon(GbIconType::Trash, QColor(QStringLiteral("#EF4444"))));
    delete_mod_btn->setStyleSheet(QStringLiteral("QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ef4444, stop:1 #b91c1c); color: #FFFFFF; font-weight: bold; padding: 5px 14px; border: 1px solid #dc2626; border-top: 1px solid rgba(255,255,255,0.35); border-radius: 5px; } QPushButton:hover { background: #dc2626; } QPushButton:pressed { background: #991b1b; }"));
    installed_top_bar->addWidget(delete_mod_btn);

    installed_layout->addLayout(installed_top_bar);

    installed_tree = new QTreeWidget(this);
    installed_tree->setStyleSheet(QStringLiteral("QTreeWidget::item { height: 34px; padding: 2px 6px; }"));
    installed_tree->setHeaderLabels({tr("НАЗВАНИЕ МОДА"), tr("СТАТУС"), tr("РАЗМЕР"), tr("ПУТЬ К ПАПКЕ")});
    installed_tree->setAlternatingRowColors(true);
    installed_tree->setRootIsDecorated(false);
    for (int i = 0; i < 4; ++i) {
        installed_tree->headerItem()->setTextAlignment(i, Qt::AlignCenter);
    }
    installed_tree->header()->setStretchLastSection(true);
    installed_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    installed_tree->header()->resizeSection(0, 320);
    installed_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    installed_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    installed_tree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    installed_layout->addWidget(installed_tree, 1);

    view_stack->addWidget(online_page);
    view_stack->addWidget(installed_page);
    main_layout->addWidget(view_stack, 1);

    // Connect switcher and installed buttons
    connect(tab_online_btn, &QPushButton::clicked, this, [this]() { OnSwitchView(0); });
    connect(tab_installed_btn, &QPushButton::clicked, this, [this]() { OnSwitchView(1); });
    connect(refresh_installed_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::RefreshInstalledMods);
    connect(open_installed_folder_btn, &QPushButton::clicked, this, [this]() {
        const auto load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) / fmt::format("{:016X}", title_id);
        std::filesystem::create_directories(load_dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(Common::FS::PathToUTF8String(load_dir))));
    });
    connect(delete_mod_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnDeleteSelectedInstalledMod);
    connect(installed_tree, &QTreeWidget::itemChanged, this, &ConfigureGameBananaMods::OnInstalledItemChanged);

    // Connect signals
    connect(search_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnSearchClicked);
    connect(search_input, &QLineEdit::returnPressed, this, &ConfigureGameBananaMods::OnSearchClicked);
    connect(refresh_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnSearchClicked);
    connect(sort_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigureGameBananaMods::OnSortChanged);
    connect(first_page_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnFirstPage);
    connect(prev_page_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnPrevPage);
    connect(next_page_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnNextPage);
    connect(mod_tree, &QTreeWidget::currentItemChanged, this, &ConfigureGameBananaMods::OnModSelected);
    connect(mod_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        OnModSelected(item, nullptr);
    });
    connect(install_btn, &QPushButton::clicked, this, &ConfigureGameBananaMods::OnInstallClicked);
    connect(open_folder_btn, &QPushButton::clicked, this, [this]() {
        const auto load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) / fmt::format("{:016X}", title_id);
        std::filesystem::create_directories(load_dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(Common::FS::PathToUTF8String(load_dir))));
    });

    SetGameInfo(title_id, game_name);
}

ConfigureGameBananaMods::~ConfigureGameBananaMods() {
    if (current_reply) {
        current_reply->abort();
    }
    if (download_reply) {
        download_reply->abort();
    }
}

void ConfigureGameBananaMods::SetGameInfo(u64 title_id_, const QString& game_name_) {
    this->title_id = title_id_;
    this->game_name = game_name_;
    this->gamebanana_game_id = ResolveGameBananaGameId(title_id, game_name);
    this->current_page = 1;

    QString clean_title = game_name;
    clean_title.remove(QRegularExpression(QStringLiteral("\\[.*?\\]")));
    clean_title.remove(QRegularExpression(QStringLiteral("\\(.*?\\)")));
    clean_title = clean_title.trimmed();

    QString game_id_badge = (gamebanana_game_id > 0)
        ? QStringLiteral(" | GameBanana ID: %1").arg(FormatNumber(gamebanana_game_id))
        : QString{};

    game_header_label->setText(StormLang(QStringLiteral("Игра: %1  |  Платформа: Nintendo Switch (1, 2)  |  Title ID: %2%3"), QStringLiteral("Game: %1  |  Platform: Nintendo Switch (1, 2)  |  Title ID: %2%3"))
        .arg(clean_title, QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char('0')).toUpper(), game_id_badge));

    SearchMods({}, 1);
}

void ConfigureGameBananaMods::OnSearchClicked() {
    SearchMods(search_input->text().trimmed(), 1);
}

void ConfigureGameBananaMods::OnSortChanged(int) {
    if (gamebanana_game_id > 0 && search_input->text().trimmed().isEmpty()) {
        SearchMods({}, 1);
    } else {
        ApplySorting();
        PopulateModTree();
    }
}

void ConfigureGameBananaMods::OnFirstPage() {
    SearchMods(search_input->text().trimmed(), 1);
}

void ConfigureGameBananaMods::OnPrevPage() {
    if (current_page > 1) {
        SearchMods(search_input->text().trimmed(), current_page - 1);
    }
}

void ConfigureGameBananaMods::OnNextPage() {
    SearchMods(search_input->text().trimmed(), current_page + 1);
}

void ConfigureGameBananaMods::ApplySorting() {
    const int sort_idx = sort_combo->currentIndex();
    if (sort_idx == 0) {
        // Downloads
        std::sort(current_mods.begin(), current_mods.end(), [](const auto& a, const auto& b) {
            return a.downloads > b.downloads;
        });
    } else if (sort_idx == 1) {
        // Likes
        std::sort(current_mods.begin(), current_mods.end(), [](const auto& a, const auto& b) {
            return a.likes > b.likes;
        });
    } else if (sort_idx == 2) {
        // Views
        std::sort(current_mods.begin(), current_mods.end(), [](const auto& a, const auto& b) {
            return a.views > b.views;
        });
    } else if (sort_idx == 3) {
        // Date / ID (newest first)
        std::sort(current_mods.begin(), current_mods.end(), [](const auto& a, const auto& b) {
            return a.id > b.id;
        });
    } else if (sort_idx == 4) {
        // Alphabetical
        std::sort(current_mods.begin(), current_mods.end(), [](const auto& a, const auto& b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
    }
}

void ConfigureGameBananaMods::PopulateModTree() {
    mod_tree->clear();
    for (const auto& item : current_mods) {
        if (!active_category_filter.isEmpty()) {
            if (!item.category.contains(active_category_filter, Qt::CaseInsensitive) &&
                !item.name.contains(active_category_filter, Qt::CaseInsensitive)) {
                continue;
            }
        }
        auto* tree_item = new QTreeWidgetItem(mod_tree);
        tree_item->setText(0, item.name);
        tree_item->setText(1, item.submitter.isEmpty() ? QStringLiteral("-") : item.submitter);
        const qint64 count_val = item.views > 0 ? item.views : item.downloads;
        tree_item->setText(2, FormatNumber(count_val));
        tree_item->setText(3, FormatNumber(item.likes));
        tree_item->setText(4, item.category.isEmpty() ? tr("Мод") : item.category);
        tree_item->setData(0, Qt::UserRole, item.id);
        tree_item->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        for (int i = 1; i < 5; ++i) {
            tree_item->setTextAlignment(i, Qt::AlignCenter);
        }
    }
}

void ConfigureGameBananaMods::SearchMods(const QString& query, int page) {
    if (current_reply) {
        current_reply->abort();
        current_reply = nullptr;
    }

    current_page = std::max(1, page);
    page_label->setText(tr("Страница %1").arg(FormatNumber(current_page)));
    first_page_btn->setEnabled(current_page > 1);
    prev_page_btn->setEnabled(current_page > 1);
    next_page_btn->setEnabled(false);

    mod_tree->clear();
    current_mods.clear();
    detail_browser->clear();
    files_combo->clear();
    install_btn->setEnabled(false);
    status_label->setText(tr("Поиск модов на GameBanana (стр. %1)...").arg(FormatNumber(current_page)));

    QString clean_title = game_name;
    clean_title.remove(QRegularExpression(QStringLiteral("\\[.*?\\]")));
    clean_title.remove(QRegularExpression(QStringLiteral("\\(.*?\\)")));
    clean_title = clean_title.trimmed();

    QUrl search_url(QStringLiteral("https://gamebanana.com/apiv11/Util/Search/Results"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("_nPage"), QString::number(current_page));
    q.addQueryItem(QStringLiteral("_nPerpage"), QStringLiteral("40"));

    QString search_term = query.trimmed();
    if (search_term.isEmpty()) {
        search_term = clean_title.isEmpty() ? QStringLiteral("Nintendo Switch") : clean_title;
    } else {
        if (!clean_title.isEmpty() && !search_term.contains(clean_title, Qt::CaseInsensitive)) {
            search_term = QStringLiteral("%1 %2").arg(clean_title, search_term);
        }
    }
    q.addQueryItem(QStringLiteral("_sSearchString"), search_term);
    q.addQueryItem(QStringLiteral("_sModelName"), QStringLiteral("Mod"));
    search_url.setQuery(q);

    QNetworkRequest request(search_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM-SWITCH-Client/4.8.8"));

    current_reply = network_manager->get(request);
    connect(current_reply, &QNetworkReply::finished, this, [this]() {
        if (!current_reply) return;
        if (current_reply->error() != QNetworkReply::NoError) {
            status_label->setText(tr("Ошибка подключения к GameBanana: %1").arg(current_reply->errorString()));
            current_reply->deleteLater();
            current_reply = nullptr;
            return;
        }

        const auto data = current_reply->readAll();
        current_reply->deleteLater();
        current_reply = nullptr;

        const auto doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            status_label->setText(tr("Не удалось прочитать ответ GameBanana."));
            return;
        }

        const auto root_obj = doc.object();
        const auto meta = root_obj.value(QStringLiteral("_aMetadata")).toObject();
        const int total_records = meta.value(QStringLiteral("_nRecordCount")).toInt();
        const bool is_complete = meta.value(QStringLiteral("_bIsComplete")).toBool();
        const int per_page = meta.value(QStringLiteral("_nPerpage")).toInt(40);
        const int total_pages = total_records > 0 ? std::max(1, (total_records + per_page - 1) / per_page) : 1;

        const auto records = root_obj.value(QStringLiteral("_aRecords")).toArray();

        for (const auto& val : records) {
            const auto obj = val.toObject();

            GameBananaModItem item;
            item.id = obj.value(QStringLiteral("_idRow")).toInt();
            item.name = obj.value(QStringLiteral("_sName")).toString();
            if (obj.contains(QStringLiteral("_aSubmitter"))) {
                item.submitter = obj.value(QStringLiteral("_aSubmitter")).toObject().value(QStringLiteral("_sName")).toString();
            }
            if (obj.contains(QStringLiteral("_aRootCategory"))) {
                item.category = obj.value(QStringLiteral("_aRootCategory")).toObject().value(QStringLiteral("_sName")).toString();
            } else if (obj.contains(QStringLiteral("_sSingularTitle"))) {
                item.category = obj.value(QStringLiteral("_sSingularTitle")).toString();
            } else if (obj.contains(QStringLiteral("_sModelName"))) {
                item.category = obj.value(QStringLiteral("_sModelName")).toString();
            }
            item.downloads = obj.value(QStringLiteral("_nDownloadCount")).toInt();
            item.likes = obj.value(QStringLiteral("_nLikeCount")).toInt();
            item.views = obj.value(QStringLiteral("_nViewCount")).toInt();

            qint64 ts = obj.value(QStringLiteral("_tsDateAdded")).toVariant().toLongLong();
            if (ts > 0) {
                item.date = QDateTime::fromSecsSinceEpoch(ts).toString(QStringLiteral("dd.MM.yyyy"));
            }

            current_mods.push_back(item);
        }

        ApplySorting();
        PopulateModTree();

        page_label->setText(total_records > 0 ? tr("Страница %1 из %2").arg(FormatNumber(current_page)).arg(FormatNumber(total_pages)) : tr("Страница %1").arg(FormatNumber(current_page)));
        first_page_btn->setEnabled(current_page > 1);
        prev_page_btn->setEnabled(current_page > 1);
        next_page_btn->setEnabled((current_page < total_pages && !is_complete) || (!records.isEmpty() && !is_complete));

        if (current_mods.empty()) {
            status_label->setText(tr("Моды для игры не найдены. Попробуйте изменить поисковый запрос."));
            if (details_widget) {
                details_widget->setVisible(false);
            }
        } else {
            status_label->setText(tr("Всего доступно модов: %1  |  Страница %2 из %3 (показано %4)").arg(FormatNumber(total_records > 0 ? total_records : current_mods.size())).arg(FormatNumber(current_page)).arg(FormatNumber(total_pages)).arg(FormatNumber(current_mods.size())));
        }
    });
}

void ConfigureGameBananaMods::OnModSelected(QTreeWidgetItem* current, QTreeWidgetItem*) {
    if (!current) return;
    int mod_id = current->data(0, Qt::UserRole).toInt();
    if (mod_id <= 0) return;

    if (details_widget) {
        details_widget->setVisible(true);
    }

    selected_mod_id = mod_id;
    selected_mod_name = current->text(0);
    LoadModDetails(mod_id);
}

void ConfigureGameBananaMods::LoadModDetails(int mod_id) {
    if (current_reply) {
        current_reply->abort();
        current_reply = nullptr;
    }

    files_combo->clear();
    current_files.clear();
    install_btn->setEnabled(false);
    detail_browser->setHtml(tr("<h3>Загрузка информации о моде...</h3>"));

    QUrl details_url(QStringLiteral("https://gamebanana.com/apiv11/Mod/%1/ProfilePage").arg(mod_id));
    QNetworkRequest request(details_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH_Emulator/7.2.0"));

    current_reply = network_manager->get(request);
    connect(current_reply, &QNetworkReply::finished, this, [this]() {
        if (!current_reply) return;
        if (current_reply->error() != QNetworkReply::NoError) {
            detail_browser->setHtml(tr("<b>Ошибка загрузки деталей:</b> %1").arg(current_reply->errorString()));
            current_reply->deleteLater();
            current_reply = nullptr;
            return;
        }

        const auto data = current_reply->readAll();
        current_reply->deleteLater();
        current_reply = nullptr;

        const auto doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) return;

        const auto obj = doc.object();
        const QString name = obj.value(QStringLiteral("_sName")).toString();
        const QString desc = obj.value(QStringLiteral("_sDescription")).toString();
        const QString text = obj.value(QStringLiteral("_sText")).toString();
        const QString author = obj.value(QStringLiteral("_aSubmitter")).toObject().value(QStringLiteral("_sName")).toString();
        const qint64 views_count = obj.value(QStringLiteral("_nViewCount")).toVariant().toLongLong();
        const qint64 likes_count = obj.value(QStringLiteral("_nLikeCount")).toVariant().toLongLong();

        const auto pal = StormTheme::GetThemePalette();
        if (inspector_title_label) {
            inspector_title_label->setText(name);
        }
        if (mod_author_pill) {
            mod_author_pill->setText(StormLang(QStringLiteral("Автор: %1"), QStringLiteral("Author: %1")).arg(author.isEmpty() ? QStringLiteral("-") : author));
        }
        if (mod_downloads_pill) {
            mod_downloads_pill->setText(StormLang(QStringLiteral("Просмотров: %1"), QStringLiteral("Views: %1")).arg(FormatNumber(views_count)));
        }
        if (mod_likes_pill) {
            mod_likes_pill->setText(StormLang(QStringLiteral("Лайков: %1"), QStringLiteral("Likes: %1")).arg(FormatNumber(likes_count)));
        }
        if (mod_category_pill) {
            mod_category_pill->setText(obj.value(QStringLiteral("_sModelName")).toString());
        }

        QString html = QStringLiteral("<div style='color:%1; line-height:1.4;'><p><i>%2</i></p><hr style='border: none;'/><div>%4</div></div>")
                           .arg(pal.textPrimary, desc, pal.border, text.isEmpty() ? desc : text);
        detail_browser->setHtml(html);

        // Parse Files
        const auto files_arr = obj.value(QStringLiteral("_aFiles")).toArray();
        for (const auto& fval : files_arr) {
            const auto fobj = fval.toObject();
            GameBananaFile file_info;
            file_info.id = fobj.value(QStringLiteral("_idRow")).toInt();
            file_info.name = fobj.value(QStringLiteral("_sFile")).toString();
            file_info.url = fobj.value(QStringLiteral("_sDownloadUrl")).toString();
            file_info.size = fobj.value(QStringLiteral("_nFilesize")).toVariant().toLongLong();
            file_info.description = fobj.value(QStringLiteral("_sDescription")).toString();

            current_files.push_back(file_info);

            const double size_mb = static_cast<double>(file_info.size) / (1024.0 * 1024.0);
            const qint64 file_dl = fobj.value(QStringLiteral("_nDownloadCount")).toVariant().toLongLong();
            const QString sz_str = StormFormat::ReadableByteSize(static_cast<qulonglong>(file_info.size));
            QString item_label = StormLang(
                QStringLiteral("%1 (%2, скачиваний: %3)").arg(file_info.name, sz_str, FormatNumber(file_dl)),
                QStringLiteral("%1 (%2, downloads: %3)").arg(file_info.name, sz_str, FormatNumber(file_dl))
            );
            if (!file_info.description.isEmpty()) {
                item_label += QStringLiteral(" - %1").arg(file_info.description);
            }
            files_combo->addItem(item_label, file_info.id);
        }

        if (!current_files.empty() && mod_size_pill) {
            mod_size_pill->setText(StormFormat::ReadableByteSize(static_cast<qulonglong>(current_files[0].size)));
        }

        // Parse screenshots from _aPreviewMedia
        current_screenshot_urls.clear();
        current_screenshot_index = 0;
        if (obj.contains(QStringLiteral("_aPreviewMedia"))) {
            const auto pmedia = obj.value(QStringLiteral("_aPreviewMedia")).toObject();
            const auto imgs = pmedia.value(QStringLiteral("_aImages")).toArray();
            for (const auto& ival : imgs) {
                const auto iobj = ival.toObject();
                QString base_url = iobj.value(QStringLiteral("_sBaseUrl")).toString();
                QString fn = iobj.value(QStringLiteral("_sFile530")).toString();
                if (fn.isEmpty()) fn = iobj.value(QStringLiteral("_sFile")).toString();
                if (!base_url.isEmpty() && !fn.isEmpty()) {
                    current_screenshot_urls.push_back(base_url + QStringLiteral("/") + fn);
                }
            }
        }
        UpdateScreenshotDisplay();

        install_btn->setEnabled(!current_files.empty());
    });
}

void ConfigureGameBananaMods::OnInstallClicked() {
    if (files_combo->currentIndex() < 0 || files_combo->currentIndex() >= static_cast<int>(current_files.size())) {
        return;
    }

    const auto& selected_file = current_files[files_combo->currentIndex()];
    DownloadAndInstallMod(selected_file, selected_mod_name);
}

void ConfigureGameBananaMods::OnDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        progress_bar->setValue(static_cast<int>((bytesReceived * 100) / bytesTotal));
    }
}

void ConfigureGameBananaMods::DownloadAndInstallMod(const GameBananaFile& file_info, const QString& mod_name) {
    if (file_info.url.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Ссылка для скачивания файла отсутствует."));
        return;
    }

    install_btn->setEnabled(false);
    progress_bar->setVisible(true);
    progress_bar->setValue(0);
    status_label->setStyleSheet(QStringLiteral("color: #38bdf8; font-weight: bold;"));
    status_label->setText(tr("Скачивание мода '%1'...").arg(file_info.name));

    QNetworkRequest request(QUrl(file_info.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM-SWITCH-Client/4.8.8"));

    download_reply = network_manager->get(request);
    connect(download_reply, &QNetworkReply::downloadProgress, this, &ConfigureGameBananaMods::OnDownloadProgress);
    connect(download_reply, &QNetworkReply::finished, this, [this, file_info, mod_name]() {
        if (!download_reply) return;
        progress_bar->setVisible(false);
        install_btn->setEnabled(true);

        if (download_reply->error() != QNetworkReply::NoError) {
            status_label->setStyleSheet(QStringLiteral("color: #ff5252; font-weight: bold;"));
            status_label->setText(tr("Ошибка скачивания: %1").arg(download_reply->errorString()));
            download_reply->deleteLater();
            download_reply = nullptr;
            return;
        }

        const auto file_data = download_reply->readAll();
        download_reply->deleteLater();
        download_reply = nullptr;

        // Save to temporary file
        const auto temp_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "temp_mods";
        std::filesystem::create_directories(temp_dir);
        const auto temp_file_path = temp_dir / file_info.name.toStdString();

        {
            std::ofstream out(temp_file_path, std::ios::binary);
            out.write(file_data.data(), file_data.size());
        }

        status_label->setText(tr("Распаковка и установка мода..."));

        const QString temp_file_str = QString::fromStdString(Common::FS::PathToUTF8String(temp_file_path));
        const QString extracted = QtCommon::Mod::ExtractMod(temp_file_str);

        // Remove temporary archive file
        std::error_code ec;
        std::filesystem::remove(temp_file_path, ec);

        if (extracted.isEmpty()) {
            status_label->setStyleSheet(QStringLiteral("color: #ff5252; font-weight: bold;"));
            status_label->setText(StormLang(QStringLiteral("Ошибка распаковки архива мода!"), QStringLiteral("Error extracting mod archive!")));
            return;
        }

        QString clean_mod_name = mod_name.trimmed();
        if (clean_mod_name.isEmpty()) {
            clean_mod_name = QFileInfo(file_info.name).baseName();
        }
        clean_mod_name = clean_mod_name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_")).trimmed();
        if (clean_mod_name.isEmpty()) {
            clean_mod_name = QStringLiteral("GameBananaMod");
        }

        const auto load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) / fmt::format("{:016X}", title_id);
        const auto target_mod_dir = load_dir / clean_mod_name.toStdString();

        const bool organized = QtCommon::Mod::OrganizeModStructure(
            std::filesystem::path(extracted.toStdString()),
            target_mod_dir);

        // Remove temporary extraction folder
        std::filesystem::remove_all(std::filesystem::path(extracted.toStdString()), ec);

        if (!organized) {
            status_label->setStyleSheet(QStringLiteral("color: #ff5252; font-weight: bold;"));
            status_label->setText(StormLang(QStringLiteral("Не удалось структурировать файлы мода!"), QStringLiteral("Could not structure mod files!")));
            QMessageBox::warning(this, tr("Ошибка установки"),
                                 tr("Не удалось распознать структуру файлов мода «%1».").arg(clean_mod_name));
            return;
        }

        // Mark this mod folder as installed from GameBanana
        {
            const auto gb_marker_path = target_mod_dir / ".gamebanana.json";
            QFile gb_marker(QString::fromStdString(gb_marker_path.string()));
            if (gb_marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QJsonObject meta;
                meta[QStringLiteral("source")] = QStringLiteral("gamebanana");
                meta[QStringLiteral("name")] = clean_mod_name;
                gb_marker.write(QJsonDocument(meta).toJson(QJsonDocument::Compact));
                gb_marker.close();
            }
        }

        // Enable the mod in settings if it was disabled
        auto& disabled = Settings::values.disabled_addons[title_id];
        disabled.erase(std::remove(disabled.begin(), disabled.end(), clean_mod_name.toStdString()), disabled.end());

        status_label->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(StormTheme::GetThemePalette().accent));
        status_label->setText(StormLang(QStringLiteral("Мод '%1' успешно установлен и активирован!").arg(clean_mod_name), QStringLiteral("Mod '%1' installed and enabled!").arg(clean_mod_name)));

        RefreshInstalledMods();

        QMessageBox::information(this, tr("Установка завершена"),
                                 tr("Мод «%1» успешно скачан, внедрен в игру и активирован в параметрах!").arg(clean_mod_name));

        emit ModInstalled();
    });
}

void ConfigureGameBananaMods::OnSwitchView(int view_index) {
    const auto pal = StormTheme::GetThemePalette();
    const QString active_style = QStringLiteral(
        "QPushButton { background-color: %1; color: %2; font-weight: bold; padding: 6px 18px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }"
    ).arg(pal.accent, pal.is_light ? QStringLiteral("#FFFFFF") : QStringLiteral("#000000"), pal.accentHover);
    const QString inactive_style = QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; font-weight: bold; padding: 6px 18px; border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background-color: %4; }"
    ).arg(pal.buttonBg, pal.textPrimary, pal.border, pal.surface);

    if (view_index == 0) {
        tab_online_btn->setChecked(true);
        tab_installed_btn->setChecked(false);
        tab_online_btn->setStyleSheet(active_style);
        tab_installed_btn->setStyleSheet(inactive_style);
        view_stack->setCurrentIndex(0);
    } else {
        tab_online_btn->setChecked(false);
        tab_installed_btn->setChecked(true);
        tab_installed_btn->setStyleSheet(active_style);
        tab_online_btn->setStyleSheet(inactive_style);
        view_stack->setCurrentIndex(1);
        RefreshInstalledMods();
    }
}

void ConfigureGameBananaMods::RefreshInstalledMods() {
    installed_tree->blockSignals(true);
    installed_tree->clear();

    const auto load_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) / fmt::format("{:016X}", title_id);
    const auto& disabled_entries = Settings::values.disabled_addons[title_id];

    int total_count = 0;
    int enabled_count = 0;

    if (std::filesystem::exists(load_dir) && std::filesystem::is_directory(load_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(load_dir)) {
            if (!entry.is_directory()) continue;

            // Only show mods actually installed from GameBanana on the GameBanana tab.
            // Manually installed mods (like MOD - M. RUS) are managed on the Add-ons tab.
            std::error_code marker_ec;
            if (!std::filesystem::exists(entry.path() / ".gamebanana.json", marker_ec)) {
                continue;
            }

            const QString mod_name = QString::fromStdString(entry.path().filename().string());
            if (mod_name.isEmpty()) continue;

            total_count++;

            const std::string mod_str = mod_name.toStdString();
            const bool is_disabled = std::ranges::find(disabled_entries, mod_str) != disabled_entries.end();
            const bool is_enabled = !is_disabled;
            if (is_enabled) {
                enabled_count++;
            }

            u64 total_size = 0;
            std::error_code ec;
            for (const auto& f : std::filesystem::recursive_directory_iterator(entry.path(), ec)) {
                if (f.is_regular_file()) {
                    total_size += f.file_size(ec);
                }
            }
            const double size_mb = static_cast<double>(total_size) / (1024.0 * 1024.0);
            QString size_str;
            if (size_mb >= 1.0) {
                size_str = tr("%1 МБ").arg(QString::number(size_mb, 'f', 1));
            } else {
                size_str = tr("%1 КБ").arg(QString::number(static_cast<double>(total_size) / 1024.0, 'f', 0));
            }

            auto* tree_item = new QTreeWidgetItem(installed_tree);
            tree_item->setText(0, mod_name);
            tree_item->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
            tree_item->setCheckState(0, is_enabled ? Qt::Checked : Qt::Unchecked);
            tree_item->setData(0, Qt::UserRole, mod_name);

            if (is_enabled) {
                tree_item->setText(1, StormLang(QStringLiteral("ВКЛЮЧЕН"), QStringLiteral("ENABLED")));
                tree_item->setForeground(1, QColor(QStringLiteral("#00f2fe")));
            } else {
                tree_item->setText(1, StormLang(QStringLiteral("ОТКЛЮЧЕН"), QStringLiteral("DISABLED")));
                tree_item->setForeground(1, QColor(QStringLiteral("#718096")));
            }
            tree_item->setTextAlignment(1, Qt::AlignCenter);

            tree_item->setText(2, size_str);
            tree_item->setForeground(2, QColor(QStringLiteral("#38bdf8")));
            tree_item->setTextAlignment(2, Qt::AlignCenter);

            tree_item->setText(3, QString::fromStdString(entry.path().string()));
            tree_item->setForeground(3, QColor(QStringLiteral("#a0aec0")));
            tree_item->setTextAlignment(3, Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (total_count == 0) {
        installed_status_label->setText(tr("Установленные моды для этой игры не найдены. Выберите нужные моды во вкладке «Каталог GameBanana»."));
    } else {
        installed_status_label->setText(tr("Всего модов: <b>%1</b> &nbsp;|&nbsp; Включено: <b style='color:#00f2fe;'>%2</b> &nbsp;|&nbsp; Отключено: <b style='color:#ff5252;'>%3</b>")
            .arg(total_count).arg(enabled_count).arg(total_count - enabled_count));
    }

    installed_tree->blockSignals(false);
}

void ConfigureGameBananaMods::OnInstalledItemChanged(QTreeWidgetItem* item, int column) {
    if (column != 0 || !item) return;

    const QString mod_name = item->data(0, Qt::UserRole).toString();
    if (mod_name.isEmpty()) return;

    const bool is_checked = (item->checkState(0) == Qt::Checked);
    const std::string mod_str = mod_name.toStdString();
    auto& disabled_entries = Settings::values.disabled_addons[title_id];

    if (is_checked) {
        std::erase(disabled_entries, mod_str);
        item->setText(1, StormLang(QStringLiteral("ВКЛЮЧЕН"), QStringLiteral("ENABLED")));
        item->setForeground(1, QColor(QStringLiteral("#00f2fe")));
    } else {
        if (std::ranges::find(disabled_entries, mod_str) == disabled_entries.end()) {
            disabled_entries.push_back(mod_str);
        }
        item->setText(1, StormLang(QStringLiteral("ОТКЛЮЧЕН"), QStringLiteral("DISABLED")));
        item->setForeground(1, QColor(QStringLiteral("#718096")));
    }

    int total_count = installed_tree->topLevelItemCount();
    int enabled_count = 0;
    for (int i = 0; i < total_count; ++i) {
        if (installed_tree->topLevelItem(i)->checkState(0) == Qt::Checked) {
            enabled_count++;
        }
    }
    installed_status_label->setText(tr("Всего модов: <b>%1</b> &nbsp;|&nbsp; Включено: <b style='color:#00f2fe;'>%2</b> &nbsp;|&nbsp; Отключено: <b style='color:#ff5252;'>%3</b>")
        .arg(total_count).arg(enabled_count).arg(total_count - enabled_count));

    UISettings::values.is_game_list_reload_pending.exchange(true);
}

void ConfigureGameBananaMods::OnDeleteSelectedInstalledMod() {
    auto* item = installed_tree->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Удаление мода"), tr("Пожалуйста, выберите мод из списка для удаления."));
        return;
    }

    const QString mod_name = item->data(0, Qt::UserRole).toString();
    if (mod_name.isEmpty()) return;

    const auto ret = QMessageBox::question(
        this, tr("Удаление мода"),
        tr("Вы действительно хотите полностью удалить мод «%1» с диска?\nЭто действие нельзя отменить.").arg(mod_name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    const auto mod_path = Common::FS::GetEdenPath(Common::FS::EdenPath::LoadDir) / fmt::format("{:016X}", title_id) / mod_name.toStdString();
    std::error_code ec;
    std::filesystem::remove_all(mod_path, ec);

    auto& disabled_entries = Settings::values.disabled_addons[title_id];
    std::erase(disabled_entries, mod_name.toStdString());

    RefreshInstalledMods();
    emit ModInstalled();
    UISettings::values.is_game_list_reload_pending.exchange(true);

    QMessageBox::information(this, tr("Мод удален"), tr("Мод «%1» успешно удален.").arg(mod_name));
}

void ConfigureGameBananaMods::OnCategoryPillClicked(const QString& category) {
    for (auto* pill : category_pill_buttons) {
        pill->setChecked(pill->text() == category);
    }
    active_category_filter = (category == StormLang(QStringLiteral("Все"), QStringLiteral("All"))) ? QString{} : category;
    PopulateModTree();
}

void ConfigureGameBananaMods::OnPrevScreenshot() {
    if (current_screenshot_urls.empty()) return;
    current_screenshot_index = (current_screenshot_index - 1 + current_screenshot_urls.size()) % current_screenshot_urls.size();
    UpdateScreenshotDisplay();
}

void ConfigureGameBananaMods::OnNextScreenshot() {
    if (current_screenshot_urls.empty()) return;
    current_screenshot_index = (current_screenshot_index + 1) % current_screenshot_urls.size();
    UpdateScreenshotDisplay();
}

void ConfigureGameBananaMods::UpdateScreenshotDisplay() {
    const auto pal = StormTheme::GetThemePalette();
    if (current_screenshot_urls.empty()) {
        if (prev_screenshot_btn) prev_screenshot_btn->setEnabled(false);
        if (next_screenshot_btn) next_screenshot_btn->setEnabled(false);
        if (screenshot_counter_label) screenshot_counter_label->setText(StormLang(QStringLiteral("Нет скриншотов"), QStringLiteral("No screenshots")));
        if (screenshot_label) screenshot_label->setPixmap(MakeScreenshotPlaceholder(280, 160, pal));
        return;
    }

    if (prev_screenshot_btn) prev_screenshot_btn->setEnabled(current_screenshot_urls.size() > 1);
    if (next_screenshot_btn) next_screenshot_btn->setEnabled(current_screenshot_urls.size() > 1);
    if (screenshot_counter_label) {
        screenshot_counter_label->setText(QStringLiteral("%1 / %2").arg(current_screenshot_index + 1).arg(current_screenshot_urls.size()));
    }

    const QString url = current_screenshot_urls[current_screenshot_index];
    if (screenshot_reply) {
        screenshot_reply->abort();
        screenshot_reply = nullptr;
    }

    screenshot_reply = network_manager->get(QNetworkRequest(QUrl(url)));
    connect(screenshot_reply, &QNetworkReply::finished, this, [this, url]() {
        if (!screenshot_reply) return;
        if (screenshot_reply->error() == QNetworkReply::NoError) {
            QPixmap pix;
            pix.loadFromData(screenshot_reply->readAll());
            if (!pix.isNull() && screenshot_label) {
                screenshot_label->setPixmap(pix.scaled(280, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        screenshot_reply->deleteLater();
        screenshot_reply = nullptr;
    });
}
