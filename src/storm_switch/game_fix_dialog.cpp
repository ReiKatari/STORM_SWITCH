// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <windows.h>
#endif

#include "storm_switch/game_fix_dialog.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <thread>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
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
#include "storm_switch/configuration/shared_widget.h"

enum class GameFixVectorIcon {
    HardwareChip,
    SpeedFast,
    BalancedScale,
    AccurateTarget,
    FactoryReset,
    GraphicsVulkan,
    CpuMemory,
    SyncStability,
    SaveProfile,
    Reset,
    Apply,
    Cancel,
    Copy,
    Info,
};

static QIcon DrawGameFixVectorIcon(GameFixVectorIcon kind, const QColor& color, int size = 16) {
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
    case GameFixVectorIcon::HardwareChip: {
        p.drawRoundedRect(QRectF(s * 0.28, s * 0.28, s * 0.44, s * 0.44), s * 0.06, s * 0.06);
        p.drawRect(QRectF(s * 0.40, s * 0.40, s * 0.20, s * 0.20));
        p.drawLine(QPointF(s * 0.38, s * 0.16), QPointF(s * 0.38, s * 0.28));
        p.drawLine(QPointF(s * 0.50, s * 0.16), QPointF(s * 0.50, s * 0.28));
        p.drawLine(QPointF(s * 0.62, s * 0.16), QPointF(s * 0.62, s * 0.28));
        p.drawLine(QPointF(s * 0.38, s * 0.72), QPointF(s * 0.38, s * 0.84));
        p.drawLine(QPointF(s * 0.50, s * 0.72), QPointF(s * 0.50, s * 0.84));
        p.drawLine(QPointF(s * 0.62, s * 0.72), QPointF(s * 0.62, s * 0.84));
        p.drawLine(QPointF(s * 0.16, s * 0.38), QPointF(s * 0.28, s * 0.38));
        p.drawLine(QPointF(s * 0.16, s * 0.50), QPointF(s * 0.28, s * 0.50));
        p.drawLine(QPointF(s * 0.16, s * 0.62), QPointF(s * 0.28, s * 0.62));
        p.drawLine(QPointF(s * 0.72, s * 0.38), QPointF(s * 0.84, s * 0.38));
        p.drawLine(QPointF(s * 0.72, s * 0.50), QPointF(s * 0.84, s * 0.50));
        p.drawLine(QPointF(s * 0.72, s * 0.62), QPointF(s * 0.84, s * 0.62));
        break;
    }
    case GameFixVectorIcon::SpeedFast: {
        p.drawLine(QPointF(s * 0.20, s * 0.24), QPointF(s * 0.48, s * 0.50));
        p.drawLine(QPointF(s * 0.48, s * 0.50), QPointF(s * 0.20, s * 0.76));
        p.drawLine(QPointF(s * 0.48, s * 0.24), QPointF(s * 0.76, s * 0.50));
        p.drawLine(QPointF(s * 0.76, s * 0.50), QPointF(s * 0.48, s * 0.76));
        break;
    }
    case GameFixVectorIcon::BalancedScale: {
        p.drawLine(QPointF(s * 0.50, s * 0.20), QPointF(s * 0.50, s * 0.80));
        p.drawLine(QPointF(s * 0.30, s * 0.80), QPointF(s * 0.70, s * 0.80));
        p.drawLine(QPointF(s * 0.22, s * 0.32), QPointF(s * 0.78, s * 0.32));
        p.drawLine(QPointF(s * 0.22, s * 0.32), QPointF(s * 0.16, s * 0.54));
        p.drawLine(QPointF(s * 0.22, s * 0.32), QPointF(s * 0.28, s * 0.54));
        p.drawLine(QPointF(s * 0.14, s * 0.54), QPointF(s * 0.30, s * 0.54));
        p.drawLine(QPointF(s * 0.78, s * 0.32), QPointF(s * 0.72, s * 0.54));
        p.drawLine(QPointF(s * 0.78, s * 0.32), QPointF(s * 0.84, s * 0.54));
        p.drawLine(QPointF(s * 0.70, s * 0.54), QPointF(s * 0.86, s * 0.54));
        break;
    }
    case GameFixVectorIcon::AccurateTarget: {
        p.drawEllipse(QRectF(s * 0.20, s * 0.20, s * 0.60, s * 0.60));
        p.drawEllipse(QRectF(s * 0.38, s * 0.38, s * 0.24, s * 0.24));
        p.drawLine(QPointF(s * 0.50, s * 0.12), QPointF(s * 0.50, s * 0.22));
        p.drawLine(QPointF(s * 0.50, s * 0.78), QPointF(s * 0.50, s * 0.88));
        p.drawLine(QPointF(s * 0.12, s * 0.50), QPointF(s * 0.22, s * 0.50));
        p.drawLine(QPointF(s * 0.78, s * 0.50), QPointF(s * 0.88, s * 0.50));
        break;
    }
    case GameFixVectorIcon::FactoryReset: {
        QRectF arc_rect(s * 0.20, s * 0.20, s * 0.60, s * 0.60);
        p.drawArc(arc_rect, 45 * 16, 270 * 16);
        p.drawLine(QPointF(s * 0.74, s * 0.34), QPointF(s * 0.86, s * 0.20));
        p.drawLine(QPointF(s * 0.74, s * 0.34), QPointF(s * 0.60, s * 0.20));
        break;
    }
    case GameFixVectorIcon::GraphicsVulkan: {
        p.drawRoundedRect(QRectF(s * 0.18, s * 0.20, s * 0.64, s * 0.48), s * 0.08, s * 0.08);
        p.drawLine(QPointF(s * 0.38, s * 0.68), QPointF(s * 0.30, s * 0.82));
        p.drawLine(QPointF(s * 0.62, s * 0.68), QPointF(s * 0.70, s * 0.82));
        p.drawLine(QPointF(s * 0.26, s * 0.82), QPointF(s * 0.74, s * 0.82));
        break;
    }
    case GameFixVectorIcon::CpuMemory: {
        p.drawRoundedRect(QRectF(s * 0.16, s * 0.30, s * 0.68, s * 0.40), s * 0.06, s * 0.06);
        p.drawRect(QRectF(s * 0.24, s * 0.38, s * 0.14, s * 0.24));
        p.drawRect(QRectF(s * 0.43, s * 0.38, s * 0.14, s * 0.24));
        p.drawRect(QRectF(s * 0.62, s * 0.38, s * 0.14, s * 0.24));
        break;
    }
    case GameFixVectorIcon::SyncStability: {
        QPainterPath path;
        path.moveTo(s * 0.50, s * 0.18);
        path.lineTo(s * 0.78, s * 0.28);
        path.lineTo(s * 0.78, s * 0.52);
        path.cubicTo(s * 0.78, s * 0.70, s * 0.50, s * 0.84, s * 0.50, s * 0.84);
        path.cubicTo(s * 0.50, s * 0.84, s * 0.22, s * 0.70, s * 0.22, s * 0.52);
        path.lineTo(s * 0.22, s * 0.28);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(s * 0.36, s * 0.50), QPointF(s * 0.46, s * 0.60));
        p.drawLine(QPointF(s * 0.46, s * 0.60), QPointF(s * 0.64, s * 0.38));
        break;
    }
    case GameFixVectorIcon::SaveProfile: {
        p.drawRoundedRect(QRectF(s * 0.20, s * 0.20, s * 0.60, s * 0.60), s * 0.08, s * 0.08);
        p.drawRect(QRectF(s * 0.32, s * 0.50, s * 0.36, s * 0.30));
        break;
    }
    case GameFixVectorIcon::Reset: {
        QRectF arc_rect(s * 0.20, s * 0.20, s * 0.60, s * 0.60);
        p.drawArc(arc_rect, 45 * 16, 270 * 16);
        p.drawLine(QPointF(s * 0.74, s * 0.34), QPointF(s * 0.86, s * 0.20));
        p.drawLine(QPointF(s * 0.74, s * 0.34), QPointF(s * 0.60, s * 0.20));
        break;
    }
    case GameFixVectorIcon::Apply: {
        p.drawLine(QPointF(s * 0.20, s * 0.52), QPointF(s * 0.42, s * 0.74));
        p.drawLine(QPointF(s * 0.42, s * 0.74), QPointF(s * 0.82, s * 0.26));
        break;
    }
    case GameFixVectorIcon::Cancel: {
        p.drawLine(QPointF(s * 0.24, s * 0.24), QPointF(s * 0.76, s * 0.76));
        p.drawLine(QPointF(s * 0.76, s * 0.24), QPointF(s * 0.24, s * 0.76));
        break;
    }
    case GameFixVectorIcon::Copy: {
        p.drawRoundedRect(QRectF(s * 0.32, s * 0.32, s * 0.50, s * 0.50), s * 0.06, s * 0.06);
        p.drawLine(QPointF(s * 0.20, s * 0.66), QPointF(s * 0.20, s * 0.20));
        p.drawLine(QPointF(s * 0.20, s * 0.20), QPointF(s * 0.66, s * 0.20));
        break;
    }
    case GameFixVectorIcon::Info: {
        p.drawEllipse(QRectF(s * 0.20, s * 0.20, s * 0.60, s * 0.60));
        p.drawLine(QPointF(s * 0.50, s * 0.44), QPointF(s * 0.50, s * 0.68));
        p.drawPoint(QPointF(s * 0.50, s * 0.34));
        break;
    }
    }

    return QIcon(pix);
}

GameFixDialog::GameFixDialog(QWidget* parent, u64 title_id_, const QString& game_path_,
                             bool force_show_, Core::System& system_,
                             const QPixmap& game_icon_)
    : QDialog(parent), m_title_id(title_id_), m_game_path(game_path_),
      m_force_show(force_show_), m_system(system_), m_game_icon(game_icon_) {
    // Resolve Title ID if 0
    if (m_title_id == 0 && !m_game_path.isEmpty()) {
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

    if (m_title_id != 0 || !m_game_path.isEmpty()) {
        m_profile = Core::GameFixDatabase::GetProfileByTitleOrPath(m_title_id, m_game_path.toStdString());
        if (m_profile != nullptr && m_title_id == 0) {
            m_title_id = m_profile->title_id;
        }
        if (m_profile == nullptr && m_title_id != 0) {
            m_profile = Core::GameFixDatabase::GetProfile(m_title_id);
        }
    }

    setWindowTitle(StormLang(
        QStringLiteral("STORM SWITCH — Авто-настройка и оптимизация"),
        QStringLiteral("STORM SWITCH — Auto-Configuration & Optimization"),
        QStringLiteral("STORM SWITCH — Auto-Konfiguration & Optimierung"),
        QStringLiteral("STORM SWITCH — Auto-configuration et optimisation")));

    resize(860, 680);
    setMinimumSize(720, 540);
    StormUI::StructureEngine::InstallEscDismiss(this);

    DetectHardwareInfo();
    SetupUI();
}

GameFixDialog::~GameFixDialog() = default;

void GameFixDialog::DetectHardwareInfo() {
    // 1. Logical CPU Cores
    const unsigned int threads = std::thread::hardware_concurrency();
    if (threads > 0) {
        m_cpu_info = StormLang(
            QStringLiteral("%1 логических ядер").arg(threads),
            QStringLiteral("%1 logical cores").arg(threads));
    } else {
        m_cpu_info = StormLang(QStringLiteral("Многоядерный ЦП"), QStringLiteral("Multi-core CPU"));
    }

    // 2. RAM in GB with comma decimal separator
#ifdef _WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem_status)) {
        const double ram_gb = static_cast<double>(mem_status.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
        m_ram_info = QStringLiteral("%1 ГБ").arg(QString::number(ram_gb, 'f', 1).replace(QLatin1Char('.'), QLatin1Char(',')));
    } else {
        m_ram_info = QStringLiteral("16,0 ГБ");
    }
#else
    m_ram_info = QStringLiteral("16,0 ГБ");
#endif

    // 3. Vulkan GPU name via Win32 EnumDisplayDevicesW
#ifdef _WIN32
    DISPLAY_DEVICEW dd;
    std::memset(&dd, 0, sizeof(dd));
    dd.cb = sizeof(dd);
    if (EnumDisplayDevicesW(nullptr, 0, &dd, 0)) {
        m_gpu_info = QString::fromWCharArray(dd.DeviceString).trimmed();
    }
#endif
    if (m_gpu_info.isEmpty()) {
        m_gpu_info = QStringLiteral("Vulkan GPU (DirectX/Vulkan)");
    }
}

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
        return StormLang(QStringLiteral("Синхронизация памяти GPU"), QStringLiteral("Sync Memory Operations"));
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
        return StormLang(QStringLiteral("Асинхронная компиляция шейдеров"), QStringLiteral("Asynchronous Shaders"));
    if (k.find("nvdec_emulation") != std::string::npos)
        return StormLang(QStringLiteral("Эмуляция видеодекодера NVDEC"), QStringLiteral("NVDEC Emulation"));
    if (k.find("astc_recompression") != std::string::npos)
        return StormLang(QStringLiteral("Пересжатие текстур ASTC"), QStringLiteral("ASTC Recompression"));
    if (k.find("accelerate_astc") != std::string::npos || k.find("astc_decode_mode") != std::string::npos)
        return StormLang(QStringLiteral("Декодирование текстур ASTC"), QStringLiteral("ASTC Texture Decoding"));
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

    // CPU & Memory
    if (k.find("cpu_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность эмуляции ЦП"), QStringLiteral("CPU Accuracy"));
    if (k.find("cpu_backend") != std::string::npos)
        return StormLang(QStringLiteral("Бэкенд ЦП (JIT/NCE)"), QStringLiteral("CPU Backend"));
    if (k.find("cpuopt_fastmem") != std::string::npos || k.find("fastmem") != std::string::npos)
        return StormLang(QStringLiteral("Быстрая память ЦП (Fastmem)"), QStringLiteral("Fastmem"));
    if (k.find("cpuopt_fastmem_exclusives") != std::string::npos)
        return StormLang(QStringLiteral("Эксклюзивный доступ Fastmem"), QStringLiteral("Fastmem Exclusives"));
    if (k.find("cpuopt_ignore_memory_aborts") != std::string::npos)
        return StormLang(QStringLiteral("Игнорирование сбоев памяти ЦП"), QStringLiteral("Ignore Memory Aborts"));
    if (k.find("use_multi_core") != std::string::npos)
        return StormLang(QStringLiteral("Многопоточная эмуляция ЦП"), QStringLiteral("Multicore CPU Emulation"));
    if (k.find("memory_layout_mode") != std::string::npos)
        return StormLang(QStringLiteral("Конфигурация памяти DRAM"), QStringLiteral("Memory Layout (DRAM)"));

    // Sync & Engine Stability
    if (k.find("gpu_fence_behavior") != std::string::npos)
        return StormLang(QStringLiteral("Поведение барьеров GPU"), QStringLiteral("GPU Fence Behavior"));
    if (k.find("dma_accuracy") != std::string::npos)
        return StormLang(QStringLiteral("Точность DMA контроллера"), QStringLiteral("DMA Accuracy"));
    if (k.find("eco_thermal_mode") != std::string::npos)
        return StormLang(QStringLiteral("Эко-режим и контроль нагрева"), QStringLiteral("Eco Thermal Mode"));
    if (k.find("use_speed_limit") != std::string::npos)
        return StormLang(QStringLiteral("Ограничение скорости 100%"), QStringLiteral("Speed Limit 100%"));
    if (k.find("use_docked_mode") != std::string::npos)
        return StormLang(QStringLiteral("Режим док-станции (Docked)"), QStringLiteral("Docked Mode"));

    // Fallback formatting
    std::string clean = raw_key;
    const auto slash_pos = clean.find_last_of("\\/");
    if (slash_pos != std::string::npos) {
        clean = clean.substr(slash_pos + 1);
    }
    std::replace(clean.begin(), clean.end(), '_', ' ');
    if (!clean.empty()) {
        clean[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(clean[0])));
    }
    return QString::fromStdString(clean);
}

QString GameFixDialog::FormatSettingValue(const std::string& raw_key, const std::string& val) const {
    const std::string k = raw_key;

    if (k.find("gpu_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Нормальная"), QStringLiteral("Normal"));
        if (val == "1") return StormLang(QStringLiteral("Высокая"), QStringLiteral("High"));
        if (val == "2") return StormLang(QStringLiteral("Экстремальная"), QStringLiteral("Extreme"));
        if (val == "-1") return StormLang(QStringLiteral("Низкая"), QStringLiteral("Low"));
    }
    if (k.find("cpu_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Авто"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точная"), QStringLiteral("Accurate"));
        if (val == "2") return StormLang(QStringLiteral("Небезопасная"), QStringLiteral("Unsafe"));
    }
    if (k.find("cpu_backend") != std::string::npos) {
        if (val == "0") return QStringLiteral("Dynarmic (JIT)");
        if (val == "1") return QStringLiteral("NCE (Native Code)");
    }
    if (k.find("resolution_setup") != std::string::npos) {
        if (val == "0") return QStringLiteral("0.5x (360p/540p)");
        if (val == "1") return QStringLiteral("0.75x (540p/810p)");
        if (val == "2") return QStringLiteral("1x (720p/1080p)");
        if (val == "3") return QStringLiteral("1.5x (1080p/1620p)");
        if (val == "4") return QStringLiteral("2x (1440p/2160p)");
    }
    if (k.find("vsync_mode") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Выкл"), QStringLiteral("Off"));
        if (val == "1") return QStringLiteral("Mailbox");
        if (val == "2") return QStringLiteral("FIFO (Вкл)");
        if (val == "3") return QStringLiteral("FIFO Relaxed");
    }
    if (k.find("scaling_filter") != std::string::npos) {
        if (val == "0") return QStringLiteral("Nearest");
        if (val == "1") return QStringLiteral("Bilinear");
        if (val == "2") return QStringLiteral("Bicubic");
        if (val == "4") return QStringLiteral("AMD FSR");
    }
    if (k.find("anti_aliasing") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Выкл"), QStringLiteral("None"));
        if (val == "1") return QStringLiteral("FXAA");
        if (val == "2") return QStringLiteral("SMAA");
    }
    if (k.find("gpu_fence_behavior") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"));
        if (val == "2") return StormLang(QStringLiteral("Быстро"), QStringLiteral("Fast"));
    }
    if (k.find("dma_accuracy") != std::string::npos) {
        if (val == "0") return StormLang(QStringLiteral("Автоматически"), QStringLiteral("Auto"));
        if (val == "1") return StormLang(QStringLiteral("Точно"), QStringLiteral("Accurate"));
        if (val == "3") return StormLang(QStringLiteral("Быстро"), QStringLiteral("Fast"));
    }

    if (val == "true" || val == "True" || val == "1")
        return StormLang(QStringLiteral("Включено"), QStringLiteral("Enabled"));
    if (val == "false" || val == "False" || val == "0")
        return StormLang(QStringLiteral("Выключено"), QStringLiteral("Disabled"));

    return QString::fromStdString(val);
}

void GameFixDialog::SetupUI() {
    const auto pal = StormTheme::GetThemePalette();
    const QColor acc(pal.accent);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: 'Segoe UI', -apple-system, sans-serif;"
        "}"
        "QGroupBox {"
        "  background-color: %3;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding-top: 14px;"
        "  margin-top: 6px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 0 10px;"
        "  color: %4;"
        "  font-weight: 800;"
        "  font-size: 11px;"
        "  letter-spacing: 0.6px;"
        "}"
        "QScrollArea {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QScrollBar:vertical {"
        "  background: %1;"
        "  width: 8px;"
        "  margin: 2px 0px;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %5;"
        "  min-height: 28px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: %4;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "  background: none;"
        "  border: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QCheckBox {"
        "  color: %2;"
        "  font-size: 11.5px;"
        "  spacing: 8px;"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 4px;"
        "  border: none;"
        "  background-color: %5;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %4;"
        "}"
        "QPushButton {"
        "  background: %5;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 16px;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "  min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "  background: %3;"
        "  color: %4;"
        "  border: none;"
        "}"
        "QPushButton:pressed {"
        "  background: %1;"
        "  border: none;"
        "}"
    ).arg(pal.bg, pal.textPrimary, pal.card, pal.accent, pal.cardAlt));

    auto* root_lay = new QVBoxLayout(this);
    root_lay->setContentsMargins(18, 18, 18, 18);
    root_lay->setSpacing(12);

    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    StormUI::StructureEngine::EnforceScrollMargins(scroll_area, 14);

    auto* scroll_content = new QWidget(scroll_area);
    auto* scroll_lay = new QVBoxLayout(scroll_content);
    scroll_lay->setContentsMargins(0, 0, 0, 0);
    scroll_lay->setSpacing(14);

    // -------------------------------------------------------------------------
    // Группа 1: «Диагностика системы / Профиль игры»
    // -------------------------------------------------------------------------
    auto* grp1 = new QGroupBox(StormLang(
        QStringLiteral("ГРУППА 1: ДИАГНОСТИКА СИСТЕМЫ И ЦЕЛЕВОЙ ПРОФИЛЬ"),
        QStringLiteral("GROUP 1: SYSTEM DIAGNOSTICS & TARGET PROFILE")), scroll_content);
    auto* grp1_lay = new QVBoxLayout(grp1);
    grp1_lay->setContentsMargins(16, 14, 16, 14);
    grp1_lay->setSpacing(12);

    // Hardware pills bar
    auto* hw_bar = new QWidget(grp1);
    auto* hw_lay = new QHBoxLayout(hw_bar);
    hw_lay->setContentsMargins(0, 0, 0, 0);
    hw_lay->setSpacing(10);

    const auto make_hw_pill = [&](const QString& label, const QString& val, GameFixVectorIcon ico) {
        auto* pill = new QWidget(hw_bar);
        pill->setStyleSheet(QStringLiteral(
            "background-color: %1; border: none; border-radius: 8px; padding: 6px 12px;"
        ).arg(pal.surface));
        auto* p_lay = new QHBoxLayout(pill);
        p_lay->setContentsMargins(4, 2, 4, 2);
        p_lay->setSpacing(8);

        auto* p_icon = new QLabel(pill);
        p_icon->setPixmap(DrawGameFixVectorIcon(ico, acc, 14).pixmap(28, 28));
        p_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
        p_lay->addWidget(p_icon);

        auto* t_lay = new QVBoxLayout();
        t_lay->setContentsMargins(0, 0, 0, 0);
        t_lay->setSpacing(1);

        auto* l_lbl = new QLabel(label, pill);
        l_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; font-weight: 700; text-transform: uppercase; border: none; background: transparent;").arg(pal.textSecondary));
        t_lay->addWidget(l_lbl);

        auto* v_lbl = new QLabel(val, pill);
        v_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
        t_lay->addWidget(v_lbl);

        p_lay->addLayout(t_lay);
        return pill;
    };

    hw_lay->addWidget(make_hw_pill(StormLang(QStringLiteral("Процессор"), QStringLiteral("CPU")), m_cpu_info, GameFixVectorIcon::HardwareChip));
    hw_lay->addWidget(make_hw_pill(StormLang(QStringLiteral("ОЗУ системы"), QStringLiteral("RAM")), m_ram_info, GameFixVectorIcon::CpuMemory));
    hw_lay->addWidget(make_hw_pill(StormLang(QStringLiteral("Графика / GPU"), QStringLiteral("GPU")), m_gpu_info, GameFixVectorIcon::GraphicsVulkan), 1);
    grp1_lay->addWidget(hw_bar);

    // Target game or global calibration info
    auto* target_row = new QWidget(grp1);
    auto* target_lay = new QHBoxLayout(target_row);
    target_lay->setContentsMargins(0, 0, 0, 0);
    target_lay->setSpacing(14);

    if (m_title_id != 0 || m_profile != nullptr) {
        auto* cover_label = new QLabel(target_row);
        cover_label->setFixedSize(64, 64);
        cover_label->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

        QPixmap raw_cover = m_game_icon;
        if (raw_cover.isNull() && m_title_id != 0) {
            const auto cache_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list";
            const auto icon_file = cache_dir / "icons" / fmt::format("{:016X}.png", m_title_id);
            const QString ip = QString::fromStdString(Common::FS::PathToUTF8String(icon_file));
            if (QFile::exists(ip)) {
                raw_cover.load(ip);
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
        const int px_sz = static_cast<int>(std::round(64 * dpr));
        QPixmap rounded_cover(px_sz, px_sz);
        rounded_cover.fill(Qt::transparent);

        QPainter cp(&rounded_cover);
        cp.setRenderHint(QPainter::Antialiasing);
        cp.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip_p;
        clip_p.addRoundedRect(0, 0, px_sz, px_sz, static_cast<int>(8 * dpr), static_cast<int>(8 * dpr));
        cp.setClipPath(clip_p);
        if (!raw_cover.isNull()) {
            cp.drawPixmap(0, 0, px_sz, px_sz, raw_cover.scaled(px_sz, px_sz, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            cp.setBrush(QColor(pal.surface));
            cp.setPen(Qt::NoPen);
            cp.drawRoundedRect(0, 0, px_sz, px_sz, static_cast<int>(8 * dpr), static_cast<int>(8 * dpr));
        }
        rounded_cover.setDevicePixelRatio(dpr);
        cover_label->setPixmap(rounded_cover);
        target_lay->addWidget(cover_label);

        auto* ginfo_lay = new QVBoxLayout();
        ginfo_lay->setContentsMargins(0, 0, 0, 0);
        ginfo_lay->setSpacing(4);

        const QString game_title = m_profile ? QString::fromStdString(m_profile->game_name) : QStringLiteral("Игра Nintendo Switch");
        auto* gtitle_lbl = new QLabel(game_title, target_row);
        QFont gtf = gtitle_lbl->font();
        gtf.setBold(true);
        gtf.setPointSize(12);
        gtitle_lbl->setFont(gtf);
        gtitle_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
        ginfo_lay->addWidget(gtitle_lbl);

        auto* gbadge_row = new QHBoxLayout();
        gbadge_row->setContentsMargins(0, 0, 0, 0);
        gbadge_row->setSpacing(8);

        const QString tid_str = QStringLiteral("TID: %1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper();
        auto* tid_btn = new QPushButton(tid_str, target_row);
        tid_btn->setIcon(DrawGameFixVectorIcon(GameFixVectorIcon::Copy, acc, 12));
        tid_btn->setCursor(Qt::PointingHandCursor);
        tid_btn->setToolTip(StormLang(QStringLiteral("Нажмите для копирования Title ID"), QStringLiteral("Click to copy Title ID")));
        connect(tid_btn, &QPushButton::clicked, [this]() {
            QApplication::clipboard()->setText(QStringLiteral("%1").arg(m_title_id, 16, 16, QLatin1Char('0')).toUpper());
        });
        gbadge_row->addWidget(tid_btn);

        auto* prof_status = new QLabel(StormLang(
            QStringLiteral("Профиль проверен STORM SWITCH"),
            QStringLiteral("Verified STORM Profile")), target_row);
        prof_status->setStyleSheet(QStringLiteral(
            "background-color: %1; color: %2; border: none; border-radius: 6px; padding: 4px 10px; font-weight: bold; font-size: 11px;"
        ).arg(pal.surface, pal.accent));
        gbadge_row->addWidget(prof_status);
        gbadge_row->addStretch();

        ginfo_lay->addLayout(gbadge_row);
        target_lay->addLayout(ginfo_lay, 1);
    } else {
        auto* global_icon = new QLabel(target_row);
        global_icon->setPixmap(DrawGameFixVectorIcon(GameFixVectorIcon::HardwareChip, acc, 24).pixmap(48, 48));
        global_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
        target_lay->addWidget(global_icon);

        auto* ginfo_lay = new QVBoxLayout();
        ginfo_lay->setContentsMargins(0, 0, 0, 0);
        ginfo_lay->setSpacing(3);

        auto* gtitle_lbl = new QLabel(StormLang(
            QStringLiteral("Глобальная аппаратная авто-калибровка ПК"),
            QStringLiteral("Global PC Hardware Auto-Calibration")), target_row);
        QFont gtf = gtitle_lbl->font();
        gtf.setBold(true);
        gtf.setPointSize(12);
        gtitle_lbl->setFont(gtf);
        gtitle_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
        ginfo_lay->addWidget(gtitle_lbl);

        auto* gsub_lbl = new QLabel(StormLang(
            QStringLiteral("Автоматический подбор оптимальных параметров эмуляции под конфигурацию вашего процессора, ОЗУ и видеокарты."),
            QStringLiteral("Automatically configures optimal emulation parameters tuned for your CPU, RAM, and graphics card.")), target_row);
        gsub_lbl->setWordWrap(true);
        gsub_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 11.5px; border: none; background: transparent;").arg(pal.textSecondary));
        ginfo_lay->addWidget(gsub_lbl);

        target_lay->addLayout(ginfo_lay, 1);
    }

    grp1_lay->addWidget(target_row);
    scroll_lay->addWidget(grp1);

    // -------------------------------------------------------------------------
    // Группа 2: «Режим автоматической оптимизации»
    // -------------------------------------------------------------------------
    auto* grp2 = new QGroupBox(StormLang(
        QStringLiteral("ГРУППА 2: РЕЖИМ АВТОМАТИЧЕСКОЙ ОПТИМИЗАЦИИ"),
        QStringLiteral("GROUP 2: AUTOMATIC OPTIMIZATION MODE")), scroll_content);
    auto* grp2_lay = new QHBoxLayout(grp2);
    grp2_lay->setContentsMargins(14, 14, 14, 14);
    grp2_lay->setSpacing(10);

    const auto make_mode_btn = [&](const QString& title, const QString& desc, GameFixVectorIcon ico, OptimizationMode mode) {
        auto* btn = new QPushButton(grp2);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);

        auto* b_lay = new QVBoxLayout(btn);
        b_lay->setContentsMargins(12, 12, 12, 12);
        b_lay->setSpacing(4);

        auto* i_lbl = new QLabel(btn);
        i_lbl->setPixmap(DrawGameFixVectorIcon(ico, acc, 16).pixmap(32, 32));
        i_lbl->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
        b_lay->addWidget(i_lbl);

        auto* t_lbl = new QLabel(title, btn);
        QFont btf = t_lbl->font();
        btf.setBold(true);
        btf.setPointSize(11);
        t_lbl->setFont(btf);
        t_lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; border: none; background: transparent;").arg(pal.textPrimary));
        b_lay->addWidget(t_lbl);

        auto* d_lbl = new QLabel(desc, btn);
        d_lbl->setWordWrap(true);
        d_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 10.5px; border: none; background: transparent;").arg(pal.textSecondary));
        b_lay->addWidget(d_lbl);

        connect(btn, &QPushButton::clicked, this, [this, mode]() {
            SelectOptimizationMode(mode);
        });

        grp2_lay->addWidget(btn);
        return btn;
    };

    m_mode_btn_fast = make_mode_btn(
        StormLang(QStringLiteral("Быстрый"), QStringLiteral("Fast")),
        StormLang(QStringLiteral("Максимум FPS, 60 кадров/с, минимальные задержки ввода"), QStringLiteral("Maximum FPS, 60 frames/sec, low input lag")),
        GameFixVectorIcon::SpeedFast,
        OptimizationMode::Fast);

    m_mode_btn_balanced = make_mode_btn(
        StormLang(QStringLiteral("Сбалансированный"), QStringLiteral("Balanced")),
        StormLang(QStringLiteral("Идеальный баланс качества, плавной подачи кадров и 60 FPS"), QStringLiteral("Optimal balance of visuals, frame pacing, and 60 FPS")),
        GameFixVectorIcon::BalancedScale,
        OptimizationMode::Balanced);

    m_mode_btn_accurate = make_mode_btn(
        StormLang(QStringLiteral("Точный"), QStringLiteral("Accurate")),
        StormLang(QStringLiteral("Максимальная точность рендеринга и геометрии без артефактов"), QStringLiteral("Maximum rendering precision without artifacts")),
        GameFixVectorIcon::AccurateTarget,
        OptimizationMode::Accurate);

    m_mode_btn_reset = make_mode_btn(
        StormLang(QStringLiteral("Заводской сброс"), QStringLiteral("Factory Reset")),
        StormLang(QStringLiteral("Сброс всех параметров к безопасным базовым настройкам"), QStringLiteral("Reset all settings to safe default baseline")),
        GameFixVectorIcon::FactoryReset,
        OptimizationMode::FactoryReset);

    m_mode_btn_balanced->setChecked(true);
    scroll_lay->addWidget(grp2);

    // -------------------------------------------------------------------------
    // Группа 3: «Рекомендуемые параметры по категориям»
    // -------------------------------------------------------------------------
    auto* grp3 = new QGroupBox(StormLang(
        QStringLiteral("ГРУППА 3: РЕКОМЕНДУЕМЫЕ ПАРАМЕТРЫ ПО КАТЕГОРИЯМ"),
        QStringLiteral("GROUP 3: RECOMMENDED SETTINGS BY CATEGORY")), scroll_content);
    auto* grp3_lay = new QVBoxLayout(grp3);
    grp3_lay->setContentsMargins(14, 14, 14, 14);
    grp3_lay->setSpacing(12);

    // Sub-card 1: Графика, Vulkan и шейдеры
    auto* cat_gfx_box = new QWidget(grp3);
    cat_gfx_box->setStyleSheet(QStringLiteral("background-color: %1; border: none; border-radius: 8px; padding: 10px;").arg(pal.surface));
    auto* cg_outer = new QVBoxLayout(cat_gfx_box);
    cg_outer->setContentsMargins(10, 8, 10, 8);
    cg_outer->setSpacing(6);

    auto* cg_hdr_lay = new QHBoxLayout();
    auto* cg_icon = new QLabel(cat_gfx_box);
    cg_icon->setPixmap(DrawGameFixVectorIcon(GameFixVectorIcon::GraphicsVulkan, acc, 14).pixmap(28, 28));
    cg_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    cg_hdr_lay->addWidget(cg_icon);

    auto* cg_title = new QLabel(StormLang(
        QStringLiteral("1. Графика, Vulkan и шейдеры"),
        QStringLiteral("1. Graphics, Vulkan & Shaders")), cat_gfx_box);
    cg_title->setStyleSheet(QStringLiteral("color: %1; font-weight: 800; font-size: 11.5px; border: none; background: transparent;").arg(pal.accent));
    cg_hdr_lay->addWidget(cg_title);
    cg_hdr_lay->addStretch();
    cg_outer->addLayout(cg_hdr_lay);

    m_cat_graphics_lay = new QVBoxLayout();
    m_cat_graphics_lay->setContentsMargins(0, 4, 0, 0);
    m_cat_graphics_lay->setSpacing(6);
    cg_outer->addLayout(m_cat_graphics_lay);
    grp3_lay->addWidget(cat_gfx_box);

    // Sub-card 2: Процессор, NCE/JIT и память
    auto* cat_cpu_box = new QWidget(grp3);
    cat_cpu_box->setStyleSheet(QStringLiteral("background-color: %1; border: none; border-radius: 8px; padding: 10px;").arg(pal.surface));
    auto* cc_outer = new QVBoxLayout(cat_cpu_box);
    cc_outer->setContentsMargins(10, 8, 10, 8);
    cc_outer->setSpacing(6);

    auto* cc_hdr_lay = new QHBoxLayout();
    auto* cc_icon = new QLabel(cat_cpu_box);
    cc_icon->setPixmap(DrawGameFixVectorIcon(GameFixVectorIcon::CpuMemory, acc, 14).pixmap(28, 28));
    cc_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    cc_hdr_lay->addWidget(cc_icon);

    auto* cc_title = new QLabel(StormLang(
        QStringLiteral("2. Процессор, NCE/JIT и память"),
        QStringLiteral("2. CPU, NCE/JIT & Memory")), cat_cpu_box);
    cc_title->setStyleSheet(QStringLiteral("color: %1; font-weight: 800; font-size: 11.5px; border: none; background: transparent;").arg(pal.accent));
    cc_hdr_lay->addWidget(cc_title);
    cc_hdr_lay->addStretch();
    cc_outer->addLayout(cc_hdr_lay);

    m_cat_cpu_lay = new QVBoxLayout();
    m_cat_cpu_lay->setContentsMargins(0, 4, 0, 0);
    m_cat_cpu_lay->setSpacing(6);
    cc_outer->addLayout(m_cat_cpu_lay);
    grp3_lay->addWidget(cat_cpu_box);

    // Sub-card 3: Синхронизация и стабильность движка
    auto* cat_sync_box = new QWidget(grp3);
    cat_sync_box->setStyleSheet(QStringLiteral("background-color: %1; border: none; border-radius: 8px; padding: 10px;").arg(pal.surface));
    auto* cs_outer = new QVBoxLayout(cat_sync_box);
    cs_outer->setContentsMargins(10, 8, 10, 8);
    cs_outer->setSpacing(6);

    auto* cs_hdr_lay = new QHBoxLayout();
    auto* cs_icon = new QLabel(cat_sync_box);
    cs_icon->setPixmap(DrawGameFixVectorIcon(GameFixVectorIcon::SyncStability, acc, 14).pixmap(28, 28));
    cs_icon->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    cs_hdr_lay->addWidget(cs_icon);

    auto* cs_title = new QLabel(StormLang(
        QStringLiteral("3. Синхронизация и стабильность движка"),
        QStringLiteral("3. Synchronization & Engine Stability")), cat_sync_box);
    cs_title->setStyleSheet(QStringLiteral("color: %1; font-weight: 800; font-size: 11.5px; border: none; background: transparent;").arg(pal.accent));
    cs_hdr_lay->addWidget(cs_title);
    cs_hdr_lay->addStretch();
    cs_outer->addLayout(cs_hdr_lay);

    m_cat_sync_lay = new QVBoxLayout();
    m_cat_sync_lay->setContentsMargins(0, 4, 0, 0);
    m_cat_sync_lay->setSpacing(6);
    cs_outer->addLayout(m_cat_sync_lay);
    grp3_lay->addWidget(cat_sync_box);

    PopulateProfileData();
    scroll_lay->addWidget(grp3);

    // Known issues resolved
    if (m_profile && (!m_profile->issues_ru.empty() || !m_profile->issues_en.empty())) {
        auto* issues_card = new QGroupBox(StormLang(
            QStringLiteral("ИЗВЕСТНЫЕ ПРОБЛЕМЫ И УСТРАНЯЕМЫЕ СБОИ"),
            QStringLiteral("RESOLVED ISSUES & STABILITY NOTES")), scroll_content);
        auto* issues_lay = new QVBoxLayout(issues_card);
        issues_lay->setContentsMargins(14, 12, 14, 12);

        const QString issues_txt = StormLang(
            QString::fromStdString(m_profile->issues_ru.empty() ? m_profile->issues_en : m_profile->issues_ru),
            QString::fromStdString(m_profile->issues_en.empty() ? m_profile->issues_ru : m_profile->issues_en));

        auto* issues_lbl = new QLabel(issues_txt, issues_card);
        issues_lbl->setWordWrap(true);
        issues_lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 11.5px; line-height: 1.4; border: none; background: transparent;").arg(pal.textSecondary));
        issues_lay->addWidget(issues_lbl);
        scroll_lay->addWidget(issues_card);
    }

    scroll_area->setWidget(scroll_content);
    root_lay->addWidget(scroll_area, 1);

    // -------------------------------------------------------------------------
    // Bottom Bar: Checkbox + Actions
    // -------------------------------------------------------------------------
    auto* footer_box = new QWidget(this);
    auto* footer_lay = new QVBoxLayout(footer_box);
    footer_lay->setContentsMargins(0, 4, 0, 0);
    footer_lay->setSpacing(10);

    if (m_title_id != 0) {
        m_dont_ask_cb = new QCheckBox(StormLang(
            QStringLiteral("Больше не показывать это окно для этой игры (запомнить выбор)"),
            QStringLiteral("Don't ask again for this game (remember choice)")), footer_box);
        footer_lay->addWidget(m_dont_ask_cb);
    }

    auto* btn_row = new QWidget(footer_box);
    auto* btn_lay = new QHBoxLayout(btn_row);
    btn_lay->setContentsMargins(0, 0, 0, 0);
    btn_lay->setSpacing(10);

    auto* save_profile_btn = new QPushButton(StormLang(
        QStringLiteral("Сохранить как профиль"),
        QStringLiteral("Save as Profile")), btn_row);
    save_profile_btn->setIcon(DrawGameFixVectorIcon(GameFixVectorIcon::SaveProfile, QColor(pal.textPrimary), 14));
    save_profile_btn->setCursor(Qt::PointingHandCursor);
    connect(save_profile_btn, &QPushButton::clicked, this, &GameFixDialog::SaveAsSettingsProfile);
    btn_lay->addWidget(save_profile_btn);

    auto* reset_btn = new QPushButton(StormLang(
        QStringLiteral("Сбросить"),
        QStringLiteral("Reset")), btn_row);
    reset_btn->setIcon(DrawGameFixVectorIcon(GameFixVectorIcon::Reset, QColor(pal.textSecondary), 14));
    reset_btn->setCursor(Qt::PointingHandCursor);
    connect(reset_btn, &QPushButton::clicked, this, &GameFixDialog::ResetToDefaults);
    btn_lay->addWidget(reset_btn);

    btn_lay->addStretch();

    auto* apply_btn = new QPushButton(StormLang(
        QStringLiteral("Применить"),
        QStringLiteral("Apply")), btn_row);
    apply_btn->setIcon(DrawGameFixVectorIcon(GameFixVectorIcon::Apply, Qt::white, 14));
    apply_btn->setCursor(Qt::PointingHandCursor);
    apply_btn->setDefault(true);
    apply_btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  font-size: 12px;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 24px;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover { background: %1; }"
        "QPushButton:pressed { background: %2; }"
    ).arg(pal.accent, pal.accentHover));
    connect(apply_btn, &QPushButton::clicked, this, [this]() {
        m_apply_fixes = true;
        m_save_to_ini = true;
        m_result = Result::ApplyAndLaunch;
        accept();
    });
    btn_lay->addWidget(apply_btn);

    auto* cancel_btn = new QPushButton(StormLang(QStringLiteral("Отмена"), QStringLiteral("Cancel")), btn_row);
    cancel_btn->setIcon(DrawGameFixVectorIcon(GameFixVectorIcon::Cancel, QColor(pal.textSecondary), 14));
    cancel_btn->setCursor(Qt::PointingHandCursor);
    connect(cancel_btn, &QPushButton::clicked, this, [this]() {
        m_result = Result::Cancel;
        reject();
    });
    btn_lay->addWidget(cancel_btn);

    footer_lay->addWidget(btn_row);
    root_lay->addWidget(footer_box);
}

void GameFixDialog::PopulateProfileData() {
    m_fix_items.clear();

    const auto categorize_key = [](const std::string& k) {
        if (k.find("gpu") != std::string::npos ||
            k.find("shader") != std::string::npos ||
            k.find("astc") != std::string::npos ||
            k.find("resolution") != std::string::npos ||
            k.find("scaling") != std::string::npos ||
            k.find("anti_aliasing") != std::string::npos ||
            k.find("anisotrop") != std::string::npos ||
            k.find("bloom") != std::string::npos ||
            k.find("vulkan") != std::string::npos ||
            k.find("nvdec") != std::string::npos ||
            k.find("aspect") != std::string::npos ||
            k.find("dyna_state") != std::string::npos ||
            k.find("flushing") != std::string::npos) {
            return 1; // Graphics
        }
        if (k.find("cpu") != std::string::npos ||
            k.find("fastmem") != std::string::npos ||
            k.find("memory") != std::string::npos ||
            k.find("core") != std::string::npos) {
            return 2; // CPU & Memory
        }
        return 3; // Sync & Stability
    };

    const auto add_item = [this, &categorize_key](const std::string& key, const std::string& target_val) {
        FixItem item;
        item.raw_key = key;
        item.keys.push_back(key);

        const QString loc_label = GetLocalizedSettingLabel(key);
        const QString fmt_val = FormatSettingValue(key, target_val);

        const int cat = categorize_key(key);
        QVBoxLayout* target_lay = (cat == 1) ? m_cat_graphics_lay
                                : (cat == 2) ? m_cat_cpu_lay
                                             : m_cat_sync_lay;

        item.checkbox = new QCheckBox(QStringLiteral("%1  →  %2").arg(loc_label, fmt_val), this);
        item.checkbox->setChecked(true);
        target_lay->addWidget(item.checkbox);

        m_fix_items.push_back(item);
    };

    if (m_profile && !m_profile->ini_settings.empty()) {
        for (const auto& [key, val] : m_profile->ini_settings) {
            add_item(key, val);
        }
    } else {
        // Hardware auto-calibration baseline
        add_item("renderer_settings/gpu_accuracy", "0");
        add_item("renderer_settings/use_asynchronous_shaders", "true");
        add_item("renderer_settings/use_disk_shader_cache", "true");
        add_item("renderer_settings/use_asynchronous_gpu_emulation", "true");
        add_item("renderer_settings/async_presentation", "true");
        add_item("renderer_settings/use_reactive_flushing", "true");
        add_item("renderer_settings/resolution_setup", "2");
        add_item("renderer_settings/anti_aliasing", "1");

        add_item("cpu_settings/cpu_accuracy", "0");
        add_item("cpu_settings/cpu_backend", "0");
        add_item("cpu_settings/cpuopt_fastmem", "true");
        add_item("cpu_settings/cpuopt_fastmem_exclusives", "true");
        add_item("cpu_settings/use_multi_core", "true");

        add_item("renderer_settings/vsync_mode", "2");
        add_item("renderer_settings/early_release_fences", "true");
        add_item("renderer_settings/sync_memory_operations", "true");
        add_item("renderer_settings/gpu_fence_behavior", "0");
        add_item("renderer_settings/use_speed_limit", "true");
    }
}

void GameFixDialog::SelectOptimizationMode(OptimizationMode mode) {
    m_current_mode = mode;

    m_mode_btn_fast->setChecked(mode == OptimizationMode::Fast);
    m_mode_btn_balanced->setChecked(mode == OptimizationMode::Balanced);
    m_mode_btn_accurate->setChecked(mode == OptimizationMode::Accurate);
    m_mode_btn_reset->setChecked(mode == OptimizationMode::FactoryReset);

    // Apply adjustments to checkboxes in Group 3 based on mode
    for (auto& item : m_fix_items) {
        if (!item.checkbox) continue;
        const std::string& k = item.raw_key;

        if (mode == OptimizationMode::Fast) {
            if (k.find("gpu_accuracy") != std::string::npos) {
                item.checkbox->setText(QStringLiteral("%1  →  %2").arg(GetLocalizedSettingLabel(k), FormatSettingValue(k, "-1")));
                item.checkbox->setChecked(true);
            } else if (k.find("sync_memory_operations") != std::string::npos ||
                       k.find("use_reactive_flushing") != std::string::npos) {
                item.checkbox->setChecked(false);
            } else {
                item.checkbox->setChecked(true);
            }
        } else if (mode == OptimizationMode::Balanced) {
            if (k.find("gpu_accuracy") != std::string::npos) {
                item.checkbox->setText(QStringLiteral("%1  →  %2").arg(GetLocalizedSettingLabel(k), FormatSettingValue(k, "0")));
            }
            item.checkbox->setChecked(true);
        } else if (mode == OptimizationMode::Accurate) {
            if (k.find("gpu_accuracy") != std::string::npos) {
                item.checkbox->setText(QStringLiteral("%1  →  %2").arg(GetLocalizedSettingLabel(k), FormatSettingValue(k, "1")));
            } else if (k.find("resolution_setup") != std::string::npos) {
                item.checkbox->setText(QStringLiteral("%1  →  %2").arg(GetLocalizedSettingLabel(k), FormatSettingValue(k, "4")));
            }
            item.checkbox->setChecked(true);
        } else if (mode == OptimizationMode::FactoryReset) {
            item.checkbox->setChecked(false);
        }
    }
}

void GameFixDialog::ResetToDefaults() {
    SelectOptimizationMode(OptimizationMode::Balanced);
}

void GameFixDialog::SaveAsSettingsProfile() {
    bool ok = false;
    const QString def_name = m_profile ? QString::fromStdString(m_profile->game_name) : QStringLiteral("Auto-Config Profile");
    const QString name = QInputDialog::getText(
        this,
        StormLang(QStringLiteral("Сохранить как профиль"), QStringLiteral("Save as Profile")),
        StormLang(QStringLiteral("Введите название профиля:"), QStringLiteral("Enter profile name:")),
        QLineEdit::Normal,
        def_name,
        &ok);

    if (!ok || name.trimmed().isEmpty()) return;

    const qint64 ts = QDateTime::currentMSecsSinceEpoch();
    const QString id = QStringLiteral("autoconfig_%1").arg(ts);

    QJsonObject root;
    root[QStringLiteral("id")] = id;
    root[QStringLiteral("name")] = name.trimmed();
    root[QStringLiteral("description")] = StormLang(
        QStringLiteral("Профиль автоматической оптимизации STORM SWITCH"),
        QStringLiteral("STORM SWITCH Auto-Configuration Profile"));
    root[QStringLiteral("version")] = QStringLiteral("17.1.0");
    root[QStringLiteral("timestamp")] = static_cast<double>(ts);

    QJsonObject bools;
    QJsonObject ints;

    for (const auto& item : m_fix_items) {
        if (!item.checkbox || !item.checkbox->isChecked()) continue;
        const std::string& k = item.raw_key;
        if (k.find("use_asynchronous_shaders") != std::string::npos) bools[QStringLiteral("use_asynchronous_shaders")] = true;
        if (k.find("use_disk_shader_cache") != std::string::npos) bools[QStringLiteral("use_disk_shader_cache")] = true;
        if (k.find("use_asynchronous_gpu_emulation") != std::string::npos) bools[QStringLiteral("use_asynchronous_gpu_emulation")] = true;
        if (k.find("async_presentation") != std::string::npos) bools[QStringLiteral("async_presentation")] = true;
        if (k.find("early_release_fences") != std::string::npos) bools[QStringLiteral("early_release_fences")] = true;
        if (k.find("sync_memory_operations") != std::string::npos) bools[QStringLiteral("sync_memory_operations")] = true;
        if (k.find("use_reactive_flushing") != std::string::npos) bools[QStringLiteral("use_reactive_flushing")] = true;

        if (k.find("gpu_accuracy") != std::string::npos) {
            ints[QStringLiteral("gpu_accuracy")] = (m_current_mode == OptimizationMode::Fast) ? -1 : (m_current_mode == OptimizationMode::Accurate ? 1 : 0);
        }
        if (k.find("resolution_setup") != std::string::npos) {
            ints[QStringLiteral("resolution_setup")] = (m_current_mode == OptimizationMode::Accurate) ? 4 : 2;
        }
    }

    root[QStringLiteral("booleans")] = bools;
    root[QStringLiteral("integers")] = ints;

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

        QMessageBox::information(this, StormLang(
            QStringLiteral("Профиль сохранён"),
            QStringLiteral("Profile Saved")),
            StormLang(
                QStringLiteral("Профиль оптимизации «%1» успешно сохранён в каталог профилей.").arg(name.trimmed()),
                QStringLiteral("Optimization profile '%1' has been saved successfully.").arg(name.trimmed())));
    }
}

GameFixDialog::Result GameFixDialog::Execute() {
    if (!m_profile && m_title_id == 0 && m_game_path.isEmpty()) {
        exec();
        return m_result;
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
            if (m_profile) {
                Core::GameFixDatabase::ApplyProfileDirectly(m_title_id, m_game_path.toStdString());
            } else {
                // Apply global settings
                Settings::UpdateGPUAccuracy();
                Settings::UpdateRescalingInfo();
                ConfigurationShared::ReloadAllActiveWidgets();
                ConfigurationShared::NotifyGlobalSettingChanged();
                Settings::LogSettings();
                m_system.ApplySettings();
            }
        }
    } else if (m_result == Result::LaunchWithoutChanges) {
        Core::GameFixDatabase::SetFixesEnabled(false);
    }

    return m_result;
}
