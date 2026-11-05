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
#include "core/loader/loader.h"
#include "core/file_sys/vfs/vfs.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

GameFixDialog::GameFixDialog(QWidget* parent, u64 title_id_, const QString& game_path_,
                             bool force_show_, Core::System& system_,
                             const QPixmap& game_icon_)
    : QDialog(parent), m_title_id(title_id_), m_game_path(game_path_),
      m_force_show(force_show_), m_system(system_), m_game_icon(game_icon_) {
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
    const std::string k = raw_key;
    // GPU & Graphics
    if (k.find("gpu_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции GPU"), QStringLiteral("GPU Accuracy"));
    if (k.find("use_disk_shader_cache") != std::string::npos)
        return StormLang(QStringLiteral("Дисковый кэш шейдеров"), QStringLiteral("Disk Shader Cache"));
    if (k.find("async_presentation") != std::string::npos)
        return StormLang(QStringLiteral("Асинхронный вывод кадров"), QStringLiteral("Async Presentation"));
    if (k.find("sync_memory_operations") != std::string::npos)
        return StormLang(QStringLiteral("Синхронизация операций памяти GPU"), QStringLiteral("Sync Memory Operations"));
    if (k.find("early_release_fences") != std::string::npos)
        return StormLang(QStringLiteral("Раннее освобождение барьеров GPU"), QStringLiteral("Early Release Fences"));
    if (k.find("use_asynchronous_gpu_emulation") != std::string::npos)
        return StormLang(QStringLiteral("Асинхронная эмуляция GPU"), QStringLiteral("Asynchronous GPU Emulation"));
    if (k.find("use_reactive_flushing") != std::string::npos)
        return StormLang(QStringLiteral("Реактивный сброс очередей GPU"), QStringLiteral("Reactive Flushing"));
    if (k.find("barrier_feedback_loops") != std::string::npos)
        return StormLang(QStringLiteral("Обратная связь барьеров"), QStringLiteral("Barrier Feedback Loops"));
    if (k.find("use_fast_gpu_time") != std::string::npos)
        return StormLang(QStringLiteral("Ускоренное время GPU"), QStringLiteral("Fast GPU Time"));
    if (k.find("use_asynchronous_shaders") != std::string::npos)
        return StormLang(QStringLiteral("Асинхронные шейдеры"), QStringLiteral("Asynchronous Shaders"));
    if (k.find("nvdec_emulation") != std::string::npos)
        return StormLang(QStringLiteral("Декодирование видео (NVDEC)"), QStringLiteral("NVDEC Video Emulation"));
    if (k.find("astc_recompression") != std::string::npos)
        return StormLang(QStringLiteral("Пересжатие текстур ASTC"), QStringLiteral("ASTC Texture Recompression"));
    if (k.find("accelerate_astc") != std::string::npos)
        return StormLang(QStringLiteral("Декодирование текстур ASTC"), QStringLiteral("ASTC Texture Decoding"));
    if (k.find("vram_usage_mode") != std::string::npos)
        return StormLang(QStringLiteral("Режим использования видеопамяти (VRAM)"), QStringLiteral("VRAM Usage Mode"));
    if (k.find("vram_budget_governor") != std::string::npos)
        return StormLang(QStringLiteral("Контроль бюджета VRAM"), QStringLiteral("VRAM Budget Governor"));
    if (k.find("vram_garbage_collection") != std::string::npos)
        return StormLang(QStringLiteral("Сборка мусора VRAM"), QStringLiteral("VRAM Garbage Collection"));
    if (k.find("gpu_fence_behavior") != std::string::npos)
        return StormLang(QStringLiteral("Поведение барьеров GPU"), QStringLiteral("GPU Fence Behavior"));
    if (k.find("dma_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции DMA"), QStringLiteral("DMA Accuracy"));
    if (k.find("dyna_state") != std::string::npos || k.find("vertex_input_dynamic_state") != std::string::npos)
        return StormLang(QStringLiteral("Динамическое состояние вершин"), QStringLiteral("Vertex Dynamic State"));
    if (k.find("resolution_setup") != std::string::npos)
        return StormLang(QStringLiteral("Масштаб разрешения"), QStringLiteral("Resolution Scale"));
    if (k.find("scaling_filter") != std::string::npos)
        return StormLang(QStringLiteral("Фильтр масштабирования (FSR)"), QStringLiteral("Scaling Filter (FSR)"));
    if (k.find("anti_aliasing") != std::string::npos)
        return StormLang(QStringLiteral("Сглаживание (SMAA / FXAA)"), QStringLiteral("Anti-Aliasing"));
    if (k.find("max_anisotropy") != std::string::npos || k.find("anisotropic_filtering") != std::string::npos)
        return StormLang(QStringLiteral("Анизотропная фильтрация"), QStringLiteral("Anisotropic Filtering"));
    if (k.find("vsync_mode") != std::string::npos || k.find("use_vsync") != std::string::npos)
        return StormLang(QStringLiteral("Вертикальная синхронизация (VSync)"), QStringLiteral("Vertical Sync (VSync)"));
    if (k.find("aspect_ratio") != std::string::npos)
        return StormLang(QStringLiteral("Соотношение сторон экрана"), QStringLiteral("Aspect Ratio"));
    if (k.find("enable_compute_pipelines") != std::string::npos)
        return StormLang(QStringLiteral("Вычислительные конвейеры Vulkan"), QStringLiteral("Vulkan Compute Pipelines"));
    if (k.find("enable_gpu_buffer_readback") != std::string::npos)
        return StormLang(QStringLiteral("Обратное чтение буферов GPU"), QStringLiteral("GPU Buffer Readback"));
    if (k.find("use_vulkan_driver_pipeline_cache") != std::string::npos)
        return StormLang(QStringLiteral("Кэш конвейеров драйвера Vulkan"), QStringLiteral("Vulkan Pipeline Cache"));
    if (k.find("eco_frame_pacing") != std::string::npos)
        return StormLang(QStringLiteral("Эко-кадрирование"), QStringLiteral("Eco Frame Pacing"));
    if (k.find("fix_bloom_effects") != std::string::npos)
        return StormLang(QStringLiteral("Исправление эффектов свечения (Bloom)"), QStringLiteral("Fix Bloom Effects"));
    if (k.find("emulate_bgr565") != std::string::npos)
        return StormLang(QStringLiteral("Эмуляция BGR565"), QStringLiteral("Emulate BGR565"));
    if (k.find("rescale_hack") != std::string::npos)
        return StormLang(QStringLiteral("Хак масштабирования геометрии"), QStringLiteral("Rescale Hack"));
    if (k.find("use_video_framerate") != std::string::npos)
        return StormLang(QStringLiteral("Частота кадров видео (24/30 FPS)"), QStringLiteral("Use Video Framerate"));
    if (k.find("fsr_sharpening_slider") != std::string::npos)
        return StormLang(QStringLiteral("Резкость FSR"), QStringLiteral("FSR Sharpening"));

    // CPU & Memory
    if (k.find("cpu_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции ЦП"), QStringLiteral("CPU Accuracy"));
    if (k.find("cpu_backend") != std::string::npos)
        return StormLang(QStringLiteral("Бэкенд ЦП (JIT/NCE)"), QStringLiteral("CPU Backend"));
    if (k.find("cpuopt_fastmem") != std::string::npos)
        return StormLang(QStringLiteral("Быстрая память ЦП (Fastmem)"), QStringLiteral("Fastmem"));
    if (k.find("cpuopt_fastmem_exclusives") != std::string::npos)
        return StormLang(QStringLiteral("Эксклюзивный доступ Fastmem"), QStringLiteral("Fastmem Exclusives"));
    if (k.find("cpuopt_ignore_memory_aborts") != std::string::npos)
        return StormLang(QStringLiteral("Игнорирование сбоев памяти ЦП"), QStringLiteral("Ignore Memory Aborts"));
    if (k.find("cpuopt_recompile_exclusives") != std::string::npos)
        return StormLang(QStringLiteral("Рекомпиляция эксклюзивов ЦП"), QStringLiteral("Recompile Exclusives"));
    if (k.find("cpuopt_unsafe_unfuse_fma") != std::string::npos)
        return StormLang(QStringLiteral("Разделение инструкций FMA"), QStringLiteral("Unfuse FMA"));
    if (k.find("use_multi_core") != std::string::npos)
        return StormLang(QStringLiteral("Многопоточная эмуляция ЦП"), QStringLiteral("Multicore CPU Emulation"));

    // System & General
    if (k.find("memory_layout_mode") != std::string::npos)
        return StormLang(QStringLiteral("Конфигурация памяти DRAM"), QStringLiteral("Memory Layout (DRAM)"));
    if (k.find("use_docked_mode") != std::string::npos)
        return StormLang(QStringLiteral("Режим консоли (ТВ / Портативный)"), QStringLiteral("Console Mode (Docked / Handheld)"));
    if (k.find("airplane_mode") != std::string::npos)
        return StormLang(QStringLiteral("Режим «В самолете»"), QStringLiteral("Airplane Mode"));
    if (k.find("language_index") != std::string::npos)
        return StormLang(QStringLiteral("Язык системы"), QStringLiteral("System Language"));
    if (k.find("region_index") != std::string::npos)
        return StormLang(QStringLiteral("Регион системы"), QStringLiteral("System Region"));
    if (k.find("disable_macro_hle") != std::string::npos)
        return StormLang(QStringLiteral("Макросы HLE GPU"), QStringLiteral("Macro HLE"));
    if (k.find("disable_macro_jit") != std::string::npos)
        return StormLang(QStringLiteral("JIT-компиляция макросов GPU"), QStringLiteral("Macro JIT"));
    if (k.find("disable_web_applet") != std::string::npos)
        return StormLang(QStringLiteral("Веб-апплет Switch"), QStringLiteral("Web Applet"));
    if (k.find("use_auto_stub") != std::string::npos)
        return StormLang(QStringLiteral("Автоматические заглушки сервисов"), QStringLiteral("Auto Stub"));

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

QString GameFixDialog::FormatSettingValue(const std::string& raw_key, const std::string& val) const {
    const std::string k = raw_key;

    // 1. GPU Accuracy
    if (k.find("gpu_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Нормальная"), QStringLiteral("Normal"));
        if (val == "1") return StormLang(QStringLiteral("Высокая"), QStringLiteral("High"));
        if (val == "2") return StormLang(QStringLiteral("Экстремальная"), QStringLiteral("Extreme"));
    }

    // 2. NVDEC Emulation
    if (k.find("nvdec_emulation") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Отключено"), QStringLiteral("Off"));
        if (val == "1") return StormLang(QStringLiteral("Программное (ЦП)"), QStringLiteral("CPU Video Decoding"));
        if (val == "2") return StormLang(QStringLiteral("Аппаратное (ГПУ)"), QStringLiteral("GPU Video Decoding"));
        if (val == "3") return StormLang(QStringLiteral("Асинхронное (ГПУ)"), QStringLiteral("Async GPU Decoding"));
    }

    // 3. ASTC Recompression
    if (k.find("astc_recompression") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Без сжатия"), QStringLiteral("Uncompressed"));
        if (val == "1") return StormLang(QStringLiteral("BC1 (Низкое качество)"), QStringLiteral("BC1 (Low quality)"));
        if (val == "2") return StormLang(QStringLiteral("BC3 (Среднее качество)"), QStringLiteral("BC3 (Medium quality)"));
    }

    // 4. ASTC Decoding / Accelerate ASTC
    if (k.find("accelerate_astc") != std::string::npos || k.find("astc_decode_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("ЦП"), QStringLiteral("CPU"));
        if (val == "1") return StormLang(QStringLiteral("ГПУ"), QStringLiteral("GPU"));
        if (val == "2") return StormLang(QStringLiteral("Асинхронно ЦП"), QStringLiteral("CPU Asynchronous"));
    }

    // 5. CPU Accuracy
    if (k.find("cpu_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точная"), QStringLiteral("Accurate"));
        if (val == "2") return StormLang(QStringLiteral("Небезопасная"), QStringLiteral("Unsafe"));
        if (val == "3") return StormLang(QStringLiteral("Параноидальная"), QStringLiteral("Paranoid"));
    }

    // 6. CPU Backend
    if (k.find("cpu_backend") != std::string::npos) {
        if (val == "0") return QStringLiteral("Dynarmic (JIT)");
        if (val == "1") return QStringLiteral("NCE (Native Code)");
    }

    // 7. VRAM Usage Mode
    if (k.find("vram_usage_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Консервативный"), QStringLiteral("Conservative"));
        if (val == "1") return StormLang(QStringLiteral("Агрессивный"), QStringLiteral("Aggressive"));
    }

    // 8. GPU Fence Behavior
    if (k.find("gpu_fence_behavior") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"));
        if (val == "2") return StormLang(QStringLiteral("Быстро"), QStringLiteral("Fast"));
    }

    // 9. DMA Accuracy
    if (k.find("dma_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"));
        if (val == "3") return StormLang(QStringLiteral("Быстро"), QStringLiteral("Fast"));
    }

    // 10. Resolution Scale
    if (k.find("resolution_setup") != std::string::npos) {
        if (val == "0") return QStringLiteral("0.5x (360p/540p)");
        if (val == "1") return QStringLiteral("0.75x (540p/810p)");
        if (val == "2") return QStringLiteral("1x (720p/1080p)");
        if (val == "3") return QStringLiteral("1.5x (1080p/1620p)");
        if (val == "4") return QStringLiteral("2x (1440p/2160p)");
        if (val == "5") return QStringLiteral("3x");
        if (val == "6") return QStringLiteral("4x");
    }

    // 11. Scaling Filter
    if (k.find("scaling_filter") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Ближайший сосед"), QStringLiteral("Nearest Neighbor"));
        if (val == "1") return StormLang(QStringLiteral("Билинейный"), QStringLiteral("Bilinear"));
        if (val == "2") return StormLang(QStringLiteral("Бикубический"), QStringLiteral("Bicubic"));
        if (val == "3") return QStringLiteral("Gaussian");
        if (val == "4") return StormLang(QStringLiteral("Масштабирование FSR"), QStringLiteral("AMD FidelityFX Super Resolution (FSR)"));
    }

    // 12. Anti-Aliasing
    if (k.find("anti_aliasing") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Выключено"), QStringLiteral("None"));
        if (val == "1") return QStringLiteral("FXAA");
        if (val == "2") return QStringLiteral("SMAA");
    }

    // 13. Max Anisotropy
    if (k.find("max_anisotropy") != std::string::npos || k.find("anisotropic_filtering") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"));
        if (val == "1") return QStringLiteral("2x");
        if (val == "2") return QStringLiteral("4x");
        if (val == "3") return QStringLiteral("8x");
        if (val == "4") return QStringLiteral("16x");
        if (val == "5") return StormLang(QStringLiteral("Выключено"), QStringLiteral("Off"));
    }

    // 14. VSync Mode
    if (k.find("vsync_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Немедленно (Выкл.)"), QStringLiteral("Immediate (Off)"));
        if (val == "1") return StormLang(QStringLiteral("Почтовый ящик (Mailbox)"), QStringLiteral("Mailbox"));
        if (val == "2") return StormLang(QStringLiteral("FIFO (Вкл.)"), QStringLiteral("FIFO (On)"));
        if (val == "3") return StormLang(QStringLiteral("FIFO Расслабленный"), QStringLiteral("FIFO Relaxed"));
    }

    // 15. Aspect Ratio
    if (k.find("aspect_ratio") != std::string::npos) {
        if (val == "0") return QStringLiteral("16:9");
        if (val == "1") return QStringLiteral("4:3");
        if (val == "2") return QStringLiteral("21:9");
        if (val == "3") return QStringLiteral("16:10");
        if (val == "4") return StormLang(QStringLiteral("Растянуть на весь экран"), QStringLiteral("Stretch to Window"));
    }

    // 16. Memory Layout Mode
    if (k.find("memory_layout_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("4 ГБ DRAM"), QStringLiteral("4GB DRAM"));
        if (val == "1") return StormLang(QStringLiteral("6 ГБ DRAM"), QStringLiteral("6GB DRAM"));
        if (val == "2") return StormLang(QStringLiteral("8 ГБ DRAM"), QStringLiteral("8GB DRAM"));
    }

    // 17. Use Docked Mode
    if (k.find("use_docked_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Портативный режим"), QStringLiteral("Handheld Mode"));
        if (val == "1") return StormLang(QStringLiteral("ТВ-режим (Docked)"), QStringLiteral("Docked Mode"));
    }

    // 18. Dyna State
    if (k.find("dyna_state") != std::string::npos) {
        if (val == "0" || val == "False" || val == "false") return StormLang(QStringLiteral("Выключено"), QStringLiteral("Disabled"));
        if (val == "1" || val == "True" || val == "true") return StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"));
        if (val == "2") return StormLang(QStringLiteral("Расширенное"), QStringLiteral("Extended"));
    }

    // 19. Language Index
    if (k.find("language_index") != std::string::npos) {
        if (val == "10") return StormLang(QStringLiteral("Русский"), QStringLiteral("Russian"));
        if (val == "1") return StormLang(QStringLiteral("Английский"), QStringLiteral("English"));
    }

    // 20. Region Index
    if (k.find("region_index") != std::string::npos) {
        if (val == "2") return StormLang(QStringLiteral("Европа"), QStringLiteral("Europe"));
        if (val == "1") return StormLang(QStringLiteral("США"), QStringLiteral("USA"));
        if (val == "0") return StormLang(QStringLiteral("Япония"), QStringLiteral("Japan"));
    }

    // Boolean fallback
    if (val == "true" || val == "True" || val == "1")
        return StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"));
    if (val == "false" || val == "False" || val == "0")
        return StormLang(QStringLiteral("Выключено"), QStringLiteral("Disabled"));

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

    // Clean High-DPI Cover Icon (zero border / frame)
    auto* cover_label = new QLabel(header_card);
    cover_label->setFixedSize(84, 84);
    cover_label->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    QPixmap raw_cover;
    if (!m_game_icon.isNull()) {
        raw_cover = m_game_icon;
    }
    if (raw_cover.isNull() && m_title_id != 0) {
        const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
        const auto icon_file = cache_dir / "icons" / fmt::format("{:016X}.png", m_title_id);
        const auto jpeg_file = cache_dir / fmt::format("{:016X}.jpeg", m_title_id);
        const QString ip = QString::fromStdString(Common::FS::PathToUTF8String(icon_file));
        const QString jp = QString::fromStdString(Common::FS::PathToUTF8String(jpeg_file));
        if (QFile::exists(ip)) {
            raw_cover.load(ip);
        } else if (QFile::exists(jp)) {
            raw_cover.load(jp);
        }
    }
    if (raw_cover.isNull() && !m_game_path.isEmpty()) {
        std::vector<u8> icon_bytes;
        const auto vfs = m_system.GetFilesystem();
        if (vfs) {
            auto vf = vfs->OpenFile(m_game_path.toStdString(), FileSys::OpenMode::Read);
            if (vf) {
                auto app_loader = Loader::GetLoader(m_system, vf);
                if (app_loader && app_loader->ReadIcon(icon_bytes) == Loader::ResultStatus::Success && !icon_bytes.empty()) {
                    raw_cover.loadFromData(icon_bytes.data(), static_cast<u32>(icon_bytes.size()));
                }
            }
        }
    }

    const qreal dpr = (devicePixelRatioF() > 0.0) ? devicePixelRatioF() : 1.0;
    const int px_sz = static_cast<int>(std::round(84 * dpr));
    QPixmap rounded_cover(px_sz, px_sz);
    rounded_cover.fill(Qt::transparent);

    if (!raw_cover.isNull()) {
        QPainter p(&rounded_cover);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip_p;
        clip_p.addRoundedRect(0, 0, px_sz, px_sz, static_cast<int>(8 * dpr), static_cast<int>(8 * dpr));
        p.setClipPath(clip_p);
        p.drawPixmap(0, 0, px_sz, px_sz, raw_cover.scaled(px_sz, px_sz, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        QPainter p(&rounded_cover);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(24, 34, 56));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, px_sz, px_sz, static_cast<int>(8 * dpr), static_cast<int>(8 * dpr));
        p.setPen(QColor(0, 240, 255));
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(static_cast<int>(12 * dpr));
        p.setFont(f);
        p.drawText(rounded_cover.rect(), Qt::AlignCenter, QStringLiteral("SWITCH"));
    }
    rounded_cover.setDevicePixelRatio(dpr);
    cover_label->setPixmap(rounded_cover);
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
    tid_btn->setMinimumWidth(220);
    tid_btn->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    tid_btn->setCursor(Qt::PointingHandCursor);
    tid_btn->setToolTip(StormLang(QStringLiteral("Нажмите, чтобы скопировать Title ID"), QStringLiteral("Click to copy Title ID")));
    tid_btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255, 255, 255, 0.08);"
        "  color: #00F0FF;"
        "  border: 1px solid rgba(0, 240, 255, 0.35);"
        "  border-radius: 5px;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-weight: 700;"
        "  font-size: 9pt;"
        "  padding: 5px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(0, 240, 255, 0.15);"
        "  border-color: #00F0FF;"
        "}"));
    connect(tid_btn, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(QStringLiteral("%1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper());
    });
    badge_lay->addWidget(tid_btn);

    auto* profile_badge = new QLabel(StormLang(
        QStringLiteral("Рекомендуемые настройки STORM SWITCH"),
        QStringLiteral("Recommended STORM SWITCH Settings")), badge_row);
    profile_badge->setStyleSheet(QStringLiteral(
        "background: #0D2B36;"
        "color: #00E5FF;"
        "border: 1px solid #00E5FF;"
        "border-radius: 5px;"
        "font-weight: 700;"
        "font-size: 8.5pt;"
        "padding: 4px 10px;"
    ));
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
    scroll_area->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #0D0E15; width: 8px; margin: 2px 0px; border-radius: 4px; border: none; }"
        "QScrollBar::handle:vertical { background: rgba(139, 92, 246, 0.55); min-height: 28px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(139, 92, 246, 0.85); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: #0D0E15; border: none; }"
    ));
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
    fixes_card->setStyleSheet(QStringLiteral(
        "QGroupBox { border: 1px solid #00E5FF; border-radius: 8px; margin-top: 14px; background: rgba(13, 43, 54, 0.45); }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 2px 8px; color: #00E5FF; font-weight: 700; background: #0D2B36; border-radius: 4px; border: 1px solid #00E5FF; }"
    ));
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

    // Centered, bold, faceted 3D header badge "Метод запуска" / "Launch Method"
    const auto pal = StormTheme::GetThemePalette();
    const QString launch_method_text = StormLangStorm11(
        QStringLiteral("Метод запуска"),
        QStringLiteral("Launch Method"),
        QStringLiteral("Startmethode"),
        QStringLiteral("Méthode de lancement"),
        QStringLiteral("启动方式"),
        QStringLiteral("실행 방식"),
        QStringLiteral("起動方法"),
        QStringLiteral("Método de inicio"),
        QStringLiteral("Metodo di avvio"),
        QStringLiteral("Método de inicialização"),
        QStringLiteral("लॉन्च विधि"));

    auto* launch_method_badge = new QLabel(launch_method_text, footer_row);
    launch_method_badge->setAlignment(Qt::AlignCenter);
    const QString badge_qss = QStringLiteral(
        "font-weight: 800; font-size: 12px; letter-spacing: 0.6px; color: %1; "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %2, stop:0.5 %3, stop:1 %4); "
        "border: 1.5px solid %5; border-top: 1.5px solid %6; border-radius: 8px; padding: 5px 18px;")
        .arg(pal.accent)
        .arg(pal.cardAlt)
        .arg(pal.card)
        .arg(pal.surface)
        .arg(pal.border)
        .arg(pal.accentHover);
    launch_method_badge->setStyleSheet(badge_qss);
    footer_lay->addWidget(launch_method_badge, 0, Qt::AlignHCenter);

    auto* btn_row = new QWidget(footer_row);
    auto* btn_lay = new QHBoxLayout(btn_row);
    btn_lay->setContentsMargins(0, 0, 0, 0);
    btn_lay->setSpacing(8);

    auto* apply_and_launch_btn = new QPushButton(StormLang(
        QStringLiteral("Применить авто-исправление и запустить"),
        QStringLiteral("Apply Auto-Fix & Launch")), btn_row);
    apply_and_launch_btn->setCursor(Qt::PointingHandCursor);
    apply_and_launch_btn->setDefault(true);
    apply_and_launch_btn->setProperty("primary", true);
    apply_and_launch_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8B5CF6, stop:0.5 #7C3AED, stop:1 #6D28D9); color: #FFFFFF; font-weight: 700; font-size: 12px; border-radius: 9px; padding: 7px 18px; border: 1.5px solid #7C3AED; border-top: 1.5px solid #A78BFA; border-bottom: 2px solid #5B21B6; min-height: 36px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #A78BFA, stop:1 #8B5CF6); border-color: #C4B5FD; border-top-color: #DDD6FE; }"
        "QPushButton:pressed { background: #6D28D9; border-color: #5B21B6; padding-top: 8px; }"
    ));
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
    session_only_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:0.5 #0F172A, stop:1 #020617); color: #E2E8F0; font-weight: 700; font-size: 12px; border-radius: 9px; padding: 7px 18px; border: 1.5px solid #334155; border-top: 1.5px solid #475569; border-bottom: 2px solid #020617; min-height: 36px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #334155, stop:1 #1E293B); border-color: #64748B; color: #FFFFFF; }"
        "QPushButton:pressed { background: #0F172A; border-color: #1E293B; padding-top: 8px; }"
    ));
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
    save_profile_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1E293B, stop:0.5 #0F172A, stop:1 #020617); color: #E2E8F0; font-weight: 700; font-size: 12px; border-radius: 9px; padding: 7px 18px; border: 1.5px solid #334155; border-top: 1.5px solid #475569; border-bottom: 2px solid #020617; min-height: 36px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #334155, stop:1 #1E293B); border-color: #64748B; color: #FFFFFF; }"
        "QPushButton:pressed { background: #0F172A; border-color: #1E293B; padding-top: 8px; }"
    ));
    connect(save_profile_btn, &QPushButton::clicked, this, [this]() {
        m_apply_fixes = true;
        m_save_to_ini = true;
        m_result = Result::ApplyAndLaunch;
        accept();
    });
    btn_lay->addWidget(save_profile_btn);

    btn_lay->addStretch();

    auto* cancel_btn = new QPushButton(StormCancelText(), btn_row);
    cancel_btn->setProperty("isCancelButton", true);
    cancel_btn->setCursor(Qt::PointingHandCursor);
    cancel_btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #334155, stop:0.5 #1E293B, stop:1 #0F172A);"
        "    color: #F1F5F9;"
        "    font-weight: 700;"
        "    font-size: 12px;"
        "    min-height: 36px;"
        "    min-width: 115px;"
        "    padding: 7px 20px;"
        "    border-radius: 9px;"
        "    border: 1.5px solid #475569;"
        "    border-top: 1.5px solid #64748B;"
        "    border-bottom: 2px solid #0F172A;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #DC2626, stop:0.5 #B91C1C, stop:1 #991B1B);"
        "    color: #FFFFFF;"
        "    border: 1.5px solid #EF4444;"
        "    border-top: 1.5px solid #F87171;"
        "    border-bottom: 2px solid #7F1D1D;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #991B1B, stop:1 #7F1D1D);"
        "    color: #FEE2E2;"
        "    border: 1.5px solid #991B1B;"
        "    border-top: 1.5px solid #7F1D1D;"
        "    padding-top: 8px;"
        "}"
    ));
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
