# STORM SWITCH

## **О проекте**
STORM SWITCH — форк эмулятора гибридной игровой консоли Nintendo Switch для платформ Android и Windows PC. Проект ориентирован на максимальную производительность, оптимизацию JIT/NCE компилятора, стабильность видеодрайверов Vulkan, поддержку сжатых форматов (NSZ/NCZ/XCZ на лету), расширенную базу авто-исправлений GameFix Database и готический пользовательский интерфейс с глубокой кастомизацией оверлеев и телеметрии.

## **Происхождение и форки**
STORM SWITCH является высокопроизводительным форком эмулятора Nintendo Switch **Eden**, на базе **Citron**, **Yuzu** и наработок **Lemon-Project** с кастомными Vulkan-пайплайнами, барьерами синхронизации для исправления растительности и теней (BotW/TotK), потоковой декомпрессией NSZ/XCZ через Zstandard с 24 ГБ кольцевым LRU-кэшем, расширенной базой авто-профилей GameFix Database и оверлеями нагрузки в реальном времени.

## **Технологический стек**
- **Языки программирования**: C++20, C, Kotlin, Java, CMake
- **Графический стек**: Vulkan 1.3, Turnip/Adreno Custom Extensions, Spirv-Cross, Shader Recompiler
- **Компиляция кода**: Dynarmic (ARM64 JIT Recompiler), Native Code Execution (NCE) для Android ARM64
- **Аудио**: Cubeb, SDL3 Audio, OpenSL ES
- **Декомпрессия и форматы**: Zstandard (блочное и solid потоковое декодирование NSZ/NCZ/XCZ с LRU-кэшем), hactool, liblz4
- **Пользовательский интерфейс**: Qt6 (Windows), Jetpack Compose / Android Native UI (Android)

## **Ключевые возможности**
- **Прямая поддержка сжатых форматов (NSZ/NCZ/XCZ)**: Потоковая распаковка Zstandard блочных дампов на лету без распаковки на диск и 24 ГБ кольцевой LRU-кэш для сплошных архивов.
- **Глубокая база авто-профилей (GameFix Database)**: Автоматическая настройка параметров памяти (8GB DRAM Layout), точности CPU (Unsafe JIT/Fastmem), ASTC-текстур и графических твиков для сотен коммерческих игр и homebrew-портов (Streets of Rage 4 на ПК и Android, Assassin's Creed: The Rebel Collection, GTA V, Half-Life, Doom 3, S.T.A.L.K.E.R. и др.).
- **Vulkan Stability and Compute Barriers**: Устранение сбоев `VK_ERROR_DEVICE_LOST`, барьеры конвейера памяти для compute dispatch (исправление теней и травы в Zelda TotK), поддержка расширений `VK_EXT_extended_dynamic_state3` и аппаратного декомпрессора ASTC.
- **Оптимизированные быстрые сохранения (Savestates)**: Разреженное сжатие страниц памяти (Sparse Page Encoding), уменьшающее вес файла сохранения в ~3 раза и ускоряющее запись и чтение, с корректным пробуждением спящих потоков гостевого ядра.
- **Встроенный менеджер дополнений и модов**: Управление DLC, обновлениями, интеграция с GameBanana Mods и встроенный браузер Amiibo со случайной генерацией UID.
- **Кроссплатформенная синхронизация сохранений (STORM SAVE SYNC)**: Прямая передача и синхронизация сохранений Switch между ПК и смартфонами Android по защищённому коду подключения с трёхсторонним разрешением конфликтов версий.
- **Готический интерфейс и неоновая телеметрия**: Оверлей нагрузки с отображением реального разрешения рендера, FPS, frametime и температуры GPU/CPU.

## **Поддерживаемые платформы и эмуляторы**
- **Операционные системы**: Android 10+ (ARM64-v8a, Snapdragon / Dimensity / Exynos), Windows 10, Windows 11 (x64, DirectX 12 / Vulkan)
- **Поддерживаемая консоль**: Nintendo Switch (Horizon OS)
- **Форматы файлов**: NSP, NSZ, XCI, XCZ, NRO, NSO
- **Поддерживаемые эмуляторы и форки**: STORM SWITCH, Eden, Lemon-Project, Citron, Yuzu, Suyu, Sudachi, Torzu, Ryujinx

## **Установка и запуск**
1. Перейдите в раздел **Releases** репозитория на GitHub.
2. Для Android: скачайте и установите подходящий APK-файл: `STORM_SWITCH.apk` (Android 10+ / Snapdragon / Dimensity / Exynos), `STORM_SWITCH_LEGACY.apk` (Adreno 6xx/7xx с кастомными драйверами Turnip) или `STORM_SWITCH_SDK27.apk` (Android 8.1+ / старые устройства).
3. Для Windows: скачайте и распакуйте портативный архив для вашей версии ОС: `STORM_SWITCH_Windows11.zip` (Windows 11 x64) или `STORM_SWITCH_Windows10.zip` (Windows 10 x64), после чего запустите `STORM_SWITCH.exe`.
4. Установите системные ключи (`prod.keys`) и актуальную прошивку Nintendo Switch (Firmware) через меню настроек эмулятора.
