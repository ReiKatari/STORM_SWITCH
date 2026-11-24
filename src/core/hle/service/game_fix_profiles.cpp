#include "game_fix_profiles.h"

namespace Core {
const std::vector<GameFixProfile> s_profiles = {
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/1890
{
        0x0100C8000CEAE000ULL,
        "Crysis Remastered",
        "• Графические искажения и битые кадровые буферы из-за несовпадения макета GENERAL в BlitColor\n• Просадки FPS при включении маскировки и тепловизора нанокостюма (CryEngine SVOGI)\n• Зависания видеороликов при аппаратном декодировании",
        "• Visual corruption and broken framebuffers caused by GENERAL layout mismatch in BlitColor\n• Framerate drops when activating Nanosuit Cloak and Thermal Vision (CryEngine SVOGI)\n• Cutscene freezes with hardware video decoding",
        "✓ Точность ГПУ: Обычная (исправлена синхронизация BlitColor и проходов рендеринга)\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Режим «В самолете»: Включено",
        "✓ GPU Accuracy: Normal (Fixed BlitColor and renderpass state synchronization)\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Airplane Mode: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\nvdec_emulation", "3"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x0100C8000CEAE800ULL}
    },
    {
        0x0100232014B9C000ULL,
        "Crysis 2 Remastered",
        "• Искажения постобработки и размытия в движении CryEngine\n• Просадки FPS в городских боях Нью-Йорка\n• Сбои потоковой подгрузки текстур",
        "• CryEngine post-processing and motion blur artifacts\n• Framerate drops in New York urban combat\n• Texture streaming stalls",
        "✓ Точность ГПУ: Обычная\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Режим «В самолете»: Включено",
        "✓ GPU Accuracy: Normal\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Airplane Mode: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\nvdec_emulation", "3"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x0100232014B9C800ULL}
    },
    {
        0x010073E014BA0000ULL,
        "Crysis 3 Remastered",
        "• Искажения освещения и отражений воды в куполе Либерти\n• Просадки FPS при высокой детализации растительности\n• Сбои памяти при загрузке уровней",
        "• Lighting and water reflection glitches in the Liberty Dome\n• Framerate drops in dense foliage areas\n• Memory faults during level transitions",
        "✓ Точность ГПУ: Обычная\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Режим «В самолете»: Включено",
        "✓ GPU Accuracy: Normal\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Airplane Mode: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\nvdec_emulation", "3"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x010073E014BA0800ULL}
    },
    {
        0x010033001F050000ULL,
        "Ys vs. Trails in the Sky: Alternative Saga",
        "• Зависание и артефакты макроблоков вступительного видеоролика\n• Искажения 2D-спрайтов и шрифтов при пересжатии текстур ASTC\n• Просадки кадровой частоты во время динамичных сражений\n• Сбои распределения памяти при переходе между аренами",
        "• Opening cinematic freeze and macroblock artifacts\n• 2D sprite and font distortion with ASTC texture recompression\n• Framerate drops during fast-paced combat\n• Memory allocation faults during arena transitions",
        "✓ Декодирование видео NVDEC: ЦП (программный декодер FFmpeg устраняет зависание роликов)\n✓ Пересжатие текстур ASTC: Без сжатия (устранение искажений 2D-графики)\n✓ Точность ГПУ: Высокая (стабильная геометрия и Z-буфер)\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ NVDEC Video Emulation: CPU (software FFmpeg decoder prevents cutscene freezes)\n✓ ASTC Texture Recompression: Uncompressed (Fixes 2D graphics distortion)\n✓ GPU Accuracy: High (Stable geometry and Z-buffer)\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x010033001F050800ULL}
    },
    {
        0x010034B00E14C000ULL,
        "Tokyo 2020 Olympics - The Official Video Game",
        "• Черный экран и зависание при запуске из-за рассинхронизации потоков и сбоев инициализации сети\n• Просадки кадровой частоты во время соревнований\n• Сбои таймингов ГПУ",
        "• Black screen and boot freeze caused by thread desync and network initialization faults\n• Framerate drops during competitions\n• GPU timing stalls",
        "✓ Режим «В самолете»: Отключено (сеть активна, корректная инициализация сокетов BSD)\n✓ Декодирование видео NVDEC: ЦП (программный декодер FFmpeg устраняет зависание роликов)\n✓ Точность ГПУ: Высокая (устранение черного экрана)\n✓ Синхронизация потоков: Корректный монитор атомиков Dynarmic (устранение дедлока)\n✓ Пересжатие текстур ASTC: Без сжатия\n✓ Асинхронная презентация и шейдеры: Включено\n✓ Быстрое время ГПУ: Отключено (синхронизация кадров Sega)\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Конфигурация памяти: 4 ГБ DRAM",
        "✓ Airplane Mode: Disabled (Network active, clean BSD socket init)\n✓ NVDEC Video Emulation: CPU (software FFmpeg decoder prevents cutscene freezes)\n✓ GPU Accuracy: High (Fixes black screen)\n✓ Thread Sync: Accurate Dynarmic atomic monitor (Fixes deadlock)\n✓ ASTC Texture Recompression: Uncompressed\n✓ Async Presentation and Shaders: Enabled\n✓ Fast GPU Time: Disabled (Sega frame pacing)\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"System\\memory_layout_mode", "0"}
        },
        {
            0x010034B00E14C800ULL,
            0x0100C60010228000ULL,
            0x0100C60010228800ULL,
            0x01008F400E75A000ULL,
            0x01008F400E75A800ULL,
            0x0100BA700E340000ULL
        }
    },
    {
        0x010003000E146000ULL,
        "Mario and Sonic at the Olympic Games Tokyo 2020",
        "• Черный экран и зависание при запуске из-за задач Hedgehog Engine 2\n• Просадки кадровой частоты во время соревнований\n• Сбои синхронизации потоков",
        "• Black screen and boot freeze in Hedgehog Engine 2 task scheduler\n• Framerate drops during competitions\n• Thread synchronization faults",
        "✓ Точность ГПУ: Высокая (устранение черного экрана)\n✓ Синхронизация потоков: Корректный монитор атомиков Dynarmic\n✓ Пересжатие текстур ASTC: Без сжатия\n✓ Асинхронная презентация и шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Конфигурация памяти: 4 ГБ DRAM",
        "✓ GPU Accuracy: High (Fixes black screen)\n✓ Thread Sync: Accurate Dynarmic atomic monitor\n✓ ASTC Texture Recompression: Uncompressed\n✓ Async Presentation and Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x010003000E146800ULL}
    },
    {
        0x01008F1008DA6000ULL,
        "Darkest Dungeon",
        "• Зависание и вылет вступительного видеоролика при программном декодировании NVDEC ЦП\n• Просадки кадровой частоты в подземельях\n• Сбои распределения памяти",
        "• Freezing and crash in opening cinematic with CPU NVDEC video decoding\n• Framerate drops in dungeons\n• Memory allocation faults",
        "✓ Декодирование видео NVDEC: Гибридное (Hybrid 3) - стабильное воспроизведение вступительных роликов\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Режим «В самолете»: Включено\n✓ Точность ГПУ: Обычная",
        "✓ NVDEC Video Emulation: Hybrid (Hybrid 3) - stable video playback\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ Airplane Mode: Enabled\n✓ GPU Accuracy: Normal",
        {
            {"Renderer\\nvdec_emulation", "3"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x01008F1008DA6800ULL, 0x01008F1008C06000ULL}
    },
    {
        0x0100DDF01A03A000ULL,
        "DAVE THE DIVER",
        "• Вылеты при смене локаций (дайвинг / суши-бар Банчо) из-за сборщика мусора Unity GC\n• Утечка дескрипторов текстур под водой\n• Просадки кадровой частоты",
        "• Crashes during transitions (Diving / Bancho Sushi) caused by Unity GC spikes\n• Underwater texture descriptor exhaustion\n• Framerate drops and stuttering",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Очистка VRAM: Отключено (устраняет микрофризы анимаций)\n✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ VRAM Garbage Collection: Disabled (Prevents animation stutter)\n✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x01008BA02525A800ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4640
{
        0x01006560184E6000ULL,
        "Mortal Kombat 1",
        "• Мгновенный вылет при запуске (UE4 TaskGraph / атомики)\n• Зависание на заставке WB Games и вылет по нехватке памяти (OOM)\n• Сбои Extended Dynamic State в шейдерах арены\n• Пропадание текстур персонажей и окружения при длительной игре",
        "• Instant crash on launch (UE4 TaskGraph / atomics)\n• WB Games intro freeze and Out of Memory crash\n• Extended Dynamic State arena shader crashes\n• Character and environment texture streaming dropouts",
        "✓ Память: 8 ГБ DRAM (устраняет дедлок и краш при загрузке боя)\n✓ Быстрое время ГПУ: Отключено (исправление зависания на заставке и стартовом тексте)\n✓ Декодирование ASTC: ГПУ\n✓ Поведение барьеров ГПУ: Стандартное (0)\n✓ Точность DMA: Стандартная\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Точность ЦП: Авто (стабильный быстрый JIT-компилятор)\n✓ Синхронизация памяти: Отключено\n✓ Реактивная очистка: Отключено\n✓ Режим полёта: Включено (пропуск серверов WB Play)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Memory Layout: 8GB DRAM (Fixes fight load crash)\n✓ Fast GPU Time: Disabled (Fixes intro and epilepsy warning freeze)\n✓ ASTC Decoding: GPU\n✓ GPU Fence Behavior: Standard (0)\n✓ DMA Accuracy: Standard\n✓ Dynamic State: Basic (EDS1)\n✓ CPU Accuracy: Auto (Stable fast JIT compiler)\n✓ Sync Memory Operations: Disabled\n✓ Reactive Flushing: Disabled\n✓ Airplane Mode: Enabled (Bypasses WB Play online check)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: High",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_recompile_exclusives", "true"},
            {"Renderer\\eco_frame_pacing", "false"},
            {"Renderer\\use_video_framerate", "false"}
        },
        {0x0100D2800D5C2000ULL, 0x01003A00184E6000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/320
{
        0x0100F2200C984000ULL,
        "Mortal Kombat 11",
        "• Разлет полигонов, мерцание геометрии и графические баги шейдеров\n• Искажение динамического освещения и эффектов крови\n• Микрофризы рендеринга",
        "• Exploding vertices, geometry flicker, and shader rendering artifacts\n• Corrupted dynamic lighting and blood particle effects\n• Pipeline micro-stutter",
        "✓ Точность ГПУ: Высокая (устраняет черные силуэты на Mali, разлет полигонов и артефакты)\n✓ Быстрое время ГПУ: Отключено\n✓ Синхронизация памяти ГПУ: Отключено (стабильный конвейер)\n✓ Реактивная очистка: Отключено (исправление освещения)\n✓ Barrier Feedback Loops: Включено (исправление частиц)\n✓ Сжатие ASTC: Без сжатия\n✓ Память: 6 ГБ DRAM",
        "✓ GPU Accuracy: High (Fixes Mali black silhouettes, exploding vertices and glitches)\n✓ Fast GPU Time: Disabled\n✓ Sync Memory Operations: Disabled\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\language_index", "10"},
            {"System\\region_index", "2"}
        },
        {0x0100B1100C4D0000ULL, 0x0100D76008684000ULL}
    },
    {
        0x01008C3005A32000ULL,
        "Unravel Two",
        "• Зависание на вступительных видеороликах и чёрный экран при переходе к геймплею\n• Потеря рендер-пасса PhyreEngine при принудительных обратных барьерах",
        "• Freeze during intro cutscenes and black screen on cutscene-to-gameplay transition\n• PhyreEngine render-pass drop when forcing barrier feedback loops",
        "✓ Аппаратный декодер видео NVDEC (GPU / MediaCodec)\n✓ Точность ГПУ: Обычная (Normal)\n✓ Обратные циклы барьеров: Отключено (сохранение 3D-рендера PhyreEngine)\n✓ Ранний сброс заборов: Отключено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ Hardware GPU/MediaCodec NVDEC Video Decoding\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Disabled (Preserves PhyreEngine 3D render pass)\n✓ Early Release Fences: Disabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x0100E5D00CC0C000ULL, 0x010099900B22E000ULL, 0x010099900B224000ULL}
    },
    {
        0x0100EC9010258000ULL,
        "Streets of Rage 4",
        "• Зависание на 0 FPS при старте из-за сетевого опроса серверов DotEmu\n• Зависание вступительных видеороликов при аппаратном декодировании NVDEC\n• Рассинхронизация 2D-презентации кадров при асинхронном выводе",
        "• 0 FPS boot hang caused by DotEmu server network polling\n• Intro video freeze on hardware GPU NVDEC decoding\n• 2D presentation desync with asynchronous presentation",
        "✓ Точность ГПУ: Высокая\n✓ NVDEC: Программный (устранение зависаний видеороликов)\n✓ Совместимое масштабирование (Legacy Rescale): Включено (устранение нарезки 2D-спрайтов при 2X+)\n✓ Асинхронная презентация: Отключено\n✓ Режим полета: Включено (пропуск сетевого опроса серверов DotEmu)",
        "✓ GPU Accuracy: High\n✓ NVDEC: CPU (Fixes video freezes)\n✓ Rescale Compatibility Mode: Enabled (Fixes 2D sprite slicing at 2X+)\n✓ Async Presentation: Disabled\n✓ Airplane Mode: Enabled (Bypasses DotEmu server polling)",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "3"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        },
        {
            0x0100EC9010258800ULL,
            0x010085800E33E000ULL,
            0x01000BD011936000ULL,
            0x0100F7A011938000ULL,
            0x0100AC300919A000ULL,
            0x01004C900C1F8000ULL,
            0x010077800F838000ULL
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/32
{
        0x01007EF00011E000ULL,
        "The Legend of Zelda: Breath of the Wild",
        "• Вылет игры при загрузке сохранения (исчерпание пула памяти 4GB DRAM)\n• Сбои десериализации акторов и физики Havok\n• Черный силуэт Линка и мерцание освещения",
        "• Crash when loading save games (4GB DRAM pool exhaustion)\n• Havok physics and actor deserialization crash\n• Link black silhouette and lighting flicker",
        "✓ Память: 8GB DRAM (предотвращает краш аллокатора при загрузке сейва)\n✓ Игнорирование сбоев памяти: Включено (стабильность физики Havok)\n✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено (стабильность кадрового буфера)\n✓ Barrier Feedback Loops: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 8GB DRAM (Prevents save loading OOM)\n✓ Ignore Memory Aborts: Enabled (Havok physics stability)\n✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "false"},
            {"Network\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "false"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x01007EF00011E800ULL}
,
        {
            {"amd", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\use_reactive_flushing", "false"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_fast_gpu_time", "false"}
            }},
            {"intel", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\use_reactive_flushing", "false"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_fast_gpu_time", "false"}
            }},
            {"adreno", {
                {"Renderer\\gpu_accuracy", "2"},
                {"Renderer\\use_reactive_flushing", "true"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\astc_recompression", "0"}
            }}
        }    },
    {
        0x01005B101DC84000ULL,
        "EA SPORTS FC 25",
        "• Зависание при старте игры / черный экран на заставке EA\n• Нехватка памяти Frostbite Engine (вылет при загрузке стадиона и режима Карьеры)\n• Зависание на сетевой аутентификации",
        "• Boot hang / black screen on EA splash screen\n• Frostbite Engine out-of-memory and Career Mode crash\n• Network handshake freeze",
        "✓ Память: 8GB DRAM (критично для движка Frostbite и Карьеры)\n✓ Режим полета: Включено (пропуск серверов EA Connect)\n✓ Асинхронные шейдеры: Отключено (стабильность загрузки)\n✓ Патчер NCE для динамических NRO (ldr:ro): Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for Frostbite Engine & Career Mode)\n✓ Airplane Mode: Enabled (Bypasses EA Connect handshake)\n✓ Asynchronous Shaders: Disabled (Prevents boot crash)\n✓ Runtime NRO NCE Patcher (ldr:ro): Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Debugging\\use_auto_stub", "true"}
        }
    },
    {
        0x010077D0238FA000ULL,
        "EA SPORTS FC 26 / FC 27",
        "• Вылет при входе в режим Карьеры (SIGILL при загрузке динамических NRO-модулей Frostbite)\n• Зависание при старте игры / черный экран на заставке EA\n• Нехватка памяти Frostbite Engine (вылет при загрузке стадиона)\n• Зависание на сетевой аутентификации",
        "• Career Mode crash (SIGILL on dynamic Frostbite NRO module load)\n• Boot hang / black screen on EA splash screen\n• Frostbite Engine out-of-memory crash\n• Network handshake freeze",
        "✓ Память: 8GB DRAM (критично для движка Frostbite и режима Карьеры)\n✓ Режим полета: Включено (пропуск серверов EA Connect)\n✓ Асинхронные шейдеры: Отключено (стабильность загрузки)\n✓ Патчер NCE для динамических NRO (ldr:ro): Включено\n✓ Авто-заглушки сервисов HLE: Включено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for Frostbite Engine & Career Mode)\n✓ Airplane Mode: Enabled (Bypasses EA Connect handshake)\n✓ Asynchronous Shaders: Disabled (Prevents boot crash)\n✓ Runtime NRO NCE Patcher (ldr:ro): Enabled\n✓ HLE Service Auto-Stub: Enabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Debugging\\use_auto_stub", "true"}
        },
        {0x010077D0238FA800ULL, 0x01005B101DC84800ULL}
    },
    {
        0x01008CA01F186000ULL,
        "Leap Year",
        "• Вылет игры через некоторое время из-за утечки памяти\n• Рассинхронизация аудиопотока",
        "• Crash after prolonged gameplay due to memory accumulation\n• Audio stream desync",
        "✓ Память: 6GB DRAM (устраняет вылет через время)\n✓ Асинхронные шейдеры: Отключено\n✓ Быстрая память: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents crash over time)\n✓ Asynchronous Shaders: Disabled\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
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
            {"System\\memory_layout_mode", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/1269q8l/skyrim_switch_turnip_drivers_sky_flicker_fix/
{
        0x01000A10041EA000ULL,
        "The Elder Scrolls V: Skyrim",
        "• Некорректное изображение водной глади и искажения отражений\n• Артефакты сжатия снега и текстур ландшафта",
        "• Incorrect water rendering and reflection distortion\n• Snow and terrain texture compression artifacts",
        "✓ Сжатие ASTC: Отключено (четкие текстуры снега и гор)\n✓ Реактивная очистка: Отключено (исправление отражений воды)\n✓ Точность ГПУ: Высокая\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ ASTC Recompression: Uncompressed (Crisp snow and terrain)\n✓ Reactive Flushing: Disabled (Fixes water reflections)\n✓ GPU Accuracy: High\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
        },
        {0x01000A10041EA800ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4711
{
        0x0100BCB0176D0000ULL,
        "Hogwarts Legacy",
        "• Фиолетовые круги и радужные засветы освещения (NaN/Inf в шейдерах точечного света и Bloom UE4 на драйверах AK 1.04 / Qualcomm / Turnip)\n• Некорректное изображение, сбои освещения и геометрии\n• Вылет при переходе между локациями Хогвартса из-за нехватки памяти",
        "• Purple circles and rainbow lighting artifacts (NaN/Inf in UE4 point-light attenuation and Bloom shaders on AK 1.04 / Qualcomm / Turnip drivers)\n• Corrupted rendering, lighting and geometry glitches\n• Out-of-memory crash during Hogwarts area transitions",
        "✓ Память: 8GB DRAM (критично для стабильности)\n✓ Исправление Bloom и FP16/MUFU NaN-защита: Включено (устраняет фиолетовые круги и радугу днем и ночью)\n✓ Реактивная очистка: Отключено (исправление освещения замка)\n✓ Barrier Feedback Loops: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 8GB DRAM (Critical for stability)\n✓ Fix Bloom & FP16/MUFU NaN Sanitization: Enabled (Eliminates purple circles and rainbow lighting day & night)\n✓ Reactive Flushing: Disabled (Fixes castle lighting)\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\fix_bloom_effects", "true"}
        },
        {0x0100B5B0112F8000ULL, 0x0100D1801648E000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4508
{
        0x0100F2C0115B6000ULL,
        "The Legend of Zelda: Tears of the Kingdom",
        "• Черный силуэт персонажей и тени в Кавернах\n• Бирюзовая сетка и артефакты Z-буфера на водных поверхностях\n• Утечки VRAM в конструкторе Ультраруки",
        "• Character silhouette and shadow artifacts in Depths\n• Water surface and depth bias cyan grid artifacts\n• Ultrahand VRAM pressure",
        "✓ Точность ГПУ: Высокая (исправление теней и освещения)\n✓ Реактивная очистка: Включено (устранение мерцания магии Ультраруки и рун)\n✓ Сжатие ASTC: Отключено (прозрачная чистая вода)\n✓ Быстрое время ГПУ: Отключено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High (Fixes character shadows and lighting)\n✓ Reactive Flushing: Enabled (Fixes Ultrahand and rune magic flickering)\n✓ ASTC Recompression: Uncompressed\n✓ Fast GPU Time: Disabled\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
        },
        {0x0100F2C0115B6800ULL}
,
        {
            {"adreno", {
                {"Renderer\\gpu_accuracy", "2"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_reactive_flushing", "true"},
                {"Renderer\\astc_recompression", "0"}
            }},
            {"amd", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\barrier_feedback_loops", "true"}
            }},
            {"nvidia", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\barrier_feedback_loops", "true"}
            }}
        },
        {
            {
                "1.4.3",
                "277178b7dba1b6d4949a84778d4abc58",
                {30, 60, 90},
                0x68c15cULL,
                0xd5cc0ULL,
                0xd5d80ULL,
                "Верифицированные адреса TotK v1.4.3: динамический шаг 30/60/90 FPS, часы мира (0x68c15c) и внутренний масштаб рендера (0xd5cc0 / 0xd5d80)",
                "Verified TotK v1.4.3 offsets: dynamic 30/60/90 FPS timing, world clock (0x68c15c), and internal render scale (0xd5cc0 / 0xd5d80)"
            }
        }    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4812
{
        0x01004D701742A000ULL,
        "Paper Mario: The Thousand-Year Door",
        "• Черный экран на катсценах в прологе\n• Сбои 2D-шрифтов диалогов и мерцание текстур",
        "• Black screen during prologue cutscenes\n• Corrupted battle text boxes and flickering textures",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3673
{
        0x0100726014352000ULL,
        "Diablo II: Resurrected",
        "• Вылет при продолжении игры / загрузке персонажа (нехватка памяти)\n• Пропадание букв и текста интерфейса (глифы шрифтов)\n• Зависание при опросе серверов Battle.net\n• Мерцание персонажа на экране выбора героя и графические артефакты\n• Просадки кадровой частоты (4 FPS / 200 ms)",
        "• Character load / continue game crash (memory exhaustion)\n• Missing UI text and font glyphs\n• Battle.net server handshake hang\n• Character flickering on selection screen and graphical artifacts\n• Frame drops (4 FPS / 200 ms)",
        "✓ Память: 6GB DRAM (запас кучи для потоковой загрузки Актов без роста PSS)\n✓ Точность ГПУ: Обычная (Normal - высокий FPS + синхронизация семафоров Slug)\n✓ Режим полёта: Включено (пропуск Battle.net)\n✓ Динамическое состояние: EDS1 + Точность DMA: Обычная (Normal, 1)\n✓ Fix Bloom Effects: Отключено (сохраняет шрифты Slug и аппаратный FP16)\n✓ Синхронные шейдеры + Sync Memory Ops + Reactive Flushing + VkPipelineCache + Barrier Feedback ON (100% глифы без обрезки)\n✓ Быстрое время ГПУ ON + Early Release Fences OFF + VRAM GC / Budget Gov / Readback OFF (плавный FPS)\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM (Act I+ streaming headroom without PSS bloat)\n✓ GPU Accuracy: Normal (High FPS + Slug semaphore sync fix)\n✓ Airplane Mode: Enabled (Bypasses Battle.net)\n✓ Dynamic State: EDS1 + DMA Accuracy: Normal (1)\n✓ Fix Bloom Effects: Disabled (Preserves Slug fonts and native FP16 speed)\n✓ Async Shaders OFF + Sync Memory Ops + Reactive Flushing + VkPipelineCache + Feedback Loops ON (100% intact glyphs)\n✓ Fast GPU Time ON + Early Fences OFF + VRAM GC / Budget Gov / Readback OFF (max FPS)\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "1"},
            {"System\\memory_layout_mode", "1"},
            {"Network\\airplane_mode", "true"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\dma_accuracy", "1"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\fix_bloom_effects", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_vulkan_driver_pipeline_cache", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\disable_buffer_reorder", "true"}
        },
        {0x01002A801458A000ULL, 0x0100916014D8C000ULL}
    },
    {
        0x010020D01AD24000ULL,
        "Animal Well",
        "• Просадки кадровой частоты (4 FPS / 200 ms) из-за высокой точности ГПУ, блокировок фенсов и реактивной очистки\n• Смещение и обрезка экрана из-за фиксации разрешения DRS Lock\n• Графические полосы и черные тайлы из-за барьеров обратной связи",
        "• Frame drops (4 FPS / 200 ms) caused by high GPU accuracy, fence stalls and reactive flushing\n• Viewport offset and clipping caused by DRS Resolution Lock\n• Graphics strips and black tiles caused by feedback loop barriers",
        "✓ Точность ГПУ: Обычная (плавные 60 FPS, устранение задержек 200 ms)\n✓ Фиксация разрешения DRS: Отключено (устранение смещения и обрезки экрана 320x180)\n✓ Динамический оптимизатор: Отключено (стабильный буфер 320x180)\n✓ Разрешение: Родное 1X (720p/1080p, чистый пиксель-арт)\n✓ Сглаживание: Отключено (сохранение пиксель-арта)\n✓ Фильтр масштабирования: Билинейный\n✓ Соотношение сторон: 16:9 (без искажений)\n✓ Режим консоли: Док-станция\n✓ Реактивная очистка: Отключено (устранение просадок кадровой частоты 4 FPS и задержек 200 ms)\n✓ Асинхронный вывод: Включено (плавные 60 FPS)\n✓ Барьеры ГПУ: По умолчанию\n✓ Точность DMA: По умолчанию\n✓ Барьеры обратной связи: Отключено (устранение черных тайлов)\n✓ Динамическое состояние: Базовое (EDS 1)\n✓ Асинхронные шейдеры: Включено (плавный геймплей)\n✓ Обратное чтение буферов ГПУ: Отключено\n✓ Вычислительные конвейеры: Включено\n✓ Быстрая память Fastmem: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Сжатие ASTC: Без сжатия",
        "✓ GPU Accuracy: Normal (Smooth 60 FPS, eliminates 200 ms latency)\n✓ DRS Resolution Lock: Disabled (Fixes 320x180 viewport offset and clipping)\n✓ Dynamic Performance Scaler: Disabled (Stable 320x180 buffer)\n✓ Resolution: Native 1X (720p/1080p, pristine pixel art)\n✓ Anti-Aliasing: None (Pixel-art preservation)\n✓ Scaling Filter: Bilinear\n✓ Aspect Ratio: 16:9\n✓ Console Mode: Docked\n✓ Reactive Flushing: Disabled (Eliminates 4 FPS / 200 ms latency stalls)\n✓ Async Presentation: Enabled (Smooth 60 FPS)\n✓ GPU Fence Behavior: Default\n✓ DMA Accuracy: Default\n✓ Barrier Feedback Loops: Disabled (Eliminates black tiles)\n✓ Dynamic State: Basic (EDS 1)\n✓ Async Shaders: Enabled (Smooth gameplay)\n✓ GPU Buffer Readback: Disabled\n✓ Compute Pipelines: Enabled\n✓ Fastmem: Enabled\n✓ Fast GPU Time: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\resolution_setup", "3"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
            {"Renderer\\aspect_ratio", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\use_docked_mode", "1"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x01002D001AD24000ULL, 0x0100650017170000ULL, 0x0100C9E01B854000ULL, 0x0100E5E01C098000ULL}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Cpu\\cpu_accuracy", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4590
{
        0x0100E26017E5E000ULL,
        "Red Dead Redemption",
        "• Хрипы и рассинхронизация звука в катсценах\n• Микрофризы физических потоков RAGE Engine",
        "• Audio crackling and desync in cutscenes\n• Micro-stutters in RAGE Engine physics threads",
        "✓ Точность ЦП: Точная\n✓ Аудио-буфер Cubeb: 80 ms\n✓ Синхронизация памяти: Отключено",
        "✓ CPU Accuracy: Accurate\n✓ Cubeb Audio Buffer: 80 ms\n✓ Sync Memory Ops: Disabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/407
{
        0x0100D870045B6000ULL,
        "Luigi's Mansion 3",
        "• Растягивание полигонов (взрывы геометрии)\n• Невидимый луч фонарика и зависания в лифте",
        "• Vertex explosion (stretched geometry)\n• Invisible flashlight beam and elevator freeze",
        "✓ Расширенное динамическое состояние: Включено\n✓ Точность ГПУ: Высокая\n✓ Точность ЦП: Точная",
        "✓ Extended Dynamic State: Enabled\n✓ GPU Accuracy: High\n✓ CPU Accuracy: Accurate",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\dma_accuracy", "0"},
            {"Cpu\\cpu_accuracy", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4163
{
        0x01004A4010F22000ULL,
        "Bayonetta 3",
        "• Невидимые персонажи и противники на чипах Snapdragon\n• Чёрный экран после QTE-добиваний\n• Зависание на карте пролога без отображения интерфейса",
        "• Invisible character/enemy models on Snapdragon SoCs\n• Black screen after QTE sequences\n• Softlock on prologue chapter map without UI",
        "✓ Контроль отсечения глубины: Включено\n✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено (отрисовка интерфейса)\n✓ Сжатие ASTC: Отключено\n✓ Синхронизация операций памяти: Включено\n✓ Конфигурация памяти: 8 ГБ DRAM",
        "✓ Depth Clip Control: Enabled\n✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled (fixes missing UI)\n✓ ASTC Recompression: Uncompressed\n✓ Sync Memory Operations: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\async_presentation", "true"}
        },
        {0x01004A4010F22800ULL, 0x01004A4010FEA000ULL, 0x01004A4010FE8000ULL, 0x01004A4010FEB800ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/186
{
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Пропадание неонового интерфейса Легиона\n• Затемнение картинки и циклический гул звука",
        "• Missing Legion neon glow effects\n• Dark screen tint and audio looping",
        "✓ Эмуляция цвета BGR565: Включено\n✓ Коррекция эффектов свечения: Включено",
        "✓ Emulate BGR565: Enabled\n✓ Fix Bloom Effects: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\emulate_bgr565", "true"},
            {"Renderer\\fix_bloom_effects", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/68
{
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Вылет на экране победы или в меню новостей",
        "• Crash on victory screen or news board (Web Applet)",
        "✓ Отключение веб-апплета: Включено\n✓ Mii Applet: LLE",
        "✓ Disable Web Applet: Enabled\n✓ Mii Applet: LLE",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Debugging\\disable_web_applet", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/131
{
        0x0100152000022000ULL,
        "Mario Kart 8 Deluxe",
        "• Отсутствие голов у персонажей Mii на трассах\n• Вылет при переходе между режимами и загрузке трасс\n• Нехватка памяти при длительных сессиях",
        "• Invisible/missing heads on Mii characters\n• Crash during mode transitions and track loading\n• Memory pressure during extended sessions",
        "✓ Требуется Firmware 18.0.0+ и системные файлы Mii\n✓ Сжатие ASTC: Отключено\n✓ Реактивный сброс: Включено\n✓ Конфигурация памяти: 6 ГБ DRAM",
        "✓ Firmware 18.0.0+ and Mii system files required\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "False"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3777
{
        0x01001F5010DFA000ULL,
        "Pokemon Legends: Arceus",
        "• Вытягивание полигонов травы и деревьев в небо\n• Сбои теней на аренах",
        "• Vertex explosion on trees and grass geometry\n• Shadow glitches during battle transitions",
        "✓ Точность ГПУ: Высокая\n✓ Анизотропная фильтрация: 16x\n✓ Декодирование ASTC на GPU: Включено",
        "✓ GPU Accuracy: High\n✓ Anisotropic Filtering: 16x\n✓ ASTC GPU Decode: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4198
{
        0x01008C30086E0000ULL,
        "Pokemon Scarlet",
        "• Утечки памяти в открытом мире Палдеи\n• Мерцание ландшафта и текстур",
        "• Open-world memory leaks in Paldea\n• Terrain and texture flickering",
        "✓ Сжатие ASTC: Отключено\n✓ Ограничение VRAM: Conservative",
        "✓ ASTC Recompression: BC3\n✓ VRAM Usage: Conservative",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_usage_mode", "1"},
            {"Cpu\\cpu_accuracy", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4199
{
        0x0100A3D0086EE000ULL,
        "Pokemon Violet",
        "• Утечки памяти в открытом мире Палдеи\n• Мерцание ландшафта и текстур",
        "• Open-world memory leaks in Paldea\n• Terrain and texture flickering",
        "✓ Сжатие ASTC: Отключено\n✓ Ограничение VRAM: Conservative",
        "✓ ASTC Recompression: BC3\n✓ VRAM Usage: Conservative",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\vram_usage_mode", "1"},
            {"Cpu\\cpu_accuracy", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3233
{
        0x0100B9F010DC4000ULL,
        "Doom Eternal",
        "• Вылет после заставки об эпилепсии из-за переполнения пула памяти (OOM)\n• Вылет драйвера Vulkan при первом выстреле / спавне BFG",
        "• Crash after epilepsy warning due to 4GB memory pool exhaustion (OOM)\n• Vulkan device loss crash on weapon fire / BFG",
        "✓ Конфигурация памяти: 6 ГБ DRAM (устраняет вылет после предупреждения об эпилепсии)\n✓ Игнорировать прерывания памяти: Включено\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Точность DMA: Safe\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents crash after epilepsy warning)\n✓ Ignore Memory Aborts: Enabled\n✓ Dynamic State: Basic (EDS1)\n✓ DMA Accuracy: Safe\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
        }
    },
    {
        0x010034B01314C000ULL,
        "Prince of Persia: The Lost Crown",
        "• Чёрный экран при воспроизведении видеовставок и анимаций амулетов",
        "• Black screen during video cutscenes and amulet animations",
        "✓ Декодирование видео (NVDEC): На ЦП\n✓ Точность DMA: Безопасная\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Fastmem Exclusives: Отключено",
        "✓ NVDEC Video Emulation: CPU\n✓ DMA Accuracy: Safe\n✓ Dynamic State: Basic (EDS1)\n✓ Fastmem Exclusives: Disabled",
        {
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"}
        },
        {0x0100BB70144F8000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4740
{
        0x010063B017DAE000ULL,
        "Batman: Arkham Knight",
        "• Вылет по нехватке памяти при погонях на Бэтмобиле",
        "• OOM crash during Batmobile chase sequences",
        "✓ Сжатие ASTC: Отключено\n✓ Память: 8GB DRAM",
        "✓ ASTC Recompression: BC1\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4785
{
        0x01000B901C46E000ULL,
        "Shin Megami Tensei V: Vengeance",
        "• Вылет движка Unreal Engine 4 при старте на чипах Snapdragon 8",
        "• Unreal Engine 4 crash on launch on Snapdragon 8 devices",
        "✓ Macro JIT / HLE: Включено\n✓ Нативное декодирование BCn: Включено",
        "✓ Macro JIT / HLE: Enabled\n✓ Native BCn Decode: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Debugging\\disable_macro_jit", "false"},
            {"Debugging\\disable_macro_hle", "false"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/334
{
        0x01003D100E9C6000ULL,
        "The Witcher 3: Wild Hunt",
        "• Вылеты памяти (OOM) и зависания диспетчера потоков REDengine\n• Мерцание теней и воды в Новиграде",
        "• REDengine thread deadlocks and host OOM memory crashes\n• Shadow and water flickering in Novigrad",
        "✓ Память: 6 ГБ DRAM (устраняет сбой аллокатора памяти)\n✓ Fastmem Exclusives: Отключено (устраняет дедлоки потоков REDengine)\n✓ Игнорировать прерывания памяти: Включено\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено",
        "✓ Memory Layout: 6GB DRAM (Prevents host allocator OOM)\n✓ Fastmem Exclusives: Disabled (Fixes REDengine thread deadlocks)\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Renderer\\fsr_sharpening_slider", "85"}
        },
        {0x0100BFF00E256000ULL, 0x0100E67012924000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4190
{
        0x010056D015DB6000ULL,
        "Sonic Frontiers",
        "• Просадки FPS и размытие текстур в открытых зонах островов\n• Мерцание теней Cyberspace и подгрузка геометрии",
        "• Open-zone framerate drops and blurry grass textures\n• Cyberspace shadow flickering and geometry pop-in",
        "✓ Память: 6GB DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        },
        {0x01004C90141A4000ULL, 0x0100508013AE0000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/112
{
        0x01004AB00A260000ULL,
        "Dark Souls: Remastered",
        "• Просадки кадровой частоты у костров и зацикливание звука баффов",
        "• Bonfire particle slowdown and weapon buff sound loop",
        "✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая (корректные шейдеры и освещение)\n✓ Игнорировать прерывания памяти: Включено\n✓ Сжатие ASTC: Отключено (нативный ASTC для Mali и Adreno)\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High (accurate shaders and lighting)\n✓ Ignore Memory Aborts: Enabled\n✓ ASTC Recompression: Uncompressed (native ASTC for Mali and Adreno)\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\async_presentation", "false"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010015100B514000ULL,
        "Super Mario Bros. Wonder",
        "• Вылет при переходе уровней и активации Чудо-цветка\n• Микрофризы при компиляции анимаций персонажей Lunchpack",
        "• Crash on stage transitions and Wonder Flower effects\n• Shader compilation stutter on Lunchpack character animations",
        "✓ Конфигурация памяти: 4 ГБ DRAM (стабильный запуск Lunchpack)\n✓ Точность ГПУ: Обычная (Normal 60 FPS)\n✓ Быстрое время ГПУ: Включено\n✓ Быстрая память: Включено\n✓ Синхронные шейдеры: Включено (защита от выпадения agl-отрисовок)\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Отключено\n✓ Барьеры обратной связи: Включено (стабильные переходы уровней)\n✓ Ранний сброс заборов: Отключено\n✓ Пропуск кадров: Отключено\n✓ Режим полета: Включено\n✓ Контроль бюджета VRAM: Отключено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ Memory Layout: 4GB DRAM (Stable Lunchpack boot)\n✓ GPU Accuracy: Normal (Smooth 60 FPS)\n✓ Fast GPU Time: Enabled\n✓ Fastmem: Enabled\n✓ Synchronous Shaders: Enabled (Prevents dropped agl draws)\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled (Smooth stage transitions)\n✓ Early Release Fences: Disabled\n✓ Frame Skipping: Disabled\n✓ Airplane Mode: Enabled\n✓ VRAM Budget Governor: Disabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Core\\memory_layout_mode", "0"},
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\smart_shader_throttle", "false"},
            {"Renderer\\use_video_framerate", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"System\\airplane_mode", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x010015100B514800ULL}
    },

    {
        0x0100C88011246000ULL,
        "Disco Elysium: The Final Cut",
        "• Вылеты при смене локаций и артефакты освещения/тумана Unity\n• Просадки FPS при включенной высокой точности ГПУ и реактивной очистке\n• Размытие текста диалогов TextMeshPro и цветовые артефакты портретов",
        "• Out of memory crash on zone transitions and Unity lighting/fog glitches\n• FPS drops caused by High GPU Accuracy and Reactive Flushing stalls\n• TextMeshPro dialogue font blur and color artifacts on painted portraits",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Точность ГПУ: Обычная (высокий FPS без статтеров)\n✓ Вычислительные конвейеры: Включено (корректный туман и освещение)\n✓ Исправление Bloom и FP16 NaN-защита: Включено\n✓ Реактивная очистка: Отключено (без задержек 200 мс)\n✓ Сжатие ASTC: Отключено (гибридный декодер)",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ GPU Accuracy: Normal (High FPS without stalls)\n✓ Compute Pipelines: Enabled (Correct fog and dynamic lighting)\n✓ Fix Bloom & FP16 NaN Sanitization: Enabled\n✓ Reactive Flushing: Disabled (Zero 200ms stalls)\n✓ ASTC Recompression: Uncompressed (Hybrid decode)",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\fix_bloom_effects", "true"},
            {"Cpu\\cpuopt_fastmem", "false"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\airplane_mode", "false"}
        },
        {0x01008C300F7F0000ULL, 0x0100E26014466000ULL}
    },
    {
        0x01008C300F7F0000ULL,
        "Disco Elysium: The Final Cut (WW)",
        "• Вылеты при смене локаций и артефакты освещения/тумана Unity\n• Размытие текста диалогов TextMeshPro и цветовые артефакты портретов",
        "• Out of memory crash on zone transitions and Unity lighting/fog glitches\n• TextMeshPro dialogue font blur and color artifacts on painted portraits",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Точность ГПУ: Обычная\n✓ Вычислительные конвейеры: Включено\n✓ Исправление Bloom: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ GPU Accuracy: Normal\n✓ Compute Pipelines: Enabled\n✓ Fix Bloom: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\fix_bloom_effects", "true"},
            {"Cpu\\cpuopt_fastmem", "false"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\airplane_mode", "false"}
        },
        {}
    },
    {
        0x0100E26014466000ULL,
        "Disco Elysium: The Final Cut (Asia)",
        "• Вылеты при смене локаций и артефакты освещения/тумана Unity\n• Размытие текста диалогов TextMeshPro и цветовые артефакты портретов",
        "• Out of memory crash on zone transitions and Unity lighting/fog glitches\n• TextMeshPro dialogue font blur and color artifacts on painted portraits",
        "✓ Память: 6GB DRAM (предотвращение вылетов Unity)\n✓ Точность ГПУ: Обычная\n✓ Вычислительные конвейеры: Включено\n✓ Исправление Bloom: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 6GB DRAM (Prevents Unity OOM crashes)\n✓ GPU Accuracy: Normal\n✓ Compute Pipelines: Enabled\n✓ Fix Bloom: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\fix_bloom_effects", "true"},
            {"Cpu\\cpuopt_fastmem", "false"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\airplane_mode", "false"}
        },
        {}
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
            {"System\\memory_layout_mode", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/2389
{
        0x0100FF500E34A000ULL,
        "Xenoblade Chronicles: Definitive Edition",
        "• Черные полигоны, пропадание геометрии и зеленые артефакты текстур\n• Черный экран и 0 FPS при запуске под NCE на Android",
        "• Black stretched polygons, missing geometry, and green texture artifacts\n• Black screen and 0 FPS boot hang under NCE on Android",
        "✓ Память: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая + Реактивная очистка\n✓ Синхронные шейдеры (устраняет пропадание геометрии)\n✓ Безопасные барьеры и кэш VRAM (без раннего сброса fence/GC)\n✓ Динамическое состояние: EDS2, ASTC: ГПУ без сжатия",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High + Reactive Flushing\n✓ Synchronous Shaders (prevents missing geometry)\n✓ Safe GPU fences & VRAM cache (disables early fence release / aggressive GC)\n✓ Dynamic State: EDS2, ASTC: GPU Uncompressed",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\smart_shader_throttle", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Cpu\\cpu_backend", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/39
{
        0x0100E95004038000ULL,
        "Xenoblade Chronicles 2",
        "• Просадки кадровой частоты в густонаселенных локациях (Гула, Мор Ардайн)",
        "• Heavy frame drops in dense titan areas (Gormott, Mor Ardain)",
        "✓ Динамическое состояние: Расширенное\n✓ Сжатие ASTC: Отключено\n✓ Память: 6GB DRAM\n✓ Быстрое время ГПУ: Включено",
        "✓ Dynamic State: EDS2\n✓ ASTC Recompression: BC3\n✓ Memory Layout: 6GB DRAM\n✓ Fast GPU Time: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4030
{
        0x010074F013262000ULL,
        "Xenoblade Chronicles 3",
        "• Зависания на 0 FPS при синхронной компиляции шейдеров и DRS-блокировке\n• Утечки VRAM и микростаттеры в битвах с 7 персонажами",
        "• 0 FPS shader compilation stalls and DRS GPU-tick stalls\n• VRAM leaks and micro-stutters during full 7-character battle parties",
        "✓ Память: 6 ГБ DRAM\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ / DRS Lock / Пропуск кадров: Отключено\n✓ Динамическое состояние: EDS2\n✓ Точность ГПУ: Высокая",
        "✓ Memory Layout: 6GB DRAM\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time / DRS Lock / Frame Skipping: Disabled\n✓ Dynamic State: EDS2\n✓ GPU Accuracy: High",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\smart_shader_throttle", "false"},
            {"Renderer\\use_video_framerate", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\optimize_spirv_output", "1"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\dma_accuracy", "0"}
        }
    },
    {
        0x0100453019AA8000ULL,
        "Xenoblade Chronicles X: Definitive Edition",
        "• Черный экран и бесконечная загрузка на EDS1 / при DRS-блокировке и быстром времени ГПУ\n• Нехватка памяти на 4 ГБ DRAM",
        "• Black screen and infinite loading under EDS1 / DRS lock and fast GPU time\n• Out-of-memory stalls on 4GB DRAM",
        "✓ Память: 6 ГБ DRAM\n✓ Динамическое состояние: EDS2\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ / DRS Lock / Пропуск кадров: Отключено",
        "✓ Memory Layout: 6GB DRAM\n✓ Dynamic State: EDS2\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time / DRS Lock / Frame Skipping: Disabled",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\dyna_state", "2"},
            {"Renderer\\resolution_setup", "3"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\smart_shader_throttle", "false"},
            {"Renderer\\use_video_framerate", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\optimize_spirv_output", "1"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\dma_accuracy", "0"}
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\astc_recompression", "0"}
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
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/30
{
        0x0100000000010000ULL,
        "Super Mario Odyssey",
        "• Мерцание 2D-рисунков на стенах\n• Артефакты дыма и тумана в Песчаном царстве",
        "• Flickering 2D wall drawings\n• Sand Kingdom smoke and fog rendering artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Сжатие ASTC: Отключено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
,
        {},
        {
            {"intel", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_fast_gpu_time", "true"}
            }},
            {"amd", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_fast_gpu_time", "true"}
            }},
            {"adreno", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\barrier_feedback_loops", "true"},
                {"Renderer\\use_fast_gpu_time", "true"}
            }}
        }    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4379
{
        0x0100121014688000ULL,
        "Metroid Prime Remastered",
        "• Статтеры при открытии дверей между отсеками\n• Мерцание эффектов визора",
        "• Door transition compilation stutter\n• Visor UI and particle flicker",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
,
        {},
        {
            {"adreno", {
                {"Renderer\\accelerate_astc", "1"},
                {"Renderer\\use_asynchronous_shaders", "true"}
            }},
            {"intel", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\accelerate_astc", "1"},
                {"Renderer\\use_fast_gpu_time", "false"}
            }},
            {"amd", {
                {"Renderer\\gpu_accuracy", "0"},
                {"Renderer\\accelerate_astc", "1"},
                {"Renderer\\use_fast_gpu_time", "false"}
            }}
        }    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/16hjklo/luigis_mansion_3_flashlight_bug_turnip_fix/
{
        0x0100DCA0064A6000ULL,
        "Luigi's Mansion 3",
        "• Артефакты динамического света фонарика и теней\n• Падение FPS в комнатах с призраками",
        "• Dynamic flashlight beam artifacts\n• FPS drops in ghost-heavy rooms",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/200
{
        0x01006F8002326000ULL,
        "Animal Crossing: New Horizons",
        "• Размытие травы и мелких объектов\n• Мерцание теней в вечернее время",
        "• Blurry grass and ground textures\n• Evening shadow flicker",
        "✓ Анизотропная фильтрация: 16x\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено",
        "✓ Anisotropic Filtering: 16x\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4155
{
        0x01005CA01580E000ULL,
        "Persona 5 Royal",
        "• Мерцание 2D UI портретов и шрифтов\n• Просадки FPS в людных районах Токио",
        "• 2D UI portrait flicker and font artifacts\n• Heavy crowds FPS drops in Shibuya and Shinjuku",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/68
{
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Рассинхрон звука при загрузке 8 бойцов\n• Микрофризы при активации спецэффектов",
        "• Audio desync with 8 active fighters\n• Visual effect stutter",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100A38018D5A000ULL,
        "EA SPORTS FC 24",
        "• Вылет при входе в режим Карьеры (SIGILL при загрузке динамических NRO-модулей Frostbite)\n• Зависание на заставке EA Connect\n• Утечки памяти в режиме карьеры",
        "• Career Mode crash (SIGILL on dynamic Frostbite NRO module load)\n• EA Connect splash freeze\n• Career mode memory leaks",
        "✓ Память: 8GB DRAM (критично для движка Frostbite и режима Карьеры)\n✓ Режим полета (В самолете): Включено (пропуск серверов EA)\n✓ Асинхронные шейдеры: Отключено\n✓ Патчер NCE для динамических NRO (ldr:ro): Включено\n✓ Авто-заглушки сервисов HLE: Включено",
        "✓ Memory Layout: 8GB DRAM (Critical for Frostbite & Career Mode)\n✓ Airplane Mode: Enabled\n✓ Asynchronous Shaders: Disabled\n✓ Runtime NRO NCE Patcher (ldr:ro): Enabled\n✓ HLE Service Auto-Stub: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Debugging\\disable_web_applet", "true"},
            {"Debugging\\use_auto_stub", "true"}
        },
        {0x0100346016EE8000ULL, 0x0100A38018D5A800ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3777
{
        0x01001F5010DFA000ULL,
        "Pokémon Legends: Arceus",
        "• Микрофризы при спавне диких покемонов в небе/траве\n• Артефакты освещения в разломах",
        "• Wild pokemon spawn micro-stutters\n• Space-time distortion lighting artifacts",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/yw2s5g/pokemon_scarlet_yuzu_turnip_crashing_fix/
{
        0x0100A3D008C5C000ULL,
        "Pokémon Scarlet / Violet",
        "• Утечки VRAM в городах и на водных просторах\n• Падение FPS и замедление анимаций",
        "• Severe VRAM leaks in towns and lakes\n• Frame drops and slowed NPC animations",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено\n✓ Реактивная очистка: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed\n✓ Reactive Flushing: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/52
{
        0x0100ABF008968000ULL,
        "Pokémon Sword / Shield",
        "• Заикания в Диких землях при подгрузке погодных условий\n• Мерцание спецэффектов Dynamax",
        "• Wild Area weather transition stutters\n• Dynamax battle effect flickering",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Точность ГПУ: Высокая",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ GPU Accuracy: High",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        },
        {0x01008DB008C2C000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3858
{
        0x01004D300C5AE000ULL,
        "Kirby and the Forgotten Land",
        "• Зависание анимаций врагов на 30 FPS\n• Артефакты отражений луж",
        "• Distant enemy 30 FPS stutter\n• Puddle reflection distortion",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/207
{
        0x010055D009F78000ULL,
        "Fire Emblem: Three Houses",
        "• Просадки FPS при масштабных битвах батальонов\n• Мерцание 2D портретов и меню",
        "• Battalion battle animation FPS drops\n• 2D UI portrait flickering",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4342
{
        0x0100A6301214E000ULL,
        "Fire Emblem Engage",
        "• Сбои шейдеров свечения колец Emblem\n• Микрофризы в Сомниэле",
        "• Emblem ring glow shader artifacts\n• Somniel hub micro-stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/186
{
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Падение кадров при цепных комбо-атаках легионов\n• Размытие динамических неоновых вывесок",
        "• Legion chain attack FPS drops\n• Blurry dynamic neon bloom reflections",
        "✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/247
{
        0x01006C300E9F0000ULL,
        "Dragon Quest XI S: Echoes of an Elusive Age",
        "• Микростаттеры при смене зон и переходе между 2D/3D режимами\n• Мерцание травы",
        "• Zone transition micro-stutters\n• Grass texture flickering",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100BB70144F8000ULL,
        "Prince of Persia: The Lost Crown",
        "• Статтеры при активации способностей управления временем\n• Искажение фоновых слоев",
        "• Time powers activation stutter\n• Background parallax distortion",
        "✓ Декодирование видео (NVDEC): На ЦП\n✓ Точность DMA: Безопасная\n✓ Динамическое состояние: Базовое (EDS1)\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ NVDEC Video Emulation: CPU\n✓ DMA Accuracy: Safe\n✓ Dynamic State: Basic (EDS1)\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Renderer\\astc_recompression", "0"}
        },
        {0x010034B01314C000ULL}
    },
    {
        0x01007820195A6000ULL,
        "Red Dead Redemption",
        "• Сбои освещения на закате и рассвете\n• Просадки FPS в городах Блэкуотер и Армадилло",
        "• Sunrise/sunset volumetric lighting corruption\n• Blackwater town FPS drops",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 6GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/334
{
        0x01003D100E9C6000ULL,
        "The Witcher 3: Wild Hunt",
        "• Вылеты памяти (OOM) и зависания диспетчера потоков REDengine\n• Мерцание теней и воды в Новиграде",
        "• REDengine thread deadlocks and host OOM memory crashes\n• Shadow and water flickering in Novigrad",
        "✓ Память: 6 ГБ DRAM (устраняет сбой аллокатора памяти)\n✓ Fastmem Exclusives: Отключено (устраняет дедлоки диспетчера потоков)\n✓ Игнорировать прерывания памяти: Включено\n✓ Точность ГПУ: Обычная\n✓ Обратные циклы барьеров: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ Memory Layout: 6GB DRAM (Prevents host allocator OOM)\n✓ Fastmem Exclusives: Disabled (Fixes REDengine thread deadlocks)\n✓ Ignore Memory Aborts: Enabled\n✓ GPU Accuracy: Normal\n✓ Barrier Feedback Loops: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_fastmem_exclusives", "false"},
            {"Renderer\\max_anisotropy", "5"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3373
{
        0x0100B04011742000ULL,
        "Monster Hunter Rise",
        "• Просадки FPS при битвах с Wyvern\n• Мерцание спецэффектов Wirebug",
        "• Wyvern combat FPS drops\n• Wirebug particle flickering",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"System\\memory_layout_mode", "0"}
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
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/230
{
        0x01006BB00C6F0000ULL,
        "The Legend of Zelda: Link's Awakening",
        "• Просадки FPS при размытии глубины резкости (Tilt-Shift)\n• Заикания в деревне Мэйб",
        "• Tilt-shift depth of field severe frame drops\n• Mabe Village traversal stutters",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Память: 4 ГБ DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4876
{
        0x01008CF01BAAC000ULL,
        "The Legend of Zelda: Echoes of Wisdom",
        "• Падения частоты кадров при создании копий предметов (Echoes)\n• Размытие воды",
        "• Echo creation frame drops\n• Water surface reflection distortion",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Быстрая память: Включено\n✓ Память: 4 ГБ DRAM",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100600017F7A000ULL,
        "Mario and Luigi: Brothership",
        "• Отображение сырых идентификаторов локализации (MES_SYS_UI_SAVELOAD_SUPPORTSAUTOSAVE) при русской системной локализации\n• Микрофризы и утечки памяти Unreal Engine 4\n• Просадки кадровой частоты",
        "• Raw localization keys (MES_SYS_UI_SAVELOAD_SUPPORTSAUTOSAVE) displayed with Russian system language\n• Unreal Engine 4 memory pressure and micro-stutters\n• Framerate drops",
        "✓ Память DRAM: 6 ГБ (устранение микрофризов UE4)\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ DRAM Memory Layout: 6GB (Fixes UE4 micro-stutters)\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "false"}
        },
        {0x0100600017F7A800ULL}
    },
    {
        0x010069200E60E000ULL,
        "Pikmin 4",
        "• Утечки памяти в открытых садах Unreal Engine 4\n• Микрофризы при спавне отряда Пикминов",
        "• Unreal Engine 4 open garden memory leaks\n• Pikmin squad spawn micro-stutters",
        "✓ Память: 6GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010067300059A000ULL,
        "Mario + Rabbids: Kingdom Battle",
        "• Диагональная полоса при запуске на полноэкранном кваде\n• Сильный пересвет и срез 3D-колонн Snowdrop Engine\n• Мерцание текстур персонажей",
        "• Diagonal line across fullscreen quad on startup\n• Severe overexposure and 3D pillar clipping (Snowdrop Engine)\n• Character model flickering",
        "✓ Память: 4GB DRAM + Режим полета\n✓ Точность ГПУ: Высокая (High)\n✓ DMA Accuracy: Безопасная (Safe=3) + Fix Bloom: Включено\n✓ Обратные циклы барьеров и Реактивная очистка: Включено\n✓ Асинхронные шейдеры и вывод: Отключено (синхронный тонемап HDR Snowdrop)\n✓ Обратное чтение буферов и Sync Memory Ops: Включено (автоэкспозиция Snowdrop)\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 4GB DRAM + Airplane Mode\n✓ GPU Accuracy: High\n✓ DMA Accuracy: Safe (3) + Fix Bloom Effects: Enabled\n✓ Barrier Feedback Loops & Reactive Flushing: Enabled\n✓ Async Shaders & Async Presentation: Disabled (synchronous Snowdrop HDR tonemapping)\n✓ GPU Buffer Readback & Sync Memory Ops: Enabled (Snowdrop auto-exposure)\n✓ ASTC Recompression: Uncompressed",
        {
            {"Core\\memory_layout_mode", "0"},
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\fix_bloom_effects", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\vram_garbage_collection", "true"},
            {"Renderer\\vram_budget_governor", "true"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\enable_gpu_buffer_readback", "true"}
        },
        {0x010067300B762000ULL}
    },
    {
        0x01005CA00F966000ULL,
        "Mario + Rabbids: Sparks of Hope",
        "• Сбои динамического освещения на планетах\n• Черные артефакты в катсценах и битвах\n• Просадки кадров",
        "• Planet lighting pass corruption\n• Black shadow artifacts during cutscenes and tactical battles\n• Frame drops",
        "✓ Точность ГПУ: Высокая\n✓ Обратные циклы барьеров: Включено\n✓ Реактивная очистка: Включено\n✓ Сжатие ASTC: Отключено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High\n✓ Barrier Feedback Loops: Enabled\n✓ Reactive Flushing: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\fix_bloom_effects", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"System\\memory_layout_mode", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/84
{
        0x01000BC0000A0000ULL,
        "Hollow Knight",
        "• Задержка ввода (задержки ввода) в босс-файтах\n• Микростаттеры при смене комнат",
        "• Boss fight frame latency\n• Room transition compilation stutters",
        "✓ Асинхронные шейдеры: Включено\n✓ Быстрая память: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100646009FBE000ULL,
        "Dead Cells",
        "• Задержка ввода в динамичных боевых секциях\n• Статтеры процедурной генерации",
        "• Combat frame latency\n• Procedural level generation stutters",
        "✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        },
        { 0x01003D200BAA2000ULL }
    },
    {
        0x01001B60133A2000ULL,
        "Outer Wilds",
        "• Просадки FPS при физическом расчете орбит планет\n• Утечки VRAM в туманностях",
        "• Solar system physics calculation slowdown\n• Space nebulae VRAM leaks",
        "✓ Память: 6GB DRAM\n✓ Точность ЦП: Точная\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ CPU Accuracy: Accurate\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\max_anisotropy", "5"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100913018F08000ULL,
        "Unicorn Overlord",
        "• Рассинхронизация 2D-анимаций Vanillaware в масштабных битвах\n• Размытие шрифтов интерфейса",
        "• Vanillaware 2D battle animation desync\n• Tactical interface font blur",
        "✓ Точность ГПУ: Высокая\n✓ Реактивная очистка: Включено\n✓ Анизотропная фильтрация: 16x\n✓ Быстрая память: Включено",
        "✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled\n✓ Anisotropic Filtering: 16x\n✓ Fastmem: Enabled"
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/16e1w3b/skyward_sword_hd_yuzu_graphic_glitch/
{
        0x01002DA013484000ULL,
        "The Legend of Zelda: Skyward Sword HD",
        "• Артефакты меча и курсора при управлении движением\n• Размытие текстур облачного моря Небоземи",
        "• Motion control sword and pointer jitter\n• Skyloft cloud sea texture shimmering",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010036601D380000ULL,
        "Super Mario Party Jamboree",
        "• Микрофризы в мини-играх на 20 игроков Koopathlon\n• Рассинхронизация счетчика очков",
        "• 20-player Koopathlon minigame stutters\n• Live board score counter desync",
        "✓ Память: 4GB DRAM\n✓ Точность ГПУ: Обычная (Normal 60 FPS)\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Отключено\n✓ Барьеры обратной связи: Включено\n✓ Сжатие ASTC: Отключено",
        "✓ Memory Layout: 4GB DRAM\n✓ GPU Accuracy: Normal (Normal 60 FPS)\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Disabled\n✓ Barrier Feedback Loops: Enabled\n✓ ASTC Recompression: Uncompressed",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x0100C9D013F0E000ULL, 0x0100965017338000ULL}
    },
    {
        0x010091801E8B2000ULL,
        "Mario & Luigi: Brothership",
        "• Задержка отклика в совместных Brother Attacks\n• Просадки FPS на морских островах Конкордии\n• Зависание на загрузке у MOD версий с большим RomFS",
        "• Brother Attacks timing lag\n• Concordia ocean sailing FPS dips\n• Loading freeze on MOD versions with large RomFS",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Сжатие ASTC: Отключено\n✓ Память: 6 ГБ\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ ASTC Recompression: Uncompressed\n✓ Memory: 6 GB\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4114
{
        0x0100C2500FC20000ULL,
        "Splatoon 3",
        "• Утечки памяти в хабе Плюхграда и в матчах Залива Самонид\n• Просадки FPS при обильном залитии карты краской и сетевые задержки лобби",
        "• Splatsville hub VRAM leaks\n• Salmon Run heavy ink coverage slowdown & match lobby delay",
        "✓ Режим полёта: Включено (офлайн-кампания без сетевых задержек)\n✓ Память: 8GB DRAM\n✓ Точность ГПУ: Высокая\n✓ Асинхронные шейдеры: Включено\n✓ Реактивная очистка: Включено",
        "✓ Airplane Mode: Enabled (Instant offline Hero mode)\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: High\n✓ Asynchronous Shaders: Enabled\n✓ Reactive Flushing: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\astc_recompression", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3501
{
        0x0100000011D90000ULL,
        "Pokemon Brilliant Diamond",
        "• Мерцание чиби-отражений в лужах и окнах Синно\n• Задержка открытия меню покедекса",
        "• Chibi puddle & window reflection flicker\n• Pokedex animation transition lag",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/186
{
        0x01007300020FA000ULL,
        "Astral Chain",
        "• Просадки FPS при вызове Легионов и комбо-атаках в Арке\n• Утечки VRAM при длительной игре",
        "• Legion summon and chain sync combo FPS drops\n• Long session VRAM exhaustion in the Ark",
        "✓ Конфигурация памяти: 6 ГБ DRAM\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Memory Layout: 6GB DRAM\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\max_anisotropy", "5"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/52
{
        0x0100ABF008968000ULL,
        "Pokemon Sword & Shield",
        "• Просадки FPS при динамической смене погоды в Диких землях Галара\n• Мерцание свечения Гигантамакса",
        "• Galar Wild Area weather dynamic lighting drops\n• Dynamax aura rendering flicker",
        "✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\max_anisotropy", "5"}
        },
        {0x01008DB008C2C000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/68
{
        0x01006A800016E000ULL,
        "Super Smash Bros. Ultimate",
        "• Инпут-лаг и просадки кадров в боях на 8 игроков\n• Зависание катсцен и задержки проверки Духов онлайн",
        "• 8-fighter intense brawl input latency\n• Spirit Board online latency & cutscene sync pause",
        "✓ Режим полёта: Включено (пропуск онлайн-опроса Духов)\n✓ Точность ЦП: Точная\n✓ Точность ГПУ: Высокая\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Airplane Mode: Enabled (Bypasses Spirit Board network check)\n✓ CPU Accuracy: Accurate\n✓ GPU Accuracy: High\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
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
        "• Зависание на логотипах 2K и Gearbox при запуске (Borderlands 2)\n• Задержка старта на 60+ секунд при сетевом опросе SHiFT\n• Просадки FPS при взрывах стихий",
        "• Hang on 2K and Gearbox splash logos on boot (Borderlands 2)\n• 60+ second boot freeze during SHiFT network ping\n• Elemental explosion particle slowdowns",
        "✓ Синхронизация операций памяти: Включено (устраняет зависание на логотипе 2K/Gearbox)\n✓ Режим полёта: Включено (мгновенный старт без ожидания SHiFT)\n✓ Память: 6 ГБ DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ Sync Memory Operations: Enabled (Fixes 2K/Gearbox logo hang)\n✓ Airplane Mode: Enabled (Instant boot without SHiFT timeout)\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\sync_memory_operations", "true"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"}
        },
        {0x010064E009F7E000ULL, 0x0100C5800B70E000ULL, 0x01008C30113B0000ULL}
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        },
        {0x0100D560130C6000ULL}
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
            {"Renderer\\gpu_accuracy", "2"}
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        },
        {0x010078D000F88000ULL}
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
            {"System\\memory_layout_mode", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/4379
{
        0x0100121014688000ULL,
        "Metroid Prime Remastered",
        "• Артефакты визора и HUD-интерфейса\n• Микрофризы при открытии переходных дверей между комнатами",
        "• Combat/Scan visor HUD graphical glitches\n• Room transition door loading stutters",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено (исправление визора)\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed (Fixes visor artifacts)\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/2389
    {
        0x010087201207C000ULL,
        "Kingdoms of Amalur: Re-Reckoning",
        "• Вылет или зависание на черном экране при запуске в режиме NCE на Android\n• Микрофризы при подгрузке открытых зон",
        "• Crash or black screen hang on startup under NCE on Android\n• Micro-stutters during open zone streaming",
        "✓ Бэкенд ЦП: Dynarmic JIT (требуется для стабильной работы 32-бит/64-бит кода движка)\n✓ Точность ГПУ: Обычная\n✓ Память: 6GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ CPU Backend: Dynarmic JIT (Required for engine code stability)\n✓ GPU Accuracy: Normal\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"Cpu\\cpu_backend", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {0x010066E012594000ULL}
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/39
{
        0x0100E95004038000ULL,
        "Xenoblade Chronicles 2",
        "• Размытие динамического разрешения и нехватка VRAM в провинции Гормотт\n• Просадки частоты кадров в битвах",
        "• Dynamic resolution blur and VRAM memory buildup in Gormott\n• Combat framerate drops",
        "✓ Точность ГПУ: Обычная\n✓ Сжатие ASTC: Отключено\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: Normal\n✓ ASTC Recompression: Uncompressed\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"System\\memory_layout_mode", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/3777
{
        0x01001F5010DFA000ULL,
        "Pokemon Legends: Arceus",
        "• Микрофризы и просадки FPS при спавне диких Покемонов\n• Утечки памяти в деревне Джубилайф",
        "• Micro-stutters and framerate drops during wild Pokemon spawns\n• Memory pressure in Jubilife Village",
        "✓ Точность ГПУ: Обычная (стабильные 30 FPS)\n✓ Память: 6GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal (Smooth 30 FPS)\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/yw2s5g/pokemon_scarlet_yuzu_turnip_crashing_fix/
{
        0x0100A3D008C5C000ULL,
        "Pokemon Scarlet",
        "• Тяжелые утечки памяти в открытом мире Палдеи (>9 ГБ видеопамяти)\n• Падения FPS и рывки камеры",
        "• Massive open world memory leaks in Paldea (>9 GB VRAM)\n• Framerate drops and camera stuttering",
        "✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено\n✓ Память: 6GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 6GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/yx72g7/pokemon_violet_memory_leak_fix_yuzu_android/
{
        0x01008F6008C5E000ULL,
        "Pokemon Violet",
        "• Тяжелые утечки памяти в открытом мире Палдеи (>9 ГБ видеопамяти)\n• Падения FPS и рывки камеры",
        "• Massive open world memory leaks in Paldea (>9 GB VRAM)\n• Framerate drops and camera stuttering",
        "✓ Точность ГПУ: Обычная\n✓ Реактивная очистка: Отключено\n✓ Память: 8GB DRAM\n✓ Быстрая память: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Memory Layout: 8GB DRAM\n✓ Fastmem: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "2"},
            {"Cpu\\cpuopt_fastmem", "true"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010071B01D260000ULL,
        "S.T.A.L.K.E.R.: Legends of the Zone Trilogy / OpenXRay Switch",
        "• Зависание и вылет при запуске из-за необработанных HLE-сервисов и онлайн-проверок\n• Нехватка стандартной памяти в локациях Зоны (Тень Чернобыля, Чистое Небо, Зов Припяти)",
        "• Startup crash and hang due to unhandled HLE services and online checks\n• Memory exhaustion in Zone locations (Shadow of Chernobyl, Clear Sky, Call of Pripyat)",
        "✓ Авто-заглушки сервисов HLE (Auto-Stub): Включено\n✓ Режим полёта: Включено\n✓ Конфигурация памяти: 8 ГБ DRAM\n✓ Точность ГПУ: Обычная\n✓ Быстрая память: Включено",
        "✓ HLE Service Auto-Stub: Enabled\n✓ Airplane Mode: Enabled\n✓ Memory Layout: 8GB DRAM\n✓ GPU Accuracy: Normal\n✓ Fastmem: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"Debugging\\use_auto_stub", "true"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        },
        {0x0100C6B01D262000ULL, 0x010004401D264000ULL, 0x0100000000000053ULL}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\gpu_accuracy", "2"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
        // src: https://github.com/Ryujinx/Ryujinx-Games-List/issues/2857
{
        0x010093801237C000ULL,
        "Metroid Dread",
        "• Просадки частоты кадров до 15-20 FPS на Snapdragon 8 Gen 3 (Adreno 750) из-за высокой точности ГПУ\n• Микрофризы при входе в зоны E.M.M.I.",
        "• Heavy framerate drops to 15-20 FPS on Snapdragon 8 Gen 3 (Adreno 750) due to high GPU accuracy\n• E.M.M.I. zone transition micro-stutters",
        "✓ Точность ГПУ: Обычная (стабильные 60 FPS на Snapdragon 8 Gen 3 / Adreno 750)\n✓ Реактивная очистка: Отключено\n✓ Быстрое время ГПУ: Включено\n✓ Обратные циклы барьеров: Включено\n✓ Асинхронные шейдеры: Включено",
        "✓ GPU Accuracy: Normal (Stable 60 FPS on Snapdragon 8 Gen 3 / Adreno 750)\n✓ Reactive Flushing: Disabled\n✓ Fast GPU Time: Enabled\n✓ Barrier Feedback Loops: Enabled\n✓ Asynchronous Shaders: Enabled",
        {
            {"System\\memory_layout_mode", "0"},
            {"System\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "false"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
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
            {"Renderer\\gpu_accuracy", "2"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"System\\airplane_mode", "true"},
            {"System\\memory_layout_mode", "0"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\astc_recompression", "0"}
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
            {"System\\memory_layout_mode", "0"},
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
            {"System\\memory_layout_mode", "0"},
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
        // src: https://reddit.com/r/EmulationOnAndroid/comments/y9w5h4/persona_5_royal_yuzu_android_portrait_flicker/
{
        0x010053201F9B4000ULL,
        "Persona 5 Royal",
        "• Черный экран / зависание ГПУ на драйверах Qualcomm Adreno\n• Некорректный clamping семплеров текстур",
        "• Black screen / GPU hang on Qualcomm Adreno drivers\n• Incorrect texture sampler clamping behavior",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
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
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
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
            {"Renderer\\dma_accuracy", "3"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"System\\memory_layout_mode", "0"},
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "0"},
            {"System\\memory_layout_mode", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
        // src: https://reddit.com/r/EmulationOnAndroid/comments/yd739u/alan_wake_remastered_switch_yuzu_fog_glitch/
{
        0x0100236015B7A000ULL,
        "Alan Wake Remastered",
        "• Искажение луча света фонарика и артефакты теней\n• Размытие и нечеткость шрифтов субтитров и интерфейса",
        "• Flashlight cone illumination artifacts and shadow glitch\n• Blurry subtitle fonts and distorted UI text rendering",
        "✓ Точность ГПУ: Высокая\n✓ Сжатие ASTC: Отключено (четкие шрифты и интерфейс)\n✓ Динамическое состояние: EDS1\n✓ Обратное чтение буферов ГПУ: Включено\n✓ Барьеры ГПУ и точность DMA: Сбалансированный / Безопасная\n✓ Быстрое время ГПУ: Отключено\n✓ Память: 6 ГБ DRAM",
        "✓ GPU Accuracy: High\n✓ ASTC Recompression: Uncompressed (sharp text and UI)\n✓ Dynamic State: EDS1\n✓ GPU Buffer Readback: Enabled\n✓ GPU Fence & DMA Accuracy: Balanced / Safe\n✓ Fast GPU Time: Disabled\n✓ Memory Layout: 6GB DRAM",
        {
            {"System\\memory_layout_mode", "1"},
            {"Core\\memory_layout_mode", "1"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\dma_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "1"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Renderer\\scaling_filter", "1"},
            {"Renderer\\anti_aliasing", "0"}
        },
        {0x0100F830138E6000ULL, 0x010074B01456A000ULL, 0x010065F014C5E000ULL, 0x0100AD6014FB8000ULL, 0x0100328014FB8000ULL, 0x0100AD5013BB6000ULL}
    },
    {
        0x019232F2781D0000ULL,
        "Dr. Mario Mania",
        "• Сбои сетевой инициализации BSD-сокетов при активном режиме «В самолете»\n• Сбои выделения кучи HB Loader",
        "• Network socket initialization failure when airplane mode is forced\n• HB Loader applet heap allocation faults",
        "✓ Режим «В самолете»: Отключено (сеть активна)\n✓ Точность ГПУ: Обычная (высокая производительность)\n✓ Конфигурация памяти: 4 ГБ DRAM (стандарт)\n✓ Асинхронные шейдеры: Включено\n✓ Асинхронный вывод: Включено\n✓ Синхронизация операций памяти: Отключено",
        "✓ Airplane Mode: Disabled (Network active)\n✓ GPU Accuracy: Normal (High performance)\n✓ Memory Layout: 4GB DRAM (Standard)\n✓ Asynchronous Shaders: Enabled\n✓ Async Presentation: Enabled\n✓ Sync Memory Operations: Disabled",
        {
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01052BA7AC450000ULL,
        "Gizmoduck",
        "• Задержки компиляции шейдеров Godot Engine\n• Сбои инициализации сетевых сокетов libnx при активном режиме полета",
        "• Godot Engine shader compilation stalls\n• libnx network socket initialization failure with airplane mode",
        "✓ Режим «В самолете»: Отключено (сеть активна)\n✓ Точность ГПУ: Обычная (высокий FPS)\n✓ Конфигурация памяти: 4 ГБ DRAM (стандартная Switch Homebrew)\n✓ Асинхронные шейдеры: Включено\n✓ Асинхронный вывод: Включено\n✓ Синхронизация операций памяти: Отключено (устранение зависаний вывода Godot)",
        "✓ Airplane Mode: Disabled (Network active)\n✓ GPU Accuracy: Normal (High FPS)\n✓ Memory Layout: 4GB DRAM (Standard Switch Homebrew)\n✓ Asynchronous Shaders: Enabled\n✓ Async Presentation: Enabled\n✓ Sync Memory Operations: Disabled (Fixes Godot presentation stalls)",
        {
            {"System\\airplane_mode", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01542031DCEC0000ULL,
        "Super Mario Bros. Remastered",
        "• Задержки компиляции шейдеров Godot\n• Сбои синхронизации видеобуферов",
        "• Godot shader compilation stalls\n• GPU framebuffer synchronization faults",
        "✓ Синхронизация операций памяти: Отключено (устранение зависаний вывода)\n✓ Точность ГПУ: Обычная\n✓ Сеть: Включено\n✓ Асинхронный вывод: Включено\n✓ Конфигурация памяти: 4 ГБ DRAM",
        "✓ Sync Memory Operations: Disabled (Fixes presentation stall)\n✓ GPU Accuracy: Normal\n✓ Network: Enabled\n✓ Async Presentation: Enabled\n✓ Memory Layout: 4GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\dma_accuracy", "0"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "false"},
            {"System\\memory_layout_mode", "0"}
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
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "1"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\vram_garbage_collection", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "false"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x010040F01EC60000ULL,
        "Aggelos 2",
        "• Черный экран при старте из-за рассинхронизации 2D-движка Clickteam Fusion при асинхронном выводе\n• Зависание опроса сетевых сокетов\n• Сбои барьеров памяти при загрузке динамических спрайтовых буферов",
        "• Black screen on boot caused by Clickteam Fusion 2D presentation queue desync with async presentation\n• Network socket polling hang\n• Memory barrier faults during dynamic sprite buffer streaming",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpu_backend", "1"}
        }
    },
    {
        0x0100A83022A22000ULL,
        "Absolum",
        "• Рассинхронизация 2D-презентации кадров и чёрный фон за 2D-сценой\n• Задержка вступительных роликов\n• Излишний нагрев из-за принудительных барьеров памяти",
        "• 2D presentation desync and black 2D background artwork\n• Intro video startup delay\n• Excessive heat from forced memory sync stalls",
        "✓ Память: 4GB DRAM + Режим полета\n✓ Точность ГПУ: Высокая (High) + Fix Bloom + NVDEC ГПУ\n✓ Асинхронный вывод и быстрое время ГПУ: Отключено (0% рассинхронизации 2D-слоёв)\n✓ Обратное чтение и Sync Memory Ops: Включено (DMA Safe=3)\n✓ Асинхронные шейдеры, реактивная очистка, барьеры и GC VRAM: Включено",
        "✓ Memory Layout: 4GB DRAM + Airplane Mode\n✓ GPU Accuracy: High + Fix Bloom + GPU NVDEC\n✓ Async Presentation & Fast GPU Time: Disabled (0% 2D layer desync)\n✓ GPU Buffer Readback & Sync Memory Ops: Enabled (DMA Safe=3)\n✓ Asynchronous Shaders, Reactive Flushing, Barrier Loops & VRAM GC: Enabled",
        {
            {"Core\\memory_layout_mode", "0"},
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\fix_bloom_effects", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\vram_garbage_collection", "true"},
            {"Renderer\\vram_budget_governor", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\dma_accuracy", "3"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"System\\airplane_mode", "true"},
            {"Network\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x010004D00A9C0000ULL,
        "Aggelos",
        "• Черный экран при старте из-за рассинхронизации 2D-движка Clickteam Fusion при асинхронном выводе\n• Зависание опроса сетевых сокетов\n• Сбои барьеров памяти при загрузке спрайтовых буферов",
        "• Black screen on boot caused by Clickteam Fusion 2D presentation queue desync with async presentation\n• Network socket polling hang\n• Memory barrier faults during sprite buffer streaming",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpu_backend", "1"}
        }
    },
    {
        0x0100F3E024DFC000ULL,
        "Another Eden Begins",
        "• Задержки компиляции шейдеров Unity при анимациях навыков\n• Зависание при опросе сетевых служб эмулятором",
        "• Unity shader compilation stutter during skill animations\n• Network socket timeout hangs during boot sequence",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x0100FC001ACE0000ULL,
        "Anvil Saga",
        "• Артефакты пиксельного текста интерфейса кузницы при пересжатии ASTC\n• Утечки памяти при длительных игровых сессиях",
        "• Smithy UI text pixelation and font artifacts with ASTC recompression\n• Memory fragmentation during long sessions",
        "",
        "",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01006DD02868A000ULL,
        "Artis Impact",
        "• Микростаттеры кадрового пейсинга 2D-сцен\n• Зависание при фоновом сетевом обращении",
        "• 2D scene frame pacing jitter\n• Background network socket hangs",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x0100EAE010560000ULL,
        "Captain Tsubasa - Rise of New Champions",
        "• Зависание вступительных и сюжетных видеороликов при аппаратном NVDEC\n• Вылет Device Loss и сбои при переходе от заставки к матчу\n• Дублирование и наложение полупрозрачных текстур лица/глаз при масштабировании разрешения 2X+\n• Статтеры при анимации супер-ударов на стадионе\n• Нехватка памяти при 27 DLC",
        "• Cutscene and opening video freezes on GPU NVDEC\n• Device Loss crash during transition from cutscene to match\n• Ghosting and duplicate face/eye decal overlay at 2X+ resolution scaling\n• Super shot cinematic animation stutters\n• Out of memory with 27 DLCs",
        "✓ Точность ГПУ: Высокая (устранение графических багов)\n✓ Быстрое время ГПУ: Отключено (исправление вылета Device Loss после заставки)\n✓ Динамическое состояние: Базовое (EDS1, стабильность конвейеров)\n✓ Синхронизация памяти: Включено (стабильность Unreal Engine 4 и устранение Device Loss)\n✓ Barrier Feedback Loops: Отключено (стабильность конвейеров)\n✓ Декодирование ASTC: ГПУ\n✓ NVDEC: Программный (устранение зависаний видеороликов)\n✓ Совместимое масштабирование (Legacy Rescale): Включено (устранение наложения текстур лица и глаз при 2X+)\n✓ Память: 6GB DRAM (стабильность со всеми DLC)",
        "✓ GPU Accuracy: High (Fixes graphical glitches)\n✓ Fast GPU Time: Disabled (Fixes Device Loss crash after cutscenes)\n✓ Dynamic State: Basic (EDS1, pipeline stability)\n✓ Sync Memory Operations: Enabled (Unreal Engine 4 stability and Device Loss fix)\n✓ Barrier Feedback Loops: Disabled (Pipeline stability)\n✓ ASTC Decoding: GPU\n✓ NVDEC: CPU (Fixes cutscene freezes)\n✓ Rescale Compatibility Mode: Enabled (Fixes face and eye decal ghosting at 2X+)\n✓ Memory Layout: 6GB DRAM (Stability with all DLCs)",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "0"}
        },
        {
            0x0100EA6010560000ULL,
            0x0100EA6010560800ULL,
            0x0100EAE010560800ULL
        }
    },
    {
        0x01009720213B0000ULL,
        "Captain Tsubasa 2 - World Fighters",
        "• Сбои после создания персонажа при неточной синхронизации ГПУ\n• Вылет Device Loss и сбои при переходе от заставки к матчу\n• Задержки компиляции шейдеров спецэффектов футбольных ударов\n• Нагрузка на память при наличии дополнений",
        "• Crashes after character creation on inaccurate GPU sync\n• Device Loss crash during transition from cutscene to match\n• Shader compilation drops during special soccer shot effects\n• Memory pressure with DLCs installed",
        "✓ Точность ГПУ: Высокая (устранение сбоев после создания персонажа)\n✓ Быстрое время ГПУ: Отключено (исправление вылета Device Loss после заставки)\n✓ Динамическое состояние: Базовое (EDS1, стабильность конвейеров)\n✓ Синхронизация памяти: Включено (стабильность Unreal Engine 4 и устранение Device Loss)\n✓ Barrier Feedback Loops: Отключено (стабильность конвейеров)\n✓ Декодирование ASTC: ГПУ\n✓ NVDEC: Программный (устранение зависаний видеороликов)\n✓ Совместимое масштабирование (Legacy Rescale): Включено (устранение наложения текстур лица и глаз при 2X+)\n✓ Вычислительные конвейеры: Включено\n✓ Игнорирование сбоев памяти: Включено\n✓ Быстрая память: Включено\n✓ Память: 8GB DRAM (стабильность с дополнениями и модами)",
        "✓ GPU Accuracy: High (Fixes character creation crash)\n✓ Fast GPU Time: Disabled (Fixes Device Loss crash after cutscenes)\n✓ Dynamic State: Basic (EDS1, pipeline stability)\n✓ Sync Memory Operations: Enabled (Unreal Engine 4 stability and Device Loss fix)\n✓ Barrier Feedback Loops: Disabled (Pipeline stability)\n✓ ASTC Decoding: GPU\n✓ NVDEC: CPU (Cutscene video stability)\n✓ Rescale Compatibility Mode: Enabled (Fixes face and eye decal ghosting at 2X+)\n✓ Compute Pipelines: Enabled\n✓ Ignore Memory Aborts: Enabled (Fixes UE4 task graph aborts)\n✓ Fastmem: Enabled\n✓ Memory Layout: 8GB DRAM (DLC and mod stability)",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\barrier_feedback_loops", "false"},
            {"Renderer\\accelerate_astc", "1"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\dyna_state", "1"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01FF434243555253ULL,
        "Castlevania: Belmont's Curse",
        "• Зависание и чёрный экран при запуске на Adreno 6xx / Snapdragon 870\n• Сбои декодирования вступительных видеороликов NVDEC\n• Сбои кастомных ASTC текстур",
        "• Black screen / boot hang on Adreno 6xx / Snapdragon 870\n• NVDEC boot video playback glitches\n• Custom ASTC texture decoding issues",
        "",
        "",
        {
            {"Cpu\\cpu_backend", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpu_accuracy", "0"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\barrier_feedback_loops", "true"}
        }
    },
    {
        0x0100FA501AF90000ULL,
        "Castlevania - Dominus Collection",
        "• Рассинхронизация двухэкранного вывода NDS при асинхронном выводе кадров\n• Ошибки позиционирования сенсорного экрана\n• Размытие шрифтов при сжатии ASTC",
        "• Dual-screen NDS presentation desync with async presentation\n• Touchscreen coordinate precision errors\n• Font blurring under ASTC recompression",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100622020F5A000ULL,
        "Clockwork Rabbit",
        "• Дрожание 2D-пикселей при скроллинге уровня\n• Рассинхронизация звуковых дорожек",
        "• 2D pixel jitter during horizontal scrolling\n• Audio track synchronization issues",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100A4601ECA8000ULL,
        "Crypt Custodian",
        "• Зависание и аварийная остановка на экране LOADING... (gml_Object_o_load_Step_0) в движке GameMaker Studio 2 YYC из-за ошибки округления FRINTX/llrint при предзагрузке 21 текстурной группы\n• Артефакты прозрачности частиц и шрифтов при пересжатии ASTC",
        "• LOADING... screen hang/abort (gml_Object_o_load_Step_0) in GameMaker Studio 2 YYC caused by FRINTX/llrint rounding overflow when prefetching 21 texture groups\n• Transparent particle and font artifacts with ASTC recompression",
        "",
        "",
        {
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        },
        {0x0100A4601ECA8800ULL}
    },
    {
        0x0100F8F00C4F2000ULL,
        "DC Super Hero Girls - Teen Power",
        "• Просадки FPS в открытом городе Метрополис\n• Дроп динамического разрешения при быстром перемещении",
        "• Framerate drops in Metropolis open city area\n• Dynamic resolution scaling drops during fast traversal",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100C6A0235D4000ULL,
        "Devil Jam",
        "• Задержка ввода и рассинхронизация ритмических таймингов с музыкой\n• Микрофризы рендеринга",
        "• Input latency and rhythm timing desync with soundtrack\n• Rendering microstutters",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x010058C017024000ULL,
        "Dungeons 3 [Nintendo Switch Edition]",
        "• Тяжелый расчет физики и ИИ существ подземелья\n• Утечки памяти и вылеты OOM при долгих игровых сессиях",
        "• Heavy dungeon creature simulation and lighting calculations\n• Memory fragmentation and out of memory crashes on long sessions",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
    {
        0x0100F7901971C000ULL,
        "Dungeons 4 [Nintendo Switch Edition]",
        "• Интенсивный расчет физики многотысячных армий подземелья\n• Переполнение видеобуферов и памяти 4 ГБ",
        "• Intensive multi-unit physics and underworld simulation\n• 4GB DRAM memory buffer exhaustion",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
    {
        0x01001BB01E8E2000ULL,
        "Fantasian Neo Dimension",
        "• Искажение фотореалистичных диорамных фонов при сжатии ASTC\n• Статтеры при переходах боевой камеры\n• Зависание видеовставок",
        "• Photographic diorama background compression artifacts under ASTC recompression\n• Battle camera transition stutters\n• Video cutscene hangs",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100FF100FB68000ULL,
        "Finding Teddy II [Definitive Edition]",
        "• Рассинхронизация 2D-презентации кадров\n• Артефакты пиксель-арта при масштабировании",
        "• 2D presentation desync with async presentation\n• Pixel art scaling artifacts",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01000EB0276F2000ULL,
        "Garfield - Escape from Monday",
        "• Шейдерные задержки при смене комнат\n• Зависание вступительных роликов",
        "• Room transition shader stutters\n• Intro video playback freezes",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01007DE013A48000ULL,
        "Golden Force",
        "• Задержка ввода в динамичных платформенных секциях\n• Нестабильность кадровой частоты 60 FPS\n• Чёрный экран и сбой конвейера Unity при низкой точности ГПУ и модах локализации",
        "• Input latency during precision platforming sections\n• 60 FPS frame pacing instability\n• Black screen and Unity pipeline crash on Low GPU Accuracy with localization mods",
        "✓ Точность ГПУ: Высокая\n✓ Вычислительные конвейеры: Включено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High\n✓ Compute Pipelines: Enabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x010027901C89C000ULL,
        "Hello Kitty - Island Adventure [Deluxe Edition]",
        "• Просадки FPS при исследовании открытого 3D-острова\n• Зависания при фоновом сетевом поиске игроков",
        "• Framerate drops during open 3D island traversal\n• Freezes caused by background matchmaking socket polling",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x0100B36008F90000ULL,
        "Inazuma Eleven - Victory Road",
        "• Зависание аниме-роликов при аппаратном декодировании NVDEC\n• Статтеры при командных спецприемах на стадионе\n• Нехватка памяти 4 ГБ",
        "• Anime cutscene freezing on GPU NVDEC\n• Stadium special move shader stutters\n• 4GB DRAM memory exhaustion",
        "",
        "",
        {
            {"Renderer\\nvdec_emulation", "3"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\async_presentation", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x010092A0172E4000ULL,
        "It Takes Two",
        "• Двойная нагрузка рендеринга разделенного экрана (Split-Screen)\n• Вылеты по нехватке памяти при использовании русской озвучки (MOD - RUS VOICE)",
        "• Heavy double-viewport split-screen rendering draw calls\n• Out-of-memory crashes when using LayeredFS Russian voice mod",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
    {
        0x0100EB60202C8000ULL,
        "Kalanoro",
        "• Рассинхронизация 2D-кадров\n• Размытие текстовых диалогов при сжатии текстур",
        "• 2D frame presentation pacing jitter\n• Dialogue font blurring under texture recompression",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\async_presentation", "false"}
        }
    },
    {
        0x0100BB901FA12000ULL,
        "Little Big Adventure - Twinsen's Quest",
        "• Задержки компиляции шейдеров Unreal Engine 4 при переходе между островами\n• Высокое потребление VRAM",
        "• Unreal Engine 4 shader compilation stutters across islands\n• Elevated VRAM memory footprint",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100E1C0252F8000ULL,
        "Maestro",
        "• Критическая задержка аудио и видео при дирижировании оркестром\n• Зависание вступительных видеороликов",
        "• Audio-visual latency critical for orchestra conducting score\n• Intro video playback freezes",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100317013770000ULL,
        "Mario + Rabbids - Sparks of Hope",
        "• Нагрузка движка Snowdrop на видеобуфер и память 4 ГБ\n• Зависания при сетевом опросе серверов Ubisoft Connect\n• Статтеры тактических спецэффектов",
        "• Snowdrop engine memory pressure exceeding 4GB DRAM\n• Hangs caused by Ubisoft Connect server network polling\n• Tactical effect shader stutters",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x0100C9C00E25C000ULL,
        "Mario Golf - Super Rush",
        "• Просадки FPS в режиме Speed Golf при одновременном беге игроков\n• Шейдерные задержки геометрии травы",
        "• Speed Golf simultaneous runner physics framerate dips\n• Grass terrain shader compilation lag",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100B99019412000ULL,
        "Mario vs. Donkey Kong",
        "• Зависание видеороликов сюжета при аппаратном NVDEC\n• Статтеры анимаций головоломок",
        "• Story FMV video playback hangs on hardware NVDEC\n• Puzzle stage animation stutters",
        "",
        "",
        {
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100EA80032EA000ULL,
        "New Super Mario Bros. U Deluxe",
        "• Микростаттеры синхронизации 60 FPS платформера\n• Замыливание текстур задников при сжатии ASTC",
        "• 60 FPS platforming synchronization jitter\n• Background texture blurring under ASTC recompression",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100ECD018EBE000ULL,
        "Paper Mario - The Thousand-Year Door",
        "• Сбои буферов отражения пола театральной сцены\n• Искажение геометрии бумажного складывания при обычной точности ГПУ",
        "• Stage floor reflection buffer rendering issues\n• Paper folding geometry inaccuracies on Normal GPU accuracy",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100C3801C786000ULL,
        "Poppy Playtime - Chapter 1",
        "• Шейдерные статтеры освещения фабрики игрушек Unreal Engine 4\n• Зависание вступительных видеокассет VHS при аппаратном NVDEC\n• Нехватка памяти",
        "• Unreal Engine 4 toy factory lighting shader stutters\n• VHS tape video playback freezes on GPU NVDEC\n• 4GB DRAM memory exhaustion",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100D3801E6CE000ULL,
        "Poppy Playtime - Chapter 2",
        "• Просадки FPS при погоне Mommy Long Legs\n• Зависания видеокассет VHS\n• Вылеты по нехватке памяти",
        "• Framerate drops during Mommy Long Legs chase sequence\n• VHS cutscene playback freezes\n• Out-of-memory crashes",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100BD601EC3E000ULL,
        "Poppy Playtime - Chapter 3",
        "• Высокая нагрузка на VRAM в открытой локации Playcare\n• Зависания видеороликов\n• Переполнение памяти",
        "• Playcare hub volumetric smoke and lighting VRAM load\n• Cutscene video freezes\n• Memory exhaustion",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100A2902051A000ULL,
        "Poppy Playtime - Chapter 4",
        "• Шейдерные задержки глубоких подземных комплексов\n• Зависания сюжетных видеороликов\n• Утечки памяти",
        "• Deep underground complex shader stutters\n• Narrative video playback freezes\n• Memory allocation pressure",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x010075D026910000ULL,
        "Poppy Playtime - Chapter 5",
        "• Пиковые нагрузки геометрии финального акта\n• Зависания видеороликов\n• Риск крашей на 4 ГБ DRAM",
        "• Peak geometry streaming load in final chapter\n• Video playback freezes\n• Crash risk on 4GB DRAM",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01008E20257E0000ULL,
        "Qliphah in Providence's Shadow",
        "• Зависание и вылет после первого удара в бою из-за нехватки памяти (4GB) и отключенных compute-шейдеров спецэффектов\n• Артефакты темных полутонов и освещения\n• Замыливание спрайтов при сжатии ASTC",
        "• Freeze and crash after first combat hit due to 4GB memory limit and disabled compute shaders\n• Dark lighting gradient banding artifacts\n• Sprite blurring under ASTC recompression",
        "✓ Память: 8GB DRAM (устранение вылетов при боевых спецэффектах и модах)\n✓ Вычислительные конвейеры: Включено (устранение зависания при первом ударе)\n✓ Точность ГПУ: Высокая\n✓ Синхронизация и Reactive Flushing: Включено",
        "✓ Memory Layout: 8GB DRAM (Fixes combat hit OOM crashes with mods)\n✓ Compute Pipelines: Enabled (Fixes freeze on first hit)\n✓ GPU Accuracy: High\n✓ Reactive Flushing: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\enable_compute_pipelines", "true"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01008970149B0000ULL,
        "Rabbids - Party of Legends",
        "• Микрофризы мини-игр для 4 игроков\n• Зависания при сетевом поиске серверов\n• Статтеры видеороликов",
        "• 4-player party minigame physics microstutters\n• Network socket hangs during matchmaking\n• Video playback stutters",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"}
        }
    },
    {
        0x01009D6022DC2000ULL,
        "Reus 2",
        "• Нагрузка симуляции планетарных биомов на оперативную память\n• Искажение текстур гигантов при сжатии ASTC",
        "• Planetary biome simulation memory consumption\n• Giant creature texture distortion under ASTC compression",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x01009A5009A9E000ULL,
        "Shining Resonance Refrain",
        "• Зависание аниме-роликов при аппаратном NVDEC\n• Сбои синхронизации музыкальной боевой системы B.A.N.D.",
        "• Anime cutscene freezing on GPU NVDEC\n• B.A.N.D. musical combat system synchronization issues",
        "",
        "",
        {
            {"Renderer\\nvdec_emulation", "2"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100FD8022DAA000ULL,
        "Super Mario Galaxy 2",
        "• Просадки FPS при расчете гравитации сферических планет\n• Ошибки буферов размытия и свечения",
        "• Framerate drops during spherical planet gravity calculations\n• Bloom lighting and blur buffer errors",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010099C022B96000ULL,
        "Super Mario Galaxy",
        "• Зависание вступительного ролика при аппаратном декодировании\n• Ошибки позиционирования указателя Star Pointer",
        "• Opening cinematic freeze with hardware NVDEC\n• Star Pointer render coordinate precision issues",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01009B90006DC000ULL,
        "Super Mario Maker 2",
        "• Зависание на вечной загрузке при сетевом поиске уровней онлайн\n• Размытие иконок элементов редактора при сжатии ASTC",
        "• Infinite loading hang on online level search socket polling\n• Level editor palette icon blurring with ASTC compression",
        "",
        "",
        {
            {"System\\airplane_mode", "true"},
            {"Services\\airplane_mode", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010022201229A000ULL,
        "Super Robot Wars 30",
        "• Размытие высокодетализированных спрайтов роботов при сжатии ASTC\n• Зависание видеовставок атак при аппаратном NVDEC\n• Вылеты по памяти при 16 DLC",
        "• High-resolution mecha sprite blurring under ASTC recompression\n• Attack animation video hangs on hardware NVDEC\n• Memory pressure with 16 DLCs",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x01006C900CC60000ULL,
        "Super Robot Wars T",
        "• Потеря четкости контуров боевой анимации мехов\n• Зависание сюжетных роликов\n• Переполнение памяти при 25 DLC",
        "• Mecha attack animation line art degradation\n• Story cutscene freezes\n• Memory overhead with 25 DLCs",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100CA400E300000ULL,
        "Super Robot Wars V",
        "• Рассинхронизация звука во время масштабных атак линкора Ямато\n• Зависания видеороликов\n• Нагрузка памяти с DLC",
        "• Audio desync during massive Yamato battleship attacks\n• Video cutscene freezes\n• Memory pressure with DLCs",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010026800E304000ULL,
        "Super Robot Wars X",
        "• Искажение магических спецэффектов и спрайтов мехов при пересжатии ASTC\n• Зависания вступительных роликов",
        "• Magic effect and mecha sprite distortion under ASTC compression\n• Opening video playback freezes",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010063301BD50000ULL,
        "Super Robot Wars Y",
        "• Размытие спрайтов боевых сцен\n• Зависания видеовставок при аппаратном декодировании\n• Нехватка памяти при 10 DLC",
        "• Combat scene sprite blurring\n• Video playback hangs on hardware decoding\n• Memory pressure with 10 DLCs",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x0100C2801F22C000ULL,
        "Tales of Berseria - Remastered",
        "• Зависание аниме-роликов при аппаратном декодировании NVDEC\n• Статтеры при динамичных комбо-атаках 60 FPS\n• Утечки памяти при русификаторе (MOD - RUS)",
        "• Anime cutscene freezing on GPU NVDEC\n• 60 FPS combo attack shader stutters\n• Memory leaks when using LayeredFS Russian mod",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01005E701D168000ULL,
        "Tales of Graces f - Remastered",
        "• Зависание опенинга при аппаратном NVDEC\n• Микрофризы скоростной боевой системы Style Shift\n• Стабильность модов",
        "• Anime opening video freeze on GPU NVDEC\n• Style Shift battle system shader microstutters\n• Mod memory stability",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100A410169A4000ULL,
        "Tales of Symphonia - Remastered",
        "• Зависание вступительного видеоролика\n• Статтеры при входе в бой\n• Стабильность выделения памяти для русификатора",
        "• Opening cinematic video playback freezes\n• Battle encounter transition stutters\n• Memory stability for Russian translation mod",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01002C0008E52000ULL,
        "Tales of Vesperia [Definitive Edition]",
        "• Зависание сюжетных аниме-роликов при аппаратном NVDEC\n• Задержки компиляции шейдеров сел-шейдинга\n• Нехватка памяти при русификаторе",
        "• Story anime cutscene freezes on GPU NVDEC\n• Cel-shading shader compilation stutters\n• Memory pressure with Russian localization mod",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x0100F1101BB9E000ULL,
        "Tales of Xillia - Remastered",
        "• Зависание аниме-опенингов Джуда и Миллы при аппаратном NVDEC\n• Просадки FPS при двойных атаках\n• Вылеты памяти при двух модах (RUS + MODS)",
        "• Jude and Milla anime opening video hangs on GPU NVDEC\n• Dual artes combo framerate drops\n• Memory crash risk with dual mods",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01001AA022B66000ULL,
        "Tentacle Tango",
        "• Рассинхронизация ритмических нажатий с музыкальным битом\n• Дрожание кадров при асинхронном выводе",
        "• Rhythm timing desync with musical beat\n• Frame presentation jitter under async presentation",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100B51020B68000ULL,
        "Terminator 2D - No Fate",
        "• Рассинхронизация 2D-презентации кадров\n• Артефакты пиксельных спрайтов терминатора при сжатии текстур",
        "• 2D frame presentation desync\n• Terminator pixel sprite degradation under texture compression",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01005EC01E6A4000ULL,
        "The Art of Dave the Diver - Digital Artbook",
        "• Цветовые полосы и замыливание детальных страниц артбука при сжатии ASTC\n• Статтеры при пролистывании изображений высокого разрешения",
        "• Color banding and pixelation of high-res art pages with ASTC recompression\n• Stutter during high-resolution page navigation",
        "",
        "",
        {
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"}
        }
    },
    {
        0x010033100691A000ULL,
        "The Coma - Recut",
        "• Рассинхронизация 2D-презентации кадров в коридорах школы\n• Зависание вступительных видеороликов при аппаратном NVDEC",
        "• 2D presentation desync in school corridors\n• Intro video playback freezes on GPU NVDEC",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100B7C01169C000ULL,
        "The Coma 2 - Vicious Sisters",
        "• Сбои динамических теней фонаря и атмосферного освещения\n• Зависание видеороликов\n• Переполнение памяти при 8 DLC",
        "• Flashlight dynamic shadow and 2D horror atmospheric lighting desync\n• Video cutscene freezes\n• Memory overhead with 8 DLCs",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100A31020078000ULL,
        "The Coma 2B - Catacomb",
        "• Зависания катсцен при преследовании монстрами\n• Задержки воспроизведения видеороликов NVDEC\n• Размытие иллюстраций",
        "• Cutscene stalls during monster chase sequences\n• NVDEC video playback freezes\n• Illustration blurring under compression",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100919027DBE000ULL,
        "The Coma 3 - Bloodlines",
        "• Рассинхронизация кадров рисованного хоррора\n• Зависания видеосюжетов при аппаратном декодировании NVDEC",
        "• 2D illustrated horror frame pacing desync\n• Video sequence hangs on GPU NVDEC decoding",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100C69018E4A000ULL,
        "The Last Faith",
        "• Утечки памяти при исследовании готического города и переходе между локациями\n• Задержки компиляции шейдеров магии и добиваний\n• Вылеты на 4 ГБ DRAM",
        "• Memory fragmentation during Gothic interconnected map traversal\n• Weapon execution and magic particle shader stutters\n• Crash risk on 4GB DRAM",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x01007A2027548000ULL,
        "The Legend of Heroes - Trails in the Sky 2nd Chapter - Remake",
        "• Нагрузка 30 DLC на память и систему\n• Шейдерные задержки в пошаговых S-Craft боях\n• Зависание аниме-роликов при аппаратном NVDEC",
        "• Heavy system and memory footprint with 30 DLCs\n• S-Craft turn-based combat shader stutters\n• Anime cutscene freezes on GPU NVDEC",
        "",
        "",
        {
            {"System\\memory_layout_mode", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100FBE015910000ULL,
        "The Legend of Heroes - Trails to Azure",
        "• Размытие текстур высокого разрешения при пересжатии ASTC\n• Зависание вступительного аниме-ролика\n• Микрофризы в режиме ускорения (Turbo Mode)",
        "• High-resolution texture degradation under ASTC compression\n• Anime opening video freeze on GPU NVDEC\n• Turbo Mode frame pacing stutter",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x0100852026502000ULL,
        "Tyrant's Realm",
        "• Статтеры компиляции ретро-шейдеров PS1-стилистики\n• Искажение аффинных текстур при сжатии",
        "• PS1-style retro dark fantasy shader compilation stutters\n• Affine texture mapping distortion under compression",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100304027592000ULL,
        "Urban Jungle",
        "• Просадки кадровой частоты из-за прозрачности множества растений\n• Шейдерные задержки комнатного освещения",
        "• Framerate drops from heavy foliage transparency overdraw\n• Room lighting shader compilation stutters",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"}
        }
    },
    {
        0x010040502453E000ULL,
        "Vampire Crawlers",
        "• Критическое падение FPS при одновременной отрисовке десятков тысяч пуль и врагов\n• Прерывания доступа к памяти гостевого процесса",
        "• Severe framerate drops with tens of thousands of projectiles and enemy sprites\n• Guest memory abort risks under swarm loads",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        }
    },
    {
        0x0100217023F6C000ULL,
        "Well Dweller",
        "• Рассинхронизация 2D-кадров\n• Потеря резкости пиксель-арта колодца",
        "• 2D frame presentation pacing jitter\n• Well pixel art sharpness degradation",
        "✓ Рекомендуемые настройки STORM SOFT для стабильности и максимального FPS",
        "✓ STORM SOFT recommended profile settings for stability and maximum FPS",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x01001B90277BE000ULL,
        "Woodo",
        "• Потеря детализации фактуры дерева и резьбы при сжатии ASTC\n• Шейдерные задержки глубины резкости и объемного света\n• Нехватка памяти",
        "• Wood grain texture and carving degradation under ASTC compression\n• Depth of field and ambient lighting shader stutters\n• Memory pressure",
        "",
        "",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"System\\memory_layout_mode", "0"}
        }
    },
    {
        0x010013F009B88000ULL,
        "Xeno Crisis",
        "• Разрыв кадров (Tearing) и микрофризы аркадного шутера 60 FPS при асинхронном выводе\n• Сбои передачи буферов Genesis/Mega Drive",
        "• Screen tearing and frame pacing stutters in 60 FPS arcade shooter with async presentation\n• Mega Drive sprite buffer sync errors",
        "",
        "",
        {
            {"Renderer\\async_presentation", "false"},
            {"Renderer\\sync_memory_operations", "true"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"}
        }
    },
    {
        0x0100BAC01E57E000ULL,
        "Ys X - Nordics",
        "• Зависание вступительных аниме-роликов при аппаратном NVDEC\n• Просадки FPS при морских боях на корабле и спецэффектах воды\n• Вылеты по нехватке памяти при 13 DLC",
        "• Anime cutscene freezes on GPU NVDEC\n• Framerate drops during naval ship battles and water particle effects\n• Memory exhaustion with 13 DLCs",
        "✓ Расширенная память 6 ГБ (DRAM 6GB: устранение вылетов с 13 DLC)\n✓ Программный NVDEC CPU\n✓ Асинхронные шейдеры\n✓ Точность ГПу: Normal",
        "✓ 6GB Extended DRAM Memory (Prevents memory exhaustion crashes with 13 DLCs)\n✓ CPU NVDEC Video Decoding\n✓ Asynchronous Shaders\n✓ GPU Accuracy: Normal",
        {
            {"System\\memory_layout_mode", "1"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x010012F01B514000ULL, 0x0100F66017AA2000ULL, 0x0100999019A4E000ULL}
    },
    {
        0x010090C022978000ULL,
        "Kingdom Hearts - HD 1.5+2.5 ReMIX",
        "• Зависание видеороликов при аппаратном декодировании NVDEC\n• Артефакты текстур меню и интерфейса при сжатии ASTC",
        "• Video playback freezes under hardware NVDEC decoding\n• Menu and UI texture compression artifacts under ASTC recompression",
        "✓ Программный декодер видео (NVDEC CPU: устранение зависания роликов)\n✓ Сжатие ASTC: Отключено (максимальная четкость интерфейса)\n✓ Точность ГПУ: Обычная (Normal)\n✓ Реактивная очистка: Включено",
        "✓ CPU NVDEC Video Decoding (Fixes cutscene freezes)\n✓ ASTC Recompression: Uncompressed (Max UI clarity)\n✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Enabled",
        {
            {"Renderer\\nvdec_emulation", "1"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "true"}
        },
        {0x010067B01CD68000ULL, 0x0100826003994000ULL, 0x01001B301CD6A000ULL, 0x010045801CD6C000ULL}
    },
    {
        0x0100535012974000ULL,
        "Hades",
        "• Микрофризы в комнатах с интенсивными снарядами и частицами\n• Задержка синхронизации памяти при вызове богов",
        "• Micro-stutters during heavy projectile and particle chambers\n• Memory sync stalls during god boon animations",
        "✓ Быстрая память (Fastmem): Включено\n✓ Точность ГПУ: Обычная (Normal)\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ Fastmem: Enabled\n✓ GPU Accuracy: Normal\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"}
        },
        {0x0100735010B46000ULL}
    },
    {
        0x01007B600D338000ULL,
        "Trine 3: The Artifacts of Power",
        "• Падение FPS и зависание на черном экране при синхронных вызовах DMA fence на GPU Mali/TBDR\n• Нестабильный кадровый темп",
        "• Framerate drops and black screen hang from synchronous DMA fence stalls on Mali/TBDR GPUs\n• Unstable frame pacing",
        "✓ Точность ГПУ: Обычная (Normal / устранение зависаний очереди)\n✓ Реактивная очистка: Отключено (стабильный FPS на Mali)\n✓ Асинхронные шейдеры: Отключено (устранение черного экрана на Mali)\n✓ ASTC рекомпрессия: Без сжатия\n✓ Масштабирование: Bilinear\n✓ Разрешение: 1X (720p/1080p)",
        "✓ GPU Accuracy: Normal (Eliminates queue stalls)\n✓ Reactive Flushing: Disabled (Stable FPS on Mali)\n✓ Asynchronous Shaders: Disabled (Fixes black screen on Mali)\n✓ ASTC Recompression: Uncompressed\n✓ Scaling: Bilinear\n✓ Resolution: 1X (720p/1080p)",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\resolution_setup", "2"},
            {"Renderer\\scaling_filter", "1"}
        }
    },
    {
        0x01007E300B298000ULL,
        "Trine 4: The Nightmare Prince",
        "• Падение FPS и сбои адресации памяти на GPU Mali/TBDR\n• Черный экран при отложенном рендеринге",
        "• Framerate drops and memory mapping faults on Mali/TBDR GPUs\n• Black screen on deferred rendering passes",
        "✓ Точность ГПУ: Обычная (Normal)\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Отключено\n✓ ASTC рекомпрессия: Без сжатия\n✓ Масштабирование: Bilinear\n✓ Разрешение: 1X (720p/1080p)",
        "✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Disabled\n✓ ASTC Recompression: Uncompressed\n✓ Scaling: Bilinear\n✓ Resolution: 1X (720p/1080p)",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\resolution_setup", "2"},
            {"Renderer\\scaling_filter", "1"}
        }
    },
    {
        0x0100E66010978000ULL,
        "Little Orpheus",
        "• Размытие экрана, мерцание и пропажа геометрии головы/лица персонажей Unity\n• Зависание при выходе из игры",
        "• Blurry screen, flickering and missing head/facial submesh geometry in Unity\n• Application hang when quitting the game",
        "✓ Буфер индексов: Адаптивная выборка подсеток (устранение пропажи лиц)\n✓ Точность ГПУ: Обычная (Normal)\n✓ Реактивная очистка: Отключено\n✓ Асинхронные шейдеры: Включено\n✓ Масштабирование: Bilinear",
        "✓ Index Buffer: Adaptive submesh bounds (Restores facial meshes)\n✓ GPU Accuracy: Normal\n✓ Reactive Flushing: Disabled\n✓ Asynchronous Shaders: Enabled\n✓ Scaling: Bilinear",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\resolution_setup", "2"},
            {"Renderer\\scaling_filter", "1"}
        }
    },
    {
        0x01000C100DF0C000ULL,
        "Sniper Elite 4",
        "• Задержка ввода (input lag) и неравномерность кадрового темпа (frame pacing)\n• Просадки FPS при синхронной компиляции шейдеров Asura engine",
        "• Input lag and frame pacing stuttering with unoptimized GPU fences\n• Framerate drops during synchronous Asura engine shader compilation",
        "✓ Точность ГПУ: Обычная (Normal / устранение задержек выборки глубины)\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Eliminates depth readback latency)\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"}
        },
        {0x01005AD011C6E000ULL}
    },
    {
        0x0100C3800B8A0000ULL,
        "Power Rangers: Battle for the Grid",
        "• Микрофризы и задержка ввода при блокирующих синхронизациях GPU fence\n• Просадки FPS в динамичных боевых сценах Unity",
        "• Input lag and frame pacing stuttering with blocking GPU fence waits\n• Framerate drops in fast-paced Unity combat sequences",
        "✓ Точность ГПУ: Обычная (Normal / неблокирующая синхронизация)\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Non-blocking fence pacing)\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"}
        },
        {0x01005A300BFD6000ULL, 0x0100B1A00C9BC000ULL, 0x0100CD800D32E000ULL}
    },
    {
        0x010046E00B51E000ULL,
        "Jump Force",
        "• Просадки FPS и фризы при переключении персонажей и суперударах UE4\n• Искажение/сбои текстур на видеочипах Mali/PanVK без BC1/BC3 декомпрессии",
        "• Framerate drops during UE4 character switches and ultimate awakenings\n• Texture corruption on Mali/PanVK GPUs lacking BC1/BC3 hardware decompression",
        "✓ Точность ГПУ: Обычная (Normal / устранение задержек)\n✓ Сжатие ASTC: Отключено (Uncompressed / исправление текстур на Mali)\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Eliminates sync stalls)\n✓ ASTC Recompression: Uncompressed (Fixes textures on Mali)\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\astc_recompression", "0"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"}
        },
        {0x0100183010F12000ULL, 0x01007D300F6E2000ULL, 0x0100C4000FC1A000ULL, 0x010091600E5BC000ULL}
    },
    {
        0x0100412019772000ULL,
        "Naruto x Boruto Ultimate Ninja STORM CONNECTIONS",
        "• Просадки FPS и фризы при динамическом инстансинге вершин в дзюцу\n• Задержка ввода из-за синхронного сброса командных буферов",
        "• Pipeline stalls during dynamic vertex attribute instancing in jutsu animations\n• Input lag from synchronous command buffer flushes",
        "✓ Точность ГПУ: Обычная (Normal / оптимизированный конвейер)\n✓ Быстрое время ГПУ: Включено\n✓ Асинхронные шейдеры: Включено\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal (Optimized pipeline throughput)\n✓ Fast GPU Time: Enabled\n✓ Asynchronous Shaders: Enabled\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "true"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"},
            {"Cpu\\cpu_accuracy", "0"}
        },
        {0x010014C01A242000ULL, 0x0100999019806000ULL}
    },
    {
        0x040015100B514000ULL,
        "Super Mario Bros. Wonder (Switch 2 Edition)",
        "• Deadlock в таймингах заборов agl engine при экспериментальных опциях\n• Ошибки загрузки системных шрифтов Switch 2\n• Микрофризы на Pipe-Rock Plateau из-за сброса текстурного кэша",
        "• Deadlock in agl engine fence timing with experimental options\n• Switch 2 system font service loading errors\n• Pipe-Rock Plateau texture cache churn micro-stutters",
        "✓ Точность ГПУ: Обычная (Normal)\n✓ Тайминги GPU: Аппаратные 31.25 МГц\n✓ Обратные циклы барьеров: Включено\n✓ Ранний сброс заборов: Отключено\n✓ Динамический регулятор производительности: Отключено\n✓ Поддержка шрифтов Switch 2 IFontService",
        "✓ GPU Accuracy: Normal\n✓ Accurate Hardware GPU Timings (31.25 MHz)\n✓ Barrier Feedback Loops: Enabled\n✓ Early Release Fences: Disabled\n✓ Dynamic Performance Scaler: Disabled\n✓ Switch 2 IFontService Font Support",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\barrier_feedback_loops", "true"},
            {"Renderer\\drs_resolution_lock", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\smart_shader_throttle", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x040015100B5B4000ULL}
    },
    {
        0x04004D300C5AE000ULL,
        "Kirby and the Forgotten Land (Switch 2 Edition)",
        "• Искажение и четверть-экранный рендеринг при нативном апскейле DLSS\n• Просадки кадровой частоты анимаций дальних врагов\n• Артефакты шейдеров отражения",
        "• Quarter-screen distortion on native DLSS active region upscale\n• Distant enemy animation frame drops\n• Puddle reflection shader artifacts",
        "✓ Точность ГПУ: Высокая (High)\n✓ Апскейлинг DLSS: Декодирование активных областей\n✓ Реактивная очистка: Включено\n✓ Быстрое время ГПУ: Отключено\n✓ Анизотропная фильтрация: 16x",
        "✓ GPU Accuracy: High\n✓ DLSS Active Region Upscale Decoding\n✓ Reactive Flushing: Enabled\n✓ Fast GPU Time: Disabled\n✓ Anisotropic Filtering: 16x",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_reactive_flushing", "true"},
            {"Renderer\\anisotropy", "4"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x04004D300C5AF000ULL}
    },
    {
        0x04006000040C2000ULL,
        "Yoshi and the Mysterious Book",
        "• Горизонтальные полосы рендеринга из-за Ampere IMAD.HI 64-бит сложения\n• Утечки VRAM в геометрии Unreal Engine\n• Размытие текстур заднего плана",
        "• Horizontal resolve stripes from Ampere IMAD.HI 64-bit addend\n• Unreal Engine geometry VRAM leak\n• Background blur texture shimmering",
        "✓ Точность ГПУ: Высокая (High)\n✓ Ampere IMAD.HI 64-бит коррекция\n✓ Асинхронные шейдеры: Включено\n✓ Быстрое время ГПУ: Отключено\n✓ Память: 8GB DRAM",
        "✓ GPU Accuracy: High\n✓ Ampere IMAD.HI 64-bit Correction\n✓ Asynchronous Shaders: Enabled\n✓ Fast GPU Time: Disabled\n✓ Memory Layout: 8GB DRAM",
        {
            {"Renderer\\gpu_accuracy", "2"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x040089000C1B0000ULL}
    },
    {
        0x0400A3A01EE4C000ULL,
        "Pokémon Pokopia",
        "• Ошибки нехватки памяти при компиляции шейдерных блоков размером более 4 ГБ\n• Зависание при вводе имени персонажа в swkbd\n• Просадки кадров на мобильных ГПУ Adreno / Mali",
        "• OOM on compiling mega shader blobs exceeding 4 GiB\n• Character name entry softlock in software keyboard\n• Framerate drops on mobile Adreno / Mali GPUs",
        "✓ Точность ГПУ: Обычная (Normal)\n✓ Поддержка swkbd v16 (0x160012)\n✓ Оптимизация кэша шейдеров >4GB\n✓ Быстрая память (Fastmem): Включено\n✓ Игнорирование сбоев памяти: Включено",
        "✓ GPU Accuracy: Normal\n✓ Software Keyboard v16 (0x160012) Support\n✓ Mega Shader Blob >4GB Caching\n✓ Fastmem: Enabled\n✓ Ignore Memory Aborts: Enabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\use_asynchronous_shaders", "true"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {}
    },
    {
        0x040074A01BF12000ULL,
        "Donkey Kong Bananza",
        "• Черный экран и падение FPS в 0 после экрана загрузки (дедлок межканальных семафоров NVN2)\n• Пропуск нативных Ampere-шейдеров F2I\n• Конфликт точности ГПУ и заборов на Switch 2",
        "• Black screen and 0 FPS deadlock after loading screen (NVN2 cross-channel semaphore hang)\n• Skipped native Ampere F2I shaders\n• Switch 2 GPU accuracy and fence behavior conflict",
        "✓ Точность ГПУ: Обычная (Normal)\n✓ Поведение заборов ГПУ: По умолчанию (Default)\n✓ Поддержка QMD 3.0 RELEASE1 (0xD0), native ASTC tiling и VP9\n✓ Асинхронные шейдеры: Отключено (стабильная загрузка Switch 2)\n✓ Быстрое время ГПУ: Отключено",
        "✓ GPU Accuracy: Normal\n✓ GPU Fence Behavior: Default\n✓ Ampere QMD 3.0 RELEASE1 (0xD0), native ASTC tiling & VP9\n✓ Asynchronous Shaders: Disabled (stable Switch 2 level loading)\n✓ Fast GPU Time: Disabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\dynamic_performance_scaler", "false"},
            {"Renderer\\enable_frame_skipping", "false"},
            {"Renderer\\vram_budget_governor", "false"},
            {"Renderer\\use_reactive_flushing", "false"},
            {"Renderer\\use_asynchronous_shaders", "false"},
            {"Renderer\\use_disk_shader_cache", "true"},
            {"Renderer\\enable_gpu_buffer_readback", "true"},
            {"Renderer\\sync_memory_operations", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"Cpu\\cpu_accuracy", "1"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {}
    },
    {
        0x0400DB100C300000ULL,
        "The Duskbloods (Network Test)",
        "• Ошибка сбоя подключения к игровым серверам FromSoftware (*.fromsoftware-game.net)\n• Блокировка DNS-запросов к *.srv.nintendo.net\n• Тайм-ауты сетевых сокетов при рукопожатии NPLN",
        "• Connection error to FromSoftware game servers (*.fromsoftware-game.net)\n• DNS lookup block on *.srv.nintendo.net\n• Network socket timeouts on NPLN TLS handshake",
        "✓ Перенаправление DNS на локальный сервер playDB-CNT loopback\n✓ Разрешение запросов *.fromsoftware-game.net и *.srv.nintendo.net\n✓ Точность ГПУ: Обычная (Normal)\n✓ Быстрое время ГПУ: Отключено\n✓ Режим полета: Отключено",
        "✓ DNS Redirection to Local playDB-CNT Loopback Server\n✓ Whitelist for *.fromsoftware-game.net and *.srv.nintendo.net\n✓ GPU Accuracy: Normal\n✓ Fast GPU Time: Disabled\n✓ Airplane Mode: Disabled",
        {
            {"Renderer\\gpu_accuracy", "1"},
            {"Renderer\\gpu_fence_behavior", "0"},
            {"Renderer\\use_fast_gpu_time", "false"},
            {"Renderer\\early_release_fences", "false"},
            {"Renderer\\nvdec_emulation", "2"},
            {"System\\airplane_mode", "false"},
            {"Cpu\\cpuopt_fastmem", "true"},
            {"Cpu\\cpuopt_ignore_memory_aborts", "true"}
        },
        {0x0400DB100C300800ULL}
    }
};
}
