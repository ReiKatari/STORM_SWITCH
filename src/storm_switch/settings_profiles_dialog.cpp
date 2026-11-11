// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/settings_profiles_dialog.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_design_engine.h"

#include <filesystem>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/configuration_shared.h"
#include "storm_switch/configuration/shared_widget.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

enum class ProfileVectorIcon {
    Sliders,
    Save,
    Import,
    Export,
    Delete,
    Edit,
    Check,
    Search,
    Close,
    PresetStar,
    CustomUser,
};

static QIcon DrawProfileVectorIcon(ProfileVectorIcon kind, const QColor& color, int size = 16) {
    const int px_sz = size * 2;
    QPixmap pix(px_sz, px_sz);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPen pen(color);
    pen.setWidthF(qMax(1.6, 1.8 * (px_sz / 16.0)));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal s = px_sz;

    switch (kind) {
    case ProfileVectorIcon::Sliders: {
        // Line 1
        p.drawLine(QPointF(s * 0.15, s * 0.28), QPointF(s * 0.85, s * 0.28));
        p.setBrush(color);
        p.drawEllipse(QRectF(s * 0.30, s * 0.20, s * 0.16, s * 0.16));
        p.setBrush(Qt::NoBrush);
        // Line 2
        p.drawLine(QPointF(s * 0.15, s * 0.50), QPointF(s * 0.85, s * 0.50));
        p.setBrush(color);
        p.drawEllipse(QRectF(s * 0.60, s * 0.42, s * 0.16, s * 0.16));
        p.setBrush(Qt::NoBrush);
        // Line 3
        p.drawLine(QPointF(s * 0.15, s * 0.72), QPointF(s * 0.85, s * 0.72));
        p.setBrush(color);
        p.drawEllipse(QRectF(s * 0.38, s * 0.64, s * 0.16, s * 0.16));
        break;
    }
    case ProfileVectorIcon::Save: {
        QPainterPath path;
        path.moveTo(s * 0.20, s * 0.20);
        path.lineTo(s * 0.68, s * 0.20);
        path.lineTo(s * 0.82, s * 0.34);
        path.lineTo(s * 0.82, s * 0.82);
        path.lineTo(s * 0.20, s * 0.82);
        path.closeSubpath();
        p.drawPath(path);
        // Inner window
        p.drawRect(QRectF(s * 0.32, s * 0.52, s * 0.38, s * 0.30));
        break;
    }
    case ProfileVectorIcon::Import: {
        // Arrow pointing down into tray
        p.drawLine(QPointF(s * 0.50, s * 0.18), QPointF(s * 0.50, s * 0.62));
        p.drawLine(QPointF(s * 0.34, s * 0.46), QPointF(s * 0.50, s * 0.62));
        p.drawLine(QPointF(s * 0.66, s * 0.46), QPointF(s * 0.50, s * 0.62));
        // Tray
        p.drawLine(QPointF(s * 0.20, s * 0.60), QPointF(s * 0.20, s * 0.82));
        p.drawLine(QPointF(s * 0.20, s * 0.82), QPointF(s * 0.80, s * 0.82));
        p.drawLine(QPointF(s * 0.80, s * 0.82), QPointF(s * 0.80, s * 0.60));
        break;
    }
    case ProfileVectorIcon::Export: {
        // Arrow pointing up out of tray
        p.drawLine(QPointF(s * 0.50, s * 0.62), QPointF(s * 0.50, s * 0.18));
        p.drawLine(QPointF(s * 0.34, s * 0.34), QPointF(s * 0.50, s * 0.18));
        p.drawLine(QPointF(s * 0.66, s * 0.34), QPointF(s * 0.50, s * 0.18));
        // Tray
        p.drawLine(QPointF(s * 0.20, s * 0.60), QPointF(s * 0.20, s * 0.82));
        p.drawLine(QPointF(s * 0.20, s * 0.82), QPointF(s * 0.80, s * 0.82));
        p.drawLine(QPointF(s * 0.80, s * 0.82), QPointF(s * 0.80, s * 0.60));
        break;
    }
    case ProfileVectorIcon::Delete: {
        // Lid
        p.drawLine(QPointF(s * 0.22, s * 0.30), QPointF(s * 0.78, s * 0.30));
        p.drawLine(QPointF(s * 0.40, s * 0.22), QPointF(s * 0.60, s * 0.22));
        // Can body
        QPainterPath path;
        path.moveTo(s * 0.28, s * 0.30);
        path.lineTo(s * 0.32, s * 0.82);
        path.lineTo(s * 0.68, s * 0.82);
        path.lineTo(s * 0.72, s * 0.30);
        p.drawPath(path);
        // Slits
        p.drawLine(QPointF(s * 0.44, s * 0.42), QPointF(s * 0.44, s * 0.72));
        p.drawLine(QPointF(s * 0.56, s * 0.42), QPointF(s * 0.56, s * 0.72));
        break;
    }
    case ProfileVectorIcon::Edit: {
        p.drawLine(QPointF(s * 0.22, s * 0.78), QPointF(s * 0.38, s * 0.78));
        p.drawLine(QPointF(s * 0.22, s * 0.78), QPointF(s * 0.26, s * 0.62));
        p.drawLine(QPointF(s * 0.26, s * 0.62), QPointF(s * 0.68, s * 0.20));
        p.drawLine(QPointF(s * 0.68, s * 0.20), QPointF(s * 0.80, s * 0.32));
        p.drawLine(QPointF(s * 0.80, s * 0.32), QPointF(s * 0.38, s * 0.74));
        p.drawLine(QPointF(s * 0.38, s * 0.74), QPointF(s * 0.22, s * 0.78));
        break;
    }
    case ProfileVectorIcon::Check: {
        p.drawLine(QPointF(s * 0.20, s * 0.52), QPointF(s * 0.42, s * 0.74));
        p.drawLine(QPointF(s * 0.42, s * 0.74), QPointF(s * 0.82, s * 0.26));
        break;
    }
    case ProfileVectorIcon::Search: {
        p.drawEllipse(QRectF(s * 0.18, s * 0.18, s * 0.44, s * 0.44));
        p.drawLine(QPointF(s * 0.52, s * 0.52), QPointF(s * 0.82, s * 0.82));
        break;
    }
    case ProfileVectorIcon::Close: {
        p.drawLine(QPointF(s * 0.24, s * 0.24), QPointF(s * 0.76, s * 0.76));
        p.drawLine(QPointF(s * 0.76, s * 0.24), QPointF(s * 0.24, s * 0.76));
        break;
    }
    case ProfileVectorIcon::PresetStar: {
        QPainterPath path;
        path.addRoundedRect(QRectF(s * 0.18, s * 0.18, s * 0.64, s * 0.64), s * 0.18, s * 0.18);
        p.drawPath(path);
        p.drawLine(QPointF(s * 0.50, s * 0.32), QPointF(s * 0.50, s * 0.68));
        p.drawLine(QPointF(s * 0.32, s * 0.50), QPointF(s * 0.68, s * 0.50));
        break;
    }
    case ProfileVectorIcon::CustomUser: {
        p.drawEllipse(QRectF(s * 0.34, s * 0.18, s * 0.32, s * 0.32));
        QPainterPath path;
        path.arcMoveTo(QRectF(s * 0.20, s * 0.56, s * 0.60, s * 0.36), 0);
        path.arcTo(QRectF(s * 0.20, s * 0.56, s * 0.60, s * 0.36), 0, 180);
        p.drawPath(path);
        break;
    }
    }

    return QIcon(pix);
}

SettingsProfilesDialog::SettingsProfilesDialog(QWidget* parent, Core::System* system)
    : QDialog(parent), m_system(system) {
    setWindowTitle(StormLang(
        QStringLiteral("STORM SWITCH - Профили настроек эмуляции"),
        QStringLiteral("STORM SWITCH - Emulation Settings Profiles"),
        QStringLiteral("STORM SWITCH - Einstellungen-Profile"),
        QStringLiteral("STORM SWITCH - Profils des paramètres")));

    resize(860, 640);
    setMinimumSize(720, 520);
    StormUI::StructureEngine::InstallEscDismiss(this);

    InitBuiltinPresets();
    SetupUI();
    ApplyTheme();
    LoadCustomProfiles();
    RebuildCards();
}

SettingsProfilesDialog::~SettingsProfilesDialog() = default;

void SettingsProfilesDialog::InitBuiltinPresets() {
    m_builtin_presets.clear();

    // Preset 0: Максимум FPS / Киберспорт
    {
        StormSettingsProfile p;
        p.id = QStringLiteral("preset_max_fps");
        p.is_preset = true;
        p.name = StormLang(
            QStringLiteral("Максимум FPS / Киберспорт"),
            QStringLiteral("Maximum FPS / Esports"));
        p.description = StormLang(
            QStringLiteral("Максимальная частота кадров и мгновенный отклик. Оптимизировано для динамичных игр, быстрых сцен и слабых конфигураций."),
            QStringLiteral("Maximum frame rate and input responsiveness. Optimized for fast-paced action games and budget hardware."));
        p.res_pill = QStringLiteral("1x (720p/1080p)");
        p.gpu_pill = StormLang(QStringLiteral("Точность GPU: Низкая"), QStringLiteral("GPU: Low"));
        p.shaders_pill = StormLang(QStringLiteral("Шейдеры: Асинхронно"), QStringLiteral("Shaders: Async"));
        p.aa_pill = StormLang(QStringLiteral("Сглаживание: Выкл"), QStringLiteral("AA: Off"));
        p.cpu_pill = QStringLiteral("CPU: Dynarmic (JIT)");
        p.vsync_pill = QStringLiteral("VSync: Mailbox");
        m_builtin_presets.push_back(p);
    }

    // Preset 1: Сбалансированный (Рекомендуется)
    {
        StormSettingsProfile p;
        p.id = QStringLiteral("preset_balanced");
        p.is_preset = true;
        p.name = StormLang(
            QStringLiteral("Сбалансированный (Рекомендуется)"),
            QStringLiteral("Balanced (Recommended)"));
        p.description = StormLang(
            QStringLiteral("Идеальный баланс высокого качества графики, стабильных 60 кадров/с и плавной подачи кадров. Подходит для 95% игр Switch."),
            QStringLiteral("Ideal balance of visual fidelity, stable 60 FPS, and smooth frame delivery. Fits 95% of Nintendo Switch titles."));
        p.res_pill = QStringLiteral("1x (720p/1080p)");
        p.gpu_pill = StormLang(QStringLiteral("Точность GPU: Обычная"), QStringLiteral("GPU: Normal"));
        p.shaders_pill = StormLang(QStringLiteral("Шейдеры: Асинхронно"), QStringLiteral("Shaders: Async"));
        p.aa_pill = QStringLiteral("Сглаживание: FXAA");
        p.cpu_pill = QStringLiteral("CPU: Dynarmic (JIT)");
        p.vsync_pill = QStringLiteral("VSync: FIFO (Вкл)");
        m_builtin_presets.push_back(p);
    }

    // Preset 2: Ультра 2K/4K Качество
    {
        StormSettingsProfile p;
        p.id = QStringLiteral("preset_ultra");
        p.is_preset = true;
        p.name = StormLang(
            QStringLiteral("Ультра 2K/4K Качество"),
            QStringLiteral("Ultra 2K/4K Quality"));
        p.description = StormLang(
            QStringLiteral("Максимальная чёткость текстур и повышенное разрешение для мощных дискретных видеокарт и мониторов 1440p/4K."),
            QStringLiteral("Maximum texture sharpness, upscale resolution, and advanced filtering for high-end GPUs and 1440p/4K displays."));
        p.res_pill = QStringLiteral("2x (1440p/2160p)");
        p.gpu_pill = StormLang(QStringLiteral("Точность GPU: Высокая"), QStringLiteral("GPU: High"));
        p.shaders_pill = StormLang(QStringLiteral("Шейдеры: Асинхронно"), QStringLiteral("Shaders: Async"));
        p.aa_pill = QStringLiteral("Сглаживание: SMAA");
        p.cpu_pill = QStringLiteral("CPU: Dynarmic (JIT)");
        p.vsync_pill = QStringLiteral("Анизотропия: 16x");
        m_builtin_presets.push_back(p);
    }

    // Preset 3: Тихий / Эко-режим
    {
        StormSettingsProfile p;
        p.id = QStringLiteral("preset_eco");
        p.is_preset = true;
        p.name = StormLang(
            QStringLiteral("Тихий / Эко-режим"),
            QStringLiteral("Quiet / Eco Mode"));
        p.description = StormLang(
            QStringLiteral("Пониженное энергопотребление, контроль нагрева и тишина вентиляторов. Идеально для ноутбуков и портативных ПК."),
            QStringLiteral("Reduced power draw, controlled thermals, and silent fans. Ideal for laptops on battery and handheld PC devices."));
        p.res_pill = QStringLiteral("1x (720p/1080p)");
        p.gpu_pill = StormLang(QStringLiteral("Точность GPU: Низкая"), QStringLiteral("GPU: Low"));
        p.shaders_pill = StormLang(QStringLiteral("Шейдеры: Асинхронно"), QStringLiteral("Shaders: Async"));
        p.aa_pill = StormLang(QStringLiteral("Сжатие ASTC: BC1"), QStringLiteral("ASTC: BC1"));
        p.cpu_pill = StormLang(QStringLiteral("Эко-режим: Включён"), QStringLiteral("Eco Thermal: On"));
        p.vsync_pill = QStringLiteral("VSync: FIFO");
        m_builtin_presets.push_back(p);
    }
}

void SettingsProfilesDialog::SetupUI() {
    auto* root_lay = new QVBoxLayout(this);
    root_lay->setContentsMargins(18, 18, 18, 18);
    root_lay->setSpacing(14);

    const auto pal = StormTheme::GetThemePalette();
    const QColor acc(pal.accent);

    // -------------------------------------------------------------------------
    // 1. Header Bar: Icon + Title + Count Subtitle + Actions
    // -------------------------------------------------------------------------
    auto* header_card = new QWidget(this);
    header_card->setObjectName(QStringLiteral("ProfilesHeaderCard"));
    auto* header_lay = new QHBoxLayout(header_card);
    header_lay->setContentsMargins(16, 12, 16, 12);
    header_lay->setSpacing(14);

    auto* header_icon_lbl = new QLabel(header_card);
    header_icon_lbl->setPixmap(DrawProfileVectorIcon(ProfileVectorIcon::Sliders, acc, 24).pixmap(48, 48));
    header_icon_lbl->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    header_lay->addWidget(header_icon_lbl);

    auto* header_titles_lay = new QVBoxLayout();
    header_titles_lay->setContentsMargins(0, 0, 0, 0);
    header_titles_lay->setSpacing(3);

    auto* title_lbl = new QLabel(StormLang(
        QStringLiteral("Профили настроек"),
        QStringLiteral("Settings Profiles")), header_card);
    QFont tf = title_lbl->font();
    tf.setBold(true);
    tf.setPointSize(14);
    title_lbl->setFont(tf);
    title_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: 800; border: none; background: transparent;").arg(pal.textPrimary));
    header_titles_lay->addWidget(title_lbl);

    m_subtitle_label = new QLabel(header_card);
    m_subtitle_label->setStyleSheet(QStringLiteral("color: %1; font-size: 11.5px; border: none; background: transparent;").arg(pal.textSecondary));
    header_titles_lay->addWidget(m_subtitle_label);

    header_lay->addLayout(header_titles_lay, 1);

    // Top action buttons: "Сохранить текущие", "Импорт", "Закрыть"
    auto* save_btn = new QPushButton(StormLang(
        QStringLiteral("Сохранить текущие"),
        QStringLiteral("Save Current")), header_card);
    save_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Save, QColor(pal.textPrimary), 14));
    save_btn->setCursor(Qt::PointingHandCursor);
    connect(save_btn, &QPushButton::clicked, this, &SettingsProfilesDialog::SaveCurrentAsProfile);
    header_lay->addWidget(save_btn);

    auto* import_btn = new QPushButton(StormLang(
        QStringLiteral("Импорт"),
        QStringLiteral("Import")), header_card);
    import_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Import, QColor(pal.textPrimary), 14));
    import_btn->setCursor(Qt::PointingHandCursor);
    connect(import_btn, &QPushButton::clicked, this, &SettingsProfilesDialog::ImportProfile);
    header_lay->addWidget(import_btn);

    auto* close_btn = new QPushButton(StormLang(
        QStringLiteral("Закрыть"),
        QStringLiteral("Close")), header_card);
    close_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Close, QColor(pal.textSecondary), 14));
    close_btn->setCursor(Qt::PointingHandCursor);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
    header_lay->addWidget(close_btn);

    root_lay->addWidget(header_card);

    // -------------------------------------------------------------------------
    // 2. Toolbar: Search Bar + Filter Tabs
    // -------------------------------------------------------------------------
    auto* tool_bar = new QWidget(this);
    auto* tool_lay = new QHBoxLayout(tool_bar);
    tool_lay->setContentsMargins(0, 0, 0, 0);
    tool_lay->setSpacing(10);

    m_search_edit = new QLineEdit(tool_bar);
    m_search_edit->setPlaceholderText(StormLang(
        QStringLiteral("Поиск профилей и параметров..."),
        QStringLiteral("Search profiles and settings...")));
    m_search_edit->setClearButtonEnabled(true);
    connect(m_search_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_search_query = text.trimmed();
        RebuildCards();
    });
    tool_lay->addWidget(m_search_edit, 1);

    // Filter tabs
    auto* tab_box = new QWidget(tool_bar);
    auto* tab_lay = new QHBoxLayout(tab_box);
    tab_lay->setContentsMargins(0, 0, 0, 0);
    tab_lay->setSpacing(6);

    m_tab_all = new QPushButton(StormLang(QStringLiteral("Все"), QStringLiteral("All")), tab_box);
    m_tab_builtin = new QPushButton(StormLang(QStringLiteral("Базовые пресеты STORM"), QStringLiteral("STORM Presets")), tab_box);
    m_tab_custom = new QPushButton(StormLang(QStringLiteral("Пользовательские"), QStringLiteral("Custom")), tab_box);

    const auto setup_tab = [&](QPushButton* btn, int idx) {
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, idx]() {
            m_filter = idx;
            m_tab_all->setChecked(idx == 0);
            m_tab_builtin->setChecked(idx == 1);
            m_tab_custom->setChecked(idx == 2);
            RebuildCards();
        });
        tab_lay->addWidget(btn);
    };

    setup_tab(m_tab_all, 0);
    setup_tab(m_tab_builtin, 1);
    setup_tab(m_tab_custom, 2);
    m_tab_all->setChecked(true);

    tool_lay->addWidget(tab_box);
    root_lay->addWidget(tool_bar);

    // -------------------------------------------------------------------------
    // 3. Scrollable Cards Container
    // -------------------------------------------------------------------------
    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    StormUI::StructureEngine::EnforceScrollMargins(scroll_area, 14);

    m_cards_container = new QWidget(scroll_area);
    m_cards_layout = new QVBoxLayout(m_cards_container);
    m_cards_layout->setContentsMargins(4, 4, 4, 4);
    m_cards_layout->setSpacing(14);

    scroll_area->setWidget(m_cards_container);
    root_lay->addWidget(scroll_area, 1);
}

void SettingsProfilesDialog::ApplyTheme() {
    setProperty("_stormDialogThemed", true);
    const auto pal = StormTheme::GetThemePalette();
    setStyleSheet(StormDesign::BuildDialogStylesheet(pal) + QStringLiteral(
        "QWidget#ProfilesHeaderCard { background-color: %1; border: 1px solid %2; border-radius: 10px; }"
        "QScrollArea { background: transparent; border: none; }"
        "QWidget#ScrollContent { background: transparent; }"
    ).arg(pal.card, pal.border));
}

void SettingsProfilesDialog::LoadCustomProfiles() {
    m_custom_profiles.clear();

    const auto config_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir);
    const auto profiles_dir = config_dir / "profiles";
    std::error_code ec;
    std::filesystem::create_directories(profiles_dir, ec);

    QDir dir(QString::fromStdString(Common::FS::PathToUTF8String(profiles_dir)));
    const QStringList files = dir.entryList({QStringLiteral("*.stormprofile"), QStringLiteral("*.json")}, QDir::Files, QDir::Time);

    for (const QString& fn : files) {
        QFile file(dir.absoluteFilePath(fn));
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QByteArray json_bytes = file.readAll();
        file.close();

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(json_bytes, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        const QJsonObject root = doc.object();
        StormSettingsProfile p;
        p.is_preset = false;
        p.file_path = dir.absoluteFilePath(fn);
        p.id = root.value(QStringLiteral("id")).toString(QFileInfo(fn).baseName());
        p.name = root.value(QStringLiteral("name")).toString(p.id);
        p.description = root.value(QStringLiteral("description")).toString();
        p.timestamp = static_cast<qint64>(root.value(QStringLiteral("timestamp")).toDouble(file.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch()));

        // Extract settings pills
        const QJsonObject booleans = root.value(QStringLiteral("booleans")).toObject();
        const QJsonObject integers = root.value(QStringLiteral("integers")).toObject();

        const int res_val = integers.value(QStringLiteral("resolution_setup")).toInt(2);
        switch (res_val) {
        case 0: p.res_pill = QStringLiteral("0.5x"); break;
        case 1: p.res_pill = QStringLiteral("0.75x"); break;
        case 2: p.res_pill = QStringLiteral("1x (720p/1080p)"); break;
        case 3: p.res_pill = QStringLiteral("1.5x"); break;
        case 4: p.res_pill = QStringLiteral("2x (1440p/2160p)"); break;
        case 5: p.res_pill = QStringLiteral("3x"); break;
        case 6: p.res_pill = QStringLiteral("4x"); break;
        default: p.res_pill = QStringLiteral("1x"); break;
        }

        const int gpu_acc = integers.value(QStringLiteral("gpu_accuracy")).toInt(0);
        p.gpu_pill = (gpu_acc == 0) ? StormLang(QStringLiteral("GPU: Обычная"), QStringLiteral("GPU: Normal"))
                   : (gpu_acc == 1) ? StormLang(QStringLiteral("GPU: Высокая"), QStringLiteral("GPU: High"))
                   : (gpu_acc == 2) ? StormLang(QStringLiteral("GPU: Экстремальная"), QStringLiteral("GPU: Extreme"))
                   : StormLang(QStringLiteral("GPU: Низкая"), QStringLiteral("GPU: Low"));

        const bool async_sh = booleans.value(QStringLiteral("use_asynchronous_shaders")).toBool(true);
        p.shaders_pill = async_sh ? StormLang(QStringLiteral("Шейдеры: Асинхронно"), QStringLiteral("Shaders: Async"))
                                  : StormLang(QStringLiteral("Шейдеры: Синхронно"), QStringLiteral("Shaders: Sync"));

        const int aa_val = integers.value(QStringLiteral("anti_aliasing")).toInt(0);
        p.aa_pill = (aa_val == 1) ? QStringLiteral("FXAA")
                  : (aa_val == 2) ? QStringLiteral("SMAA")
                  : StormLang(QStringLiteral("Сглаживание: Выкл"), QStringLiteral("AA: Off"));

        const int cpu_back = integers.value(QStringLiteral("cpu_backend")).toInt(0);
        p.cpu_pill = (cpu_back == 1) ? QStringLiteral("CPU: NCE") : QStringLiteral("CPU: Dynarmic");

        const int vsync_val = integers.value(QStringLiteral("vsync_mode")).toInt(2);
        p.vsync_pill = (vsync_val == 0) ? QStringLiteral("VSync: Выкл")
                     : (vsync_val == 1) ? QStringLiteral("VSync: Mailbox")
                     : QStringLiteral("VSync: FIFO");

        m_custom_profiles.push_back(p);
    }
}

void SettingsProfilesDialog::RebuildCards() {
    // Clear layout
    while (QLayoutItem* item = m_cards_layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    const auto pal = StormTheme::GetThemePalette();
    const QColor acc(pal.accent);

    const int total_presets = static_cast<int>(m_builtin_presets.size());
    const int total_custom = static_cast<int>(m_custom_profiles.size());
    m_subtitle_label->setText(StormLang(
        QStringLiteral("Всего: 4 базовых пресета STORM • %1 пользовательских профилей").arg(total_custom),
        QStringLiteral("Total: 4 STORM presets • %1 custom profile(s)").arg(total_custom)));

    const auto matches_search = [this](const StormSettingsProfile& p) {
        if (m_search_query.isEmpty()) return true;
        const QString q = m_search_query.toLower();
        return p.name.toLower().contains(q) ||
               p.description.toLower().contains(q) ||
               p.res_pill.toLower().contains(q) ||
               p.gpu_pill.toLower().contains(q) ||
               p.shaders_pill.toLower().contains(q) ||
               p.cpu_pill.toLower().contains(q);
    };

    const auto make_pill = [&](const QString& text) {
        auto* pill = new QLabel(text);
        pill->setStyleSheet(QStringLiteral(
            "background-color: %1;"
            "color: %2;"
            "border: none;"
            "border-radius: 5px;"
            "padding: 3px 8px;"
            "font-size: 11px;"
            "font-weight: 600;"
        ).arg(pal.surface, pal.textPrimary));
        return pill;
    };

    // -------------------------------------------------------------------------
    // Section 1: БАЗОВЫЕ ПРЕСЕТЫ STORM
    // -------------------------------------------------------------------------
    if (m_filter == 0 || m_filter == 1) {
        auto* section_hdr = new QLabel(StormLang(
            QStringLiteral("БАЗОВЫЕ ПРЕСЕТЫ STORM"),
            QStringLiteral("STORM BUILT-IN PRESETS")), m_cards_container);
        section_hdr->setStyleSheet(QStringLiteral(
            "color: %1; font-weight: 800; font-size: 11px; letter-spacing: 0.8px; margin-top: 4px; border: none; background: transparent;"
        ).arg(pal.accent));
        m_cards_layout->addWidget(section_hdr);

        int displayed_presets = 0;
        for (int i = 0; i < total_presets; ++i) {
            const auto& p = m_builtin_presets[i];
            if (!matches_search(p)) continue;
            displayed_presets++;

            auto* card = new QWidget(m_cards_container);
            card->setStyleSheet(QStringLiteral(
                "QWidget {"
                "  background-color: %1;"
                "  border: none;"
                "  border-radius: 10px;"
                "}"
            ).arg(pal.card));

            auto* card_lay = new QHBoxLayout(card);
            card_lay->setContentsMargins(16, 14, 16, 14);
            card_lay->setSpacing(14);

            auto* icon_lbl = new QLabel(card);
            icon_lbl->setPixmap(DrawProfileVectorIcon(ProfileVectorIcon::PresetStar, acc, 18).pixmap(36, 36));
            icon_lbl->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
            card_lay->addWidget(icon_lbl);

            auto* info_lay = new QVBoxLayout();
            info_lay->setContentsMargins(0, 0, 0, 0);
            info_lay->setSpacing(5);

            auto* name_lbl = new QLabel(p.name, card);
            QFont nf = name_lbl->font();
            nf.setBold(true);
            nf.setPointSize(11);
            name_lbl->setFont(nf);
            name_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
            info_lay->addWidget(name_lbl);

            auto* desc_lbl = new QLabel(p.description, card);
            desc_lbl->setWordWrap(true);
            desc_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; border: none; background: transparent;").arg(pal.textSecondary));
            info_lay->addWidget(desc_lbl);

            auto* pills_lay = new QHBoxLayout();
            pills_lay->setContentsMargins(0, 2, 0, 0);
            pills_lay->setSpacing(6);
            if (!p.res_pill.isEmpty()) pills_lay->addWidget(make_pill(p.res_pill));
            if (!p.gpu_pill.isEmpty()) pills_lay->addWidget(make_pill(p.gpu_pill));
            if (!p.shaders_pill.isEmpty()) pills_lay->addWidget(make_pill(p.shaders_pill));
            if (!p.aa_pill.isEmpty()) pills_lay->addWidget(make_pill(p.aa_pill));
            if (!p.cpu_pill.isEmpty()) pills_lay->addWidget(make_pill(p.cpu_pill));
            if (!p.vsync_pill.isEmpty()) pills_lay->addWidget(make_pill(p.vsync_pill));
            pills_lay->addStretch();
            info_lay->addLayout(pills_lay);

            card_lay->addLayout(info_lay, 1);

            auto* apply_btn = new QPushButton(StormLang(QStringLiteral("Применить"), QStringLiteral("Apply")), card);
            apply_btn->setCursor(Qt::PointingHandCursor);
            apply_btn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                "  color: #FFFFFF;"
                "  font-weight: 700;"
                "  font-size: 12px;"
                "  border: none;"
                "  border-radius: 8px;"
                "  padding: 8px 18px;"
                "  min-width: 80px;"
                "}"
                "QPushButton:hover {"
                "  background: %1;"
                "}"
                "QPushButton:pressed {"
                "  background: %2;"
                "}"
            ).arg(pal.accent, pal.accentHover));
            connect(apply_btn, &QPushButton::clicked, this, [this, i]() {
                ApplyPreset(i);
            });
            card_lay->addWidget(apply_btn);

            m_cards_layout->addWidget(card);
        }

        if (displayed_presets == 0 && !m_search_query.isEmpty()) {
            auto* no_match = new QLabel(StormLang(
                QStringLiteral("Нет пресетов, соответствующих запросу поиска."),
                QStringLiteral("No presets matching the search query.")), m_cards_container);
            no_match->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; padding: 6px; border: none; background: transparent;").arg(pal.textSecondary));
            m_cards_layout->addWidget(no_match);
        }
    }

    // -------------------------------------------------------------------------
    // Section 2: ПОЛЬЗОВАТЕЛЬСКИЕ ПРОФИЛИ
    // -------------------------------------------------------------------------
    if (m_filter == 0 || m_filter == 2) {
        auto* section_hdr = new QLabel(StormLang(
            QStringLiteral("ПОЛЬЗОВАТЕЛЬСКИЕ ПРОФИЛИ"),
            QStringLiteral("CUSTOM PROFILES")), m_cards_container);
        section_hdr->setStyleSheet(QStringLiteral(
            "color: %1; font-weight: 800; font-size: 11px; letter-spacing: 0.8px; margin-top: 10px; border: none; background: transparent;"
        ).arg(pal.accent));
        m_cards_layout->addWidget(section_hdr);

        if (m_custom_profiles.empty()) {
            auto* empty_card = new QWidget(m_cards_container);
            empty_card->setStyleSheet(QStringLiteral(
                "background-color: %1; border: none; border-radius: 10px; padding: 24px;"
            ).arg(pal.card));
            auto* empty_lay = new QVBoxLayout(empty_card);
            empty_lay->setAlignment(Qt::AlignCenter);
            empty_lay->setSpacing(8);

            auto* empty_icon = new QLabel(empty_card);
            empty_icon->setPixmap(DrawProfileVectorIcon(ProfileVectorIcon::CustomUser, QColor(pal.textSecondary), 20).pixmap(40, 40));
            empty_icon->setAlignment(Qt::AlignCenter);
            empty_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
            empty_lay->addWidget(empty_icon);

            auto* empty_lbl = new QLabel(StormLang(
                QStringLiteral("Нет сохранённых пользовательских профилей"),
                QStringLiteral("No custom profiles saved yet")), empty_card);
            empty_lbl->setAlignment(Qt::AlignCenter);
            empty_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; font-size: 13px; border: none; background: transparent;").arg(pal.textPrimary));
            empty_lay->addWidget(empty_lbl);

            auto* empty_sub = new QLabel(StormLang(
                QStringLiteral("Нажмите кнопку «Сохранить текущие» вверху, чтобы сохранить текущие настройки в файл, или «Импорт» для загрузки профиля .stormprofile."),
                QStringLiteral("Click 'Save Current' above to save your active settings to a file, or 'Import' to load a .stormprofile file.")), empty_card);
            empty_sub->setAlignment(Qt::AlignCenter);
            empty_sub->setWordWrap(true);
            empty_sub->setStyleSheet(QStringLiteral("color: %1; font-size: 11.5px; border: none; background: transparent;").arg(pal.textSecondary));
            empty_lay->addWidget(empty_sub);

            m_cards_layout->addWidget(empty_card);
        } else {
            int displayed_custom = 0;
            for (const auto& cp : m_custom_profiles) {
                if (!matches_search(cp)) continue;
                displayed_custom++;

                auto* card = new QWidget(m_cards_container);
                card->setStyleSheet(QStringLiteral(
                    "QWidget {"
                    "  background-color: %1;"
                    "  border: none;"
                    "  border-radius: 10px;"
                    "}"
                ).arg(pal.card));

                auto* card_lay = new QHBoxLayout(card);
                card_lay->setContentsMargins(16, 14, 16, 14);
                card_lay->setSpacing(14);

                auto* icon_lbl = new QLabel(card);
                icon_lbl->setPixmap(DrawProfileVectorIcon(ProfileVectorIcon::CustomUser, acc, 18).pixmap(36, 36));
                icon_lbl->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
                card_lay->addWidget(icon_lbl);

                auto* info_lay = new QVBoxLayout();
                info_lay->setContentsMargins(0, 0, 0, 0);
                info_lay->setSpacing(5);

                auto* title_row = new QHBoxLayout();
                title_row->setContentsMargins(0, 0, 0, 0);
                title_row->setSpacing(8);

                auto* name_lbl = new QLabel(cp.name, card);
                QFont nf = name_lbl->font();
                nf.setBold(true);
                nf.setPointSize(11);
                name_lbl->setFont(nf);
                name_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
                title_row->addWidget(name_lbl);

                if (cp.timestamp > 0) {
                    const QString date_str = QDateTime::fromMSecsSinceEpoch(cp.timestamp).toString(QStringLiteral("dd.MM.yyyy, hh:mm"));
                    auto* date_lbl = new QLabel(date_str, card);
                    date_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 10.5px; border: none; background: transparent;").arg(pal.textSecondary));
                    title_row->addWidget(date_lbl);
                }
                title_row->addStretch();
                info_lay->addLayout(title_row);

                if (!cp.description.isEmpty()) {
                    auto* desc_lbl = new QLabel(cp.description, card);
                    desc_lbl->setWordWrap(true);
                    desc_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; border: none; background: transparent;").arg(pal.textSecondary));
                    info_lay->addWidget(desc_lbl);
                }

                auto* pills_lay = new QHBoxLayout();
                pills_lay->setContentsMargins(0, 2, 0, 0);
                pills_lay->setSpacing(6);
                if (!cp.res_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.res_pill));
                if (!cp.gpu_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.gpu_pill));
                if (!cp.shaders_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.shaders_pill));
                if (!cp.aa_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.aa_pill));
                if (!cp.cpu_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.cpu_pill));
                if (!cp.vsync_pill.isEmpty()) pills_lay->addWidget(make_pill(cp.vsync_pill));
                pills_lay->addStretch();
                info_lay->addLayout(pills_lay);

                card_lay->addLayout(info_lay, 1);

                // Action buttons: Apply, Export, Rename, Delete
                auto* actions_lay = new QHBoxLayout();
                actions_lay->setContentsMargins(0, 0, 0, 0);
                actions_lay->setSpacing(6);

                auto* apply_btn = new QPushButton(StormLang(QStringLiteral("Применить"), QStringLiteral("Apply")), card);
                apply_btn->setCursor(Qt::PointingHandCursor);
                apply_btn->setStyleSheet(QStringLiteral(
                    "QPushButton {"
                    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                    "  color: #FFFFFF;"
                    "  font-weight: 700;"
                    "  font-size: 12px;"
                    "  border: none;"
                    "  border-radius: 7px;"
                    "  padding: 6px 14px;"
                    "}"
                    "QPushButton:hover { background: %1; }"
                ).arg(pal.accent, pal.accentHover));
                connect(apply_btn, &QPushButton::clicked, this, [this, cp]() {
                    ApplyCustomProfile(cp);
                });
                actions_lay->addWidget(apply_btn);

                auto* export_btn = new QPushButton(card);
                export_btn->setToolTip(StormLang(QStringLiteral("Экспорт профиля"), QStringLiteral("Export profile")));
                export_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Export, QColor(pal.textSecondary), 14));
                export_btn->setCursor(Qt::PointingHandCursor);
                connect(export_btn, &QPushButton::clicked, this, [this, cp]() {
                    ExportProfile(cp);
                });
                actions_lay->addWidget(export_btn);

                auto* rename_btn = new QPushButton(card);
                rename_btn->setToolTip(StormLang(QStringLiteral("Переименовать профиль"), QStringLiteral("Rename profile")));
                rename_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Edit, QColor(pal.textSecondary), 14));
                rename_btn->setCursor(Qt::PointingHandCursor);
                connect(rename_btn, &QPushButton::clicked, this, [this, cp]() {
                    RenameProfile(cp);
                });
                actions_lay->addWidget(rename_btn);

                auto* delete_btn = new QPushButton(card);
                delete_btn->setToolTip(StormLang(QStringLiteral("Удалить профиль"), QStringLiteral("Delete profile")));
                delete_btn->setIcon(DrawProfileVectorIcon(ProfileVectorIcon::Delete, QColor(239, 68, 68), 14));
                delete_btn->setCursor(Qt::PointingHandCursor);
                connect(delete_btn, &QPushButton::clicked, this, [this, cp]() {
                    DeleteProfile(cp);
                });
                actions_lay->addWidget(delete_btn);

                card_lay->addLayout(actions_lay);

                m_cards_layout->addWidget(card);
            }

            if (displayed_custom == 0 && !m_search_query.isEmpty()) {
                auto* no_match = new QLabel(StormLang(
                    QStringLiteral("Нет пользовательских профилей, соответствующих запросу поиска."),
                    QStringLiteral("No custom profiles matching the search query.")), m_cards_container);
                no_match->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; padding: 6px; border: none; background: transparent;").arg(pal.textSecondary));
                m_cards_layout->addWidget(no_match);
            }
        }
    }

    m_cards_layout->addStretch();
}

void SettingsProfilesDialog::ApplyPreset(int index) {
    const bool is_global = Settings::IsConfiguringGlobal();

    const auto set_val = [is_global](auto& setting, auto val) {
        setting.SetGlobal(is_global);
        setting.SetValue(val);
    };

    if (index == 0) { // Максимум FPS / Киберспорт
        set_val(Settings::values.resolution_setup, Settings::ResolutionSetup::Res1X);
        set_val(Settings::values.gpu_accuracy, Settings::GpuAccuracy::Low);
        set_val(Settings::values.accelerate_astc, Settings::AstcDecodeMode::CpuAsynchronous);
        set_val(Settings::values.astc_recompression, Settings::AstcRecompression::Bc1);
        set_val(Settings::values.use_asynchronous_shaders, true);
        set_val(Settings::values.use_disk_shader_cache, true);
        set_val(Settings::values.use_asynchronous_gpu_emulation, true);
        set_val(Settings::values.async_presentation, true);
        set_val(Settings::values.early_release_fences, true);
        set_val(Settings::values.sync_memory_operations, false);
        set_val(Settings::values.use_reactive_flushing, false);
        set_val(Settings::values.eco_thermal_mode, false);
        set_val(Settings::values.anti_aliasing, Settings::AntiAliasing::None);
        set_val(Settings::values.scaling_filter, Settings::ScalingFilter::Bilinear);
        set_val(Settings::values.vsync_mode, Settings::VSyncMode::Mailbox);
        set_val(Settings::values.cpu_accuracy, Settings::CpuAccuracy::Auto);
        set_val(Settings::values.cpu_backend, Settings::CpuBackend::Dynarmic);
        set_val(Settings::values.use_speed_limit, false);
    } else if (index == 1) { // Сбалансированный
        set_val(Settings::values.resolution_setup, Settings::ResolutionSetup::Res1X);
        set_val(Settings::values.gpu_accuracy, Settings::GpuAccuracy::Normal);
        set_val(Settings::values.accelerate_astc, Settings::AstcDecodeMode::Gpu);
        set_val(Settings::values.astc_recompression, Settings::AstcRecompression::Uncompressed);
        set_val(Settings::values.use_asynchronous_shaders, true);
        set_val(Settings::values.use_disk_shader_cache, true);
        set_val(Settings::values.use_asynchronous_gpu_emulation, true);
        set_val(Settings::values.async_presentation, true);
        set_val(Settings::values.early_release_fences, true);
        set_val(Settings::values.sync_memory_operations, true);
        set_val(Settings::values.use_reactive_flushing, true);
        set_val(Settings::values.eco_thermal_mode, false);
        set_val(Settings::values.anti_aliasing, Settings::AntiAliasing::Fxaa);
        set_val(Settings::values.scaling_filter, Settings::ScalingFilter::Fsr);
        set_val(Settings::values.vsync_mode, Settings::VSyncMode::Fifo);
        set_val(Settings::values.cpu_accuracy, Settings::CpuAccuracy::Auto);
        set_val(Settings::values.cpu_backend, Settings::CpuBackend::Dynarmic);
        set_val(Settings::values.use_speed_limit, true);
        set_val(Settings::values.speed_limit, static_cast<u16>(100));
    } else if (index == 2) { // Ультра 2K/4K Качество
        set_val(Settings::values.resolution_setup, Settings::ResolutionSetup::Res2X);
        set_val(Settings::values.gpu_accuracy, Settings::GpuAccuracy::High);
        set_val(Settings::values.accelerate_astc, Settings::AstcDecodeMode::Gpu);
        set_val(Settings::values.astc_recompression, Settings::AstcRecompression::Uncompressed);
        set_val(Settings::values.use_asynchronous_shaders, true);
        set_val(Settings::values.use_disk_shader_cache, true);
        set_val(Settings::values.use_asynchronous_gpu_emulation, true);
        set_val(Settings::values.async_presentation, true);
        set_val(Settings::values.early_release_fences, true);
        set_val(Settings::values.sync_memory_operations, true);
        set_val(Settings::values.use_reactive_flushing, true);
        set_val(Settings::values.eco_thermal_mode, false);
        set_val(Settings::values.anti_aliasing, Settings::AntiAliasing::Smaa);
        set_val(Settings::values.scaling_filter, Settings::ScalingFilter::Fsr);
        set_val(Settings::values.max_anisotropy, Settings::AnisotropyMode::X16);
        set_val(Settings::values.vsync_mode, Settings::VSyncMode::Fifo);
        set_val(Settings::values.cpu_accuracy, Settings::CpuAccuracy::Accurate);
        set_val(Settings::values.cpu_backend, Settings::CpuBackend::Dynarmic);
        set_val(Settings::values.use_speed_limit, true);
        set_val(Settings::values.speed_limit, static_cast<u16>(100));
    } else if (index == 3) { // Тихий / Эко-режим
        set_val(Settings::values.resolution_setup, Settings::ResolutionSetup::Res1X);
        set_val(Settings::values.gpu_accuracy, Settings::GpuAccuracy::Low);
        set_val(Settings::values.accelerate_astc, Settings::AstcDecodeMode::CpuAsynchronous);
        set_val(Settings::values.astc_recompression, Settings::AstcRecompression::Bc1);
        set_val(Settings::values.use_asynchronous_shaders, true);
        set_val(Settings::values.use_disk_shader_cache, true);
        set_val(Settings::values.use_asynchronous_gpu_emulation, true);
        set_val(Settings::values.async_presentation, true);
        set_val(Settings::values.early_release_fences, true);
        set_val(Settings::values.sync_memory_operations, false);
        set_val(Settings::values.use_reactive_flushing, false);
        set_val(Settings::values.eco_thermal_mode, true);
        set_val(Settings::values.anti_aliasing, Settings::AntiAliasing::None);
        set_val(Settings::values.scaling_filter, Settings::ScalingFilter::Bilinear);
        set_val(Settings::values.vsync_mode, Settings::VSyncMode::Fifo);
        set_val(Settings::values.cpu_accuracy, Settings::CpuAccuracy::Auto);
        set_val(Settings::values.cpu_backend, Settings::CpuBackend::Dynarmic);
        set_val(Settings::values.use_speed_limit, true);
        set_val(Settings::values.speed_limit, static_cast<u16>(100));
    }

    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();
    ConfigurationShared::ReloadAllActiveWidgets();
    ConfigurationShared::NotifyGlobalSettingChanged();
    Settings::LogSettings();
    if (m_system) {
        m_system->ApplySettings();
    }

    const QString preset_name = (index >= 0 && static_cast<size_t>(index) < m_builtin_presets.size())
                                    ? m_builtin_presets[static_cast<size_t>(index)].name
                                    : QStringLiteral("Пресет");

    QMessageBox::information(this, StormLang(
        QStringLiteral("Пресет применён"),
        QStringLiteral("Preset Applied")),
        StormLang(
            QStringLiteral("Базовый пресет «%1» успешно применён к конфигурации эмулятора.").arg(preset_name),
            QStringLiteral("Built-in preset '%1' has been successfully applied.").arg(preset_name)));
}

void SettingsProfilesDialog::ApplyCustomProfile(const StormSettingsProfile& profile) {
    QFile file(profile.file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, StormLang(QStringLiteral("Ошибка"), QStringLiteral("Error")),
                             StormLang(QStringLiteral("Не удалось открыть файл профиля."), QStringLiteral("Failed to open profile file.")));
        return;
    }

    const QByteArray json_bytes = file.readAll();
    file.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(json_bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, StormLang(QStringLiteral("Ошибка"), QStringLiteral("Error")),
                             StormLang(QStringLiteral("Файл профиля повреждён или имеет неверный формат."), QStringLiteral("Corrupted or invalid profile format.")));
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonObject booleans = root.value(QStringLiteral("booleans")).toObject();
    const QJsonObject integers = root.value(QStringLiteral("integers")).toObject();
    const QJsonObject shorts = root.value(QStringLiteral("shorts")).toObject();

    const bool is_global = Settings::IsConfiguringGlobal();

    // Booleans
    if (booleans.contains(QStringLiteral("use_asynchronous_shaders"))) {
        Settings::values.use_asynchronous_shaders.SetGlobal(is_global);
        Settings::values.use_asynchronous_shaders.SetValue(booleans.value(QStringLiteral("use_asynchronous_shaders")).toBool());
    }
    if (booleans.contains(QStringLiteral("use_disk_shader_cache"))) {
        Settings::values.use_disk_shader_cache.SetGlobal(is_global);
        Settings::values.use_disk_shader_cache.SetValue(booleans.value(QStringLiteral("use_disk_shader_cache")).toBool());
    }
    if (booleans.contains(QStringLiteral("use_asynchronous_gpu_emulation"))) {
        Settings::values.use_asynchronous_gpu_emulation.SetGlobal(is_global);
        Settings::values.use_asynchronous_gpu_emulation.SetValue(booleans.value(QStringLiteral("use_asynchronous_gpu_emulation")).toBool());
    }
    if (booleans.contains(QStringLiteral("async_presentation"))) {
        Settings::values.async_presentation.SetGlobal(is_global);
        Settings::values.async_presentation.SetValue(booleans.value(QStringLiteral("async_presentation")).toBool());
    }
    if (booleans.contains(QStringLiteral("early_release_fences"))) {
        Settings::values.early_release_fences.SetGlobal(is_global);
        Settings::values.early_release_fences.SetValue(booleans.value(QStringLiteral("early_release_fences")).toBool());
    }
    if (booleans.contains(QStringLiteral("sync_memory_operations"))) {
        Settings::values.sync_memory_operations.SetGlobal(is_global);
        Settings::values.sync_memory_operations.SetValue(booleans.value(QStringLiteral("sync_memory_operations")).toBool());
    }
    if (booleans.contains(QStringLiteral("use_reactive_flushing"))) {
        Settings::values.use_reactive_flushing.SetGlobal(is_global);
        Settings::values.use_reactive_flushing.SetValue(booleans.value(QStringLiteral("use_reactive_flushing")).toBool());
    }
    if (booleans.contains(QStringLiteral("eco_thermal_mode"))) {
        Settings::values.eco_thermal_mode.SetGlobal(is_global);
        Settings::values.eco_thermal_mode.SetValue(booleans.value(QStringLiteral("eco_thermal_mode")).toBool());
    }
    if (booleans.contains(QStringLiteral("use_speed_limit"))) {
        Settings::values.use_speed_limit.SetGlobal(is_global);
        Settings::values.use_speed_limit.SetValue(booleans.value(QStringLiteral("use_speed_limit")).toBool());
    }
    if (booleans.contains(QStringLiteral("use_docked_mode"))) {
        Settings::values.use_docked_mode.SetGlobal(is_global);
        Settings::values.use_docked_mode.SetValue(booleans.value(QStringLiteral("use_docked_mode")).toBool() ? Settings::ConsoleMode::Docked : Settings::ConsoleMode::Handheld);
    }

    // Integers
    if (integers.contains(QStringLiteral("resolution_setup"))) {
        Settings::values.resolution_setup.SetGlobal(is_global);
        Settings::values.resolution_setup.SetValue(static_cast<Settings::ResolutionSetup>(integers.value(QStringLiteral("resolution_setup")).toInt()));
    }
    if (integers.contains(QStringLiteral("gpu_accuracy"))) {
        Settings::values.gpu_accuracy.SetGlobal(is_global);
        Settings::values.gpu_accuracy.SetValue(static_cast<Settings::GpuAccuracy>(integers.value(QStringLiteral("gpu_accuracy")).toInt()));
    }
    if (integers.contains(QStringLiteral("astc_recompression"))) {
        Settings::values.astc_recompression.SetGlobal(is_global);
        Settings::values.astc_recompression.SetValue(static_cast<Settings::AstcRecompression>(integers.value(QStringLiteral("astc_recompression")).toInt()));
    }
    if (integers.contains(QStringLiteral("accelerate_astc"))) {
        Settings::values.accelerate_astc.SetGlobal(is_global);
        Settings::values.accelerate_astc.SetValue(static_cast<Settings::AstcDecodeMode>(integers.value(QStringLiteral("accelerate_astc")).toInt()));
    }
    if (integers.contains(QStringLiteral("vsync_mode"))) {
        Settings::values.vsync_mode.SetGlobal(is_global);
        Settings::values.vsync_mode.SetValue(static_cast<Settings::VSyncMode>(integers.value(QStringLiteral("vsync_mode")).toInt()));
    }
    if (integers.contains(QStringLiteral("scaling_filter"))) {
        Settings::values.scaling_filter.SetGlobal(is_global);
        Settings::values.scaling_filter.SetValue(static_cast<Settings::ScalingFilter>(integers.value(QStringLiteral("scaling_filter")).toInt()));
    }
    if (integers.contains(QStringLiteral("anti_aliasing"))) {
        Settings::values.anti_aliasing.SetGlobal(is_global);
        Settings::values.anti_aliasing.SetValue(static_cast<Settings::AntiAliasing>(integers.value(QStringLiteral("anti_aliasing")).toInt()));
    }
    if (integers.contains(QStringLiteral("cpu_accuracy"))) {
        Settings::values.cpu_accuracy.SetGlobal(is_global);
        Settings::values.cpu_accuracy.SetValue(static_cast<Settings::CpuAccuracy>(integers.value(QStringLiteral("cpu_accuracy")).toInt()));
    }
    if (integers.contains(QStringLiteral("cpu_backend"))) {
        Settings::values.cpu_backend.SetGlobal(is_global);
        Settings::values.cpu_backend.SetValue(static_cast<Settings::CpuBackend>(integers.value(QStringLiteral("cpu_backend")).toInt()));
    }

    // Shorts
    if (shorts.contains(QStringLiteral("speed_limit"))) {
        Settings::values.speed_limit.SetGlobal(is_global);
        Settings::values.speed_limit.SetValue(static_cast<u16>(shorts.value(QStringLiteral("speed_limit")).toInt(100)));
    }

    Settings::UpdateGPUAccuracy();
    Settings::UpdateRescalingInfo();
    ConfigurationShared::ReloadAllActiveWidgets();
    ConfigurationShared::NotifyGlobalSettingChanged();
    Settings::LogSettings();
    if (m_system) {
        m_system->ApplySettings();
    }

    QMessageBox::information(this, StormLang(
        QStringLiteral("Профиль применён"),
        QStringLiteral("Profile Applied")),
        StormLang(
            QStringLiteral("Пользовательский профиль «%1» успешно применён.").arg(profile.name),
            QStringLiteral("Custom profile '%1' has been successfully applied.").arg(profile.name)));
}

void SettingsProfilesDialog::SaveCurrentAsProfile() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        StormLang(QStringLiteral("Сохранить профиль"), QStringLiteral("Save Profile")),
        StormLang(QStringLiteral("Введите имя нового профиля:"), QStringLiteral("Enter profile name:")),
        QLineEdit::Normal,
        StormLang(QStringLiteral("Мой профиль"), QStringLiteral("My Profile")),
        &ok);

    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    const QString clean_name = name.trimmed();
    const qint64 ts = QDateTime::currentMSecsSinceEpoch();
    const QString id = QStringLiteral("profile_%1").arg(ts);

    QJsonObject root;
    root[QStringLiteral("id")] = id;
    root[QStringLiteral("name")] = clean_name;
    root[QStringLiteral("description")] = StormLang(QStringLiteral("Пользовательский профиль настроек"), QStringLiteral("Custom settings profile"));
    root[QStringLiteral("version")] = QStringLiteral("17.1.0");
    root[QStringLiteral("timestamp")] = static_cast<double>(ts);

    QJsonObject bools;
    bools[QStringLiteral("use_asynchronous_shaders")] = Settings::values.use_asynchronous_shaders.GetValue();
    bools[QStringLiteral("use_disk_shader_cache")] = Settings::values.use_disk_shader_cache.GetValue();
    bools[QStringLiteral("use_asynchronous_gpu_emulation")] = Settings::values.use_asynchronous_gpu_emulation.GetValue();
    bools[QStringLiteral("async_presentation")] = Settings::values.async_presentation.GetValue();
    bools[QStringLiteral("early_release_fences")] = Settings::values.early_release_fences.GetValue();
    bools[QStringLiteral("sync_memory_operations")] = Settings::values.sync_memory_operations.GetValue();
    bools[QStringLiteral("use_reactive_flushing")] = Settings::values.use_reactive_flushing.GetValue();
    bools[QStringLiteral("eco_thermal_mode")] = Settings::values.eco_thermal_mode.GetValue();
    bools[QStringLiteral("use_speed_limit")] = Settings::values.use_speed_limit.GetValue();
    bools[QStringLiteral("use_docked_mode")] = (Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked);
    root[QStringLiteral("booleans")] = bools;

    QJsonObject ints;
    ints[QStringLiteral("resolution_setup")] = static_cast<int>(Settings::values.resolution_setup.GetValue());
    ints[QStringLiteral("gpu_accuracy")] = static_cast<int>(Settings::values.gpu_accuracy.GetValue());
    ints[QStringLiteral("astc_recompression")] = static_cast<int>(Settings::values.astc_recompression.GetValue());
    ints[QStringLiteral("accelerate_astc")] = static_cast<int>(Settings::values.accelerate_astc.GetValue());
    ints[QStringLiteral("vsync_mode")] = static_cast<int>(Settings::values.vsync_mode.GetValue());
    ints[QStringLiteral("scaling_filter")] = static_cast<int>(Settings::values.scaling_filter.GetValue());
    ints[QStringLiteral("anti_aliasing")] = static_cast<int>(Settings::values.anti_aliasing.GetValue());
    ints[QStringLiteral("cpu_accuracy")] = static_cast<int>(Settings::values.cpu_accuracy.GetValue());
    ints[QStringLiteral("cpu_backend")] = static_cast<int>(Settings::values.cpu_backend.GetValue());
    ints[QStringLiteral("renderer_backend")] = static_cast<int>(Settings::values.renderer_backend.GetValue());
    root[QStringLiteral("integers")] = ints;

    QJsonObject shorts;
    shorts[QStringLiteral("speed_limit")] = static_cast<int>(Settings::values.speed_limit.GetValue());
    root[QStringLiteral("shorts")] = shorts;

    const auto config_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir);
    const auto profiles_dir = config_dir / "profiles";
    std::error_code ec;
    std::filesystem::create_directories(profiles_dir, ec);

    const auto target_file = profiles_dir / fmt::format("{}.stormprofile", id.toStdString());
    QFile file(QString::fromStdString(Common::FS::PathToUTF8String(target_file)));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        LoadCustomProfiles();
        RebuildCards();

        QMessageBox::information(this, StormLang(
            QStringLiteral("Профиль сохранён"),
            QStringLiteral("Profile Saved")),
            StormLang(
                QStringLiteral("Профиль «%1» успешно сохранён в коллекцию.").arg(clean_name),
                QStringLiteral("Profile '%1' has been saved successfully.").arg(clean_name)));
    }
}

void SettingsProfilesDialog::ImportProfile() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        StormLang(QStringLiteral("Импорт профиля настроек"), QStringLiteral("Import Settings Profile")),
        QString(),
        StormLang(QStringLiteral("Профили STORM (*.stormprofile *.json)"), QStringLiteral("STORM Profiles (*.stormprofile *.json)")));

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, StormLang(QStringLiteral("Ошибка"), QStringLiteral("Error")),
                             StormLang(QStringLiteral("Не удалось прочитать выбранный файл."), QStringLiteral("Failed to read selected file.")));
        return;
    }

    const QByteArray json_bytes = file.readAll();
    file.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(json_bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, StormLang(QStringLiteral("Ошибка"), QStringLiteral("Error")),
                             StormLang(QStringLiteral("Файл не является корректным профилем STORM."), QStringLiteral("Invalid STORM profile format.")));
        return;
    }

    QJsonObject root = doc.object();
    const QString orig_name = root.value(QStringLiteral("name")).toString(QFileInfo(path).baseName());
    const qint64 ts = QDateTime::currentMSecsSinceEpoch();
    const QString id = QStringLiteral("imported_%1").arg(ts);
    root[QStringLiteral("id")] = id;

    const auto config_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir);
    const auto profiles_dir = config_dir / "profiles";
    std::error_code ec;
    std::filesystem::create_directories(profiles_dir, ec);

    const auto target_file = profiles_dir / fmt::format("{}.stormprofile", id.toStdString());
    QFile out_file(QString::fromStdString(Common::FS::PathToUTF8String(target_file)));
    if (out_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        doc.setObject(root);
        out_file.write(doc.toJson(QJsonDocument::Indented));
        out_file.close();

        LoadCustomProfiles();
        RebuildCards();

        QMessageBox::information(this, StormLang(
            QStringLiteral("Импорт завершён"),
            QStringLiteral("Import Successful")),
            StormLang(
                QStringLiteral("Профиль «%1» успешно импортирован.").arg(orig_name),
                QStringLiteral("Profile '%1' was successfully imported.").arg(orig_name)));
    }
}

void SettingsProfilesDialog::ExportProfile(const StormSettingsProfile& profile) {
    const QString dest = QFileDialog::getSaveFileName(
        this,
        StormLang(QStringLiteral("Экспорт профиля"), QStringLiteral("Export Profile")),
        profile.name + QStringLiteral(".stormprofile"),
        StormLang(QStringLiteral("Профили STORM (*.stormprofile)"), QStringLiteral("STORM Profiles (*.stormprofile)")));

    if (dest.isEmpty()) return;

    if (QFile::exists(dest)) {
        QFile::remove(dest);
    }

    if (QFile::copy(profile.file_path, dest)) {
        QMessageBox::information(this, StormLang(
            QStringLiteral("Экспорт завершён"),
            QStringLiteral("Export Completed")),
            StormLang(
                QStringLiteral("Профиль «%1» успешно экспортирован.").arg(profile.name),
                QStringLiteral("Profile '%1' was exported successfully.").arg(profile.name)));
    }
}

void SettingsProfilesDialog::RenameProfile(const StormSettingsProfile& profile) {
    bool ok = false;
    const QString new_name = QInputDialog::getText(
        this,
        StormLang(QStringLiteral("Переименование профиля"), QStringLiteral("Rename Profile")),
        StormLang(QStringLiteral("Новое имя профиля:"), QStringLiteral("New profile name:")),
        QLineEdit::Normal,
        profile.name,
        &ok);

    if (!ok || new_name.trimmed().isEmpty()) return;

    QFile file(profile.file_path);
    if (!file.open(QIODevice::ReadOnly)) return;

    const QByteArray json_bytes = file.readAll();
    file.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(json_bytes, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject root = doc.object();
        root[QStringLiteral("name")] = new_name.trimmed();
        doc.setObject(root);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            LoadCustomProfiles();
            RebuildCards();
        }
    }
}

void SettingsProfilesDialog::DeleteProfile(const StormSettingsProfile& profile) {
    const auto res = QMessageBox::question(
        this,
        StormLang(QStringLiteral("Удаление профиля"), QStringLiteral("Delete Profile")),
        StormLang(
            QStringLiteral("Вы действительно хотите удалить профиль «%1»?").arg(profile.name),
            QStringLiteral("Are you sure you want to delete profile '%1'?").arg(profile.name)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (res == QMessageBox::Yes) {
        QFile::remove(profile.file_path);
        LoadCustomProfiles();
        RebuildCards();
    }
}
