// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/game_fix_database.h"
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

namespace Core {

static const std::vector<GameFixProfile> s_profiles = {
    {
        0x010033001F050000ULL,
        "Ys vs. Trails in the Sky: Alternative Saga",
        "• Зависание и артефакты макроблоков вступительного видеоролика\n• Искажения 2D-спрайтов и шрифтов при пересжатии текстур ASTC\n• Просадки кадровой частоты во время динамичных сражений\n• Сбои распределения памяти при переходе между аренами",
        "• Opening cinematic freeze and macroblock artifacts\n• 2D sprite and font distortion with ASTC texture recompression\n• Framerate drops during fast-paced combat\n• Memory allocation faults during arena transitions",
        "✓ Декодирование видео NVDEC: ЦП (программный декодер FFmpeg устраняет зависание роликов)\n✓ Пересжатие текстур ASTC: Без сжатия (устранение искажений 2D-графики)\n✓ Точность ГПУ: Высокая (стабильная геометрия и Z-буфер)\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ NVDEC Video Emulation: CPU (software FFmpeg decoder prevents cutscene freezes)\n✓ ASTC Texture Recompression: Uncompressed (Fixes 2D graphics distortion)\n✓ GPU Accuracy: High (Stable geometry and Z-buffer)\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\nvdec_emulation", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x010033001F050800ULL}
    },
    {
        0x010034B00E14C000ULL,
        "Tokyo 2020 Olympics - The Official Video Game",
        "• Черный экран и зависание при запуске из-за инициализации сервисов и пересжатия текстур\n• Просадки кадровой частоты во время соревнований\n• Сбои синхронизации потоков",
        "• Black screen and boot freeze caused by service initialization and texture compression\n• Framerate drops during competitions\n• Thread synchronization faults",
        "✓ Точность ГПУ: Высокая (устранение черного экрана)\n✓ Пересжатие текстур ASTC: Без сжатия\n✓ Асинхронная презентация и шейдеры: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM",
        "✓ GPU Accuracy: High (Fixes black screen)\n✓ ASTC Texture Recompression: Uncompressed\n✓ Async Presentation and Shaders: Enabled\n✓ Fast GPU Time: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "1"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x010034B00E14C800ULL}
    },
    {
        0x010003000E146000ULL,
        "Mario and Sonic at the Olympic Games Tokyo 2020",
        "• Черный экран и зависание при запуске из-за задач Hedgehog Engine 2\n• Просадки кадровой частоты во время соревнований\n• Сбои синхронизации потоков",
        "• Black screen and boot freeze in Hedgehog Engine 2 task scheduler\n• Framerate drops during competitions\n• Thread synchronization faults",
        "✓ Точность ГПУ: Высокая (устранение черного экрана)\n✓ Пересжатие текстур ASTC: Без сжатия\n✓ Асинхронная презентация и шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM",
        "✓ GPU Accuracy: High (Fixes black screen)\n✓ ASTC Texture Recompression: Uncompressed\n✓ Async Presentation and Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_unsafe_ignore_global_monitor", "true"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x010003000E146800ULL}
    },
    {
        0x01008F1008DA6000ULL,
        "Darkest Dungeon",
        "• Зависание и вылет вступительного видеоролика при программном декодировании NVDEC ЦП\n• Просадки кадровой частоты в подземельях\n• Сбои распределения памяти",
        "• Freezing and crash in opening cinematic with CPU NVDEC video decoding\n• Framerate drops in dungeons\n• Memory allocation faults",
        "✓ Декодирование видео NVDEC: Гибридное (Hybrid 3) — стабильное воспроизведение вступительных роликов\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Режим «В самолете»: Включено\n✓ Точность ГПУ: Обычная",
        "✓ NVDEC Video Emulation: Hybrid (Hybrid 3) — stable video playback\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Airplane Mode: Enabled\n✓ GPU Accuracy: Normal",
        {
            {"Renderer\\nvdec_emulation", "3"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x01008F1008DA6800ULL}
    },
    {
        0x0100DDF01A03A000ULL,
        "DAVE THE DIVER",
        "• Вылеты при смене локаций (дайвинг / суши-бар Банчо) из-за сборщика мусора Unity GC\n• Утечка дескрипторов текстур под водой\n• Просадки кадровой частоты",
        "• Crashes during transitions (Diving / Bancho Sushi) caused by Unity GC spikes\n• Underwater texture descriptor exhaustion\n• Framerate drops and stuttering",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Очистка VRAM: Отключено (устраняет микрофризы анимаций)\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ VRAM Garbage Collection: Disabled (Prevents animation stutter)\n✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x0100DDF01A03A800ULL}
    },
    {
        0x010077001A8D4000ULL,
        "Brotato",
        "• Просадки FPS при спавне тысяч снарядов и врагов на поздних волнах (15-20)\n• Микрофризы рендеринга Godot Engine\n• Задержка отклика ввода",
        "• Framerate drops during massive bullet/enemy swarms on waves 15-20\n• Godot Engine pipeline stalls\n• Input latency during intense combat",
        "✓ Точность ГПУ: Обычная (идеальные 60 FPS)\n✓ Раннее освобождение барьеров: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Циклы обратной связи: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Perfect 60 FPS)\n✓ Early Release Fences: Enabled\n✓ Fast GPU Time: Enabled\n✓ Barrier Feedback Loops: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x010077001A8D4800ULL}
    },
    {
        0x01008BA02525A000ULL,
        "Dispatch",
        "• Графические артефакты геометрии и мерцание полигонов в кинематографических сценах\n• Разрывы буферов кадров и вылеты шейдеров\n• Просадки FPS при динамическом освещении",
        "• Geometry artifacts and polygon flickering during cinematic sequences\n• Framebuffer tearing and shader crashes\n• Framerate drops during dynamic lighting transitions",
        "✓ Точность ГПУ: Обычная (устраняет мерцание и графические артефакты)\n✓ Точность DMA: Безопасно (0)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Циклы обратной связи: Включено (корректные тени Unreal Engine)\n✓ Синхронизация памяти ГПУ: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM",
        "✓ GPU Accuracy: Normal (Eliminates flickering and visual glitches)\n✓ DMA Accuracy: Safe (0)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Barrier Feedback Loops: Enabled (Correct Unreal Engine shadows)\n✓ Sync Memory Operations: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x01008BA02525A800ULL}
    },
    {
        0x01006560184E6000ULL,
        "Mortal Kombat 1",
        "• Мгновенный вылет при запуске (UE4 TaskGraph / атомики)\n• Зависание на заставке WB Games и вылет по нехватке памяти (OOM)\n• Сбои Extended Dynamic State в шейдерах арены\n• Пропадание текстур персонажей и окружения при длительной игре",
        "• Instant crash on launch (UE4 TaskGraph / atomics)\n• WB Games intro freeze and Out of Memory crash\n• Extended Dynamic State arena shader crashes\n• Character and environment texture streaming dropouts",
        "✓ Память: 8 ГБ DRAM (устраняет дедлок и краш при загрузке боя)\n✓ Быстрое время ГПУ: Отключено (исправление зависания на заставке и стартовом тексте)\n✓ Декодирование ASTC: ГПУ\n✓ Поведение барьеров ГПУ: Стандартное (0)\n✓ Точность DMA: Стандартная\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Точность ЦП: Авто (стабильный быстрый JIT-компилятор)\n✓ Синхронизация памяти: Отключено\n✓ Реактивная очистка: Отключено\n✓ Режим полёта: Включено (пропуск серверов WB Play)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Точность ГПУ: Обычная",
        "✓ Memory Layout: 8GB DRAM (Fixes fight load crash)\n✓ Fast GPU Time: Disabled (Fixes intro and epilepsy warning freeze)\n✓ ASTC Decoding: GPU\n✓ GPU Fence Behavior: Standard (0)\n✓ DMA Accuracy: Standard\n✓ Dynamic State: Basic (EDS1)\n✓ CPU Accuracy: Auto (Stable fast JIT compiler)\n✓ Sync Memory Operations: Disabled\n✓ Reactive Flushing: Disabled\n✓ Airplane Mode: Enabled (Bypasses WB Play online check)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: Normal",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\eco_frame_pacing", "false"},
            {"Renderer\\use_video_framerate", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_recompile_exclusives", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\gpu_accuracy", "0"}
        },
        {0x0100D2800D5C2000ULL}
    },
    {
        0x0100F2200C984000ULL,
        "Mortal Kombat 11",
        "• Разлет полигонов, мерцание геометрии и графические баги шейдеров\n• Искажение динамического освещения и эффектов крови\n• Микрофризы рендеринга",
        "• Exploding vertices, geometry flicker, and shader rendering artifacts\n• Corrupted dynamic lighting and blood particle effects\n• Pipeline micro-stutter",
        "✓ Точность ГПУ: Обычная (устраняет разлет полигонов и артефакты)\n✓ Быстрое время ГПУ: Отключено\n✓ Синхронизация памяти ГПУ: Отключено (стабильный конвейер)\n✓ Реактивная очистка: Отключено (исправление освещения)\n✓ Barrier Feedback Loops: Включено (исправление частиц)\n✓ Сжатие ASTC: Без сжатия\n✓ Память: 6 ГБ DRAM",
        "✓ GPU Accuracy: Normal (Fixes exploding vertices and glitches)\n✓ Fast GPU Time: Disabled\n✓ Sync Memory Operations: Disabled\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"System\\airplane_mode", "false"},
            {"System\\language_index", "10"},
            {"System\\region_index", "2"}
        },
        {0x0100B1100C4D0000ULL}
    },
    {
        0x0100EC9010258000ULL,
        "Streets of Rage 4",
        "• Полная поддержка и стабильная работа на ПК (Windows 10/11) и Android: 60 FPS, устранено зависание сетевого сокета DotEmu и видеовставок NVDEC",
        "• Full verified support and rock-solid performance on PC (Windows 10/11) and Android: 60 FPS, DotEmu network socket hang and NVDEC video desync fixed",
        "✓ Режим полета: Отключено (устраняет дедлок сетевого опроса DotEmu)\n✓ Декодирование видео: NVDEC ГПУ\n✓ Синхронизация памяти ГПУ: Отключено (предотвращает зависание на 0 FPS)\n✓ Игнорирование сбоев памяти: Включено\n✓ Реактивная очистка: Отключено\n✓ Обратное чтение буферов ГПУ: Отключено",
        "✓ Airplane Mode: Disabled (Fixes DotEmu network deadlock)\n✓ NVDEC Emulation: GPU Video Decoding\n✓ Sync Memory Operations: Disabled (prevents 0 FPS hang)\n✓ Ignore Memory Aborts: Enabled\n✓ Reactive Flushing: Disabled\n✓ GPU Buffer Readback: Disabled",
        {
            {"System\\airplane_mode", "false"},
            {"Network\\airplane_mode", "false"},
            {"Services\\airplane_mode", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\enable_compute_pipelines", "false"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x010085800E33E000ULL, 0x01000BD011936000ULL, 0x0100F7A011938000ULL, 0x0100BA700E340000ULL, 0x0100C60010228000ULL, 0x0100AC300919A000ULL}
    },
    {
        0x01007EF00011E000ULL,
        "The Legend of Zelda: Breath of the Wild",
        "• Вылет игры при загрузке сохранения (исчерпание пула памяти 4GB DRAM)\n• Сбои десериализации акторов и физики Havok\n• Черный силуэт Линка и мерцание освещения",
        "• Crash when loading save games (4GB DRAM pool exhaustion)\n• Havok physics and actor deserialization crash\n• Link black silhouette and lighting flicker",
        "✓ Память: 8GB DRAM (предотвращает краш аллокатора при загрузке сейва)\n✓ Игнорирование сбоев памяти: Включено (стабильность физики Havok)\n✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено (стабильность кадрового буфера)\n✓ Barrier Feedback Loops: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 8GB DRAM (Prevents save loading OOM)\n✓ Ignore Memory Aborts: Enabled (Havok physics stability)\n✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\nvdec_emulation", "1"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
            {"Renderer\\enable_compute_pipelines", "false"},
            {"System\\airplane_mode", "false"},
            {"Network\\airplane_mode", "false"},
        },
        {0x01007EF00011E800ULL}
    },
    {
        0x01005B101DC84000ULL,
        "EA SPORTS FC 25",
        "• Зависание при старте игры / черный экран на заставке EA\n• Нехватка памяти Frostbite Engine (вылет при загрузке стадиона)\n• Зависание на сетевой аутентификации",
        "• Boot hang / black screen on EA splash screen\n• Frostbite Engine out-of-memory crash\n• Network handshake freeze",
        "✓ Память: 8GB DRAM (критично для движка Frostbite)\n✓ Режим полета: Включено (пропуск серверов EA Connect)\n✓ Асинхронные шейдеры: Отключено (стабильность загрузки)\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for Frostbite Engine)\n✓ Airplane Mode: Enabled (Bypasses EA Connect handshake)\n✓ Asynchronous Shaders: Disabled (Prevents boot crash)\n✓ Fastmem: Enabled",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010077D0238FA000ULL,
        "EA SPORTS FC 26",
        "• Зависание при старте игры / черный экран на заставке EA\n• Нехватка памяти Frostbite Engine (вылет при загрузке стадиона)\n• Зависание на сетевой аутентификации",
        "• Boot hang / black screen on EA splash screen\n• Frostbite Engine out-of-memory crash\n• Network handshake freeze",
        "✓ Память: 8GB DRAM (критично для движка Frostbite)\n✓ Режим полета: Включено (пропуск серверов EA Connect)\n✓ Асинхронные шейдеры: Отключено (стабильность загрузки)\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for Frostbite Engine)\n✓ Airplane Mode: Enabled (Bypasses EA Connect handshake)\n✓ Asynchronous Shaders: Disabled (Prevents boot crash)\n✓ Fastmem: Enabled",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01008CA01F186000ULL,
        "Leap Year",
        "• Вылет игры через некоторое время из-за утечки памяти\n• Рассинхронизация аудиопотока",
        "• Crash after prolonged gameplay due to memory accumulation\n• Audio stream desync",
        "✓ Память: 6GB DRAM (устраняет вылет через время)\n✓ Асинхронные шейдеры: Отключено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents crash over time)\n✓ Asynchronous Shaders: Disabled\n✓ Fastmem: Enabled",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100B0E020356000ULL,
        "The Legend of Heroes: Trails in the Sky",
        "• Стробоскопическое мерцание экрана и окружения\n• Пропадание текстур персонажей и задних планов\n• Артефакты сжатия ASTC",
        "• Screen and environment strobing flicker\n• Missing character and background textures\n• ASTC compression artifacts",
        "✓ Сжатие ASTC: Отключено (устраняет пропадание текстур)\n✓ Асинхронные шейдеры: Отключено (ликвидация мерцания)\n✓ Реактивная очистка: Отключено\n✓ Память: 6GB DRAM",
        "✓ ASTC Recompression: Uncompressed (Fixes missing textures)\n✓ Asynchronous Shaders: Disabled (Fixes flickering)\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01000B0012E4E000ULL,
        "Cronos: Before the Ashes",
        "• Некорректное темное изображение и сбои освещения UE4\n• Артефакты постобработки и размытия",
        "• Incorrect dark image and UE4 lighting corruption\n• Post-processing and motion blur artifacts",
        "✓ Реактивная очистка: Отключено (исправление освещения UE4)\n✓ Точность ГПУ: Высокая (точность шейдеров FP16)\n✓ Сжатие ASTC: Отключено\n✓ Barrier Feedback Loops: Включено",
        "✓ Reactive Flushing: Disabled (Fixes UE4 lighting)\n✓ GPU Accuracy: High (FP16 shader precision)\n✓ ASTC Recompression: Uncompressed\n✓ Barrier Feedback Loops: Enabled",
        {
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
    {
        0x010055700C30A000ULL,
        "The Outer Worlds",
        "• Некорректное изображение и черные ореолы вокруг объектов\n• Сбои TAA и темные артефакты геометрии UE4",
        "• Corrupted rendering and black halos around objects\n• TAA glitches and dark UE4 geometry artifacts",
        "✓ Реактивная очистка: Отключено (исправление черных ореолов)\n✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Barrier Feedback Loops: Включено\n✓ Память: 8GB DRAM",
        "✓ Reactive Flushing: Disabled (Fixes black halos)\n✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Barrier Feedback Loops: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"}
        }
    },
    {
        0x01000A10041EA000ULL,
        "The Elder Scrolls V: Skyrim",
        "• Некорректное изображение водной глади и искажения отражений\n• Артефакты сжатия снега и текстур ландшафта",
        "• Incorrect water rendering and reflection distortion\n• Snow and terrain texture compression artifacts",
        "✓ Сжатие ASTC: Отключено (четкие текстуры снега и гор)\n✓ Реактивная очистка: Отключено (исправление отражений воды)\n✓ Точность ГПУ: Высокая\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ ASTC Recompression: Uncompressed (Crisp snow and terrain)\n✓ Reactive Flushing: Disabled (Fixes water reflections)\n✓ GPU Accuracy: High\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"}
        },
        {0x01000A10041EA800ULL}
    },
    {
        0x0100BCB0176D0000ULL,
        "Hogwarts Legacy",
        "• Некорректное изображение, сбои освещения и геометрии\n• Вылет при переходе между локациями Хогвартса из-за нехватки памяти",
        "• Corrupted rendering, lighting and geometry glitches\n• Out-of-memory crash during Hogwarts area transitions",
        "✓ Память: 8GB DRAM (критично для стабильности)\n✓ Реактивная очистка: Отключено (исправление освещения замка)\n✓ Barrier Feedback Loops: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 8GB DRAM (Critical for stability)\n✓ Reactive Flushing: Disabled (Fixes castle lighting)\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\astc_recompression", "0"}
        },
        {0x0100B5B0112F8000ULL, 0x0100D1801648E000ULL}
    },
    {
        0x0100F2C0115B6000ULL,
        "The Legend of Zelda: Tears of the Kingdom",
        "• Черный силуэт персонажей и тени в Кавернах\n• Бирюзовая сетка и артефакты Z-буфера на водных поверхностях\n• Утечки VRAM в конструкторе Ультраруки",
        "• Character silhouette and shadow artifacts in Depths\n• Water surface and depth bias cyan grid artifacts\n• Ultrahand VRAM pressure",
        "✓ Точность ГПУ: Высокая (исправление теней и освещения)\n✓ Реактивная очистка: Включено (устранение мерцания магии Ультраруки и рун)\n✓ Сжатие ASTC: Отключено (прозрачная чистая вода)\n✓ Быстрое время ГПУ: Отключено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High (Fixes character shadows and lighting)\n✓ Reactive Flushing: Enabled (Fixes Ultrahand and rune magic flickering)\n✓ ASTC Recompression: Uncompressed\n✓ Fast GPU Time: Disabled\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x0100F2C0115B6800ULL}
    },
    {
        0x01004D701742A000ULL,
        "Paper Mario: The Thousand-Year Door",
        "• Черный экран на катсценах в прологе\n• Сбои 2D-шрифтов диалогов и мерцание текстур",
        "• Black screen during prologue cutscenes\n• Corrupted battle text boxes and flickering textures",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x01002A801458A000ULL,
        "Diablo II: Resurrected",
        "• Вылет при продолжении игры / загрузке персонажа (нехватка памяти)\n• Зависание при опросе серверов Battle.net\n• Мерцание персонажа на экране выбора героя и графические артефакты\n• Просадки кадровой частоты (4 FPS / 200 ms)",
        "• Character load / continue game crash (нехватка памяти)\n• Battle.net server handshake hang\n• Character flickering on selection screen and graphical artifacts\n• Frame drops (4 FPS / 200 ms)",
        "✓ Память: 8GB DRAM (критично для загрузки персонажа!)\n✓ Режим полёта: Включено (пропуск Battle.net)\n✓ Быстрое время ГПУ: Включено (стабильные 60 FPS)\n✓ Сжатие ASTC: Отключено (устраняет полосы и квадраты)\n✓ Точность ЦП: Авто (устранение заторможенности)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Отключено (устраняет мерцание персонажа)\n✓ Обратное чтение буферов ГПУ: Отключено (устраняет 4 FPS / 200 ms)",
        "✓ Memory Layout: 8GB DRAM (Critical for character loading!)\n✓ Airplane Mode: Enabled (Bypasses Battle.net)\n✓ Fast GPU Time: Enabled (Stable 60 FPS)\n✓ ASTC Recompression: Uncompressed (Fixes decal artifacts)\n✓ CPU Accuracy: Auto (Eliminates sluggishness)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Disabled (Fixes character flickering)\n✓ GPU Buffer Readback: Disabled (Fixes 4 FPS / 200 ms)",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "false"}
        },
        {0x0100916014D8C000ULL, 0x0100726014352000ULL}
    },
    {
        0x010020D01AD24000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100650017170000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100C9E01B854000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010020D01AD24000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x0100E5E01C098000ULL, 0x0100650017170000ULL, 0x0100C9E01B854000ULL}
    },
    {
        0x0100650017170000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }

    },
    {
        0x0100C6000EEA8000ULL,
        "Warhammer 40,000: Mechanicus",
        "• Невозможно сохранить прогресс игры (ошибка сохранения)",
        "• Unable to save game progress (infinite save loop)",
        "✓ Поддержка RenameDirectory в STORM SWITCH 4.6.0+\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ RenameDirectory support in STORM SWITCH 4.6.0+\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100923008C54000ULL,
        "LEGO Star Wars: The Skywalker Saga",
        "• Бесконечная загрузка на заставке\n• Зависание асинхронного таймера GPU при обращении к ресурсам\n• Блокировка сетевой телеметрии WB на Android",
        "• Infinite loading screen (TT Games loading indicator loop)\n• GPU async timer deadlock during asset initialization\n• WB telemetry network deadlock on Android",
        "✓ Режим полёта: Включено (критично для запуска на Android!)\n✓ Быстрое время ГПУ: Отключено (устраняет вечную загрузку!)\n✓ Динамическое состояние: Базовое\n✓ Точность ГПУ: Высокая\n✓ Точность ЦП: Точная\n✓ Реактивная очистка: Отключено\n✓ Память: 8GB DRAM",
        "✓ Airplane Mode: Enabled (Critical for Android launch!)\n✓ Fast GPU Time: Disabled (Fixes infinite loading!)\n✓ Dynamic State: Basic\n✓ GPU Accuracy: High\n✓ CPU Accuracy: Accurate\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\dyna_state", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpu_accuracy", "0"}
        }
    },
    {
        0x0100E26017E5E000ULL,
        "Red Dead Redemption",
        "• Хрипы и рассинхронизация звука в катсценах\n• Микрофризы физических потоков RAGE Engine",
        "• Audio crackling and desync in cutscenes\n• Micro-stutters in RAGE Engine physics threads",
        "✓ Точность ЦП: Точная\n✓ Аудио-буфер Cubeb: 80 ms\n✓ Синхронизация памяти: Отключено",
        "✓ CPU Accuracy: Accurate\n✓ Cubeb Audio Buffer: 80 ms\n✓ Sync Memory Ops: Disabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\sync_memory_operations", "false"}
        }
    },
    {
        0x0100D870045B6000ULL,
        "Luigi's Mansion 3",
        "• Растягивание полигонов (взрывы геометрии)\n• Невидимый луч фонарика и зависания в лифте",
        "• Vertex explosion (stretched geometry)\n• Invisible flashlight beam and elevator freeze",
        "✓ Расширенное динамическое состояние: Включено\n✓ Точность ГПУ: Высокая\n✓ Точность ЦП: Точная",
        "✓ Extended Dynamic State: Enabled\n✓ GPU Accuracy: High\n✓ CPU Accuracy: Accurate",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\dyna_state", "2"}
        }
    },
    {
        0x01004A4010F22000ULL,
        "Bayonetta 3",
        "• Невидимые персонажи и противники на чипах Snapdragon\n• Чёрный экран после QTE-добиваний",
        "• Invisible character/enemy models on Snapdragon SoCs\n• Black screen after QTE sequences",
        "✓ Контроль отсечения глубины: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Depth Clip Control: Enabled (STORM DRIVER)\n✓ GPU Accuracy: High",
        {
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Пропадание неонового интерфейса Легиона\n• Затемнение картинки и циклический гул звука",
        "• Missing Legion neon glow effects\n• Dark screen tint and audio looping",
        "✓ Эмуляция цвета BGR565: Включено\n✓ Коррекция эффектов свечения: Включено",
        "✓ Emulate BGR565: Enabled\n✓ Fix Bloom Effects: Enabled",
        {
            {"Renderer\\emulate_bgr565", "true"},
            {"Renderer\\fix_bloom_effects", "true"}
        }
    },
    {
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Вылет на экране победы или в меню новостей",
        "• Crash on victory screen or news board (Web Applet)",
        "✓ Отключение веб-апплета: Включено\n✓ Mii Applet: LLE",
        "✓ Disable Web Applet: Enabled\n✓ Mii Applet: LLE",
        {
            {"Debugging\\disable_web_applet", "true"}
        }
    },
    {
        0x0100152000022000ULL,
        "Mario Kart 8 Deluxe",
        "• Отсутствие голов у персонажей Mii на трассах\n• Вылет при переходе между режимами и загрузке трасс\n• Нехватка памяти при длительных сессиях",
        "• Invisible/missing heads on Mii characters\n• Crash during mode transitions and track loading\n• Memory pressure during extended sessions",
        "✓ Требуется Firmware 18.0.0+ и системные файлы Mii\n✓ Сжатие ASTC: Отключено\n✓ Реактивный сброс: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM",
        "✓ Firmware 18.0.0+ and Mii system files required\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01001F5010DFA000ULL,
        "Pokemon Legends: Arceus",
        "• Вытягивание полигонов травы и деревьев в небо\n• Сбои теней на аренах",
        "• Vertex explosion on trees and grass geometry\n• Shadow glitches during battle transitions",
        "✓ Точность ГПУ: Высокая\n✓ Анизотропная фильтрация: 16x\n✓ Декодирование ASTC на GPU: Включено",
        "✓ GPU Accuracy: High\n✓ Anisotropic Filtering: 16x\n✓ ASTC GPU Decode: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x01008C30086E0000ULL,
        "Pokemon Scarlet",
        "• Утечки памяти в открытом мире Палдеи\n• Мерцание ландшафта и текстур",
        "• Open-world memory leaks in Paldea\n• Terrain and texture flickering",
        "✓ Сжатие ASTC: Отключено\n✓ Ограничение VRAM: Conservative",
        "✓ ASTC Recompression: BC3\n✓ VRAM Usage: Conservative",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_usage_mode", "1"}
        }
    },
    {
        0x0100A3D0086EE000ULL,
        "Pokemon Violet",
        "• Утечки памяти в открытом мире Палдеи\n• Мерцание ландшафта и текстур",
        "• Open-world memory leaks in Paldea\n• Terrain and texture flickering",
        "✓ Сжатие ASTC: Отключено\n✓ Ограничение VRAM: Conservative",
        "✓ ASTC Recompression: BC3\n✓ VRAM Usage: Conservative",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_usage_mode", "1"}
        }
    },
    {
        0x0100B9F010DC4000ULL,
        "Doom Eternal",
        "• Вылет после заставки об эпилепсии из-за переполнения пула памяти (OOM)\n• Вылет драйвера Vulkan при первом выстреле / спавне BFG",
        "• Crash after epilepsy warning due to 4GB memory pool exhaustion (OOM)\n• Vulkan device loss crash on weapon fire / BFG",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устраняет вылет после предупреждения об эпилепсии)\n✓ Игнорировать прерывания памяти: Включено\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Точность DMA: Safe\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents crash after epilepsy warning)\n✓ Ignore Memory Aborts: Enabled\n✓ Dynamic State: Basic (EDS1)\n✓ DMA Accuracy: Safe\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x010034B01314C000ULL,
        "Prince of Persia: The Lost Crown",
        "• Чёрный экран при воспроизведении видеовставок и анимаций амулетов",
        "• Black screen during video cutscenes and amulet animations",
        "✓ Декодирование видео (NVDEC): На GPU\n✓ Fastmem Exclusives: Отключено",
        "✓ NVDEC Video Emulation: GPU\n✓ Fastmem Exclusives: Disabled",
        {
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"}
        }
    },
    {
        0x010063B017DAE000ULL,
        "Batman: Arkham Knight",
        "• Вылет по нехватке памяти при погонях на Бэтмобиле",
        "• OOM crash during Batmobile chase sequences",
        "✓ Сжатие ASTC: Отключено\n✓ Память: 8GB DRAM",
        "✓ ASTC Recompression: BC1\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\astc_recompression", "0"},
            {"System\\memory_layout_mode", "2"}
        }
    },
    {
        0x01000B901C46E000ULL,
        "Shin Megami Tensei V: Vengeance",
        "• Вылет движка Unreal Engine 4 при старте на чипах Snapdragon 8",
        "• Unreal Engine 4 crash on launch on Snapdragon 8 devices",
        "✓ Macro JIT / HLE: Включено\n✓ Нативное декодирование BCn: Включено",
        "✓ Macro JIT / HLE: Enabled\n✓ Native BCn Decode: Enabled",
        {
            {"Debugging\\disable_macro_jit", "false"},
            {"Debugging\\disable_macro_hle", "false"}
        }
    },
    {
        0x01003D100E9C6000ULL,
        "The Witcher 3: Wild Hunt",
        "• Зависание физики волос/одежды Геральта в Новиграде",
        "• HairWorks and physics freezes in Novigrad",
        "✓ Fastmem Exclusives: Включено",
        "✓ Fastmem Exclusives: Enabled",
        {
            {"Cpu\\cpuopt_fastmem_exclusives", "true"},
            {"Renderer\\fsr_sharpening_slider", "85"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x0100760012E4A000ULL,
        "Mario + Rabbids Sparks of Hope",
        "• Вылет движка Snowdrop при переходе в тактический бой",
        "• Snowdrop engine crash on tactical combat transition",
        "✓ Точность DMA: Safe\n✓ Точность ГПУ: Высокая\n✓ Barrier Feedback Loops: Включено",
        "✓ DMA Accuracy: Safe\n✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled",
        {
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
    {
        0x010056D015DB6000ULL,
        "Sonic Frontiers",
        "• Просадки FPS и размытие текстур в открытых зонах островов\n• Мерцание теней Cyberspace и подгрузка геометрии",
        "• Open-zone framerate drops and blurry grass textures\n• Cyberspace shadow flickering and geometry pop-in",
        "✓ Память: 6GB DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"}
        },
        {0x01004C90141A4000ULL, 0x0100508013AE0000ULL}
    },
    {
        0x01004AB00A260000ULL,
        "Dark Souls: Remastered",
        "• Просадки кадровой частоты у костров и зацикливание звука баффов",
        "• Bonfire particle slowdown and weapon buff sound loop",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая (корректные шейдеры и освещение)\n✓ Игнорировать прерывания памяти: Включено\n✓ Сжатие ASTC: Отключено (нативный ASTC для Mali и Adreno)\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High (accurate shaders and lighting)\n✓ Ignore Memory Aborts: Enabled\n✓ ASTC Recompression: Uncompressed (native ASTC for Mali and Adreno)\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\async_presentation", "false"}
        }
    },
    {
        0x01001A8005FB2000ULL,
        "Dark Souls: Remastered",
        "• Просадки кадровой частоты у костров и зацикливание звука баффов",
        "• Bonfire particle slowdown and weapon buff sound loop",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая (корректные шейдеры и освещение)\n✓ Игнорировать прерывания памяти: Включено\n✓ Сжатие ASTC: Отключено (нативный ASTC для Mali и Adreno)\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High (accurate shaders and lighting)\n✓ Ignore Memory Aborts: Enabled\n✓ ASTC Recompression: Uncompressed (native ASTC for Mali and Adreno)\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\async_presentation", "false"}
        }
    },
    {
        0x01004AB00A266000ULL,
        "Dark Souls: Remastered",
        "• Просадки кадровой частоты у костров и зацикливание звука баффов",
        "• Bonfire particle slowdown and weapon buff sound loop",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая (корректные шейдеры и освещение)\n✓ Игнорировать прерывания памяти: Включено\n✓ Сжатие ASTC: Отключено (нативный ASTC для Mali и Adreno)\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High (accurate shaders and lighting)\n✓ Ignore Memory Aborts: Enabled\n✓ ASTC Recompression: Uncompressed (native ASTC for Mali and Adreno)\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\async_presentation", "false"}
        }
    },
    {
        0x010028600EBDA000ULL,
        "Super Mario 3D World + Bowser's Fury",
        "• Оптимизация физики и анимаций персонажей\n• Микрофризы при компиляции шейдеров",
        "• Optimization for physics and character animations\n• Shader compilation stutter on character animations",
        "✓ Конфигурация памяти: 4 ГБ DRAM\n✓ Точность ГПУ: Обычная (плавные 60 кадров/с)\n✓ Быстрое время ГПУ: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 4GB DRAM\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010015100B5B4000ULL,
        "Super Mario Bros. Wonder",
        "• Вылет при переходе уровней и активации Чудо-цветка (нехватка стандартной памяти 4GB)\n• Микрофризы при компиляции анимаций персонажей",
        "• Crash on stage transitions and Wonder Flower effects (4GB memory limit)\n• Shader compilation stutter on character animations",
        "✓ Конфигурация памяти: 6 ГБ DRAM (предотвращение переполнения памяти)\n✓ Точность ГПУ: Обычная (Normal 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents memory overflow)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },

    {
        0x0100C88011246000ULL,
        "Disco Elysium: The Final Cut",
        "• Утечка памяти и вылеты при смене локаций\n• Размытие и мерцание текста диалогов TextMeshPro\n• Цветовые артефакты акварельных портретов и фонов",
        "• Out of memory (нехватка памяти) crash on zone transitions\n• TextMeshPro dialogue font blur and jitter\n• Color compression artifacts on painted portraits and backdrops",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Сжатие ASTC: Отключено\n✓ Динамическое состояние: Базовое\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ ASTC Recompression: Uncompressed (Max art fidelity)\n✓ Dynamic State: EDS1\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01008C300F7F0000ULL,
        "Disco Elysium: The Final Cut (WW)",
        "• Утечка памяти и вылеты при смене локаций\n• Размытие и мерцание текста диалогов TextMeshPro\n• Цветовые артефакты акварельных портретов и фонов",
        "• Out of memory (нехватка памяти) crash on zone transitions\n• TextMeshPro dialogue font blur and jitter\n• Color compression artifacts on painted portraits and backdrops",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Сжатие ASTC: Отключено\n✓ Динамическое состояние: Базовое\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ ASTC Recompression: Uncompressed (Max art fidelity)\n✓ Dynamic State: EDS1\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100E26014466000ULL,
        "Disco Elysium: The Final Cut (Asia)",
        "• Утечка памяти и вылеты при смене локаций\n• Размытие и мерцание текста диалогов TextMeshPro\n• Цветовые артефакты акварельных портретов и фонов",
        "• Out of memory (нехватка памяти) crash on zone transitions\n• TextMeshPro dialogue font blur and jitter\n• Color compression artifacts on painted portraits and backdrops",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Сжатие ASTC: Отключено\n✓ Динамическое состояние: Базовое\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ ASTC Recompression: Uncompressed (Max art fidelity)\n✓ Dynamic State: EDS1\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01003AE017DB0000ULL,
        "Batman: Arkham City",
        "• Просадки FPS при планировании над городом и микрофризы",
        "• FPS drops and micro-stutters while gliding across Arkham City",
        "✓ Точность ЦП: Точная\n✓ Память: 6GB DRAM\n✓ Динамическое состояние: Базовое",
        "✓ CPU Accuracy: Accurate\n✓ Memory Layout: 6GB DRAM\n✓ Dynamic State: EDS1",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\dyna_state", "0"}
        }
    },
    {
        0x0100FF500E34A000ULL,
        "Xenoblade Chronicles: Definitive Edition",
        "• Мерцание текстур открытого мира и артефакты облаков",
        "• Open world texture shimmering and cloud rendering artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Динамическое состояние: Расширенное\n✓ Сжатие ASTC: Отключено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Dynamic State: EDS2\n✓ ASTC Recompression: BC3\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_fast_gpu_time", "true"}
        }
    },
    {
        0x0100E95004038000ULL,
        "Xenoblade Chronicles 2",
        "• Просадки кадровой частоты в густонаселенных локациях (Гула, Мор Ардайн)",
        "• Heavy frame drops in dense titan areas (Gormott, Mor Ardain)",
        "✓ Динамическое состояние: Расширенное\n✓ Сжатие ASTC: Отключено\n✓ Память: 6GB DRAM\n✓ Быстрое время ГПУ: Включено",
        "✓ Dynamic State: EDS2\n✓ ASTC Recompression: BC3\n✓ Memory Layout: 6GB DRAM\n✓ Fast GPU Time: Enabled",
        {
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\use_fast_gpu_time", "true"}
        }
    },
    {
        0x010074F013262000ULL,
        "Xenoblade Chronicles 3",
        "• Утечки VRAM и микростаттеры в битвах с 7 персонажами",
        "• VRAM leaks and micro-stutters during full 7-character battle parties",
        "✓ Сжатие ASTC: Отключено\n✓ Память: 6GB DRAM\n✓ Динамическое состояние: Расширенное\n✓ Точность ГПУ: Высокая",
        "✓ ASTC Recompression: BC3\n✓ Memory Layout: 6GB DRAM\n✓ Dynamic State: EDS2\n✓ GPU Accuracy: High",
        {
            {"Renderer\\astc_recompression", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_fast_gpu_time", "true"}
        }
    },
    {
        0x0100E67012924000ULL,
        "The Witcher 3: Wild Hunt - Complete Edition",
        "• Вылеты по памяти и дедлоки потоков REDengine в Новиграде и Туссенте\n• Мерцание теней и воды",
        "• REDengine thread deadlocks and OOM crashes in Novigrad and Toussaint\n• Shadow and water flickering",
        "✓ Память: 6 ГБ DRAM (устраняет падение аллокатора памяти хоста)\n✓ Fastmem Exclusives: Отключено (устраняет дедлоки диспетчера потоков)\n✓ Игнорировать прерывания памяти: Включено\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM (Prevents host memory allocator OOM)\n✓ Fastmem Exclusives: Disabled (Fixes REDengine thread deadlocks)\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x010074600EE26000ULL,
        "Need for Speed: Hot Pursuit Remastered",
        "• Зависание и бесконечная загрузка при авторизации на серверах EA Autolog\n• Сетевые таймауты при запуске",
        "• Infinite loading hang during EA Autolog server authorization\n• Network connection handshake timeout",
        "✓ Режим полета (В самолете): Включено (пропуск онлайн-проверки)\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM\n✓ Сжатие ASTC: Отключено",
        "✓ Airplane Mode: Enabled (Bypasses EA Autolog offline)\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x010067300059A000ULL,
        "Mario + Rabbids: Kingdom Battle",
        "• Сильный пересвет и ослепляющий блум\n• Черные тени и артефакты освещения\n• Мерцание текстур персонажей",
        "• Severe overexposure and blinding bloom glow\n• Black shadows and lighting pass corruption\n• Character model flickering",
        "✓ Точность ГПУ: Высокая (устранение пересвета и черных теней)\n✓ Обратные циклы барьеров: Включено\n✓ Реактивная очистка: Включено\n✓ Сжатие ASTC: Отключено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High (Fixes bloom & black shadows)\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005CA00F966000ULL,
        "Mario + Rabbids: Sparks of Hope",
        "• Сбои динамического освещения на планетах\n• Черные артефакты в катсценах и битвах\n• Просадки кадров",
        "• Planet lighting pass corruption\n• Black shadow artifacts during cutscenes and tactical battles\n• Frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Реактивная очистка: Включено\n✓ Сжатие ASTC: Отключено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01005CF01E784000ULL,
        "Teenage Mutant Ninja Turtles: Splintered Fate",
        "• Зависание на титульном экране из-за сетевого ожидания NIM и SSL сервисов\n• Микрофризы при спавне врагов",
        "• Title screen freeze caused by NIM and SSL network connection waiting\n• Stutter during enemy combat waves",
        "✓ Режим полета (В самолете): Включено (пропуск сетевого ожидания)\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Airplane Mode: Enabled (Skips online network handshake)\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100000000010000ULL,
        "Super Mario Odyssey",
        "• Мерцание 2D-рисунков на стенах\n• Артефакты дыма и тумана в Песчаном царстве",
        "• Flickering 2D wall drawings\n• Sand Kingdom smoke and fog rendering artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100121014688000ULL,
        "Metroid Prime Remastered",
        "• Статтеры при открытии дверей между отсеками\n• Мерцание эффектов визора",
        "• Door transition compilation stutter\n• Visor UI and particle flicker",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100DCA0064A6000ULL,
        "Luigi's Mansion 3",
        "• Артефакты динамического света фонарика и теней\n• Падение FPS в комнатах с призраками",
        "• Dynamic flashlight beam artifacts\n• FPS drops in ghost-heavy rooms",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01006F8002326000ULL,
        "Animal Crossing: New Horizons",
        "• Размытие травы и мелких объектов\n• Мерцание теней в вечернее время",
        "• Blurry grass and ground textures\n• Evening shadow flicker",
        "✓ Анизотропная фильтрация: 16x\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено",
        "✓ Anisotropic Filtering: 16x\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005CA01580E000ULL,
        "Persona 5 Royal",
        "• Мерцание 2D UI портретов и шрифтов\n• Просадки FPS в людных районах Токио",
        "• 2D UI portrait flicker and font artifacts\n• Heavy crowds FPS drops in Shibuya and Shinjuku",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Рассинхрон звука при загрузке 8 бойцов\n• Микрофризы при активации спецэффектов",
        "• Audio desync with 8 active fighters\n• Visual effect stutter",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100A38018D5A000ULL,
        "EA SPORTS FC 24",
        "• Зависание на заставке EA Connect\n• Утечки памяти в режиме карьеры",
        "• EA Connect splash freeze\n• Career mode memory leaks",
        "✓ Режим полета (В самолете): Включено (пропуск серверов EA)\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ Airplane Mode: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "2"}
        }
    },
    {
        0x01001F5010DFA000ULL,
        "Pokémon Legends: Arceus",
        "• Микрофризы при спавне диких покемонов в небе/траве\n• Артефакты освещения в разломах",
        "• Wild pokemon spawn micro-stutters\n• Space-time distortion lighting artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100A3D008C5C000ULL,
        "Pokémon Scarlet / Violet",
        "• Утечки VRAM в городах и на водных просторах\n• Падение FPS и замедление анимаций",
        "• Severe VRAM leaks in towns and lakes\n• Frame drops and slowed NPC animations",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100ABF008968000ULL,
        "Pokémon Sword / Shield",
        "• Заикания в Диких землях при подгрузке погодных условий\n• Мерцание спецэффектов Dynamax",
        "• Wild Area weather transition stutters\n• Dynamax battle effect flickering",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ GPU Accuracy: High",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01004D300C5AE000ULL,
        "Kirby and the Forgotten Land",
        "• Зависание анимаций врагов на 30 FPS\n• Артефакты отражений луж",
        "• Distant enemy 30 FPS stutter\n• Puddle reflection distortion",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010055D009F78000ULL,
        "Fire Emblem: Three Houses",
        "• Просадки FPS при масштабных битвах батальонов\n• Мерцание 2D портретов и меню",
        "• Battalion battle animation FPS drops\n• 2D UI portrait flickering",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100A6301214E000ULL,
        "Fire Emblem Engage",
        "• Сбои шейдеров свечения колец Emblem\n• Микрофризы в Сомниэле",
        "• Emblem ring glow shader artifacts\n• Somniel hub micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Падение кадров при цепных комбо-атаках легионов\n• Размытие динамических неоновых вывесок",
        "• Legion chain attack FPS drops\n• Blurry dynamic neon bloom reflections",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01006F801BC4C000ULL,
        "Shin Megami Tensei V: Vengeance",
        "• Нагрев и просадки кадров в песчаных бурях Даата\n• Черные тени демонов",
        "• Da'at sandstorm thermal throttle and FPS drops\n• Demon shadow corruption",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"System\\memory_layout_mode", "2"}
        }
    },
    {
        0x01006C300E9F0000ULL,
        "Dragon Quest XI S: Echoes of an Elusive Age",
        "• Микростаттеры при смене зон и переходе между 2D/3D режимами\n• Мерцание травы",
        "• Zone transition micro-stutters\n• Grass texture flickering",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100CC80140F8000ULL,
        "Triangle Strategy",
        "• Пересвет HD-2D эффектов глубины резкости\n• Мерцание теней на изометрических картах",
        "• HD-2D depth-of-field overexposure\n• Isometric grid shadow flickering",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100BB70144F8000ULL,
        "Prince of Persia: The Lost Crown",
        "• Статтеры при активации способностей управления временем\n• Искажение фоновых слоев",
        "• Time powers activation stutter\n• Background parallax distortion",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01007820195A6000ULL,
        "Red Dead Redemption",
        "• Сбои освещения на закате и рассвете\n• Просадки FPS в городах Блэкуотер и Армадилло",
        "• Sunrise/sunset volumetric lighting corruption\n• Blackwater town FPS drops",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010023A017E94000ULL,
        "Batman: Arkham Knight",
        "• Критические просадки FPS при езде на Бэтмобиле\n• Вылеты по нехватке VRAM в Готэме",
        "• Severe Batmobile driving FPS drops\n• Gotham City open-world OOM crashes",
        "✓ Память: 8GB DRAM\n✓ Сжатие ASTC: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Memory Layout: 8GB DRAM\n✓ ASTC Recompression: Uncompressed\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\memory_layout_mode", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01003AE017DB0000ULL,
        "Batman: Arkham City",
        "• Мерцание снега и тумана над городом\n• Микрофризы при планировании с плащом",
        "• Snow particle and fog flickering\n• Glide traversal micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010071700F8BA000ULL,
        "Crash Bandicoot 4: It's About Time",
        "• Рассинхрон инпут-лага в сложных платформенных секциях\n• Размытие фонов",
        "• Frame pacing latency in tight platforming sections\n• Blurry background assets",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100B6E00B360000ULL,
        "Spyro Reignited Trilogy",
        "• Заикания звука в катсценах\n• Мерцание теней на травяных холмах",
        "• Audio stutter in cutscenes\n• Shadow shimmering on grassy hills",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ GPU Accuracy: High",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x0100770008DD8000ULL,
        "Monster Hunter Generations Ultimate",
        "• Мерцание текстур монстров и эффектов крови\n• Просадки в мультиплеере",
        "• Monster skin texture flicker\n• Multiplayer combat frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Анизотропная фильтрация: 16x\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Anisotropic Filtering: 16x\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\max_anisotropy", "5"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010034501659E000ULL,
        "Crisis Core: Final Fantasy VII Reunion",
        "• Статтеры при вращении рулетки DMW (Digital Mind Wave)\n• Сбои освещения в Мидгаре",
        "• DMW reel spinning stutter\n• Midgar volumetric lighting corruption",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01003D100E9C6000ULL,
        "The Witcher 3: Wild Hunt",
        "• Вылеты памяти (OOM) и зависания диспетчера потоков REDengine\n• Мерцание теней и воды в Новиграде",
        "• REDengine thread deadlocks and host OOM memory crashes\n• Shadow and water flickering in Novigrad",
        "✓ Память: 6 ГБ DRAM (устраняет сбой аллокатора памяти)\n✓ Fastmem Exclusives: Отключено (устраняет дедлоки диспетчера потоков)\n✓ Игнорировать прерывания памяти: Включено\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Memory Layout: 6GB DRAM (Prevents host allocator OOM)\n✓ Fastmem Exclusives: Disabled (Fixes REDengine thread deadlocks)\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x010074F013262000ULL,
        "Xenoblade Chronicles 3",
        "• Микростаттеры и утечки VRAM в масштабных битвах\n• Артефакты частиц Ouroboros",
        "• Battle scene micro-stutters and VRAM leaks\n• Ouroboros transformation particle artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "2"}
        }
    },
    {
        0x0100FF500E34A000ULL,
        "Xenoblade Chronicles: Definitive Edition",
        "• Размытие текстур персонажей на расстоянии\n• Мерцание травяного покрова на равнинах Гуры",
        "• Distant character texture blur\n• Bionis Leg grass shimmering",
        "✓ Точность ГПУ: Высокая\n✓ Анизотропная фильтрация: 16x\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Anisotropic Filtering: 16x\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\max_anisotropy", "5"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100B04011742000ULL,
        "Monster Hunter Rise",
        "• Просадки FPS при битвах с Wyvern\n• Мерцание спецэффектов Wirebug",
        "• Wyvern combat FPS drops\n• Wirebug particle flickering",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100BB600DC30000ULL,
        "DOOM Eternal",
        "• Вылет после заставки об эпилепсии из-за переполнения пула памяти (OOM)\n• Ослепляющие вспышки плазменных взрывов и статтеры Glory Kill",
        "• Crash after epilepsy warning due to 4GB memory pool exhaustion (OOM)\n• Blinding plasma explosion flashes and Glory kill stutters",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устраняет вылет после предупреждения об эпилепсии)\n✓ Игнорировать прерывания памяти: Включено\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Точность DMA: Safe\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents crash after epilepsy warning)\n✓ Ignore Memory Aborts: Enabled\n✓ Dynamic State: Basic (EDS1)\n✓ DMA Accuracy: Safe\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x01005C9014168000ULL,
        "NieR:Automata The End of YoRHa Edition",
        "• Задержка отклика в секциях пулевого ада\n• Мерцание пустынного песка",
        "• Bullet-hell section frame pacing latency\n• Desert sand shimmer",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100224016A90000ULL,
        "Persona 4 Golden",
        "• Размытие текстур в ТВ-мире\n• Артефакты меню",
        "• TV World blurry textures\n• UI menu texture artifacts",
        "✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100A7301646E000ULL,
        "Unicorn Overlord",
        "• Размытие спрайтов персонажей на глобальной карте\n• Микростаттеры битв",
        "• Overworld sprite blur\n• Battle start micro-stutters",
        "✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено\n✓ Анизотропная фильтрация: 16x",
        "✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Anisotropic Filtering: 16x",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x01004F3017772000ULL,
        "Dave the Diver",
        "• Статтеры при глубоководных погружениях\n• Артефакты эффектов пузырей",
        "• Deep sea diving stutters\n• Underwater bubble particle glitches",
        "✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено\n✓ Асинхронные шейдеры: Включено",
        "✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01006BB00C6F0000ULL,
        "The Legend of Zelda: Link's Awakening",
        "• Просадки FPS при размытии глубины резкости (Tilt-Shift)\n• Заикания в деревне Мэйб",
        "• Tilt-shift depth of field severe frame drops\n• Mabe Village traversal stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Память: 4 ГБ DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01008CF01BAAC000ULL,
        "The Legend of Zelda: Echoes of Wisdom",
        "• Падения частоты кадров при создании копий предметов (Echoes)\n• Размытие воды",
        "• Echo creation frame drops\n• Water surface reflection distortion",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Память: 4 ГБ DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01007760086F8000ULL,
        "Bayonetta 2",
        "• Сбои динамического освещения и теней в катсценах\n• Просадки FPS при битвах с ангелами",
        "• Dynamic lighting and shadow glitches in cutscenes\n• Angel combat scene frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100BC0018138000ULL,
        "Super Mario RPG",
        "• Статтеры анимаций диалогов\n• Сбои синхронизации изометрического освещения",
        "• Dialogue animation stutters\n• Isometric lighting pass desync",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010069200E60E000ULL,
        "Pikmin 4",
        "• Утечки памяти в открытых садах Unreal Engine 4\n• Микрофризы при спавне отряда Пикминов",
        "• Unreal Engine 4 open garden memory leaks\n• Pikmin squad spawn micro-stutters",
        "✓ Память: 6GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100FA701A7CA000ULL,
        "Princess Peach: Showtime!",
        "• Просадки FPS при смене театральных декораций и костюмов\n• Мерцание теней сцены",
        "• Theater stage transition frame drops\n• Stage lighting shadow flicker",
        "✓ Память: 4GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 4GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100C2A01A03A000ULL,
        "Luigi's Mansion 2 HD",
        "• Мерцание луча фонарика Dark-Light\n• Артефакты призрачных следов",
        "• Dark-Light flashlight beam flickering\n• Ghost trail rendering artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01000BC0000A0000ULL,
        "Hollow Knight",
        "• Задержка ввода (задержки ввода) в босс-файтах\n• Микростаттеры при смене комнат",
        "• Boss fight frame latency\n• Room transition compilation stutters",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100735010B46000ULL,
        "Hades",
        "• Микрофризы при комнатах с большим числом снарядов и спецэффектов",
        "• High particle and projectile chamber micro-stutters",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010041B01211E000ULL,
        "Sea of Stars",
        "• Размытие пиксель-арта\n• Рассинхронизация динамического освещения солнца/луны",
        "• Pixel art sprite blur\n• Sun/moon eclipse dynamic lighting desync",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100656017E9C000ULL,
        "Octopath Traveler II",
        "• Пересвет и размытие глубины резкости в HD-2D\n• Микростаттеры при смене дня и ночи",
        "• HD-2D depth-of-field overexposure\n• Day/night transition micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010052C00BAB8000ULL,
        "Bravely Default II",
        "• Утечки памяти в Unreal Engine 4 на карте мира\n• Просадки FPS в битвах",
        "• Unreal Engine 4 overworld memory leaks\n• Battle start FPS drops",
        "✓ Память: 4GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 4GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01003D200BAA2000ULL,
        "Dead Cells",
        "• Задержка ввода в динамичных боевых секциях\n• Статтеры процедурной генерации",
        "• Combat frame latency\n• Procedural level generation stutters",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01001B60133A2000ULL,
        "Outer Wilds",
        "• Просадки FPS при физическом расчете орбит планет\n• Утечки VRAM в туманностях",
        "• Solar system physics calculation slowdown\n• Space nebulae VRAM leaks",
        "✓ Память: 6GB DRAM\n✓ Точность ЦП: Точная\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ CPU Accuracy: Accurate\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010067600DF2A000ULL,
        "Subnautica",
        "• Задержка прогрузки чанков морского дна\n• Микрофризы при управлении батискафом",
        "• Ocean floor chunk loading delays\n• Seamoth traversal stutters",
        "✓ Память: 6GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01009850119BC000ULL,
        "Ori and the Will of the Wisps",
        "• Просадки FPS при скоростном перемещении по локациям\n• Мерцание фонового света",
        "• Fast traversal frame drops\n• Volumetric light shimmering",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x01000CD00DE1E000ULL,
        "Alien: Isolation",
        "• Артефакты динамических теней на станции «Севастополь»\n• Шум отражений",
        "• Dynamic shadow artifacts in dark corridors\n• Specular reflection noise",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Память: 4GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Anisotropic Filtering: 16x\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01005AF00BA7A000ULL,
        "Metroid Dread",
        "• Просадки FPS до 15-20 к/с на Snapdragon 8 Gen 3 (Adreno 750) из-за высокой точности ГПУ\n• Микрофризы при входе в зоны E.M.M.I.",
        "• Framerate drops to 15-20 FPS on Snapdragon 8 Gen 3 (Adreno 750) due to high GPU accuracy\n• E.M.M.I. zone transition micro-stutters",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS на Snapdragon 8 Gen 3 / Adreno 750)\n✓ Реактивная очистка: Отключено\n✓ Быстрое время ГПУ: Включено\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS on Snapdragon 8 Gen 3 / Adreno 750)\n✓ Reactive Flushing: Disabled\n✓ Fast GPU Time: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010077200889E000ULL,
        "Donkey Kong Country: Tropical Freeze",
        "• Просадки частоты кадров до 15 FPS на Snapdragon 8 Gen 3 (Adreno 750) из-за высокой точности ГПУ\n• Микростаттеры при полетах на бочках",
        "• Heavy framerate drops to 15 FPS on Snapdragon 8 Gen 3 (Adreno 750) due to high GPU accuracy\n• Barrel blast transition micro-stutters",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS на Snapdragon 8 Gen 3 / Adreno 750)\n✓ Реактивная очистка: Отключено\n✓ Быстрое время ГПУ: Включено\n✓ Обратные циклы барьеров: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS on Snapdragon 8 Gen 3 / Adreno 750)\n✓ Reactive Flushing: Disabled\n✓ Fast GPU Time: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x0100C1F0051B2000ULL}
    },
    {
        0x0100483017770000ULL,
        "Kirby's Return to Dream Land Deluxe",
        "• Размытие контуров сел-шейдинга персонажей\n• Просадки FPS при суперспособностях Кирби",
        "• Cel-shaded outline blurring\n• Super ability particle frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010085C0084FA000ULL,
        "Captain Toad: Treasure Tracker",
        "• Мерцание теней на трехмерных диорамах уровней\n• Артефакты глубины резкости",
        "• Shadow flickering on 3D diorama puzzles\n• Depth-of-field blur artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01006000040C2000ULL,
        "Yoshi's Crafted World",
        "• Утечки памяти в Unreal Engine 4 на картонных декорациях\n• Размытие текстур заднего плана",
        "• Unreal Engine 4 cardboard diorama VRAM leaks\n• Background blur texture shimmering",
        "✓ Память: 4GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 4GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010019401051C000ULL,
        "Mario Strikers: Battle League",
        "• Рассинхронизация анимации гиперударов (Hyper Strikes)\n• Статтеры при взрывах спецэффектов",
        "• Hyper Strike comic animation desync\n• Stadium particle burst stutters",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100E2B017DAE000ULL,
        "Super Bomberman R 2",
        "• Просадки FPS при массовых взрывах в режиме «Замок»\n• Рассинхронизация сетевого таймера и зависание на проверке серверов Konami",
        "• Castle mode multi-explosion slowdowns\n• Konami server handshake stall and local multiplayer sync latency",
        "✓ Режим полёта: Включено (пропуск ожидания серверов Konami)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Airplane Mode: Enabled (Bypasses Konami server check)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01002390146AC000ULL,
        "Advance Wars 1+2: Re-Boot Camp",
        "• Задержки загрузки анимаций командиров (CO Powers)\n• Размытие 2D-спрайтов техники",
        "• CO Power full-screen anime animation delay\n• Tactical map sprite blur",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01002D801A1DE000ULL,
        "WarioWare: Move It!",
        "• Задержка отклика в микроиграх с быстрой сменой позы\n• Пропуск кадров на переходах",
        "• Microgame form change input latency\n• Transition animation frame skips",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100D6F015F70000ULL,
        "No Man's Sky",
        "• Вылеты по нехватке памяти при входе в атмосферу планет\n• Артефакты процедурной генерации",
        "• Planetary entry OOM memory crashes\n• Procedural voxel terrain artifacting",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "2"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01000BD016550000ULL,
        "Portal: Companion Collection",
        "• Сбои рекурсивного рендеринга порталов\n• Рассинхронизация физики кубов в Source Engine",
        "• Recursive portal view rendering glitches\n• Source Engine physics stutter",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010058C01570E000ULL,
        "Persona 4 Golden",
        "• Микростаттеры при перемещении по туманной Инабе\n• Рассинхронизация звука аниме-вставок",
        "• Foggy Inaba traversal micro-stutters\n• Anime video cutscene audio desync",
        "✓ Быстрая память: Включено\n✓ Точность ГПУ: Высокая\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ GPU Accuracy: High\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x010072001570A000ULL,
        "Persona 3 Portable",
        "• Размытие 2D-портретов и визуальной новеллы\n• Просадки FPS на верхних этажах Тартара",
        "• Visual novel 2D portrait blur\n• Tartarus upper floor frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01004C80170A6000ULL,
        "Tunic",
        "• Сбои страниц руководства в изометрическом виде\n• Шум объемного освещения",
        "• Instruction manual overlay rendering glitches\n• Isometric volumetric light noise",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100DDF01A03A000ULL,
        "Dave the Diver",
        "• Задержка прогрузки глубоководных биомов\n• Микрофризы в суши-ресторане Банчо",
        "• Deep sea biome transition latency\n• Bancho Sushi rush hour micro-stutters",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01002BE01584E000ULL,
        "Cult of the Lamb",
        "• Просадки FPS при большом числе последователей в поселении\n• Сбои динамических теней",
        "• Cult camp high follower count slowdown\n• Ritual dynamic shadow flickering",
        "✓ Быстрая память: Включено\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено",
        "✓ Fastmem: Enabled\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100913018F08000ULL,
        "Unicorn Overlord",
        "• Рассинхронизация 2D-анимаций Vanillaware в масштабных битвах\n• Размытие шрифтов интерфейса",
        "• Vanillaware 2D battle animation desync\n• Tactical interface font blur",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x\n✓ Fastmem: Enabled",
    },
    {
        0x01002DA013484000ULL,
        "The Legend of Zelda: Skyward Sword HD",
        "• Артефакты меча и курсора при управлении движением\n• Размытие текстур облачного моря Небоземи",
        "• Motion control sword and pointer jitter\n• Skyloft cloud sea texture shimmering",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010028600EBDA000ULL,
        "Super Mario 3D World + Bowser's Fury",
        "• Просадки FPS при появлении Яростного Боузера в открытом море\n• Мерцание шейдеров дождя и лавы",
        "• Bowser's Fury open sea stormy weather FPS drops\n• Rain splash and lava dynamic shader flickering",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010036601D380000ULL,
        "Super Mario Party Jamboree",
        "• Микрофризы в мини-играх на 20 игроков Koopathlon\n• Рассинхронизация счетчика очков",
        "• 20-player Koopathlon minigame stutters\n• Live board score counter desync",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x010091801E8B2000ULL,
        "Mario & Luigi: Brothership",
        "• Задержка отклика в совместных Brother Attacks\n• Просадки FPS на морских островах Конкордии\n• Зависание на загрузке у MOD версий с большим RomFS",
        "• Brother Attacks timing lag\n• Concordia ocean sailing FPS dips\n• Loading freeze on MOD versions with large RomFS",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено\n✓ Память: 6 ГБ\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory: 6 GB\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x01006D0017F7A000ULL}
    },
    {
        0x010036B0034E4000ULL,
        "Super Mario Party",
        "• Микрозадержки анимаций кубиков и персонажей\n• Сбои полупрозрачности воды в речных сплавах",
        "• Dice roll animation micro-stutters\n• River Survival water transparency glitches",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01006FE013472000ULL,
        "Mario Party Superstars",
        "• Мерцание теней на классических досках N64\n• Размытие миниатюр правил мини-игр",
        "• Retro N64 board shadow flickering\n• Minigame instruction modal blur",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100F7701140E000ULL,
        "Mario Golf: Super Rush",
        "• Размытие сетки рельефа грина при прицеливании\n• Просадки FPS при массовом забеге Speed Golf",
        "• Green terrain grid blur\n• Speed Golf stamina sprint slowdown",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100BDE00862A000ULL,
        "Mario Tennis Aces",
        "• Инпут-лаг в замедлении времени Zone Speed\n• Артефакты свечения ракетки при спец-ударах",
        "• Zone Speed slow-motion input lag\n• Special Shot racket glow artifacts",
        "✓ Точность ЦП: Высокая\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01003C700009C000ULL,
        "Splatoon 2",
        "• Мерцание отражений чернил на металлических поверхностях\n• Микрофризы в хабе площади Инкополиса",
        "• Metallic surface ink reflection flicker\n• Inkopolis Square hub micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100C2500FC20000ULL,
        "Splatoon 3",
        "• Утечки памяти в хабе Плюхграда и в матчах Залива Самонид\n• Просадки FPS при обильном залитии карты краской и сетевые задержки лобби",
        "• Splatsville hub VRAM leaks\n• Salmon Run heavy ink coverage slowdown & match lobby delay",
        "✓ Режим полёта: Включено (офлайн-кампания без сетевых задержек)\n✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Airplane Mode: Enabled (Instant offline Hero mode)\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100A3900C3E2000ULL,
        "Paper Mario: The Origami King",
        "• Сбои отрисовки кольцевой арены в битвах\n• Артефакты конфетти и бумажных складок",
        "• Ring puzzle battle arena glitches\n• Confetti paper fold texture artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01002B00111A2000ULL,
        "Hyrule Warriors: Age of Calamity",
        "• Тяжелые просадки FPS при массовом скоплении монстров на экране\n• Утечки VRAM в битвах Чудищ",
        "• Massive enemy swarm heavy slowdown\n• Divine Beast battle VRAM exhaustion",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100AE0009A80000ULL,
        "Hyrule Warriors: Definitive Edition",
        "• Размытие спецэффектов комбо Focus Spirit\n• Микрофризы при спавне отрядов офицеров",
        "• Focus Spirit attack bloom blur\n• Officer squad spawn micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01007E3015C3E000ULL,
        "Fire Emblem Warriors: Three Hopes",
        "• Падение производительности в масштабных боях на картах Фодлана\n• Артефакты теней полководцев",
        "• Large scale Fodlan battlefield FPS dips\n• Commander shadow map rendering errors",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000011D90000ULL,
        "Pokemon Brilliant Diamond",
        "• Мерцание чиби-отражений в лужах и окнах Синно\n• Задержка открытия меню покедекса",
        "• Chibi puddle & window reflection flicker\n• Pokedex animation transition lag",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010018E011D92000ULL,
        "Pokemon Shining Pearl",
        "• Мерцание чиби-отражений в лужах и окнах Синно\n• Задержка открытия меню покедекса",
        "• Chibi puddle & window reflection flicker\n• Pokedex animation transition lag",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x010003F003A34000ULL,
        "Pokemon Let's Go, Pikachu!",
        "• Задержка круга прицеливания при броске покебола\n• Мерцание травы на маршрутах Канто",
        "• Pokeball throw capture ring input lag\n• Kanto route grass shader flickering",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100151003A36000ULL,
        "Pokemon Let's Go, Eevee!",
        "• Задержка круга прицеливания при броске покебола\n• Мерцание травы на маршрутах Канто",
        "• Pokeball throw capture ring input lag\n• Kanto route grass shader flickering",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100B870097D6000ULL,
        "Shin Megami Tensei V",
        "• Просадки FPS на песчаных барханах пустыни Да'ат в Unreal Engine 4\n• Размытие магических заклинаний Нахобино",
        "• Da'at desert sand dunes UE4 slowdown\n• Nahobino Magatsuhi skill blur",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100C48008890000ULL,
        "Xenoblade Chronicles 2: Torna ~ The Golden Country",
        "• Утечки памяти на гигантских просторах Титана Торны\n• Размытие облачного покрова и листвы",
        "• Torna Titan open terrain VRAM leaks\n• Cloud sea and foliage shimmering",
        "✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010049900F546000ULL,
        "Super Mario 3D All-Stars",
        "• Мерцание текстур воды в Mario Sunshine\n• Рассинхронизация звуковых дорожек в Mario Galaxy",
        "• Sunshine water reflection texture flickering\n• Galaxy orchestrated audio drift",
        "✓ Точность ЦП: Высокая\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010065B00AE8E000ULL,
        "Cuphead",
        "• Инпут-лаг при скоростных уклонениях и парированиях розовых снарядов\n• Размытие пленочного зерна 1930-х",
        "• Precision parry / dash input lag\n• 1930s film grain shader blurring",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100D5700DC34000ULL,
        "Ori and the Blind Forest: Definitive Edition",
        "• Сбои изометрических источников света в лесу Нибель\n• Микрофризы при скоростном Bash-прыжке",
        "• Nibel forest dynamic volumetric light glitches\n• High-speed Bash chain micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100216008E88000ULL,
        "Slay the Spire",
        "• Микрозадержки анимаций розыгрыша карт и реликвий\n• Размытие текстовых описаний баффов",
        "• Card play and relic particle animation delay\n• Buff description text blur",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01008C300B4D2000ULL,
        "Into the Breach",
        "• Задержка прорисовки изометрической тактической сетки 8x8\n• Размытие анимаций атаки Мехов",
        "• 8x8 tactical grid UI latency\n• Mech attack pixel-art animation blur",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100C4D01D026000ULL,
        "Tales of Series (Berseria / Graces f / Symphonia)",
        "• Зависание на 0 FPS и черный экран при смене локаций\n• Рассинхронизация видео-переходов NVDEC",
        "• 0 FPS freeze and black screen on location transitions\n• NVDEC video transition desync",
        "✓ Сжатие ASTC: Отключено (устраняет зависание при смене локаций)\n✓ Реактивная очистка: Включено\n✓ Эмуляция NVDEC: ГПУ видеоядро (NVDEC)\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ ASTC Recompression: Uncompressed (Fixes location transition freeze!)\n✓ Reactive Flushing: Enabled\n✓ NVDEC Emulation: GPU Video Core\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Просадки FPS при вызове Легионов и комбо-атаках в Арке\n• Утечки VRAM при длительной игре",
        "• Legion summon and chain sync combo FPS drops\n• Long session VRAM exhaustion in the Ark",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005FA00BAFA000ULL,
        "Fire Emblem: Three Houses",
        "• Утечки памяти при перемещении по монастырю Гаррег Мах\n• Просадки FPS на тактической сетке боев",
        "• Garreg Mach Monastery roaming memory leaks\n• Tactical grid battle FPS dips",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100F4C009322000ULL,
        "Pikmin 3 Deluxe",
        "• Размытие текстур фруктов и сока\n• Мерцание теней в заданиях Олимара",
        "• Fruit texture and juice rendering blur\n• Olimar side-story shadow shimmering",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01007E3006DDA000ULL,
        "Kirby Star Allies",
        "• Рассинхронизация спецэффектов сердец дружбы Friend Heart\n• Микрофризы в битвах четверки героев",
        "• Friend Heart dynamic glow desync\n• 4-player team battle micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x0100ABF008968000ULL,
        "Pokemon Sword & Shield",
        "• Просадки FPS при динамической смене погоды в Диких землях Галара\n• Мерцание свечения Гигантамакса",
        "• Galar Wild Area weather dynamic lighting drops\n• Dynamax aura rendering flicker",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
    {
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Инпут-лаг и просадки кадров в боях на 8 игроков\n• Зависание катсцен и задержки проверки Духов онлайн",
        "• 8-fighter intense brawl input latency\n• Spirit Board online latency & cutscene sync pause",
        "✓ Режим полёта: Включено (пропуск онлайн-опроса Духов)\n✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Spirit Board network check)\n✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01004C00000CE000ULL,
        "Dragon Quest XI S: Echoes of an Elusive Age",
        "• Микрофризы при быстрой скачке на лошади по королевству Гелиодор в UE4\n• Размытие волос и брони",
        "• Heliodor kingdom high-speed horse ride micro-stutters\n• Character hair and armor texture blur",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01007D401662A000ULL,
        "NieR:Automata The End of YoRHa Edition",
        "• Просадки FPS при скоростном полете на поде по Руинам Города\n• Артефакты частиц bullet-hell сфер",
        "• Ruined City flight unit sequence FPS drops\n• Bullet-hell glowing sphere particle artifacts",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100778006E88000ULL,
        "Bayonetta 1 & 2",
        "• Инпут-лаг в замедлении времени Witch Time\n• Пересвет эффектов Umbran Climax",
        "• Witch Time slow-motion input lag\n• Umbran Climax overexposed bloom",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005CA0157CA000ULL,
        "Persona 5 Royal",
        "• Рассинхронизация комикс-переходов и анимаций All-Out Attack\n• Микрофризы во Дворцах Метаверсума",
        "• All-Out Attack comic transition desync\n• Metaverse Palace shadow infiltration micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100780016140000ULL,
        "Live A Live",
        "• Размытие пиксельных спрайтов при переходе между эпохами\n• Микрофризы в боях на шахматной сетке",
        "• HD-2D era chapter transition sprite blur\n• Grid battle attack sequence stutters",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01000BD00E756000ULL,
        "Tony Hawk's Pro Skater 1 + 2",
        "• Зависание на заставке и бесконечный спиннер авторизации Activision\n• Сетевой таймаут при старте",
        "• Infinite Activision authorization spinner on startup\n• Online handshake deadlock",
        "✓ Режим полёта: Включено (пропуск онлайн-авторизации Activision)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ Airplane Mode: Enabled (Bypasses Activision online login)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100F7A00B704000ULL,
        "Crash Team Racing Nitro-Fueled",
        "• Зависание в главном меню при опросе серверов Pit Stop\n• Микрофризы анимаций подиума",
        "• Pit Stop server telemetry handshake freeze\n• Podium animation micro-stutters",
        "✓ Режим полёта: Включено (пропуск серверов Pit Stop)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Airplane Mode: Enabled (Bypasses Pit Stop online check)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }

    },
    {
        0x01007E300B70C000ULL,
        "Borderlands Legendary Collection (1, 2, TPS)",
        "• Задержка старта на 60+ секунд и зависание при сетевом опросе SHiFT\n• Просадки FPS при взрывах стихий",
        "• 60+ second boot freeze during SHiFT network ping\n• Elemental explosion particle slowdowns",
        "✓ Режим полёта: Включено (мгновенный старт без ожидания SHiFT)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Airplane Mode: Enabled (Instant boot without SHiFT timeout)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x01001B300B9BE000ULL,
        "Diablo III: Eternal Collection",
        "• Задержка и зависание на экране сезонов при проверке Battle.net\n• Микрофризы при спавне элитных паков",
        "• Battle.net seasonal handshake timeout freeze\n• Elite mob pack spawn stutter",
        "✓ Режим полёта: Включено (пропуск серверов Battle.net)\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Airplane Mode: Enabled (Skips Battle.net server check)\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        },
        {0x010032F00C04A000ULL, 0x01001B700A654000ULL}
    },
    {
        0x01005E4017C7A000ULL,
        "Demon Slayer: Kimetsu no Yaiba - The Hinokami Chronicles",
        "• Зависание на заставке при онлайн-верификации CyberConnect2\n• Просадки кадров в спецприемах дыхания",
        "• Title screen freeze during CyberConnect2 online verification loop\n• Breathing form ultimate attack FPS drops",
        "✓ Режим полёта: Включено (пропуск онлайн-верификации)\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Skips online verification)\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100830007780000ULL,
        "Monster Hunter Rise / Sunbreak",
        "• Микрофризы при опросе сетевого лобби Hunter Search\n• Утечки VRAM в локациях Джунглей и Цитадели",
        "• Hunter Search lobby broadcast micro-freezes\n• Jungle and Citadel VRAM leaks",
        "✓ Режим полёта: Включено (устранение микрофризов Hunter Search)\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Fixes Hunter Search lobby lag)\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010052900FA62000ULL,
        "Burnout Paradise Remastered",
        "• Зависание на заставке при обращении к серверам новостей EA Paradise City\n• Микрофризы в авариях",
        "• EA Paradise City news server connection freeze\n• Crash sequence micro-stutters",
        "✓ Режим полёта: Включено (пропуск серверов EA)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Airplane Mode: Enabled (Bypasses EA online news)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100650012270000ULL,
        "Overcooked! All You Can Eat",
        "• Зависание на этапе инициализации Team17 Cross-Play сервисов на Android\n• Рассинхронизация физики кухни",
        "• Team17 Cross-Play service handshake freeze on Android\n• Kitchen item physics desync",
        "✓ Режим полёта: Включено (пропуск Team17 Cross-Play)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Team17 Cross-Play)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01001FB012014000ULL,
        "Plants vs. Zombies: Battle for Neighborville",
        "• Зависание на стартовом экране из-за онлайн-требований Frostbite EA\n• Утечки памяти в хабе Беспечного парка",
        "• Frostbite EA online login requirement deadlock on title screen\n• Giddy Park hub memory leaks",
        "✓ Режим полёта: Включено (пропуск онлайн-авторизации EA)\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses EA online login requirement)\n✓ Memory Layout: 8GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100D5600DE44000ULL,
        "Risk of Rain 2",
        "• Сетевой таймаут PlayFab в главном меню\n• Просадки FPS при спавне сотен монстров на 5+ стадии",
        "• PlayFab matchmaking network timeout\n• High-stage horde particle slowdowns",
        "✓ Режим полёта: Включено (пропуск PlayFab таймаутов)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Airplane Mode: Enabled (Bypasses PlayFab timeout)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x0100E2600AE5C000ULL,
        "Minecraft Dungeons",
        "• Зацикливание авторизации учетной записи Microsoft / Xbox Live\n• Микрофризы процедурных подземелий",
        "• Microsoft / Xbox Live telemetry sign-in loop\n• Procedural dungeon generation stutters",
        "✓ Режим полёта: Включено (пропуск авторизации Microsoft)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Microsoft telemetry)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01006B4009B74000ULL,
        "Dragon Ball FighterZ",
        "• Зависание на старте при поиске сетевого лобби Bandai Namco\n• Инпут-лаг в комбо",
        "• Bandai Namco online lobby connection loop on title screen\n• Combo input latency",
        "✓ Режим полёта: Включено (пропуск сетевого лобби)\n✓ Точность ЦП: Точная\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses online lobby search)\n✓ CPU Accuracy: Accurate\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010077C000B90000ULL,
        "Dragon Ball Xenoverse 2",
        "• Зависание в городе Контон при опросе сетевых серверов\n• Просадки FPS в рейдах на 6 игроков",
        "• Conton City server polling freeze\n• 6-player raid boss FPS drops",
        "✓ Режим полёта: Включено (пропуск сетевого сервера Контона)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ Airplane Mode: Enabled (Bypasses Conton City server)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100B1A01B20A000ULL,
        "NBA 2K24 / 2K23",
        "• Зависание на экране обновления ростеров (пропуск серверов 2K Sports)\n• Утечки памяти в режиме MyCAREER",
        "• 2K Sports roster server handshake freeze\n• MyCAREER mode memory leaks",
        "✓ Режим полёта: Включено (пропуск серверов 2K Sports)\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses 2K server handshake)\n✓ Memory Layout: 8GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100827018DEE000ULL,
        "Just Dance Series (2020-2024)",
        "• Зависание на заставке при подключении к Ubisoft Connect\n• Рассинхронизация видеопотока клипов",
        "• Ubisoft Connect server connection loop on boot\n• Video stream audio sync delay",
        "✓ Режим полёта: Включено (пропуск серверов Ubisoft Connect)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Ubisoft Connect)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005B900C09A000ULL,
        "Clubhouse Games: 51 Worldwide Classics",
        "• Задержка и зависание при локальном сетевом поиске столов\n• Размытие текстур игровых досок",
        "• Local lobby discovery loop freeze on startup\n• Board texture and piece blur",
        "✓ Режим полёта: Включено (мгновенная загрузка офлайн)\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Instant offline boot)\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100BDE00862A000ULL,
        "Mario Tennis Aces",
        "• Зависание на проверке онлайн-турниров при старте\n• Просадки FPS при Zone Shot спецэффектах",
        "• Tournament online leaderboard check hang\n• Zone Shot particle slowdowns",
        "✓ Режим полёта: Включено (пропуск онлайн-проверки турниров)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Airplane Mode: Enabled (Skips tournament online check)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "1"}
        }
    },
    {
        0x0100874011EE6000ULL,
        "Mario Golf: Super Rush",
        "• Задержка и зависание при сетевом поиске ранговых матчей\n• Мерцание дальних флажков и травы",
        "• Ranked match network polling hang\n• Course grass and flag shadow flickering",
        "✓ Режим полёта: Включено (мгновенный старт офлайн)\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Instant offline start)\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01007E3019808000ULL,
        "Shin Megami Tensei V: Vengeance",
        "• Вылеты из-за нехватки памяти (нехватка памяти) на открытых картах Да'ат в UE4\n• Розовые артефакты текстур",
        "• Unreal Engine 4 Da'at open-world Out-Of-Memory crashes\n• Corrupted/pink ASTC texture tiles",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100121014688000ULL,
        "Metroid Prime Remastered",
        "• Артефакты визора и HUD-интерфейса\n• Микрофризы при открытии переходных дверей между комнатами",
        "• Combat/Scan visor HUD graphical glitches\n• Room transition door loading stutters",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено (исправление визора)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed (Fixes visor artifacts)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01005E3017B46000ULL,
        "Crisis Core -Final Fantasy VII- Reunion",
        "• Микрозадержки при срабатывании барабана рулетки DMW в бою\n• Мерцание динамического освещения в UE4",
        "• DMW combat reel digital mind wave hitches\n• UE4 dynamic lighting and shadow flickering",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100D15017D3E000ULL,
        "Octopath Traveler II",
        "• Размытие пиксель-арт спрайтов персонажей и HD-2D окружения\n• Швы на текстурах воды",
        "• HD-2D sprite blur and character outline softening\n• Water surface shader seam lines",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100570017106000ULL,
        "Tactics Ogre: Reborn",
        "• Микрозадержки при исполнении спец-ударов и магии\n• Сглаживание пиксельных шрифтов диалогов",
        "• Finishing moves particle stutters\n• Retro dialogue font smoothing",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100AE600BA12000ULL,
        "Dragon's Dogma: Dark Arisen",
        "• Зависание на старте при сетевой верификации серверов Пешек (Pawns)\n• Просадки FPS в битвах с химерами и грифонами",
        "• Pawn server network verification hang on boot\n• Chimera and Griffin boss particle slowdowns",
        "✓ Режим полёта: Включено (пропуск серверов Пешек)\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Pawn server checks)\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01000B300B250000ULL,
        "Warframe",
        "• Бесконечный цикл ожидания подключения к серверам при старте\n• Утечки памяти в реле",
        "• Infinite server login connection loop on title screen\n• Relay hub memory pressure",
        "✓ Режим полёта: Включено (пропуск сетевого ожидания)\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено",
        "✓ Airplane Mode: Enabled (Bypasses network connection retry)\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010077001A8D4000ULL,
        "Brotato",
        "• Просадки FPS ниже 60 при спавне больших волн врагов и сотен пуль\n• Избыточная нагрузка на GPU при точности High",
        "• Framerate drops below 60 during large monster waves and bullet storms\n• Excessive GPU overhead with High GPU accuracy",
        "✓ Точность ЦП: Авто / Небезопасная (максимальная скорость для сотен мобов)\n✓ Точность ГПУ: Обычная (стабильные 60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Отключено",
        "✓ CPU Accuracy: Auto / Unsafe (Maximum CPU throughput for 200+ mobs)\n✓ GPU Accuracy: Normal (Locked 60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Disabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x010085C019864000ULL,
        "Vampire Survivors",
        "• Задержки физики и анимаций при заполнении экрана тысячами монстров",
        "• Physics and animation slowdowns during screen-filling enemy swarms",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"}
        }
    },
    {
        0x0100FF500E34A000ULL,
        "Xenoblade Chronicles: Definitive Edition",
        "• Просадки FPS и утечки VRAM в открытых локациях (Bionis Leg)\n• Микрофризы при подгрузке текстур высокого разрешения",
        "• Framerate drops and VRAM pressure in large open zones (Bionis Leg)\n• Texture streaming micro-stutters",
        "✓ Точность ГПУ: Обычная (стабильная производительность)\n✓ Сжатие ASTC: Отключено (чистые текстуры без задержек)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: Normal (Smooth performance)\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100E95004038000ULL,
        "Xenoblade Chronicles 2",
        "• Размытие динамического разрешения и нехватка VRAM в провинции Гормотт\n• Просадки частоты кадров в битвах",
        "• Dynamic resolution blur and VRAM memory buildup in Gormott\n• Combat framerate drops",
        "✓ Точность ГПУ: Обычная\n✓ Сжатие ASTC: Отключено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: Normal\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01008D3017B4C000ULL,
        "Super Mario RPG",
        "• Микрофризы при переходе между локациями и в диалогах\n• Сбои глубины теней персонажей",
        "• Micro-stutters during scene transitions and battle dialogs\n• Character shadow depth inaccuracies",
        "✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010006701B4B2000ULL,
        "Princess Peach: Showtime!",
        "• Падение FPS на движке Unreal Engine 4 при трансформациях\n• Мерцание геометрии и теней",
        "• Framerate drops on Unreal Engine 4 during transformations\n• Geometry and shadow flickering",
        "✓ Точность ГПУ: Обычная\n✓ Память: 8GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Memory Layout: 8GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100C8901844A000ULL,
        "Luigi's Mansion 2 HD",
        "• Компиляционные задержки шейдеров в темных комнатах особняка\n• Мерцание динамического освещения фонарика",
        "• Shader compilation stutters in dark mansion corridors\n• Flashlight dynamic lighting flickering",
        "✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100B9301A4A0000ULL,
        "F-Zero 99",
        "• Зависание на стартовом экране при попытке подключения к серверам Nintendo\n• Бесконечная синхронизация онлайна",
        "• Hang on title screen attempting to connect to Nintendo servers\n• Infinite online network sync loop",
        "✓ Режим полёта: Включено (пропуск сетевого зависания)\n✓ Точность ЦП: Авто / Небезопасная (60 FPS)\n✓ Точность ГПУ: Обычная",
        "✓ Airplane Mode: Enabled (Bypasses network connection hang)\n✓ CPU Accuracy: Auto / Unsafe (60 FPS)\n✓ GPU Accuracy: Normal",
        {
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100427010476000ULL,
        "Super Mario 3D All-Stars",
        "• Накладные расходы двойной эмуляции в Super Mario Sunshine и Super Mario Galaxy\n• Потрескивания звука при нехватке CPU",
        "• Double-emulation overhead in Super Mario Sunshine and Galaxy\n• Audio crackling from CPU thread starvation",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe (Maximum JIT performance)\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01001F5010DFA000ULL,
        "Pokemon Legends: Arceus",
        "• Микрофризы и просадки FPS при спавне диких Покемонов\n• Утечки памяти в деревне Джубилайф",
        "• Micro-stutters and framerate drops during wild Pokemon spawns\n• Memory pressure in Jubilife Village",
        "✓ Точность ГПУ: Обычная (стабильные 30 FPS)\n✓ Память: 6GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal (Smooth 30 FPS)\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100A3D008C5C000ULL,
        "Pokemon Scarlet",
        "• Тяжелые утечки памяти в открытом мире Палдеи (>9 ГБ видеопамяти)\n• Падения FPS и рывки камеры",
        "• Massive open world memory leaks in Paldea (>9 GB VRAM)\n• Framerate drops and camera stuttering",
        "✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено\n✓ Память: 6GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01008F6008C5E000ULL,
        "Pokemon Violet",
        "• Тяжелые утечки памяти в открытом мире Палдеи (>9 ГБ видеопамяти)\n• Падения FPS и рывки камеры",
        "• Massive open world memory leaks in Paldea (>9 GB VRAM)\n• Framerate drops and camera stuttering",
        "✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено\n✓ Память: 8GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 8GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100D1AB10000000ULL,
        "Diablo I + Hellfire (DevilutionX)",
        "• Порт DevilutionX для Diablo I и дополнения Hellfire\n• Поддержка MPQ (diabdat, hellfire, hfmonk, ru)\n• Оптимизация 2D Software/Hardware SDL2 рендера",
        "• DevilutionX port for Diablo I and Hellfire expansion\n• MPQ support (diabdat, hellfire, hfmonk, ru)\n• Optimized 2D SDL2 rendering",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"}
        }
    },
    {
        0x0100B00B51230000ULL,
        "Grand Theft Auto V (GTA V Homebrew Port)",
        "• Кастомный Homebrew-порт GTA V для Nintendo Switch\n• Поддержка RPF архивов, аудио и DLC-паков в romfs/\n• Высокое потребление VRAM и тяжелая геометрия Лос-Сантоса",
        "• GTA V Custom Homebrew Switch port\n• RPF archives, audio and DLC packs support in romfs/\n• Heavy Los Santos geometry and VRAM pressure",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная\n✓ Сжатие ASTC: Отключено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled",
        {
            {"Core\\memory_layout_mode", "2"},
            {"System\\memory_layout_mode", "2"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\gpu_fence_behavior", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100671000000000ULL,
        "Grand Theft Auto: San Andreas (re3-sa)",
        "• Порт открытого движка re3-sa для GTA San Andreas\n• Загрузка ресурсов main.scm, models, data, txd, dff\n• Стабильные 60 FPS на открытой карте штата Сан-Андреас",
        "• re3-sa open engine port for GTA San Andreas\n• main.scm, models, data, txd, dff assets loading\n• Locked 60 FPS across San Andreas",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100672000000000ULL,
        "Grand Theft Auto: Vice City (reVC)",
        "• Порт открытого движка reVC для GTA Vice City\n• Загрузка ресурсов gxt, audio, txd, dff\n• Стабильные 60 FPS в Вайс-Сити",
        "• reVC open engine port for GTA Vice City\n• gxt, audio, txd, dff assets loading\n• Locked 60 FPS in Vice City",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100673000000000ULL,
        "Grand Theft Auto III (re3)",
        "• Порт открытого движка re3 для GTA III\n• Загрузка ресурсов Либерти-Сити, коллизий и текстур\n• Стабильные 60 FPS",
        "• re3 open engine port for GTA III\n• Liberty City collision and texture streaming\n• Locked 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000018ULL,
        "Half-Life 1 / Black Mesa Classic (Xash3D FWGS)",
        "• Движок Xash3D FWGS для Half-Life 1, Blue Shift, Opposing Force, Counter-Strike\n• Загрузка PAK/WAD ресурсов и GoldSrc карт\n• Максимальная производительность и отзывчивость управления",
        "• Xash3D FWGS engine for Half-Life 1, Blue Shift, Opposing Force, CS\n• PAK/WAD resources and GoldSrc maps loading\n• Maximum responsiveness and framerate",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100D00100000000ULL,
        "DOOM 1, 2, Final, Plutonia, TNT (GZDoom / PrBoom / Crispy)",
        "• Порты GZDoom, PrBoom, Crispy Doom для классической серии DOOM\n• Поддержка WAD файлов, DeHackEd патчей и модов Sigil/Plutonia/TNT\n• 60 FPS с чистым виброоткликом",
        "• GZDoom, PrBoom, Crispy Doom ports for classic DOOM series\n• WAD files, DeHackEd patches and Sigil/Plutonia/TNT mods\n• Smooth 60 FPS with rumble",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100D00300000000ULL,
        "DOOM 3 (dhewm3 Switch)",
        "• Порт dhewm3 (id Tech 4) для DOOM 3 и Resurrection of Evil\n• Поддержка динамического освещения и попиксельных теней id Tech 4\n• 8 ГБ DRAM для кэширования PK4",
        "• dhewm3 (id Tech 4) port for DOOM 3 and Resurrection of Evil\n• id Tech 4 dynamic lighting and per-pixel shadow rendering\n• 8GB DRAM for PK4 caching",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000071ULL,
        "Quake I / Quake II / Quake III (Quakespasm / Yamagi / ioquake3)",
        "• Порты движков Quakespasm (Quake 1), Yamagi Quake II, ioquake3\n• Загрузка PAK и PK3 файлов, GL-рендеринг\n• Идеальные 60 FPS с гироскопом",
        "• Quakespasm (Quake 1), Yamagi Quake II, ioquake3 ports\n• PAK and PK3 loading, hardware GL rendering\n• Locked 60 FPS with gyro support",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010000000000003DULL,
        "Duke Nukem 3D / Shadow Warrior / Blood (EDuke32 / VoidSW / NBlood)",
        "• Порты Build Engine (EDuke32, VoidSW, NBlood) для культовых 2.5D шутеров\n• Загрузка GRP и RFF ресурсов, вокселей и полигонального рендера\n• Стабильные 60 FPS",
        "• Build Engine ports (EDuke32, VoidSW, NBlood) for classic 2.5D shooters\n• GRP and RFF assets, voxels and polygonal render\n• Locked 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000077ULL,
        "Return to Castle Wolfenstein / Wolf3D (iortcw / ECWolf)",
        "• Порты движков id Tech 3 (iortcw) и ECWolf для Wolfenstein 3D и RtCW\n• Поддержка PK3 и WL6 карт\n• 60 FPS с аппаратным освещением",
        "• id Tech 3 (iortcw) and ECWolf ports for Wolfenstein 3D and RtCW\n• PK3 and WL6 map support\n• Smooth 60 FPS with hardware lighting",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01000000000000F1ULL,
        "Fallout 1 & Fallout 2 (fallout-ce / fallout2-ce)",
        "• Порты Community Edition для Fallout 1 и Fallout 2\n• Загрузка master.dat, critter.dat и кастомных шрифтов\n• Стабильная работа мыши/тача и 60 FPS",
        "• Community Edition ports for Fallout 1 and Fallout 2\n• master.dat, critter.dat and custom fonts loading\n• Smooth touch/mouse input and 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000030ULL,
        "Heroes of Might and Magic II & III (fheroes2 / VCMI Switch)",
        "• Порты движков fheroes2 и VCMI для Героев Меча и Магии II и III (In the Wake of Gods / Horn of the Abyss)\n• Загрузка LOD, AGG, SND ресурсов и модов",
        "• fheroes2 and VCMI ports for Heroes of Might and Magic II and III (WoG / HotA)\n• LOD, AGG, SND assets and mods loading",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010000000000004DULL,
        "Max Payne 1 (Max Payne Switch)",
        "• Нативный порт Max Payne для Nintendo Switch\n• Загрузка архивов RAS (x_data.ras, x_russian.ras)\n• Плавный Bullet-Time и 60 FPS",
        "• Native Max Payne port for Nintendo Switch\n• RAS archives loading (x_data.ras, x_english.ras)\n• Smooth Bullet-Time and locked 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010000000000006DULL,
        "The Elder Scrolls III: Morrowind (OpenMW Switch)",
        "• Порт современного 3D-движка OpenMW для TES III: Morrowind\n• Загрузка ESM, BSA, текстурных паков и модов\n• 8 ГБ DRAM для открытого мира Вварденфелла",
        "• OpenMW modern 3D engine port for TES III: Morrowind\n• ESM, BSA, texture packs and mods loading\n• 8GB DRAM for Vvardenfell open world",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010000000000004AULL,
        "Star Wars: Jedi Outcast & Academy (OpenJK Switch)",
        "• Порт OpenJK для Star Wars Jedi Knight II: Jedi Outcast и Jedi Academy\n• Стабильные 60 FPS в дуэлях на световых мечах",
        "• OpenJK port for Star Wars Jedi Knight II: Jedi Outcast and Academy\n• Smooth 60 FPS in lightsaber duels",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000053ULL,
        "S.T.A.L.K.E.R.: Shadow of Chernobyl (OpenXRay Switch)",
        "• Порт движка X-Ray 1.6 (OpenXRay) для S.T.A.L.K.E.R.: Тень Чернобыля\n• Загрузка DB-архивов и gamedata\n• 8 ГБ DRAM для локаций Зоны",
        "• OpenXRay engine port for S.T.A.L.K.E.R.: Shadow of Chernobyl\n• DB archives and gamedata loading\n• 8GB DRAM for Zone locations",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010000000000005CULL,
        "Sonic CD / Sonic 1 / Sonic 2 / Mania (RSDK Decompilation)",
        "• Порты Retro Engine (RSDKv3, RSDKv4, RSDKv5) для классических игр Sonic\n• Загрузка Data.rsdk и модов\n• Идеальные 60 FPS",
        "• Retro Engine (RSDKv3, RSDKv4, RSDKv5) decompilations for classic Sonic games\n• Data.rsdk and mods loading\n• Flawless 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000064ULL,
        "Super Mario 64 (SM64-NX / Render96)",
        "• Порт SM64-NX и Render96 для Super Mario 64 с широкоформатным рендером и HD-моделями\n• Стабильные 60 FPS",
        "• SM64-NX and Render96 port for Super Mario 64 with widescreen and HD models\n• Locked 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000074ULL,
        "Zelda: Ocarina of Time / Majora's Mask (Ship of Harkinian / 2S2H)",
        "• Порты Ship of Harkinian (OoT) и 2 Ship 2 Harkinian (MM)\n• Загрузка OTR архивов, поддержка 60 FPS, свободная камера и HD текстуры",
        "• Ship of Harkinian (OoT) and 2 Ship 2 Harkinian (MM) ports\n• OTR archives loading, 60 FPS support, free camera and HD textures",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000022ULL,
        "Cave Story / Cave Story+ / AM2R (NXEngine-evo / AM2R)",
        "• Порты NXEngine-evo для Cave Story и нативный порт AM2R (Another Metroid 2 Remake)\n• Чистый 2D-рендеринг и 60 FPS",
        "• NXEngine-evo ports for Cave Story and native AM2R port\n• Crisp 2D rendering and 60 FPS",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000068ULL,
        "Theme Hospital / Caesar III (CorsixTH / Julius / Augustus)",
        "• Порты CorsixTH (Theme Hospital) и Julius/Augustus (Caesar III)\n• Высокое разрешение и сенсорное управление",
        "• CorsixTH (Theme Hospital) and Julius/Augustus (Caesar III) ports\n• High resolution and touch controls",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000091ULL,
        "Command & Conquer / Red Alert / Dune II (OpenRA / Dune Legacy)",
        "• Порты OpenRA (C&C, Red Alert, Dune 2000) и Dune Legacy (Dune II)\n• Быстрый расчет AI юнитов на Unsafe JIT",
        "• OpenRA (C&C, Red Alert, Dune 2000) and Dune Legacy (Dune II) ports\n• Fast RTS AI unit simulation on Unsafe JIT",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000001000ULL,
        "RetroArch & Emulators (PPSSPP, Flycast, ScummVM, MelonDS, mGBA, DuckStation)",
        "• Мультисистемные эмуляторы и оболочки для запуска ретро-платформ на Switch\n• 8 ГБ DRAM для тяжелых ядер (Flycast, PPSSPP, DuckStation)\n• Максимальная скорость JIT компиляции",
        "• Multi-system emulators and frontends for retro platforms\n• 8GB DRAM for memory-heavy cores (Flycast, PPSSPP, DuckStation)\n• Maximum JIT compiler performance",
        "✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000003000ULL,
        "Homebrew Utilities & Overlays (NX-Shell, DBI, Goldleaf, JKSV, Checkpoint, EdiZon, Tesla)",
        "• Системные Homebrew-утилиты Switch для управления сейвами, файлами и оверлеями\n• Мгновенный отклик файловой системы и стабильная работа",
        "• Switch Homebrew system utilities for saves, files and overlays\n• Instant filesystem responsiveness and stability",
        "✓ Точность ЦП: Авто / Небезопасная\n✓ Точность ГПУ: Обычная (60 FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Auto / Unsafe\n✓ GPU Accuracy: Normal (60 FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100000000000055ULL,
        "Grand Theft Auto V (GTA V Homebrew Port / PC Wrapper)",
        "• Высокие требования к памяти потоков и текстурам открытого мира Лос-Сантоса\n• Зависание сетевых сокетов и многопоточных вызовов IPC",
        "• High thread memory pressure and Los Santos open-world texture bandwidth\n• Network socket handshake and multithreaded IPC freezes",
        "✓ Память: 8GB DRAM (критично для стабильной работы открытого мира)\n✓ Точность ЦП: Авто / Небезопасная (максимальный FPS)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Режим полёта: Включено (пропуск сетевых хуков)\n✓ Авто-заглушки: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for open-world stability)\n✓ CPU Accuracy: Auto / Unsafe (Max FPS)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Airplane Mode: Enabled\n✓ Auto Stub: Enabled",
        {
            {"System\\memory_layout_mode", "2"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\airplane_mode", "true"},
            {"Debugging\\use_auto_stub", "true"}
        }
    },
    {
        0x0100000000000034ULL,
        "Grand Theft Auto: The Trilogy / GTA III / VC / SA (re3 / reVC / Homebrew Ports)",
        "• Просадки частоты кадров при рендере геометрии города\n• Рассинхронизация аудио-потоков радио",
        "• City geometry rendering framerate dips\n• Radio station audio stream desynchronization",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ЦП: Авто / Небезопасная\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ CPU Accuracy: Auto / Unsafe\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01002EF01A316000ULL,
        "Brotato",
        "• Просадки FPS при спавне волн врагов\n• Микрофризы расчёта физики снарядов",
        "• Framerate drops during massive horde waves\n• Projectile physics calculation micro-stutters",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Обычная\n✓ Язык: Русский (Регион Европа)",
        "✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: Normal\n✓ Language: Russian (Region Europe)",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "0"},
            {"System\\language_index", "10"},
            {"System\\region_index", "2"}
        }
    },
    {
        0x010089A0197E4000ULL,
        "Vampire Survivors",
        "• Микрофризы и просадки кадров на 25+ минуте при тысячах спрайтов на экране\n• Утечки памяти движка Phaser",
        "• Micro-stutters and framerate drops at 25+ min with thousands of sprites\n• Phaser engine memory leaks",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Обычная",
        "✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: Normal",
        {
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "0"}
        }
    },
    {
        0x010097F018538000ULL,
        "Dave the Diver",
        "• Просадки кадровой частоты во время ночной охоты и шторма\n• Утечки VRAM в суши-баре Bancho Sushi",
        "• Framerate drops during stormy dives and night hunting\n• VRAM memory spikes in Bancho Sushi restaurant",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010093801237C000ULL,
        "Metroid Dread",
        "• Просадки частоты кадров до 15-20 FPS на Snapdragon 8 Gen 3 (Adreno 750) из-за высокой точности ГПУ\n• Микрофризы при входе в зоны E.M.M.I.",
        "• Heavy framerate drops to 15-20 FPS on Snapdragon 8 Gen 3 (Adreno 750) due to high GPU accuracy\n• E.M.M.I. zone transition micro-stutters",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS на Snapdragon 8 Gen 3 / Adreno 750)\n✓ Реактивная очистка: Отключено\n✓ Быстрое время ГПУ: Включено\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS on Snapdragon 8 Gen 3 / Adreno 750)\n✓ Reactive Flushing: Disabled\n✓ Fast GPU Time: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        },
        {0x01005AF00BA7A000ULL}
    },
    {
        0x010042D00D900000ULL,
        "LEGO Star Wars: The Skywalker Saga",
        "• Падение FPS на открытых планетах (Корусант, Татуин)\n• Утечки видеопамяти при смене планет",
        "• Open-world planet performance drops (Coruscant, Tatooine)\n• Hyperdrive transition VRAM spikes",
        "✓ Память: 6GB DRAM\n✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    // AI: Не менять (на будущее) / DO NOT MODIFY
    {
        0x0100307018934000ULL,
        "Signalis",
        "• Просадки FPS до 4 кадров/сек из-за неверной точности ЦП\n• Артефакты вертикальной и горизонтальной развертки и медленная распаковка ASTC",
        "• Low framerate (4 FPS) caused by strict CPU timing mode\n• Scanline artifacts and slow ASTC texture decompression",
        "✓ Точность ЦП: Авто (Dynarmic JIT)\n✓ Точность ГПУ: Обычная\n✓ Декодирование ASTC: ГПУ (Аппаратное)\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Быстрое время ГПУ: Включено",
        "✓ CPU Accuracy: Auto (Dynarmic JIT)\n✓ GPU Accuracy: Normal\n✓ ASTC Decode: GPU (Accelerated)\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled",
        {
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
    {
        0x0100E65002BB8000ULL,
        "Stardew Valley",
        "• Микрофризы при смене дней и сохранении на ферме\n• Просадки FPS во время дождя и фестивалей",
        "• Day transition and farm autosave micro-stutters\n• Rain particles and festival FPS drops",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Точность ГПУ: Обычная",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ GPU Accuracy: Normal",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "0"}
        }
    },
    {
        0x01002FC00412C000ULL,
        "Little Nightmares: Complete Edition",
        "• Вылеты на движке Unreal Engine 4 в Чреве\n• Сбои динамических теней фонарика\n• Просадки FPS от высокой точности ГПУ",
        "• Unreal Engine 4 Maw transition crashes\n• Flashlight dynamic shadow artifacts\n• Heavy framerate drops caused by High GPU accuracy",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устранение вылетов UE4)\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Циклы обратной связи: Включено (тени и освещение UE4)\n✓ Реактивная очистка: Отключено\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Fixes UE4 crashes)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Barrier Feedback Loops: Enabled (UE4 lighting and shadows)\n✓ Reactive Flushing: Disabled\n✓ Dynamic State: Basic (EDS 1)\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010097100EDD6000ULL,
        "Little Nightmares II",
        "• Вылеты из-за нехватки памяти (OOM) в Бледном городе\n• Артефакты тумана и объемного света\n• Просадки FPS от высокой точности ГПУ",
        "• Pale City memory pressure (OOM) crashes\n• Volumetric fog and lighting artifacts\n• Heavy framerate drops caused by High GPU accuracy",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устранение вылетов памяти)\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Циклы обратной связи: Включено\n✓ Реактивная очистка: Отключено\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Fixes memory exhaustion crashes)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Disabled\n✓ Dynamic State: Basic (EDS 1)\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010066101A55A000ULL,
        "Little Nightmares III",
        "• Высокие требования к DRAM и шейдерам спирали\n• Сбои многопоточности Unreal Engine 5\n• Просадки кадровой частоты",
        "• High DRAM and Spiral shader complexity\n• Unreal Engine 5 multithreading synchronization\n• Framerate dips",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устранение сбоев UE5)\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Циклы обратной связи: Включено\n✓ Реактивная очистка: Отключено\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Fixes UE5 crashes)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Disabled\n✓ Dynamic State: Basic (EDS 1)\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01000B900D8B0000ULL,
        "Cadence of Hyrule: Crypt of the NecroDancer",
        "• Рассинхронизация ритмического аудио-движка\n• Задержка обработки ввода стрелок\n• Просадки кадров в подземельях",
        "• Rhythm audio engine timing desynchronization\n• Beat input delay\n• Dungeon combat framerate dips",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Точность ЦП: Авто\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ CPU Accuracy: Auto\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100CEA007D08000ULL,
        "Crypt of the NecroDancer: Nintendo Switch Edition",
        "• Рассинхронизация такта ударов и музыки в подземелье\n• Задержка аудио-буфера\n• Сбои некратных 16-байт операций DMA",
        "• Beat synchronization jitter in procedural dungeons\n• Audio buffer latency\n• Non-16-byte DMA transfer stalls",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Точность ЦП: Авто\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ CPU Accuracy: Auto\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100BDA01AABC000ULL,
        "Rift of the NecroDancer",
        "• Рассинхронизация дорожек ритм-битв в мини-играх\n• Инпут-лаг комбо",
        "• Rhythm lane timing desync during minigames\n• Combo input response latency",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100D59022590000ULL,
        "Scott Pilgrim vs. The World: The Game - Complete Edition",
        "• Зависание на заставке Ubisoft Connect и сбои опроса сети NIFM\n• Сбои перекрывающихся текстур GpuModified в кэше текстур\n• Просадки FPS при высокой точности ГПУ",
        "• Ubisoft Connect boot hang and NIFM network query spam\n• Texture cache GpuModified overlap crashes\n• Framerate drops caused by High GPU accuracy",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Режим полёта: Включено (пропуск Ubisoft Connect)\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Airplane Mode: Enabled (Bypasses Ubisoft Connect)\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100B11027658000ULL,
        "Defender of the Crown: The Legend Returns",
        "• Пользовательский сбой при запуске (Userspace PANIC debug_buffer_err_code=0)\n• Нехватка выделенной оперативной памяти для эмуляции\n• Рассинхронизация 60 FPS",
        "• Boot crash (Userspace PANIC debug_buffer_err_code=0)\n• Emulation DRAM pool allocation exhaustion\n• 60 FPS timing desynchronization",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устранение краша виртуальной машины)\n✓ Игнорирование сбоев памяти: Включено (устранение Userspace PANIC)\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Eliminates VM crash)\n✓ Ignore Memory Aborts: Enabled (Fixes Userspace PANIC)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010059D020C26000ULL,
        "Marvel Cosmic Invasion",
        "• Пользовательский сбой Userspace PANIC debug_buffer_err_code=1A80A\n• Просадки FPS при спавне волн врагов и взрывов\n• Микрофризы расчёта физики",
        "• Userspace PANIC debug_buffer_err_code=1A80A\n• Framerate drops during enemy waves and heavy particles\n• Physics calculation micro-stutters",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010057901E9E6000ULL,
        "Underling Uprising",
        "• Обращения к неразмеченной памяти Unmapped Device ReadBlock 0x1000\n• Просадки кадровой частоты из-за барьеров конвейера\n• Микрофризы подгрузки спрайтов",
        "• Unmapped Device ReadBlock 0x1000 warnings\n• Pipeline fence stalls and framerate dips\n• Sprite streaming micro-stutters",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Обычная (плавные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Раннее освобождение барьеров: Включено\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Early Release Fences: Enabled\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010094D023A28000ULL,
        "Drill Core",
        "• Зависание при процедурной генерации буровых платформ\n• Утечки памяти при спавне сотен монстров",
        "• Procedural platform generation freeze\n• Swarm particle memory buildup",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Динамическое состояние: Базовое\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Dynamic State: Basic\n✓ Fastmem: Enabled\n✓ Auto Stub: Enabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dyna_state", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100C9D013F0E000ULL,
        "Super Mario Party Jamboree",
        "• Вылет при переходе между мини-играми (выход за границы аудиобуфера)\n• Фризы при быстрой смене сцен",
        "• Crash during minigame transitions (audio buffer out-of-bounds)\n• Freezes on rapid scene changes",
        "",
        "",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100AA80194B0000ULL,
        "Pikmin 4",
        "• Вылет при отключении/переподключении контроллера\n• Случайный краш при смене профилей ввода NPad",
        "• Crash on controller disconnect/reconnect\n• Random crash during NPad input profile changes",
        "",
        "",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100EB501BB68000ULL,
        "Luigi's Mansion 2 HD",
        "• Вылет из-за утечки колбэков HID-вибрации после выгрузки сцены\n• Использование памяти после освобождения (use-after-free)",
        "• Crash from HID vibration callback leak after scene unload\n• Use-after-free in input callback system",
        "",
        "",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"}
        }
    },
    {
        0x0100DE801A1C0000ULL,
        "Sid Meier's Civilization VII",
        "• Зависание при попытке запуска Web-апплета y2k\n• Черный экран вместо меню",
        "• Hang attempting to launch y2k Web Applet\n• Black screen instead of main menu",
        "",
        "",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\use_reactive_flushing", "false"}
        }
    },
    {
        0x010053201F9B4000ULL,
        "Persona 5 Royal",
        "• Черный экран / зависание ГПУ на драйверах Qualcomm Adreno\n• Некорректный clamping семплеров текстур",
        "• Black screen / GPU hang on Qualcomm Adreno drivers\n• Incorrect texture sampler clamping behavior",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x010015100B514000ULL,
        "Metroid Prime Remastered",
        "• Задержки компиляции шейдеров при открытии дверей и в комнатах лавы\n• Микрофризы при быстрой смене оружия",
        "• Shader compilation stutter during door transitions and lava chambers\n• Beam switching hitching",
        "✓ Точность ГПУ: Обычная\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: Normal\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x010044700DEB0000ULL,
        "Assassin's Creed: The Rebel Collection",
        "• Случайные вылеты из-за сбоев разыменования нулевых указателей в анимациях\n• Мерцание геометрии и исчезновение NPC из-за рассинхронизации барьеров и окклюзии\n• Волны бьют сквозь палубу и Т-поза персонажей из-за неточного FMA",
        "• Random crash from null pointer memory access in complex animations\n• Geometry flickering and disappearing NPCs due to fence desync and occlusion queries\n• Ocean waves clipping through deck and character T-pose from unfused FMA",
        "✓ Точность ЦП: Точно (расчёт физики воды и волн, устранение Т-позы)\n✓ Обратное чтение буферов ГПУ: Включено (окклюзия и видимость NPC)\n✓ Барьеры ГПУ: Стандартный режим (устранение мерцания геометрии)\n✓ Асинхронные шейдеры: Включено (устранение микрофризов)\n✓ Синхронизация операций памяти: Включено\n✓ Реактивный сброс памяти: Включено\n✓ Асинхронный вывод кадров: Включено (стабильные 60 FPS)\n✓ Сборщик мусора VRAM: Отключено\n✓ Игнорировать прерывания памяти: Включено\n✓ Быстрая память: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая",
        "✓ CPU Accuracy: Accurate (Ocean waves and buoyancy physics, fixes T-pose)\n✓ GPU Buffer Readback: Enabled (Occlusion and NPC visibility)\n✓ GPU Barriers: Standard mode (Fixes geometry flickering)\n✓ Asynchronous Shaders: Enabled (Stutter elimination)\n✓ Sync Memory Operations: Enabled\n✓ Reactive Flushing: Enabled\n✓ Async Presentation: Enabled (Stable 60 FPS)\n✓ VRAM Garbage Collection: Disabled\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High",
        {
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_unsafe_unfuse_fma", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\dyna_state", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        },
        {0x010044700DC30000ULL, 0x010044700DEB0001ULL}
    },
    {
        0x0100670014482000ULL,
        "Assassin's Creed: The Ezio Collection",
        "• Просадки кадровой частоты (4-15 FPS) из-за рассинхрона буферов\n• Мерцание теней и исчезновение объектов на зданиях Венеции и Флоренции",
        "• Severe framerate drops (4-15 FPS) caused by buffer desynchronization\n• Shadow flickering and disappearing objects in Venice and Florence",
        "✓ Барьеры ГПУ: Стандартный режим (ликвидация мерцания объектов)\n✓ Обратное чтение буферов ГПУ: Включено\n✓ Синхронизация операций памяти: Включено\n✓ Реактивный сброс памяти: Включено\n✓ Асинхронный вывод кадров: Включено\n✓ Сборщик мусора VRAM: Отключено\n✓ Точность ГПУ: Обычная\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Fences: Standard mode (fixes flickering objects)\n✓ GPU Buffer Readback: Enabled\n✓ Sync Memory Operations: Enabled\n✓ Reactive Flushing: Enabled\n✓ Async Presentation: Enabled\n✓ VRAM Garbage Collection: Disabled\n✓ GPU Accuracy: Normal\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x01007F600B134000ULL,
        "Assassin's Creed III Remastered",
        "• Просадки FPS до 4-6 к/с при рендеринге снега и воды в Бостоне\n• Мерцание моделей и исчезновение NPC из-за раннего сброса фенсов",
        "• FPS drops to 4-6 in snow and water rendering in Boston\n• Mesh flickering and disappearing NPCs due to early fence release",
        "✓ Барьеры ГПУ: Стандартный режим (устранение мерцания моделей)\n✓ Обратное чтение буферов ГПУ: Включено\n✓ Синхронизация операций памяти: Включено\n✓ Реактивный сброс памяти: Включено\n✓ Асинхронный вывод кадров: Включено (стабильные 60 FPS)\n✓ Сборщик мусора VRAM: Отключено\n✓ Точность ГПУ: Обычная\n✓ Игнорировать прерывания памяти: Включено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Fences: Standard mode (fixes mesh flickering)\n✓ GPU Buffer Readback: Enabled\n✓ Sync Memory Operations: Enabled\n✓ Reactive Flushing: Enabled\n✓ Async Presentation: Enabled (Stable 60 FPS)\n✓ VRAM Garbage Collection: Disabled\n✓ GPU Accuracy: Normal\n✓ Ignore Memory Aborts: Enabled\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x0100236015B7A000ULL,
        "Alan Wake Remastered",
        "• Искажение луча света фонарика и артефакты теней\n• Размытие и нечеткость шрифтов субтитров и интерфейса",
        "• Flashlight cone illumination artifacts and shadow glitch\n• Blurry subtitle fonts and distorted UI text rendering",
        "✓ Динамическое состояние: Базовое (EDS 1 — корректный луч фонарика)\n✓ Сжатие ASTC: Отключено (четкие шрифты и интерфейс)\n✓ Обратное чтение буферов ГПУ: Включено\n✓ Быстрое время ГПУ: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6 ГБ DRAM",
        "✓ Dynamic State: Basic (EDS 1 — correct flashlight cone)\n✓ ASTC Recompression: Uncompressed (sharp text and UI)\n✓ GPU Buffer Readback: Enabled\n✓ Fast GPU Time: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x019232F2781D0000ULL,
        "Dr. Mario Mania",
        "• Черный экран при загрузке 2D-слоев текстур Godot\n• Сбои синхронизации видеобуферов\n• Риск вызова системных LLE апплетов",
        "• Black screen during Godot 2D texture layer uploads\n• Framebuffer synchronization stalls\n• LLE applet invocation risk",
        "✓ Синхронизация операций памяти: Включено\n✓ Точность ГПУ: Высокая\n✓ Точность DMA: Безопасная\n✓ Асинхронный вывод кадров: Отключено\n✓ Апплеты: Полная HLE эмуляция",
        "✓ Sync Memory Operations: Enabled\n✓ GPU Accuracy: High\n✓ DMA Accuracy: Safe\n✓ Async Presentation: Disabled\n✓ Applets: Full HLE Emulation",
        {
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x01052BA7AC450000ULL,
        "Gizmoduck",
        "• Черный экран при инициализации графического движка Godot\n• Рассинхронизация буферов Nouveau\n• Сбои отрисовки шрифтовых атласов",
        "• Black screen on Godot engine initialization\n• Nouveau buffer desynchronization\n• Font atlas rendering stalls",
        "✓ Синхронизация операций памяти: Включено\n✓ Точность ГПУ: Высокая\n✓ Точность DMA: Безопасная\n✓ Асинхронный вывод кадров: Отключено",
        "✓ Sync Memory Operations: Enabled\n✓ GPU Accuracy: High\n✓ DMA Accuracy: Safe\n✓ Async Presentation: Disabled",
        {
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "true"},
        }
    },
    {
        0x01542031DCEC0000ULL,
        "Super Mario Bros. Remastered",
        "• Черный экран при переходе к игровой сцене\n• Задержки вывода кадров Godot\n• Сбои синхронизации видеопамяти",
        "• Black screen when transitioning to game scene\n• Godot frame presentation stalls\n• GPU memory synchronization faults",
        "✓ Синхронизация операций памяти: Включено\n✓ Точность ГПУ: Высокая\n✓ Точность DMA: Безопасная\n✓ Сеть: Включено",
        "✓ Sync Memory Operations: Enabled\n✓ GPU Accuracy: High\n✓ DMA Accuracy: Safe\n✓ Network: Enabled",
        {
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x019232F2781D0000ULL,
        "Dr. Mario Mania",
        "• Сбои сетевой инициализации BSD-сокетов при активном режиме «В самолете»\n• Сбои выделения кучи HB Loader",
        "• Network socket initialization failure when airplane mode is forced\n• HB Loader applet heap allocation faults",
        "✓ Режим «В самолете»: Отключено (сеть активна)\n✓ Точность ГПУ: Высокая\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Асинхронный вывод: Включено",
        "✓ Airplane Mode: Disabled (Network active)\n✓ GPU Accuracy: High\n✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Async Presentation: Enabled",
        {
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x01052BA7AC450000ULL,
        "Gizmoduck",
        "• Ошибки инициализации Homebrew портов и сокетов\n• Просадки FPS при низкой точности ГПУ",
        "• Homebrew port and socket initialization errors\n• FPS drops with low GPU accuracy",
        "✓ Режим «В самолете»: Отключено (сеть активна)\n✓ Точность ГПУ: Высокая\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Disabled (Network active)\n✓ GPU Accuracy: High\n✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    },
    {
        0x0100F43008C44000ULL,
        "Pokemon Legends: Z-A",
        "• Зависание на черном экране при загрузке тяжелых текстур ASTC на ЦП\n• Вылеты по нехватке памяти (OOM) при использовании русификатора (MOD - RUS)\n• Задержки компиляции шейдеров",
        "• Black screen hang during heavy ASTC texture decode on CPU\n• Out-of-memory crashes when using LayeredFS Russian mod\n• Shader compilation stutter",
        "✓ Аппаратное декодирование ASTC: ГПУ (устранение зависания загрузки)\n✓ Конфигурация памяти: 6 ГБ DRAM (устранение вылетов мода)\n✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: BC1 (экономия VRAM)\n✓ Асинхронные шейдеры: Включено\n✓ Асинхронный вывод: Включено",
        "✓ ASTC Hardware Decoding: GPU (Fixes loading black screen hang)\n✓ Memory Layout: 6GB DRAM (Prevents mod OOM crash)\n✓ GPU Accuracy: High\n✓ ASTC Recompression: BC1 (Saves VRAM)\n✓ Asynchronous Shaders: Enabled\n✓ Async Presentation: Enabled",
        {
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\astc_recompression", "1"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\vram_garbage_collection", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "false"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"}
        }
    }
};

static const std::unordered_map<std::string, std::string> s_baseline_ini = {
    {"Cpu\\cpu_accuracy", "0"},
    {"Renderer\\gpu_accuracy", "1"},
    {"Renderer\\async_presentation", "true"},
    {"Renderer\\use_asynchronous_shaders", "true"},
    {"Renderer\\use_fast_gpu_time", "false"},
    {"Renderer\\sync_memory_operations", "false"},
    {"Renderer\\use_reactive_flushing", "false"},
    {"Renderer\\use_video_framerate", "false"},
    {"Renderer\\eco_frame_pacing", "true"},
    {"Renderer\\dma_accuracy", "0"},
    {"Renderer\\gpu_fence_behavior", "0"},
    {"Renderer\\astc_recompression", "1"},
    {"Cpu\\cpuopt_fastmem", "true"},
    {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
    {"System\\airplane_mode", "false"},
    {"Services\\airplane_mode", "false"},
    {"Network\\airplane_mode", "false"},
    {"System\\memory_layout_mode", "1"},
    {"Core\\memory_layout_mode", "1"},
    {"Renderer\\enable_compute_pipelines", "true"},
    {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
    {"Renderer\\use_disk_shader_cache", "true"},
    {"Renderer\\enable_gpu_buffer_readback", "false"},
    {"Renderer\\vram_garbage_collection", "true"},
    {"Renderer\\early_release_fences", "false"}
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

    const auto ign_aborts = GetSetting(settings, "Cpu\\cpuopt_ignore_memory_aborts", "true");
    if (ign_aborts == "false" || ign_aborts == "0") {
        out += "✓ Игнорировать прерывания памяти: Отключено (стандартная обработка исключений памяти)\n";
    } else {
        out += "✓ Игнорировать прерывания памяти: Включено (защита от падений и аварийных вылетов при обращениях за границы буфера)\n";
    }

    const auto airplane = GetSetting(settings, "System\\airplane_mode", "true");
    if (airplane == "false" || airplane == "0") {
        out += "✓ Режим «В самолете»: Отключено (активные сетевые интерфейсы)\n";
    } else {
        out += "✓ Режим «В самолете»: Включено (предотвращает зависание сетевых сокетов и ожидание серверов)\n";
    }

    const auto mem_layout = GetSetting(settings, "System\\memory_layout_mode", GetSetting(settings, "Core\\memory_layout_mode", "0"));
    if (mem_layout == "2") {
        out += "✓ Память DRAM: Экстремальная 8 ГБ (критично для стабильности и предотвращения вылетов Out of Memory движка)\n";
    } else if (mem_layout == "1") {
        out += "✓ Память DRAM: Расширенная 6 ГБ (устраняет вылеты при длительной игре и утечках памяти)\n";
    } else {
        out += "✓ Память DRAM: Стандартная 4 ГБ (оригинальный объем памяти Switch без лишнего расхода ОЗУ)\n";
    }

    // Specific game compatibility overrides
    const auto loops = GetSetting(settings, "Renderer\\barrier_feedback_loops", "");
    if (loops == "true" || loops == "1") {
        out += "✓ Обратная связь барьеров ГПУ: Включено (устранение темных ореолов и графических сбоев постобработки в игре)\n";
    }

    const auto gpu_acc = GetSetting(settings, "Renderer\\gpu_accuracy", "");
    if (gpu_acc == "1") {
        out += "✓ Точность ГПУ: Высокая (устранение визуальных дефектов, полос и искажения геометрии в данной игре)\n";
    } else if (gpu_acc == "2") {
        out += "✓ Точность ГПУ: Экстремальная (максимальная точность расчетов для устранения сбоев рендеринга)\n";
    }

    const auto nvdec = GetSetting(settings, "Renderer\\nvdec_emulation", "");
    if (nvdec == "1") {
        out += "✓ Декодирование видео NVDEC: ЦП (программный декодер FFmpeg устраняет зависание внутриигровых видеороликов)\n";
    } else if (nvdec == "0") {
        out += "✓ Декодирование видео NVDEC: Отключено (пропуск проблемных видеопотоков)\n";
    }

    const auto cpu_acc = GetSetting(settings, "Cpu\\cpu_accuracy", "");
    if (cpu_acc == "1") {
        out += "✓ Точность ЦП: Точный (повышенная точность инструкций для исключения рассинхронизации логики)\n";
    }

    const auto fastmem = GetSetting(settings, "Cpu\\cpuopt_fastmem", "");
    if (fastmem == "false" || fastmem == "0") {
        out += "✓ Эмуляция Host MMU (fastmem): Отключено (программный контроль памяти для устранения падений в игре)\n";
    }

    const auto async_shaders = GetSetting(settings, "Renderer\\use_asynchronous_shaders", "");
    if (async_shaders == "false" || async_shaders == "0") {
        out += "✓ Асинхронная компиляция шейдеров: Отключено (синхронная сборка во избежание сбоев старта игры)\n";
    }

    const auto sync_mem = GetSetting(settings, "Renderer\\sync_memory_operations", "");
    if (sync_mem == "true" || sync_mem == "1") {
        out += "✓ Синхронизация операций памяти: Включено (полная синхронизация модификаций текстур в видеопамяти)\n";
    }

    const auto react_flush = GetSetting(settings, "Renderer\\use_reactive_flushing", "");
    if (react_flush == "true" || react_flush == "1") {
        out += "✓ Реактивный сброс памяти: Включено (своевременный сброс модифицированных видеобуферов)\n";
    }

    const auto vram_gc = GetSetting(settings, "Renderer\\vram_garbage_collection", "");
    if (vram_gc == "false" || vram_gc == "0") {
        out += "✓ Сборщик мусора VRAM: Отключено (устраняет просадки FPS и задержки 200 мс)\n";
    }

    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }

    return out;
}

static std::string BuildFixesEn(const std::unordered_map<std::string, std::string>& settings) {
    std::string out;

    const auto ign_aborts = GetSetting(settings, "Cpu\\cpuopt_ignore_memory_aborts", "true");
    if (ign_aborts == "false" || ign_aborts == "0") {
        out += "✓ Ignore Memory Aborts: Disabled (standard memory abort handling)\n";
    } else {
        out += "✓ Ignore Memory Aborts: Enabled (prevents crashes on out-of-bounds guest memory accesses)\n";
    }

    const auto airplane = GetSetting(settings, "System\\airplane_mode", "true");
    if (airplane == "false" || airplane == "0") {
        out += "✓ Airplane Mode: Disabled (active network interfaces)\n";
    } else {
        out += "✓ Airplane Mode: Enabled (prevents network socket hangs and server matchmaking delays)\n";
    }

    const auto mem_layout = GetSetting(settings, "System\\memory_layout_mode", GetSetting(settings, "Core\\memory_layout_mode", "0"));
    if (mem_layout == "2") {
        out += "✓ DRAM Memory Layout: 8GB Extreme (critical to prevent out-of-memory engine crashes)\n";
    } else if (mem_layout == "1") {
        out += "✓ DRAM Memory Layout: 6GB Expanded (prevents crashes during prolonged gameplay and memory leaks)\n";
    } else {
        out += "✓ DRAM Memory Layout: 4GB Standard (original Switch console memory layout without extra RAM overhead)\n";
    }

    const auto loops = GetSetting(settings, "Renderer\\barrier_feedback_loops", "");
    if (loops == "true" || loops == "1") {
        out += "✓ GPU Barrier Feedback Loops: Enabled (fixes dark halos and post-processing visual bugs)\n";
    }

    const auto gpu_acc = GetSetting(settings, "Renderer\\gpu_accuracy", "");
    if (gpu_acc == "1") {
        out += "✓ GPU Accuracy: High (eliminates visual defects and geometry artifacts in this game)\n";
    } else if (gpu_acc == "2") {
        out += "✓ GPU Accuracy: Extreme (maximum calculation precision to eliminate rendering glitches)\n";
    }

    const auto nvdec = GetSetting(settings, "Renderer\\nvdec_emulation", "");
    if (nvdec == "1") {
        out += "✓ NVDEC Video Emulation: CPU (software FFmpeg decoder prevents in-game video cutscenes from freezing)\n";
    } else if (nvdec == "0") {
        out += "✓ NVDEC Video Emulation: Disabled (video stream playback bypassed)\n";
    }

    const auto cpu_acc = GetSetting(settings, "Cpu\\cpu_accuracy", "");
    if (cpu_acc == "1") {
        out += "✓ CPU Accuracy: Accurate (enhanced instruction precision to prevent logic desynchronization)\n";
    }

    const auto fastmem = GetSetting(settings, "Cpu\\cpuopt_fastmem", "");
    if (fastmem == "false" || fastmem == "0") {
        out += "✓ Host MMU Emulation (Fastmem): Disabled (software memory control to eliminate crashes)\n";
    }

    const auto async_shaders = GetSetting(settings, "Renderer\\use_asynchronous_shaders", "");
    if (async_shaders == "false" || async_shaders == "0") {
        out += "✓ Asynchronous Shaders: Disabled (synchronous pipeline compilation preventing boot crashes)\n";
    }

    const auto sync_mem = GetSetting(settings, "Renderer\\sync_memory_operations", "");
    if (sync_mem == "true" || sync_mem == "1") {
        out += "✓ Sync Memory Operations: Enabled (full synchronization of GPU memory modifications)\n";
    }

    const auto react_flush = GetSetting(settings, "Renderer\\use_reactive_flushing", "");
    if (react_flush == "true" || react_flush == "1") {
        out += "✓ Reactive Flushing: Enabled (prompt flushing of modified render targets)\n";
    }

    const auto vram_gc = GetSetting(settings, "Renderer\\vram_garbage_collection", "");
    if (vram_gc == "false" || vram_gc == "0") {
        out += "✓ VRAM Garbage Collection: Disabled (Eliminates FPS drops and 200ms queue stalls)\n";
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

    enriched.ini_settings = s_baseline_ini;
    for (const auto& [k, v] : base.ini_settings) {
        enriched.ini_settings[k] = v;
    }

    enriched.fixes_ru = BuildFixesRu(enriched.ini_settings);
    enriched.fixes_en = BuildFixesEn(enriched.ini_settings);
    return enriched;
}

static GameFixProfile CreateUniversalProfile(u64 title_id, const std::string& name_hint = "") {
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

    // Check s_profiles
    for (const auto& profile : s_profiles) {
        bool matches = (profile.title_id == title_id || (profile.title_id & ~0x1FFFULL) == base_title_id);
        if (!matches) {
            for (u64 alt : profile.alt_title_ids) {
                if (alt == title_id || (alt & ~0x1FFFULL) == base_title_id) {
                    matches = true;
                    break;
                }
            }
        }
        if (matches) {
            auto enriched = std::make_unique<GameFixProfile>(CreateEnrichedProfile(profile, title_id));
            auto* ptr = enriched.get();
            s_enriched_cache[title_id] = std::move(enriched);
            return ptr;
        }
    }

    // Unprofiled game: synthesize universal profile dynamically!
    auto universal = std::make_unique<GameFixProfile>(CreateUniversalProfile(title_id));
    auto* ptr = universal.get();
    s_enriched_cache[title_id] = std::move(universal);
    return ptr;
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
        std::string hex_id = fmt::format("{:016x}", profile.title_id);
        if (lower.find(hex_id) != std::string::npos) {
            return GetProfile(profile.title_id);
        }

        // Custom keyword matching
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
        if (game_lower.find("no man's sky") != std::string::npos && (lower.find("no man") != std::string::npos || lower.find("nms") != std::string::npos)) {
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
        if ((game_lower.find("gta v") != std::string::npos || game_lower.find("gta 5") != std::string::npos) && (lower.find("gta v") != std::string::npos || lower.find("gta 5") != std::string::npos || lower.find("gtav") != std::string::npos)) {
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
        if (game_lower.find("gta v") != std::string::npos && (lower.find("gta v") != std::string::npos || lower.find("gta 5") != std::string::npos || lower.find("0100b00b51230000") != std::string::npos)) {
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
        if (game_lower.find("openxray") != std::string::npos && (lower.find("openxray") != std::string::npos || lower.find("stalker") != std::string::npos || lower.find("s.t.a.l.k.e.r") != std::string::npos)) {
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

    auto universal = std::make_unique<GameFixProfile>(CreateUniversalProfile(pseudo_tid, clean_name));
    auto* ptr = universal.get();
    s_enriched_cache[pseudo_tid] = std::move(universal);
    return ptr;
}

bool GameFixDatabase::HasProfile(u64 title_id) {
    return title_id != 0;
}

const std::vector<GameFixProfile>& GameFixDatabase::GetAllProfiles() {
    return s_profiles;
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

    // Merge settings from profile
    for (const auto& [full_key, val] : profile->ini_settings) {
        if (full_key == "Renderer\\aspect_ratio" || full_key == "Renderer\\resolution_setup" ||
            full_key == "System\\use_docked_mode" || full_key == "Renderer\\anti_aliasing" ||
            full_key == "Renderer\\scaling_filter" || full_key == "Renderer\\fsr_sharpening_slider" ||
            full_key == "Renderer\\max_anisotropy") {
            continue;
        }
        auto slash = full_key.find('\\');
        if (slash != std::string::npos) {
            auto sec = full_key.substr(0, slash);
            auto key = full_key.substr(slash + 1);
            if (key == "aspect_ratio" || key == "resolution_setup" || key == "use_docked_mode" ||
                key == "anti_aliasing" || key == "scaling_filter" || key == "fsr_sharpening_slider" ||
                key == "max_anisotropy") {
                continue;
            }
            std::string sanitized_val = val;
            sections[sec][key] = sanitized_val;
            sections[sec][key + "\\use_global"] = "false";
            sections[sec][key + "\\default"] = "false";
            if (key == "memory_layout_mode") {
                std::string mem_val = val;
#ifdef __ANDROID__
                int mode = 0;
                if (!mem_val.empty()) {
                    mode = std::atoi(mem_val.c_str());
                }
                if (mode >= 2) {
                    mem_val = "1";
                }
#endif
                sections["Core"][key] = mem_val;
                sections["Core"][key + "\\use_global"] = "false";
                sections["Core"][key + "\\default"] = "false";
                sections["System"][key] = mem_val;
                sections["System"][key + "\\use_global"] = "false";
                sections["System"][key + "\\default"] = "false";
            }
            if (key == "airplane_mode") {
                sections["System"][key] = val;
                sections["System"][key + "\\use_global"] = "false";
                sections["System"][key + "\\default"] = "false";
                sections["Services"][key] = val;
                sections["Services"][key + "\\use_global"] = "false";
                sections["Services"][key + "\\default"] = "false";
                sections["Network"][key] = val;
                sections["Network"][key + "\\use_global"] = "false";
                sections["Network"][key + "\\default"] = "false";
            }
        }
    }
    sections["StormEden"]["storm_fix_applied"] = "true";

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
        sections["StormEden"]["storm_fix_dont_ask"] = "true";
    } else {
        if (sections.count("StormEden")) {
            sections["StormEden"].erase("storm_fix_dont_ask");
            if (sections["StormEden"].empty()) {
                sections.erase("StormEden");
            }
        }
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
    try {
        std::filesystem::path custom_path = Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir) / "custom";
        if (!std::filesystem::exists(custom_path)) return 0;

        for (const auto& entry : std::filesystem::directory_iterator(custom_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ini") {
                std::ifstream in(entry.path());
                if (!in.is_open()) continue;
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();

                if (content.find("storm_fix_dont_ask") != std::string::npos) {
                    std::istringstream stream(content);
                    std::string line;
                    std::string new_content;
                    bool modified = false;
                    while (std::getline(stream, line)) {
                        if (line.find("storm_fix_dont_ask") != std::string::npos) {
                            modified = true;
                            continue;
                        }
                        new_content += line + "\n";
                    }
                    if (modified) {
                        std::ofstream out(entry.path(), std::ios::trunc);
                        if (out.is_open()) {
                            out << new_content;
                            count++;
                        }
                    }
                }
            }
        }
    } catch (...) {
    }
    LOG_INFO(Frontend, "Reset GameFix suppression on {} configuration files", count);
    return count;
}

static bool s_fixes_enabled = true;

void GameFixDatabase::SetFixesEnabled(bool enabled) {
    s_fixes_enabled = enabled;
}

bool GameFixDatabase::AreFixesEnabled() {
    return s_fixes_enabled;
}

bool GameFixDatabase::ApplyProfileDirectly(u64 title_id) {
    try {
        const auto* profile = GetProfile(title_id);
        if (!profile) {
            return false;
        }

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
            setting.SetGlobal(false);
            setting.SetValue(val);
        };

        for (const auto& [full_key, val] : profile->ini_settings) {
            if (full_key == "Renderer\\aspect_ratio" || full_key == "Renderer\\resolution_setup" ||
                full_key == "System\\use_docked_mode" || full_key == "Renderer\\anti_aliasing" ||
                full_key == "Renderer\\scaling_filter" || full_key == "Renderer\\fsr_sharpening_slider" ||
                full_key == "Renderer\\max_anisotropy") {
                continue;
            }
            if (full_key == "Renderer\\gpu_accuracy") {
                apply_setting(Settings::values.gpu_accuracy, static_cast<Settings::GpuAccuracy>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\barrier_feedback_loops") {
                apply_setting(Settings::values.barrier_feedback_loops, val == "true" || val == "1");
            } else if (full_key == "Renderer\\use_reactive_flushing") {
                apply_setting(Settings::values.use_reactive_flushing, val == "true" || val == "1");
            } else if (full_key == "Renderer\\astc_recompression") {
                apply_setting(Settings::values.astc_recompression, static_cast<Settings::AstcRecompression>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\use_asynchronous_shaders") {
                apply_setting(Settings::values.use_asynchronous_shaders, val == "true" || val == "1");
            } else if (full_key == "Renderer\\max_anisotropy") {
                apply_setting(Settings::values.max_anisotropy, static_cast<Settings::AnisotropyMode>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\fsr_sharpening_slider") {
                apply_setting(Settings::values.fsr_sharpening_slider, static_cast<u8>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\scaling_filter") {
                apply_setting(Settings::values.scaling_filter, static_cast<Settings::ScalingFilter>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\anti_aliasing") {
                apply_setting(Settings::values.anti_aliasing, static_cast<Settings::AntiAliasing>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\fix_bloom_effects") {
                apply_setting(Settings::values.fix_bloom_effects, val == "true" || val == "1");
            } else if (full_key == "Renderer\\enable_gpu_buffer_readback") {
                apply_setting(Settings::values.enable_gpu_buffer_readback, val == "true" || val == "1");
            } else if (full_key == "Renderer\\early_release_fences") {
                apply_setting(Settings::values.early_release_fences, val == "true" || val == "1");
            } else if (full_key == "Renderer\\sync_memory_operations") {
                apply_setting(Settings::values.sync_memory_operations, val == "true" || val == "1");
            } else if (full_key == "Renderer\\use_fast_gpu_time" || full_key == "Renderer\\gpu_clock") {
                if (val == "true" || val == "1") {
                    apply_setting(Settings::values.gpu_clock, Settings::GpuClock::Boost);
                } else if (val == "2") {
                    apply_setting(Settings::values.gpu_clock, Settings::GpuClock::Overclock);
                } else {
                    apply_setting(Settings::values.gpu_clock, Settings::GpuClock::Normal);
                }
            } else if (full_key == "Renderer\\dyna_state") {
                apply_setting(Settings::values.dyna_state, static_cast<Settings::ExtendedDynamicState>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\nvdec_emulation") {
                apply_setting(Settings::values.nvdec_emulation, static_cast<Settings::NvdecEmulation>(safe_stoi(val, 1)));
            } else if (full_key == "Renderer\\async_presentation") {
                apply_setting(Settings::values.async_presentation, val == "true" || val == "1");
            } else if (full_key == "Renderer\\vertex_input_dynamic_state") {
                apply_setting(Settings::values.vertex_input_dynamic_state, val == "true" || val == "1");
            } else if (full_key == "Renderer\\accelerate_astc") {
                apply_setting(Settings::values.accelerate_astc, static_cast<Settings::AstcDecodeMode>(safe_stoi(val, 1)));
            } else if (full_key == "Renderer\\gpu_fence_behavior") {
                auto fence_val = static_cast<Settings::GpuFenceBehavior>(safe_stoi(val, 0));
                if (fence_val >= Settings::GpuFenceBehavior::Accurate && Settings::values.sync_memory_operations.GetValue()) {
                    LOG_WARNING(Frontend, "GameFixDatabase: Clamped gpu_fence_behavior to Default (0) on {:#016x} to prevent deadlock", title_id);
                    fence_val = Settings::GpuFenceBehavior::Default;
                }
                apply_setting(Settings::values.gpu_fence_behavior, fence_val);
            } else if (full_key == "System\\airplane_mode" || full_key == "Services\\airplane_mode" || full_key == "Network\\airplane_mode") {
                apply_setting(Settings::values.airplane_mode, val == "true" || val == "1");
            } else if (full_key == "System\\memory_layout_mode" || full_key == "Core\\memory_layout_mode") {
                int mode = safe_stoi(val, 0);
#ifdef __ANDROID__
                if (mode >= 2) {
                    mode = 1;
                }
#endif
                apply_setting(Settings::values.memory_layout_mode, static_cast<Settings::MemoryLayout>(mode));
            } else if (full_key == "Cpu\\cpu_backend") {
                apply_setting(Settings::values.cpu_backend, static_cast<Settings::CpuBackend>(safe_stoi(val, 1)));
            } else if (full_key == "Cpu\\cpu_accuracy") {
                apply_setting(Settings::values.cpu_accuracy, static_cast<Settings::CpuAccuracy>(safe_stoi(val, 1)));
            } else if (full_key == "Cpu\\cpuopt_fastmem") {
                apply_setting(Settings::values.cpuopt_fastmem, val == "true" || val == "1");
            } else if (full_key == "Cpu\\cpuopt_ignore_memory_aborts") {
                apply_setting(Settings::values.cpuopt_ignore_memory_aborts, val == "true" || val == "1");
            } else if (full_key == "Cpu\\cpuopt_recompile_exclusives") {
                apply_setting(Settings::values.cpuopt_recompile_exclusives, val == "true" || val == "1");
            } else if (full_key == "Cpu\\cpuopt_fastmem_exclusives") {
                apply_setting(Settings::values.cpuopt_fastmem_exclusives, val == "true" || val == "1");
            } else if (full_key == "Cpu\\cpuopt_unsafe_ignore_global_monitor") {
                apply_setting(Settings::values.cpuopt_unsafe_ignore_global_monitor, val == "true" || val == "1");
            } else if (full_key == "Renderer\\use_vulkan_driver_pipeline_cache") {
                apply_setting(Settings::values.use_vulkan_driver_pipeline_cache, val == "true" || val == "1");
            } else if (full_key == "Renderer\\use_disk_shader_cache") {
                apply_setting(Settings::values.use_disk_shader_cache, val == "true" || val == "1");
            } else if (full_key == "Renderer\\enable_compute_pipelines") {
                apply_setting(Settings::values.enable_compute_pipelines, val == "true" || val == "1");
            } else if (full_key == "Renderer\\sync_memory_operations") {
                apply_setting(Settings::values.sync_memory_operations, val == "true" || val == "1");
            } else if (full_key == "Renderer\\use_video_framerate") {
                apply_setting(Settings::values.use_video_framerate, val == "true" || val == "1");
            } else if (full_key == "Renderer\\eco_frame_pacing") {
                apply_setting(Settings::values.eco_frame_pacing, val == "true" || val == "1");
            } else if (full_key == "Renderer\\dma_accuracy") {
                apply_setting(Settings::values.dma_accuracy, static_cast<Settings::DmaAccuracy>(safe_stoi(val, 0)));
            } else if (full_key == "Renderer\\vram_garbage_collection") {
                apply_setting(Settings::values.vram_garbage_collection, val == "true" || val == "1");
            } else if (full_key == "Renderer\\early_release_fences") {
                apply_setting(Settings::values.early_release_fences, val == "true" || val == "1");
            } else if (full_key == "System\\eco_thermal_mode") {
                apply_setting(Settings::values.eco_thermal_mode, val == "true" || val == "1");
            } else if (full_key == "System\\airplane_mode" || full_key == "Services\\airplane_mode" || full_key == "Network\\airplane_mode") {
                apply_setting(Settings::values.airplane_mode, val == "true" || val == "1");
            } else if (full_key == "System\\language_index") {
                apply_setting(Settings::values.language_index, static_cast<Settings::Language>(safe_stoi(val, static_cast<int>(Settings::Language::Russian))));
            } else if (full_key == "System\\region_index") {
                apply_setting(Settings::values.region_index, static_cast<Settings::Region>(safe_stoi(val, static_cast<int>(Settings::Region::Europe))));
            } else if (full_key == "LibraryApplet\\cabinet_applet_mode") {
                apply_setting(Settings::values.cabinet_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\controller_applet_mode") {
                apply_setting(Settings::values.controller_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\error_applet_mode") {
                apply_setting(Settings::values.error_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\swkbd_applet_mode") {
                apply_setting(Settings::values.swkbd_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\mii_edit_applet_mode") {
                apply_setting(Settings::values.mii_edit_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\photo_viewer_applet_mode") {
                apply_setting(Settings::values.photo_viewer_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            } else if (full_key == "LibraryApplet\\offline_web_applet_mode") {
                apply_setting(Settings::values.offline_web_applet_mode, static_cast<Settings::AppletMode>(safe_stoi(val, 0)));
            }
        }
        Settings::UpdateGPUAccuracy();
        Settings::UpdateRescalingInfo();
        LOG_INFO(Frontend, "Directly applied GameFix profile in-memory for {:#016x}", title_id);
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

    if (sections["StormEden"]["storm_fix_applied"] != "true") {
        return false;
    }

    for (const auto& [full_key, expected_val] : profile->ini_settings) {
        auto slash = full_key.find('\\');
        if (slash == std::string::npos) continue;
        std::string sec = full_key.substr(0, slash);
        std::string key = full_key.substr(slash + 1);

        if (!sections.count(sec) || !sections[sec].count(key)) {
            continue;
        }
        if (sections[sec][key] != expected_val) {
            return false;
        }
        if (sections[sec][key + "\\use_global"] == "true") {
            return false;
        }
    }
    return true;
}

} // namespace Core
