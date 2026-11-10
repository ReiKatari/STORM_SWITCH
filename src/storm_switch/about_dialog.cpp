// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>
#include <fmt/ranges.h>
#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "storm_switch/about_dialog.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()} {
    static const std::string build_id = std::string{Common::g_build_id};
    static const std::string yuzu_build =
        fmt::format("{} {}", std::string{Common::g_build_name}, std::string{Common::g_build_version});

    const auto override_build =
        fmt::format(fmt::runtime(std::string(Common::g_title_bar_format_idle)), build_id);
    const auto yuzu_build_version = override_build.empty() ? yuzu_build : override_build;

    ui->setupUi(this);
    resize(840, 580);
    setMinimumSize(740, 500);

    const QIcon app_logo = QApplication::windowIcon();
    if (!app_logo.isNull()) {
        ui->labelLogo->setPixmap(app_logo.pixmap(176, 176));
    }

    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    setStyleSheet(StormTheme::GetDialogStyle(pal) + QLatin1Char('\n') +
                  StormDesign::BuildDialogStylesheet(pal));

    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    const bool is_ru = (lang.rfind("ru", 0) == 0 || lang.rfind("be", 0) == 0 || lang.rfind("uk", 0) == 0);

    setWindowTitle(StormLangStorm11(
        QStringLiteral("О программе — STORM SWITCH"),
        QStringLiteral("About — STORM SWITCH"),
        QStringLiteral("Über — STORM SWITCH"),
        QStringLiteral("À propos — STORM SWITCH"),
        QStringLiteral("关于 — STORM SWITCH"),
        QStringLiteral("정보 — STORM SWITCH"),
        QStringLiteral("バージョン情報 — STORM SWITCH"),
        QStringLiteral("Acerca de — STORM SWITCH"),
        QStringLiteral("Informazioni — STORM SWITCH"),
        QStringLiteral("Sobre — STORM SWITCH"),
        QStringLiteral("के बारे में — STORM SWITCH")
    ));

    ui->labelYuzu->setText(QStringLiteral(
        "<html><head/><body><p><span style='font-size:24pt; font-weight:800; color:%1;'>STORM SWITCH</span></p></body></html>")
        .arg(pal.accent));

    ui->labelBuildInfo->setText(
        ui->labelBuildInfo->text().arg(QString::fromStdString(yuzu_build_version),
                                       QString::fromUtf8(Common::g_build_date).left(10)));

    const QString about_body = StormLangStorm11(
        QStringLiteral("STORM SWITCH — высокопроизводительный мультиплатформенный эмулятор Nintendo Switch и Nintendo Switch 2 на базе Eden, созданный для максимальной производительности, стабильности конвейера Vulkan, потокового запуска сжатых архивов (NSZ/NCZ/XCZ и .nup), облачной синхронизации сохранений STORM SAVE SYNC и глубокой кастомизации.<br/><br/>"
                       "STORM SWITCH распространяется под открытой лицензией GPLv3.0+ и включает расширенную базу автопрофилей GameFix Database, поддержку модов GameBanana, менеджер читов и Amiibo.<br/><br/>"
                       "Данное программное обеспечение предназначено исключительно для запуска легально полученных копий игр."),
        QStringLiteral("STORM SWITCH is an advanced high-performance multi-platform Nintendo Switch and Nintendo Switch 2 emulator built atop Eden for maximum speed, Vulkan pipeline stability, on-the-fly compressed container streaming (NSZ/NCZ/XCZ and .nup), STORM SAVE SYNC cloud synchronization, and deep customization.<br/><br/>"
                       "STORM SWITCH is distributed under the open-source GPLv3.0+ license and integrates an expanded GameFix Database, GameBanana mods, Cheats Manager, and Amiibo Manager.<br/><br/>"
                       "This software is intended exclusively for running legally obtained copies of games."),
        QStringLiteral("STORM SWITCH ist ein fortschrittlicher Hochleistungs-Emulator für Nintendo Switch und Nintendo Switch 2 auf Basis von Eden, entwickelt für maximale Performance, Vulkan-Stabilität und tiefe Anpassbarkeit.<br/><br/>"
                       "STORM SWITCH wird unter der GPLv3.0+-Lizenz veröffentlicht und dient ausschließlich dem Ausführen legal erworbener Spielkopien."),
        QStringLiteral("STORM SWITCH est un émulateur haute performance pour Nintendo Switch et Nintendo Switch 2 basé sur Eden, conçu pour des performances maximales, la stabilité Vulkan et une personnalisation poussée.<br/><br/>"
                       "Distribué sous licence GPLv3.0+, ce logiciel est destiné exclusivement à l'exécution de copies de jeux légalement acquises."),
        QStringLiteral("STORM SWITCH 是基于 Eden 打造的高性能 Nintendo Switch 与 Nintendo Switch 2 模拟器，专注于极致性能、Vulkan 稳定性、压缩格式直读与云存档同步。<br/><br/>"
                       "本软件基于 GPLv3.0+ 许可证发布，仅可用于运行合法获得的游戏副本。"),
        QStringLiteral("STORM SWITCH는 최고의 성능과 Vulkan 안정성, 압축 포맷 스트리밍 및 클라우드 세이브 동기화를 위해 제작된 Eden 기반의 고성능 Nintendo Switch / Switch 2 에뮬레이터입니다.<br/><br/>"
                       "GPLv3.0+ 라이선스로 배포되며 합법적으로 취득한 게임 사본 실행 전용입니다."),
        QStringLiteral("STORM SWITCH は、最高性能・Vulkan 安定性・圧縮アーカイブ直接再生・クラウドセーブ同期を追求して設計された Eden ベースの Nintendo Switch / Switch 2 エミュレータです。<br/><br/>"
                       "GPLv3.0+ ライセンスの下で配布され、合法的に取得したゲームバックアップの実行のみを目的としています。"),
        QStringLiteral("STORM SWITCH es un emulador de alto rendimiento para Nintendo Switch y Nintendo Switch 2 basado en Eden, diseñado para ofrecer máxima velocidad, estabilidad Vulkan y sincronización en la nube.<br/><br/>"
                       "Distribuido bajo licencia GPLv3.0+, destinado exclusivamente a copias legales de juegos."),
        QStringLiteral("STORM SWITCH è un emulatore ad alte prestazioni per Nintendo Switch e Nintendo Switch 2 basato su Eden, progettato per massima velocità, stabilità Vulkan e sincronizzazione cloud.<br/><br/>"
                       "Distribuito con licenza GPLv3.0+, destinato esclusivamente a copie di giochi legalmente ottenute."),
        QStringLiteral("STORM SWITCH é um emulador de alto desempenho para Nintendo Switch e Nintendo Switch 2 baseado no Eden, desenvolvido para máxima velocidade, estabilidade Vulkan e sincronização na nuvem.<br/><br/>"
                       "Distribuído sob licença GPLv3.0+, destinado exclusivamente a cópias legais de jogos."),
        QStringLiteral("STORM SWITCH ईडन पर आधारित एक उच्च-प्रदर्शन Nintendo Switch और Nintendo Switch 2 एमुलेटर है, जो अधिकतम गति और Vulkan स्थिरता के लिए बनाया गया है।<br/><br/>"
                       "GPLv3.0+ लाइसेंस के अंतर्गत वितरित, केवल कानूनी रूप से प्राप्त गेम प्रतियों के लिए।")
    );

    ui->labelAbout->setText(QStringLiteral(
        "<html><head/><body><p style='line-height:1.4;'><span style='font-size:10pt; color:%1;'>%2</span></p></body></html>")
        .arg(pal.textPrimary, about_body));

    const QString link_source = StormLangStorm11(
        QStringLiteral("Исходный код GitHub"), QStringLiteral("GitHub Source"), QStringLiteral("GitHub-Quellcode"),
        QStringLiteral("Code source GitHub"), QStringLiteral("GitHub 源代码"), QStringLiteral("GitHub 소스 코드"),
        QStringLiteral("GitHub ソースコード"), QStringLiteral("Código en GitHub"), QStringLiteral("Codice GitHub"),
        QStringLiteral("Código no GitHub"), QStringLiteral("GitHub स्रोत कोड"));

    const QString link_license = StormLangStorm11(
        QStringLiteral("Лицензия (GPLv3)"), QStringLiteral("License (GPLv3)"), QStringLiteral("Lizenz (GPLv3)"),
        QStringLiteral("Licence (GPLv3)"), QStringLiteral("许可证 (GPLv3)"), QStringLiteral("라이선스 (GPLv3)"),
        QStringLiteral("ライセンス (GPLv3)"), QStringLiteral("Licencia (GPLv3)"), QStringLiteral("Licenza (GPLv3)"),
        QStringLiteral("Licença (GPLv3)"), QStringLiteral("लाइसेंस (GPLv3)"));

    ui->labelLinks->setText(QStringLiteral(
        "<html><head/><body><p>"
        "<a href='https://stormgamesworld.ru/programs/STORM_SWITCH'><span style='text-decoration: underline; color:%1;'>STORM GAMES WORLD</span></a> | "
        "<a href='https://github.com/ReiKatari/STORM_SWITCH'><span style='text-decoration: underline; color:%1;'>%2</span></a> | "
        "<a href='https://eden-emulator.github.io/'><span style='text-decoration: underline; color:%1;'>Eden Project</span></a> | "
        "<a href='https://git.eden-emu.dev/eden-emu/eden/src/branch/master/LICENSE.txt'><span style='text-decoration: underline; color:%1;'>%3</span></a>"
        "</p></body></html>")
        .arg(pal.accent, link_source, link_license));

    const QString liability_text = StormLangStorm11(
        QStringLiteral("&quot;Nintendo Switch&quot; является зарегистрированным товарным знаком Nintendo. STORM SWITCH не связан с Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; is a registered trademark of Nintendo. STORM SWITCH is not affiliated with Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; ist eine eingetragene Marke von Nintendo. STORM SWITCH steht in keiner Verbindung zu Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; est une marque déposée de Nintendo. STORM SWITCH n'est pas affilié à Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; 是任天堂的注册商标。STORM SWITCH 与任天堂无任何关联。"),
        QStringLiteral("&quot;Nintendo Switch&quot;는 Nintendo의 등록 상표입니다. STORM SWITCH는 Nintendo와 관련이 없습니다."),
        QStringLiteral("&quot;Nintendo Switch&quot; は任天堂の登録商標です。STORM SWITCH は任天堂とは一切関係ありません。"),
        QStringLiteral("&quot;Nintendo Switch&quot; es una marca registrada de Nintendo. STORM SWITCH no está afiliado a Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; è un marchio registrato di Nintendo. STORM SWITCH non è affiliato a Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; é uma marca registrada da Nintendo. STORM SWITCH não é afiliado à Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; निन्टेंडो का पंजीकृत ट्रेडमार्क है। STORM SWITCH निन्टेंडो से संबद्ध नहीं है।")
    );

    ui->labelLiability->setText(QStringLiteral(
        "<html><head/><body><p><span style='font-size:8pt; color:%1;'>%2</span></p></body></html>")
        .arg(pal.textSecondary, liability_text));

    // Build tabbed About + Expanded Credits view synchronized 1:1 with README.md
    auto* tab_widget = new QTabWidget(this);
    auto* overview_page = new QWidget(tab_widget);
    auto* overview_layout = new QVBoxLayout(overview_page);
    overview_layout->setContentsMargins(10, 12, 10, 10);
    overview_layout->setSpacing(10);

    ui->verticalLayout->removeWidget(ui->labelAbout);
    ui->verticalLayout->removeItem(ui->verticalSpacer_2);
    ui->verticalLayout->removeWidget(ui->labelLinks);
    ui->verticalLayout->removeWidget(ui->labelLiability);

    overview_layout->addWidget(ui->labelAbout);
    overview_layout->addStretch(1);
    overview_layout->addWidget(ui->labelLinks);
    overview_layout->addWidget(ui->labelLiability);

    auto* credits_browser = new QTextBrowser(tab_widget);
    credits_browser->setOpenExternalLinks(true);
    credits_browser->setFrameShape(QFrame::NoFrame);

    const QString credits_ru = QStringLiteral(
        "<div style='font-size:9.5pt; line-height:1.45; color:%1;'>"
        "<p>Проект <b>STORM SWITCH</b> создан благодаря многолетнему труду, открытым исследованиям и инженерному мастерству разработчиков мирового сообщества эмуляции и свободного ПО. Выражаем глубокую признательность всем авторам, командам и участникам:</p>"
        "<h4 style='color:%2; margin-bottom:4px;'>1. Базовые эмуляторы и фундамент архитектуры</h4><ul>"
        "<li><b>Команда Eden Emulator (Camille LaVey, Crueter и все контрибьюторы Eden)</b> — за открытую кодовую базу, независимое развитие ядра, продвинутые оптимизации компилятора, графического стека и системных сервисов.</li>"
        "<li><b>Команда Yuzu и Citra (Bunnei, Lioncache, Subv, BreadFish64, Blinkhawk, Morph, Rodrigo, MerryMage, byte[], FearlessTobi, ogniK, german77, toastUnlimited и контрибьюторы)</b> — за создание фундаментальной открытой архитектуры эмуляции Nintendo Switch и Nintendo 3DS.</li>"
        "<li><b>Команда Ryujinx и Ryubing (gdkchan, Ac_K, Mary, riperiperi, Thog, LotP1, GreemDev и контрибьюторы)</b> — за эталонную точность эмуляции, фундаментальные исследования Horizon OS, сервисов HLE, аудиоподсистемы и GPU Maxwell.</li>"
        "<li><b>jarrodnorwell и проект Sudachi</b> — за ключевые исправления совместимости современных игр, апплетов, сервисов HLE и прошивок.</li>"
        "<li><b>Команда Citron Emulator (zephyron, BignBooty, EmuGamer)</b> — за развитие мобильной и настольной платформ, управление шейдерами и стабильность на Android.</li>"
        "<li><b>Ghael-V и команда Lemon-Project</b> — за исследования потоковой декомпрессии NSZ и XCZ, барьеров синхронизации Vulkan и разреженных быстрых сохранений.</li>"
        "<li><b>Команды Suyu, Torzu, Uzuy и Sumi</b> — за сохранение, аудит и развитие независимых открытых ветвей эмуляции.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>2. Исследования архитектуры Nintendo Switch 2 и Next-Gen проектов</h4><ul>"
        "<li><b>Команда Eglantine</b> — за пионерские исследования низкоуровневых механизмов, системных вызовов и архитектуры консоли нового поколения.</li>"
        "<li><b>MythosOfs и проект Orchid</b> — за глубокие изыскания в области архитектуры Tegra T239 (Ampere), трансляции шейдерного микрокода, HLE-сервисов и графических интерфейсов следующего поколения.</li>"
        "<li><b>zortos293 и проект Lichii</b> — за инновационные исследования структур ядра, сервиса системных шрифтов pl:u, метаданных NACP и формата пакетов .nup.</li>"
        "<li><b>moceure (BananaNative &amp; eden-nx2-android)</b> — за выдающиеся достижения в адаптации Native Code Execution (NCE), виртуальной памяти и мобильной эмуляции NX2 на Android.</li>"
        "<li><b>sikoGD и проект playDB-CNT</b> — за разработку инфраструктуры сетевого протокола NPLN и stand-in серверов локального мультиплеера.</li>"
        "<li><b>Команда Pound (pound-emu)</b> — за открытые исследования, декомпиляцию и анализ алгоритмов графического конвейера, активных областей DLSS и тайловой памяти нового поколения.</li>"
        "<li><b>0xNikilite и проект Oboromi</b> — за неоценимый вклад в реверс-инжиниринг и анализ подсистем операционной системы NX2.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>3. Графические драйверы, компиляторы ЦП и низкоуровневое ядро</h4><ul>"
        "<li><b>Rob Clark, Danylo Piliaiev, Connor Abbott, Samuel Iglesias Gonsálvez и разработчики Mesa 3D / Turnip / Freedreno</b> — за открытый мобильный драйвер Vulkan под Qualcomm Adreno.</li>"
        "<li><b>Alyssa Rosenzweig, Boris Brezillon, Erik Faye-Lund, команда Collabora и разработчики PanVK / Panfrost</b> — за открытые драйверы и компиляторы шейдеров для архитектур ARM Mali (Bifrost, Valhall, Immortals).</li>"
        "<li><b>bylaws (Billy Laws)</b> — за создание библиотеки libadrenotools и динамической подгрузки пользовательских GPU-драйверов на Android.</li>"
        "<li><b>K11MCH1, MrPurple666, Weab-chan, whitebelyash и мейнтейнеры сборок Turnip / Qualcomm</b> — за регулярную сборку, патчинг и тестирование мобильных видеодрайверов.</li>"
        "<li><b>MerryMage (merryhime) и контрибьюторы Dynarmic</b> — за высокоскоростной и эталонный динамический JIT-компилятор ARM64 и ARM32.</li>"
        "<li><b>Команда Skyline и Strato (Mark, Billy Laws, Lynx и контрибьюторы)</b> — за фундаментальные наработки по прямому исполнению гостевого кода ARM64 NCE на Android.</li>"
        "<li><b>Mitsunari Shigeo (herumi / xbyak) и авторы Oak</b> — за высокопроизводительные генераторы машинного кода x86_64 и AArch64.</li>"
        "<li><b>SciresM, TuxSH, fincs, hexkyz, mtheall и команды Atmosphère &amp; Switchbrew</b> — за документацию аппаратной платформы, спецификации ядра Mesosphere, криптографии NCA и инструментарий hactool / libnx.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>4. Форматы данных, графические технологии, аудио и системные библиотеки</h4><ul>"
        "<li><b>nicoboss и контрибьюторы проекта NSZ</b> — за спецификацию и алгоритмы потоково-совместимого сжатия контейнеров NSZ / NCZ / XCZ.</li>"
        "<li><b>Yann Collet и команды Zstandard (Meta) &amp; LZ4</b> — за эталонные сверхбыстрые алгоритмы сжатия и декомпрессии данных.</li>"
        "<li><b>Команда Khronos Group</b> — за спецификации стандарта Vulkan, SPIR-V, Vulkan-Headers, glslang и SPIRV-Cross.</li>"
        "<li><b>Команда AMD GPUOpen (FidelityFX &amp; Vulkan Memory Allocator)</b> — за технологии масштабирования FidelityFX Super Resolution (FSR) / CAS и библиотеку управления видеопамятью VMA.</li>"
        "<li><b>Команда Sirit (Rodrigo и контрибьюторы)</b> — за быструю библиотеку динамической генерации модулей SPIR-V.</li>"
        "<li><b>The Qt Company и контрибьюторы проекта Qt</b> — за кроссплатформенный графический фреймворк настольного интерфейса.</li>"
        "<li><b>Sam Lantinga и команда Simple DirectMedia Layer (SDL), Mozilla (Cubeb) и Google Oboe / OpenAL</b> — за кроссплатформенный стек контроллеров, датчиков движения, HD-Rumble и аудио с низкой задержкой.</li>"
        "<li><b>Команды FFmpeg, VideoLAN (dav1d) и Xiph (Opus)</b> — за декодирование мультимедийных потоков H.264 / VP9 / AV1 и многоканального звука.</li>"
        "<li><b>Авторы библиотек Boost, fmt (Victor Zverovich), nlohmann/json, OpenSSL, MbedTLS, libusb, enet и Inno Setup (Jordan Russell)</b> — за надёжный фундамент системного программирования, сети и установщика.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>5. Модификации, базы метаданных, экосистема и сообщество</h4><ul>"
        "<li><b>Платформа GameBanana и авторы модификаций сообщества</b> — за открытый API и крупнейший каталог пользовательских модов и графических улучшений.</li>"
        "<li><b>blawar и контрибьюторы TitleDB, а также N3evin и проект AmiiboAPI</b> — за открытые базы идентификаторов игр, обложек, обновлений и метаданных фигурок Amiibo.</li>"
        "<li><b>Авторы технических патчей 60 FPS, Dynamic FPS и Ultrawide (ChanseyIsTheBest, StevensND, Fl4sh, KeatonTheBot, boy181 и др.)</b> — за развитие патчей производительности и совместимости.</li>"
        "<li><b>Команда STORM TEAM &amp; портал STORM GAMES WORLD</b> — за инженерное развитие эмулятора, облако STORM SAVE SYNC, базу автопрофилей GameFix Database, дизайн STORM AURORA FLUID и инфраструктуру экосистемы.</li>"
        "<li><b>Участники сообщества, авторы баг-репортов и тестировщики с 4PDA, GitHub, Discord и профильных форумов</b> — за тестирование сотен игр, подробные отчёты, логи и неоценимую обратную связь.</li></ul>"
        "</div>").arg(pal.textPrimary, pal.accent);

    const QString credits_en = QStringLiteral(
        "<div style='font-size:9.5pt; line-height:1.45; color:%1;'>"
        "<p><b>STORM SWITCH</b> is made possible by the dedication, open research, and engineering excellence of the global emulation and open-source software community. We extend our sincere gratitude to all teams, researchers, and contributors:</p>"
        "<h4 style='color:%2; margin-bottom:4px;'>1. Core Emulators &amp; Architectural Foundation</h4><ul>"
        "<li><b>Eden Emulator Team (Camille LaVey, Crueter, and all Eden contributors)</b> — open-source foundation, independent core evolution, compiler optimizations, graphics stack, and HLE services.</li>"
        "<li><b>Yuzu &amp; Citra Teams (Bunnei, Lioncache, Subv, BreadFish64, Blinkhawk, Morph, Rodrigo, MerryMage, byte[], FearlessTobi, ogniK, german77, toastUnlimited, and contributors)</b> — pioneering open-source Nintendo Switch and 3DS emulation architecture.</li>"
        "<li><b>Ryujinx &amp; Ryubing Teams (gdkchan, Ac_K, Mary, riperiperi, Thog, LotP1, GreemDev, and contributors)</b> — reference accuracy, deep Horizon OS research, HLE services, audio subsystem, and Maxwell GPU documentation.</li>"
        "<li><b>jarrodnorwell &amp; Sudachi</b> — modern title compatibility fixes, system applets, HLE services, and firmware support.</li>"
        "<li><b>Citron Emulator Team (zephyron, BignBooty, EmuGamer)</b> — Android and desktop innovations, shader pipeline management, and mobile stability.</li>"
        "<li><b>Ghael-V &amp; Lemon-Project</b> — streaming NSZ/XCZ decompression research, Vulkan synchronization barriers, and sparse savestate optimizations.</li>"
        "<li><b>Suyu, Torzu, Uzuy &amp; Sumi Teams</b> — preserving, auditing, and advancing independent community emulation branches.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>2. Nintendo Switch 2 &amp; Next-Gen Architecture Research</h4><ul>"
        "<li><b>Eglantine Team</b> — pioneering research into next-generation low-level mechanisms, syscalls, and console architecture.</li>"
        "<li><b>MythosOfs &amp; Orchid</b> — deep research into Tegra T239 (Ampere), shader microcode translation, HLE services, and next-gen graphics interfaces.</li>"
        "<li><b>zortos293 &amp; Lichii</b> — innovative kernel structure research, pl:u system font service, NACP metadata, and .nup package format support.</li>"
        "<li><b>moceure (BananaNative &amp; eden-nx2-android)</b> — outstanding achievements adapting Native Code Execution (NCE), virtual memory, and mobile NX2 emulation on Android.</li>"
        "<li><b>sikoGD &amp; playDB-CNT</b> — NPLN network protocol infrastructure and local multiplayer stand-in servers.</li>"
        "<li><b>Pound Team (pound-emu)</b> — open graphics pipeline analysis, DLSS active regions, and next-gen sparse tile mapping research.</li>"
        "<li><b>0xNikilite &amp; Oboromi</b> — reverse engineering and analysis of NX2 operating system subsystems.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>3. GPU Drivers, CPU Compilers &amp; Low-Level Kernel</h4><ul>"
        "<li><b>Rob Clark, Danylo Piliaiev, Connor Abbott, Samuel Iglesias Gonsálvez &amp; Mesa 3D / Turnip / Freedreno Developers</b> — open-source Vulkan drivers for Qualcomm Adreno GPUs.</li>"
        "<li><b>Alyssa Rosenzweig, Boris Brezillon, Erik Faye-Lund, Collabora &amp; PanVK / Panfrost Developers</b> — open-source drivers and shader compilers for ARM Mali GPUs (Bifrost, Valhall, Immortals).</li>"
        "<li><b>bylaws (Billy Laws)</b> — libadrenotools custom GPU driver loading on Android without root access.</li>"
        "<li><b>K11MCH1, MrPurple666, Weab-chan, whitebelyash &amp; Turnip / Qualcomm Driver Maintainers</b> — building, patching, profiling, and testing custom mobile GPU drivers.</li>"
        "<li><b>MerryMage (merryhime) &amp; Dynarmic Contributors</b> — blazing-fast ARM64 and ARM32 dynamic JIT recompiler.</li>"
        "<li><b>Skyline &amp; Strato Teams (Mark, Billy Laws, Lynx, and contributors)</b> — foundational ARM64 Native Code Execution (NCE) engineering on Android.</li>"
        "<li><b>Mitsunari Shigeo (herumi / xbyak) &amp; Oak Authors</b> — high-speed x86_64 and AArch64 runtime code generators.</li>"
        "<li><b>SciresM, TuxSH, fincs, hexkyz, mtheall &amp; Atmosphère / Switchbrew Teams</b> — hardware documentation, Mesosphere kernel specs, NCA cryptography, and hactool / libnx tooling.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>4. Data Formats, Graphics Tech, Audio &amp; System Libraries</h4><ul>"
        "<li><b>nicoboss &amp; NSZ Contributors</b> — streamable NSZ / NCZ / XCZ compressed game container specifications and tooling.</li>"
        "<li><b>Yann Collet &amp; Zstandard (Meta) / LZ4 Teams</b> — reference ultra-fast real-time compression algorithms.</li>"
        "<li><b>Khronos Group</b> — Vulkan API specifications, SPIR-V, Vulkan-Headers, glslang, and SPIRV-Cross.</li>"
        "<li><b>AMD GPUOpen (FidelityFX &amp; Vulkan Memory Allocator)</b> — FidelityFX Super Resolution (FSR) / CAS upscaling and VMA GPU memory management.</li>"
        "<li><b>Sirit Team (Rodrigo and contributors)</b> — fast runtime SPIR-V module emitter.</li>"
        "<li><b>The Qt Company &amp; Qt Contributors</b> — cross-platform desktop UI framework.</li>"
        "<li><b>Sam Lantinga &amp; SDL Team, Mozilla (Cubeb), and Google Oboe / OpenAL</b> — cross-platform controller, motion gyro, HD-Rumble, and low-latency audio stack.</li>"
        "<li><b>FFmpeg, VideoLAN (dav1d) &amp; Xiph (Opus) Teams</b> — high-performance H.264 / VP9 / AV1 video and multichannel audio decoding.</li>"
        "<li><b>Boost, fmt (Victor Zverovich), nlohmann/json, OpenSSL, MbedTLS, libusb, enet &amp; Inno Setup (Jordan Russell)</b> — rock-solid C++ systems, networking, and installer foundations.</li></ul>"
        "<h4 style='color:%2; margin-bottom:4px;'>5. Mods, Metadata Databases, Ecosystem &amp; Community</h4><ul>"
        "<li><b>GameBanana &amp; Community Mod Authors</b> — open API and the largest repository of community mods, visual upgrades, and translations.</li>"
        "<li><b>blawar &amp; TitleDB Contributors, and N3evin &amp; AmiiboAPI</b> — open title metadata, cover art, update databases, and Amiibo figurine APIs.</li>"
        "<li><b>60 FPS, Dynamic FPS &amp; Ultrawide Patch Authors (ChanseyIsTheBest, StevensND, Fl4sh, KeatonTheBot, boy181, and others)</b> — technical performance and compatibility patches.</li>"
        "<li><b>STORM TEAM &amp; STORM GAMES WORLD</b> — emulator engineering, STORM SAVE SYNC cloud service, GameFix Database, STORM AURORA FLUID design system, and ecosystem infrastructure.</li>"
        "<li><b>Community Testers &amp; Bug Reporters across 4PDA, GitHub, Discord, and Forums</b> — testing hundreds of games across diverse PC and mobile hardware and providing detailed logs and feedback.</li></ul>"
        "</div>").arg(pal.textPrimary, pal.accent);

    credits_browser->setHtml(is_ru ? credits_ru : credits_en);

    const QString tab_about_title = StormLangStorm11(
        QStringLiteral("О программе"), QStringLiteral("About"), QStringLiteral("Über"),
        QStringLiteral("À propos"), QStringLiteral("关于"), QStringLiteral("정보"),
        QStringLiteral("概要"), QStringLiteral("Acerca de"), QStringLiteral("Informazioni"),
        QStringLiteral("Sobre"), QStringLiteral("परिचय"));

    const QString tab_credits_title = StormLangStorm11(
        QStringLiteral("Благодарности"), QStringLiteral("Credits & Acknowledgments"), QStringLiteral("Danksagungen"),
        QStringLiteral("Remerciements"), QStringLiteral("致谢"), QStringLiteral("감사의 말"),
        QStringLiteral("謝辞"), QStringLiteral("Agradecimientos"), QStringLiteral("Riconoscimenti"),
        QStringLiteral("Agradecimentos"), QStringLiteral("आभार"));

    tab_widget->addTab(overview_page, tab_about_title);
    tab_widget->addTab(credits_browser, tab_credits_title);
    ui->verticalLayout->addWidget(tab_widget, 1);

    StormUI::StructureEngine::ModernizeForm(this);
    StormUI::StructureEngine::InstallEscDismiss(this);
}

AboutDialog::~AboutDialog() = default;
