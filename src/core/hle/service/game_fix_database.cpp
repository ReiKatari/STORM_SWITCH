// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/game_fix_database.h"
#include "core/hle/service/game_fix_profiles.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include "common/cityhash.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/settings.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace Core {


namespace {
u64 GetHostPhysicalRamBytes() {
#ifdef _WIN32
    MEMORYSTATUSEX mem_info{};
    mem_info.dwLength = sizeof(mem_info);
    if (GlobalMemoryStatusEx(&mem_info)) {
        return static_cast<u64>(mem_info.ullTotalPhys);
    }
    return 0;
#elif defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return static_cast<u64>(pages) * static_cast<u64>(page_size);
    }
    return 0;
#else
    return 0;
#endif
}

bool HasAtLeast7GbHostRam() {
    const u64 total_ram_bytes = GetHostPhysicalRamBytes();
    if (total_ram_bytes > 0) {
        return total_ram_bytes >= (7ULL * 1024 * 1024 * 1024);
    }
    return true;
}

static std::string s_detected_vendor = "other";
static u64 s_last_applied_tid = 0;

bool HasAtLeast5GbHostRam() {
    const u64 total_ram_bytes = GetHostPhysicalRamBytes();
    if (total_ram_bytes > 0) {
        return total_ram_bytes >= (5200ULL * 1024 * 1024);
    }
    return true;
}
} // namespace






static const std::unordered_map<std::string, std::string> s_baseline_ini = {
    // 1-15: Графика и видео
    {"Renderer\\gpu_accuracy", "2"},
    {"Renderer\\nvdec_emulation", "2"},
    {"Renderer\\accelerate_astc", "1"},
    {"Renderer\\astc_recompression", "0"},
    {"Renderer\\use_asynchronous_shaders", "true"},
    {"Renderer\\use_disk_shader_cache", "true"},
    {"Renderer\\use_fast_gpu_time", "false"},
    {"Renderer\\async_presentation", "true"},
    {"Renderer\\use_reactive_flushing", "false"},
    {"Renderer\\vram_garbage_collection", "false"},
    {"Renderer\\early_release_fences", "false"},
    {"Renderer\\gpu_fence_behavior", "0"},
    {"Renderer\\dma_accuracy", "0"},

    // 16-19: Процессор и память
    {"Cpu\\cpu_accuracy", "0"},
    {"Cpu\\cpuopt_fastmem", "true"},
    {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
    {"System\\memory_layout_mode", "0"},

    // 20: Система и сеть
    {"System\\airplane_mode", "false"},
    {"Services\\airplane_mode", "false"},
    {"Network\\airplane_mode", "false"},

    // Дополнительные флаги стабильности Vulkan
    {"Renderer\\enable_compute_pipelines", "true"},
    {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
    {"Renderer\\enable_gpu_buffer_readback", "false"}
};

static std::string GetSetting(const std::unordered_map<std::string, std::string>& settings, const std::string& key, const std::string& def) {
    auto it = settings.find(key);
    if (it != settings.end()) {
        return it->second;
    }
    return def;
}

static std::string BuildFixesRu(const std::unordered_map<std::string, std::string>& settings) {
    std::string out;

    // Секция 1: Графика и видео (15 пунктов)
    out += "🎮 <b>Графика и видео:</b>\n";

    // 1. Графический API
    out += "• <b>Графический API:</b> Vulkan (наивысшая производительность и стабильность)\n";

    // 2. Точность ГПУ
    const auto gpu_acc = GetSetting(settings, "Renderer\\gpu_accuracy", "1");
    if (gpu_acc == "0") {
        out += "• <b>Точность ГПУ:</b> Низкая (максимальная скорость)\n";
    } else if (gpu_acc == "1") {
        out += "• <b>Точность ГПУ:</b> Обычная (максимальная кадровая частота)\n";
    } else if (gpu_acc == "2") {
        out += "• <b>Точность ГПУ:</b> Высокая (стабильная геометрия и Z-буфер)\n";
    } else {
        out += "• <b>Точность ГПУ:</b> Экстремальная (максимальная точность расчетов)\n";
    }

    // 3. Декодирование видео NVDEC
    const auto nvdec = GetSetting(settings, "Renderer\\nvdec_emulation", "2");
    if (nvdec == "0") {
        out += "• <b>Декодирование видео NVDEC:</b> Отключено (пропуск проблемных видеопотоков)\n";
    } else if (nvdec == "1") {
        out += "• <b>Декодирование видео NVDEC:</b> ЦП (программный декодер FFmpeg устраняет зависания)\n";
    } else if (nvdec == "3") {
        out += "• <b>Декодирование видео NVDEC:</b> Гибридное (аппаратное с ЦП-подстраховкой)\n";
    } else {
        out += "• <b>Декодирование видео NVDEC:</b> ГПУ (аппаратное декодирование видеокадров)\n";
    }

    // 4. Декодирование текстур ASTC
    const auto astc_dec = GetSetting(settings, "Renderer\\accelerate_astc", "1");
    if (astc_dec == "0") {
        out += "• <b>Декодирование текстур ASTC:</b> ЦП (декодирование силами процессора)\n";
    } else if (astc_dec == "2") {
        out += "• <b>Декодирование текстур ASTC:</b> ЦП (Асинхронно, декодирование по требованию)\n";
    } else if (astc_dec == "3") {
        out += "• <b>Декодирование текстур ASTC:</b> Гибридное (оптимальное распределение нагрузки)\n";
    } else {
        out += "• <b>Декодирование текстур ASTC:</b> ГПУ (вычислительные шейдеры)\n";
    }

    // 5. Пересжатие текстур ASTC
    const auto astc_rec = GetSetting(settings, "Renderer\\astc_recompression", "0");
    if (astc_rec == "1") {
        out += "• <b>Пересжатие текстур ASTC:</b> BC1 (низкое качество, экономия памяти)\n";
    } else if (astc_rec == "2") {
        out += "• <b>Пересжатие текстур ASTC:</b> BC3 (среднее качество, баланс)\n";
    } else if (astc_rec == "3") {
        out += "• <b>Пересжатие текстур ASTC:</b> BC5 (высокое качество карт нормалей)\n";
    } else {
        out += "• <b>Пересжатие текстур ASTC:</b> Без сжатия (оригинальное качество текстур)\n";
    }

    // 6. Асинхронные шейдеры
    const auto async_shaders = GetSetting(settings, "Renderer\\use_asynchronous_shaders", "true");
    if (async_shaders == "false" || async_shaders == "0") {
        out += "• <b>Асинхронная компиляция шейдеров:</b> Отключено (синхронная сборка пайплайнов)\n";
    } else {
        out += "• <b>Асинхронная компиляция шейдеров:</b> Включено (устранение микрофризов)\n";
    }

    // 7. Дисковый кэш шейдеров
    const auto disk_cache = GetSetting(settings, "Renderer\\use_disk_shader_cache", "true");
    if (disk_cache == "false" || disk_cache == "0") {
        out += "• <b>Дисковый кэш шейдеров:</b> Отключено (шейдеры собираются заново)\n";
    } else {
        out += "• <b>Дисковый кэш шейдеров:</b> Включено (быстрая повторная загрузка сцен)\n";
    }

    // 8. Быстрое время ГПУ
    const auto fast_gpu_time = GetSetting(settings, "Renderer\\use_fast_gpu_time", "false");
    if (fast_gpu_time == "true" || fast_gpu_time == "1") {
        out += "• <b>Быстрое время ГПУ:</b> Включено (предотвращает дроп динамического разрешения)\n";
    } else {
        out += "• <b>Быстрое время ГПУ:</b> Отключено (стандартная синхронизация времени)\n";
    }

    // 9. Асинхронная презентация
    const auto async_pres = GetSetting(settings, "Renderer\\async_presentation", "true");
    if (async_pres == "false" || async_pres == "0") {
        out += "• <b>Асинхронный вывод кадров:</b> Отключено (синхронный кадровый пейсинг)\n";
    } else {
        out += "• <b>Асинхронный вывод кадров:</b> Включено (плавный вывод кадрового буфера)\n";
    }

    // 10. Синхронизация операций памяти
    const auto sync_mem = GetSetting(settings, "Renderer\\sync_memory_operations", "false");
    if (sync_mem == "true" || sync_mem == "1") {
        out += "• <b>Синхронизация операций памяти:</b> Включено (целостность буферов)\n";
    } else {
        out += "• <b>Синхронизация операций памяти:</b> Отключено (высокая пропускная способность)\n";
    }

    // 11. Реактивный сброс памяти
    const auto react_flush = GetSetting(settings, "Renderer\\use_reactive_flushing", "false");
    if (react_flush == "true" || react_flush == "1") {
        out += "• <b>Реактивный сброс памяти:</b> Включено (своевременный сброс буферов)\n";
    } else {
        out += "• <b>Реактивный сброс памяти:</b> Отключено (устранение задержек рендеринга)\n";
    }

    // 12. Сборщик мусора VRAM
    const auto vram_gc = GetSetting(settings, "Renderer\\vram_garbage_collection", "false");
    if (vram_gc == "true" || vram_gc == "1") {
        out += "• <b>Сборщик мусора VRAM:</b> Включено (периодическое высвобождение видеопамяти)\n";
    } else {
        out += "• <b>Сборщик мусора VRAM:</b> Отключено (устраняет просадки и задержки 200 мс)\n";
    }

    // 13. Досрочное освобождение фенсов
    const auto early_fences = GetSetting(settings, "Renderer\\early_release_fences", "false");
    if (early_fences == "true" || early_fences == "1") {
        out += "• <b>Досрочное освобождение фенсов:</b> Включено (снижение латентности команд)\n";
    } else {
        out += "• <b>Досрочное освобождение фенсов:</b> Отключено (стабильная синхронизация очереди)\n";
    }

    // 14. Поведение барьеров ГПУ
    const auto fence_beh = GetSetting(settings, "Renderer\\gpu_fence_behavior", "0");
    if (fence_beh == "1") {
        out += "• <b>Поведение барьеров ГПУ:</b> Принудительный сброс (Flush)\n";
    } else if (fence_beh == "2") {
        out += "• <b>Поведение барьеров ГПУ:</b> Досрочное (Early)\n";
    } else {
        out += "• <b>Поведение барьеров ГПУ:</b> По умолчанию (Default)\n";
    }

    // 15. Точность DMA
    const auto dma_acc = GetSetting(settings, "Renderer\\dma_accuracy", "0");
    if (dma_acc == "1") {
        out += "• <b>Точность DMA:</b> Обычная (Normal, точная передача блоков памяти)\n";
    } else {
        out += "• <b>Точность DMA:</b> Быстрая (Fast, максимальная скорость передачи)\n";
    }

    // Секция 2: Процессор и память (4 пункта)
    out += "\n⚙️ <b>Процессор и память:</b>\n";

    // 16. Точность ЦП
    const auto cpu_acc = GetSetting(settings, "Cpu\\cpu_accuracy", "0");
    if (cpu_acc == "1") {
        out += "• <b>Точность ЦП:</b> Точный (Accurate, исключение рассинхронизации)\n";
    } else if (cpu_acc == "2") {
        out += "• <b>Точность ЦП:</b> Небезопасный (Unsafe, максимальный разгон)\n";
    } else {
        out += "• <b>Точность ЦП:</b> Авто (Auto, автоматическая адаптация Dynarmic)\n";
    }

    // 17. Быстрая память (Fastmem)
    const auto fastmem = GetSetting(settings, "Cpu\\cpuopt_fastmem", "true");
    if (fastmem == "false" || fastmem == "0") {
        out += "• <b>Быстрая память (Fastmem):</b> Отключено (программный контроль памяти)\n";
    } else {
        out += "• <b>Быстрая память (Fastmem):</b> Включено (прямой доступ Host MMU)\n";
    }

    // 18. Игнорировать прерывания памяти
    const auto ign_aborts = GetSetting(settings, "Cpu\\cpuopt_ignore_memory_aborts", "true");
    if (ign_aborts == "false" || ign_aborts == "0") {
        out += "• <b>Игнорировать прерывания памяти:</b> Отключено (стандартная обработка сбоев)\n";
    } else {
        out += "• <b>Игнорировать прерывания памяти:</b> Включено (защита от крашей за границами буфера)\n";
    }

    // 19. Конфигурация памяти DRAM
    const auto mem_layout = GetSetting(settings, "System\\memory_layout_mode", GetSetting(settings, "Core\\memory_layout_mode", "0"));
    if (mem_layout == "2") {
        out += "• <b>Конфигурация памяти DRAM:</b> Экстремальная 8 ГБ (защита от вылетов OOM)\n";
    } else if (mem_layout == "1") {
        out += "• <b>Конфигурация памяти DRAM:</b> Расширенная 6 ГБ (устраняет вылеты при утечках)\n";
    } else {
        out += "• <b>Конфигурация памяти DRAM:</b> Стандартная 4 ГБ (оригинальный объем памяти Switch)\n";
    }

    // Секция 3: Система и сеть (1 пункт)
    out += "\n🌐 <b>Система и сеть:</b>\n";

    // 20. Режим «В самолете»
    const auto airplane = GetSetting(settings, "System\\airplane_mode", "false");
    if (airplane == "true" || airplane == "1") {
        out += "• <b>Режим «В самолете»:</b> Включено (предотвращает дедлоки сетевых сокетов)\n";
    } else {
        out += "• <b>Режим «В самолете»:</b> Отключено (сетевые интерфейсы активны)\n";
    }

    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }

    return out;
}

static std::string BuildFixesEn(const std::unordered_map<std::string, std::string>& settings) {
    std::string out;

    // Section 1: Graphics and Video (15 items)
    out += "🎮 <b>Graphics and Video:</b>\n";

    // 1. Graphics API
    out += "• <b>Graphics API:</b> Vulkan (highest performance and stability)\n";

    // 2. GPU Accuracy
    const auto gpu_acc = GetSetting(settings, "Renderer\\gpu_accuracy", "1");
    if (gpu_acc == "0") {
        out += "• <b>GPU Accuracy:</b> Low (maximum speed)\n";
    } else if (gpu_acc == "1") {
        out += "• <b>GPU Accuracy:</b> Normal (maximum framerate)\n";
    } else if (gpu_acc == "2") {
        out += "• <b>GPU Accuracy:</b> High (stable geometry and Z-buffer)\n";
    } else {
        out += "• <b>GPU Accuracy:</b> Extreme (maximum calculation precision)\n";
    }

    // 3. NVDEC Video Emulation
    const auto nvdec = GetSetting(settings, "Renderer\\nvdec_emulation", "2");
    if (nvdec == "0") {
        out += "• <b>NVDEC Video Emulation:</b> Disabled (bypass problematic video streams)\n";
    } else if (nvdec == "1") {
        out += "• <b>NVDEC Video Emulation:</b> CPU (software FFmpeg decoder prevents cutscene freezes)\n";
    } else if (nvdec == "3") {
        out += "• <b>NVDEC Video Emulation:</b> Hybrid (hardware decoding with CPU fallback)\n";
    } else {
        out += "• <b>NVDEC Video Emulation:</b> GPU (hardware video decoding)\n";
    }

    // 4. ASTC Texture Decoding
    const auto astc_dec = GetSetting(settings, "Renderer\\accelerate_astc", "1");
    if (astc_dec == "0") {
        out += "• <b>ASTC Texture Decoding:</b> CPU (software processor decoding)\n";
    } else if (astc_dec == "2") {
        out += "• <b>ASTC Texture Decoding:</b> CPU Asynchronous (demand decoding)\n";
    } else if (astc_dec == "3") {
        out += "• <b>ASTC Texture Decoding:</b> Hybrid (optimal workload distribution)\n";
    } else {
        out += "• <b>ASTC Texture Decoding:</b> GPU (compute shaders)\n";
    }

    // 5. ASTC Texture Recompression
    const auto astc_rec = GetSetting(settings, "Renderer\\astc_recompression", "0");
    if (astc_rec == "1") {
        out += "• <b>ASTC Texture Recompression:</b> BC1 (low quality, maximum RAM savings)\n";
    } else if (astc_rec == "2") {
        out += "• <b>ASTC Texture Recompression:</b> BC3 (medium quality, balanced)\n";
    } else if (astc_rec == "3") {
        out += "• <b>ASTC Texture Recompression:</b> BC5 (high quality normal maps)\n";
    } else {
        out += "• <b>ASTC Texture Recompression:</b> Uncompressed (original texture fidelity)\n";
    }

    // 6. Asynchronous Shaders
    const auto async_shaders = GetSetting(settings, "Renderer\\use_asynchronous_shaders", "true");
    if (async_shaders == "false" || async_shaders == "0") {
        out += "• <b>Asynchronous Shaders:</b> Disabled (synchronous pipeline compilation)\n";
    } else {
        out += "• <b>Asynchronous Shaders:</b> Enabled (eliminates ingame stuttering)\n";
    }

    // 7. Disk Shader Cache
    const auto disk_cache = GetSetting(settings, "Renderer\\use_disk_shader_cache", "true");
    if (disk_cache == "false" || disk_cache == "0") {
        out += "• <b>Disk Shader Cache:</b> Disabled (shaders recompiled on each run)\n";
    } else {
        out += "• <b>Disk Shader Cache:</b> Enabled (fast subsequent scene loading)\n";
    }

    // 8. Fast GPU Time
    const auto fast_gpu_time = GetSetting(settings, "Renderer\\use_fast_gpu_time", "false");
    if (fast_gpu_time == "true" || fast_gpu_time == "1") {
        out += "• <b>Fast GPU Time:</b> Enabled (prevents dynamic resolution drops)\n";
    } else {
        out += "• <b>Fast GPU Time:</b> Disabled (standard timing synchronization)\n";
    }

    // 9. Async Presentation
    const auto async_pres = GetSetting(settings, "Renderer\\async_presentation", "true");
    if (async_pres == "false" || async_pres == "0") {
        out += "• <b>Async Presentation:</b> Disabled (synchronous frame presentation)\n";
    } else {
        out += "• <b>Async Presentation:</b> Enabled (smooth frame pacing)\n";
    }

    // 10. Sync Memory Operations
    const auto sync_mem = GetSetting(settings, "Renderer\\sync_memory_operations", "false");
    if (sync_mem == "true" || sync_mem == "1") {
        out += "• <b>Sync Memory Operations:</b> Enabled (buffer data consistency)\n";
    } else {
        out += "• <b>Sync Memory Operations:</b> Disabled (high bandwidth throughput)\n";
    }

    // 11. Reactive Flushing
    const auto react_flush = GetSetting(settings, "Renderer\\use_reactive_flushing", "false");
    if (react_flush == "true" || react_flush == "1") {
        out += "• <b>Reactive Flushing:</b> Enabled (prompt render target flushing)\n";
    } else {
        out += "• <b>Reactive Flushing:</b> Disabled (prevents rendering pipeline stalls)\n";
    }

    // 12. VRAM Garbage Collection
    const auto vram_gc = GetSetting(settings, "Renderer\\vram_garbage_collection", "false");
    if (vram_gc == "true" || vram_gc == "1") {
        out += "• <b>VRAM Garbage Collection:</b> Enabled (periodic video memory reclamation)\n";
    } else {
        out += "• <b>VRAM Garbage Collection:</b> Disabled (eliminates FPS drops and 200ms queue stalls)\n";
    }

    // 13. Early Release Fences
    const auto early_fences = GetSetting(settings, "Renderer\\early_release_fences", "false");
    if (early_fences == "true" || early_fences == "1") {
        out += "• <b>Early Release Fences:</b> Enabled (lower command latency)\n";
    } else {
        out += "• <b>Early Release Fences:</b> Disabled (stable queue synchronization)\n";
    }

    // 14. GPU Fence Behavior
    const auto fence_beh = GetSetting(settings, "Renderer\\gpu_fence_behavior", "0");
    if (fence_beh == "1") {
        out += "• <b>GPU Fence Behavior:</b> Flush (immediate flush)\n";
    } else if (fence_beh == "2") {
        out += "• <b>GPU Fence Behavior:</b> Early (early signaling)\n";
    } else {
        out += "• <b>GPU Fence Behavior:</b> Default (driver recommended)\n";
    }

    // 15. DMA Accuracy
    const auto dma_acc = GetSetting(settings, "Renderer\\dma_accuracy", "0");
    if (dma_acc == "1") {
        out += "• <b>DMA Accuracy:</b> Normal (accurate memory block transfer)\n";
    } else {
        out += "• <b>DMA Accuracy:</b> Fast (maximum transfer speed)\n";
    }

    // Section 2: CPU and Memory (4 items)
    out += "\n⚙️ <b>CPU and Memory:</b>\n";

    // 16. CPU Accuracy
    const auto cpu_acc = GetSetting(settings, "Cpu\\cpu_accuracy", "0");
    if (cpu_acc == "1") {
        out += "• <b>CPU Accuracy:</b> Accurate (enhanced precision to prevent logic desynchronization)\n";
    } else if (cpu_acc == "2") {
        out += "• <b>CPU Accuracy:</b> Unsafe (maximum throughput with relaxed checks)\n";
    } else {
        out += "• <b>CPU Accuracy:</b> Auto (dynamic JIT adaptation)\n";
    }

    // 17. Fastmem
    const auto fastmem = GetSetting(settings, "Cpu\\cpuopt_fastmem", "true");
    if (fastmem == "false" || fastmem == "0") {
        out += "• <b>Fastmem (Host MMU):</b> Disabled (software memory control)\n";
    } else {
        out += "• <b>Fastmem (Host MMU):</b> Enabled (direct hardware page mapping)\n";
    }

    // 18. Ignore Memory Aborts
    const auto ign_aborts = GetSetting(settings, "Cpu\\cpuopt_ignore_memory_aborts", "true");
    if (ign_aborts == "false" || ign_aborts == "0") {
        out += "• <b>Ignore Memory Aborts:</b> Disabled (standard memory abort handling)\n";
    } else {
        out += "• <b>Ignore Memory Aborts:</b> Enabled (prevents crashes on out-of-bounds guest memory accesses)\n";
    }

    // 19. Memory Layout
    const auto mem_layout = GetSetting(settings, "System\\memory_layout_mode", GetSetting(settings, "Core\\memory_layout_mode", "0"));
    if (mem_layout == "2") {
        out += "• <b>DRAM Memory Layout:</b> 8GB Extreme (prevents out-of-memory engine crashes)\n";
    } else if (mem_layout == "1") {
        out += "• <b>DRAM Memory Layout:</b> 6GB Expanded (prevents memory exhaustion during long sessions)\n";
    } else {
        out += "• <b>DRAM Memory Layout:</b> 4GB Standard (original Switch console memory layout)\n";
    }

    // Section 3: System and Network (1 item)
    out += "\n🌐 <b>System and Network:</b>\n";

    // 20. Airplane Mode
    const auto airplane = GetSetting(settings, "System\\airplane_mode", "false");
    if (airplane == "true" || airplane == "1") {
        out += "• <b>Airplane Mode:</b> Enabled (prevents network socket hangs and server matchmaking delays)\n";
    } else {
        out += "• <b>Airplane Mode:</b> Disabled (active network interfaces)\n";
    }

    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }

    return out;
}

static GameFixProfile CreateEnrichedProfile(const GameFixProfile& base, u64 target_title_id = 0) {
    GameFixProfile enriched;
    enriched.title_id = target_title_id != 0 ? target_title_id : base.title_id;
    enriched.alt_title_ids = base.alt_title_ids;
    enriched.game_name = base.game_name;
    enriched.issues_ru = base.issues_ru;
    enriched.issues_en = base.issues_en;

    enriched.ini_settings = base.ini_settings.empty() ? s_baseline_ini : base.ini_settings;

    enriched.fixes_ru = BuildFixesRu(enriched.ini_settings);
    enriched.fixes_en = BuildFixesEn(enriched.ini_settings);
    return enriched;
}

[[maybe_unused]] static GameFixProfile CreateUniversalProfile(u64 title_id, const std::string& name_hint = "") {
    GameFixProfile universal;
    universal.title_id = title_id;
    if (!name_hint.empty()) {
        universal.game_name = name_hint;
    } else if (title_id != 0) {
        universal.game_name = fmt::format("Игра {:016X}", title_id);
    } else {
        universal.game_name = "Пользовательская игра";
    }

    universal.issues_ru =
        "• Микрофризы при компиляции шейдеров во время игрового процесса\n"
        "• Зависание вступительных видеороликов при аппаратном декодировании NVDEC\n"
        "• Сбои и задержки сетевых сервисов при поиске серверов\n"
        "• Риск аварийного завершения эмулятора при обращениях за границы буфера памяти";

    universal.issues_en =
        "• Ingame stuttering caused by on-demand shader compilation\n"
        "• Intro and cutscene freezes with hardware NVDEC decoding\n"
        "• Network socket stalls during server connection attempts\n"
        "• Risk of emulator crash on out-of-bounds guest memory accesses";

    universal.ini_settings = s_baseline_ini;
    universal.fixes_ru = BuildFixesRu(universal.ini_settings);
    universal.fixes_en = BuildFixesEn(universal.ini_settings);
    return universal;
}

static std::mutex s_profile_mutex;
static std::unordered_map<u64, std::unique_ptr<GameFixProfile>> s_enriched_cache;
static std::unordered_map<std::string, const GameFixProfile*> s_path_lookup_cache;
static std::unordered_map<u64, std::vector<const GameFixProfile*>> s_id_to_profiles_index;
static std::once_flag s_index_once_flag;

static void EnsureProfileIndexBuilt() {
    std::call_once(s_index_once_flag, [] {
        s_id_to_profiles_index.reserve(s_profiles.size() * 2);
        for (const auto& profile : s_profiles) {
            s_id_to_profiles_index[profile.title_id].push_back(&profile);
            const u64 base_id = profile.title_id & ~0x1FFFULL;
            if (base_id != profile.title_id) {
                s_id_to_profiles_index[base_id].push_back(&profile);
            }
            for (u64 alt : profile.alt_title_ids) {
                s_id_to_profiles_index[alt].push_back(&profile);
                const u64 alt_base = alt & ~0x1FFFULL;
                if (alt_base != alt) {
                    s_id_to_profiles_index[alt_base].push_back(&profile);
                }
            }
        }
    });
}

const GameFixProfile* GameFixDatabase::GetProfile(u64 title_id) {
    if (title_id == 0) return nullptr;

    std::lock_guard lock(s_profile_mutex);

    auto it = s_enriched_cache.find(title_id);
    if (it != s_enriched_cache.end()) {
        return it->second.get();
    }

    const u64 base_title_id = title_id & ~0x1FFFULL;
    auto it_base = s_enriched_cache.find(base_title_id);
    if (it_base != s_enriched_cache.end()) {
        return it_base->second.get();
    }

    EnsureProfileIndexBuilt();
    std::vector<const GameFixProfile*> matching_profiles;
    if (auto it_idx = s_id_to_profiles_index.find(title_id); it_idx != s_id_to_profiles_index.end()) {
        matching_profiles = it_idx->second;
    } else if (auto it_b = s_id_to_profiles_index.find(base_title_id); it_b != s_id_to_profiles_index.end()) {
        matching_profiles = it_b->second;
    }

    if (!matching_profiles.empty()) {
        // Merge matching profiles in sequence (earlier base + later enhancements)
        GameFixProfile merged = *matching_profiles.front();
        merged.title_id = title_id;
        for (size_t i = 1; i < matching_profiles.size(); ++i) {
            const auto* next_p = matching_profiles[i];
            if (!next_p->issues_ru.empty()) merged.issues_ru = next_p->issues_ru;
            if (!next_p->issues_en.empty()) merged.issues_en = next_p->issues_en;
            if (!next_p->game_name.empty()) merged.game_name = next_p->game_name;
            for (const auto& [k, v] : next_p->ini_settings) {
                merged.ini_settings[k] = v;
            }
            for (u64 alt : next_p->alt_title_ids) {
                if (std::find(merged.alt_title_ids.begin(), merged.alt_title_ids.end(), alt) == merged.alt_title_ids.end()) {
                    merged.alt_title_ids.push_back(alt);
                }
            }
        }
        auto enriched = std::make_unique<GameFixProfile>(CreateEnrichedProfile(merged, title_id));
        auto* ptr = enriched.get();
        s_enriched_cache[title_id] = std::move(enriched);
        return ptr;
    }

    return nullptr;
}

const GameFixProfile* GameFixDatabase::GetProfileByTitleOrPath(u64 title_id, const std::string& name_or_path) {
    if (title_id != 0) {
        const auto* p = GetProfile(title_id);
        if (p && !name_or_path.empty() && p->game_name.rfind("Игра ", 0) == 0) {
            std::lock_guard lock(s_profile_mutex);
            auto it = s_enriched_cache.find(title_id);
            if (it != s_enriched_cache.end()) {
                std::filesystem::path fp(name_or_path);
                std::string clean = fp.stem().string();
                auto brk = clean.find('[');
                if (brk != std::string::npos) clean = clean.substr(0, brk);
                auto par = clean.find('(');
                if (par != std::string::npos) clean = clean.substr(0, par);
                while (!clean.empty() && (clean.back() == ' ' || clean.back() == '_' || clean.back() == '-')) clean.pop_back();
                if (!clean.empty()) {
                    it->second->game_name = clean;
                }
            }
        }
        if (p) return p;
    }

    if (name_or_path.empty()) return nullptr;
    {
        std::lock_guard lock(s_profile_mutex);
        auto it_p = s_path_lookup_cache.find(name_or_path);
        if (it_p != s_path_lookup_cache.end()) {
            return it_p->second;
        }
    }

    // Check if name_or_path contains a 16-character hex Title ID
    for (size_t i = 0; i + 16 <= name_or_path.size(); ++i) {
        if ((i == 0 || !std::isxdigit(static_cast<unsigned char>(name_or_path[i - 1]))) &&
            (i + 16 == name_or_path.size() || !std::isxdigit(static_cast<unsigned char>(name_or_path[i + 16])))) {
            bool all_hex = true;
            for (size_t j = 0; j < 16; ++j) {
                if (!std::isxdigit(static_cast<unsigned char>(name_or_path[i + j]))) {
                    all_hex = false;
                    break;
                }
            }
            if (all_hex) {
                try {
                    u64 parsed = std::stoull(name_or_path.substr(i, 16), nullptr, 16);
                    if (parsed != 0) {
                        return GetProfile(parsed);
                    }
                } catch (...) {}
            }
        }
    }

    std::string lower = name_or_path;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& profile : s_profiles) {
        std::string game_lower = profile.game_name;
        std::transform(game_lower.begin(), game_lower.end(), game_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // Check full game name or title hex in string
        std::string hex_id = fmt::format(fmt::runtime("{:016x}"), profile.title_id);
        if (lower.find(hex_id) != std::string::npos) {
            return GetProfile(profile.title_id);
        }

        // Custom keyword matching
        if (game_lower.find("alan wake") != std::string::npos &&
            (lower.find("alan wake") != std::string::npos || lower.find("alan_wake") != std::string::npos ||
             lower.find("0100ad5013bb6000") != std::string::npos || lower.find("0100236015b7a000") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("belmont") != std::string::npos &&
            (lower.find("belmont") != std::string::npos || lower.find("curse") != std::string::npos ||
             lower.find("01ff434243555253") != std::string::npos ||
             (lower.find("castlevania") != std::string::npos && (lower.find("belmont") != std::string::npos || lower.find("curse") != std::string::npos)))) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("ys vs") != std::string::npos && (lower.find("ys vs") != std::string::npos || lower.find("alternative saga") != std::string::npos || lower.find("sora no kiseki") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("darkest dungeon") != std::string::npos && (lower.find("darkest dungeon") != std::string::npos || lower.find("darkest_dungeon") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("animal well") != std::string::npos && (lower.find("animal well") != std::string::npos || lower.find("animal_well") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("streets of rage") != std::string::npos && (lower.find("streets of rage") != std::string::npos || lower.find("sor4") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("splintered fate") != std::string::npos && (lower.find("splintered fate") != std::string::npos || lower.find("tmnt") != std::string::npos || lower.find("ninja turtles") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("breath of the wild") != std::string::npos && (lower.find("breath of the wild") != std::string::npos || lower.find("botw") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tears of the kingdom") != std::string::npos && (lower.find("tears of the kingdom") != std::string::npos || lower.find("totk") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("mario odyssey") != std::string::npos && (lower.find("odyssey") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("rabbids") != std::string::npos && (lower.find("rabbids") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("sonic frontiers") != std::string::npos && (lower.find("frontiers") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("witcher") != std::string::npos && (lower.find("witcher") != std::string::npos || lower.find("wild hunt") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("alan wake") != std::string::npos && (lower.find("alan wake") != std::string::npos || lower.find("0100236015b7a000") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("pikmin") != std::string::npos && lower.find("pikmin") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("peach") != std::string::npos && lower.find("peach") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("hollow knight") != std::string::npos && (lower.find("hollow knight") != std::string::npos || lower.find("hollow_knight") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("hades") != std::string::npos && lower.find("hades") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dead cells") != std::string::npos && (lower.find("dead cells") != std::string::npos || lower.find("dead_cells") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("outer wilds") != std::string::npos && lower.find("outer wilds") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("subnautica") != std::string::npos && lower.find("subnautica") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("alien") != std::string::npos && lower.find("alien") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("metroid dread") != std::string::npos && (lower.find("dread") != std::string::npos || lower.find("metroid") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tropical freeze") != std::string::npos && (lower.find("tropical freeze") != std::string::npos || lower.find("donkey kong") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dream land") != std::string::npos && (lower.find("dream land") != std::string::npos || lower.find("kirby") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("captain toad") != std::string::npos && (lower.find("captain toad") != std::string::npos || lower.find("treasure tracker") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("crafted world") != std::string::npos && (lower.find("crafted world") != std::string::npos || lower.find("yoshi") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("strikers") != std::string::npos && (lower.find("strikers") != std::string::npos || lower.find("battle league") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("bomberman") != std::string::npos && (lower.find("bomberman") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("advance wars") != std::string::npos && (lower.find("advance wars") != std::string::npos || lower.find("re-boot") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("warioware") != std::string::npos && (lower.find("warioware") != std::string::npos || lower.find("move it") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("no man's sky") != std::string::npos && (lower.find("no man") != std::string::npos || lower.find("nms") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("portal") != std::string::npos && (lower.find("portal") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("persona 4") != std::string::npos && (lower.find("persona 4") != std::string::npos || lower.find("p4g") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("persona 3") != std::string::npos && (lower.find("persona 3") != std::string::npos || lower.find("p3p") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tunic") != std::string::npos && (lower.find("tunic") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dave the diver") != std::string::npos && (lower.find("dave the diver") != std::string::npos || lower.find("dave_the_diver") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("cult of the lamb") != std::string::npos && (lower.find("cult of the lamb") != std::string::npos || lower.find("cult_of_the_lamb") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("unicorn overlord") != std::string::npos && (lower.find("unicorn overlord") != std::string::npos || lower.find("unicorn_overlord") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("skyward sword") != std::string::npos && (lower.find("skyward sword") != std::string::npos || lower.find("skyward_sword") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("bowser's fury") != std::string::npos && (lower.find("bowser") != std::string::npos || lower.find("3d world") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("jamboree") != std::string::npos && lower.find("jamboree") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("brothership") != std::string::npos && (lower.find("brothership") != std::string::npos || lower.find("mario & luigi") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("splatoon") != std::string::npos && lower.find("splatoon") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("origami king") != std::string::npos && (lower.find("origami") != std::string::npos || lower.find("paper mario") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("age of calamity") != std::string::npos && (lower.find("calamity") != std::string::npos || lower.find("hyrule warriors") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("cuphead") != std::string::npos && lower.find("cuphead") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("blind forest") != std::string::npos && (lower.find("blind forest") != std::string::npos || lower.find("ori") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("slay the spire") != std::string::npos && (lower.find("slay the spire") != std::string::npos || lower.find("slay_the_spire") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("into the breach") != std::string::npos && (lower.find("into the breach") != std::string::npos || lower.find("into_the_breach") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tales of") != std::string::npos && (lower.find("tales") != std::string::npos || lower.find("berseria") != std::string::npos || lower.find("symphonia") != std::string::npos || lower.find("vesperia") != std::string::npos || lower.find("graces") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("astral chain") != std::string::npos && (lower.find("astral chain") != std::string::npos || lower.find("astral_chain") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("three houses") != std::string::npos && (lower.find("three houses") != std::string::npos || lower.find("fe3h") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("smash bros") != std::string::npos && (lower.find("smash") != std::string::npos || lower.find("ssbu") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dragon quest") != std::string::npos && (lower.find("dragon quest") != std::string::npos || lower.find("dq11") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("nier") != std::string::npos && (lower.find("nier") != std::string::npos || lower.find("automata") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("bayonetta") != std::string::npos && lower.find("bayonetta") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("persona 5") != std::string::npos && (lower.find("persona 5") != std::string::npos || lower.find("p5r") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("live a live") != std::string::npos && (lower.find("live a live") != std::string::npos || lower.find("live_a_live") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tony hawk") != std::string::npos && (lower.find("tony hawk") != std::string::npos || lower.find("thps") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("nitro-fueled") != std::string::npos && (lower.find("nitro") != std::string::npos || lower.find("ctr") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if ((game_lower.find("mortal kombat 1") != std::string::npos || game_lower.find("mk1") != std::string::npos) && (lower.find("mortal kombat 1") != std::string::npos || lower.find("mk1") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("mortal kombat") != std::string::npos && (lower.find("mortal kombat") != std::string::npos || lower.find("mk11") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("gothic") != std::string::npos && lower.find("gothic") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("prince of persia") != std::string::npos && (lower.find("prince") != std::string::npos || lower.find("lost crown") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("echoes of wisdom") != std::string::npos && (lower.find("echoes") != std::string::npos || lower.find("wisdom") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("luigi's mansion 3") != std::string::npos && (lower.find("mansion 3") != std::string::npos || lower.find("lm3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("batman") != std::string::npos && (lower.find("batman") != std::string::npos || lower.find("arkham") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("kingdom come") != std::string::npos && (lower.find("kingdom come") != std::string::npos || lower.find("kcd") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tomb raider") != std::string::npos && lower.find("tomb raider") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("borderlands") != std::string::npos && lower.find("borderlands") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("diablo iii") != std::string::npos && (lower.find("diablo iii") != std::string::npos || lower.find("diablo 3") != std::string::npos || lower.find("d3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("demon slayer") != std::string::npos && (lower.find("demon slayer") != std::string::npos || lower.find("hinokami") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if ((game_lower.find("gta v") != std::string::npos || game_lower.find("gta 5") != std::string::npos) && (lower.find("gta v") != std::string::npos || lower.find("gta 5") != std::string::npos || lower.find("gtav") != std::string::npos || lower.find("0100b00b51230000") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if ((game_lower.find("gta") != std::string::npos || game_lower.find("grand theft auto") != std::string::npos) && (lower.find("gta") != std::string::npos || lower.find("grand theft auto") != std::string::npos || lower.find("san andreas") != std::string::npos || lower.find("vice city") != std::string::npos || lower.find("re3") != std::string::npos || lower.find("revc") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("monster hunter") != std::string::npos && (lower.find("monster hunter") != std::string::npos || lower.find("mhr") != std::string::npos || lower.find("sunbreak") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("burnout paradise") != std::string::npos && (lower.find("burnout") != std::string::npos || lower.find("paradise") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("overcooked") != std::string::npos && lower.find("overcooked") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("neighborville") != std::string::npos && (lower.find("neighborville") != std::string::npos || lower.find("pvz") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("risk of rain") != std::string::npos && (lower.find("risk of rain") != std::string::npos || lower.find("ror2") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("minecraft dungeons") != std::string::npos && (lower.find("dungeons") != std::string::npos || lower.find("mcd") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dragon ball fighterz") != std::string::npos && (lower.find("fighterz") != std::string::npos || lower.find("dbfz") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("xenoverse 2") != std::string::npos && (lower.find("xenoverse") != std::string::npos || lower.find("dbxv2") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("nba 2k") != std::string::npos && lower.find("nba 2k") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("just dance") != std::string::npos && lower.find("just dance") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tokyo 2020") != std::string::npos && (lower.find("tokyo 2020") != std::string::npos || lower.find("olympic") != std::string::npos || lower.find("tokyo_2020") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("51 worldwide") != std::string::npos && (lower.find("clubhouse") != std::string::npos || lower.find("51 worldwide") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tennis aces") != std::string::npos && (lower.find("tennis aces") != std::string::npos || lower.find("mario tennis") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("super rush") != std::string::npos && (lower.find("super rush") != std::string::npos || lower.find("mario golf") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("shin megami tensei") != std::string::npos && (lower.find("megami tensei") != std::string::npos || lower.find("smt5") != std::string::npos || lower.find("smtv") != std::string::npos || lower.find("vengeance") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("metroid prime") != std::string::npos && (lower.find("metroid prime") != std::string::npos || lower.find("prime remastered") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("crisis core") != std::string::npos && (lower.find("crisis core") != std::string::npos || lower.find("ffvii") != std::string::npos || lower.find("reunion") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("octopath traveler") != std::string::npos && (lower.find("octopath") != std::string::npos || lower.find("octopath2") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("sonic frontiers") != std::string::npos && (lower.find("sonic frontiers") != std::string::npos || lower.find("frontiers") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("tactics ogre") != std::string::npos && (lower.find("tactics ogre") != std::string::npos || lower.find("reborn") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dragon's dogma") != std::string::npos && (lower.find("dragon's dogma") != std::string::npos || lower.find("dragons dogma") != std::string::npos || lower.find("dark arisen") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("warframe") != std::string::npos && lower.find("warframe") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("brotato") != std::string::npos && lower.find("brotato") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("vampire survivors") != std::string::npos && (lower.find("vampire") != std::string::npos || lower.find("survivors") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("xenoblade") != std::string::npos && (lower.find("xenoblade") != std::string::npos || lower.find("xcde") != std::string::npos || lower.find("xc2") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("mario rpg") != std::string::npos && (lower.find("mario rpg") != std::string::npos || lower.find("super mario rpg") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("showtime") != std::string::npos && (lower.find("showtime") != std::string::npos || lower.find("princess peach") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("luigi's mansion 2") != std::string::npos && (lower.find("mansion 2") != std::string::npos || lower.find("dark moon") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("f-zero") != std::string::npos && (lower.find("f-zero") != std::string::npos || lower.find("fzero") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("3d all-stars") != std::string::npos && (lower.find("all-stars") != std::string::npos || lower.find("all stars") != std::string::npos || lower.find("sunshine") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("arceus") != std::string::npos && (lower.find("arceus") != std::string::npos || lower.find("legends") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("scarlet") != std::string::npos && lower.find("scarlet") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("violet") != std::string::npos && lower.find("violet") != std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("devilutionx") != std::string::npos && (lower.find("devilutionx") != std::string::npos || lower.find("diablo") != std::string::npos || lower.find("hellfire") != std::string::npos || lower.find("diabdat") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("re3-sa") != std::string::npos && (lower.find("re3-sa") != std::string::npos || lower.find("san andreas") != std::string::npos || lower.find("gtasa") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("revc") != std::string::npos && (lower.find("revc") != std::string::npos || lower.find("vice city") != std::string::npos || lower.find("gtavc") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("re3") != std::string::npos && (lower.find("re3") != std::string::npos || lower.find("gta 3") != std::string::npos || lower.find("gta iii") != std::string::npos || lower.find("gta3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("xash3d") != std::string::npos && (lower.find("xash3d") != std::string::npos || lower.find("half-life") != std::string::npos || lower.find("halflife") != std::string::npos || lower.find("valve.wad") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("gzdoom") != std::string::npos && (lower.find("gzdoom") != std::string::npos || lower.find("prboom") != std::string::npos || lower.find("crispy") != std::string::npos || lower.find("doom.wad") != std::string::npos || lower.find("doom2.wad") != std::string::npos || lower.find("plutonia") != std::string::npos || lower.find("sigil") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("dhewm3") != std::string::npos && (lower.find("dhewm3") != std::string::npos || lower.find("doom 3") != std::string::npos || lower.find("doom3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("quakespasm") != std::string::npos && (lower.find("quake") != std::string::npos || lower.find("quakespasm") != std::string::npos || lower.find("yamagi") != std::string::npos || lower.find("ioquake3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("eduke32") != std::string::npos && (lower.find("duke") != std::string::npos || lower.find("duke3d") != std::string::npos || lower.find("voidsw") != std::string::npos || lower.find("shadow warrior") != std::string::npos || lower.find("nblood") != std::string::npos || lower.find("blood.rff") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("iortcw") != std::string::npos && (lower.find("iortcw") != std::string::npos || lower.find("ecwolf") != std::string::npos || lower.find("wolfenstein") != std::string::npos || lower.find("wolf3d") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("fallout-ce") != std::string::npos && (lower.find("fallout") != std::string::npos || lower.find("fallout-ce") != std::string::npos || lower.find("fallout2-ce") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("fheroes2") != std::string::npos && (lower.find("heroes") != std::string::npos || lower.find("fheroes2") != std::string::npos || lower.find("vcmi") != std::string::npos || lower.find("homm") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("max payne") != std::string::npos && (lower.find("max payne") != std::string::npos || lower.find("maxpayne") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("openmw") != std::string::npos && (lower.find("openmw") != std::string::npos || lower.find("morrowind") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("openjk") != std::string::npos && (lower.find("openjk") != std::string::npos || lower.find("jedi outcast") != std::string::npos || lower.find("jedi academy") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("openxray") != std::string::npos && (lower.find("openxray") != std::string::npos || lower.find("stalker") != std::string::npos || lower.find("s.t.a.l.k.e.r") != std::string::npos || lower.find("chernobyl") != std::string::npos || lower.find("pripyat") != std::string::npos || lower.find("clear sky") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("ea sports fc 26") != std::string::npos && (lower.find("fc 27") != std::string::npos || lower.find("fc27") != std::string::npos || lower.find("fc 26") != std::string::npos || lower.find("fc26") != std::string::npos || lower.find("fifa 27") != std::string::npos || lower.find("fifa 26") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("ea sports fc 25") != std::string::npos && (lower.find("fc 25") != std::string::npos || lower.find("fc25") != std::string::npos || lower.find("fifa 25") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("ea sports fc 24") != std::string::npos && (lower.find("fc 24") != std::string::npos || lower.find("fc24") != std::string::npos || lower.find("fifa 24") != std::string::npos || lower.find("fifa") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("diablo ii") != std::string::npos && (lower.find("diablo ii") != std::string::npos || lower.find("diablo 2") != std::string::npos || lower.find("resurrected") != std::string::npos || lower.find("0100726014352000") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("disco elysium") != std::string::npos && (lower.find("disco elysium") != std::string::npos || lower.find("disco_elysium") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("hogwarts legacy") != std::string::npos && (lower.find("hogwarts") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower == "crysis 3 remastered" && (lower.find("crysis 3") != std::string::npos || lower.find("crysis3") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower == "crysis 2 remastered" && (lower.find("crysis 2") != std::string::npos || lower.find("crysis2") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower == "crysis remastered" && lower.find("crysis") != std::string::npos && lower.find("crysis 2") == std::string::npos && lower.find("crysis 3") == std::string::npos) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("amalur") != std::string::npos && (lower.find("amalur") != std::string::npos || lower.find("re-reckoning") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("rsdk") != std::string::npos && (lower.find("rsdk") != std::string::npos || lower.find("sonic cd") != std::string::npos || lower.find("data.rsdk") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("sm64-nx") != std::string::npos && (lower.find("sm64") != std::string::npos || lower.find("render96") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("ship of harkinian") != std::string::npos && (lower.find("harkinian") != std::string::npos || lower.find("soh") != std::string::npos || lower.find("2s2h") != std::string::npos || lower.find("ocarina of time") != std::string::npos || lower.find("majora") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("nxengine") != std::string::npos && (lower.find("nxengine") != std::string::npos || lower.find("cave story") != std::string::npos || lower.find("am2r") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("corsixth") != std::string::npos && (lower.find("corsixth") != std::string::npos || lower.find("theme hospital") != std::string::npos || lower.find("julius") != std::string::npos || lower.find("augustus") != std::string::npos || lower.find("caesar") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("openra") != std::string::npos && (lower.find("openra") != std::string::npos || lower.find("dune legacy") != std::string::npos || lower.find("command & conquer") != std::string::npos || lower.find("red alert") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("retroarch") != std::string::npos && (lower.find("retroarch") != std::string::npos || lower.find("ppsspp") != std::string::npos || lower.find("flycast") != std::string::npos || lower.find("scummvm") != std::string::npos || lower.find("melonds") != std::string::npos || lower.find("mgba") != std::string::npos || lower.find("duckstation") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }
        if (game_lower.find("homebrew utilities") != std::string::npos && (lower.find("nx-shell") != std::string::npos || lower.find("dbi") != std::string::npos || lower.find("goldleaf") != std::string::npos || lower.find("jksv") != std::string::npos || lower.find("checkpoint") != std::string::npos || lower.find("edizon") != std::string::npos || lower.find("awoo") != std::string::npos || lower.find("tesla") != std::string::npos)) {
            return GetProfile(profile.title_id);
        }

        if (lower.find(game_lower) != std::string::npos) {
            return GetProfile(profile.title_id);
        }
    }

    // Dynamic universal profile for unprofiled game
    std::filesystem::path fp(name_or_path);
    std::string clean_name = fp.stem().string();
    auto brk = clean_name.find('[');
    if (brk != std::string::npos) clean_name = clean_name.substr(0, brk);
    auto par = clean_name.find('(');
    if (par != std::string::npos) clean_name = clean_name.substr(0, par);
    while (!clean_name.empty() && (clean_name.back() == ' ' || clean_name.back() == '_' || clean_name.back() == '-')) clean_name.pop_back();
    if (clean_name.empty()) clean_name = "Пользовательская игра";

    u64 pseudo_tid = Common::CityHash64(name_or_path.data(), name_or_path.size());
    std::lock_guard lock(s_profile_mutex);
    auto it = s_enriched_cache.find(pseudo_tid);
    if (it != s_enriched_cache.end()) {
        return it->second.get();
    }

    return nullptr;
}

bool GameFixDatabase::HasProfile(u64 title_id) {
    return GetProfile(title_id) != nullptr;
}

const std::vector<GameFixProfile>& GameFixDatabase::GetAllProfiles() {
    return s_profiles;
}

static bool s_fixes_enabled = true;
static std::vector<std::string> s_excluded_keys;

void GameFixDatabase::SetExcludedKeys(const std::vector<std::string>& excluded_keys) {
    s_excluded_keys = excluded_keys;
}

bool GameFixDatabase::IsKeyExcluded(const std::string& full_key) {
    for (const auto& k : s_excluded_keys) {
        if (k == full_key) {
            return true;
        }
    }
    return false;
}

bool GameFixDatabase::ApplyProfileToPerGameConfig(u64 title_id, const std::string& config_file_path) {
    const auto* profile = GetProfile(title_id);
    if (!profile) {
        return false;
    }

    std::filesystem::path path(config_file_path);
    std::filesystem::create_directories(path.parent_path());

    // Read existing INI if present
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
    if (std::filesystem::exists(path)) {
        std::ifstream file(path);
        std::string line;
        std::string current_section;
        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
            } else {
                auto eq = line.find('=');
                if (eq != std::string::npos && !current_section.empty()) {
                    auto key = line.substr(0, eq);
                    auto val = line.substr(eq + 1);
                    sections[current_section][key] = val;
                }
            }
        }
    }

    // Merge settings from profile without touching aspect_ratio or user scaling preferences
    for (const auto& [full_key, val] : profile->ini_settings) {
        if (IsKeyExcluded(full_key)) {
            continue;
        }
        if (full_key == "Renderer\\aspect_ratio" || full_key == "Renderer\\resolution_setup" ||
            full_key == "System\\use_docked_mode" || full_key == "Renderer\\anti_aliasing" ||
            full_key == "Renderer\\scaling_filter" || full_key == "Renderer\\fsr_sharpening_slider" ||
            full_key == "Renderer\\max_anisotropy" || full_key == "Renderer\\frame_pacing_mode" ||
            full_key == "Renderer\\backend") {
            continue;
        }
        auto slash = full_key.find('\\');
        if (slash != std::string::npos) {
            auto sec = full_key.substr(0, slash);
            auto key = full_key.substr(slash + 1);
            if (key == "aspect_ratio" || key == "resolution_setup" || key == "use_docked_mode" ||
                key == "anti_aliasing" || key == "scaling_filter" || key == "fsr_sharpening_slider" ||
                key == "max_anisotropy" || key == "frame_pacing_mode" || key == "backend") {
                continue;
            }
            if (key == "airplane_mode") {
                sec = "Services";
                if (sections.count("System")) {
                    sections["System"].erase("airplane_mode");
                    sections["System"].erase("airplane_mode\\use_global");
                    sections["System"].erase("airplane_mode\\default");
                }
                if (sections.count("Network")) {
                    sections["Network"].erase("airplane_mode");
                    sections["Network"].erase("airplane_mode\\use_global");
                    sections["Network"].erase("airplane_mode\\default");
                }
            } else if (key == "memory_layout_mode") {
                sec = "Core";
                if (sections.count("System")) {
                    sections["System"].erase("memory_layout_mode");
                    sections["System"].erase("memory_layout_mode\\use_global");
                    sections["System"].erase("memory_layout_mode\\default");
                }
            } else if (key == "use_auto_stub" || key == "disable_web_applet" ||
                       key == "disable_macro_jit" || key == "disable_macro_hle") {
                sec = "Debugging";
            }
            std::string sanitized_val = val;
            if (key == "memory_layout_mode") {
                int mode = 0;
                if (!sanitized_val.empty()) {
                    mode = std::atoi(sanitized_val.c_str());
                }
                if (mode >= 1 && !HasAtLeast5GbHostRam()) {
                    sanitized_val = "0";
                } else if (mode >= 2 && !HasAtLeast7GbHostRam()) {
                    sanitized_val = "1";
                }
            }
            sections[sec][key] = sanitized_val;
            sections[sec][key + "\\use_global"] = "false";
            sections[sec][key + "\\default"] = "false";
        }
    }
    sections["StormSwitch"]["storm_fix_applied"] = "true";
    sections.erase("StormEden");

    // Write back INI
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;

    for (const auto& [sec, kvs] : sections) {
        out << "[" << sec << "]\n";
        for (const auto& [k, v] : kvs) {
            out << k << "=" << v << "\n";
        }
        out << "\n";
    }

    LOG_INFO(Frontend, "Applied GameFix profile for {:#016x} to {}", title_id, config_file_path);
    return true;
}

void GameFixDatabase::SetDontAskAgain(u64 title_id, const std::string& config_file_path, bool dont_ask) {
    if (config_file_path.empty()) return;
    std::filesystem::path path(config_file_path);
    if (!dont_ask && !std::filesystem::exists(path)) {
        return;
    }
    std::filesystem::create_directories(path.parent_path());

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
    if (std::filesystem::exists(path)) {
        std::ifstream file(path);
        std::string line;
        std::string current_section;
        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
            } else {
                auto eq = line.find('=');
                if (eq != std::string::npos && !current_section.empty()) {
                    auto key = line.substr(0, eq);
                    auto val = line.substr(eq + 1);
                    sections[current_section][key] = val;
                }
            }
        }
    }

    if (dont_ask) {
        sections["StormSwitch"]["storm_fix_dont_ask"] = "true";
    } else if (sections.count("StormSwitch")) {
        sections["StormSwitch"].erase("storm_fix_dont_ask");
        if (sections["StormSwitch"].empty()) {
            sections.erase("StormSwitch");
        }
    }
    sections.erase("StormEden");
    if (sections.empty()) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        return;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return;

    for (const auto& [sec, kvs] : sections) {
        out << "[" << sec << "]\n";
        for (const auto& [k, v] : kvs) {
            out << k << "=" << v << "\n";
        }
        out << "\n";
    }
}

int GameFixDatabase::ResetAllDontAskAgain() {
    int count = 0;
    const auto config_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
    if (!std::filesystem::exists(config_dir)) return 0;

    for (const auto& entry : std::filesystem::directory_iterator(config_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".ini") continue;
        std::ifstream file(entry.path());
        if (!file.is_open()) continue;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        if (content.find("storm_fix_dont_ask") != std::string::npos) {
            std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
            std::istringstream iss(content);
            std::string line, cur_sec;
            while (std::getline(iss, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
                if (line.empty() || line[0] == '#' || line[0] == ';') continue;
                if (line.front() == '[' && line.back() == ']') cur_sec = line.substr(1, line.size() - 2);
                else {
                    auto eq = line.find('=');
                    if (eq != std::string::npos && !cur_sec.empty()) sections[cur_sec][line.substr(0, eq)] = line.substr(eq + 1);
                }
            }
            bool erased = false;
            if (sections.count("StormSwitch") && sections["StormSwitch"].erase("storm_fix_dont_ask") > 0) {
                erased = true;
            }
            if (sections.count("StormEden") && sections["StormEden"].erase("storm_fix_dont_ask") > 0) {
                erased = true;
            }
            if (erased) {
                std::ofstream out(entry.path(), std::ios::trunc);
                for (const auto& [sec, kvs] : sections) {
                    out << "[" << sec << "]\n";
                    for (const auto& [k, v] : kvs) out << k << "=" << v << "\n";
                    out << "\n";
                }
                count++;
            }
        }
    }
    return count;
}

void GameFixDatabase::SetFixesEnabled(bool enabled) {
    s_fixes_enabled = enabled;
}

bool GameFixDatabase::AreFixesEnabled() {
    return s_fixes_enabled;
}

void GameFixDatabase::SanitizeSettingsForHostHardware() {
    const u64 ram_bytes = GetHostPhysicalRamBytes();
    if (ram_bytes == 0) {
        return;
    }

    auto apply_session_only = [](auto& setting, auto val) {
        if (setting.UsingGlobal()) {
            setting.SetGlobal(false);
        }
        setting.SetValue(val);
    };

    const auto mem_mode = Settings::values.memory_layout_mode.GetValue();
    if (!Settings::IsCurrentProgramSwitch2() && !HasAtLeast5GbHostRam()) {
        if (mem_mode != Settings::MemoryLayout::Memory_4Gb) {
            apply_session_only(Settings::values.memory_layout_mode, Settings::MemoryLayout::Memory_4Gb);
            LOG_INFO(Frontend, "Hardware Auto-Correction: Clamped memory_layout_mode to 4GB on <=4GB RAM device ({} MB)",
                     ram_bytes / (1024ULL * 1024ULL));
        }
        if (Settings::values.vram_usage_mode.GetValue() != Settings::VramUsageMode::Conservative) {
            apply_session_only(Settings::values.vram_usage_mode, Settings::VramUsageMode::Conservative);
        }
        if (Settings::values.astc_recompression.GetValue() == Settings::AstcRecompression::Uncompressed) {
            apply_session_only(Settings::values.astc_recompression, Settings::AstcRecompression::Bc3);
            LOG_INFO(Frontend, "Hardware Auto-Correction: Enabled BC3 ASTC recompression on <=4GB RAM device to prevent OOM");
        }
    } else if (!HasAtLeast7GbHostRam()) {
        if (mem_mode == Settings::MemoryLayout::Memory_8Gb) {
            apply_session_only(Settings::values.memory_layout_mode, Settings::MemoryLayout::Memory_6Gb);
            LOG_INFO(Frontend, "Hardware Auto-Correction: Clamped memory_layout_mode 8GB -> 6GB on <7GB RAM device ({} MB)",
                     ram_bytes / (1024ULL * 1024ULL));
        }
        if (Settings::values.vram_usage_mode.GetValue() == Settings::VramUsageMode::Aggressive) {
            apply_session_only(Settings::values.vram_usage_mode, Settings::VramUsageMode::Conservative);
        }
    }
}

static void ApplySettingsMap(const std::unordered_map<std::string, std::string>& settings_map) {
    auto safe_stoi = [](const std::string& s, int def = 0) -> int {
        try {
            if (s == "true") return 1;
            if (s == "false") return 0;
            return std::stoi(s);
        } catch (...) {
            return def;
        }
    };

    auto apply_setting = [](auto& setting, auto val) {
        if constexpr (requires { setting.UsingGlobal(); setting.SetGlobal(false); }) {
            if (!setting.UsingGlobal()) {
                return;
            }
            setting.SetGlobal(false);
        }
        setting.SetValue(val);
    };

    for (const auto& [full_key, val] : settings_map) {
        if (GameFixDatabase::IsKeyExcluded(full_key)) {
            continue;
        }
        // STRICT PRESERVATION: Never override aspect ratio, user scaling filters or graphics API backend
        if (full_key == "Renderer\\aspect_ratio" || full_key == "Renderer\\resolution_setup" ||
            full_key == "System\\use_docked_mode" || full_key == "Renderer\\anti_aliasing" ||
            full_key == "Renderer\\scaling_filter" || full_key == "Renderer\\fsr_sharpening_slider" ||
            full_key == "Renderer\\max_anisotropy" || full_key == "Renderer\\frame_pacing_mode" ||
            full_key == "Renderer\\backend") {
            continue;
        }

        if (full_key == "Renderer\\gpu_accuracy") {
            apply_setting(Settings::values.gpu_accuracy, static_cast<Settings::GpuAccuracy>(safe_stoi(val, 1)));
        } else if (full_key == "Renderer\\dma_accuracy") {
            apply_setting(Settings::values.dma_accuracy, static_cast<Settings::DmaAccuracy>(safe_stoi(val, 3)));
        } else if (full_key == "Renderer\\drs_resolution_lock") {
            apply_setting(Settings::values.drs_resolution_lock, val == "true" || val == "1");
        } else if (full_key == "Renderer\\rescale_hack") {
            apply_setting(Settings::values.rescale_hack, val == "true" || val == "1");
        } else if (full_key == "Renderer\\dynamic_performance_scaler") {
            apply_setting(Settings::values.dynamic_performance_scaler, val == "true" || val == "1");
        } else if (full_key == "Renderer\\barrier_feedback_loops") {
            apply_setting(Settings::values.barrier_feedback_loops, val == "true" || val == "1");
        } else if (full_key == "Renderer\\use_reactive_flushing") {
            apply_setting(Settings::values.use_reactive_flushing, val == "true" || val == "1");
        } else if (full_key == "Renderer\\astc_recompression") {
            apply_setting(Settings::values.astc_recompression, static_cast<Settings::AstcRecompression>(safe_stoi(val, 0)));
        } else if (full_key == "Renderer\\use_asynchronous_shaders") {
            apply_setting(Settings::values.use_asynchronous_shaders, val == "true" || val == "1");
        } else if (full_key == "Renderer\\enable_gpu_buffer_readback") {
            apply_setting(Settings::values.enable_gpu_buffer_readback, val == "true" || val == "1");
        } else if (full_key == "Renderer\\early_release_fences") {
            apply_setting(Settings::values.early_release_fences, val == "true" || val == "1");
        } else if (full_key == "Renderer\\sync_memory_operations") {
            apply_setting(Settings::values.sync_memory_operations, val == "true" || val == "1");
        } else if (full_key == "Renderer\\use_fast_gpu_time") {
            apply_setting(Settings::values.use_fast_gpu_time, val == "true" || val == "1");
        } else if (full_key == "Renderer\\nvdec_emulation") {
            auto nvdec_val = static_cast<Settings::NvdecEmulation>(safe_stoi(val, 1));
#ifdef __ANDROID__
            if (nvdec_val == Settings::NvdecEmulation::Gpu) {
                nvdec_val = Settings::NvdecEmulation::Hybrid;
            }
#endif
            apply_setting(Settings::values.nvdec_emulation, nvdec_val);
        } else if (full_key == "Renderer\\async_presentation") {
            apply_setting(Settings::values.async_presentation, val == "true" || val == "1");
        } else if (full_key == "Renderer\\accelerate_astc") {
            apply_setting(Settings::values.accelerate_astc, static_cast<Settings::AstcDecodeMode>(safe_stoi(val, 1)));
        } else if (full_key == "Renderer\\gpu_fence_behavior") {
            apply_setting(Settings::values.gpu_fence_behavior, static_cast<Settings::GpuFenceBehavior>(safe_stoi(val, 0)));
        } else if (full_key == "Renderer\\dyna_state") {
            apply_setting(Settings::values.dyna_state, static_cast<Settings::ExtendedDynamicState>(safe_stoi(val, 0)));
        } else if (full_key == "Renderer\\vram_usage_mode") {
            apply_setting(Settings::values.vram_usage_mode, static_cast<Settings::VramUsageMode>(safe_stoi(val, 1)));
        } else if (full_key == "Renderer\\backend") {
            apply_setting(Settings::values.renderer_backend, static_cast<Settings::RendererBackend>(safe_stoi(val, 1)));
        } else if (full_key == "Renderer\\fix_bloom_effects") {
            apply_setting(Settings::values.fix_bloom_effects, val == "true" || val == "1");
        } else if (full_key == "Renderer\\eco_frame_pacing") {
            apply_setting(Settings::values.eco_frame_pacing, val == "true" || val == "1");
        } else if (full_key == "Renderer\\emulate_bgr565") {
            apply_setting(Settings::values.emulate_bgr565, val == "true" || val == "1");
        } else if (full_key == "Renderer\\use_video_framerate") {
            apply_setting(Settings::values.use_video_framerate, val == "true" || val == "1");
        } else if (full_key == "Renderer\\use_vulkan_driver_pipeline_cache") {
            apply_setting(Settings::values.use_vulkan_driver_pipeline_cache, val == "true" || val == "1");
        } else if (full_key == "Renderer\\use_disk_shader_cache") {
            apply_setting(Settings::values.use_disk_shader_cache, val == "true" || val == "1");
        } else if (full_key == "Renderer\\enable_compute_pipelines") {
            apply_setting(Settings::values.enable_compute_pipelines, val == "true" || val == "1");
        } else if (full_key == "Renderer\\vram_garbage_collection") {
            apply_setting(Settings::values.vram_garbage_collection, val == "true" || val == "1");
        } else if (full_key == "Renderer\\enable_frame_skipping") {
            apply_setting(Settings::values.enable_frame_skipping, val == "true" || val == "1");
        } else if (full_key == "Renderer\\vram_budget_governor") {
            apply_setting(Settings::values.vram_budget_governor, val == "true" || val == "1");
        } else if (full_key == "Renderer\\smart_shader_throttle") {
            apply_setting(Settings::values.smart_shader_throttle, val == "true" || val == "1");
        } else if (full_key == "Renderer\\enable_buffer_history") {
            apply_setting(Settings::values.enable_buffer_history, val == "true" || val == "1");
        } else if (full_key == "Renderer\\optimize_spirv_output") {
            apply_setting(Settings::values.optimize_spirv_output, static_cast<Settings::SpirvOptimizeMode>(safe_stoi(val, 1)));
        } else if (full_key == "System\\airplane_mode" || full_key == "Services\\airplane_mode" || full_key == "Network\\airplane_mode") {
            apply_setting(Settings::values.airplane_mode, val == "true" || val == "1");
        } else if (full_key == "System\\memory_layout_mode" || full_key == "Core\\memory_layout_mode") {
            if (Settings::IsCurrentProgramSwitch2()) {
                continue;
            }
            int mode = safe_stoi(val, 0);
            if (mode >= 1 && !HasAtLeast5GbHostRam()) {
                mode = 0;
            } else if (mode >= 2 && !HasAtLeast7GbHostRam()) {
                mode = 1;
            }
            apply_setting(Settings::values.memory_layout_mode, static_cast<Settings::MemoryLayout>(mode));
        } else if (full_key == "System\\language_index") {
            apply_setting(Settings::values.language_index, static_cast<Settings::Language>(safe_stoi(val, 1)));
        } else if (full_key == "System\\region_index") {
            apply_setting(Settings::values.region_index, static_cast<Settings::Region>(safe_stoi(val, 1)));
        } else if (full_key == "Cpu\\cpu_accuracy") {
            apply_setting(Settings::values.cpu_accuracy, static_cast<Settings::CpuAccuracy>(safe_stoi(val, 0)));
        } else if (full_key == "Cpu\\cpu_backend") {
            apply_setting(Settings::values.cpu_backend, static_cast<Settings::CpuBackend>(safe_stoi(val, 1)));
        } else if (full_key == "Cpu\\cpuopt_fastmem") {
            apply_setting(Settings::values.cpuopt_fastmem, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_ignore_memory_aborts") {
            apply_setting(Settings::values.cpuopt_ignore_memory_aborts, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_fastmem_exclusives") {
            apply_setting(Settings::values.cpuopt_fastmem_exclusives, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_recompile_exclusives") {
            apply_setting(Settings::values.cpuopt_recompile_exclusives, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_unsafe_unfuse_fma") {
            apply_setting(Settings::values.cpuopt_unsafe_unfuse_fma, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_unsafe_host_mmu") {
            apply_setting(Settings::values.cpuopt_unsafe_host_mmu, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_unsafe_reduce_fp_error") {
            apply_setting(Settings::values.cpuopt_unsafe_reduce_fp_error, val == "true" || val == "1");
        } else if (full_key == "Cpu\\cpuopt_unsafe_ignore_standard_fpcr") {
            apply_setting(Settings::values.cpuopt_unsafe_ignore_standard_fpcr, val == "true" || val == "1");
        } else if (full_key == "Cpu\\use_storm_tier2_compiler") {
            apply_setting(Settings::values.use_storm_tier2_compiler, val == "true" || val == "1");
        } else if (full_key == "Core\\sync_core_speed" || full_key == "System\\sync_core_speed") {
            apply_setting(Settings::values.sync_core_speed, val == "true" || val == "1");
        } else if (full_key == "Core\\cpu_clock" || full_key == "Cpu\\cpu_clock") {
            apply_setting(Settings::values.cpu_clock, static_cast<Settings::CpuClock>(safe_stoi(val, 0)));
        } else if (full_key == "Core\\gpu_clock" || full_key == "Renderer\\gpu_clock") {
            apply_setting(Settings::values.gpu_clock, static_cast<Settings::GpuClock>(safe_stoi(val, 0)));
        } else if (full_key == "Renderer\\force_max_clock") {
            apply_setting(Settings::values.renderer_force_max_clock, val == "true" || val == "1");
        } else if (full_key == "Renderer\\enable_frame_skipping") {
            apply_setting(Settings::values.enable_frame_skipping, val == "true" || val == "1");
        } else if (full_key == "Renderer\\vertex_input_dynamic_state") {
            apply_setting(Settings::values.vertex_input_dynamic_state, val == "true" || val == "1");
        } else if (full_key == "Debugging\\disable_macro_hle") {
            apply_setting(Settings::values.disable_macro_hle, val == "true" || val == "1");
        } else if (full_key == "Debugging\\disable_macro_jit") {
            apply_setting(Settings::values.disable_macro_jit, val == "true" || val == "1");
        } else if (full_key == "Debugging\\disable_web_applet") {
            apply_setting(Settings::values.disable_web_applet, val == "true" || val == "1");
        } else if (full_key == "Debugging\\use_auto_stub") {
            apply_setting(Settings::values.use_auto_stub, val == "true" || val == "1");
        }
    }
}

std::string GameFixDatabase::NormalizeGpuVendor(u32 vendor_id, const std::string& driver_name) {
    if (vendor_id == 0x10DE) return "nvidia";
    if (vendor_id == 0x1002) return "amd";
    if (vendor_id == 0x8086) return "intel";
    if (vendor_id == 0x5143) return "adreno";
    if (vendor_id == 0x13B5) return "mali";

    std::string s = driver_name;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (s.find("nvidia") != std::string::npos || s.find("geforce") != std::string::npos ||
        s.find("tegra") != std::string::npos || s.find("nvk") != std::string::npos ||
        s.find("nouveau") != std::string::npos) {
        return "nvidia";
    }
    if (s.find("amd") != std::string::npos || s.find("radeon") != std::string::npos ||
        s.find("radv") != std::string::npos || s.find("amdvlk") != std::string::npos) {
        return "amd";
    }
    if (s.find("intel") != std::string::npos || s.find("anv") != std::string::npos ||
        s.find("arc") != std::string::npos || s.find("iris") != std::string::npos) {
        return "intel";
    }
    if (s.find("adreno") != std::string::npos || s.find("qualcomm") != std::string::npos ||
        s.find("freedreno") != std::string::npos || s.find("turnip") != std::string::npos) {
        return "adreno";
    }
    if (s.find("xclipse") != std::string::npos) {
        return "xclipse";
    }
    if (vendor_id == 0x144D || s.find("samsung") != std::string::npos) {
        return "xclipse";
    }
    if (s.find("mali") != std::string::npos || s.find("immortalis") != std::string::npos ||
        s.find("panfrost") != std::string::npos || s.find("panvk") != std::string::npos ||
        s.find("valhall") != std::string::npos || s.find("bifrost") != std::string::npos) {
        return "mali";
    }
    return "other";
}

void GameFixDatabase::SetDetectedGpuVendor(const std::string& vendor_or_driver) {
    s_detected_vendor = NormalizeGpuVendor(0, vendor_or_driver);
    LOG_INFO(Frontend, "[GameFixDatabase] Detected GPU vendor normalized to '{}' (from '{}')",
             s_detected_vendor, vendor_or_driver);
}

void GameFixDatabase::SetDetectedGpuVendorFromDriver(u32 vendor_id, const std::string& driver_name) {
    s_detected_vendor = NormalizeGpuVendor(vendor_id, driver_name);
    LOG_INFO(Frontend, "[GameFixDatabase] Detected GPU vendor normalized to '{}' (vendor_id=0x{:X}, driver='{}')",
             s_detected_vendor, vendor_id, driver_name);
}

const std::string& GameFixDatabase::GetDetectedGpuVendor() {
    return s_detected_vendor;
}

bool GameFixDatabase::ReapplyVendorOverrides(u64 title_id) {
    try {
        const auto* profile = GetProfile(title_id);
        if (!profile && s_last_applied_tid != 0) {
            profile = GetProfile(s_last_applied_tid);
        }
        if (!profile) {
            return false;
        }
        if (s_detected_vendor.empty() || !profile->vendor_overrides.count(s_detected_vendor)) {
            return false;
        }

        const auto& vendor_map = profile->vendor_overrides.at(s_detected_vendor);
        ApplySettingsMap(vendor_map);

        SanitizeSettingsForHostHardware();
        Settings::UpdateGPUAccuracy();
        Settings::UpdateRescalingInfo();
        LOG_INFO(Frontend, "[GameFixDatabase] Reapplied {} vendor overrides for '{}' on {:#016x}",
                 vendor_map.size(), s_detected_vendor, profile->title_id);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<AppliedProfileOverride> GameFixDatabase::GetAppliedOverrides(u64 title_id) {
    const auto* profile = GetProfile(title_id);
    if (!profile) {
        return {};
    }

    std::vector<AppliedProfileOverride> result;
    result.reserve(profile->ini_settings.size() + 8);
    std::unordered_map<std::string, size_t> key_indices;

    for (const auto& [k, v] : profile->ini_settings) {
        key_indices[k] = result.size();
        result.push_back({k, v, "base"});
    }

    if (!s_detected_vendor.empty() && profile->vendor_overrides.count(s_detected_vendor)) {
        for (const auto& [k, v] : profile->vendor_overrides.at(s_detected_vendor)) {
            auto it = key_indices.find(k);
            if (it != key_indices.end()) {
                result[it->second].value = v;
                result[it->second].source = s_detected_vendor;
            } else {
                key_indices[k] = result.size();
                result.push_back({k, v, s_detected_vendor});
            }
        }
    }

    return result;
}

bool GameFixDatabase::ApplyProfileDirectly(u64 title_id, const std::string& name_or_path) {
    try {
        const auto* profile = GetProfile(title_id);
        if (!profile && !name_or_path.empty()) {
            profile = GetProfileByTitleOrPath(title_id, name_or_path);
        }
        if (!profile) {
            SanitizeSettingsForHostHardware();
            return false;
        }

        s_last_applied_tid = profile->title_id;

        // 1. Apply base settings
        ApplySettingsMap(profile->ini_settings);

        // 2. Apply detected vendor overrides
        if (!s_detected_vendor.empty() && profile->vendor_overrides.count(s_detected_vendor)) {
            const auto& vendor_map = profile->vendor_overrides.at(s_detected_vendor);
            ApplySettingsMap(vendor_map);
            LOG_INFO(Frontend, "[GameFixDatabase] Applied {} vendor overrides for '{}' on {:#016x}",
                     vendor_map.size(), s_detected_vendor, profile->title_id);
        }

        SanitizeSettingsForHostHardware();
        Settings::UpdateGPUAccuracy();
        Settings::UpdateRescalingInfo();
        LOG_INFO(Frontend, "Directly applied GameFix profile in-memory for {:#016x}", profile->title_id);
        return true;
    } catch (...) {
        return false;
    }
}

bool GameFixDatabase::IsFixApplied(u64 title_id, const std::string& config_file_path) {
    if (config_file_path.empty() || !std::filesystem::exists(config_file_path)) {
        return false;
    }
    const auto* profile = GetProfile(title_id);
    if (!profile) {
        return false;
    }
    std::ifstream file(config_file_path);
    if (!file.is_open()) {
        return false;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
    std::string line;
    std::string current_section;
    while (std::getline(file, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
        } else {
            auto eq = line.find('=');
            if (eq != std::string::npos && !current_section.empty()) {
                auto key = line.substr(0, eq);
                auto val = line.substr(eq + 1);
                sections[current_section][key] = val;
            }
        }
    }

    if ((sections.count("StormSwitch") && sections["StormSwitch"]["storm_fix_applied"] == "true") ||
        (sections.count("StormEden") && sections["StormEden"]["storm_fix_applied"] == "true")) {
        return true;
    }
    return false;
}





} // namespace Core
