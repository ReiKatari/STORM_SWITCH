// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/game_fix_dialog.h"

#include <filesystem>
#include <fstream>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "common/cityhash.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/game_fix_database.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

GameFixDialog::GameFixDialog(QWidget* parent, u64 title_id_, const QString& game_path_,
                             bool force_show_, Core::System& system_)
    : QDialog(parent), m_title_id(title_id_), m_game_path(game_path_),
      m_force_show(force_show_), m_system(system_) {
    // Resolve Title ID if 0
    if (m_title_id == 0) {
        static const QRegularExpression tid_regex(QStringLiteral(R"(([0-9a-fA-F]{16}))"));
        const auto match = tid_regex.match(m_game_path);
        if (match.hasMatch()) {
            bool ok = false;
            const u64 parsed = match.captured(1).toULongLong(&ok, 16);
            if (ok && parsed != 0) {
                m_title_id = parsed;
            }
        }
    }

    m_profile = Core::GameFixDatabase::GetProfileByTitleOrPath(m_title_id, m_game_path.toStdString());
    if (m_profile != nullptr && m_title_id == 0) {
        m_title_id = m_profile->title_id;
    }
    if (m_profile == nullptr && m_title_id != 0) {
        m_profile = Core::GameFixDatabase::GetProfile(m_title_id);
    }

    setWindowTitle(StormLang(
        QStringLiteral("STORM SWITCH — Профиль оптимизации и авто-исправления"),
        QStringLiteral("STORM SWITCH — Game Optimization & Auto-Fix Profile"),
        QStringLiteral("STORM SWITCH — Spieloptimierungs- und Auto-Fix-Profil"),
        QStringLiteral("STORM SWITCH — Profil d'optimisation et d'auto-correction")
    ));

    resize(800, 600);
    setMinimumSize(680, 480);
    StormUI::StructureEngine::InstallEscDismiss(this);

    SetupUI();
}

GameFixDialog::~GameFixDialog() = default;

QString GameFixDialog::GetLocalizedSettingLabel(const std::string& raw_key) const {
    const std::string key_lower = raw_key;
    if (key_lower.find("gpu_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции ГПУ"), QStringLiteral("GPU Accuracy"));
    if (key_lower.find("nvdec_emulation") != std::string::npos)
        return StormLang(QStringLiteral("Декодирование видео (NVDEC)"), QStringLiteral("NVDEC Video Emulation"));
    if (key_lower.find("astc_recompression") != std::string::npos)
        return StormLang(QStringLiteral("Пересжатие текстур ASTC"), QStringLiteral("ASTC Texture Recompression"));
    if (key_lower.find("accelerate_astc") != std::string::npos)
        return StormLang(QStringLiteral("Декодирование текстур ASTC"), QStringLiteral("ASTC Texture Decoding"));
    if (key_lower.find("use_asynchronous_shaders") != std::string::npos)
        return StormLang(QStringLiteral("Асинхронные шейдеры"), QStringLiteral("Asynchronous Shaders"));
    if (key_lower.find("use_asynchronous_gpu_emulation") != std::string::npos)
        return StormLang(QStringLiteral("Асинхронная эмуляция ГПУ"), QStringLiteral("Asynchronous GPU Emulation"));
    if (key_lower.find("barrier_feedback_loops") != std::string::npos)
        return StormLang(QStringLiteral("Обратная связь барьеров"), QStringLiteral("Barrier Feedback Loops"));
    if (key_lower.find("vulkan_device") != std::string::npos)
        return StormLang(QStringLiteral("Графическое устройство Vulkan"), QStringLiteral("Vulkan Device"));
    if (key_lower.find("backend") != std::string::npos)
        return StormLang(QStringLiteral("Графический бэкенд"), QStringLiteral("Graphics Backend"));
    if (key_lower.find("use_speed_limit") != std::string::npos)
        return StormLang(QStringLiteral("Ограничение скорости кадров"), QStringLiteral("Speed Limit"));
    if (key_lower.find("speed_limit") != std::string::npos)
        return StormLang(QStringLiteral("Процент скорости эмуляции"), QStringLiteral("Speed Limit Percent"));
    if (key_lower.find("cpu_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции ЦП"), QStringLiteral("CPU Accuracy"));
    if (key_lower.find("aspect_ratio") != std::string::npos)
        return StormLang(QStringLiteral("Соотношение сторон экрана"), QStringLiteral("Aspect Ratio"));
    if (key_lower.find("resolution_setup") != std::string::npos)
        return StormLang(QStringLiteral("Масштаб разрешения"), QStringLiteral("Resolution Scale"));
    if (key_lower.find("scaling_filter") != std::string::npos)
        return StormLang(QStringLiteral("Фильтр масштабирования (FSR)"), QStringLiteral("Scaling Filter (FSR)"));
    if (key_lower.find("anti_aliasing") != std::string::npos)
        return StormLang(QStringLiteral("Сглаживание (SMAA / FXAA)"), QStringLiteral("Anti-Aliasing"));
    if (key_lower.find("use_vsync") != std::string::npos)
        return StormLang(QStringLiteral("Вертикальная синхронизация (VSync)"), QStringLiteral("Vertical Sync (VSync)"));
    if (key_lower.find("use_reactive_flushing") != std::string::npos)
        return StormLang(QStringLiteral("Реактивный сброс очередей GPU"), QStringLiteral("Reactive Flushing"));
    if (key_lower.find("use_fast_gpu_time") != std::string::npos)
        return StormLang(QStringLiteral("Ускоренное время GPU"), QStringLiteral("Fast GPU Time"));

    // Fallback: strip section and format nicely
    std::string clean = raw_key;
    const auto slash_pos = clean.find_last_of("\\/");
    if (slash_pos != std::string::npos) {
        clean = clean.substr(slash_pos + 1);
    }
    std::replace(clean.begin(), clean.end(), '_', ' ');
    if (!clean.empty()) {
        clean[0] = static_cast<char>(std::toupper(clean[0]));
    }
    return QString::fromStdString(clean);
}

QString GameFixDialog::FormatSettingValue(const std::string& key, const std::string& val) const {
    if (val == "0") return StormLang(QStringLiteral("Выключено"), QStringLiteral("Disabled"));
    if (val == "1") return StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"));
    if (val == "true") return StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"));
    if (val == "false") return StormLang(QStringLiteral("Выключено"), QStringLiteral("Disabled"));
    if (val == "High") return StormLang(QStringLiteral("Высокая"), QStringLiteral("High"));
    if (val == "Normal") return StormLang(QStringLiteral("Нормальная"), QStringLiteral("Normal"));
    if (val == "Extreme") return StormLang(QStringLiteral("Экстремальная"), QStringLiteral("Extreme"));
    if (val == "Auto") return StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"));
    return QString::fromStdString(val);
}

void GameFixDialog::SetupUI() {
    setStyleSheet(StormTheme::GetDialogStyle() + StormCancelButtonQssRule());
    auto* root_lay = new QVBoxLayout(this);
    root_lay->setContentsMargins(16, 16, 16, 16);
    root_lay->setSpacing(12);

    // -------------------------------------------------------------------------
    // 1. Header Card (Cover + Title + Metadata)
    // -------------------------------------------------------------------------
    auto* header_card = new QGroupBox(this);
    header_card->setObjectName(QStringLiteral("BentoGameHeaderCard"));
    header_card->setProperty("stormHeroCard", true);

    auto* header_lay = new QHBoxLayout(header_card);
    header_lay->setContentsMargins(14, 12, 14, 12);
    header_lay->setSpacing(14);

    // Rounded Cover Icon
    auto* cover_label = new QLabel(header_card);
    cover_label->setFixedSize(80, 80);
    QPixmap cover_pix(80, 80);
    cover_pix.fill(QColor(15, 23, 42));
    {
        QPainter painter(&cover_pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(24, 34, 56));
        painter.setPen(QColor(0, 240, 255, 120));
        painter.drawRoundedRect(0, 0, 80, 80, 10, 10);
        painter.setPen(QColor(0, 240, 255));
        QFont f = painter.font();
        f.setPointSize(26);
        painter.setFont(f);
        painter.drawText(cover_pix.rect(), Qt::AlignCenter, QStringLiteral("🎮"));
    }
    cover_label->setPixmap(cover_pix);
    header_lay->addWidget(cover_label);

    auto* title_lay = new QVBoxLayout();
    title_lay->setContentsMargins(0, 0, 0, 0);
    title_lay->setSpacing(4);

    QString game_title = m_profile ? QString::fromStdString(m_profile->game_name) : QString();
    if (game_title.isEmpty()) {
        game_title = StormLang(QStringLiteral("Игра Nintendo Switch"), QStringLiteral("Nintendo Switch Game"));
    }

    auto* title_lbl = new QLabel(game_title, header_card);
    QFont tf = title_lbl->font();
    tf.setBold(true);
    tf.setPointSize(12);
    title_lbl->setFont(tf);
    title_lbl->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: bold;"));
    title_lbl->setWordWrap(true);
    title_lay->addWidget(title_lbl);

    auto* badge_row = new QWidget(header_card);
    auto* badge_lay = new QHBoxLayout(badge_row);
    badge_lay->setContentsMargins(0, 0, 0, 0);
    badge_lay->setSpacing(8);

    const QString tid_str = QStringLiteral("TID: %1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper();
    auto* tid_btn = new QPushButton(tid_str, badge_row);
    tid_btn->setCursor(Qt::PointingHandCursor);
    tid_btn->setToolTip(StormLang(QStringLiteral("Нажмите, чтобы скопировать Title ID"), QStringLiteral("Click to copy Title ID")));
    connect(tid_btn, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(QStringLiteral("%1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper());
    });
    badge_lay->addWidget(tid_btn);

    auto* profile_badge = new QLabel(StormLang(
        QStringLiteral("Профиль: Оптимизирован для STORM SWITCH"),
        QStringLiteral("Profile: Optimized for STORM SWITCH")), badge_row);
    profile_badge->setStyleSheet(QStringLiteral("color: #00F0FF; font-weight: 600; font-size: 8.5pt;"));
    badge_lay->addWidget(profile_badge);
    badge_lay->addStretch();
    title_lay->addWidget(badge_row);

    header_lay->addLayout(title_lay);
    root_lay->addWidget(header_card);

    // -------------------------------------------------------------------------
    // 2. Scrollable Body
    // -------------------------------------------------------------------------
    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    StormUI::StructureEngine::EnforceScrollMargins(scroll_area, 18);

    auto* scroll_content = new QWidget(scroll_area);
    auto* scroll_lay = new QVBoxLayout(scroll_content);
    scroll_lay->setContentsMargins(0, 0, 0, 0);
    scroll_lay->setSpacing(12);

    // Section 1: Recommended Fixes (with individual checkboxes)
    auto* fixes_card = new QGroupBox(StormLang(
        QStringLiteral("Рекомендуемые исправления и оптимизации"),
        QStringLiteral("Recommended Fixes & Optimizations")), scroll_content);
    StormUI::StructureEngine::StyleAsVolumetricCard(fixes_card);
    auto* fixes_lay = new QVBoxLayout(fixes_card);
    fixes_lay->setSpacing(8);

    if (m_profile && !m_profile->ini_settings.empty()) {
        for (const auto& [key, val] : m_profile->ini_settings) {
            FixItem item;
            item.keys.push_back(key);
            const QString localized_label = GetLocalizedSettingLabel(key);
            const QString formatted_val = FormatSettingValue(key, val);
            item.checkbox = new QCheckBox(QStringLiteral("%1 → %2").arg(localized_label, formatted_val), fixes_card);
            item.checkbox->setChecked(true);
            fixes_lay->addWidget(item.checkbox);
            m_fix_items.push_back(item);
        }
    } else {
        auto* no_fixes = new QLabel(StormLang(
            QStringLiteral("Для этой игры все параметры уже соответствуют оптимальным настройкам."),
            QStringLiteral("All settings for this game already match optimal values.")), fixes_card);
        fixes_lay->addWidget(no_fixes);
    }
    scroll_lay->addWidget(fixes_card);

    // Section 2: What Changes (table "было → станет")
    auto* changes_card = new QGroupBox(StormLang(
        QStringLiteral("Что изменится (настройка: было → станет)"),
        QStringLiteral("Changes Overview (setting: current → target)")), scroll_content);
    StormUI::StructureEngine::StyleAsVolumetricCard(changes_card);
    auto* changes_lay = new QVBoxLayout(changes_card);

    m_changes_table = new QTableWidget(changes_card);
    m_changes_table->setColumnCount(3);
    m_changes_table->setHorizontalHeaderLabels({
        StormLang(QStringLiteral("Параметр"), QStringLiteral("Setting")),
        StormLang(QStringLiteral("Текущее значение"), QStringLiteral("Current")),
        StormLang(QStringLiteral("Рекомендуемое"), QStringLiteral("Target"))
    });
    m_changes_table->horizontalHeader()->setStretchLastSection(true);
    m_changes_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_changes_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_changes_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    StormUI::StructureEngine::ModernizeDataView(m_changes_table);
    m_changes_table->setMinimumHeight(140);

    PopulateProfileData();
    changes_lay->addWidget(m_changes_table);
    scroll_lay->addWidget(changes_card);

    // Section 3: Known Issues
    if (m_profile && (!m_profile->issues_ru.empty() || !m_profile->issues_en.empty())) {
        auto* issues_card = new QGroupBox(StormLang(
            QStringLiteral("Известные проблемы игры и устраняемые сбои"),
            QStringLiteral("Known Game Issues Resolved by Profile")), scroll_content);
        StormUI::StructureEngine::StyleAsVolumetricCard(issues_card);
        auto* issues_lay = new QVBoxLayout(issues_card);

        const QString issues_txt = StormLang(
            QString::fromStdString(m_profile->issues_ru.empty() ? m_profile->issues_en : m_profile->issues_ru),
            QString::fromStdString(m_profile->issues_en.empty() ? m_profile->issues_ru : m_profile->issues_en));

        auto* issues_lbl = new QLabel(issues_txt, issues_card);
        issues_lbl->setWordWrap(true);
        issues_lbl->setStyleSheet(QStringLiteral("color: #E2E8F0; line-height: 1.4;"));
        issues_lay->addWidget(issues_lbl);
        scroll_lay->addWidget(issues_card);
    }

    // Section 4: Profile Source
    auto* source_card = new QGroupBox(StormLang(
        QStringLiteral("Источник профиля"),
        QStringLiteral("Profile Source")), scroll_content);
    StormUI::StructureEngine::StyleAsVolumetricCard(source_card);
    auto* source_lay = new QVBoxLayout(source_card);

    auto* source_lbl = new QLabel(StormLang(
        QStringLiteral("Официальная база проверенных профилей STORM SWITCH. Профиль протестирован на стабильность 60/30 FPS и плавность геймплея."),
        QStringLiteral("Official STORM SWITCH verified profile database. Tested for smooth 60/30 FPS performance and stability.")), source_card);
    source_lbl->setWordWrap(true);
    source_lbl->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.7); font-size: 8.5pt;"));
    source_lay->addWidget(source_lbl);
    scroll_lay->addWidget(source_card);

    scroll_area->setWidget(scroll_content);
    root_lay->addWidget(scroll_area, 1);

    // -------------------------------------------------------------------------
    // 3. Footer (Checkbox + Action Buttons)
    // -------------------------------------------------------------------------
    auto* footer_row = new QWidget(this);
    auto* footer_lay = new QVBoxLayout(footer_row);
    footer_lay->setContentsMargins(0, 4, 0, 0);
    footer_lay->setSpacing(10);

    m_dont_ask_cb = new QCheckBox(StormLang(
        QStringLiteral("Больше не показывать это окно для этой игры (запомнить выбор)"),
        QStringLiteral("Don't ask again for this game (remember choice)")), footer_row);
    footer_lay->addWidget(m_dont_ask_cb);

    auto* btn_row = new QWidget(footer_row);
    auto* btn_lay = new QHBoxLayout(btn_row);
    btn_lay->setContentsMargins(0, 0, 0, 0);
    btn_lay->setSpacing(8);

    auto* apply_and_launch_btn = new QPushButton(StormLang(
        QStringLiteral("Применить и запустить"),
        QStringLiteral("Apply & Launch")), btn_row);
    apply_and_launch_btn->setCursor(Qt::PointingHandCursor);
    apply_and_launch_btn->setDefault(true);
    apply_and_launch_btn->setProperty("primary", true);
    connect(apply_and_launch_btn, &QPushButton::clicked, this, [this]() {
        m_apply_fixes = true;
        m_save_to_ini = true;
        m_result = Result::ApplyAndLaunch;
        accept();
    });
    btn_lay->addWidget(apply_and_launch_btn);

    auto* session_only_btn = new QPushButton(StormLang(
        QStringLiteral("Только для этого запуска"),
        QStringLiteral("This Session Only")), btn_row);
    session_only_btn->setCursor(Qt::PointingHandCursor);
    connect(session_only_btn, &QPushButton::clicked, this, [this]() {
        m_apply_fixes = true;
        m_save_to_ini = false;
        m_result = Result::ApplyAndLaunch;
        accept();
    });
    btn_lay->addWidget(session_only_btn);

    auto* save_profile_btn = new QPushButton(StormLang(
        QStringLiteral("Сохранить в профиль игры"),
        QStringLiteral("Save to Game Profile")), btn_row);
    save_profile_btn->setCursor(Qt::PointingHandCursor);
    connect(save_profile_btn, &QPushButton::clicked, this, [this]() {
        m_apply_fixes = true;
        m_save_to_ini = true;
        m_result = Result::ApplyAndLaunch;
        accept();
    });
    btn_lay->addWidget(save_profile_btn);

    btn_lay->addStretch();

    auto* cancel_btn = new QPushButton(StormLang(
        QStringLiteral("Отмена"),
        QStringLiteral("Cancel")), btn_row);
    cancel_btn->setProperty("isCancelButton", true);
    cancel_btn->setCursor(Qt::PointingHandCursor);
    connect(cancel_btn, &QPushButton::clicked, this, [this]() {
        m_result = Result::Cancel;
        reject();
    });
    btn_lay->addWidget(cancel_btn);

    footer_lay->addWidget(btn_row);
    root_lay->addWidget(footer_row);
}

void GameFixDialog::PopulateProfileData() {
    if (!m_changes_table || !m_profile) return;

    m_changes_table->setRowCount(0);
    int row = 0;
    for (const auto& [key, target_val] : m_profile->ini_settings) {
        m_changes_table->insertRow(row);

        const QString setting_name = GetLocalizedSettingLabel(key);
        auto* item_name = new QTableWidgetItem(setting_name);
        item_name->setFlags(item_name->flags() & ~Qt::ItemIsEditable);
        m_changes_table->setItem(row, 0, item_name);

        const QString curr_val = StormLang(QStringLiteral("По умолчанию"), QStringLiteral("Default"));
        auto* item_curr = new QTableWidgetItem(curr_val);
        item_curr->setFlags(item_curr->flags() & ~Qt::ItemIsEditable);
        m_changes_table->setItem(row, 1, item_curr);

        const QString target_formatted = FormatSettingValue(key, target_val);
        auto* item_target = new QTableWidgetItem(target_formatted);
        item_target->setFlags(item_target->flags() & ~Qt::ItemIsEditable);
        item_target->setForeground(QColor(0, 240, 255));
        m_changes_table->setItem(row, 2, item_target);

        row++;
    }
}

GameFixDialog::Result GameFixDialog::Execute() {
    if (!m_profile) {
        return Result::LaunchWithoutChanges;
    }

    // Check dont_ask status
    QByteArray utf8_str = m_game_path.toUtf8();
    const auto file_path_hash = Common::CityHash64(utf8_str.constData(), static_cast<std::size_t>(utf8_str.size()));
    const auto specific_config = fmt::format("{:016X}_{:016X}", m_title_id, file_path_hash);
    const auto legacy_config = fmt::format("{:016X}", m_title_id);

    std::filesystem::path custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
    std::string target_ini = (custom_path / (specific_config + ".ini")).string();
    std::string check_ini = target_ini;
    if (!std::filesystem::exists(check_ini) && std::filesystem::exists(custom_path / (legacy_config + ".ini"))) {
        check_ini = (custom_path / (legacy_config + ".ini")).string();
    }

    bool dont_ask = false;
    if (std::filesystem::exists(check_ini)) {
        std::ifstream f(check_ini);
        std::string l;
        while (std::getline(f, l)) {
            if (l.find("storm_fix_dont_ask=true") != std::string::npos ||
                l.find("storm_fix_dont_ask = true") != std::string::npos) {
                dont_ask = true;
                break;
            }
        }
    }

    if (dont_ask && !m_force_show) {
        Core::GameFixDatabase::SetFixesEnabled(true);
        return Result::ApplyAndLaunch;
    }

    exec();

    m_dont_ask_checked = m_dont_ask_cb && m_dont_ask_cb->isChecked();
    if (m_dont_ask_checked != dont_ask) {
        Core::GameFixDatabase::SetDontAskAgain(m_title_id, target_ini, m_dont_ask_checked);
        Core::GameFixDatabase::SetDontAskAgain(m_title_id, (custom_path / (legacy_config + ".ini")).string(), m_dont_ask_checked);
    }

    if (m_result == Result::ApplyAndLaunch) {
        m_excluded_keys.clear();
        for (const auto& item : m_fix_items) {
            if (item.checkbox && !item.checkbox->isChecked()) {
                for (const auto& k : item.keys) {
                    m_excluded_keys.push_back(k);
                }
            }
        }
        Core::GameFixDatabase::SetExcludedKeys(m_excluded_keys);
        Core::GameFixDatabase::SetFixesEnabled(true);
        if (m_save_to_ini) {
            Core::GameFixDatabase::ApplyProfileDirectly(m_title_id, m_game_path.toStdString());
        }
    } else if (m_result == Result::LaunchWithoutChanges) {
        Core::GameFixDatabase::SetFixesEnabled(false);
    }

    return m_result;
}
