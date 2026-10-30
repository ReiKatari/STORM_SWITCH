// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIODevice>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStyleOption>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <ankerl/unordered_dense.h>
#include "common/settings.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/hle/service/game_fix_database.h"
#include "core/loader/loader.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"

inline QString StormLang(const QString& ru, const QString& en,
                         const QString& de, const QString& fr,
                         const QString& zh, const QString& ja,
                         const QString& ar, const QString& es,
                         const QString& pt, const QString& it = QString(),
                         const QString& ko = QString()) {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    if (lang.rfind("pt", 0) == 0 && !pt.isEmpty()) return pt;
    if (lang.rfind("it", 0) == 0 && !it.isEmpty()) return it;
    if (lang.rfind("ko", 0) == 0 && !ko.isEmpty()) return ko;
    return StormLang(ru, en, de, fr, zh, ja, ar, es);
}
#include "storm_switch/configuration/retroarch_shader_catalog.h"
#include "storm_switch/loading_screen.h"
#include "ui_loading_screen.h"
#include "video_core/rasterizer_interface.h"

#if !YUZU_QT_MOVIE_MISSING
#include <QMovie>
#endif

namespace {

struct LoadingDashboardWidgets {
    QLabel* engine_badge = nullptr;
    QLabel* meta_pills_label = nullptr;
    QPushButton* copy_tid_btn = nullptr;
    QString current_tid_hex;

    std::array<QLabel*, 8> card_titles{};
    std::array<QLabel*, 8> card_values{};
    std::array<QFrame*, 4> step_frames{};
    std::array<QLabel*, 4> step_labels{};

    QLabel* timer_badge = nullptr;
    QLabel* tip_title_badge = nullptr;
    QLabel* tip_text_label = nullptr;
    QPushButton* next_tip_btn = nullptr;
    QTimer* live_tick_timer = nullptr;

    std::chrono::steady_clock::time_point boot_start_time{};
    int current_tip_index = 0;
    int active_step = 0;
};

static ankerl::unordered_dense::map<const LoadingScreen*, LoadingDashboardWidgets> s_dash_map;

static int GetTipCount() {
    return 6;
}

static QString GetLocalizedTipText(int idx) {
    switch (idx % GetTipCount()) {
    case 0:
        return StormLang(
            QStringLiteral("Быстрое сохранение и загрузка: нажмите <b>F5</b> для мгновенного сохранения и <b>F7</b> для быстрой загрузки в любой момент игры."),
            QStringLiteral("Quick Save & Load: press <b>F5</b> for instant save and <b>F7</b> for quick load at any moment during gameplay."),
            QStringLiteral("Schnellspeichern & Laden: Drücken Sie <b>F5</b> zum sofortigen Speichern und <b>F7</b> zum Schnellladen im Spiel."),
            QStringLiteral("Sauvegarde et chargement rapides : appuyez sur <b>F5</b> pour sauvegarder et <b>F7</b> pour charger instantanément."),
            QStringLiteral("快速保存与读取：在游戏过程中随时按 <b>F5</b> 立即保存，按 <b>F7</b> 快速读取进度。"),
            QStringLiteral("クイックセーブ＆ロード：ゲーム中に <b>F5</b> で即座に保存、<b>F7</b> で素早くロードできます。"),
            QStringLiteral("الحفظ والتحميل السريع: اضغط <b>F5</b> للحفظ الفوري و <b>F7</b> للتحميل السريع في أي لحظة."),
            QStringLiteral("Guardado y carga rápida: pulsa <b>F5</b> para guardar al instante y <b>F7</b> para cargar rápidamente."),
            QStringLiteral("Salvataggio e caricamento rapido: premi <b>F5</b> per salvare e <b>F7</b> per caricare istantaneamente."),
            QStringLiteral("Salvamento e carregamento rápido: pressione <b>F5</b> para salvar e <b>F7</b> para carregar instantaneamente."),
            QStringLiteral("빠른 저장 및 불러오기: 게임 중 언제든지 <b>F5</b>로 즉시 저장하고 <b>F7</b>로 빠르게 불러올 수 있습니다."));
    case 1:
        return StormLang(
            QStringLiteral("Шейдеры RetroArch в реальном времени: нажмите кнопку <b>«ШЕЙДЕР»</b> в нижней панели, чтобы переключать 74 эффекта (CRT, VHS, Anime4K, резкость) на лету!"),
            QStringLiteral("Real-time RetroArch Shaders: click the <b>\"SHADER\"</b> button in the footer bar to switch 74 effects (CRT, VHS, Anime4K, Sharpen) on the fly!"),
            QStringLiteral("Echtzeit-RetroArch-Shader: Klicken Sie unten auf <b>„SHADER“</b>, um 74 Effekte (CRT, VHS, Anime4K, Schärfe) sofort umzuschalten!"),
            QStringLiteral("Shaders RetroArch en temps réel : cliquez sur <b>« SHADER »</b> dans la barre inférieure pour basculer entre 74 effets à la volée !"),
            QStringLiteral("实时 RetroArch 着色器：点击底部状态栏的 <b>“着色器”</b> 按钮，即可动态切换 74 种视觉特效（CRT、VHS、Anime4K、锐化）！"),
            QStringLiteral("リアルタイム RetroArch シェーダー：下部バーの <b>「シェーダー」</b> ボタンから74種類のエフェクトを即座に切り替え可能！"),
            QStringLiteral("مظللات RetroArch الفورية: انقر على زر <b>«المظلل»</b> في الشريط السفلي للتبديل بين 74 تأثيراً بصرياً فوراً!"),
            QStringLiteral("Shaders RetroArch en tiempo real: ¡haz clic en <b>«SHADER»</b> en la barra inferior para cambiar entre 74 efectos al vuelo!"),
            QStringLiteral("Shader RetroArch in tempo reale: clicca su <b>«SHADER»</b> nella barra inferiore per cambiare 74 effetti al volo!"),
            QStringLiteral("Shaders RetroArch em tempo real: clique em <b>«SHADER»</b> na barra inferior para alternar 74 efeitos instantaneamente!"),
            QStringLiteral("실시간 RetroArch 셰이더: 하단 바의 <b>'셰이더'</b> 버튼을 클릭하여 74가지 효과를 즉시 전환하세요!"));
    case 2:
        return StormLang(
            QStringLiteral("Менеджер состояний (<b>F8</b>): открывает визуальные слоты сохранений со скриншотами, точным временем и резервной защитой."),
            QStringLiteral("Save State Manager (<b>F8</b>): opens visual save slots with screenshots, timestamps, and atomic crash protection."),
            QStringLiteral("Spielstand-Manager (<b>F8</b>): Öffnet visuelle Speicherplätze mit Screenshots, Zeitstempeln und Absturzschutz."),
            QStringLiteral("Gestionnaire d'états (<b>F8</b>) : ouvre les emplacements visuels avec captures d'écran et horodatage."),
            QStringLiteral("即时存档管理器 (<b>F8</b>)：打开带有预览截图、时间戳和防损坏保护的可视化存档槽位。"),
            QStringLiteral("ステートセーブマネージャー (<b>F8</b>)：スクリーンショットとタイムスタンプ付きの視覚的なスロットを開きます。"),
            QStringLiteral("مدير حالات الحفظ (<b>F8</b>): يفتح فتحات الحفظ المرئية مع لقطات الشاشة والطوابع الزمنية."),
            QStringLiteral("Gestor de estados (<b>F8</b>): abre ranuras visuales de guardado con capturas de pantalla y marcas de tiempo."),
            QStringLiteral("Gestore stati di salvataggio (<b>F8</b>): apre slot visivi con screenshot e protezione dai crash."),
            QStringLiteral("Gerenciador de estados (<b>F8</b>): abre slots visuais com capturas de tela e proteção contra falhas."),
            QStringLiteral("강제 저장 관리자(<b>F8</b>): 스크린샷과 타임스탬프가 포함된 시각적 저장 슬롯을 엽니다."));
    case 3:
        return StormLang(
            QStringLiteral("Изоляция авто-исправлений: профили оптимизации применяются строго на время сессии игры и автоматически восстанавливают ваши глобальные настройки после выхода."),
            QStringLiteral("Auto-Fix Session Isolation: optimization profiles apply strictly for the game session and automatically restore your global settings upon exit."),
            QStringLiteral("Auto-Fix-Sitzungsisolierung: Optimierungsprofile gelten nur für die Spielsitzung und stellen globale Einstellungen beim Beenden wieder her."),
            QStringLiteral("Isolation de session Auto-Fix : les profils s'appliquent uniquement à la session de jeu sans altérer vos paramètres globaux."),
            QStringLiteral("自动修复会话隔离：游戏优化配置仅在当前运行会话期间生效，退出游戏后将 100% 自动恢复您的全局设置。"),
            QStringLiteral("自動修正のセッション分離：最適化プロファイルはゲーム実行中のみ適用され、終了時に全体設定が自動的に復元されます。"),
            QStringLiteral("عزل جلسة الإصلاح التلقائي: تُطبّق ملفات التحسين أثناء جلسة اللعب فقط وتُستعاد إعداداتك العامة تلقائياً عند الخروج."),
            QStringLiteral("Aislamiento de sesión Auto-Fix: los perfiles se aplican solo durante la sesión y restauran tu configuración global al salir."),
            QStringLiteral("Isolamento sessione Auto-Fix: i profili si applicano solo alla sessione di gioco ripristinando le impostazioni globali all'uscita."),
            QStringLiteral("Isolamento de sessão Auto-Fix: os perfis aplicam-se apenas à sessão de jogo e restauram as configurações globais ao sair."),
            QStringLiteral("자동 수정 세션 격리: 최적화 프로필은 게임 세션 동안만 적용되며 종료 시 전역 설정이 자동으로 복원됩니다."));
    case 4:
        return StormLang(
            QStringLiteral("Полноэкранный режим и курсор: нажмите <b>F11</b> или <b>Alt+Enter</b> для переключения полного экрана, а <b>F9</b> — для захвата мыши в камере."),
            QStringLiteral("Fullscreen & Mouse Panning: press <b>F11</b> or <b>Alt+Enter</b> to toggle fullscreen, and <b>F9</b> to toggle mouse camera panning."),
            QStringLiteral("Vollbild & Maus-Kamera: Drücken Sie <b>F11</b> oder <b>Alt+Enter</b> für Vollbild und <b>F9</b> für Maus-Kamerasteuerung."),
            QStringLiteral("Plein écran et souris : appuyez sur <b>F11</b> ou <b>Alt+Enter</b> pour le plein écran, et <b>F9</b> pour la caméra souris."),
            QStringLiteral("全屏与鼠标视角：按 <b>F11</b> 或 <b>Alt+Enter</b> 切换全屏模式，按 <b>F9</b> 切换鼠标视角控制。"),
            QStringLiteral("フルスクリーンとマウス視点：<b>F11</b> または <b>Alt+Enter</b> で全画面切替、<b>F9</b> でマウスカメラ操作を切り替えます。"),
            QStringLiteral("ملء الشاشة وتحريك الفأرة: اضغط <b>F11</b> أو <b>Alt+Enter</b> لملء الشاشة، و <b>F9</b> لتحريك الكاميرا بالفأرة."),
            QStringLiteral("Pantalla completa y ratón: pulsa <b>F11</b> o <b>Alt+Enter</b> para pantalla completa y <b>F9</b> para cámara con ratón."),
            QStringLiteral("Schermo intero e mouse: premi <b>F11</b> o <b>Alt+Enter</b> per lo schermo intero e <b>F9</b> per la visuale col mouse."),
            QStringLiteral("Tela cheia e mouse: pressione <b>F11</b> ou <b>Alt+Enter</b> para tela cheia e <b>F9</b> para câmera no mouse."),
            QStringLiteral("전체 화면 및 마우스 시점: <b>F11</b> 또는 <b>Alt+Enter</b>로 전체 화면을 전환하고 <b>F9</b>로 마우스 시점을 전환합니다."));
    default:
        return StormLang(
            QStringLiteral("Динамическое управление графикой: масштаб разрешения, фильтр FSR/SGSR, сглаживание SMAA и режим Док-станции можно переключать прямо во время игры!"),
            QStringLiteral("Dynamic Graphics Control: resolution scale, FSR/SGSR filter, SMAA anti-aliasing, and Docked mode can be switched live during gameplay!"),
            QStringLiteral("Dynamische Grafiksteuerung: Auflösung, FSR/SGSR-Filter, SMAA-Kantenglättung und Dock-Modus lassen sich live im Spiel umschalten!"),
            QStringLiteral("Contrôle graphique dynamique : l'échelle de résolution, le filtre FSR/SGSR, le SMAA et le mode Dock sont modifiables en plein jeu !"),
            QStringLiteral("动态图形控制：分辨率倍率、FSR/SGSR 缩放滤镜、SMAA 抗锯齿以及底座/掌机模式均可在游戏运行时实时切换！"),
            QStringLiteral("動的グラフィックス制御：解像度スケール、FSR/SGSRフィルター、SMAA、ドックモードはゲーム中にリアルタイムで切り替え可能です！"),
            QStringLiteral("تحكم ديناميكي بالرسومات: يمكن تغيير دقة العرض ومرشح FSR/SGSR ومضاد التعرج SMAA ووضع القاعدة مباشرة أثناء اللعب!"),
            QStringLiteral("Control gráfico dinámico: ¡la escala de resolución, el filtro FSR/SGSR, SMAA y el modo Dock se pueden cambiar en vivo durante el juego!"),
            QStringLiteral("Controllo grafico dinamico: scala risoluzione, filtro FSR/SGSR, SMAA e modalità Dock sono modificabili in tempo reale in gioco!"),
            QStringLiteral("Controle gráfico dinâmico: escala de resolução, filtro FSR/SGSR, SMAA e modo Dock podem ser alterados ao vivo durante o jogo!"),
            QStringLiteral("동적 그래픽 제어: 해상도 배율, FSR/SGSR 필터, SMAA 안티앨리어싱 및 독 모드를 게임 중 실시간으로 전환할 수 있습니다!"));
    }
}

static void UpdateStepStyles(LoadingDashboardWidgets& dash, int active_step) {
    dash.active_step = active_step;
    const std::array<QString, 4> names = {
        StormLang(QStringLiteral("1. Файловая система"), QStringLiteral("1. Virtual FS"), QStringLiteral("1. Dateisystem"), QStringLiteral("1. Système VFS"), QStringLiteral("1. 虚拟文件系统"), QStringLiteral("1. ファイルシステム"), QStringLiteral("1. نظام الملفات"), QStringLiteral("1. Sistema VFS"), QStringLiteral("1. File System"), QStringLiteral("1. Sistema VFS"), QStringLiteral("1. 파일 시스템")),
        StormLang(QStringLiteral("2. Загрузчик и метаданные"), QStringLiteral("2. Loader & NACP"), QStringLiteral("2. Loader & Metadaten"), QStringLiteral("2. Chargeur & NACP"), QStringLiteral("2. 加载器与元数据"), QStringLiteral("2. ローダー＆メタデータ"), QStringLiteral("2. المحمل والبيانات"), QStringLiteral("2. Cargador y NACP"), QStringLiteral("2. Loader e NACP"), QStringLiteral("2. Carregador e NACP"), QStringLiteral("2. 로더 및 메타데이터")),
        StormLang(QStringLiteral("3. Конвейеры шейдеров"), QStringLiteral("3. Shader Pipelines"), QStringLiteral("3. Shader-Pipelines"), QStringLiteral("3. Pipelines Shaders"), QStringLiteral("3. 着色器管线编译"), QStringLiteral("3. シェーダーパイプライン"), QStringLiteral("3. أنابيب المظللات"), QStringLiteral("3. Pipelines de Shaders"), QStringLiteral("3. Pipeline Shader"), QStringLiteral("3. Pipelines de Shaders"), QStringLiteral("3. 셰이더 파이프라인")),
        StormLang(QStringLiteral("4. Запуск ядра и GPU"), QStringLiteral("4. Kernel & GPU Launch"), QStringLiteral("4. Kernel- & GPU-Start"), QStringLiteral("4. Lancement Noyau & GPU"), QStringLiteral("4. 内核与 GPU 启动"), QStringLiteral("4. カーネル＆GPU起動"), QStringLiteral("4. تشغيل النواة و GPU"), QStringLiteral("4. Inicio de Kernel y GPU"), QStringLiteral("4. Avvio Kernel e GPU"), QStringLiteral("4. Início do Kernel e GPU"), QStringLiteral("4. 커널 및 GPU 시작")),
    };

    for (int i = 0; i < 4; ++i) {
        if (!dash.step_frames[i] || !dash.step_labels[i]) {
            continue;
        }
        if (i < active_step) {
            dash.step_frames[i]->setStyleSheet(QStringLiteral(
                "QFrame { background: rgba(16, 185, 129, 0.14); border: 1px solid rgba(16, 185, 129, 0.48); border-radius: 7px; }"));
            dash.step_labels[i]->setStyleSheet(QStringLiteral(
                "QLabel { color: #34D399; font-size: 11px; font-weight: 700; background: transparent; border: none; }"));
            dash.step_labels[i]->setText(QStringLiteral("✓ ") + names[i]);
        } else if (i == active_step) {
            dash.step_frames[i]->setStyleSheet(QStringLiteral(
                "QFrame { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 229, 255, 0.22), stop:1 rgba(56, 189, 248, 0.14)); border: 1.5px solid #00E5FF; border-radius: 7px; }"));
            dash.step_labels[i]->setStyleSheet(QStringLiteral(
                "QLabel { color: #FFFFFF; font-size: 11px; font-weight: 800; background: transparent; border: none; }"));
            dash.step_labels[i]->setText(QStringLiteral("⚡ ") + names[i]);
        } else {
            dash.step_frames[i]->setStyleSheet(QStringLiteral(
                "QFrame { background: rgba(15, 23, 42, 0.55); border: 1px solid rgba(148, 163, 184, 0.18); border-radius: 7px; }"));
            dash.step_labels[i]->setStyleSheet(QStringLiteral(
                "QLabel { color: #FFFFFF; font-size: 11px; font-weight: 600; background: transparent; border: none; }"));
            dash.step_labels[i]->setText(names[i]);
        }
    }
}

static void RefreshDashboardTexts(LoadingDashboardWidgets& dash) {
    const bool autofix_on = Core::GameFixDatabase::AreFixesEnabled();
    if (dash.engine_badge) {
        if (autofix_on) {
            dash.engine_badge->setText(StormLang(
                QStringLiteral("🛡️ АВТО-ИСПРАВЛЕНИЕ АКТИВНО"),
                QStringLiteral("🛡️ AUTO-FIX PROFILE ACTIVE"),
                QStringLiteral("🛡️ AUTO-FIX-PROFIL AKTIV"),
                QStringLiteral("🛡️ PROFIL AUTO-FIX ACTIF"),
                QStringLiteral("🛡️ 自动修复配置已激活"),
                QStringLiteral("🛡️ 自動修正プロファイル有効"),
                QStringLiteral("🛡️ ملف الإصلاح التلقائي نشط"),
                QStringLiteral("🛡️ PERFIL AUTO-FIX ACTIVO"),
                QStringLiteral("🛡️ PROFILO AUTO-FIX ATTIVO"),
                QStringLiteral("🛡️ PERFIL AUTO-FIX ATIVO"),
                QStringLiteral("🛡️ 자동 수정 프로필 활성")));
            dash.engine_badge->setStyleSheet(QStringLiteral(
                "QLabel { background: rgba(16, 185, 129, 0.16); color: #34D399; border: 1px solid rgba(16, 185, 129, 0.45); border-radius: 6px; padding: 3px 10px; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; }"));
        } else {
            dash.engine_badge->setText(StormLang(
                QStringLiteral("⚡ ДВИЖОК STORM SWITCH"),
                QStringLiteral("⚡ STORM SWITCH ENGINE"),
                QStringLiteral("⚡ STORM SWITCH ENGINE"),
                QStringLiteral("⚡ MOTEUR STORM SWITCH"),
                QStringLiteral("⚡ STORM SWITCH 高速引擎"),
                QStringLiteral("⚡ STORM SWITCH エンジン"),
                QStringLiteral("⚡ محرك STORM SWITCH"),
                QStringLiteral("⚡ MOTOR STORM SWITCH"),
                QStringLiteral("⚡ MOTORE STORM SWITCH"),
                QStringLiteral("⚡ MOTOR STORM SWITCH"),
                QStringLiteral("⚡ STORM SWITCH 엔진")));
            dash.engine_badge->setStyleSheet(QStringLiteral(
                "QLabel { background: rgba(0, 210, 255, 0.14); color: #38BDF8; border: 1px solid rgba(0, 210, 255, 0.40); border-radius: 6px; padding: 3px 10px; font-size: 10.5px; font-weight: 800; letter-spacing: 0.5px; }"));
        }
    }

    if (dash.copy_tid_btn) {
        if (!dash.current_tid_hex.isEmpty()) {
            dash.copy_tid_btn->setVisible(true);
            dash.copy_tid_btn->setText(QStringLiteral("🆔 %1 📋").arg(dash.current_tid_hex));
            dash.copy_tid_btn->setToolTip(StormLang(
                QStringLiteral("Нажмите, чтобы скопировать Title ID игры в буфер обмена"),
                QStringLiteral("Click to copy the game's Title ID to clipboard"),
                QStringLiteral("Klicken, um die Title-ID in die Zwischenablage zu kopieren"),
                QStringLiteral("Cliquer pour copier le Title ID dans le presse-papiers"),
                QStringLiteral("点击将游戏的 Title ID 复制到剪贴板"),
                QStringLiteral("クリックしてTitle IDをクリップボードにコピー"),
                QStringLiteral("انقر لنسخ معرف اللعبة Title ID إلى الحافظة"),
                QStringLiteral("Haz clic para copiar el Title ID al portapapeles"),
                QStringLiteral("Clicca per copiare il Title ID negli appunti"),
                QStringLiteral("Clique para copiar o Title ID para a área de transferência"),
                QStringLiteral("클릭하여 게임 Title ID를 클립보드에 복사")));
        } else {
            dash.copy_tid_btn->setVisible(false);
        }
    }

    // 1. Graphics API & GPU Accuracy
    const QString api_str = (Settings::values.renderer_backend.GetValue() == Settings::RendererBackend::Vulkan)
                                ? QStringLiteral("Vulkan")
                                : QStringLiteral("OpenGL");
    const auto gpu_acc = Settings::values.gpu_accuracy.GetValue();
    QString gpu_acc_str;
    if (gpu_acc == Settings::GpuAccuracy::Low) {
        gpu_acc_str = StormLang(QStringLiteral("Быстрая"), QStringLiteral("Fast"), QStringLiteral("Schnell"), QStringLiteral("Rapide"), QStringLiteral("快速"), QStringLiteral("高速"), QStringLiteral("سريع"), QStringLiteral("Rápida"), QStringLiteral("Veloce"), QStringLiteral("Rápida"), QStringLiteral("빠름"));
    } else {
        gpu_acc_str = StormLang(QStringLiteral("Высокая"), QStringLiteral("High"), QStringLiteral("Hoch"), QStringLiteral("Élevée"), QStringLiteral("高"), QStringLiteral("高"), QStringLiteral("عالية"), QStringLiteral("Alta"), QStringLiteral("Alta"), QStringLiteral("Alta"), QStringLiteral("높음"));
    }
    if (dash.card_titles[0]) dash.card_titles[0]->setText(StormLang(QStringLiteral("🎮 ГРАФИКА И GPU"), QStringLiteral("🎮 GRAPHICS & GPU"), QStringLiteral("🎮 GRAFIK & GPU"), QStringLiteral("🎮 GRAPHISMES & GPU"), QStringLiteral("🎮 图形接口与精度"), QStringLiteral("🎮 グラフィックス＆GPU"), QStringLiteral("🎮 الرسومات و GPU"), QStringLiteral("🎮 GRÁFICOS Y GPU"), QStringLiteral("🎮 GRAFICA E GPU"), QStringLiteral("🎮 GRÁFICOS E GPU"), QStringLiteral("🎮 그래픽 및 GPU")));
    if (dash.card_values[0]) dash.card_values[0]->setText(QStringLiteral("%1 · %2").arg(api_str, gpu_acc_str));

    // 2. Resolution & Console Mode
    QString res_str = QStringLiteral("1x (720p/1080p)");
    switch (Settings::values.resolution_setup.GetValue()) {
    case Settings::ResolutionSetup::Res1_4X: res_str = QStringLiteral("0.25x"); break;
    case Settings::ResolutionSetup::Res1_2X: res_str = QStringLiteral("0.5x"); break;
    case Settings::ResolutionSetup::Res3_4X: res_str = QStringLiteral("0.75x"); break;
    case Settings::ResolutionSetup::Res1X: res_str = QStringLiteral("1x (Native)"); break;
    case Settings::ResolutionSetup::Res5_4X: res_str = QStringLiteral("1.25x"); break;
    case Settings::ResolutionSetup::Res3_2X: res_str = QStringLiteral("1.5x"); break;
    case Settings::ResolutionSetup::Res2X: res_str = QStringLiteral("2x (1440p/4K)"); break;
    case Settings::ResolutionSetup::Res3X: res_str = QStringLiteral("3x"); break;
    case Settings::ResolutionSetup::Res4X: res_str = QStringLiteral("4x"); break;
    case Settings::ResolutionSetup::Res5X: res_str = QStringLiteral("5x"); break;
    case Settings::ResolutionSetup::Res6X: res_str = QStringLiteral("6x"); break;
    case Settings::ResolutionSetup::Res7X: res_str = QStringLiteral("7x"); break;
    case Settings::ResolutionSetup::Res8X: res_str = QStringLiteral("8x"); break;
    default: break;
    }
    const bool is_docked = (Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked);
    const QString dock_str = is_docked
        ? StormLang(QStringLiteral("Док-станция"), QStringLiteral("Docked"), QStringLiteral("Dock-Modus"), QStringLiteral("Docké"), QStringLiteral("主机底座"), QStringLiteral("ドック"), QStringLiteral("القاعدة"), QStringLiteral("Dock"), QStringLiteral("Dock"), QStringLiteral("Dock"), QStringLiteral("독 모드"))
        : StormLang(QStringLiteral("Портативный"), QStringLiteral("Handheld"), QStringLiteral("Handheld"), QStringLiteral("Portable"), QStringLiteral("掌机模式"), QStringLiteral("携帯"), QStringLiteral("محمول"), QStringLiteral("Portátil"), QStringLiteral("Portatile"), QStringLiteral("Portátil"), QStringLiteral("휴대 모드"));
    if (dash.card_titles[1]) dash.card_titles[1]->setText(StormLang(QStringLiteral("🖥️ МАСШТАБ И РЕЖИМ"), QStringLiteral("🖥️ SCALE & MODE"), QStringLiteral("🖥️ SKALIERUNG & MODUS"), QStringLiteral("🖥️ ÉCHELLE & MODE"), QStringLiteral("🖥️ 分辨率与模式"), QStringLiteral("🖥️ 解像度＆モード"), QStringLiteral("🖥️ الدقة والوضع"), QStringLiteral("🖥️ ESCALA Y MODO"), QStringLiteral("🖥️ SCALA E MODALITÀ"), QStringLiteral("🖥️ ESCALA E MODO"), QStringLiteral("🖥️ 해상도 및 모드")));
    if (dash.card_values[1]) dash.card_values[1]->setText(QStringLiteral("%1 · %2").arg(res_str, dock_str));

    // 3. Scaling Filter & Anti-Aliasing
    QString filter_str = QStringLiteral("FSR");
    switch (Settings::values.scaling_filter.GetValue()) {
    case Settings::ScalingFilter::NearestNeighbor: filter_str = QStringLiteral("Nearest"); break;
    case Settings::ScalingFilter::Bilinear: filter_str = QStringLiteral("Bilinear"); break;
    case Settings::ScalingFilter::Bicubic: filter_str = QStringLiteral("Bicubic"); break;
    case Settings::ScalingFilter::Gaussian: filter_str = QStringLiteral("Gaussian"); break;
    case Settings::ScalingFilter::Lanczos: filter_str = QStringLiteral("Lanczos"); break;
    case Settings::ScalingFilter::ScaleForce: filter_str = QStringLiteral("ScaleForce"); break;
    case Settings::ScalingFilter::Fsr: filter_str = QStringLiteral("AMD FSR"); break;
    case Settings::ScalingFilter::Area: filter_str = QStringLiteral("Area"); break;
    case Settings::ScalingFilter::Mmpx: filter_str = QStringLiteral("MMPX"); break;
    case Settings::ScalingFilter::ZeroTangent: filter_str = QStringLiteral("Zero-Tangent"); break;
    case Settings::ScalingFilter::BSpline: filter_str = QStringLiteral("B-Spline"); break;
    case Settings::ScalingFilter::Mitchell: filter_str = QStringLiteral("Mitchell"); break;
    case Settings::ScalingFilter::Spline1: filter_str = QStringLiteral("Spline-1"); break;
    case Settings::ScalingFilter::Sgsr: filter_str = QStringLiteral("Snapdragon SGSR"); break;
    case Settings::ScalingFilter::SgsrEdge: filter_str = QStringLiteral("SGSR Edge"); break;
    default: break;
    }
    QString aa_str = StormLang(QStringLiteral("Отключено"), QStringLiteral("Off"), QStringLiteral("Aus"), QStringLiteral("Désactivé"), QStringLiteral("关闭"), QStringLiteral("オフ"), QStringLiteral("إيقاف"), QStringLiteral("Desact."), QStringLiteral("Disatt."), QStringLiteral("Desat."), QStringLiteral("끄기"));
    if (Settings::values.anti_aliasing.GetValue() == Settings::AntiAliasing::Fxaa) aa_str = QStringLiteral("FXAA");
    else if (Settings::values.anti_aliasing.GetValue() == Settings::AntiAliasing::Smaa) aa_str = QStringLiteral("SMAA");
    if (dash.card_titles[2]) dash.card_titles[2]->setText(StormLang(QStringLiteral("✨ ФИЛЬТР И СГЛАЖИВАНИЕ"), QStringLiteral("✨ FILTER & AA"), QStringLiteral("✨ FILTER & GLÄTTUNG"), QStringLiteral("✨ FILTRE & AA"), QStringLiteral("✨ 缩放滤镜与抗锯齿"), QStringLiteral("✨ フィルター＆AA"), QStringLiteral("✨ المرشح والتنعيم"), QStringLiteral("✨ FILTRO Y AA"), QStringLiteral("✨ FILTRO E AA"), QStringLiteral("✨ FILTRO E AA"), QStringLiteral("✨ 필터 및 안티앨리어싱")));
    if (dash.card_values[2]) dash.card_values[2]->setText(QStringLiteral("%1 · %2").arg(filter_str, aa_str));

    // 4. RetroArch Post-Processing Shader
    const int shader_idx = std::clamp(Settings::values.post_processing_shader.GetValue(), 0,
                                      static_cast<int>(StormShaders::GetRetroArchShaderCatalog().size()) - 1);
    QString shader_name = QString::fromUtf8(StormShaders::GetRetroArchShaderCatalog()[shader_idx].short_name);
    if (dash.card_titles[3]) dash.card_titles[3]->setText(StormLang(QStringLiteral("🎨 ШЕЙДЕР RETROARCH"), QStringLiteral("🎨 RETROARCH SHADER"), QStringLiteral("🎨 RETROARCH-SHADER"), QStringLiteral("🎨 SHADER RETROARCH"), QStringLiteral("🎨 RetroArch 着色器"), QStringLiteral("🎨 RetroArch シェーダー"), QStringLiteral("🎨 مظلل RETROARCH"), QStringLiteral("🎨 SHADER RETROARCH"), QStringLiteral("🎨 SHADER RETROARCH"), QStringLiteral("🎨 SHADER RETROARCH"), QStringLiteral("🎨 RetroArch 셰이더")));
    if (dash.card_values[3]) dash.card_values[3]->setText(shader_idx == 0 ? StormLang(QStringLiteral("Отключено (Оригинал)"), QStringLiteral("Off (Original)"), QStringLiteral("Aus (Original)"), QStringLiteral("Désactivé (Original)"), QStringLiteral("关闭 (原始画面)"), QStringLiteral("オフ (オリジナル)"), QStringLiteral("إيقاف (الأصلي)"), QStringLiteral("Desact. (Original)"), QStringLiteral("Disatt. (Originale)"), QStringLiteral("Desat. (Original)"), QStringLiteral("꺼짐 (원본)")) : shader_name);

    // 5. CPU & Memory Layout
    const auto mem_mode = Settings::values.memory_layout_mode.GetValue();
    QString ram_str = QStringLiteral("4 GB");
    if (mem_mode == Settings::MemoryLayout::Memory_6Gb) ram_str = QStringLiteral("6 GB");
    else if (mem_mode == Settings::MemoryLayout::Memory_8Gb) ram_str = QStringLiteral("8 GB");
    else if (mem_mode == Settings::MemoryLayout::Memory_10Gb) ram_str = QStringLiteral("10 GB");
    else if (mem_mode == Settings::MemoryLayout::Memory_12Gb) ram_str = QStringLiteral("12 GB");
    if (dash.card_titles[4]) dash.card_titles[4]->setText(StormLang(QStringLiteral("🧠 ПРОЦЕССОР И ОЗУ"), QStringLiteral("🧠 CPU & MEMORY"), QStringLiteral("🧠 CPU & SPEICHER"), QStringLiteral("🧠 CPU & MÉMOIRE"), QStringLiteral("🧠 处理器与内存"), QStringLiteral("🧠 CPU＆メモリ"), QStringLiteral("🧠 المعالج والذاكرة"), QStringLiteral("🧠 CPU Y MEMORIA"), QStringLiteral("🧠 CPU E MEMORIA"), QStringLiteral("🧠 CPU E MEMÓRIA"), QStringLiteral("🧠 CPU 및 메모리")));
    if (dash.card_values[4]) dash.card_values[4]->setText(QStringLiteral("Dynarmic JIT · %1").arg(ram_str));

    // 6. VSync & Frame Pacing
    QString vsync_str = QStringLiteral("Mailbox");
    switch (Settings::values.vsync_mode.GetValue()) {
    case Settings::VSyncMode::Immediate: vsync_str = StormLang(QStringLiteral("Выключено (Immediate)"), QStringLiteral("Off (Immediate)"), QStringLiteral("Aus (Immediate)"), QStringLiteral("Désactivé"), QStringLiteral("关闭 (立即)"), QStringLiteral("オフ (即時)"), QStringLiteral("إيقاف"), QStringLiteral("Desactivado"), QStringLiteral("Disattivato"), QStringLiteral("Desativado"), QStringLiteral("끄기")); break;
    case Settings::VSyncMode::Mailbox: vsync_str = QStringLiteral("Mailbox"); break;
    case Settings::VSyncMode::Fifo: vsync_str = QStringLiteral("FIFO (VSync)"); break;
    case Settings::VSyncMode::FifoRelaxed: vsync_str = QStringLiteral("FIFO Relaxed"); break;
    default: break;
    }
    const QString speed_str = Settings::values.use_speed_limit.GetValue()
        ? QStringLiteral("%1%").arg(Settings::values.speed_limit.GetValue())
        : StormLang(QStringLiteral("Максимум FPS"), QStringLiteral("Unlocked"), QStringLiteral("Entsperrt"), QStringLiteral("Débloqué"), QStringLiteral("不限帧率"), QStringLiteral("無制限"), QStringLiteral("غير محدود"), QStringLiteral("Desbloqueado"), QStringLiteral("Sbloccato"), QStringLiteral("Desbloqueado"), QStringLiteral("제한 없음"));
    if (dash.card_titles[5]) dash.card_titles[5]->setText(StormLang(QStringLiteral("⚡ СИНХРОНИЗАЦИЯ КАДРОВ"), QStringLiteral("⚡ FRAME PACING"), QStringLiteral("⚡ BILDSYNCHRONISATION"), QStringLiteral("⚡ SYNCHRO D'IMAGES"), QStringLiteral("⚡ 垂直同步与帧率"), QStringLiteral("⚡ フレーム同期"), QStringLiteral("⚡ مزامنة الإطارات"), QStringLiteral("⚡ SINCRONIZACIÓN"), QStringLiteral("⚡ SINCRONIZZAZIONE"), QStringLiteral("⚡ SINCRONIZAÇÃO"), QStringLiteral("⚡ 프레임 동기화")));
    if (dash.card_values[5]) dash.card_values[5]->setText(QStringLiteral("%1 · %2").arg(vsync_str, speed_str));

    // 7. Shader Cache & ASTC
    const QString cache_str = Settings::values.use_disk_shader_cache.GetValue()
        ? StormLang(QStringLiteral("Диск. кэш ВКЛ"), QStringLiteral("Disk Cache ON"), QStringLiteral("Disk-Cache EIN"), QStringLiteral("Cache disque ON"), QStringLiteral("磁盘缓存开启"), QStringLiteral("キャッシュ ON"), QStringLiteral("التخزين المؤقت مفعل"), QStringLiteral("Caché disco ON"), QStringLiteral("Cache disco ON"), QStringLiteral("Cache disco ON"), QStringLiteral("디스크 캐시 켜짐"))
        : StormLang(QStringLiteral("Диск. кэш ВЫКЛ"), QStringLiteral("Disk Cache OFF"), QStringLiteral("Disk-Cache AUS"), QStringLiteral("Cache disque OFF"), QStringLiteral("磁盘缓存关闭"), QStringLiteral("キャッシュ OFF"), QStringLiteral("التخزين المؤقت معطل"), QStringLiteral("Caché disco OFF"), QStringLiteral("Cache disco OFF"), QStringLiteral("Cache disco OFF"), QStringLiteral("디스크 캐시 꺼짐"));
    QString astc_str = QStringLiteral("ASTC GPU");
    if (Settings::values.accelerate_astc.GetValue() == Settings::AstcDecodeMode::Cpu) astc_str = QStringLiteral("ASTC CPU");
    else if (Settings::values.accelerate_astc.GetValue() == Settings::AstcDecodeMode::CpuAsynchronous) astc_str = QStringLiteral("ASTC Async");
    if (dash.card_titles[6]) dash.card_titles[6]->setText(StormLang(QStringLiteral("💾 КЭШ И ТЕКСТУРЫ"), QStringLiteral("💾 CACHE & TEXTURES"), QStringLiteral("💾 CACHE & TEXTUREN"), QStringLiteral("💾 CACHE & TEXTURES"), QStringLiteral("💾 缓存与纹理加速"), QStringLiteral("💾 キャッシュ＆テクスチャ"), QStringLiteral("💾 التخزين والقوام"), QStringLiteral("💾 CACHÉ Y TEXTURAS"), QStringLiteral("💾 CACHE E TEXTURE"), QStringLiteral("💾 CACHE E TEXTURAS"), QStringLiteral("💾 캐시 및 텍스처")));
    if (dash.card_values[6]) dash.card_values[6]->setText(QStringLiteral("%1 · %2").arg(cache_str, astc_str));

    // 8. Launch Profile & Audio
    const QString prof_str = autofix_on
        ? StormLang(QStringLiteral("Авто-исправление"), QStringLiteral("Auto-Fix Active"), QStringLiteral("Auto-Fix Aktiv"), QStringLiteral("Auto-Fix Actif"), QStringLiteral("自动修复已启用"), QStringLiteral("自動修正適用中"), QStringLiteral("إصلاح تلقائي"), QStringLiteral("Auto-Fix Activo"), QStringLiteral("Auto-Fix Attivo"), QStringLiteral("Auto-Fix Ativo"), QStringLiteral("자동 수정 적용됨"))
        : StormLang(QStringLiteral("Стандартный"), QStringLiteral("Standard Profile"), QStringLiteral("Standardprofil"), QStringLiteral("Profil Standard"), QStringLiteral("标准全局配置"), QStringLiteral("標準プロファイル"), QStringLiteral("الملف القياسي"), QStringLiteral("Perfil Estándar"), QStringLiteral("Profilo Standard"), QStringLiteral("Perfil Padrão"), QStringLiteral("표준 프로필"));
    if (dash.card_titles[7]) dash.card_titles[7]->setText(StormLang(QStringLiteral("🛡️ ПРОФИЛЬ ЗАПУСКА"), QStringLiteral("🛡️ LAUNCH PROFILE"), QStringLiteral("🛡️ STARTPROFIL"), QStringLiteral("🛡️ PROFIL DE LANCEMENT"), QStringLiteral("🛡️ 启动优化配置"), QStringLiteral("🛡️ 起動プロファイル"), QStringLiteral("🛡️ ملف التشغيل"), QStringLiteral("🛡️ PERFIL DE INICIO"), QStringLiteral("🛡️ PROFILO DI AVVIO"), QStringLiteral("🛡️ PERFIL DE INÍCIO"), QStringLiteral("🛡️ 실행 프로필")));
    if (dash.card_values[7]) dash.card_values[7]->setText(prof_str);

    // Tips section
    if (dash.tip_title_badge) {
        dash.tip_title_badge->setText(StormLang(
            QStringLiteral("💡 СОВЕТ (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 PRO TIP (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 TIPP (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 ASTUCE (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 实用技巧 (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 ヒント (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 نصيحة (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 CONSEJO (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 CONSIGLIO (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 DICA (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount()),
            QStringLiteral("💡 팁 (%1/%2)").arg((dash.current_tip_index % GetTipCount()) + 1).arg(GetTipCount())));
    }
    if (dash.tip_text_label) {
        dash.tip_text_label->setText(GetLocalizedTipText(dash.current_tip_index));
    }
    if (dash.next_tip_btn) {
        dash.next_tip_btn->setText(StormLang(
            QStringLiteral("Следующий ➔"),
            QStringLiteral("Next Tip ➔"),
            QStringLiteral("Nächster ➔"),
            QStringLiteral("Suivant ➔"),
            QStringLiteral("下一条 ➔"),
            QStringLiteral("次へ ➔"),
            QStringLiteral("التالي ➔"),
            QStringLiteral("Siguiente ➔"),
            QStringLiteral("Successivo ➔"),
            QStringLiteral("Próxima ➔"),
            QStringLiteral("다음 팁 ➔")));
    }

    UpdateStepStyles(dash, dash.active_step);
}

} // namespace

LoadingScreen::LoadingScreen(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::LoadingScreen>()),
      previous_stage(VideoCore::LoadCallbackStage::Complete) {
    ui->setupUi(this);
    setMinimumSize(Layout::MinimumSize::Width, Layout::MinimumSize::Height);

    // Upgrade center_card into a sleek wide Aurora glassmorphic dashboard
    ui->center_card->setMinimumSize(1080, 0);
    ui->center_card->setMaximumSize(1440, 16777215);
    ui->center_card->setStyleSheet(QStringLiteral(
        "QFrame#center_card {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(13, 18, 30, 0.97), stop:0.55 rgba(9, 13, 22, 0.98), stop:1 rgba(6, 9, 16, 0.98));"
        "  border: 1.5px solid rgba(0, 210, 255, 0.36);"
        "  border-radius: 18px;"
        "  padding: 20px;"
        "}"
        "QLabel { background-color: transparent; background: transparent; border: none; }"));

    ui->card_layout->setSpacing(14);
    ui->card_layout->setContentsMargins(24, 20, 24, 20);

    // Re-arrange icon + title + metadata into a modern horizontal Hero Banner at the top of card_layout
    ui->card_layout->removeWidget(ui->game_icon);
    ui->card_layout->removeWidget(ui->game_title);
    ui->card_layout->removeWidget(ui->game_meta);

    auto* hero_frame = new QFrame(ui->center_card);
    hero_frame->setObjectName(QStringLiteral("hero_frame"));
    hero_frame->setMinimumHeight(182);
    hero_frame->setStyleSheet(QStringLiteral(
        "QFrame#hero_frame { background: rgba(15, 23, 42, 0.62); border: 1px solid rgba(56, 189, 248, 0.22); border-radius: 14px; }"));
    auto* hero_layout = new QHBoxLayout(hero_frame);
    hero_layout->setContentsMargins(18, 16, 18, 16);
    hero_layout->setSpacing(20);

    ui->game_icon->setMinimumSize(148, 148);
    ui->game_icon->setMaximumSize(148, 148);
    ui->game_icon->setStyleSheet(QStringLiteral(
        "QLabel#game_icon { background: transparent; border: none; padding: 0px; }"));
    hero_layout->addWidget(ui->game_icon, 0, Qt::AlignVCenter);

    auto* hero_right = new QVBoxLayout();
    hero_right->setContentsMargins(0, 2, 0, 2);
    hero_right->setSpacing(8);

    auto* title_row = new QHBoxLayout();
    title_row->setSpacing(12);
    ui->game_title->setMinimumHeight(96);
    ui->game_title->setMaximumHeight(124);
    ui->game_title->setWordWrap(true);
    ui->game_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title_row->addWidget(ui->game_title, 1);

    auto& dash = s_dash_map[this];
    dash.boot_start_time = std::chrono::steady_clock::now();

    dash.engine_badge = new QLabel(hero_frame);
    dash.engine_badge->setAlignment(Qt::AlignCenter);
    title_row->addWidget(dash.engine_badge, 0, Qt::AlignRight | Qt::AlignTop);
    hero_right->addLayout(title_row);

    ui->game_meta->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->game_meta->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 10.5pt; background: transparent; border: none;"));
    hero_right->addWidget(ui->game_meta);

    auto* badges_row = new QHBoxLayout();
    badges_row->setSpacing(10);
    dash.meta_pills_label = new QLabel(hero_frame);
    dash.meta_pills_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 10pt; background: transparent; border: none;"));
    badges_row->addWidget(dash.meta_pills_label);

    dash.copy_tid_btn = new QPushButton(hero_frame);
    dash.copy_tid_btn->setCursor(Qt::PointingHandCursor);
    dash.copy_tid_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(0, 210, 255, 0.12); color: #38BDF8; border: 1px solid rgba(0, 210, 255, 0.38); border-radius: 6px; padding: 3px 10px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: 700; }"
        "QPushButton:hover { background: rgba(0, 210, 255, 0.25); color: #FFFFFF; border: 1px solid #00E5FF; }"));
    connect(dash.copy_tid_btn, &QPushButton::clicked, this, [this]() {
        auto it = s_dash_map.find(this);
        if (it == s_dash_map.end() || it->second.current_tid_hex.isEmpty()) {
            return;
        }
        QGuiApplication::clipboard()->setText(it->second.current_tid_hex);
        if (it->second.copy_tid_btn) {
            it->second.copy_tid_btn->setText(StormLang(
                QStringLiteral("✅ ID скопирован: %1"),
                QStringLiteral("✅ ID Copied: %1"),
                QStringLiteral("✅ ID kopiert: %1"),
                QStringLiteral("✅ ID copié : %1"),
                QStringLiteral("✅ ID 已复制: %1"),
                QStringLiteral("✅ ID コピー済: %1"),
                QStringLiteral("✅ تم نسخ المعرف: %1"),
                QStringLiteral("✅ ID copiado: %1"),
                QStringLiteral("✅ ID copiato: %1"),
                QStringLiteral("✅ ID copiado: %1"),
                QStringLiteral("✅ ID 복사됨: %1")).arg(it->second.current_tid_hex));
            QTimer::singleShot(1800, this, [this]() {
                auto it2 = s_dash_map.find(this);
                if (it2 != s_dash_map.end() && it2->second.copy_tid_btn && !it2->second.current_tid_hex.isEmpty()) {
                    it2->second.copy_tid_btn->setText(QStringLiteral("🆔 %1 📋").arg(it2->second.current_tid_hex));
                }
            });
        }
    });
    badges_row->addWidget(dash.copy_tid_btn);
    badges_row->addStretch(1);
    hero_right->addLayout(badges_row);

    hero_layout->addLayout(hero_right, 1);

    // 8-Card Hardware & Emulation Profile Grid (2 rows x 4 columns)
    auto* grid_container = new QWidget(ui->center_card);
    grid_container->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    auto* profile_grid = new QGridLayout(grid_container);
    profile_grid->setContentsMargins(0, 0, 0, 0);
    profile_grid->setHorizontalSpacing(10);
    profile_grid->setVerticalSpacing(8);

    for (int i = 0; i < 8; ++i) {
        auto* card = new QFrame(grid_container);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background: rgba(15, 23, 42, 0.52); border: 1px solid rgba(148, 163, 184, 0.18); border-radius: 9px; }"));
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(11, 7, 11, 7);
        cl->setSpacing(2);

        auto* t_lbl = new QLabel(card);
        t_lbl->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 9.5px; font-weight: 800; letter-spacing: 0.4px; background: transparent; border: none;"));
        auto* v_lbl = new QLabel(card);
        v_lbl->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 11.5px; font-weight: 700; background: transparent; border: none;"));

        cl->addWidget(t_lbl);
        cl->addWidget(v_lbl);
        dash.card_titles[i] = t_lbl;
        dash.card_values[i] = v_lbl;
        profile_grid->addWidget(card, i / 4, i % 4);
        profile_grid->setColumnStretch(i % 4, 1);
    }

    // 4-Step Visual Pipeline Stepper Row
    auto* steps_container = new QWidget(ui->center_card);
    steps_container->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    auto* steps_layout = new QHBoxLayout(steps_container);
    steps_layout->setContentsMargins(0, 2, 0, 0);
    steps_layout->setSpacing(8);
    for (int i = 0; i < 4; ++i) {
        auto* sf = new QFrame(steps_container);
        auto* sl = new QHBoxLayout(sf);
        sl->setContentsMargins(10, 5, 10, 5);
        auto* st = new QLabel(sf);
        st->setAlignment(Qt::AlignCenter);
        sl->addWidget(st);
        dash.step_frames[i] = sf;
        dash.step_labels[i] = st;
        steps_layout->addWidget(sf, 1);
    }

    // Add live elapsed timer badge to stage_layout
    dash.timer_badge = new QLabel(ui->center_card);
    dash.timer_badge->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(0, 242, 254, 0.12); color: #00F2FE; border: 1px solid rgba(0, 242, 254, 0.32); border-radius: 5px; padding: 2px 8px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: 700; }"));
    dash.timer_badge->setText(QStringLiteral("⏱ 00:00.0"));
    ui->stage_layout->insertWidget(1, dash.timer_badge, 0, Qt::AlignRight | Qt::AlignVCenter);

    // Interactive Pro-Tips & Hotkeys Banner at the bottom of center_card
    auto* tip_frame = new QFrame(ui->center_card);
    tip_frame->setStyleSheet(QStringLiteral(
        "QFrame { background: rgba(15, 23, 42, 0.72); border: 1px solid rgba(56, 189, 248, 0.24); border-radius: 10px; }"));
    auto* tip_layout = new QHBoxLayout(tip_frame);
    tip_layout->setContentsMargins(12, 8, 12, 8);
    tip_layout->setSpacing(10);

    dash.tip_title_badge = new QLabel(tip_frame);
    dash.tip_title_badge->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(56, 189, 248, 0.16); color: #38BDF8; border: 1px solid rgba(56, 189, 248, 0.35); border-radius: 5px; padding: 3px 8px; font-size: 10px; font-weight: 800; }"));
    tip_layout->addWidget(dash.tip_title_badge, 0, Qt::AlignVCenter);

    dash.tip_text_label = new QLabel(tip_frame);
    dash.tip_text_label->setWordWrap(true);
    dash.tip_text_label->setTextFormat(Qt::RichText);
    dash.tip_text_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 11.5px; background: transparent; border: none;"));
    tip_layout->addWidget(dash.tip_text_label, 1);


    dash.next_tip_btn = new QPushButton(tip_frame);
    dash.next_tip_btn->setCursor(Qt::PointingHandCursor);
    dash.next_tip_btn->setMinimumHeight(26);
    dash.next_tip_btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(0, 210, 255, 0.15); color: #67E8F9; border: 1px solid rgba(0, 210, 255, 0.38); border-radius: 5px; padding: 3px 10px; font-size: 11px; font-weight: 700; }"
        "QPushButton:hover { background: rgba(0, 210, 255, 0.30); color: #FFFFFF; border: 1px solid #00E5FF; }"));

    connect(dash.next_tip_btn, &QPushButton::clicked, this, [this]() {
        auto it = s_dash_map.find(this);
        if (it == s_dash_map.end()) return;
        it->second.current_tip_index = (it->second.current_tip_index + 1) % GetTipCount();
        RefreshDashboardTexts(it->second);
    });

    tip_layout->addWidget(dash.next_tip_btn, 0, Qt::AlignVCenter);

    // Insert widgets into card_layout in clean vertical order:
    // 0: hero_frame
    // 1: grid_container
    // 2: steps_container
    // 3: progress_layout (already at index 0 after removals)
    // 4: tip_frame
    ui->card_layout->insertWidget(0, hero_frame);
    ui->card_layout->insertWidget(1, grid_container);
    ui->card_layout->insertWidget(2, steps_container);
    ui->card_layout->addWidget(tip_frame);

    // Live 100ms timer for elapsed boot time and tip rotation
    dash.live_tick_timer = new QTimer(this);
    dash.live_tick_timer->setInterval(100);
    connect(dash.live_tick_timer, &QTimer::timeout, this, [this]() {
        if (!isVisible()) {
            return;
        }
        auto it = s_dash_map.find(this);
        if (it == s_dash_map.end()) {
            return;
        }
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second.boot_start_time).count();
        const int mins = static_cast<int>(elapsed_ms / 60000);
        const int secs = static_cast<int>((elapsed_ms % 60000) / 1000);
        const int tenths = static_cast<int>((elapsed_ms % 1000) / 100);
        if (it->second.timer_badge) {
            it->second.timer_badge->setText(
                QStringLiteral("⏱ %1:%2.%3")
                    .arg(mins, 2, 10, QLatin1Char('0'))
                    .arg(secs, 2, 10, QLatin1Char('0'))
                    .arg(tenths));
        }
    });

    opacity_effect = new QGraphicsOpacityEffect(this);
    opacity_effect->setOpacity(1);
    ui->fade_parent->setGraphicsEffect(opacity_effect);
    fadeout_animation = std::make_unique<QPropertyAnimation>(opacity_effect, "opacity");
    fadeout_animation->setDuration(260);
    fadeout_animation->setStartValue(1);
    fadeout_animation->setEndValue(0);
    fadeout_animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(fadeout_animation.get(), &QPropertyAnimation::finished, [this] {
        auto it = s_dash_map.find(this);
        if (it != s_dash_map.end() && it->second.live_tick_timer) {
            it->second.live_tick_timer->stop();
        }
        hide();
        opacity_effect->setOpacity(1);
        emit Hidden();
    });

    progress_animation = std::make_unique<QPropertyAnimation>(ui->progress_bar, "value");
    progress_animation->setEasingCurve(QEasingCurve::OutQuad);

    watchdog_timer = new QTimer(this);
    watchdog_timer->setSingleShot(true);
    connect(watchdog_timer, &QTimer::timeout, this, [this] {
        if (isVisible()) {
            OnLoadComplete();
        }
    });

    connect(this, &LoadingScreen::LoadProgress, this, &LoadingScreen::OnLoadProgress,
            Qt::QueuedConnection);
    qRegisterMetaType<VideoCore::LoadCallbackStage>();

    RefreshDashboardTexts(dash);
}

LoadingScreen::~LoadingScreen() {
    s_dash_map.erase(this);
}

void LoadingScreen::SetGameInfo(const QString& name, const QString& version, const QString& dev,
                                u64 title_id, const QPixmap& icon, const QString& format) {
    if (!icon.isNull()) {
        ui->game_icon->setPixmap(icon.scaled(148, 148, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    ui->game_title->setMinimumHeight(96);
    ui->game_title->setMaximumHeight(124);
    ui->game_title->setWordWrap(true);
    ui->game_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    const QString display_name = name.isEmpty()
        ? StormLang(QStringLiteral("Запуск игры..."), QStringLiteral("Launching Game..."), QStringLiteral("Spiel wird gestartet..."), QStringLiteral("Lancement du jeu..."), QStringLiteral("正在启动游戏..."), QStringLiteral("ゲームを起動中..."), QStringLiteral("جاري تشغيل اللعبة..."), QStringLiteral("Iniciando juego..."), QStringLiteral("Avvio gioco..."), QStringLiteral("Iniciando jogo..."), QStringLiteral("게임 실행 중..."))
        : name;
    if (display_name.length() > 62) {
        ui->game_title->setStyleSheet(QStringLiteral(
            "color: #FFFFFF; font-size: 14pt; font-weight: 800; padding: 0px; "
            "background: transparent; border: none;"));
    } else if (display_name.length() > 36) {
        ui->game_title->setStyleSheet(QStringLiteral(
            "color: #FFFFFF; font-size: 15.5pt; font-weight: 800; padding: 0px; "
            "background: transparent; border: none;"));
    } else {
        ui->game_title->setStyleSheet(QStringLiteral(
            "color: #FFFFFF; font-size: 17.5pt; font-weight: 800; padding: 0px; "
            "background: transparent; border: none;"));
    }
    ui->game_title->setText(display_name);

    QStringList game_info;
    if (!version.isEmpty()) {
        const QString ver_lbl = StormLang(QStringLiteral("Версия:"), QStringLiteral("Version:"), QStringLiteral("Version:"), QStringLiteral("Version :"), QStringLiteral("版本:"), QStringLiteral("バージョン:"), QStringLiteral("الإصدار:"), QStringLiteral("Versión:"), QStringLiteral("Versione:"), QStringLiteral("Versão:"), QStringLiteral("버전:"));
        game_info << QStringLiteral("<span style='color:#FFFFFF;'>🏷️ %1</span> <span style='color:#FFFFFF; font-weight:700;'>%2</span>").arg(ver_lbl, version);
    }
    if (!dev.isEmpty()) {
        const QString dev_lbl = StormLang(QStringLiteral("Разработчик:"), QStringLiteral("Developer:"), QStringLiteral("Entwickler:"), QStringLiteral("Développeur :"), QStringLiteral("开发商:"), QStringLiteral("開発元:"), QStringLiteral("المطور:"), QStringLiteral("Desarrollador:"), QStringLiteral("Sviluppatore:"), QStringLiteral("Desenvolvedor:"), QStringLiteral("개발사:"));
        game_info << QStringLiteral("<span style='color:#FFFFFF;'>🏢 %1</span> <span style='color:#FFFFFF; font-weight:600;'>%2</span>").arg(dev_lbl, dev);
    }
    if (!format.isEmpty()) {
        const QString fmt_lbl = StormLang(QStringLiteral("Контейнер:"), QStringLiteral("Container:"), QStringLiteral("Format:"), QStringLiteral("Format :"), QStringLiteral("封装格式:"), QStringLiteral("フォーマット:"), QStringLiteral("الصيغة:"), QStringLiteral("Formato:"), QStringLiteral("Formato:"), QStringLiteral("Formato:"), QStringLiteral("포맷:"));
        game_info << QStringLiteral("<span style='color:#FFFFFF;'>📦 %1</span> <span style='color:#38bdf8; font-weight:700;'>%2</span>").arg(fmt_lbl, format.toUpper());
    }

    ui->game_meta->setText(game_info.join(QStringLiteral("&nbsp;&nbsp;&nbsp;·&nbsp;&nbsp;&nbsp;")));

    auto& dash = s_dash_map[this];
    if (title_id != 0) {
        dash.current_tid_hex = QString::number(title_id, 16).toUpper().rightJustified(16, QLatin1Char('0'));
    } else {
        dash.current_tid_hex.clear();
    }
    dash.boot_start_time = std::chrono::steady_clock::now();
    dash.active_step = 1;
    RefreshDashboardTexts(dash);
    if (dash.live_tick_timer && !dash.live_tick_timer->isActive()) {
        dash.live_tick_timer->start();
    }
}

void LoadingScreen::Prepare(Loader::AppLoader& loader) {
    std::vector<u8> buffer;
    if (loader.ReadBanner(buffer) == Loader::ResultStatus::Success) {
#ifdef YUZU_QT_MOVIE_MISSING
        QPixmap map;
        map.loadFromData(buffer.data(), static_cast<uint>(buffer.size()));
        ui->banner->setPixmap(map);
#else
        backing_mem = std::make_unique<QByteArray>(reinterpret_cast<char*>(buffer.data()),
                                                   static_cast<int>(buffer.size()));
        backing_buf = std::make_unique<QBuffer>(backing_mem.get());
        backing_buf->open(QIODevice::ReadOnly);
        animation = std::make_unique<QMovie>(backing_buf.get(), QByteArray());
        animation->start();
        ui->banner->setMovie(animation.get());
#endif
        buffer.clear();
    }
    if (loader.ReadLogo(buffer) == Loader::ResultStatus::Success) {
        QPixmap map;
        map.loadFromData(buffer.data(), static_cast<uint>(buffer.size()));
        ui->logo->setPixmap(map.scaled(120, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        buffer.clear();
    }

    if (ui->game_icon->pixmap().isNull()) {
        std::vector<u8> icon_bytes;
        if (loader.ReadIcon(icon_bytes) == Loader::ResultStatus::Success) {
            QPixmap map;
            map.loadFromData(icon_bytes.data(), static_cast<uint>(icon_bytes.size()));
            ui->game_icon->setPixmap(map.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    load_completed = false;
    previous_total = 0;
    slow_shader_compile_start = false;
    if (watchdog_timer) {
        watchdog_timer->stop();
    }
    if (progress_animation) {
        progress_animation->stop();
    }
    opacity_effect->setOpacity(1.0);
    auto& dash = s_dash_map[this];
    dash.boot_start_time = std::chrono::steady_clock::now();
    if (dash.live_tick_timer) {
        dash.live_tick_timer->start();
    }
    OnLoadProgress(VideoCore::LoadCallbackStage::Prepare, 0, 0);
}

void LoadingScreen::OnLoadComplete() {
    if (watchdog_timer) {
        watchdog_timer->stop();
    }
    auto it = s_dash_map.find(this);
    if (it != s_dash_map.end() && it->second.live_tick_timer) {
        it->second.live_tick_timer->stop();
    }
    if (!isVisible()) {
        return;
    }
    if (fadeout_animation && fadeout_animation->state() != QAbstractAnimation::Running) {
        fadeout_animation->start(QPropertyAnimation::KeepWhenStopped);
        QTimer::singleShot(fadeout_animation->duration() + 50, this, [this] {
            if (isVisible()) {
                hide();
                opacity_effect->setOpacity(1);
                emit Hidden();
            }
        });
    } else if (!fadeout_animation) {
        hide();
        emit Hidden();
    }
}

void LoadingScreen::ShowShutdownState() {
    if (watchdog_timer) {
        watchdog_timer->stop();
    }
    if (progress_animation) {
        progress_animation->stop();
    }
    fadeout_animation->stop();
    opacity_effect->setOpacity(1.0);
    ui->stage->setText(StormLang(
        QStringLiteral("ЗАВЕРШЕНИЕ ЭМУЛЯЦИИ И СОХРАНЕНИЕ ДАННЫХ..."),
        QStringLiteral("SHUTTING DOWN EMULATION & SAVING DATA..."),
        QStringLiteral("EMULATION WIRD BEENDET & DATEN WERDEN GESPEICHERT..."),
        QStringLiteral("ARRÊT DE L'ÉMULATION ET SAUVEGARDE DES DONNÉES..."),
        QStringLiteral("正在停止模拟并安全保存数据..."),
        QStringLiteral("エミュレーションを終了しデータを保存しています..."),
        QStringLiteral("جاري إيقاف المحاكاة وحفظ البيانات..."),
        QStringLiteral("CERRANDO EMULACIÓN Y GUARDANDO DATOS..."),
        QStringLiteral("ARRESTO EMULAZIONE E SALVATAGGIO DATI..."),
        QStringLiteral("ENCERRANDO EMULAÇÃO E SALVANDO DADOS..."),
        QStringLiteral("에뮬레이션 종료 및 데이터 저장 중...")));
    ui->value->setText(QString{});
    ui->progress_bar->setRange(0, 0);
    show();
    raise();
}

void LoadingScreen::OnLoadProgress(VideoCore::LoadCallbackStage stage, std::size_t value,
                                   std::size_t total) {
    using namespace std::chrono;
    const auto now = steady_clock::now();

    if (load_completed && stage != VideoCore::LoadCallbackStage::Prepare) {
        return;
    }

    stage_translations = {
        {VideoCore::LoadCallbackStage::Prepare, StormLang(
            QStringLiteral("ПОДГОТОВКА СИСТЕМЫ И ФАЙЛОВ ИГРЫ..."),
            QStringLiteral("PREPARING SYSTEM & GAME FILESYSTEM..."),
            QStringLiteral("SYSTEM & SPIELDATEIEN WERDEN VORBEREITET..."),
            QStringLiteral("PRÉPARATION DU SYSTÈME ET DES FICHIERS..."),
            QStringLiteral("正在准备系统与游戏虚拟文件系统..."),
            QStringLiteral("システムとゲームファイルを準備中..."),
            QStringLiteral("جاري تحضير النظام وملفات اللعبة..."),
            QStringLiteral("PREPARANDO SISTEMA Y ARCHIVOS DEL JUEGO..."),
            QStringLiteral("PREPARAZIONE SISTEMA E FILE DI GIOCO..."),
            QStringLiteral("PREPARANDO SISTEMA E ARQUIVOS DO JOGO..."),
            QStringLiteral("시스템 및 게임 파일 준비 중..."))},
        {VideoCore::LoadCallbackStage::Build, StormLang(
            QStringLiteral("КОМПИЛЯЦИЯ КОНВЕЙЕРОВ ШЕЙДЕРОВ: %1 / %2 (%3%)"),
            QStringLiteral("COMPILING SHADER PIPELINES: %1 / %2 (%3%)"),
            QStringLiteral("SHADER-PIPELINES KOMPILIEREN: %1 / %2 (%3%)"),
            QStringLiteral("COMPILATION DES SHADERS : %1 / %2 (%3%)"),
            QStringLiteral("正在编译着色器管线: %1 / %2 (%3%)"),
            QStringLiteral("シェーダーパイプラインをコンパイル中: %1 / %2 (%3%)"),
            QStringLiteral("تجميع أنابيب المظللات: %1 / %2 (%3%)"),
            QStringLiteral("COMPILANDO PIPELINES DE SHADERS: %1 / %2 (%3%)"),
            QStringLiteral("COMPILAZIONE PIPELINE SHADER: %1 / %2 (%3%)"),
            QStringLiteral("COMPILANDO PIPELINES DE SHADERS: %1 / %2 (%3%)"),
            QStringLiteral("셰이더 파이프라인 컴파일 중: %1 / %2 (%3%)"))},
        {VideoCore::LoadCallbackStage::Complete, StormLang(
            QStringLiteral("ЗАПУСК ИГРОВОГО ПРОЦЕССА И ГРАФИЧЕСКОГО ЯДРА..."),
            QStringLiteral("LAUNCHING GAME PROCESS & GRAPHICS ENGINE..."),
            QStringLiteral("SPIELPROZESS & GRAFIK-ENGINE WERDEN GESTARTET..."),
            QStringLiteral("LANCEMENT DU JEU ET DU MOTEUR GRAPHIQUE..."),
            QStringLiteral("正在启动游戏进程与图形渲染核心..."),
            QStringLiteral("ゲームプロセスとグラフィックスエンジンを起動中..."),
            QStringLiteral("جاري إطلاق عملية اللعبة ومحرك الرسومات..."),
            QStringLiteral("INICIANDO PROCESO DEL JUEGO Y MOTOR GRÁFICO..."),
            QStringLiteral("AVVIO PROCESSO DI GIOCO E MOTORE GRAFICO..."),
            QStringLiteral("INICIANDO PROCESSO DO JOGO E MOTOR GRÁFICO..."),
            QStringLiteral("게임 프로세스 및 그래픽 엔진 시작 중..."))},
    };

    auto& dash = s_dash_map[this];

    if (stage != previous_stage) {
        previous_stage = stage;
        slow_shader_compile_start = false;
    }

    if (stage == VideoCore::LoadCallbackStage::Prepare) {
        load_completed = false;
        if (progress_animation) {
            progress_animation->stop();
        }
        if (watchdog_timer) {
            watchdog_timer->stop();
        }
        UpdateStepStyles(dash, 1);
        ui->progress_bar->setRange(0, 0);
        ui->stage->setText(stage_translations[stage]);
        ui->value->setText(QString{});
    } else if (stage == VideoCore::LoadCallbackStage::Build) {
        ui->progress_bar->setRange(0, 1000);
        UpdateStepStyles(dash, 2);

        if (total > 0 && value >= total) {
            if (progress_animation) {
                progress_animation->stop();
            }
            ui->progress_bar->setValue(1000);
            UpdateStepStyles(dash, 3);
            ui->stage->setText(stage_translations[VideoCore::LoadCallbackStage::Complete]);
            ui->value->setText(QString{});
        } else {
            const int target_value = (total > 0)
                ? static_cast<int>(std::clamp((static_cast<double>(value) / static_cast<double>(total)) * 1000.0, 0.0, 1000.0))
                : 1000;
            const int pct = target_value / 10;

            const int current_value = ui->progress_bar->value();
            if (target_value > current_value) {
                if (progress_animation) {
                    progress_animation->stop();
                    progress_animation->setStartValue(current_value);
                    progress_animation->setEndValue(target_value);
                    progress_animation->setDuration(120);
                    progress_animation->start();
                } else {
                    ui->progress_bar->setValue(target_value);
                }
            } else {
                ui->progress_bar->setValue(target_value);
            }

            ui->stage->setText(stage_translations[stage].arg(value).arg(total).arg(pct));

            QString estimate;
            if (now - previous_time > milliseconds{50} || slow_shader_compile_start) {
                if (!slow_shader_compile_start) {
                    slow_shader_start = steady_clock::now();
                    slow_shader_compile_start = true;
                    slow_shader_first_value = value;
                }
                const auto diff = duration_cast<milliseconds>(now - slow_shader_start);
                if (diff > milliseconds{600} && (value > slow_shader_first_value) && total > value) {
                    const double shaders_done = static_cast<double>(value - slow_shader_first_value);
                    const double secs_elapsed = std::max(0.1, static_cast<double>(diff.count()) / 1000.0);
                    const int rate_per_sec = std::max(1, static_cast<int>(shaders_done / secs_elapsed));
                    const auto eta_mseconds =
                        static_cast<long>(static_cast<double>(total - slow_shader_first_value) /
                                          shaders_done * diff.count());
                    const QString eta_str = QTime(0, 0, 0, 0)
                        .addMSecs(std::max<long>(eta_mseconds - diff.count() + 1000, 1000))
                        .toString(QStringLiteral("mm:ss"));
                    estimate = StormLang(
                        QStringLiteral("⚡ %1 шейд./с · Осталось: %2"),
                        QStringLiteral("⚡ %1 sh/s · ETA: %2"),
                        QStringLiteral("⚡ %1 Sh./s · Restzeit: %2"),
                        QStringLiteral("⚡ %1 sh/s · Restant : %2"),
                        QStringLiteral("⚡ %1 个/秒 · 剩余: %2"),
                        QStringLiteral("⚡ %1 sh/秒 · 残り: %2"),
                        QStringLiteral("⚡ %1 مظلل/ث · المتبقي: %2"),
                        QStringLiteral("⚡ %1 sh/s · Restante: %2"),
                        QStringLiteral("⚡ %1 sh/s · Rimanente: %2"),
                        QStringLiteral("⚡ %1 sh/s · Restante: %2"),
                        QStringLiteral("⚡ %1 sh/초 · 남은 시간: %2")).arg(rate_per_sec).arg(eta_str);
                }
            }
            ui->value->setText(estimate);
        }
    } else if (stage == VideoCore::LoadCallbackStage::Complete) {
        load_completed = true;
        if (progress_animation) {
            progress_animation->stop();
        }
        UpdateStepStyles(dash, 3);
        ui->progress_bar->setRange(0, 1000);
        ui->progress_bar->setValue(1000);

        ui->stage->setText(stage_translations[stage]);
        ui->value->setText(QString{});

        if (watchdog_timer) {
            watchdog_timer->start(45000);
        }
    }

    previous_time = now;
}

void LoadingScreen::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void LoadingScreen::mousePressEvent(QMouseEvent* event) {
    if (load_completed) {
        OnLoadComplete();
    }
    QWidget::mousePressEvent(event);
}

void LoadingScreen::keyPressEvent(QKeyEvent* event) {
    if (load_completed) {
        OnLoadComplete();
    }
    QWidget::keyPressEvent(event);
}

void LoadingScreen::Clear() {
#ifndef YUZU_QT_MOVIE_MISSING
    animation.reset();
    backing_buf.reset();
    backing_mem.reset();
#endif
    if (watchdog_timer) {
        watchdog_timer->stop();
    }
    auto it = s_dash_map.find(this);
    if (it != s_dash_map.end() && it->second.live_tick_timer) {
        it->second.live_tick_timer->stop();
    }
    if (progress_animation) {
        progress_animation->stop();
    }
    load_completed = false;
    ui->game_icon->clear();
    ui->game_title->clear();
    ui->game_meta->clear();
}
