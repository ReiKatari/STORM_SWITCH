# STORM SWITCH 15.3.0

> 🚀 **Release 15.3.0** — *High-performance architectural update: Complete text rendering and FPS fix for Diablo II: Resurrected via non-blocking texture atlas reactive flushing & feedback loop barriers, 8–10x faster Settings opening, instantaneous cold start & multi-threaded shader cache loading, CRT-Royale post-processing shader state synchronization, Android Dimensity/Mali SDL3 OpenSL ES audio fix, Quick Settings filter switching, persistent game fix prompts, and Windows 11 / ROG Ally High-Performance APU/GPU boost.*
>
> 💡 **Релиз 15.3.0** — *Высокопроизводительное архитектурное обновление: Полное исправление рендеринга шрифтов/текста и стабильные 60 FPS в Diablo II: Resurrected через неблокирующий пречек реактивного сброса текстурного атласа и пайплайн-барьеры обратной связи, ускорение открытия меню настроек в 8–10 раз, мгновенный холодный старт и многопоточная загрузка дискового кэша шейдеров, синхронизация шейдеров CRT-Royale, исправление звука SDL3 OpenSL ES на процессорах MediaTek Dimensity / Mali в Android, быстрое переключение фильтров без мерцания, сохранение выбора диалога автофиксов и аппаратный буст GPU/APU на Windows 11 и ASUS ROG Ally.*

---

### 🌟 1. Ключевые исправления и подсистемы / Key Features & Fixes

- 🎮 **[Diablo II: Resurrected — Полное исправление шрифтов и 60 FPS / Font & Text Rendering Fix]**:
  - **Non-blocking `MustFlushRegion` Pre-check (`gpu_thread.cpp`)**: Устранены фризы и микростаттеры при обновлении глифов текста и динамических текстур интерфейса. Добавлен неблокирующий пречек необходимости сброса памяти без вызова дорогостоящих синхронных DMA-чтений (`SafeRead`) и блокировки графического конвейера гостевого процессора.
  - **Reactive Texture Atlas Flushing & Pipeline Feedback Loops (`vk_rasterizer.cpp`)**: Реализован механизм реактивного сброса кэша текстурного атласа шрифтов и автоматическая расстановка барьеров конвейера (`VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT` / `barrier_feedback_loops`) при циклической перезаписи поверхностей, что гарантирует 100% четкое отображение всех игровых шрифтов, диалогов и описаний предметов без артефактов и падения FPS.
  - **Встроенный Auto-Fix профиль (`game_fix_database.cpp` & `GameFixDatabase.kt`)**: Для тайтлов Diablo II: Resurrected (`0100726014352000`, `01002A801458A000`) по умолчанию активированы оптимальные флаги: `use_reactive_flushing=true`, `barrier_feedback_loops=true`, `use_fast_gpu_time=false`, `gpu_accuracy=0`, `dyna_state=0`.

- 🖥️ **[Синхронизация шейдеров CRT-Royale и меню графики / CRT-Royale State Synchronization]**:
  - **Единый источник истины (`post_processing_shader`)**: Устранена рассинхронизация состояния шейдеров постобработки между нижней панелью статуса (Footer Status Bar), каталогом RetroArch (`retroarch_shader_catalog.h`) и диалогом настроек графики (`ConfigureGraphics`).
  - **Нативная поддержка CRT-Royale**: Выбор шейдера CRT-Royale мгновенно применяется на лету во время игры и корректно отображается в селекторе нижней строки без сброса на дефолтные фильтры.

- ⚡ **[Экстремальное ускорение запуска и интерфейса / 8–10x Faster Settings, Cold Start & Shader Loading]**:
  - **Мгновенное открытие окна настроек (8–10x Faster Settings Dialog)**: Внедрен потокобезопасный кэш перечисления аудиоустройств `GetDeviceListForSink` в `sink_details.cpp` с TTL-инвалидацией, устраняющий зависание UI при опросе системных аудиомикшеров. Оптимизирован сброс настроек на диск — единичный пакетный вызов `WriteToIni` вместо каскадных операций записи.
  - **Мгновенный холодный старт (Instant Cold Start)**: Отложенная проверка прошивки NAND и предварительный прогрев кэша аудиоустройств в фоновом потоке сократили время запуска эмулятора до долей секунды.
  - **Многопоточная загрузка кэша шейдеров (Multi-threaded Shader Cache)**: Распараллеливание разбора дискового кэша шейдеров (`vk_pipeline_cache.cpp`) ускоряет загрузку игр на 30–50% на многоядерных процессорах и устраняет повторные избыточные перезагрузки драйверов ввода.

- 📱 **[Исправления для Android и портативных ПК / Android & Handheld Optimizations]**:
  - **Сохранение выбора «Больше не спрашивать» (`Issue #39`)**: В диалог рекомендуемых игровых фиксов (`GameFixDialogFragment.kt`) добавлен чекбокс «Не спрашивать снова для этой игры» с сохранением настроек в `SharedPreferences` (`game_fix_prompt_<title_id>`).
  - **Персистентность рабочих потоков пайплайна (`Issue #30`)**: Исправлен сброс значения «Pipeline Worker Threads» в настройках Android — выбранное количество потоков сохраняется и восстанавливается после перезапуска приложения.
  - **Поддержка чипсетов MediaTek Dimensity / Mali (`Issue #29`)**: Устранены искажения и щелчки звука на графических процессорах Mali и чипах Dimensity благодаря правильной инициализации буферов SDL3 OpenSL ES в `sdl3_sink.cpp`.
  - **Переключение фильтров без мерцания (`Issue #38`)**: Реализовано бесшовное применение фильтров постобработки в Android Quick Settings без мигания экрана и перезапуска графического контекста.
  - **Аппаратный буст APU/GPU на Windows 11 и ROG Ally (`Issue #22`)**: Отключение Windows 11 EcoQoS (Power Throttling) для основного процесса и экспорт директив `NvOptimusEnablement` / `AmdPowerXpressRequestHighPerformance` для задействования максимальной производительности дискретных GPU и мобильных гибридных APU.

---

### 🔐 2. Контрольные суммы и проверка целостности / Verification & Hashes

| Файл / Asset | Платформа / Назначение | Размер / Size | SHA-256 |
| :--- | :--- | :--- | :--- |
| **STORM_SWITCH_15.3.0_Windows11.zip** | Windows 11 x64 (AVX2 / Vulkan 1.4 / VC CRT) | 39.52 МБ | `28c5d21b50ed3928c92876b73fc0ef44a5f3c207666f3c6fa44fd89d28d94a64` |
| **STORM_SWITCH_15.3.0_Windows10.zip** | Windows 10 x64 Universal CRT Bundle | 39.52 МБ | `28c5d21b50ed3928c92876b73fc0ef44a5f3c207666f3c6fa44fd89d28d94a64` |
| **STORM_SWITCH_15.3.0.apk** | Android 10+ Universal (Snapdragon, Tensor, Dimensity) | 36.77 МБ | `9771e4b7d3a538e82c51cbe68f5b9754e003655f1a17ebc7e7866eea883e97dd` |
| **STORM_SWITCH_15.3.0_LEGACY.apk** | Android Adreno 6xx / 7xx Legacy | 33.25 МБ | `e2c1f59ed19fcd89276debc73be61f48c32fc714a70a549720b26ef1312f78f3` |
| **STORM_SWITCH_15.3.0_SDK27.apk** | Android 8.1+ (API 27+) Compatibility | 33.24 МБ | `20d69ca28dc61f7eba59cfcec14a9b85f80d826cf7c1f6f57cfd173ded8c354a` |

---

<details>
<summary><b>🔐 Прямые ссылки для загрузки (Direct Download Links)</b></summary>

- **[`STORM_SWITCH_15.3.0_Windows11.zip`](https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/STORM_SWITCH_15.3.0_Windows11.zip)** (39.52 МБ) — `sha256:28c5d21b50ed3928c92876b73fc0ef44a5f3c207666f3c6fa44fd89d28d94a64`
- **[`STORM_SWITCH_15.3.0_Windows10.zip`](https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/STORM_SWITCH_15.3.0_Windows10.zip)** (39.52 МБ) — `sha256:28c5d21b50ed3928c92876b73fc0ef44a5f3c207666f3c6fa44fd89d28d94a64`
- **[`STORM_SWITCH_15.3.0.apk`](https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/STORM_SWITCH_15.3.0.apk)** (36.77 МБ) — `sha256:9771e4b7d3a538e82c51cbe68f5b9754e003655f1a17ebc7e7866eea883e97dd`
- **[`STORM_SWITCH_15.3.0_LEGACY.apk`](https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/STORM_SWITCH_15.3.0_LEGACY.apk)** (33.25 МБ) — `sha256:e2c1f59ed19fcd89276debc73be61f48c32fc714a70a549720b26ef1312f78f3`
- **[`STORM_SWITCH_15.3.0_SDK27.apk`](https://stormgamesworld.ru/uploads/programs/STORM_SWITCH/STORM_SWITCH_15.3.0_SDK27.apk)** (33.24 МБ) — `sha256:20d69ca28dc61f7eba59cfcec14a9b85f80d826cf7c1f6f57cfd173ded8c354a`

</details>

---

<details>
<summary><b>📖 Руководство по установке и обновлению / Installation & Update Guide</b></summary>

1. **Автоматически / Automatic Update**: STORM SWITCH уведомит о доступности версии 15.3.0 и выполнит обновление в один клик.
2. **Вручную на Windows / Manual on Windows**: Распакуйте `STORM_SWITCH_15.3.0_Windows11.zip` (или `Windows10.zip`) в каталог эмулятора с заменой файлов. Пользовательские ключи, прошивка и сохранения в папке `user/` сохраняются неизменными.
3. **Вручную на Android / Manual on Android**: Установите `STORM_SWITCH_15.3.0.apk` (или подходящий билд `_LEGACY.apk` / `_SDK27.apk`) поверх существующей версии приложения.

</details>
