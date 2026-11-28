// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 Torzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shared_translation.h"

#include <map>
#include <memory>
#include <utility>
#include <QCoreApplication>
#include "common/settings.h"
#include "common/settings_enums.h"
#include "common/settings_setting.h"
#include "common/time_zone.h"
#include "qt_common/config/uisettings.h"

namespace ConfigurationShared {

static QString TranslateConfigText(const char* text, const char* disambiguation = nullptr) {
    if (!text || text[0] == '\0') {
        return QString();
    }
    std::string cur_lang = UISettings::values.language.GetValue();
    if (cur_lang.empty()) {
        cur_lang = QLocale::system().name().toStdString();
    }
    if (cur_lang == "en") {
        return QString::fromUtf8(text);
    }

    struct LangItem {
        const char* key = nullptr;
        const char* ru = nullptr;
        const char* ar = nullptr;
        const char* de = nullptr;
        const char* fr = nullptr;
        const char* zh = nullptr;
        const char* ja = nullptr;
        const char* es = nullptr;
        const char* ko = nullptr;
        const char* it = nullptr;
        const char* pt = nullptr;
        const char* hi = nullptr;
    };

    static const LangItem s_table[] = {
        {"Save States Mode:", "Режим быстрых сохранений:", "وضع الحفظ السريع:", "Schnellspeicher-Modus:", "Mode de sauvegardes rapides :", "快速保存模式:", "クイックセーブモード:", "Modo de guardados rápidos:", "빠른 저장 모드:", "Modalità salvataggi rapidi:", "Modo de salvamentos rápidos:", "क्विक सेव मोड:"},
        {"Save States Mode", "Режим быстрых сохранений", "وضع الحفظ السريع", "Schnellspeicher-Modus", "Mode de sauvegardes rapides", "快速保存模式", "クイックセーブモード", "Modo de guardados rápidos", "빠른 저장 모드", "Modalità salvataggi rapidi", "Modo de salvamentos rápidos", "क्विक सेव मोड"},
        {"Controls quick save and quick load behavior. Guest RAM/CPU-only save states can crash unverified games.", "Управляет поведением быстрых сохранений и загрузок. Сохранения только оперативной памяти и ЦП могут приводить к зависанию непроверенных игр.", "يتحكم في سلوك الحفظ والتحميل السريع. قد يؤدي حفظ الذاكرة والمعالج فقط إلى تعطل الألعاب غير المعتمدة.", "Steuert das Verhalten von Schnellspeichern und Schnellladen. Reine RAM/CPU-Zustände können ungetestete Spiele abstürzen lassen.", "Contrôle le comportement de sauvegarde et chargement rapides. Les sauvegardes mémoire/CPU seules peuvent planter les jeux non vérifiés.", "控制快速保存与快速读取行为。仅保存客户机内存与CPU寄存器的状态可能导致未经测试的游戏卡死或崩溃。", "クイックセーブ・クイックロードの挙動を設定します。ゲストRAM/CPUのみのステート保存は未検証タイトルでクラッシュやフリーズの原因となる場合があります。", "Controla el comportamiento del guardado y carga rápidos. Los guardados de solo RAM/CPU de invitado pueden bloquear juegos no verificados.", "빠른 저장 및 빠른 불러오기 동작을 제어합니다. 게스트 RAM/CPU 전용 상태 저장은 검증되지 않은 게임에서 충돌을 일으킬 수 있습니다.", "Controlla il comportamento di salvataggio e caricamento rapido. I salvataggi solo RAM/CPU possono bloccare i giochi non verificati.", "Controla o comportamento de salvamento e carregamento rápido. Estados apenas de RAM/CPU podem travar jogos não verificados.", "क्विक सेव और क्विक लोड व्यवहार को नियंत्रित करता है। केवल गेस्ट RAM/CPU सेव स्टेट्स असत्यापित गेम्स को क्रैश कर सकते हैं।"},
        {"Verified games only", "Только проверенные игры", "الألعاب المعتمدة فقط", "Nur verifizierte Spiele", "Jeux vérifiés uniquement", "仅限已验证游戏", "検証済みゲームのみ", "Solo juegos verificados", "검증된 게임만", "Solo giochi verificati", "Apenas jogos verificados", "केवल सत्यापित गेम्स"},
        {"All games (experimental)", "Все игры (экспериментально)", "جميع الألعاب (تجريبي)", "Alle Spiele (experimentell)", "Tous les jeux (expérimental)", "所有游戏 (实验性)", "すべてのゲーム (実験的)", "Todos los juegos (experimental)", "모든 게임 (실험적)", "Tutti i giochi (sperimentale)", "Todos os jogos (experimental)", "सभी गेम्स (प्रायोगिक)"},
        {"Quick Save (Experimental)", "Быстрое сохранение (эксперимент.)", "حفظ سريع (تجريبي)", "Schnellspeichern (experimentell)", "Sauvegarde rapide (expérimental)", "快速保存 (实验性)", "クイックセーブ (実験的)", "Guardado rápido (experimental)", "빠른 저장 (실험적)", "Salvataggio rapido (sperimentale)", "Salvamento rápido (experimental)", "क्विक सेव (प्रायोगिक)"},
        {"Quick Load (Experimental)", "Быстрая загрузка (эксперимент.)", "تحميل سريع (تجريبي)", "Schnellladen (experimentell)", "Chargement rapide (expérimental)", "快速读取 (实验性)", "クイックロード (実験的)", "Carga rápida (experimental)", "빠른 불러오기 (실험적)", "Caricamento rapido (sperimentale)", "Carregamento rápido (experimental)", "क्विक लोड (प्रायोगिक)"},
        {"CPU Clocks", "Частоты ЦП", "ترددات المعالج", "CPU-Takte", "Fréquences CPU", "CPU 时钟", "CPU クロック", "Frecuencias CPU", "CPU 클럭", "Frequenze CPU", "Frequências da CPU", "CPU क्लॉक"},
        {"GPU Clocks", "Частоты ГПУ", "ترددات معالج الرسوميات", "GPU-Takte", "Fréquences GPU", "GPU 时钟", "GPU クロック", "Frecuencias GPU", "GPU 클럭", "Frequenze GPU", "Frequências da GPU", "GPU क्लॉक"},
        {"Eco Thermal Mode", "Эко-термальный режим", "الوضع الحراري الاقتصادي", "Eco-Thermal-Modus", "Mode thermique éco", "环保节能模式", "エコ熱管理モード", "Modo térmico ecológico", "에코 발열 제어 모드", "Modalità termica Eco", "Modo térmico ecológico", "इको थर्मल मोड"},
        {"Low-End Turbo", "Турбо для слабых ПК", "وضع التوربو للأجهزة الضعيفة", "Low-End-Turbo", "Turbo pour machines modestes", "低端设备加速", "低スペック高速化", "Turbo para gama baja", "저사양 PC 터보", "Turbo per PC di fascia bassa", "Turbo para PCs fracos", "लो-एंड टर्बो"},
        {"Thermal Governor", "Термоконтроллер", "متحكم الحرارة", "Thermal Governor", "Régulateur thermique", "温度调节器", "温度ガバナー", "Regulador térmico", "발열 제어기", "Regolatore termico", "Controlador térmico", "थर्मल गवर्नर"},
        {"Limit Speed Percent", "Ограничение скорости в процентах", "نسبة تحديد السرعة", "Geschwindigkeitsbegrenzung in Prozent", "Pourcentage de limite de vitesse", "运行速度百分比限制", "速度制限パーセント", "Límite de velocidad en porcentaje", "속도 제한 비율", "Percentuale di limite della velocità", "Percentagem do limitador de velocidade", "गति प्रतिशत सीमित करें"},
        {"Turbo Speed", "Турбо скорость", "سرعة التوربو", "Turbo-Geschwindigkeit", "Vitesse Turbo", "加速速度", "ターボ速度", "Velocidad turbo", "터보 속도", "Modalità Turbo", "Velocidade Turbo", "टर्बो गति"},
        {"Slow Speed", "Замедленная скорость", "السرعة البطيئة", "Verlangsamte Geschwindigkeit", "Vitesse lente", "减速速度", "低速", "Velocidad lenta", "느린 속도", "Modalità Lenta", "Velocidade Reduzida", "धीमी गति"},
        {"Synchronize Core Speed", "Синхронизировать скорость ядра", "مزامنة سرعة النواة", "Kerngeschwindigkeit synchronisieren", "Synchroniser la vitesse du cœur", "同步核心速度", "コア速度を同期", "Sincronizar velocidad del núcleo", "코어 속도 동기화", "Sincronizza velocità core", "Sincronizar velocidade do núcleo", "कोर गति सिंक्रनाइज़ करें"},
        {"Multicore CPU Emulation", "Многоядерная эмуляция ЦП", "محاكاة المعالج متعدد النواة", "Mehrkern-CPU-Emulation", "Émulation CPU multicœur", "多核 CPU 模拟", "マルチコア CPU エミュレーション", "Emulación de CPU multinúcleo", "다중코어 CPU 에뮬레이션", "Emulazione CPU multi-core", "Emulação de CPU Multicore", "मल्टीकोर CPU पायदान एमुलेशन"},
        {"Memory Layout", "Схема памяти (DRAM)", "مخطط الذاكرة (DRAM)", "Speicher-Layout (DRAM)", "Disposition de la mémoire (DRAM)", "内存布局 (DRAM)", "メモリレイアウト (DRAM)", "Distribución de memoria (DRAM)", "메모리 레이아웃", "Layout di memoria", "Layout de memória", "मेमोरी लेआउट (DRAM)"},
        {"Memory Layout (DRAM)", "Схема памяти (DRAM)", "مخطط الذاكرة (DRAM)", "Speicher-Layout (DRAM)", "Disposition de la mémoire (DRAM)", "内存布局 (DRAM)", "メモリレイアウト (DRAM)", "Distribución de memoria (DRAM)", "메모리 구성 (DRAM)", "Layout memoria (DRAM)", "Layout de memória (DRAM)", "मेमोरी लेआउट (DRAM)"},
        {"Custom RTC Date:", "Пользовательское время RTC:", "تاريخ RTC مخصص:", "Benutzerdefinierte RTC-Zeit:", "Date RTC personnalisée :", "自定义 RTC 时间:", "カスタム RTC 日時:", "Fecha RTC personalizada:", "사용자 지정 RTC 날짜:", "Data RTC personalizzata:", "Data personalizada do RTC:", "कस्टम RTC तिथि:"},
        {"Custom RTC Offset:", "Смещение RTC:", "إزاحة RTC:", "RTC-Offset:", "Décalage RTC :", "RTC 偏移量:", "RTC オフセット:", "Compensación RTC:", "사용자 지정 RTC 오프셋:", "Offset RTC personalizzato:", "Deslocamento RTC personalizado:", "कस्टम RTC ऑफ़सेट:"},
        {"Custom RTC Offset", "Смещение RTC", "إزاحة RTC", "RTC-Offset", "Décalage RTC", "RTC 偏移量", "RTC オフセット", "Compensación RTC", "사용자 지정 RTC 오프셋", "Offset RTC personalizzato", "Deslocamento RTC personalizado", "कस्टम RTC ऑफ़सेट"},
        {"Custom RTC", "Пользовательское время RTC", "تاريخ RTC مخصص", "Benutzerdefinierte RTC-Zeit", "Date RTC personnalisée", "自定义 RTC 时间", "カスタム RTC 日時", "Fecha RTC personalizada", "사용자 지정 RTC", "RTC personalizzato", "RTC personalizado", "कस्टम RTC"},
        {"Custom RNG Seed", "Пользовательский сид RNG", "بذرة عشوائية مخصصة (RNG)", "Benutzerdefinierter RNG-Seed", "Graine RNG personnalisée", "自定义随机数种子", "カスタム RNG シード", "Semilla RNG personalizada", "사용자 지정 RNG 시드", "Seed RNG personalizzato", "Semente RNG personalizada", "कस्टम RNG सीड"},
        {"Accuracy:", "Точность:", "الدقة:", "Genauigkeit:", "Précision :", "精确度:", "精度:", "Precisión:", "정확도:", "Precisione:", "Precisão:", "सटीकता:"},
        {"Accurate", "Точный", "دقيق", "Präzise", "Précis", "高精度", "正確", "Preciso", "정확", "Accurata", "Preciso", "सटीक"},
        {"Auto", "Авто", "تلقائي", "Auto", "Auto", "自动", "自動", "Automático", "자동", "Automatico", "Automático", "स्वचालित"},
        {"Unsafe", "Небезопасно", "غير آمن", "Unsicher", "Non sécurisé", "不安全", "非安全", "Inseguro", "불안정", "Non sicuro", "Não seguro", "असुरक्षित"},
        {"Unsafe (fast)", "Небезопасно (быстро)", "غير آمن (سريع)", "Unsicher (schnell)", "Non sécurisé (rapide)", "不安全 (极速)", "非安全 (高速)", "Inseguro (rápido)", "불안전(빠름)", "Non sicuro (veloce)", "Inseguro (rápido)", "असुरक्षित (तेज़)"},
        {"Safe (stable)", "Безопасно (стабильно)", "آمن (مستقر)", "Sicher (stabil)", "Sécurisé (stable)", "安全 (稳定)", "安全 (安定)", "Seguro (estable)", "안전(안정적)", "Sicuro (stabile)", "Seguro (estável)", "सुरक्षित (स्थिर)"},
        {"Paranoid (disables most optimizations)", "Параноидальный (отключает оптимизации)", "مفرط في الدقة (يعطل التحسينات)", "Paranoid (deaktiviert die meisten Optimierungen)", "Paranoïaque (désactive la plupart des optimisations)", "极致安全 (禁用大部分优化)", "パラノイド (大半の最適化を無効化)", "Paranoico (desactiva la mayoría de optimizaciones)", "편집증적(대부분의 최적화를 비활성화함)", "Paranoica (disabilita la maggior parte delle ottimizzazioni)", "Paranoia (desativa a maioria das otimizações)", "अति-सुरक्षित (अधिकांश अनुकूलन अक्षम)"},
        {"Debugging", "Отладка", "تصحيح الأخطاء", "Debugging", "Débogage", "调试", "デバッグ", "Depuración", "디버깅", "Debug", "Depuração", "डीबगिंग"},
        {"Custom CPU Ticks", "Пользовательские такты ЦП", "ترددات مخصصة للمعالج", "Benutzerdefinierte CPU-Ticks", "Ticks CPU personnalisés", "自定义 CPU 时钟节拍", "カスタム CPU ティック", "Ticks de CPU personalizados", "사용자 지정 CPU 틱", "Tick CPU personalizzati", "Ciclos de CPU Personalizados", "कस्टम CPU Ticks"},
        {"CPU Affinity Pinning", "Привязка потоков ЦП", "تثبيت أنوية المعالج", "CPU-Affinity Pinning", "Affinité des cœurs CPU", "CPU 核心亲和性绑定", "CPU アフィニティ固定", "Fijación de afinidad de CPU", "CPU 코어 선호도 고정", "Affinità core CPU", "Fixação de afinidade da CPU", "CPU कोर एफिनिटी पिनिंग"},
        {"STORM Tier-2 JIT Compiler", "Компилятор STORM Tier-2 (Суперблок JIT)", "مترجم STORM Tier-2 JIT", "STORM Tier-2 JIT-Compiler", "Compilateur JIT STORM Tier-2", "STORM Tier-2 JIT 编译器", "STORM Tier-2 JIT コンパイラ", "Compilador JIT STORM Tier-2", "STORM Tier-2 JIT 컴파일러", "Compilatore JIT STORM Tier-2", "Compilador JIT STORM Tier-2", "STORM Tier-2 JIT कंपाइलर"},
        {"Enables the zero-overhead STORM Tier-2 instruction pre-optimizer and superblock JIT tuner (canonicalizes ARMv8.3 PAC/BTI hints and accelerates CNTVCT_EL0 counter reads).", "Включает оптимизирующий компилятор STORM Tier-2 без накладных расходов (преобразует инструкции ARMv8.3 PAC/BTI в суперблоки и ускоряет чтение таймеров CNTVCT_EL0).", "يفعل محسن تعليمات STORM Tier-2 ومضبط JIT الفائق بدون حمل إضافي.", "Aktiviert den verlustfreien STORM Tier-2 Befehls-Voroptimierer und Superblock-JIT-Tuner.", "Active le pré-optimiseur d'instructions STORM Tier-2 sans surcoût et le réglage JIT superbloc.", "启用零开销 STORM Tier-2 指令预优化器与超块 JIT 调优器。", "ゼロオーバーヘッドの STORM Tier-2 命令プリオプティマイザおよびスーパーブロック JIT チューナーを有効にします。", "Habilita el preoptimizador de instrucciones STORM Tier-2 sin sobrecarga y el ajustador JIT de superbloques.", "오버헤드 없는 STORM Tier-2 명령어 사전 최적화기 및 슈퍼블록 JIT 튜너를 활성화합니다 (ARMv8.3 PAC/BTI 정규화 및 CNTVCT_EL0 카운터 가속).", "Abilita il pre-ottimizzatore di istruzioni STORM Tier-2 a zero overhead e il tuner JIT superblock (accelera ARMv8.3 PAC/BTI e letture CNTVCT_EL0).", "Ativa o pré-otimizador de instruções STORM Tier-2 sem sobrecarga e o sintonizador JIT superbloco (acelera ARMv8.3 PAC/BTI e leituras CNTVCT_EL0).", "शून्य-ओवरहेड STORM Tier-2 निर्देश प्री-ऑप्टिमाइज़र और सुपरब्लॉक JIT ट्यूनर सक्षम करता है (ARMv8.3 PAC/BTI और CNTVCT_EL0 टाइमर पठन को तेज़ करता है)।"},
        {"Enable Frame Interpolation", "Интерполяция кадров (генерация промежуточных кадров)", "توليد الإطارات البينية (LSFG)", "Frame-Interpolation aktivieren", "Activer l'interpolation d'images", "启用帧插值 (中间帧生成)", "フレーム補間を有効化 (中間フレーム生成)", "Habilitar interpolación de fotogramas", "프레임 보간 활성화 (중간 프레임 생성)", "Abilita interpolazione fotogrammi", "Ativar interpolação de quadros", "फ़्रेम इंटरपोलेशन सक्षम करें (मध्यवर्ती फ़्रेम निर्माण)"},
        {"Generates intermediate frames using Lossless Scaling / LSFG temporal interpolation.", "Генерирует промежуточные кадры с помощью временной интерполяции Lossless Scaling / LSFG для повышения плавности.", "توليد إطارات وسيطة باستخدام تقنية الاستيفاء الزمني Lossless Scaling / LSFG لزيادة السلاسة.", "Erzeugt Zwischenbilder mittels temporaler Interpolation (Lossless Scaling / LSFG) für flüssigeres Gameplay.", "Génère des images intermédiaires via l'interpolation temporelle Lossless Scaling / LSFG pour fluidifier le rendu.", "使用 Lossless Scaling / LSFG 时间插值算法生成中间帧，大幅提升画面流畅度。", "Lossless Scaling / LSFG 時間補間を使用して中間フレームを生成し、映像の滑らかさを向上させます。", "Genera fotogramas intermedios mediante interpolación temporal Lossless Scaling / LSFG para mayor fluidez.", "Lossless Scaling / LSFG 시간적 보간을 사용하여 중간 프레임을 생성하고 부드러운 화면을 제공합니다.", "Genera fotogrammi intermedi utilizzando l'interpolazione temporale Lossless Scaling / LSFG per una maggiore fluidità.", "Gera quadros intermediários usando interpolação temporal Lossless Scaling / LSFG para maior fluidez.", "स्मूथ गेमप्ले के लिए Lossless Scaling / LSFG टेम्पोरल इंटरपोलेशन का उपयोग करके मध्यवर्ती फ़्रेम बनाता है।"},
        {"Pipeline Compilation Threads", "Потоки компиляции шейдеров", "خيوط تجميع خطوط الأنابيب", "Pipeline-Kompilierungs-Threads", "Threads de compilation des pipelines", "管线编译线程数", "パイプラインコンパイルスレッド", "Hilos de compilación de tuberías", "파이프라인 컴파일 스레드", "Thread di compilazione pipeline", "Threads de compilação de pipeline", "पाइपलाइन संकलन थ्रेड्स"},
        {"Number of background worker threads for Vulkan pipeline compilation (0 = Auto).", "Количество фоновых рабочих потоков для компиляции конвейеров Vulkan (0 = авто).", "عدد خيوط العمل الخلفية لتجميع خطوط أنابيب Vulkan (0 = تلقائي).", "Anzahl der Hintergrund-Worker-Threads für die Vulkan-Pipeline-Kompilierung (0 = Automatisch).", "Nombre de threads de travail en arrière-plan pour la compilation des pipelines Vulkan (0 = Auto).", "用于 Vulkan 管线后台编译的工作线程数量 (0 = 自动)。", "Vulkan パイプラインコンパイル用のバックグラウンドワーカースレッド数 (0 = 自動)。", "Número de subprocesos en segundo plano para compilar canalizaciones de Vulkan (0 = Automático).", "Vulkan 파이프라인 컴파일을 위한 백그라운드 작업자 스레드 수입니다 (0 = 자동).", "Numero di thread di lavoro in background per la compilazione della pipeline Vulkan (0 = Automatico).", "Número de threads de trabalho em segundo plano para compilação de pipeline Vulkan (0 = Automático).", "Vulkan पाइपलाइन संकलन के लिए पृष्ठभूमि कार्यकर्ता थ्रेड्स की संख्या (0 = ऑटो)।"},
        {"Airplane Mode", "Режим полёта", "وضع الطيران", "Flugmodus", "Mode avion", "飞行模式", "機内モード", "Modo avión", "비행기 모드", "Modalità aereo", "Modo avião", "हवाई जहाज मोड"},
        {"Disables all wireless and network communication.", "Отключает все беспроводные и сетевые соединения.", "تعطيل جميع الاتصالات اللاسلكية والشبكية.", "Deaktiviert die gesamte drahtlose und Netzwerkkommunikation.", "Désactive toutes les communications sans fil et réseau.", "禁用所有无线与网络通信。", "すべてのワイヤレス通信およびネットワーク通信を無効化します。", "Deshabilita todas las comunicaciones inalámbricas y de red.", "모든 무선 및 네트워크 통신을 비활성화합니다.", "Disabilita tutte le comunicazioni wireless e di rete.", "Desativa todas as comunicações sem fio e de rede.", "सभी वायरलेस और नेटवर्क संचार अक्षम करता है।"},
        {"API:", "Графический API:", "واجهة برمجة الرسوميات:", "Grafik-API:", "API graphique :", "图形 API:", "グラフィックス API:", "API de gráficos:", "그래픽 API:", "API grafica:", "API gráfica:", "ग्राफ़िक्स API:"},
        {"Device:", "Устройство:", "الجهاز:", "Gerät:", "Appareil :", "设备:", "デバイス:", "Dispositivo:", "장치:", "Dispositivo:", "Dispositivo:", "डिवाइस:"},
        {"VSync Mode:", "Режим VSync:", "وضع VSync:", "VSync-Modus:", "Mode VSync :", "垂直同步模式:", "VSync モード:", "Modo VSync:", "수직 동기화 모드:", "Modalità VSync:", "Modo de Sincronização vertical:", "वी-सिंक मोड:"},
        {"Resolution:", "Разрешение:", "الدقة:", "Auflösung:", "Résolution :", "分辨率:", "解像度:", "Resolución:", "해상도:", "Risoluzione:", "Resolução:", "रिज़ॉल्यूशन:"},
        {"Window Adapting Filter:", "Фильтр масштабирования окна:", "مرشح ملاءمة النافذة:", "Fensteranpassungsfilter:", "Filtre d'adaptation de fenêtre :", "窗口自适应滤镜:", "ウィンドウ適応フィルター:", "Filtro de adaptación de ventana:", "창 적응형 필터:", "Filtro di adattamento alla finestra:", "Filtro de adaptação de janela:", "विंडो अनुकूलन फ़िल्टर:"},
        {"FSR Sharpness:", "Резкость FSR:", "حدة FSR:", "FSR-Schärfe:", "Netteté FSR :", "FSR 锐度:", "FSR シャープネス:", "Nitidez FSR:", "FSR 선명도:", "Nitidezza FSR:", "Nitidez do FSR:", "FSR तीक्ष्णता:"},
        {"Anti-Aliasing Method:", "Метод сглаживания:", "طريقة منع التعرج:", "Kantenglättungsmethode:", "Méthode anticrénelage :", "抗锯齿方法:", "アンチエイリアシング方式:", "Método de suavizado:", "안티에일리어싱 방식:", "Metodo di anti-aliasing:", "Método de Anti-Aliasing", "एंटी-अलियासिंग विधि:"},
        {"Fullscreen Mode:", "Полноэкранный режим:", "وضع ملء الشاشة:", "Vollbildmodus:", "Mode plein écran :", "全屏模式:", "全画面モード:", "Modo de pantalla completa:", "전체 화면 모드:", "Modalità schermo intero:", "Tela Cheia", "पूर्ण स्क्रीन मोड:"},
        {"Aspect Ratio:", "Соотношение сторон:", "نسبة الأبعاد:", "Seitenverhältnis:", "Format d'image :", "宽高比:", "アスペクト比:", "Relación de aspecto:", "화면 비율:", "Rapporto d'aspetto:", "Proporção do Ecrã:", "पहलू अनुपात:"},
        {"Use persistent pipeline cache", "Использовать постоянный кэш конвейера", "استخدام ذاكرة خطوط الأنابيب الدائمة", "Dauerhaften Pipeline-Cache verwenden", "Utiliser le cache de pipeline persistant", "使用持久化管线缓存", "永続パイプラインキャッシュを使用", "Usar caché de tuberías persistente", "지속적 파이프라인 캐시 사용", "Usa la cache persistente delle pipeline", "Usar cache persistente de pipeline", "स्थायी पाइपलाइन कैश का उपयोग करें"},
        {"Use asynchronous GPU emulation", "Асинхронная эмуляция ГПУ", "محاكاة غير متزامنة لمعالج الرسوميات", "Asynchrone GPU-Emulation", "Émulation GPU asynchrone", "异步 GPU 模拟", "非同期 GPU エミュレーション", "Emulación asíncrona de GPU", "비동기 GPU 에뮬레이션 사용", "Abilita l'emulazione asincrona della GPU", "Usar emulação assíncrona de GPU", "अतुल्यकालिक GPU अनुकरण का उपयोग करें"},
        {"Eco Frame Pacing", "Эко-выравнивание кадров", "تنظيم الإطارات الاقتصادي", "Eco-Frame-Pacing", "Régulation d'images éco", "环保帧平滑", "エコフレームペーシング", "Compensación de fotogramas ecológica", "에코 프레임 페이싱", "Frame pacing Eco", "Ritmo de quadros Eco", "इको फ़्रेम पेसिंग"},
        {"Enable Frame Generation (LSFG)", "Включить генерацию кадров (LSFG)", "تفعيل توليد الإطارات (LSFG)", "Frame-Generierung aktivieren (LSFG)", "Activer la génération d'images (LSFG)", "启用帧生成 (LSFG)", "フレーム生成を有効化 (LSFG)", "Habilitar generación de fotogramas (LSFG)", "프레임 생성 활성화 (LSFG)", "Abilita generazione fotogrammi (LSFG)", "Ativar geração de quadros (LSFG)", "फ़्रेम जनरेशन सक्षम करें (LSFG)"},
        {"Generates intermediate frames using Lossless Scaling optical flow to double or quadruple FPS.", "Генерирует промежуточные кадры с помощью оптического потока Lossless Scaling (LSFG) для удвоения или учетверения FPS.", "يولد إطارات وسيطة باستخدام التدفق البصري لمضاعفة معدل الإطارات.", "Generiert Zwischenbilder mittels Lossless-Scaling-Optical-Flow zur Verdopplung oder Vervierfachung der FPS.", "Génère des images intermédiaires via le flux optique Lossless Scaling pour doubler ou quadrupler les FPS.", "使用 Lossless Scaling 光流算法生成中间帧，使帧率翻倍或提升至 4 倍。", "Lossless Scaling オプティカルフローを使用して中間フレームを生成し、FPS を 2〜4 倍に向上させます。", "Genera fotogramas intermedios mediante flujo óptico Lossless Scaling para duplicar o cuadruplicar los FPS.", "Lossless Scaling 광학 흐름을 사용하여 중간 프레임을 생성해 FPS를 2배~4배로 높입니다.", "Genera fotogrammi intermedi tramite flusso ottico Lossless Scaling per raddoppiare o quadruplicare gli FPS.", "Gera quadros intermediários usando fluxo óptico Lossless Scaling para dobrar ou quadruplicar o FPS.", "FPS को दोगुना या चौगुना करने के लिए Lossless Scaling ऑप्टिकल फ़्लो का उपयोग करके मध्यवर्ती फ़्रेम उत्पन्न करता है।"},
        {"Frame Generation Multiplier:", "Множитель генерации кадров (x2-x4):", "مضاعف توليد الإطارات:", "Frame-Generierungs-Multiplikator:", "Multiplicateur de génération d'images :", "帧生成倍率:", "フレーム生成倍率:", "Multiplicador de generación de fotogramas:", "프레임 생성 배율:", "Moltiplicatore generazione fotogrammi:", "Multiplicador de geração de quadros:", "फ़्रेम जनरेशन गुणक:"},
        {"Multiplier for generated frames (2x = double FPS, 3x = triple FPS, 4x = quadruple FPS).", "Коэффициент умножения кадров (2x = удвоение FPS, 3x = утроение FPS, 4x = учетверение FPS).", "معامل مضاعفة الإطارات المولدة (2x إلى 4x).", "Multiplikator für generierte Frames (2x = doppelte FPS, 3x = dreifache FPS, 4x = vierfache FPS).", "Multiplicateur d'images générées (2x = double FPS, 3x = triple FPS, 4x = quadruple FPS).", "生成帧的倍率（2x = 双倍帧率，3x = 三倍帧率，4x = 四倍帧率）。", "生成フレームの倍率（2x = 2倍 FPS、3x = 3倍 FPS、4x = 4倍 FPS）。", "Multiplicador de fotogramas generados (2x = doble FPS, 3x = triple FPS, 4x = cuádruple FPS).", "생성 프레임 배율입니다 (2x = 2배 FPS, 3x = 3배 FPS, 4x = 4배 FPS).", "Moltiplicatore dei fotogrammi generati (2x = doppio FPS, 3x = triplo FPS, 4x = quadruplo FPS).", "Multiplicador de quadros gerados (2x = dobro de FPS, 3x = triplo de FPS, 4x = quádruplo de FPS).", "जनरेट किए गए फ़्रेम का गुणक (2x = दोगुना FPS, 3x = तिगुना FPS, 4x = चौगुना FPS)।"},
        {"Target Frame Rate (Hz):", "Целевая частота кадров (Гц):", "معدل الإطارات المستهدف (هرتز):", "Ziel-Bildwiederholrate (Hz):", "Fréquence d'images cible (Hz) :", "目标帧率 (Hz):", "目標フレームレート (Hz):", "Tasa de fotogramas objetivo (Hz):", "목표 프레임 레이트 (Hz):", "Frequenza fotogrammi target (Hz):", "Taxa de quadros alvo (Hz):", "लक्ष्य फ़्रेम दर (Hz):"},
        {"Target output frame rate in Hz for Frame Generation.", "Целевая выходная частота обновления экрана в Гц для генерации кадров.", "معدل الإطارات النهائي المستهدف بالهرتز لتوليد الإطارات.", "Ziel-Ausgabebildrate in Hz für die Frame-Generierung.", "Fréquence d'images de sortie cible en Hz pour la génération d'images.", "帧生成的目标输出帧率 (Hz)。", "フレーム生成の目標出力フレームレート (Hz)。", "Tasa de fotogramas de salida objetivo en Hz para la generación de fotogramas.", "프레임 생성의 목표 출력 주사율(Hz)입니다.", "Frequenza di aggiornamento di uscita target in Hz per la generazione di fotogrammi.", "Taxa de atualização de saída alvo em Hz para geração de quadros.", "फ़्रेम जनरेशन के लिए Hz में लक्ष्य आउटपुट फ़्रेम दर।"},
        {"Optical Flow Scale (%):", "Масштаб оптического потока (%):", "مقياس التدفق البصري (%):", "Optical-Flow-Skalierung (%):", "Échelle du flux optique (%) :", "光流分辨率缩放 (%):", "オプティカルフロースケール (%):", "Escala de flujo óptico (%):", "광학 흐름 스케일 (%):", "Scala flusso ottico (%):", "Escala de fluxo óptico (%):", "ऑप्टिकल फ़्लो स्केल (%):"},
        {"Manual resolution scale percentage for optical flow estimation.", "Ручной масштаб разрешения в процентах для расчета векторов оптического потока.", "نسبة مقياس الدقة اليدوي لحساب التدفق البصري.", "Manuelle Auflösungsskalierung in Prozent für die Optical-Flow-Berechnung.", "Pourcentage d'échelle de résolution manuelle pour l'estimation du flux optique.", "用于光流运动估计的手动分辨率缩放百分比。", "オプティカルフロー推定の手動解像度スケール（%）。", "Porcentaje de escala de resolución manual para la estimación de flujo óptico.", "광학 흐름 추정을 위한 수동 해상도 배율(%)입니다.", "Percentuale manuale della scala di risoluzione per la stima del flusso ottico.", "Porcentagem manual da escala de resolução para estimativa de fluxo óptico.", "ऑप्टिकल फ़्लो अनुमान के लिए मैन्युअल रिज़ॉल्यूशन स्केल प्रतिशत।"},
        {"Frame Queue Depth:", "Глубина очереди кадров:", "عمق طابور الإطارات:", "Frame-Warteschlangentiefe:", "Profondeur de la file d'images :", "帧队列深度:", "フレームキュー深度:", "Profundidad de cola de fotogramas:", "프레임 대기열 깊이:", "Profondità coda fotogrammi:", "Profundidade da fila de quadros:", "फ़्रेम कतार गहराई:"},
        {"Number of frames queued for optical flow interpolation.", "Количество буферизуемых кадров в очереди интерполяции оптического потока.", "عدد الإطارات في طابور استيفاء التدفق البصري.", "Anzahl der gepufferten Frames für die Optical-Flow-Interpolation.", "Nombre d'images mises en file d'attente pour l'interpolation du flux optique.", "用于光流插值缓冲的帧队列深度。", "オプティカルフロー補間のためにキューイングされるフレーム数。", "Número de fotogramas en cola para la interpolación de flujo óptico.", "광학 흐름 보간을 위해 대기열에 넣는 프레임 수입니다.", "Numero di fotogrammi in coda per l'interpolazione del flusso ottico.", "Número de quadros enfileirados para interpolação de fluxo óptico.", "ऑप्टिकल फ़्लो इंटरपोलेशन के लिए कतारबद्ध फ़्रेमों की संख्या।"},
        {"Debug Optical Flow Vectors", "Отладка векторов оптического потока", "تصحيح متجهات التدفق البصري", "Optical-Flow-Vektoren debuggen", "Déboguer les vecteurs de flux optique", "调试光流运动矢量", "オプティカルフローベクトルのデバッグ表示", "Depurar vectores de flujo óptico", "광학 흐름 벡터 디버그", "Debug vettori flusso ottico", "Depurar vetores de fluxo óptico", "ऑप्टिकल फ़्लो वेक्टर डीबग करें"},
        {"Visualizes optical flow motion vectors on screen for debugging.", "Визуализирует векторы движения оптического потока на экране для отладки.", "يعرض متجهات حركة التدفق البصري على الشاشة لأغراض التصحيح.", "Visualisiert Optical-Flow-Bewegungsvektoren zur Fehlersuche auf dem Bildschirm.", "Visualise les vecteurs de mouvement du flux optique à l'écran pour le débogage.", "在屏幕上可视化显示光流运动矢量以供调试。", "デバッグ用にオプティカルフローの動きベクトルを画面上に可視化します。", "Visualiza los vectores de movimiento de flujo óptico en pantalla para depuración.", "디버깅을 위해 화면에 광학 흐름 모션 벡터를 시각화합니다.", "Visualizza sullo schermo i vettori di movimento del flusso ottico per il debug.", "Visualiza os vetores de movimento do fluxo óptico na tela para depuração.", "डीबगिंग के लिए स्क्रीन पर ऑप्टिकल फ़्लो मोशन वेक्टर प्रदर्शित करता है।"},
        {"Auto Optical Flow Scale", "Авто-масштаб оптического потока", "مقياس التدفق البصري التلقائي", "Automatische Optical-Flow-Skalierung", "Échelle automatique du flux optique", "自动光流缩放", "自動オプティカルフロースケール", "Escala de flujo óptico automática", "자동 광학 흐름 스케일", "Escala de flujo óptico automático", "Escala de flujo óptico automático", "ऑटो ऑप्टिकल फ़्लो स्केल"},
        {"Automatically calculates optical flow grid scale for frame generation.", "Автоматически рассчитывает оптимальный масштаб сетки оптического потока в зависимости от разрешения.", "يحسب مقياس شبكة التدفق البصري تلقائياً لتوليد الإطارات.", "Berechnet automatisch die optimale Optical-Flow-asterskalierung für die Frame-Generierung.", "Calcule automatiquement l'échelle de grille de flux optique pour la génération d'images.", "自动计算用于帧生成的最佳光流网格缩放比例。", "フレーム生成用のオプティカルフローグリッドスケールを自動計算します。", "Calcula automáticamente la escala de cuadrícula de flujo óptico para la generación de fotogramas.", "프레임 생성을 위한 광학 흐름 그리드 스케일을 자동으로 계산합니다.", "Calcola automaticamente la scala della griglia del flusso ottico per la generazione di fotogrammi.", "Calcula automaticamente a escala da grade de fluxo óptico para geração de quadros.", "फ़्रेम जनरेशन के लिए ऑप्टिकल फ़्लो ग्रिड स्केल की स्वचालित गणना करता है।"},
        {"FP16 Frame Generation", "Ускорение FP16 для генерации кадров", "توليد الإطارات بدقة FP16", "FP16-Frame-Generierung", "Génération d'images FP16", "FP16 半精度帧生成", "FP16 フレーム生成", "Generación de fotogramas FP16", "FP16 프레임 생성", "Generazione fotogrammi FP16", "Geração de quadros FP16", "FP16 फ़्रेम जनरेशन"},
        {"Uses 16-bit half precision floating point for frame generation.", "Использует быстрые 16-битные вычисления половинной точности (FP16) для ускорения генерации кадров.", "يستخدم الحسابات العشرية بنصف الدقة 16-بت لتسريع توليد الإطارات.", "Verwendet 16-Bit-Halbpräzisions-Gleitkommazahlen (FP16) zur Beschleunigung der Frame-Generierung.", "Utilise des calculs flottants 16 bits demi-précision (FP16) pour accélérer la génération d'images.", "使用 16 位半精度浮点数 (FP16) 加速帧生成计算。", "フレーム生成に 16 ビット半精度浮動小数点 (FP16) を使用して高速化します。", "Utiliza punto flotante de media precisión de 16 bits (FP16) para acelerar la generación de fotogramas.", "프레임 생성 가속을 위해 16비트 반정밀도 부동소수점(FP16)을 사용합니다.", "Utiliza coma flotante de media precisión de 16 bits para la generación de fotogramas.", "Utiliza coma flotante de media precisión de 16 bits para la generación de fotogramas.", "फ़्रेम जनरेशन के लिए 16-बिट हाफ़-प्रिसीजन फ़्लोटिंग पॉइंट (FP16) का उपयोग करता है।"},
        {"GPU Fence Behavior:", "Барьеры ГПУ:", "سلوك حواجز معالج الرسوميات:", "GPU-Fence-Verhalten:", "Comportement des barrières GPU :", "GPU 栅栏同步行为:", "GPU フェンス動作:", "Comportamiento de barreras GPU:", "GPU 펜스 동작:", "Comportamento fence GPU:", "Comportamento de cercas (fences) da GPU:", "GPU फेंस व्यवहार:"},
        {"VRAM Usage Mode:", "Использование видеопамяти:", "وضع استخدام ذاكرة الفيديو:", "VRAM-Nutzungsmodus:", "Mode d'utilisation VRAM :", "显存使用模式:", "VRAM 使用モード:", "Modo de uso de VRAM:", "VRAM 사용 모드:", "Modalità di utilizzo della VRAM:", "Modo de Uso da VRAM:", "VRAM उपयोग मोड:"},
        {"NVDEC emulation:", "Эмуляция NVDEC:", "محاكاة NVDEC:", "NVDEC-Emulation:", "Émulation NVDEC :", "NVDEC 视频解码模拟:", "NVDEC エミュレーション:", "Emulación NVDEC:", "NVDEC 에뮬레이션:", "Emulazione NVDEC:", "Emulação NVDEC:", "NVDEC अनुकरण:"},
        {"Anisotropic Filtering:", "Анизотропная фильтрация:", "التصفية متباينة الخواص:", "Anisotrope Filterung:", "Filtrage anisotrope :", "各向异性过滤:", "異方性フィルタリング:", "Filtrado anisotrópico:", "비등방성 필터링:", "Filtro anisotropico:", "Filtro Anisotrópico:", "एनिसोट्रोपिक फ़िल्टरिंग:"},
        {"ASTC Decoding Method:", "Метод декодирования ASTC:", "طريقة فك تشفير ASTC:", "ASTC-Dekodierungsmethode:", "Méthode de décodage ASTC :", "ASTC 解码方法:", "ASTC デコード方式:", "Método de decodificación ASTC:", "ASTC 디코딩 방식:", "Metodo di decodifica ASTC:", "Método de Decodificação ASTC:", "ASTC डिकोडिंग विधि:"},
        {"Frame Pacing Mode (Vulkan only)", "Режим стабилизации кадров (только Vulkan)", "وضع ضبط توقيت الإطارات (Vulkan فقط)", "Frame-Pacing-Modus (nur Vulkan)", "Mode de calage d'images (Vulkan uniquement)", "帧同步模式 (仅限 Vulkan)", "フレームペーシングモード (Vulkan のみ)", "Modo de sincronización de fotogramas (solo Vulkan)", "프레임 페이싱 모드(Vulkan 전용)", "Modalità frame pacing (solo Vulkan)", "Modo de Ritmo de Quadros (apenas Vulkan)", "फ्रेम पेसिंग मोड (केवल Vulkan)"},
        {"ASTC Recompression Method:", "Метод пересжатия ASTC:", "طريقة إعادة ضغط ASTC:", "ASTC-Rekompression:", "Méthode de recompression ASTC :", "ASTC 重压缩方式:", "ASTC 再圧縮方式:", "Método de recompresión ASTC:", "ASTC 재압축 방식:", "Metodo di ricompressione ASTC:", "Método de Recompressão ASTC:", "ASTC पुनः संपीड़न विधि:"},
        {"Lock Dynamic Resolution", "Блокировка динамического разрешения", "قفل الدقة الديناميكية", "Dynamische Auflösung sperren", "Verrouiller la résolution dynamique", "锁定动态分辨率", "動的解像度ロック", "Bloquear resolución dinámica", "동적 해상도 고정", "Blocca risoluzione dinamica", "Travar resolução dinâmica", "डायनेमिक रिज़ॉल्यूशन लॉक करें"},
        {"Sync Memory Operations", "Синхронизация операций памяти", "مزامنة عمليات الذاكرة", "Speicheroperationen synchronisieren", "Synchroniser les opérations de mémoire", "同步内存操作", "メモリオペレーションの同期", "Sincronizar operaciones de memoria", "메모리 작업 동기화", "Sincronizza operazioni di memoria", "Sincronizar operações de memória", "मेमोरी संचालन सिंक्रनाइज़ करें"},
        {"Force Maximum Clocks (PC and Mobile)", "Принудительная максимальная частота", "فرض أقصى ترددات", "Maximale Taktraten erzwingen", "Forcer les fréquences maximales", "强制最高时钟频率", "最大クロックを強制", "Forzar frecuencias máximas", "최대 클럭 강제 적용 (PC 및 모바일)", "Forza frequenze massime (PC e Mobile)", "Forçar frequências máximas (PC e Mobile)", "अधिकतम क्लॉक बाध्य करें (PC और मोबाइल)"},
        {"Force maximum clocks (Vulkan only)", "Принудительная максимальная частота (только Vulkan)", "فرض أقصى ترددات (Vulkan فقط)", "Maximale Taktraten erzwingen (nur Vulkan)", "Forcer les fréquences maximales (Vulkan uniquement)", "强制最高时钟频率 (仅限 Vulkan)", "最大クロックを強制 (Vulkan のみ)", "Forzar frecuencias máximas (solo Vulkan)", "최대 클럭 강제 적용(Vulkan 전용)", "Forza clock massimi (solo Vulkan)", "Forçar clock máximo (somente Vulkan)", "अधिकतम क्लॉक बाध्य करें (केवल Vulkan)"},
        {"Early Release Fences", "Раннее освобождение барьеров", "تحرير المزامنة المبكر (Fences)", "Frühes Freigeben von Fences", "Libération anticipée des barrières", "提前释放同步栅栏 (Fences)", "フェンスの早期解放", "Liberación temprana de barreras", "GPU 펜스 조기 해제", "Rilascio anticipato fence GPU", "Liberação antecipada de fences", "फ़ेंस शीघ्र रिलीज़ करें"},
        {"Optimize SPIR-V Output", "Оптимизировать SPIR-V вывод", "تحسين مخرجات SPIR-V", "SPIR-V-Ausgabe optimieren", "Optimiser la sortie SPIR-V", "优化 SPIR-V 输出", "SPIR-V 出力を最適化", "Optimizar salida SPIR-V", "SPIR-V 출력 최적화", "Ottimizza output SPIR-V", "Otimizar saída SPIR-V", "SPIR-V आउटपुट अनुकूलित करें"},
        {"Fast GPU Time", "Быстрое время ГПУ", "توقيت GPU السريع", "Schnelle GPU-Zeit", "Temps GPU rapide", "快速 GPU 时间", "高速 GPU タイマー", "Tiempo rápido de GPU", "빠른 CPU 시간", "Tempo GPU veloce", "Tempo Rápido da GPU", "तेज़ GPU समय"},
        {"Enable Frame Skipping", "Включить пропуск кадров", "تفعيل تخطي الإطارات", "Frame-Skipping aktivieren", "Activer le saut d'images", "启用跳帧", "フレームスキップを有効化", "Habilitar salto de fotogramas", "프레임 스킵 활성화", "Abilita salto fotogrammi", "Ativar pulo de quadros", "सक्षम करें Frame Skipping"},
        {"Use Vulkan pipeline cache", "Кэш конвейеров Vulkan", "استخدام ذاكرة خطوط أنابيب Vulkan", "Vulkan-Pipeline-Cache verwenden", "Utiliser le cache de pipeline Vulkan", "使用 Vulkan 管线缓存", "Vulkan パイプラインキャッシュを使用", "Usar caché de tuberías de Vulkan", "Vulkan 파이프라인 캐시 사용", "Usa la cache delle pipeline di Vulkan", "Utilizar cache de pipeline do Vulkan", "Vulkan पाइपलाइन कैश का उपयोग करें"},
        {"Enable asynchronous shader compilation", "Включить асинхронную компиляцию шейдеров", "تجميع الشيدر غير المتزامن", "Asynchrone Shader-Kompilierung aktivieren", "Activer la compilation asynchrone des shaders", "启用异步着色器编译", "非同期シェーダーコンパイルを有効化", "Habilitar compilación asíncrona de shaders", "비동기 셰이더 컴파일 활성화", "Abilita la compilazione asincrona degli shader", "Ativar compilação assíncrona de shaders", "असिंक्रोनस शेडर संकलन सक्षम करें"},
        {"Sync to framerate of video playback", "Синхронизировать с частотой кадров видео", "المزامنة مع معدل إطارات تشغيل الفيديو", "Mit Video-Framerate synchronisieren", "Synchroniser avec le framerate de la vidéo", "与视频播放帧率同步", "動画再生のフレームレートに同期", "Sincronizar con tasa de fotogramas de video", "비디오 재생 프레임 속도에 동기화", "Sincronizza il framerate a quello del video", "Sincronizar com o framerate da reprodução de vídeo", "वीडियो प्लेबैक फ़्रेमरेट से सिंक करें"},
        {"Enable Reactive Flushing", "Включить реактивный сброс", "تفعيل التفريغ التفاعلي", "Reaktives Flushing aktivieren", "Activer la vidange réactive", "启用响应式刷新 (Reactive Flushing)", "リアクティブフラッシュを有効化", "Habilitar vaciado reactivo", "반응형 플러싱 활성화", "Abilita il flushing reattivo", "Habilitar Flushing Reativo", "रिएक्टिव फ़्लशिंग सक्षम करें"},
        {"Barrier feedback loops", "Циклы обратной связи барьеров", "حلقات التغذية الراجعة للحواجز", "Barrieren-Feedbackschleifen", "Boucles de rétroaction des barrières", "栅栏反馈循环", "バリアフィードバックループ", "Bucles de retroalimentación de barreras", "피드백 루프 차단", "Cicli di feedback dei barrier", "Ciclos de feedback de barreira", "बैरियर फ़ीडबैक लूप्स"},
        {"Enable buffer history", "Включить историю буфера", "تفعيل سجل التخزين المؤقت", "Pufferverlauf aktivieren", "Activer l'historique du tampon", "启用缓冲区历史记录", "バッファ履歴を有効化", "Habilitar historial de búfer", "버퍼 기록 활성화", "Abilita cronologia buffer", "Ativar histórico de buffer", "सक्षम करें buffer history"},
        {"Smart Shader Throttle", "Умный троттлинг шейдеров", "التحكم الذكي بمترجم الشيدر", "Smart Shader Throttle", "Régulation intelligente des shaders", "智能着色器节流", "スマートシェーダースロットル", "Regulador inteligente de shaders", "스마트 셰이더 스로틀링", "Regolazione intelligente shader", "Controle inteligente de shaders", "स्मार्ट शेडर थ्रॉटल"},
        {"VRAM Garbage Collection", "Очистка видеопамяти", "تنظيف ذاكرة الفيديو (VRAM)", "VRAM-Speicherbereinigung", "Nettoyage de la mémoire VRAM", "显存垃圾回收", "VRAM ガベージコレクション", "Recolección de basura VRAM", "VRAM 가비지 컬렉션", "Pulizia memoria VRAM", "Coleta de lixo de VRAM", "VRAM गार्बेज कलेक्शन"},
        {"Enable HDR10", "Включить HDR10", "تفعيل HDR10", "HDR10 aktivieren", "Activer HDR10", "启用 HDR10", "HDR10 を有効化", "Habilitar HDR10", "HDR10 활성화", "Abilita HDR10", "Ativar HDR10", "HDR10 सक्षम करें"},
        {"Low Latency Display", "Низкая задержка вывода", "عرض منخفض الكمون", "Anzeige mit geringer Latenz", "Affichage à faible latence", "低延迟显示", "低遅延ディスプレイ", "Pantalla de baja latencia", "저지연 디스플레이", "Schermo a bassa latenza", "Tela de baixa latência", "कम विलंबता डिस्प्ले"},
        {"Minimizes presentation queue depth to reduce display input latency.", "Минимизирует глубину очереди вывода кадров для снижения задержки ввода.", "يقلل عمق قائمة العرض لتخفيض زمن استجابة الإدخال.", "Minimiert die Warteschlangentiefe zur Reduzierung der Eingabelatenz.", "Minimise la file de présentation pour réduire la latence d'entrée.", "最小化帧呈现队列深度以降低显示输入延迟。", "表示キューの深さを最小化し、入力遅延を低減します。", "Minimiza la cola de presentación para reducir la latencia de entrada.", "프레임 대기열 깊이를 최소화하여 입력 지연을 줄입니다.", "Riduce la coda di presentazione per abbassare la latenza di input.", "Minimiza a fila de apresentação para reduzir a latência de entrada.", "इनपुट विलंबता कम करने के लिए प्रेजेंटेशन कतार की गहराई कम करता है।"},
        {"Enable cheats by default", "Включать читы по умолчанию", "تفعيل الغش افتراضياً", "Cheats standardmäßig aktivieren", "Activer les codes de triche par défaut", "默认启用金手指", "チートをデフォルトで有効にする", "Activar trucos por defecto", "치트 기본 활성화", "Abilita trucchi predefiniti", "Ativar trapaças por padrão", "डिफ़ॉल्ट रूप से चीट्स सक्षम करें"},
        {"Enables regular cheats when a game starts. Master Code entries are always enabled. Individual cheats can still be toggled from Cheats in the menu bar while the game is running.", "Автоматически активирует обычные читы при запуске игры. Мастер-коды всегда активны. Отдельные читы можно переключать в меню «Читы» во время игры.", "يُفعّل رموز الغش العادية عند بدء اللعبة. رموز الماستر مفعلة دائماً.", "Aktiviert reguläre Cheats beim Spielstart. Master-Codes sind immer aktiv. Einzelne Cheats können im Menü umgeschaltet werden.", "Active les codes de triche normaux au démarrage du jeu. Les codes maîtres sont toujours actifs.", "启动游戏时自动启用常规金手指。主代码始终启用，运行中可随时在菜单中单独开关。", "ゲーム開始時に通常チートを有効にします。マスターコードは常に有効です。実行中もメニューから個別に切り替え可能です。", "Activa los trucos normales al iniciar el juego. Los códigos maestros siempre están activos.", "게임 시작 시 일반 치트를 자동으로 활성화합니다. 마스터 코드는 항상 활성화됩니다.", "Abilita i trucchi normali all'avvio del gioco. I codici master sono sempre attivi.", "Ativa trapaças comuns ao iniciar o jogo. Códigos mestre ficam sempre ativos.", "गेम शुरू होने पर सामान्य चीट्स सक्षम करता है। मास्टर कोड हमेशा सक्रिय रहते हैं।"},
        {"VRAM Budget Governor", "Контроллер бюджета видеопамяти", "منظم ميزانية VRAM", "VRAM-Budget-Governor", "Régulateur de budget VRAM", "显存预算调度器", "VRAM 予算ガバナー", "Regulador de presupuesto VRAM", "VRAM 예산 제어기", "Regolatore budget VRAM", "Controlador de orçamento de VRAM", "VRAM बजट गवर्नर"},
        {"Enable GPU buffer readback", "Обратное чтение буфера ГПУ", "تفعيل إعادة قراءة مخزن GPU", "GPU-Puffer-Rücklesung aktivieren", "Activer la relecture du tampon GPU", "启用 GPU 缓冲区回读", "GPU バッファのリードバックを有効化", "Habilitar relectura del búfer de GPU", "GPU 버퍼 되읽기 활성화", "Abilita rilettura buffer GPU", "Ativar releitura de buffer da GPU", "GPU बफ़र रीडबैक सक्षम करें"},
        {"Floating Translate Button", "Плавающая кнопка перевода", "زر الترجمة العائم", "Schwebender Übersetzungs-Button", "Bouton de traduction flottant", "悬浮翻译按钮", "フローティング翻訳ボタン", "Botón de traducción flotante", "플로팅 번역 버튼", "Pulsante traduzione flottante", "Botão flutuante de tradução", "फ़्लोटिंग अनुवाद बटन"},
        {"Conservative", "Экономный", "اقتصادي", "Konservativ", "Économe", "保守", "控えめ", "Conservador", "소극적", "Conservativa", "Conservador", "संरक्षित (किफ़ायती)"},
        {"Normal", "Нормальный", "عادي", "Normal", "Normal", "正常", "標準", "Normal", "보통", "Normale", "Normal (Recomendado)", "सामान्य"},
        {"Aggressive", "Агрессивный", "عدواني", "Aggressiv", "Agressif", "激进", "積極的", "Agresivo", "적극적", "Aggressiva", "Agressivo", "आक्रामक"},
        {"Boost", "Турбо", "تعزيز", "Boost", "Boost", "加速", "ブースト", "Boost", "부스트", "Boost (1700MHz)", "Boost (1700MHz)", "बूस्ट"},
        {"Overclock", "Разгон", "كسر السرعة", "Übertaktung", "Overclocking", "超频", "オーバークロック", "Overclock", "오버클럭", "Veloce (2000MHz)", "Rápido (2000MHz)", "ओवरक्लॉक"},
        {"Docked", "В доке", "في القاعدة (Docked)", "Dock-Modus", "Mode téléviseur", "主机模式 (底座)", "TV モード", "En base (Dock)", "독 모드", "Dock", "Ancorado", "डॉक किया गया (Docked)"},
        {"Handheld", "Портативный", "محمول", "Handheld-Modus", "Mode portable", "掌机模式", "携帯モード", "Modo portátil", "휴대 모드", "Portatile", "Portátil", "हैंडहेल्ड (Handheld)"},
        {"Disabled", "Отключено", "معطل", "Deaktiviert", "Désactivé", "已禁用", "無効", "Desactivado", "비활성화됨", "Disabilitato", "Desativado", "अक्षम"},
        {"Always ask (Default)", "Всегда спрашивать (по умолчанию)", "السؤال دائماً (افتراضي)", "Immer fragen (Standard)", "Toujours demander (par défaut)", "总是询问 (默认)", "常に確認 (デフォルト)", "Preguntar siempre (predeterminado)", "항상 묻기(기본값)", "Chiedi sempre (Predefinito)", "Sempre perguntar (Padrão)", "हमेशा पूछें (डिफ़ॉल्ट)"},
        {"Only if game specifies not to stop", "Только если игра требует не останавливать", "فقط إذا طلبت اللعبة عدم الإيقاف", "Nur wenn das Spiel festlegt, nicht zu stoppen", "Seulement si le jeu demande de ne pas arrêter", "仅在游戏明确要求不停止时", "ゲームが停止しないよう指定している場合のみ", "Solo si el juego especifica no detenerse", "게임에서 멈추지 말라고 명시한 경우에만", "Solo se il gioco richiede di non essere arrestato", "Somente se o jogo especificar para não parar", "केवल जब गेम रोकने से मना करे"},
        {"Never ask", "Никогда не спрашивать", "عدم السؤال إطلاقاً", "Nie fragen", "Ne jamais demander", "从不询问", "確認しない", "No preguntar nunca", "절대 묻지 않음", "Non chiedere mai", "Nunca perguntar", "कभी न पूछें"},
        {"Tree View", "Дерево", "عرض شجري", "Baumansicht", "Vue arborescente", "树状视图", "ツリー表示", "Vista de árbol", "트리 보기", "Vista ad albero", "Visualização em Árvore", "ट्री दृश्य"},
        {"Grid View", "Сетка", "عرض شبكي", "Gitteransicht", "Vue en grille", "网格视图", "グリッド表示", "Vista de cuadrícula", "격자 보기", "Vista a griglia", "Visualização em Grade", "ग्रिड दृश्य"},
        {"4GB DRAM (Default)", "4 ГБ DRAM (По умолчанию)", "4 جيجابايت DRAM (افتراضي)", "4 GB DRAM (Standard)", "4 Go DRAM (Par défaut)", "4GB 内存 (默认)", "4GB DRAM (デフォルト)", "4 GB DRAM (Predeterminado)", "4GB DRAM (기본값)", "4 GB DRAM (Predefinito)", "4 GB DRAM (Padrão)", "4GB DRAM (डिफ़ॉल्ट)"},
        {"6GB DRAM (Unsafe)", "6 ГБ DRAM (Небезопасно)", "6 جيجابايت DRAM (غير آمن)", "6 GB DRAM (Unsicher)", "6 Go DRAM (Non sécurisé)", "6GB 内存 (非安全)", "6GB DRAM (非安全)", "6 GB DRAM (Inseguro)", "6GB DRAM (불안정)", "6 GB DRAM (Non sicuro)", "6 GB DRAM (Não seguro)", "6GB DRAM (असुरक्षित)"},
        {"8GB DRAM", "8 ГБ DRAM", "8 جيجابايت DRAM", "8 GB DRAM", "8 Go DRAM", "8GB 内存", "8GB DRAM", "8 GB DRAM", "8GB DRAM", "8 GB DRAM", "8 GB DRAM", "8 GB DRAM"},
        {"10GB DRAM (Unsafe)", "10 ГБ DRAM (Небезопасно)", "10 جيجابايت DRAM (غير آمن)", "10 GB DRAM (Unsicher)", "10 Go DRAM (Non sécurisé)", "10GB 内存 (非安全)", "10GB DRAM (非安全)", "10 GB DRAM (Inseguro)", "10GB DRAM (불안정)", "10 GB DRAM (Non sicuro)", "10 GB DRAM (Não seguro)", "10GB DRAM (असुरक्षित)"},
        {"12GB DRAM (Unsafe)", "12 ГБ DRAM (Небезопасно)", "12 جيجابايт DRAM (غير آمن)", "12 GB DRAM (Unsicher)", "12 Go DRAM (Non sécurisé)", "12GB 内存 (非安全)", "12GB DRAM (非安全)", "12 GB DRAM (Inseguro)", "12GB DRAM (불안정)", "12 GB DRAM (Non sicuro)", "12 GB DRAM (Não seguro)", "12GB DRAM (असुरक्षित)"},
        {"Very Small (16 MB)", "Очень малый (16 МБ)", "صغير جداً (16 ميجابايت)", "Sehr klein (16 MB)", "Très petit (16 Mo)", "极小 (16 MB)", "極小 (16 MB)", "Muy pequeño (16 MB)", "아주 작은 크기(16 MB)", "Molto piccola (16 MB)", "Muito Pequeno (16 MB)", "बहुत छोटा (16 MB)"},
        {"Small (32 MB)", "Малый (32 МБ)", "صغير (32 ميجابايت)", "Klein (32 MB)", "Petit (32 Mo)", "较小 (32 MB)", "小 (32 MB)", "Pequeño (32 MB)", "작은 크기(32 MB)", "Piccola (32 MB)", "Pequeno (32 MB)", "छोटा (32 MB)"},
        {"Normal (128 MB)", "Нормальный (128 МБ)", "عادي (128 ميجابايت)", "Normal (128 MB)", "Normal (128 Mo)", "正常 (128 MB)", "標準 (128 MB)", "Normal (128 MB)", "보통 크기(128 MB)", "Normale (128 MB)", "Normal (128 MB)", "सामान्य (128 MB)"},
        {"Large (256 MB)", "Большой (256 МБ)", "كبير (256 ميجابايت)", "Groß (256 MB)", "Grand (256 Mo)", "较大 (256 MB)", "大 (256 MB)", "Grande (256 MB)", "큰 크기(256 MB)", "Grande (256 MB)", "Grande (256 MB)", "बड़ा (256 MB)"},
        {"Very Large (512 MB)", "Очень большой (512 МБ)", "كبير جداً (512 ميجابايت)", "Sehr groß (512 MB)", "Très grand (512 Mo)", "极大 (512 MB)", "極大 (512 MB)", "Muy grande (512 MB)", "아주 큰 크기(512 MB)", "Molto grande (512 MB)", "Muito Grande (512 MB)", "बहुत बड़ा (512 MB)"},
        {"Very Low (4 MB)", "Очень низкий (4 МБ)", "منخفض جداً (4 ميجابايت)", "Sehr niedrig (4 MB)", "Très bas (4 Mo)", "极低 (4 MB)", "極低 (4 MB)", "Muy bajo (4 MB)", "매우 낮은 크기(4 MB)", "Molto bassa (4 MB)", "Muito Baixo (4 MB)", "Very निम्न (4 MB)"},
        {"Low (8 MB)", "Низкий (8 МБ)", "منخفض (8 ميجابايت)", "Niedrig (8 MB)", "Bas (8 Mo)", "较低 (8 MB)", "低 (8 MB)", "Bajo (8 MB)", "낮은 크기(8 MB)", "Bassa (8 MB)", "Baixo (8 MB)", "निम्न (8 MB)"},
        {"Normal (16 MB)", "Нормальный (16 МБ)", "عادي (16 ميجابايت)", "Normal (16 MB)", "Normal (16 Mo)", "正常 (16 MB)", "標準 (16 MB)", "Normal (16 MB)", "보통 크기(16 MB)", "Normale (16 MB)", "Normal (16 MB)", "सामान्य (16 MB)"},
        {"Medium (32 MB)", "Средний (32 МБ)", "متوسط (32 ميجابايت)", "Mittel (32 MB)", "Moyen (32 Mo)", "中等 (32 MB)", "中 (32 MB)", "Medio (32 MB)", "중간 크기(32 MB)", "Media (32 MB)", "Médio (32 MB)", "मध्यम (32 MB)"},
        {"High (64 MB)", "Высокий (64 МБ)", "عالي (64 ميجابايت)", "Hoch (64 MB)", "Élevé (64 Mo)", "较高 (64 MB)", "高 (64 MB)", "Alto (64 MB)", "높은 크기(64 MB)", "Alta (64 MB)", "Alto (64 MB)", "उच्च (64 MB)"},
        {"Very Low (32)", "Очень низкий (32)", "منخفض جداً (32)", "Sehr niedrig (32)", "Très bas (32)", "极低 (32)", "極低 (32)", "Muy bajo (32)", "매우 낮은 크기(32)", "Molto basso (32)", "Muito Baixo (32)", "Very निम्न (32)"},
        {"Low (64)", "Низкий (64)", "منخفض (64)", "Niedrig (64)", "Bas (64)", "较低 (64)", "低 (64)", "Bajo (64)", "낮은 크기(64)", "Basso (64)", "Baixo (64)", "निम्न (64)"},
        {"Normal (128)", "Нормальный (128)", "عادي (128)", "Normal (128)", "Normal (128)", "正常 (128)", "標準 (128)", "Normal (128)", "보통 크기(128)", "Normale (128)", "Normal (128)", "सामान्य (128)"},
        {"Medium (256)", "Средний (256)", "متوسط (256)", "Mittel (256)", "Moyen (256)", "中等 (256)", "中 (256)", "Medio (256)", "보통(256)", "Medio (256)", "Médio (256)", "मध्यम (256)"},
        {"High (512)", "Высокий (512)", "عالي (512)", "Hoch (512)", "Élevé (512)", "较高 (512)", "高 (512)", "Alto (512)", "빠름(512)", "Alto (512)", "Alto (512)", "उच्च (512)"},
        {"ExtendedDynamicState 1", "ExtendedDynamicState 1", "ExtendedDynamicState 1", "ExtendedDynamicState 1", "ExtendedDynamicState 1", "扩展动态状态 1", "拡張ダイナミックステート 1", "ExtendedDynamicState 1", "확장된 동적 상태 1", "Stato dinamico esteso 1", "Estado Dinâmico Estendido 1", "ExtendedDynamicState 1"},
        {"ExtendedDynamicState 2", "ExtendedDynamicState 2", "ExtendedDynamicState 2", "ExtendedDynamicState 2", "ExtendedDynamicState 2", "扩展动态状态 2", "拡張ダイナミックステート 2", "ExtendedDynamicState 2", "확장된 동적 상태 2", "Stato dinamico esteso 2", "Estado Dinâmico Estendido 2", "ExtendedDynamicState 2"},
        {"ExtendedDynamicState 3", "ExtendedDynamicState 3", "ExtendedDynamicState 3", "ExtendedDynamicState 3", "ExtendedDynamicState 3", "扩展动态状态 3", "拡張ダイナミックステート 3", "ExtendedDynamicState 3", "확장된 동적 상태 3", "Stato dinamico esteso 3", "Estado Dinâmico Estendido 3", "ExtendedDynamicState 3"},
        {"Default", "По умолчанию", "افتراضي", "Standard", "Par défaut", "默认", "デフォルト", "Por defecto", "기본값", "Predefinito", "Padrão", "डिफ़ॉल्ट"},
        {"Default (16:9)", "По умолчанию (16:9)", "افتراضي (16:9)", "Standard (16:9)", "Par défaut (16:9)", "默认 (16:9)", "デフォルト (16:9)", "Por defecto (16:9)", "기본(16:9)", "Predefinito (16:9)", "Padrão (16:9)", "डिफ़ॉल्ट (16:9)"},
        {"Stretch to Window", "Растянуть до окна", "تمديد إلى النافذة", "Auf Fenstergröße strecken", "Étirer à la fenêtre", "拉伸至窗口", "ウィンドウに合わせる", "Estirar a la ventana", "화면 늘리기", "Allunga a finestra", "Esticar à Janela", "विंडो के आकार में फैलाएँ"},
        {"Borderless Windowed", "Безрамочный режим", "نافذة بلا حدود", "Rahmenloses Fenster", "Fenêtré sans bordure", "无边框窗口", "ボーダーレスウィンドウ", "Ventana sin bordes", "테두리 없는 창", "Finestra senza bordi", "Janela sem bordas", "बॉर्डरलेस विंडो"},
        {"Exclusive Fullscreen", "Эксклюзивный полноэкранный режим", "ملء الشاشة الحصري", "Exklusives Vollbild", "Plein écran exclusif", "独占全屏", "排他フルスクリーン", "Pantalla completa exclusiva", "독점 전체 화면", "Schermo intero esclusivo", "Tela cheia exclusiva", "एक्सक्लूसिव फ़ुलस्क्रीन"},
        {"None", "Нет", "لا شيء", "Keine", "Aucun", "无", "なし", "Ninguno", "없음", "Nessuno", "Nenhum", "कोई नहीं"},
        {"Off", "Выкл", "إيقاف", "Aus", "Désactivé", "关闭", "オフ", "Desactivado", "끔", "Disattivo", "Desligado", "बंद"},
        {"Never", "Никогда", "أبداً", "Nie", "Jamais", "从不", "なし", "Nunca", "안 함", "Mai", "Nunca", "कभी नहीं"},
        {"On Load", "При загрузке", "عند التحميل", "Beim Laden", "Au chargement", "加载时", "読み込み時", "Al cargar", "로드 시", "Al caricamento", "Ao carregar", "On लोड करें"},
        {"Always", "Всегда", "دائماً", "Immer", "Toujours", "总是", "常に", "Siempre", "항상", "Sempre", "Sempre", "हमेशा"},
        {"GPU Video Decoding (Default)", "ГПУ", "GPU (افتراضي)", "GPU (Standard)", "GPU (par défaut)", "GPU (默认)", "GPU (デフォルト)", "GPU (predeterminado)", "GPU 비디오 디코딩(기본값)", "Decodifica video GPU (predefinita)", "Decodificação de vídeo pela GPU (Padrão)", "GPU वीडियो डिकोडिंग (डिफ़ॉल्ट)"},
        {"CPU Video Decoding", "ЦП", "CPU", "CPU", "CPU", "CPU", "CPU", "CPU", "CPU 비디오 디코딩", "Decodifica video CPU", "Decodificação de vídeo pela CPU", "CPU वीडियो डिकोडिंग"},
        {"No Video Output", "Без вывода видео", "بدون إخراج فيديو", "Keine Videoausgabe", "Aucune sortie vidéo", "无视频输出", "ビデオ出力なし", "Sin salida de video", "비디오 출력 없음", "Nessun output video", "Sem saída de vídeo", "कोई वीडियो आउटपुट नहीं"},
        {"Uncompressed (Best quality)", "Без сжатия (наилучшее качество)", "غير مضغوط (أفضل جودة)", "Unkomprimiert (Beste Qualität)", "Non compressé (Meilleure qualité)", "未压缩 (最高质量)", "非圧縮 (最高品質)", "Sin comprimir (mejor calidad)", "비압축(최고 품질)", "Nessuna compressione (qualità migliore)", "Descompactado (Melhor Qualidade)", "असंपीड़ित (सर्वोत्तम गुणवत्ता)"},
        {"BC1 (Low quality)", "BC1 (Низкое качество)", "BC1 (جودة منخفضة)", "BC1 (Geringe Qualität)", "BC1 (Basse qualité)", "BC1 (低质量)", "BC1 (低品質)", "BC1 (baja calidad)", "BC1(저품질)", "BC1 (qualità bassa)", "BC1 (Baixa qualidade)", "BC1 (निम्न गुणवत्ता)"},
        {"BC3 (Medium quality)", "BC3 (Среднее качество)", "BC3 (جودة متوسطة)", "BC3 (Mittlere Qualität)", "BC3 (Qualité moyenne)", "BC3 (中等质量)", "BC3 (中品質)", "BC3 (calidad media)", "BC3(중간 품질)", "BC3 (qualità media)", "BC3 (Média qualidade)", "BC3 (मध्यम गुणवत्ता)"},
        {"BC5 (High quality)", "BC5 (Высокое качество)", "BC5 (جودة عالية)", "BC5 (Hohe Qualität)", "BC5 (Haute qualité)", "BC5 (高质量)", "BC5 (高品質)", "BC5 (alta calidad)", "BC5 (高品質)", "BC5 (alta qualità)", "BC5 (alta qualidade)", "BC5 (उच्च गुणवत्ता)"},
        {"Hybrid Video Decoding", "Гибридный", "هجين", "Hybrid", "Hybride", "混合", "ハイブリッド", "Híbrido", "하이브리드 비디오 디코딩", "Híbrido", "Híbrido", "हाइब्रिड वीडियो डिकोडिंग"},
        {"Hybrid", "Гибридный", "هجين", "Hybrid", "Hybride", "混合", "ハイブリッド", "Híbrido", "하이브리드", "Híbrido", "Híbrido", "हाइब्रिड"},
        {"Carousel View", "Карусель", "عرض دوار", "Karussellansicht", "Vue carrousel", "旋转木马视图", "カルーセル表示", "Vista de carrusel", "캐러셀 보기", "Vista carosello", "Exibição em carrossel", "कैरोसेल दृश्य"},
        {"Fast", "Быстро", "سريع", "Schnell", "Rapide", "快速", "高速", "Rápido", "빠름", "Veloce", "Rápido", "तेज़"},
        {"Strict", "Строго", "صارم", "Strikt", "Strict", "严格", "厳格", "Estricto", "염격", "Rigoroso", "Estrito", "सख़्त (Strict)"},
        {"Immediate", "Немедленно", "فوري", "Sofort", "Immédiat", "立即", "即時", "Inmediato", "즉시", "Immediato", "Imediato", "तत्काल (Immediate)"},
        {"Balanced", "Сбалансированно", "متوازن", "Ausgewogen", "Équilibré", "平衡", "バランス", "Equilibrado", "균형", "Bilanciata", "Balanceado", "संतुलित"},
        {"Automatic", "Автоматически", "تلقائي", "Automatisch", "Automatique", "自动", "自動", "Automático", "자동", "Automatico", "Automático", "स्वचालित"},
        {"Custom frontend", "Пользовательский интерфейс", "واجهة أمامية مخصصة", "Benutzerdefiniertes Frontend", "Interface personnalisée", "自定义前端", "カスタムフロントエンド", "Frontend personalizado", "사용자 정의 프론트엔드", "Frontend personalizzato", "Frontend customizado", "कस्टम फ़्रंटएंड"},
        {"Real applet", "Настоящий апплет", "بريمج حقيقي", "Echtes Applet", "Véritable applet", "真实小程序", "実際のアプレット", "Applet real", "실제 애플릿", "Applet reale", "Applet real", "वास्तविक एप्लेट"},
        {"CPU Asynchronous", "ЦП (Асинхронно)", "المعالج (غير متزامن)", "CPU asynchron", "CPU asynchrone", "CPU 异步", "CPU 非同期", "CPU asíncrono", "비동기 CPU", "CPU (Asincrono)", "CPU Assíncrona", "CPU असिंक्रोनस"},
        {"Force 4:3", "Принудительно 4:3", "إجبار 4:3", "4:3 erzwingen", "Forcer 4:3", "强制 4:3", "強制 4:3", "Forzar 4:3", "강제 4:3", "Forza 4:3", "Forçar 4:3", "4:3 बाध्य करें"},
        {"Force 21:9", "Принудительно 21:9 (UltraWide)", "إجبار 21:9", "21:9 erzwingen", "Forcer 21:9", "强制 21:9", "強制 21:9", "Forzar 21:9", "강제 21:9", "Forza 21:9", "Forçar 21:9", "21:9 बाध्य करें"},
        {"Force 16:10", "Принудительно 16:10 (Steam Deck / WUXGA)", "إجبار 16:10", "16:10 erzwingen", "Forcer 16:10", "强制 16:10", "強制 16:10", "Forzar 16:10", "강제 16:10", "Forza 16:10", "Forçar 16:10", "16:10 बाध्य करें"},
        {"Force 32:9 (Super UltraWide)", "Принудительно 32:9 (Super UltraWide)", "إجبار 32:9", "32:9 erzwingen", "Forcer 32:9", "强制 32:9", "強制 32:9", "Forzar 32:9", "強制 32:9", "Forza 32:9 (Super UltraWide)", "Forçar 32:9 (Super UltraWide)", "32:9 बाध्य करें (Super UltraWide)"},
        {"Force 1:1 (Square)", "Принудительно 1:1 (Квадрат)", "إجبار 1:1", "1:1 erzwingen", "Forcer 1:1", "强制 1:1", "強制 1:1", "Forzar 1:1", "強制 1:1", "Forzar 1:1", "Forzar 1:1", "1:1 बाध्य करें (वर्गाकार)"},
        {"Force 3:2 (Portable / Surface)", "Принудительно 3:2 (Surface / Портатив)", "إجبار 3:2", "3:2 erzwingen", "Forcer 3:2", "强制 3:2", "強制 3:2", "Forzar 3:2", "強制 3:2", "Forzar 3:2", "Forzar 3:2", "3:2 बाध्य करें (पोर्टेबल / Surface)"},
        {"Force 5:4 (SXGA)", "Принудительно 5:4 (SXGA)", "إجبار 5:4", "5:4 erzwingen", "Forcer 5:4", "强制 5:4", "強制 5:4", "Forzar 5:4", "強制 5:4", "Forzar 5:4", "Forzar 5:4", "5:4 बाध्य करें (SXGA)"},
        {"Force 18:9 (2:1 Univisium)", "Принудительно 18:9 (2:1 Univisium)", "إجبار 18:9", "18:9 erzwingen", "Forcer 18:9", "强制 18:9", "強制 18:9", "Forzar 18:9", "強制 18:9", "Forza 18:9 (2:1 Univisium)", "Forçar 18:9 (2:1 Univisium)", "18:9 बाध्य करें (2:1 Univisium)"},
        {"Force 19.5:9 (Modern Mobile)", "Принудительно 19.5:9 (Смартфоны)", "إجبار 19.5:9", "19.5:9 erzwingen", "Forcer 19.5:9", "强制 19.5:9", "強制 19.5:9", "Forzar 19.5:9", "強制 19.5:9", "Forza 19.5:9 (Mobile)", "Forçar 19.5:9 (Mobile)", "19.5:9 बाध्य करें (आधुनिक मोबाइल)"},
        {"Force 20:9 (Cinema Mobile)", "Принудительно 20:9 (Киноформат)", "إجبار 20:9", "20:9 erzwingen", "Forcer 20:9", "强制 20:9", "強制 20:9", "Forzar 20:9", "強制 20:9", "Forza 20:9 (Cinema Mobile)", "Forçar 20:9 (Cinema Mobile)", "20:9 बाध्य करें (सिनेमा मोबाइल)"},
        {"Force 21:10 (Wide 16:10)", "Принудительно 21:10 (Широкий 16:10)", "إجبار 21:10", "21:10 erzwingen", "Forcer 21:10", "强制 21:10", "強制 21:10", "Forzar 21:10", "強制 21:10", "Forza 21:10 (Wide 16:10)", "Forçar 21:10 (Wide 16:10)", "21:10 बाध्य करें (वाइड 16:10)"},
        {"Force 24:10 (3840x1600 UltraWide+)", "Принудительно 24:10 (3840x1600 UltraWide+)", "إجبار 24:10", "24:10 erzwingen", "Forcer 24:10", "强制 24:10", "強制 24:10", "Forzar 24:10", "強制 24:10", "Forza 24:10 (3840x1600 UltraWide+)", "Forçar 24:10 (3840x1600 UltraWide+)", "24:10 बाध्य करें (3840x1600 UltraWide+)"},
        {"Force 32:10 (Super UltraWide 16:10)", "Принудительно 32:10 (Super UltraWide 16:10)", "إجبار 32:10", "32:10 erzwingen", "Forcer 32:10", "强制 32:10", "強制 32:10", "Forzar 32:10", "強制 32:10", "Forza 32:10 (Super UltraWide 16:10)", "Forçar 32:10 (Super UltraWide 16:10)", "32:10 बाध्य करें (Super UltraWide 16:10)"},
        {"Force 48:9 (Triple Monitor 3x16:9)", "Принудительно 48:9 (Три монитора 3x16:9)", "إجبار 48:9", "48:9 erzwingen", "Forcer 48:9", "强制 48:9", "強制 48:9", "Forzar 48:9", "強制 48:9", "Forza 48:9 (Triplo monitor 3x16:9)", "Forçar 48:9 (Monitor triplo 3x16:9)", "48:9 बाध्य करें (ट्रिपल मॉनिटर 3x16:9)"},
        {"Auto Window Aspect (True 3D Fit)", "Авто по размеру окна (Честный 3D-формат)", "تلقائي حسب النافذة", "Automatisch an Fenster anpassen (3D)", "Format auto de la fenêtre (3D)", "自动窗口比例 (3D)", "ウィンドウ比率に自動調整 (3D)", "Ajuste automático a la ventana (3D)", "자동 창 비율 (실제 3D 맞춤)", "Proporzioni automatiche finestra (3D reale)", "Proporção automática da janela (ajuste 3D real)", "ऑटो विंडो अनुपात (सच्चा 3D फ़िट)"},
        {"Auto-Patch 3D Aspect Ratio (No Stretch)", "Авто-патч 3D-камеры под формат экрана (без растяжения)", "تصحيح تلقائي لأبعاد 3D", "3D-Seitenverhältnis automatisch patchen", "Patch auto du format 3D", "自动修补 3D 宽高比", "3Dアスペクト比自動パッチ", "Parche automático de aspecto 3D", "3D 화면비 자동 패치 (늘어짐 없음)", "Patch automatica formato 3D (senza stiramento)", "Patch automático de proporção 3D (sem esticar)", "3D आस्पेक्ट रेशियो ऑटो-पैच (बिना खिंचाव)"},
        {"Auto-Unlock 60/120 FPS", "Авто-разблокировка 60/120 FPS", "فتح 60/120 إطار تلقائياً", "60/120 FPS automatisch entsperren", "Déverrouillage auto 60/120 FPS", "自动解锁 60/120 FPS", "60/120 FPS 自動ロック解除", "Desbloqueo automático de 60/120 FPS", "60/120 FPS 자동 잠금 해제", "Sblocco automatico 60/120 FPS", "Desbloqueio automático de 60/120 FPS", "60/120 FPS ऑटो-अनलॉक"},
        {"Nearest Neighbor", "Ближайший сосед", "أقرب جار", "Nächster Nachbar", "Plus proche voisin", "最近邻", "最近傍", "Vecino más cercano", "최근접 이웃", "Vicino più prossimo", "Vizinho mais próximo", "निकटतम पड़ोसी"},
        {"Bilinear", "Билинейный", "ثنائي الخطي", "Bilinear", "Bilinéaire", "双线性", "バイリニア", "Bilineal", "바이리니어", "Bilineare", "Bilinear (Suave)", "द्वि-रैखिक (Bilinear)"},
        {"Bicubic", "Бикубический", "ثنائي التكعيب", "Bikubisch", "Bicubique", "双三次", "バイキュービック", "Bicúbico", "바이큐빅", "Bicubico", "Bicúbico", "द्वि-घनीय (Bicubic)"},
        {"Gaussian", "Гауссов", "جاوسي", "Gauß", "Gaussien", "高斯", "ガウス", "Gaussiano", "가우시안", "Gaussiano", "Gaussiano", "गॉसियन (Gaussian)"},
        {"Lanczos", "Ланцош", "لانكزوس", "Lanczos", "Lanczos", "兰索斯", "ランチョス", "Lanczos", "Lanczos 3", "Lanczos 3", "Lanczos 3", "Lanczos (लैंक्ज़ोस)"},
        {"Japanese (日本語)", "Японский (日本語)", "اليابانية (日本語)", "Japanisch (日本語)", "Japonais (日本語)", "日语 (日本語)", "日本語", "Japonés (日本語)", "일본어(日本語)", "Giapponese  (日本語)", "Japonês (日本語)", "जापानी (日本語)"},
        {"American English", "Американский английский", "الإنجليزية الأمريكية", "Amerikanisches Englisch", "Anglais américain", "美式英语", "アメリカ英語", "Inglés estadounidense", "미국식 영어", "Inglese americano", "Inglês Americano", "अमेरिकी अंग्रेज़ी"},
        {"French (français)", "Французский (français)", "الفرنسية (français)", "Französisch (français)", "Français (français)", "法语 (français)", "フランス語 (français)", "Francés (français)", "프랑스어(français)", "Francese (français)", "Francês (français)", "फ़्रेंच (français)"},
        {"German (Deutsch)", "Немецкий (Deutsch)", "الألمانية (Deutsch)", "Deutsch", "Allemand (Deutsch)", "德语 (Deutsch)", "ドイツ語 (Deutsch)", "Alemán (Deutsch)", "독일어(Deutsch)", "Tedesco (Deutsch)", "Alemão (Deutsch)", "जर्मन (Deutsch)"},
        {"Italian (italiano)", "Итальянский (italiano)", "الإيطالية (italiano)", "Italienisch (italiano)", "Italien (italiano)", "意大利语 (italiano)", "イタリア語 (italiano)", "Italiano (italiano)", "이탈리아어(italiano)", "Italiano", "Italiano", "इतालवी (italiano)"},
        {"Spanish (español)", "Испанский (español)", "الإسبانية (español)", "Spanisch (español)", "Espagnol (español)", "西班牙语 (español)", "スペイン語 (español)", "Español (español)", "스페인어(español)", "Spagnolo (español)", "Espanhol (español)", "स्पेनिश (español)"},
        {"Chinese", "Китайский", "الصينية", "Chinesisch", "Chinois", "中文", "中国語", "Chino", "중국어", "Cinese", "Chinês", "चीनी"},
        {"Korean (한국어)", "Корейский (한국어)", "الكورية (한국어)", "Koreanisch (한국어)", "Coréen (한국어)", "韩语 (한국어)", "韓国語 (한국어)", "Coreano (한국어)", "한국어(Korean)", "Coreano (한국어)", "Coreano (한국어)", "कोरियाई (한국어)"},
        {"Dutch (Nederlands)", "Нидерландский (Nederlands)", "الهولندية (Nederlands)", "Niederländisch (Nederlands)", "Néerlandais (Nederlands)", "荷兰语 (Nederlands)", "オランダ語 (Nederlands)", "Holandés (Nederlands)", "네덜란드어(Nederlands)", "Olandese (Nederlands)", "Holandês (Nederlands)", "डच (Nederlands)"},
        {"Portuguese (português)", "Португальский (português)", "البرتغالية (português)", "Portugiesisch (português)", "Portugais (português)", "葡萄牙语 (português)", "ポルトガル語 (português)", "Portugués (português)", "포르투갈어(português)", "Portoghese (português)", "Português (português)", "पुर्तगाली (português)"},
        {"Russian (Русский)", "Русский (Русский)", "الروسية (Русский)", "Russisch (Русский)", "Russe (Русский)", "俄语 (Русский)", "ロシア語 (Русский)", "Ruso (Русский)", "러시아어(Русский)", "Russo (Русский)", "Russo (Русский)", "रूसी (Русский)"},
        {"Taiwanese", "Тайваньский", "التايوانية", "Taiwanisch", "Taïwanais", "繁体中文 (台湾)", "台湾語", "Taiwanés", "대만어", "Taiwanese", "Taiwanês", "ताइवानी चीनी"},
        {"British English", "Британский английский", "الإنجليزية البريطانية", "Britisches Englisch", "Anglais britannique", "英式英语", "イギリス英語", "Inglés británico", "영국식 영어", "Inglese britannico", "Inglês Britânico", "ब्रिटिश अंग्रेज़ी"},
        {"Simplified Chinese", "Упрощенный китайский", "الصينية المبسطة", "Vereinfachtes Chinesisch", "Chinois simplifié", "简体中文", "簡体字中国語", "Chino simplificado", "중국어 간체", "Cinese semplificato", "Chinês Simplificado", "सरलीकृत चीनी"},
        {"Traditional Chinese (正體中文)", "Традиционный китайский (正體中文)", "الصينية التقليدية (正體中文)", "Traditionelles Chinesisch (正體中文)", "Chinois traditionnel (正體中文)", "繁体中文 (正體中文)", "繁体字中国語 (正體中文)", "Chino tradicional (正體中文)", "중국어 번체(正體中文)", "Cinese tradizionale (正體中文)", "Chinês Tradicional (正 體 中文)", "पारंपरिक चीनी (正體中文)"},
        {"Brazilian Portuguese", "Бразильский португальский", "البرتغالية البرازيلية", "Brasilianisches Portugiesisch", "Portugais brésilien", "巴西葡萄牙语", "ブラジルポルトガル語", "Portugués brasileño", "브라질 포르투갈어", "Portoghese brasiliano", "Português brasileiro", "ब्राज़ीलियाई पुर्तगाली"},
        {"Brazilian Portuguese (português do Brasil)", "Бразильский португальский (português do Brasil)", "البرتغالية البرازيلية (português do Brasil)", "Brasilianisches Portugiesisch (português do Brasil)", "Portugais brésilien (português do Brasil)", "巴西葡萄牙语 (português do Brasil)", "ブラジルポルトガル語 (português do Brasil)", "Portugués brasileño (português do Brasil)", "브라질 포르투갈어(português do Brasil)", "Portoghese brasiliano (português do Brasil)", "Português do Brasil (Brazilian Portuguese)", "ब्राज़ीलियाई पुर्तगाली (português do Brasil)"},
        {"Latin American Spanish", "Латиноамериканский испанский", "الإسبانية الأمريكية اللاتينية", "Lateinamerikanisches Spanisch", "Espagnol d'Amérique latine", "拉美西班牙语", "ラテンアメリカスペイン語", "Español latinoamericano", "라틴아메리카 스페인어", "Spagnolo latino-americano", "Espanhol Latino-Americano", "लैटिन अमेरिकी स्पेनिश"},
        {"CPU", "ЦП", "معالج", "CPU", "CPU", "CPU", "CPU", "CPU", "CPU", "CPU", "CPU", "सीपीयू (CPU)"},
        {"GPU", "ГПУ", "معالج الرسوميات", "GPU", "GPU", "GPU", "GPU", "GPU", "GPU", "GPU", "GPU", "GPU (ग्राफ़िक्स)"},
        {"Emulate BGR565", "Эмуляция BGR565", "محاكاة BGR565", "BGR565 emulieren", "Émuler BGR565", "模拟 BGR565", "BGR565 をエミュレート", "Emular BGR565", "BGR565 에뮬레이션", "Emula BGR565", "Emular BGR565", "BGR565 एमुलेट करें"},
        {"Extended Dynamic State", "Расширенное динамическое состояние", "الحالة الديناميكية الموسعة", "Extended Dynamic State", "Extended Dynamic State", "扩展动态状态", "拡張ダイナミックステート", "Estado dinámico extendido", "확장된 동적 상태", "Stato dinamico esteso", "Estado Dinâmico Estendido", "विस्तारित गतिशील स्थिति"},
        {"Vertex Input Dynamic State", "Динамическое состояние ввода вершин", "الحالة الديناميكية لإدخال الرؤوس", "Vertex Input Dynamic State", "Vertex Input Dynamic State", "顶点输入动态状态", "頂点入力ダイナミックステート", "Estado dinámico de entrada de vértices", "정점 입력 동적 상태", "Stato dinamico input vertici", "Estado Dinâmico de Entrada de Vértices", "वर्टेक्स इनपुट डायनेमिक स्टेट"},
        {"Sample Shading", "Множественное затенение", "تظليل العينات", "Sample-Shading", "Ombrage d'échantillon", "采样着色", "サンプルシェーディング", "Sombreado de muestras", "샘플 셰이딩", "Sample shading", "Sombreamento por Amostragem", "सैंपल शेडिंग"},
        {"GPU Unswizzle", "Десвиззлинг ГПУ", "فك التفاف معالج الرسوميات", "GPU-Unswizzle", "GPU Unswizzle", "GPU 反交织", "GPU アンスウィズル", "Desentrelazado de GPU", "GPU 언스위즐", "Unswizzle GPU", "Desembaralhamento GPU (Unswizzle)", "GPU अनस्विज़ल (Unswizzle)"},
        {"GPU Unswizzle Max Texture Size", "Максимальный размер текстур для десвиззлинга ГПУ", "أقصى حجم للقوام لفك التفاف GPU", "Maximale Texturgröße für GPU-Unswizzle", "Taille max de texture pour GPU Unswizzle", "GPU 反交织最大纹理尺寸", "GPU アンスウィズル最大テクスチャサイズ", "Tamaño máximo de textura para GPU Unswizzle", "GPU 언스위즐 최대 텍스처 크기", "Dimensione massima texture Unswizzle GPU", "Tamanho Máximo de Textura para GPU Unswizzle", "GPU अनस्विज़ल अधिकतम टेक्सचर आकार"},
        {"GPU Unswizzle Stream Size", "Размер потока десвиззлинга ГПУ", "حجم تدفق فك التفاف GPU", "Stream-Größe für GPU-Unswizzle", "Taille du flux GPU Unswizzle", "GPU 反交织流大小", "GPU アンスウィズルストリームサイズ", "Tamaño de flujo de GPU Unswizzle", "GPU 언스위즐 스트림 크기", "Dimensione flusso Unswizzle GPU", "Tamanho do Fluxo de GPU Unswizzle", "GPU अनस्विज़ल स्ट्रीम आकार"},
        {"GPU Unswizzle Chunk Size", "Размер блока десвиззлинга ГПУ", "حجم كتلة فك التفاف GPU", "Blockgröße für GPU-Unswizzle", "Taille de bloc GPU Unswizzle", "GPU 反交织块大小", "GPU アンスウィズルチャンクサイズ", "Tamaño de fragmento de GPU Unswizzle", "GPU 언스위즐 청크 크기", "Dimensione blocco Unswizzle GPU", "Tamanho do Bloco de GPU Unswizzle", "GPU अनस्विज़ल चंक आकार"},
        {"Enables Horizon's built-in overlay applet. Press and hold the home button for 1 second to show it.", "Включает встроенный оверлей Horizon. Нажмите и удерживайте кнопку Home в течение 1 секунды для отображения.", "تفعيل تراكب Horizon المدمج. اضغط مع الاستمرار على زر الصفحة الرئيسية لمدة ثانية واحدة لإظهاره.", "Aktiviert das integrierte Horizon-Overlay-Applet. Halten Sie die Home-Taste 1 Sekunde lang gedrückt, um es anzuzeigen.", "Active l'applet de superposition intégrée d'Horizon. Maintenez le bouton Home enfoncé pendant 1 seconde pour l'afficher.", "启用 Horizon 内置叠加小程序。按住 Home 键 1 秒即可显示。", "Horizon の内蔵オーバーレイアプレットを有効化します。Home ボタンを1秒長押しして表示します。", "Habilita la superposición integrada de Horizon. Mantenga presionado el botón de inicio durante 1 segundo para mostrarlo.", "Horizon의 내장 오버레이 애플릿을 활성화합니다. HOME 버튼을 1초 동안 길게 누르면 애플릿이 나타납니다.", "Abilita l'applet overlay integrato di Horizon. Tieni premuto il pulsante HOME per 1 secondo per aprirlo.", "Ativa a sobreposição integrada do Horizon. Mantenha o botão Home pressionado por 1 segundo para abrir.", "Horizon के अंतर्निहित ओवरले एप्लेट को सक्षम करता है। इसे दिखाने के लिए HOME बटन को 1 सेकंड तक दबाए रखें।"},
        {"Synchronizes CPU core speed with the game's maximum rendering speed to boost FPS without affecting game speed (animations, physics, etc.).\nCan help reduce stuttering at lower framerates.", "Синхронизирует скорость ядер ЦП с максимальной скоростью рендеринга игры для повышения FPS без влияния на скорость игры (анимации, физика и т. д.).\nПомогает уменьшить рывки при низкой частоте кадров.", "مزامنة سرعة أنوية المعالج مع أقصى سرعة تصيير للعبة لتعزيز الإطارات دون التأثير على سرعة اللعبة.\nتساعد في تقليل التقطيع عند معدلات الإطارات المنخفضة.", "Synchronisiert die CPU-Kerngeschwindigkeit mit der maximalen Rendergeschwindigkeit, um FPS ohne Beeinflussung der Spielgeschwindigkeit zu steigern.\nKann Ruckler bei niedrigen Bildraten reduzieren.", "Synchronise la vitesse des cœurs CPU avec la vitesse maximale de rendu du jeu pour améliorer les FPS sans affecter la vitesse du jeu.\nPeut aider à réduire les saccades à faible fréquence d'images.", "将 CPU 核心速度与游戏最大渲染速度同步，在不影响游戏内部运行速度的前提下提升帧率。\n有助于减少低帧率时的微卡顿。", "ゲームの最大描画速度に CPU コア速度を同期させ、ゲーム自体の速度に影響を与えずに FPS を向上させます。\n低フレームレート時のカクつきを抑えます。", "Sincroniza la velocidad de los núcleos de CPU con la velocidad máxima de renderizado del juego para mejorar los FPS sin afectar la velocidad del juego.\nAyuda a reducir tirones a bajas tasas de fotogramas.", "CPU 코어 속도를 게임의 최대 렌더링 속도와 동기화하여 게임 속도(애니메이션, 물리 등)에 영향을 주지 않고 FPS를 향상시킵니다.\n낮은 프레임률에서 발생하는 끊김 현상을 줄이는 데 도움될 수 있습니다.", "Sincronizza la velocità dei core della CPU con la velocità massima di rendering del gioco, in modo da aumentare gli FPS senza impattare sulla velocità del gioco (animazioni, fisica, ecc.).\nPuò ridurre lo stuttering quando il framerate è basso.", "Sincroniza a velocidade do núcleo da CPU com a taxa máxima de renderização do jogo para aumentar o FPS sem afetar a velocidade do jogo (animações, física, etc.).\nPode ajudar a reduzir travamentos (engasgos) em taxas de quadros mais baixas.", "गेम की आंतरिक गति (एनीमेशन, भौतिकी आदि) को प्रभावित किए बिना FPS बढ़ाने के लिए CPU कोर गति को अधिकतम रेंडरिंग गति के साथ सिंक करता है।\nकम फ़्रेमरेट पर अटकने को कम करता है।"},
        {"Raises the clock the emulated CPU reports, which removes some FPS limiters.\nWeaker CPUs may see reduced performance, and certain games may behave improperly.", "Повышает тактовую частоту, которую сообщает эмулируемый ЦП, снимая некоторые ограничители FPS.\nНа слабых ЦП производительность может снизиться, а некоторые игры могут работать некорректно.", "يرفع تردد المعالج الذي يتم الإبلاغ عنه لمحاكاة أسرع، مما يزيل بعض محددات الإطارات.\nقد ينخفض الأداء على المعالجات الضعيفة وتتصرف بعض الألعاب بشكل غير صحيح.", "Erhöht den Takt, den die emulierte CPU meldet, was einige FPS-Begrenzungen aufhebt.\nSchwächere CPUs können Leistungseinbußen erleiden und manche Spiele reagieren fehlerhaft.", "Augmente la fréquence signalée par le processeur émulé, supprimant ainsi certains limiteurs de FPS.\nLes processeurs moins puissants peuvent voir leurs performances réduites.", "提高模拟 CPU 报告的时钟频率，以解除部分游戏的帧率限制。\n较弱的 CPU 可能会降低性能，某些游戏可能出现异常。", "エミュレートされた CPU が報告するクロックを引き上げ、一部の FPS 制限を解除します。\n低性能な CPU ではパフォーマンスが低下する場合があり、一部のゲームで不具合が生じる可能性があります。", "Aumenta la frecuencia de reloj que informa la CPU emulada, lo que elimina algunos limitadores de FPS.\nLas CPUs menos potentes pueden ver reducido su rendimiento.", "에뮬레이션된 CPU가 보고하는 클럭을 높여 일부 FPS 제한을 해제합니다.\n저사양 CPU에서는 성능이 저하되거나 일부 게임이 오작동할 수 있습니다.", "Aumenta la frequenza segnalata dalla CPU emulata, rimuovendo alcuni limitatori di FPS.\nLe CPU più deboli potrebbero subire cali di prestazioni.", "Aumenta o clock reportado pela CPU emulada, removendo alguns limitadores de FPS.\nCPUs mais fracas podem perder desempenho.", "एमुलेटेड CPU द्वारा रिपोर्ट की गई क्लॉक को बढ़ाता है, जिससे कुछ FPS सीमाएँ हट जाती हैं।\nकमज़ोर CPU पर प्रदर्शन घट सकता है।"},
        {"Specifies how videos should be decoded.\nIt can either use the CPU or the GPU for decoding, or perform no decoding at all (black screen on videos).\nIn most cases, GPU decoding provides the best performance.", "Определяет способ декодирования видео.\nМожет использовать ЦП или ГПУ для декодирования, либо отключить его полностью (черный экран в видеороликах).\nВ большинстве случаев декодирование на ГПУ обеспечивает наилучшую производительность.", "يحدد كيفية فك تشفير الفيديو.\nيمكن استخدام المعالج أو معالج الرسوميات، أو عدم فك التشفير إطلاقاً.\nفي معظم الحالات، يوفر فك التشفير عبر الرسوميات أفضل أداء.", "Gibt an, wie Videos dekodiert werden sollen.\nKann CPU oder GPU nutzen oder die Dekodierung deaktivieren.\nIn den meisten Fällen bietet die GPU-Dekodierung die beste Leistung.", "Indique le mode de décodage des vidéos.\nPeut utiliser le processeur ou le processeur graphique, ou désactiver le décodage.\nDans la plupart des cas, le décodage GPU offre les meilleures performances.", "指定视频解码方式。\n可选择使用 CPU 或 GPU 进行解码，或完全不进行解码 (视频黑屏)。\n大多数情况下，GPU 解码性能最佳。", "動画のデコード方法を指定します。\nCPU または GPU を使用するか、デコードを行わない（動画が黒画面になります）かを選択できます。\n通常は GPU デコードが最も高いパフォーマンスを発揮します。", "Especifica cómo deben decodificarse los videos.\nPuede usar la CPU o la GPU, o no realizar decodificación.\nEn la mayoría de los casos, la decodificación por GPU ofrece el mejor rendimiento.", "비디오를 디코딩하는 방법을 지정합니다.\n디코딩에 CPU 또는 GPU를 사용할 수 있으며, 디코딩을 전혀 수행하지 않을 수도 있습니다(비디오가 검은 화면으로 표시됨).\n대부분의 경우 GPU 디코딩이 가장 좋은 성능을 제공합니다.", "Specifica come i video devono essere decodificati.\nÈ possibile utilizzare sia la CPU o la GPU per la decodifica, o addirittura non decodificare proprio (mostrando uno schermo nero al posto dei video).\nNella maggior parte dei casi, la decodifica tramite GPU fornisce prestazioni migliori.", "Especifica como os vídeos devem ser decodificados.\nTanto a CPU quanto a GPU podem ser utilizadas para decodificação, ou não decodificar nada (tela preta nos vídeos).\nNa maioria dos casos, a decodificação pela GPU fornece um melhor desempenho.", "निर्दिष्ट करता है कि वीडियो कैसे डिकोड किए जाएँ (CPU, GPU या कोई आउटपुट नहीं)।\nअधिकांश मामलों में GPU डिकोडिंग सर्वोत्तम प्रदर्शन देती है।"},
        {"This option controls how ASTC textures should be decoded.\nCPU: Use the CPU for decoding.\nGPU: Use the GPU's compute shaders to decode ASTC textures (recommended).\nCPU Asynchronously: Use the CPU to decode ASTC textures on demand. EliminatesASTC decoding\nstuttering but may present artifacts.", "Определяет метод декодирования текстур ASTC.\nЦП: декодирование силами ЦП.\nГПУ: декодирование вычислительными шейдерами ГПУ (рекомендуется).\nЦП (Асинхронно): декодирование на ЦП по требованию. Устраняет заикания, но возможны временные артефакты.", "يحدد هذا الخيار كيفية فك تشفير قوام ASTC.\nالمعالج: فك التشفير عبر المعالج.\nمعالج الرسوميات: استخدام شيدرات الحساب (موصى به).\nغير متزامن: فك التشفير حسب الطلب.", "Steuert, wie ASTC-Texturen dekodiert werden.\nCPU: Dekodierung durch CPU.\nGPU: Compute-Shader der GPU verwenden (empfohlen).\nCPU asynchron: Dekodierung bei Bedarf.", "Contrôle la méthode de décodage des textures ASTC.\nCPU : Décodage par le processeur.\nGPU : Shaders de calcul GPU (recommandé).\nCPU asynchrone : Décodage à la demande.", "控制 ASTC 纹理的解码方式。\nCPU: 使用 CPU 进行解码。\nGPU: 使用 GPU 计算着色器解码 (推荐)。\nCPU 异步: 按需在 CPU 上解码，消除卡顿但可能出现短暂画面瑕疵。", "ASTC テクスチャのデコード方法を制御します。\nCPU: CPU でデコードします。\nGPU: GPU コンピュートシェーダーを使用します (推奨)。\nCPU 非同期: 必要に応じてデコードし、スタッターを解消しますが一時的な乱れが生じる場合があります。", "Controla cómo se decodifican las texturas ASTC.\nCPU: Usa la CPU para decodificar.\nGPU: Usa shaders de cómputo de la GPU (recomendado).\nCPU asíncrona: Decodifica bajo demanda.", "이 옵션은 ASTC 텍스처를 디코딩하는 방식을 제어합니다.\nCPU: CPU를 사용하여 디코딩합니다.\nGPU: GPU의 컴퓨트 셰이더를 사용하여 ASTC 텍스처를 디코딩합니다(권장).\nCPU 비동기: 필요에 따라 CPU를 사용하여 ASTC 텍스처를 디코딩합니다. ASTC 디코딩으로 인한 끊김 현상이 없어지지만, 아티팩트가 발생할 수 있습니다.", "Quest'opzione controlla come le texture ASTC vengono decodificate.\nCPU: Usa la CPU per la decodifica.\nGPU: Usa i compute shader della GPU per decodificare le texture ASTC (consigliato).\nCPU (Asincrono): Usa la CPU per decodificare le texture ASTC se richiesto. Elimina lo stuttering causato dalla decodifica ma potrebbe generare artefatti visivi.", "Esta opção controla como as texturas ASTC devem ser decodificadas.\nCPU: Usa a CPU para a decodificação.\nGPU: Usa os compute shaders da GPU para decodificar texturas ASTC (recomendado).\nCPU Assíncrona: Usa a CPU para decodificar texturas ASTC sob demanda. Elimina travamentos (engasgos) de decodificação ASTC,\nmas pode apresentar artefatos visuais.", "नियंत्रित करता है कि ASTC टेक्सचर कैसे डिकोड किए जाएँ।\nCPU: CPU द्वारा डिकोडिंग।\nGPU: GPU कंप्यूट शेडर द्वारा डिकोडिंग (अनुशंसित)।\nCPU असिंक्रोनस: अटकने को समाप्त करता है लेकिन अस्थायी दृश्य त्रुटियाँ दिखा सकता है।"},
        {"Makes the game believe GPU work finishes faster than it does, so it stops lowering resolution and render distance to fit the Switch's clocks.", "Заставляет игру считать, что задачи ГПУ выполняются быстрее, предотвращая снижение динамического разрешения и дальности прорисовки.", "يجعل اللعبة تعتقد أن عمل معالج الرسوميات ينتهي أسرع، مما يمنعها من خفض الدقة ومسافة الرؤية.", "Täuscht dem Spiel vor, dass die GPU-Arbeit schneller beendet ist, wodurch das Absenken von Auflösung und Sichtweite verhindert wird.", "Fait croire au jeu que le rendu GPU est plus rapide, évitant ainsi la baisse de résolution et de distance d'affichage.", "使游戏认为 GPU 工作比实际更快完成，从而防止其自动降低动态分辨率和渲染视距。", "GPU の処理が実際よりも速く完了したとゲームに認識させ、動的解像度や描画距離の低下を防ぎます。", "Hace que el juego crea que el trabajo de la GPU termina más rápido, evitando que baje la resolución dinámica y la distancia de dibujado.", "GPU 작업이 실제보다 빨리 완료된 것처럼 게임이 인식하게 하여 Switch 클럭에 맞춘 해상도 및 가시거리 저하를 방지합니다.", "Hace que el juego crea que el trabajo de la GPU termina más rápido, evitando que baje la resolución dinámica y la distancia de dibujado.", "Hace que el juego crea que el trabajo de la GPU termina más rápido, evitando que baje la resolución dinámica y la distancia de dibujado.", "Makes the game believe GPU work finishes faster than it does, so it stops lowering रिज़ॉल्यूशन and render distance to fit the Switch's clocks."},
        {"Pins emulation threads to physical performance cores.", "Привязывает потоки эмуляции к физическим производительным ядрам ЦП.", "تثبيت خيوط المحاكاة على أنوية الأداء الفعلية.", "Pinnt Emulations-Threads an physische Leistungskerne.", "Assigne les threads d'émulation aux cœurs de performance physiques.", "将模拟线程绑定到物理性能核心 (P-Cores)。", "エミュレーションスレッドを物理的な高性能コアに固定します。", "Fija los hilos de emulación a núcleos de rendimiento físicos.", "에뮬레이션 스레드를 물리 고성능 코어에 고정합니다.", "Assegna i thread di emulazione ai core fisici ad alte prestazioni.", "Fixa as threads de emulação nos núcleos físicos de alto desempenho.", "एमुलेशन थ्रेड्स को भौतिक उच्च-प्रदर्शन CPU कोर पर पिन करता है।"},
        {"Reduces power draw and frame stuttering with adaptive delivery.", "Снижает энергопотребление и рывки кадров за счет адаптивной подачи.", "يقلل استهلاك الطاقة وتقطيع الإطارات عبر التقديم التكيفي.", "Reduziert Stromverbrauch und Frame-Ruckler durch adaptive Bereitstellung.", "Réduit la consommation d'énergie et les saccades grâce à une distribution adaptative des images.", "通过自适应帧递送降低能耗并消除画面微卡顿。", "適応型フレーム配信により消費電力を抑え、画面のカクつきを低減します。", "Reduce el consumo de energía y los tirones con entrega adaptativa de fotogramas.", "적응형 프레임 전달로 전력 소모와 끊김을 줄입니다.", "Riduce il consumo energetico e gli scatti grazie alla distribuzione adattiva dei fotogrammi.", "Reduz o consumo de energia e engasgos com entrega adaptativa de quadros.", "अनुकूली फ़्रेम वितरण के साथ बिजली की खपत और अटकने को कम करता है।"},
        {"Locks dynamic resolution scale to prevent resolution drops.", "Блокирует динамическое масштабирование разрешения, предотвращая падение четкости.", "قفل مقياس الدقة الديناميكي لمنع هبوط الجودة.", "Sperrt die dynamische Auflösungsskalierung, um ein Absinken der Schärfe zu verhindern.", "Verrouille l'échelle de résolution dynamique pour éviter les chutes de netteté.", "锁定动态分辨率缩放，防止画质分辨率下降。", "動的解像度スケーリングを固定し、解像度の低下を防止します。", "Bloquea la escala de resolución dinámica para evitar caídas de resolución.", "해상도 저하를 방지하기 위해 동적 해상도 배율을 고정합니다.", "Blocca la scala di risoluzione dinamica per evitare cali di nitidezza.", "Trava a escala de resolução dinâmica para evitar quedas de nitidez.", "रिज़ॉल्यूशन गिरने से रोकने के लिए डायनेमिक रिज़ॉल्यूशन स्केल को लॉक करता है।"},
        {"Dynamically regulates priority of background shader compilation threads to prevent microstutters.", "Динамически регулирует приоритет фоновых потоков компиляции шейдеров для предотвращения микрофризов.", "تنظيم أولوية خيوط تجميع الشيدر في الخلفية ديناميكياً لمنع التقطيع البسيط.", "Regelt dynamisch die Priorität von Shader-Kompilierungs-Threads im Hintergrund zur Vermeidung von Mikrorucklern.", "Régule dynamiquement la priorité des threads de compilation de shaders en arrière-plan pour éviter les micro-saccades.", "动态调节后台着色器编译线程的优先级，消除微卡顿。", "バックグラウンドのシェーダーコンパイルスレッドの優先度を動的に調整し、マイクロスタッターを防ぎます。", "Regula dinámicamente la prioridad de los hilos de compilación de shaders para evitar microtirones.", "미세 끊김을 방지하기 위해 백그라운드 셰이더 컴파일 스레드의 우선순위를 동적으로 조절합니다.", "Regola dinamicamente la priorità dei thread di compilazione shader in background per evitare micro-scatti.", "Regula dinamicamente a prioridade das threads de compilação de shaders em segundo plano para evitar microengasgos.", "सूक्ष्म अटकाव को रोकने के लिए पृष्ठभूमि शेडर संकलन थ्रेड्स की प्राथमिकता को गतिशील रूप से नियंत्रित करता है।"},
        {"Periodic background collection of unused texture buffers and ASTC cache to prevent VRAM leaks.", "Периодическая фоновая очистка неиспользуемых текстурных буферов и кэша ASTC для предотвращения утечек видеопамяти.", "تنظيف دوري في الخلفية لمخازن القوام غير المستخدمة وذاكرة ASTC لتجنب تسرب ذاكرة الرسوميات.", "Regelmäßige Hintergrundbereinigung ungenutzter Texturpuffer und des ASTC-Caches gegen VRAM-Lecks.", "Nettoyage périodique en arrière-plan des tampons de texture inutilisés et du cache ASTC pour éviter les fuites de VRAM.", "定期在后台回收未使用的纹理缓冲区和 ASTC 缓存，防止显存泄漏。", "未使用のテクスチャバッファと ASTC キャッシュをバックグラウンドで定期解放し、VRAM リークを防ぎます。", "Limpieza periódica en segundo plano de búferes de textura no utilizados y caché ASTC para evitar fugas de VRAM.", "VRAM 누수를 방지하기 위해 사용하지 않는 텍스처 버퍼와 ASTC 캐시를 주기적으로 정리합니다.", "Pulizia periodica in background dei buffer texture inutilizzati e della cache ASTC per prevenire perdite di VRAM.", "Limpeza periódica em segundo plano de buffers de textura não utilizados e cache ASTC para evitar vazamento de VRAM.", "VRAM लीक रोकने के लिए अप्रयुक्त टेक्सचर बफ़र्स और ASTC कैश की समय-समय पर पृष्ठभूमि सफ़ाई करता है।"},
        {"Prevents out-of-memory crashes by unloading textures when VRAM exceeds 85%.", "Предотвращает вылеты из-за нехватки памяти, выгружая неактивные текстуры при заполнении видеопамяти выше 85%.", "يمنع الانهيارات بسبب نفاد الذاكرة عن طريق تفريغ القوام عندما يتجاوز استخدام VRAM نسبة 85%.", "Verhindert Abstürze durch Speichermangel, indem Texturen entladen werden, wenn der VRAM zu über 85 % belegt ist.", "Empêche les plantages par manque de mémoire en déchargeant les textures lorsque l'utilisation de la VRAM dépasse 85 %.", "当显存占用超过 85% 时自动卸载非活动纹理，防止内存不足导致闪退崩溃。", "VRAM の使用率が85%を超えた際に非アクティブなテクスチャをアンロードし、メモリ不足によるクラッシュを防ぎます。", "Evita cierres por falta de memoria descargando texturas inactivas cuando el uso de VRAM supera el 85%.", "VRAM 사용량이 85%를 초과하면 비활성 텍스처를 언로드하여 메모리 부족 충돌을 방지합니다.", "Previene arresti anomali per memoria esaurita scaricando le texture inattive oltre l'85% di VRAM.", "Evita travamentos por falta de memória descarregando texturas inativas quando a VRAM ultrapassa 85%.", "VRAM उपयोग 85% से अधिक होने पर निष्क्रिय टेक्सचर अनलोड करके मेमोरी की कमी से होने वाले क्रैश को रोकता है।"},
        {"Maximizes smoothness on entry-level multi-core processors and weak GPUs.", "Максимизирует плавность работы на начальных многоядерных процессорах и слабых ГПУ.", "زيادة السلاسة إلى أقصى حد على المعالجات متعددة الأنوية الاقتصادية ومعالجات الرسوميات الضعيفة.", "Maximiert die Flüssigkeit auf Einstiegs-Mehrkernprozessoren und schwachen GPUs.", "Maximise la fluidité sur les processeurs multicœurs d'entrée de gamme et les cartes graphiques modestes.", "在入门级多核处理器和弱 GPU 上最大化帧率平滑度。", "エントリー向けマルチコア CPU や低スペック GPU での滑らかさを最大化します。", "Maximiza la fluidez en procesadores multinúcleo básicos y GPUs de gama baja.", "보급형 멀티코어 프로세서 및 저사양 GPU에서 프레임 매끄러움을 극대화합니다.", "Massimizza la fluidità sui processori multicore entry-level e sulle GPU meno potenti.", "Maximiza a fluidez em processadores multinúcleo de entrada e GPUs modestas.", "एंट्री-लेवल मल्टी-कोर प्रोसेसर और कमज़ोर GPU पर सुगमता को अधिकतम करता है।"},
        {"Post-Processing Shader:", "Шейдер постобработки:", "مظلل المعالجة اللاحقة:", "Post-Processing-Shader:", "Shader de post-traitement :", "后期处理着色器：", "ポストプロセスシェーダー:", "Shader de posprocesamiento:", "후처리 셰이더:", "Shader di post-elaborazione:", "Shader de pós-processamento:", "पोस्ट-प्रोसेसिंग शेडर:"},
        {"Applies a RetroArch post-processing shader effect (CRT, scanlines, sharpening, color grading, bloom) to the final rendered image.", "Применяет экранный шейдер постобработки RetroArch (ЭЛТ-монитор, сканлайны, повышение чёткости, цветокоррекция, свечение) к итоговому изображению.", "تطبيق تأثير مظلل المعالجة اللاحقة RetroArch على الصورة النهائية.", "Wendet einen RetroArch-Post-Processing-Shader (CRT, Scanlines, Schärfung, Farbkorrektur) auf das fertige Bild an.", "Applique un shader de post-traitement RetroArch (CRT, scanlines, netteté, étalonnage) à l'image finale.", "对最终渲染画面应用 RetroArch 后期处理着色器效果（CRT、扫描线、锐化、调色、泛光）。", "最終描画映像に RetroArch ポストプロセスシェーダー効果（CRT、スキャンライン、シャープ化、色補正）を適用します。", "Aplica un shader de posprocesamiento RetroArch (CRT, líneas de escaneo, nitidez, etalonaje) a la imagen final.", "최종 렌더링 화면에 RetroArch 후처리 셰이더 효과(CRT, 스캔라인, 선명화, 색보정, 블룸)를 적용합니다.", "Applica un effetto shader di post-elaborazione RetroArch (CRT, scanline, nitidezza, color grading, bloom) all'immagine finale.", "Aplica um efeito de shader de pós-processamento RetroArch (CRT, scanlines, nitidez, gradação de cor, bloom) à imagem final.", "अंतिम रेंडर की गई छवि पर RetroArch पोस्ट-प्रोसेसिंग शेडर प्रभाव (CRT, स्कैनलाइन, शार्पनिंग, कलर ग्रेडिंग, ब्लूम) लागू करता है।"},
        {"Use Optimized Vertex Buffers", "Оптимизированные вершинные буферы", "استخدام مخازن الرؤوس المحسنة", "Optimierte Vertex-Puffer verwenden", "Utiliser des tampons de sommets optimisés", "使用优化的顶点缓冲区", "最適化された頂点バッファを使用", "Usar búferes de vértices optimizados", "최적화된 정점 버퍼 사용", "Usa buffer vertici ottimizzati", "Usar buffers de vértices otimizados", "अनुकूलित वर्टेक्स बफ़र का उपयोग करें"},
        {"Optimizes vertex buffer memory allocations and uploads in Vulkan to reduce CPU overhead and improve geometry throughput.", "Оптимизирует выделение памяти и загрузку вершинных буферов в Vulkan для снижения нагрузки на ЦП и ускорения обработки геометрии.", "تحسين تخصيص وتحميل مخازن الرؤوس في Vulkan لتقليل حمل المعالج وتسريع الهندسة.", "Optimiert die Speicherzuweisung und das Hochladen von Vertex-Puffern in Vulkan, um die CPU-Last zu senken.", "Optimise l'allocation et le transfert des tampons de sommets sous Vulkan pour réduire la charge CPU.", "优化 Vulkan 中的顶点缓冲区内存分配与上传，降低 CPU 开销并提升几何吞吐量。", "Vulkan における頂点バッファのメモリ割り当てとアップロードを最適化し、CPU 負荷を軽減します。", "Optimiza la asignación y carga de búferes de vértices en Vulkan para reducir la carga de la CPU.", "Vulkan에서 정점 버퍼 메모리 할당 및 업로드를 최적화하여 CPU 부하를 줄이고 지오메트리 처리량을 향상합니다.", "Ottimizza l'allocazione e il caricamento dei buffer dei vertici in Vulkan per ridurre il carico CPU e migliorare la geometria.", "Otimiza a alocação e envio de buffers de vértices no Vulkan para reduzir o uso da CPU e acelerar a geometria.", "CPU ओवरहेड कम करने और ज्योमेट्री थ्रूपुट बढ़ाने के लिए Vulkan में वर्टेक्स बफ़र मेमोरी आवंटन और अपलोड को अनुकूलित करता है।"},
        {"Reduces CPU and GPU temperatures by 10-15°C during frame wait intervals without dropping target frame rate.", "Снижает температуру ЦП и ГПУ на 10-15°C во время интервалов ожидания кадров без падения целевого фреймрейта.", "تقليل حرارة المعالج والرسوميات بمقدار 10-15 درجة أثناء فترات انتظار الإطارات دون انخفاض معدل الإطارات.", "Senkt CPU- und GPU-Temperaturen in Frame-Warteintervallen um 10-15 °C, ohne die Zielbildrate zu verringern.", "Réduit les températures CPU et GPU de 10 à 15 °C pendant les temps d'attente d'images sans baisser la fréquence cible.", "在帧等待间隙将 CPU 和 GPU 温度降低 10-15°C，且完全不降低目标帧率。", "フレーム待機中にターゲットフレームレートを維持したまま、CPU と GPU の温度を 10〜15℃ 低減させます。", "Reduce la temperatura de CPU y GPU en 10-15 °C durante los intervalos de espera de fotogramas sin perder fluidez.", "목표 프레임레이트를 떨어뜨리지 않고 프레임 대기 구간 동안 CPU 및 GPU 온도를 10~15°C 낮춥니다.", "Riduce le temperature di CPU e GPU di 10-15 °C durante gli intervalli di attesa dei fotogrammi senza perdere FPS.", "Reduz as temperaturas da CPU e GPU em 10-15 °C durante os intervalos de espera de quadros sem perder FPS.", "लक्ष्य फ़्रेम दर गिराए बिना फ़्रेम प्रतीक्षा अंतराल के दौरान CPU और GPU के तापमान को 10-15°C तक कम करता है।"},
        {"Locks maximum performance for CPU and GPU, disables aggressive throttling and power saving.", "Фиксирует максимальную производительность для ЦП и ГПУ, отключает агрессивный троттлинг и энергосбережение.", "تثبيت أقصى أداء للمعالج والرسوميات وتعطيل وضع توفير الطاقة والحد من التردد.", "Sperrt die maximale Leistung für CPU und GPU und deaktiviert Drosselung sowie Energiesparmodi.", "Verrouille les performances maximales pour le CPU et le GPU, désactivant l'économie d'énergie agressive.", "锁定 CPU 和 GPU 的最高性能状态，禁用激进降频和节能限制。", "CPU と GPU を最高パフォーマンス状態に固定し、省電力やサーマルスロットリングを無効化します。", "Fija el rendimiento máximo para CPU y GPU, deshabilitando el ahorro de energía agresivo.", "CPU와 GPU를 최고 성능 상태로 고정하고 공격적인 스로틀링 및 절전 제한을 비활성화합니다.", "Blocca CPU e GPU alle massime prestazioni, disattivando il risparmio energetico e il throttling aggressivo.", "Trava CPU e GPU no desempenho máximo, desativando economia de energia e limitação térmica agressiva.", "CPU और GPU को अधिकतम प्रदर्शन पर लॉक करता है तथा आक्रामक थ्रॉटलिंग और पावर सेविंग को अक्षम करता है।"},
        {"Enables HDR10 (BT.2020 PQ / ST2084) color space for compatible HDR and OLED displays.", "Включает цветовое пространство HDR10 (BT.2020 PQ / ST2084) для совместимых дисплеев HDR и OLED.", "تفعيل مساحة الألوان HDR10 للشاشات المتوافقة.", "Aktiviert den HDR10-Farbraum (BT.2020 PQ / ST2084) für kompatible HDR- und OLED-Bildschirme.", "Active l'espace colorimétrique HDR10 (BT.2020 PQ / ST2084) pour les écrans compatibles.", "为兼容的 HDR 和 OLED 显示器启用 HDR10 (BT.2020 PQ / ST2084) 色彩空间。", "対応する HDR および OLED ディスプレイ向けに HDR10 (BT.2020 PQ / ST2084) 色空間を有効化します。", "Habilita el espacio de color HDR10 (BT.2020 PQ / ST2084) para pantallas compatibles.", "호환되는 HDR 및 OLED 디스플레이에 대해 HDR10(BT.2020 PQ / ST2084) 색 공간을 활성화합니다.", "Abilita lo spazio colore HDR10 (BT.2020 PQ / ST2084) per schermi HDR e OLED compatibili.", "Ativa o espaço de cores HDR10 (BT.2020 PQ / ST2084) para telas HDR e OLED compatíveis.", "संगत HDR और OLED डिस्प्ले के लिए HDR10 (BT.2020 PQ / ST2084) कलर स्पेस सक्षम करता है।"},
        {"Emulates BGR565 color format by software swapping red and blue channels.", "Эмулирует цветовой формат BGR565 путем программной перестановки красного и синего каналов.", "محاكاة تنسيق الألوان BGR565 عن طريق تبديل القنوات الحمراء والزرقاء برمجياً.", "Emuliert das BGR565-Farbformat durch softwarebasiertes Vertauschen von Rot- und Blaukanal.", "Émule le format de couleur BGR565 par permutation logicielle des canaux rouge et bleu.", "通过软件置换红蓝通道来模拟 BGR565 颜色格式。", "赤と青のチャンネルをソフトウェアで入れ替えることで BGR565 カラーフォーマットを再現します。", "Emula el formato de color BGR565 intercambiando por software los canales rojo y azul.", "빨간색과 파란색 채널을 소프트웨어로 교체하여 BGR565 색상 포맷을 에뮬레이션합니다.", "Emula il formato colore BGR565 scambiando via software i canali rosso e blu.", "Emula o formato de cor BGR565 trocando por software os canais vermelho e azul.", "लाल और नीले चैनलों की सॉफ़्टवेयर अदला-बदली करके BGR565 रंग प्रारूप का अनुकरण करता है।"},
        {"Releases synchronization fences earlier to reduce frame presentation latency.", "Освобождает барьеры синхронизации раньше для снижения задержки вывода кадров.", "تحرير حواجز المزامنة مبكراً لتقليل زمن استجابة عرض الإطارات.", "Gibt Synchronisations-Fences früher frei, um die Latenz bei der Bildausgabe zu verringern.", "Libère les barrières de synchronisation plus tôt pour réduire la latence d'affichage des images.", "更早释放同步栅栏 (Fences)，以降低画面呈现的输入延迟。", "同期フェンスを早期に解放し、フレーム表示の遅延を低減します。", "Libera las barreras de sincronización antes para reducir la latencia de presentación de fotogramas.", "프레임 표시 지연 시간을 줄이기 위해 동기화 펜스를 조기에 해제합니다.", "Rilascia anticipatamente le fence di sincronizzazione per ridurre la latenza di presentazione dei fotogrammi.", "Libera as cercas de sincronização mais cedo para reduzir a latência de exibição de quadros.", "फ़्रेम प्रस्तुति विलंबता कम करने के लिए सिंक्रनाइज़ेशन फ़ेंस को पहले रिलीज़ करता है।"},
        {"Runs optimization passes on compiled SPIR-V shaders.", "Выполняет проходы оптимизации для скомпилированных шейдеров SPIR-V.", "تشغيل مراحل التحسين على شيدرات SPIR-V المجمعة.", "Führt Optimierungsdurchläufe auf kompilierten SPIR-V-Shadern aus.", "Exécute des passes d'optimisation sur les shaders SPIR-V compilés.", "对编译后的 SPIR-V 着色器执行优化通道。", "コンパイルされた SPIR-V シェーダーに対して最適化パスを実行します。", "Ejecuta pasadas de optimización en shaders SPIR-V compilados.", "컴파일된 SPIR-V 셰이더에 최적화 패스를 실행합니다.", "Esegue passaggi di ottimizzazione sugli shader SPIR-V compilati.", "Executa etapas de otimização nos shaders SPIR-V compilados.", "संकलित SPIR-V शेडर्स पर अनुकूलन पास चलाता है।"},
        {"Uses fast monotonic GPU timer queries for frame pacing.", "Использует быстрые монотонные запросы таймера ГПУ для выравнивания кадров.", "استخدام استعلامات توقيت GPU رتيبة سريعة لضبط الإطارات.", "Verwendet schnelle monotone GPU-Timer-Abfragen für die Frame-Taktung.", "Utilise des requêtes de minuterie GPU monotones et rapides pour le calage des images.", "使用快速单调 GPU 计时器查询进行帧平滑。", "高速な単調増加 GPU タイマークエリを使用してフレームペーシングを行います。", "Utiliza consultas de temporizador GPU monótonas y rápidas para el ritmo de fotogramas.", "프레임 페이싱을 위해 빠른 단조 GPU 타이머 쿼리를 사용합니다.", "Usa query rapide del timer monotono GPU per il frame pacing.", "Usa consultas rápidas de temporizador monotônico da GPU para ritmo de quadros.", "फ़्रेम पेसिंग के लिए तेज़ मोनोटोनिक GPU टाइमर क्वेरी का उपयोग करता है।"},
        {"Skips rendering non-critical frames when emulation falls behind target rate.", "Пропускает рендеринг некритичных кадров при отставании эмуляции от целевой скорости.", "تخطي تصيير الإطارات غير الحرجة عندما يتأخر المحاكي عن السرعة المطلوبة.", "Überspringt das Rendern unkritischer Frames, wenn die Emulation hinter die Zielrate zurückfällt.", "Ignore le rendu des images non critiques lorsque l'émulation prend du retard sur la cadence cible.", "当模拟进度落后于目标帧率时，跳过非关键帧的渲染。", "エミュレーションが目標速度に追いつかない場合、重要度の低いフレームの描画をスキップします。", "Omite el renderizado de fotogramas no críticos cuando la emulación se retrasa respecto a la velocidad objetivo.", "에뮬레이션 속도가 목표 속도보다 뒤처질 때 중요하지 않은 프레임 렌더링을 건너뜁니다.", "Salta il rendering dei fotogrammi non critici quando l'emulazione scende sotto la velocità target.", "Pula a renderização de quadros não críticos quando a emulação fica abaixo da taxa alvo.", "जब एमुलेशन लक्ष्य दर से पीछे रह जाता है तो गैर-महत्वपूर्ण फ़्रेमों की रेंडरिंग छोड़ देता है।"},
        {"Reduces power consumption and fan noise on portable devices.", "Снижает энергопотребление и шум вентилятора на портативных устройствах.", "تقليل استهلاك الطاقة وضوضاء المروحة على الأجهزة المحمولة.", "Reduziert Stromverbrauch und Lüftergeräusche auf tragbaren Geräten.", "Réduit la consommation d'énergie et le bruit du ventilateur sur les appareils portables.", "降低便携式设备的功耗与风扇噪音。", "ポータブルデバイスでの消費電力とファンの騒音を抑制します。", "Reduce el consumo de energía y el ruido del ventilador en dispositivos portátiles.", "휴대용 기기에서 전력 소모와 팬 소음을 줄입니다.", "Riduce il consumo energetico e il rumore delle ventole sui dispositivi portatili.", "Reduz o consumo de energia e o ruído da ventoinha em dispositivos portáteis.", "पोर्टेबल उपकरणों पर बिजली की खपत और पंखे के शोर को कम करता है।"},
        {"Displays floating quick translation overlay button.", "Отображает плавающую кнопку быстрого перевода.", "عرض زر تراكب الترجمة السريعة العائم.", "Zeigt einen schwebenden Schnellübersetzungs-Button an.", "Affiche un bouton flottant de traduction rapide.", "在界面上显示悬浮快速翻译按钮。", "フローティングクイック翻訳ボタンを表示します。", "Muestra un botón flotante de traducción rápida.", "빠른 화면 번역을 위한 플로팅 오버레이 버튼을 표시합니다.", "Mostra il pulsante flottante per la traduzione rapida su schermo.", "Exibe o botão flutuante de sobreposição para tradução rápida.", "त्वरित स्क्रीन अनुवाद के लिए फ़्लोटिंग ओवरले बटन प्रदर्शित करता है।"},
        {"Dynamic performance scaler", "Динамическое масштабирование производительности", "مقياس الأداء الديناميكي", "Dynamische Leistungsskalierung", "Mise à l'échelle dynamique des performances", "动态性能缩放", "動的パフォーマンススケーラー", "Escalador dinámico de rendimiento", "동적 성능 스케일러", "Scaler dinamico delle prestazioni", "Escalador dinâmico de desempenho", "डायनेमिक परफ़ॉर्मेंस स्केलर"},
        {"Automatically adjusts rendering resolution to maintain the target frame rate. When frames take too long, resolution steps down; when frames are fast, it recovers. Inspired by Atmosphere's dynamic resolution system.", "Автоматически регулирует разрешение рендеринга для поддержания целевой частоты кадров. При падении производительности разрешение снижается, а при стабильном фреймрейте - восстанавливается.", "يضبط دقة العرض تلقائياً للحفاظ على معدل الإطارات المستهدف. عندما تستغرق الإطارات وقتاً طويلاً، تنخفض الدقة، وعندما تكون سريعة، تستعيد عافيتها.", "Passt die Renderauflösung automatisch an, um die Zielbildrate zu halten. Wenn Frames zu lange dauern, sinkt die Auflösung; wenn Frames schnell sind, wird sie wiederhergestellt.", "Ajuste automatiquement la résolution de rendu pour maintenir la fréquence d'images cible. Lorsque les images prennent trop de temps, la résolution diminue ; lorsque les images sont rapides, elle est rétablie.", "自动调整渲染分辨率以保持目标帧率。当帧生成时间过长时降低分辨率，当帧率充裕时恢复分辨率。", "ターゲットフレームレートを維持するために描画解像度を自動的に調整します。負荷が高い時は解像度を下げ、余裕がある時は復元します。", "Ajusta automáticamente la resolución de renderizado para mantener los FPS objetivo. Cuando los fotogramas tardan más, la resolución se reduce; cuando van rápido, se restablece.", "목표 프레임레이트를 유지하도록 렌더링 해상도를 자동 조절합니다. 부하가 높으면 해상도를 낮추고 여유가 생기면 즉시 복구합니다.", "Regola automaticamente la risoluzione di rendering per mantenere il framerate target, riducendola sotto carico e ripristinandola appena possibile.", "Ajusta automaticamente a resolução de renderização para manter a taxa de quadros alvo, reduzindo sob carga e recuperando quando estável.", "लक्ष्य फ़्रेम दर बनाए रखने के लिए रेंडरिंग रिज़ॉल्यूशन को स्वचालित रूप से समायोजित करता है। लोड बढ़ने पर रिज़ॉल्यूशन घटाता है और सामान्य होने पर बहाल करता है।"},
        {"Automatically patches 3D camera frustum constants in game executables to match the selected aspect ratio (4:3, 16:10, 18:9, 21:9, 32:9, 48:9, etc.) without stretching 3D geometry.", "Автоматически патчит константы 3D-камеры в исполняемых файлах игры под выбранное соотношение сторон (4:3, 16:10, 18:9, 21:9, 32:9, 48:9 и др.) без растяжения 3D-геометрии.", "يقوم تلقائياً بتعديل ثوابت الكاميرا ثلاثية الأبعاد في ملفات اللعبة لتتناسب مع نسبة العرض إلى الارتفاع المحددة (4:3، 16:10، 18:9، 21:9، 32:9، 48:9، إلخ) دون تمديد الهندسة ثلاثية الأبعاد.", "Patcht automatisch die 3D-Kamera-Frustum-Konstanten in Spiel-Executables passend zum gewählten Seitenverhältnis (4:3, 16:10, 18:9, 21:9, 32:9, 48:9 usw.) ohne Streckung der 3D-Geometrie.", "Patche automatiquement les constantes de caméra 3D dans les exécutables du jeu pour correspondre au format d'image sélectionné (4:3, 16:10, 18:9, 21:9, 32:9, 48:9, etc.) sans étirer la géométrie 3D.", "自动修补游戏可执行文件中的 3D 摄像机视锥体常量，以匹配所选宽高比（4:3、16:10、18:9、21:9、32:9、48:9 等），而不会拉伸 3D 几何图形。", "選択したアスペクト比（4:3、16:10、18:9、21:9、32:9、48:9 など）に合わせてゲーム実行ファイル内の 3D カメラ定数を自動パッチし、3D ジオメトリの引き伸ばしを防ぎます。", "Parchea automáticamente las constantes de cámara 3D en los ejecutables del juego según la relación de aspecto seleccionada (4:3, 16:10, 18:9, 21:9, 32:9, 48:9, etc.) sin estirar la geometría 3D.", "3D 지오메트리를 늘리지 않고 선택한 화면비(4:3, 16:10, 18:9, 21:9, 32:9, 48:9 등)에 맞춰 게임 실행 파일의 3D 카메라 상수를 자동 패치합니다.", "Applica automaticamente patch alle costanti della telecamera 3D per adattarle al formato scelto (4:3, 16:10, 21:9, 32:9, ecc.) senza deformare la geometria 3D.", "Aplica patch automático nas constantes da câmera 3D do jogo para corresponder à proporção escolhida (4:3, 16:10, 21:9, 32:9, etc.) sem esticar a geometria 3D.", "3D ज्योमेट्री को खींचे बिना चयनित आस्पेक्ट रेशियो (4:3, 16:10, 18:9, 21:9, 32:9, 48:9 आदि) से मेल खाने के लिए गेम के 3D कैमरा स्थिरांक को स्वचालित रूप से पैच करता है।"},
        {"Automatically unlocks 30 FPS VSync/swap-interval caps to allow 60/120 FPS rendering.", "Автоматически снимает ограничение 30 FPS (интервал VSync), позволяя рендерить игру при 60/120 FPS.", "يفك تلقائياً قيد 30 إطاراً في الثانية (فاصل VSync) للسماح بالعرض بمعدل 60/120 إطاراً في الثانية.", "Hebt das 30-FPS-VSync-Limit automatisch auf, um Rendering mit 60/120 FPS zu ermöglichen.", "Déverrouille automatiquement la limite VSync de 30 FPS pour permettre un rendu à 60/120 FPS.", "自动解除 30 FPS 垂直同步（交换间隔）上限，允许以 60/120 FPS 进行渲染。", "30 FPS の垂直同期（スワップ間隔）制限を自動解除し、60/120 FPS での描画を可能にします。", "Desbloquea automáticamente el límite VSync de 30 FPS para permitir el renderizado a 60/120 FPS.", "30 FPS VSync 제한을 자동으로 해제하여 60/120 FPS 렌더링을 허용합니다.", "Sblocca automaticamente il limite VSync a 30 FPS per consentire il rendering a 60/120 FPS.", "Desbloqueia automaticamente os limites de 30 FPS do VSync para permitir renderização a 60/120 FPS.", "60/120 FPS रेंडरिंग की अनुमति देने के लिए 30 FPS VSync सीमा को स्वचालित रूप से अनलॉक करता है।"},
        {"Selects the scaling filter used when adapting the rendered image to the window size.\nBilinear and Bicubic are standard filters.\nAMD FidelityFX Super Resolution (FSR) and Qualcomm SGSR provide sharp upscaling.", "Выбирает фильтр масштабирования при адаптации изображения под размер окна.\nБилинейный и Бикубический - стандартные фильтры.\nAMD FidelityFX Super Resolution (FSR) и Qualcomm SGSR обеспечивают чёткий апскейлинг.", "يحدد مرشح التحجيم المستخدم عند تكييف الصورة المعروضة مع حجم النافذة.\nثنائي الخطية وثنائي التكعيب هما مرشحان قياسيان.\nيوفر FSR وSGSR ترقية حادة للصورة.", "Wählt den Skalierungsfilter zur Anpassung des gerenderten Bildes an die Fenstergröße.\nBilinear und Bikubisch sind Standardfilter.\nAMD FSR und Qualcomm SGSR bieten scharfes Upscaling.", "Sélectionne le filtre de mise à l'échelle utilisé pour adapter l'image rendue à la taille de la fenêtre.\nBilinéaire et Bicubique sont des filtres standards.\nAMD FSR et Qualcomm SGSR offrent une mise à l'échelle nette.", "选择将渲染图像适配到窗口大小时使用的缩放过滤器。\n双线性和双三次为标准过滤器。\nAMD FSR 和高通 SGSR 提供清晰的超分辨率缩放。", "描画画像をウィンドウサイズに合わせる際のスケーリングフィルターを選択します。\nバイリニアとバイキュービックは標準フィルターです。\nAMD FSR と Qualcomm SGSR はシャープなアップスケーリングを提供します。", "Selecciona el filtro de escalado al adaptar la imagen renderizada al tamaño de la ventana.\nBilineal y Bicúbico son filtros estándar.\nAMD FSR y Qualcomm SGSR ofrecen un reescalado nítido.", "렌더링된 이미지를 창 크기에 맞출 때 사용할 스케일링 필터를 선택합니다.\n쌍선형 및 쌍삼차는 표준 필터이며, AMD FSR과 Qualcomm SGSR은 선명한 업스케일링을 제공합니다.", "Seleziona il filtro di ridimensionamento usato per adattare l'immagine alla finestra.\nBilineare e Bicubico sono standard; AMD FSR e Qualcomm SGSR offrono upscaling nitido.", "Seleciona o filtro de escala usado ao ajustar a imagem ao tamanho da janela.\nBilinear e Bicúbico são padrões; AMD FSR e Qualcomm SGSR oferecem upscaling nítido.", "रेंडर की गई छवि को विंडो आकार में ढालने के लिए स्केलिंग फ़िल्टर चुनता है।\nबाइलीनियर और बाइक्यूबिक मानक फ़िल्टर हैं; AMD FSR और Qualcomm SGSR तीक्ष्ण अपस्केलिंग प्रदान करते हैं।"},
        {"Controls the GPU emulation mode.\nMost games render fine with Fast, but Accurate is still required for some.\nParticles tend to only render correctly with Accurate mode.", "Уровень точности ГП.\n«Нормальный» работает быстрее, но может вызывать графические артефакты в некоторых играх.\n«Высокий» медленнее, но исправляет графические ошибки.\nЧастота обновления определяет периодичность синхронизации состояния ГП.", "مستوى دقة معالج الرسوميات.\nعادي أسرع ولكنه قد يسبب مشاكل رسومية في بعض الألعاب.\nعالي أبطأ ولكنه يصلح المشاكل الرسومية.", "Die zu verwendende GPU-Genauigkeitsstufe.\nNormal ist schneller, kann aber in einigen Spielen Grafikfehler verursachen.\nHoch ist langsamer, behebt aber Grafikprobleme.", "Le niveau de précision du GPU à utiliser.\nNormal est plus rapide, mais peut causer des problèmes graphiques.\nÉlevé est plus lent, mais corrige les problèmes graphiques.", "要使用的 GPU 精度级别。\n标准速度更快，但在某些游戏中可能导致图形问题。\n高精度较慢，但可修复图形问题。", "使用する GPU 精度レベル。\nノーマルは高速ですが、一部のゲームで描画不具合が発生する場合があります。\nハイは低速ですが、描画不具合を修正します。", "Nivel de precisión de GPU.\nNormal es más rápido, pero puede causar fallos gráficos en algunos juegos.\nAlto es más lento, pero corrige errores gráficos.", "GPU 에뮬레이션 모드를 제어합니다.\n대부분의 게임은 고속 모드에서도 제대로 렌더링되지만, 일부 게임에서는 정확 모드가 필요합니다.\n특히 조각들은 정확 모드에서만 제대로 렌더링되는 경향이 있습니다.", "Controlla la precisione dell'emulazione GPU.\nLa modalità normale è più veloce, ma la modalità accurata risolve glitch e particelle in alcuni giochi.", "Controla o modo de precisão da GPU.\nA maioria dos jogos funciona bem no modo rápido, mas partículas e efeitos complexos exigem o modo preciso.", "GPU एमुलेशन सटीकता मोड को नियंत्रित करता है।\nअधिकांश गेम तेज़ मोड में ठीक चलते हैं, लेकिन कुछ पार्टिकल्स और प्रभावों के लिए सटीक मोड आवश्यक है।"},
        {"Controls the DMA read mode.\nUnsafe is faster, while Safe is more stable and can fix issues in some games.\nDefault follows the GPU Accuracy setting.", "Уровень точности DMA.\n«Нормально» работает быстрее, но может вызывать графические ошибки в отдельных играх.\n«Безопасно» медленнее, но устраняет графические сбои.", "مستوى دقة DMA.\nعادي أسرع ولكنه قد يسبب مشاكل رسومية.\nآمن أبطأ ولكنه يصلح المشاكل الرسومية.", "Die zu verwendende DMA-Genauigkeitsstufe.\nNormal ist schneller, kann aber Grafikfehler verursachen.\nSicher ist langsamer, behebt aber Grafikprobleme.", "Le niveau de précision DMA à utiliser.\nNormal est plus rapide, mais peut causer des problèmes graphiques.\nSûr est plus lent, mais corrige les problèmes graphiques.", "要使用的 DMA 精度级别。\n标准速度更快，但可能导致图形问题。\n安全较慢，但可修复图形问题。", "使用する DMA 精度レベル。\nノーマルは高速ですが、描画不具合が発生する場合があります。\nセーフは低速ですが、描画不具合を修正します。", "Nivel de precisión DMA.\nNormal es más rápido, pero puede causar fallos gráficos.\nSeguro es más lento, pero corrige errores gráficos.", "DMA 읽기 모드를 제어합니다.\n불안정 모드는 더 빠르며, 안전 모드는 더 안정적이며 일부 게임에서 발생하는 문제를 해결할 수 있습니다.\n기본값은 GPU 정확도 설정을 따릅니다.", "Controlla la modalità di lettura DMA.\nNon sicuro è più veloce, mentre Sicuro è più stabile e corregge errori grafici.\nPredefinito segue la precisione GPU.", "Controla o modo de leitura DMA.\nInseguro é mais rápido, enquanto Seguro é mais estável e corrige falhas em alguns jogos.\nPadrão segue a precisão da GPU.", "DMA रीड मोड को नियंत्रित करता है।\nअसुरक्षित तेज़ है, जबकि सुरक्षित अधिक स्थिर है और कुछ गेम्स की समस्याओं को ठीक करता है।\nडिफ़ॉल्ट GPU सटीकता का अनुसरण करता है।"},
        {"Controls the GPU fence synchronization behavior.\nImmediate is the fastest option, but can introduce some issues.\nBalanced offers better compatibility and may fix issues in some games.\nAccurate further improves compatibility at the cost of some performance.\nStrict is the slowest option, but can fix issues that require stricter synchronization.\nDefault follows the GPU Accuracy setting.", "Управляет синхронизацией барьеров (fences) ГП.\n«По умолчанию» следует настройке точности ГП.\n«Мгновенно» сигнализирует барьеры сразу (быстрее всего).\n«Сбалансированно» ждёт только при необходимости.\n«Точно» всегда дожидается завершения команд ГП.", "يتحكم في كيفية مزامنة حواجز GPU.\nالافتراضي يتبع إعداد دقة GPU.\nفوري يرسل الإشارات فوراً.\nمتوازن ينتظر عند الضرورة فقط.\nدقيق ينتظر دائماً انتهاء GPU.", "Steuert die Synchronisierung von GPU-Fences.\nStandard folgt der GPU-Genauigkeit.\nSofort signalisiert Fences direkt (am schnellsten).\nAusgewogen wartet nur bei Bedarf.\nPräzise wartet immer auf die GPU (am genauesten).", "Contrôle la synchronisation des barrières GPU.\nPar défaut suit la précision GPU.\nImmédiat signale immédiatement.\nÉquilibré attend uniquement si nécessaire.\nPrécis attend toujours la fin du GPU.", "控制 GPU 栅栏（Fence）的同步方式。\n默认遵循 GPU 精度设置。\n立即会马上发出信号（最快）。\n平衡仅在需要时等待。\n精确始终等待 GPU 完成（最准）。", "GPU フェンスの同期方法を制御します。\nデフォルトは GPU 精度設定に従います。\n即時（最速）、バランス（必要時のみ待機）、正確（常に GPU 完了を待機）から選択できます。", "Controla cómo se sincronizan las vallas (fences) de GPU.\nPredeterminado sigue la precisión de GPU.\nInmediato señala al instante.\nEquilibrado espera solo cuando es necesario.\nPreciso espera siempre a la GPU.", "GPU 펜스 동기화 동작을 제어합니다.\n즉시는 가장 빠른 옵션이지만 일부 문제가 발생할 수 있습니다.\n균형은 호환성이 더 좋고 일부 게임에서 발생하는 문제를 해결할 수 있습니다.\n정확은 성능 저하를 감수하고 호환성을 더욱 향상시킵니다.\n엄격은 가장 느린 옵션이지만 더 엄격한 동기화가 필요한 문제를 해결할 수 있습니다.\n기본값은 GPU 정확도 설정을 따릅니다.", "Controlla la sincronizzazione delle fence GPU.\nImmediato è il più veloce; Bilanciato e Accurato migliorano la compatibilità; Rigoroso risolve problemi di sincronizzazione complessi.", "Controla a sincronização de cercas (fences) da GPU.\nImediato é o mais rápido; Equilibrado e Preciso aumentam a compatibilidade; Estrito resolve falhas severas de sincronização.", "GPU फ़ेंस सिंक्रनाइज़ेशन व्यवहार को नियंत्रित करता है।\nतत्काल सबसे तेज़ है; संतुलित और सटीक बेहतर संगतता देते हैं; सख़्त कठिन सिंक समस्याओं को ठीक करता है।"},
        {"Preserves GPU-modified data by reading it back before uploading.\nSome games require this to render certain effects properly.", "Включает обратное чтение изменённых буферов ГП в память ЦП.\nТребуется некоторым играм для корректной физики или эффектов рендеринга, но заметно снижает производительность.", "يمكّن قراءة مخازن GPU المعدلة مرة أخرى إلى ذاكرة CPU.\nمطلوب لبعض الألعاب للفيزياء أو التأثيرات، ولكنه يقلل الأداء.", "Aktiviert das Zurücklesen geänderter GPU-Puffer in den CPU-Speicher.\nWird von einigen Spielen für Physik oder Effekte benötigt, verringert jedoch die Leistung.", "Active la relecture des tampons GPU modifiés vers la mémoire CPU.\nRequis par certains jeux pour la physique ou les effets, mais réduit les performances.", "启用将修改后的 GPU 缓冲区读回 CPU 内存。\n某些游戏需要此功能才能实现正常的物理或渲染效果，但会显著降低性能。", "変更された GPU バッファを CPU メモリへ書き戻す機能を有効にします。\n一部のゲームで物理演算やエフェクトに必要ですが、パフォーマンスが低下します。", "Habilita la lectura de búferes de GPU modificados de vuelta a la memoria de CPU.\nNecesario en algunos juegos para físicas o efectos, pero reduce el rendimiento.", "수정된 GPU 데이터를 업로드하기 전에 다시 읽어들여 보존합니다.\n일부 게임에서는 특정 효과를 제대로 렌더링하기 위해 이 기능이 필요합니다.", "Preserva i dati modificati dalla GPU rileggendoli prima del caricamento.\nNecessario in alcuni giochi per rendere correttamente fisica o effetti speciali.", "Preserva os dados modificados pela GPU relendo-os antes do envio.\nNecessário em alguns jogos para renderizar efeitos e física corretamente.", "अपलोड करने से पहले GPU द्वारा संशोधित डेटा को वापस पढ़कर सुरक्षित रखता है।\nकुछ गेम्स में विशेष प्रभावों को सही ढंग से रेंडर करने के लिए यह आवश्यक है।"},
        {"Polish (polski)", "Польский (polski)", "البولندية (polski)", "Polnisch (polski)", "Polonais (polski)", "波兰语 (polski)", "ポーランド語 (polski)", "Polaco (polski)", "폴란드어 (polski)", "Polacco (polski)", "Polonês (polski)", "पोलिश (polski)"},
        {"Celsius (°C)", "Цельсий (°C)", "مئوية (°C)", "Celsius (°C)", "Celsius (°C)", "摄氏度 (°C)", "摂氏 (°C)", "Celsius (°C)", "摂氏 (°C)", "Цельсий (°C)", "Цельсий (°C)", "सेल्सियस (°C)"},
        {"Fahrenheit (°F)", "Фаренгейт (°F)", "فهرنهايت (°F)", "Fahrenheit (°F)", "Fahrenheit (°F)", "华氏度 (°F)", "華氏 (°F)", "Fahrenheit (°F)", "華氏 (°F)", "Фаренгейт (°F)", "Фаренгейт (°F)", "फ़ारेनहाइट (°F)"},
        {"Errors Only", "Только ошибки", "الأخطاء فقط", "Nur Fehler", "Erreurs uniquement", "仅错误", "エラーのみ", "Solo errores", "오류만", "Solo errori", "Apenas erros", "केवल त्रुटियाँ"},
        {"Standard", "Стандартный", "قياسي", "Standard", "Standard", "标准", "標準", "Estándar", "기본", "Standard", "Padrão", "मानक"},
        {"Verbose", "Подробный", "مفصل", "Ausführlich", "Détaillé", "详细", "詳細", "Detallado", "상세", "Dettagliato", "Detalhado", "विस्तृत"},
        {"All", "Все", "الكل", "Alle", "Tout", "全部", "すべて", "Todo", "전체", "Tutti", "Todos", "सभी"},
        {"Turbo", "Турбо", "توربو", "Turbo", "Turbo", "加速 (Turbo)", "ターボ", "Turbo", "터보", "Turbo", "Turbo", "टर्बो"},
        {"Slow", "Замедление", "بطيء", "Langsam", "Lent", "减速 (Slow)", "スロー", "Lento", "저속", "Lento", "Lento", "धीमा"},
        {"CPU Page Tables", "Таблицы страниц ЦП", "جداول صفحات المعالج", "CPU-Seitentabellen", "Tables de pages CPU", "CPU 页表", "CPU ページテーブル", "Tablas de páginas CPU", "CPU 페이지 테이블", "Tabelle di pagina CPU", "Tabelas de páginas da CPU", "CPU पेज टेबल"},
        {"CPU Block Linking", "Связывание блоков ЦП", "ربط كتل المعالج", "CPU-Block-Verknüpfung", "Liaison de blocs CPU", "CPU 块链接", "CPU ブロックリンク", "Enlace de bloques CPU", "CPU 블록 연결", "Collegamento blocchi CPU", "Vinculação de blocos de CPU", "CPU ब्लॉक लिंकिंग"},
        {"CPU Constant Propagation", "Распространение констант ЦП", "نشر الثوابت للمعالج", "CPU-Konstantenweitergabe", "Propagation de constantes CPU", "CPU 常数传播", "CPU 定数伝播", "Propagación de constantes CPU", "CPU 상수 전파", "Propagazione costanti CPU", "Propagação de constantes da CPU", "CPU स्थिरांक प्रसार"},
        {"CPU Context Elimination", "Исключение мертвого контекста ЦП", "إلغاء سياق المعالج الميت", "CPU-Kontexteliminierung", "Élimination de contexte CPU", "CPU 上下文消除", "CPU コンテキスト削除", "Eliminación de contexto CPU", "CPU 컨텍스트 제거", "Eliminazione contesto CPU", "Eliminação de contexto da CPU", "CPU संदर्भ निष्कासन"},
        {"CPU Fast Dispatcher", "Быстрый диспетчер ЦП", "موزع المعالج السريع", "CPU-Schnelldispatcher", "Répartiteur rapide CPU", "CPU 快速调度器", "CPU 高速ディスパッチャ", "Despachador rápido CPU", "CPU 고속 디스패처", "Dispatcher veloce CPU", "Despachante rápido da CPU", "CPU तेज़ डिस्पैचर"},
        {"CPU Fastmem", "Быстрая память ЦП (Fastmem)", "الذاكرة السريعة للمعالج (Fastmem)", "CPU-Fastmem", "Mémoire rapide CPU (Fastmem)", "CPU 快速内存 (Fastmem)", "CPU 高速メモリ (Fastmem)", "Memoria rápida CPU (Fastmem)", "CPU 고속 메모리 (Fastmem)", "Memoria rapida CPU (Fastmem)", "Memória rápida da CPU (Fastmem)", "CPU तेज़ मेमोरी (Fastmem)"},
        {"CPU Fastmem Exclusives", "Эксклюзивы быстрой памяти", "العمليات الحصرية للذاكرة السريعة", "CPU-Fastmem-Exklusivoperationen", "Exclusives de mémoire rapide CPU", "CPU 快速内存互斥", "CPU Fastmem 排他処理", "Exclusivos de memoria rápida CPU", "CPU Fastmem 전용 연산", "Esclusive memoria rapida CPU", "Exclusivos de memória rápida da CPU", "CPU तेज़ मेमोरी विशिष्ट"},
        {"CPU Ignore Memory Aborts", "Игнорировать сбои памяти ЦП", "تجاهل أخطاء الذاكرة للمعالج", "CPU-Speicherabbrüche ignorieren", "Ignorer les interruptions mémoire CPU", "CPU 忽略内存异常", "CPU メモリアボートを無視", "Ignorar fallos de memoria CPU", "CPU 메모리 중단 무시", "Ignora interruzioni memoria CPU", "Ignorar abortos de memória da CPU", "CPU मेमोरी निरस्तीकरण अनदेखा करें"},
        {"CPU Misc IR Optimizations", "Оптимизации промежуточного кода ЦП", "تحسينات IR المتنوعة للمعالج", "CPU-IR-Optimierungen", "Optimisations IR diverses CPU", "CPU 杂项 IR 优化", "CPU 各種 IR 最適化", "Optimizaciones IR diversas CPU", "CPU 기타 IR 최적화", "Ottimizzazioni varie IR CPU", "Otimizações diversas de IR da CPU", "CPU विविध IR अनुकूलन"},
        {"CPU Recompile Exclusives", "Перекомпиляция эксклюзивов ЦП", "إعادة ترجمة العمليات الحصرية", "CPU-Neuübersetzung von Exklusiven", "Recompilation d'exclusives CPU", "CPU 重新编译互斥指令", "CPU 排他命令の再コンパイル", "Recompilación de exclusivos CPU", "CPU 전용 명령 재컴파일", "Ricompilazione esclusive CPU", "Recompilação de exclusivos da CPU", "CPU विशिष्ट पुनः संकलन"},
        {"CPU Reduce Misalignment Checks", "Сократить проверки выравнивания ЦП", "تقليل فحوصات عدم المحاذاة", "CPU-Fehlausrichtungsprüfungen reduzieren", "Réduire les vérifications de désalignement", "CPU 减少未对齐检查", "CPU アライメント検証の削減", "Reducir comprobaciones de desalineación CPU", "CPU 정렬되지 않은 검사 감소", "Riduci controlli di disallineamento CPU", "Reduzir verificações de desalinhamento da CPU", "CPU गलत संरेखण जाँच कम करें"},
        {"CPU Return Stack Buffer", "Буфер стека возвратов ЦП (RSB)", "مخزن مكدس العودة للمعالج", "CPU-Return-Stack-Buffer (RSB)", "Mémoire tampon de pile de retour CPU", "CPU 返回堆栈缓冲区 (RSB)", "CPU リターンスタックバッファ (RSB)", "Búfer de pila de retorno CPU", "CPU 반환 스택 버퍼 (RSB)", "Buffer stack di ritorno CPU", "Buffer de pilha de retorno da CPU (RSB)", "CPU रिटर्न स्टैक बफ़र"},
        {"Network Interface", "Сетевой интерфейс", "واجهة الشبكة", "Netzwerkschnittstelle", "Interface réseau", "网络接口", "ネットワークインターフェース", "Interfaz de red", "네트워크 인터페이스", "Interfaccia di rete", "Interface de rede", "नेटवर्क इंटरफ़ेस"},
        {"UDP Input Servers", "UDP-серверы ввода", "خوادم إدخال UDP", "UDP-Eingabeserver", "Serveurs d'entrée UDP", "UDP 输入服务器", "UDP 入力サーバー", "Servidores de entrada UDP", "UDP 입력 서버", "Server di input UDP", "Servidores UDP de entrada", "UDP इनपुट सर्वर"},
        {"Randomize Amiibo ID", "Случайный ID Amiibo", "معرف Amiibo عشوائي", "Amiibo-ID randomisieren", "ID Amiibo aléatoire", "随机 Amiibo ID", "Amiibo ID をランダム化", "Aleatorizar ID de Amiibo", "Amiibo ID 무작위화", "Randomizza ID Amiibo", "Gerar ID aleatório de Amiibo", "Amiibo ID यादृच्छिक करें"},
        {"Disable Buffer Reorder", "Отключить переупорядочивание буферов", "تعطيل إعادة ترتيب المخازن", "Puffer-Neuanordnung deaktivieren", "Désactiver le réordonnancement de tampons", "禁用缓冲区重新排序", "バッファ再順序付けを無効化", "Desactivar reordenación de búferes", "버퍼 재정렬 비활성화", "Disabilita riordino buffer", "Desativar reordenação de buffers", "बफ़र पुनर्गठन अक्षम करें"},
        {"Disable Macro HLE", "Отключить Macro HLE", "تعطيل Macro HLE", "Macro HLE deaktivieren", "Désactiver Macro HLE", "禁用宏 HLE", "マクロ HLE を無効化", "Desactivar Macro HLE", "매크로 HLE 비활성화", "Disabilita Macro HLE", "Desativar Macro HLE", "मैक्रो HLE अक्षम करें"},
        {"Disable Macro JIT", "Отключить Macro JIT", "تعطيل Macro JIT", "Macro JIT deaktivieren", "Désactiver Macro JIT", "禁用宏 JIT", "マクロ JIT を無効化", "Desactivar Macro JIT", "매크로 JIT 비활성화", "Disabilita Macro JIT", "Desativar Macro JIT", "मैक्रो JIT अक्षम करें"},
        {"Disable Shader Loop Safety Checks", "Отключить проверки циклов в шейдерах", "تعطيل فحوصات أمان حلقات المظللات", "Shader-Schleifen-Sicherheitsprüfungen deaktivieren", "Désactiver la sécurité des boucles de nuanceurs", "禁用着色器循环安全检查", "シェーダーループの安全性検査を無効化", "Desactivar comprobaciones de bucles en sombreadores", "셰이더 루프 안전 검사 비활성화", "Disabilita controlli di sicurezza loop shader", "Desativar verificações de segurança de loops de sombreador", "शेडर लूप सुरक्षा जाँच अक्षम करें"},
        {"Raw Input", "Прямой ввод (Raw Input)", "الإدخال الخام (Raw Input)", "Direkteingabe (Raw Input)", "Entrée brute (Raw Input)", "原始输入 (Raw Input)", "生入力 (Raw Input)", "Entrada directa (Raw Input)", "원시 입력 (Raw Input)", "Input raw (grezzo)", "Entrada bruta (Raw Input)", "कच्चा इनपुट (Raw Input)"},
        {"Mouse Panning", "Управление камерой мышью", "تحريك الكاميرا بالماوس", "Maus-Kamerabewegung", "Contrôle caméra à la souris", "鼠标视角平移", "マウスによる視点移動", "Control de cámara con ratón", "마우스 시점 이동", "Panoramica con mouse", "Controle de câmera com mouse", "माउस पैनिंग"},
        {"Enable Vibration", "Включить вибрацию", "تفعيل الاهتزاز", "Vibration aktivieren", "Activer les vibrations", "启用震动", "振動を有効化", "Habilitar vibración", "진동 활성화", "Abilita vibrazione", "Ativar vibração", "कंपन सक्षम करें"},
        {"Enable Motion", "Включить датчики движения", "تفعيل مستشعرات الحركة", "Bewegungssteuerung aktivieren", "Activer les capteurs de mouvement", "启用体感动作", "モーション操作を有効化", "Habilitar control de movimiento", "모션 제어 활성화", "Abilita sensori di movimento", "Ativar controle por movimento", "मोशन सक्षम करें"},
        {"Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet.", "Выбирает между быстрым встроенным интерфейсом STORM SWITCH и настоящим системным апплетом прошивки Switch.", "يحدد ما إذا كان سيتم استخدام واجهة STORM SWITCH السريعة أو تطبيق النظام الأصلي.", "Wählt zwischen dem schnellen STORM-Frontend und dem echten Switch-Firmware-Applet.", "Choisit entre l'interface rapide de STORM SWITCH et la véritable applet du firmware Switch.", "选择使用 STORM SWITCH 内置快速前端界面或真实的 Switch 固件系统小程序。", "STORM SWITCH の高速カスタムフロントエンドか Switch 実機ファームウェアのアプレットかを選択します。", "Selecciona entre la interfaz rápida de STORM SWITCH o el applet real del firmware de Switch.", "STORM SWITCH의 빠른 내장 프론트엔드와 실제 Switch 펌웨어 애플릿 중 하나를 선택합니다.", "Sceglie tra l'interfaccia rapida integrata di STORM SWITCH e l'applet reale del firmware Switch.", "Escolhe entre a interface rápida integrada do STORM SWITCH e o applet real do firmware do Switch.", "STORM SWITCH के तेज़ कस्टम फ़्रंटएंड या वास्तविक Switch फ़र्मवेयर एप्लेट के बीच चयन करता है।"},
        {"Selects the host audio output backend (cubeb, SDL2, or null). Auto selects the lowest-latency backend.", "Выбирает звуковой движок вывода аудио (cubeb, SDL2 или без звука). «Авто» выбирает минимальную задержку.", "يختار محرك إخراج الصوت المضيف (cubeb أو SDL2). الوضع التلقائي يختار أقل زمن انتقال.", "Wählt das Host-Audio-Backend (cubeb, SDL2 oder Null). Auto wählt die geringste Latenz.", "Sélectionne le moteur de sortie audio hôte (cubeb, SDL2 ou muet). Auto choisit la latence minimale.", "选择主机音频输出后端引擎（cubeb、SDL2 或静音）。自动模式将选择最低延迟后端。", "ホストのオーディオ出力バックエンド（cubeb、SDL2、Null）を選択します。自動は最小遅延を選択します。", "Selecciona el motor de salida de audio del sistema (cubeb, SDL2 o nulo). Auto elige la menor latencia.", "호스트 오디오 출력 백엔드(cubeb, SDL2 또는 무음)를 선택합니다. 자동은 최소 지연 백엔드를 선택합니다.", "Seleziona il motore di uscita audio (cubeb, SDL2 o nullo). Auto sceglie la latenza minima.", "Seleciona o motor de saída de áudio (cubeb, SDL2 ou nulo). Automático escolhe a menor latência.", "होस्ट ऑडियो आउटपुट इंजन (cubeb, SDL2 या मूक) चुनता है। ऑटो सबसे कम विलंबता वाला विकल्प चुनता है।"},
        {"Selects the physical headphones or speakers used for game audio playback.", "Выбирает физическое устройство воспроизведения звука (наушники или динамики).", "يختار جهاز إخراج الصوت الفعلي (سماعات الرأس أو مكبرات الصوت).", "Wählt das physische Audio-Ausgabegerät (Kopfhörer oder Lautsprecher).", "Sélectionne le périphérique physique de sortie audio (casque ou haut-parleurs).", "选择用于播放游戏声音的扬声器或耳机设备。", "ゲームの音声出力に使用するスピーカーやヘッドホンを選択します。", "Selecciona los altavoces o auriculares utilizados para reproducir el audio del juego.", "게임 오디오 재생에 사용할 스피커 또는 헤드폰 장치를 선택합니다.", "Seleziona gli altoparlanti o le cuffie utilizzati per la riproduzione dell'audio.", "Seleciona os alto-falantes ou fones de ouvido usados para reproduzir o áudio do jogo.", "गेम ऑडियो प्लेबैक के लिए उपयोग किए जाने वाले स्पीकर या हेडफ़ोन चुनता है।"},
        {"Selects the microphone device used by games that support voice input.", "Выбирает микрофон для игр, поддерживающих голосовой ввод.", "يختار جهاز الميكروفون المستخدم في الألعاب التي تدعم الإدخال الصوتي.", "Wählt das Mikrofon für Spiele mit Spracheingabe-Unterstützung.", "Sélectionne le microphone utilisé par les jeux prenant en charge l'entrée vocale.", "选择支持语音输入的游戏所使用的麦克风设备。", "音声入力対応ゲームで使用するマイクデバイスを選択します。", "Selecciona el micrófono utilizado en los juegos compatibles con entrada de voz.", "음성 입력을 지원하는 게임에서 사용할 마이크 장치를 선택합니다.", "Seleziona il microfono utilizzato dai giochi che supportano l'input vocale.", "Seleciona o microfone usado por jogos compatíveis com entrada de voz.", "वॉयस इनपुट का समर्थन करने वाले गेम्स के लिए माइक्रोफ़ोन डिवाइस चुनता है।"},
        {"Completely mutes all emulated game audio output.", "Полностью отключает весь звук эмулируемой игры.", "كتم جميع أصوات اللعبة بالكامل.", "Schaltet die gesamte Audioausgabe des Emulators stumm.", "Coupe complètement tout le son du jeu émulé.", "完全静音模拟器的所有游戏音频输出。", "エミュレート中のすべてのゲーム音声をミュートします。", "Silencia por completo toda la salida de audio del juego.", "에뮬레이션 중인 모든 게임 오디오 출력을 음소거합니다.", "Disattiva completamente tutto l'audio del gioco emulato.", "Silencia completamente toda a saída de áudio do jogo.", "एमुलेट किए गए गेम के सभी ऑडियो आउटपुट को पूरी तरह म्यूट करता है।"},
        {"Adjusts the master output volume of emulated game audio (0-200%).", "Регулирует общую громкость звука в играх (0-200%).", "ضبط مستوى الصوت الرئيسي للعبة (0-200%).", "Passt die Gesamtlautstärke der Spielaudioausgabe an (0-200 %).", "Règle le volume principal du son du jeu (0-200 %).", "调节游戏音频的主输出音量（0-200%）。", "ゲーム音声のマスター音量を調整します（0〜200%）。", "Ajusta el volumen principal del audio del juego (0-200%).", "게임 오디오의 마스터 출력 볼륨을 조절합니다 (0~200%).", "Regola il volume principale dell'audio di gioco (0-200%).", "Ajusta o volume principal do áudio do jogo (0-200%).", "गेम ऑडियो की मुख्य आउटपुट आवाज़ समायोजित करता है (0-200%)।"},
        {"Automatically mutes game audio whenever the STORM SWITCH window loses focus.", "Автоматически приглушает звук игры, когда окно STORM SWITCH свёрнуто или неактивно.", "كتم صوت اللعبة تلقائياً عندما تفقد نافذة المحاكي التركيز.", "Schaltet den Spielton automatisch stumm, sobald das Emulatorfenster im Hintergrund ist.", "Coupe automatiquement le son lorsque la fenêtre STORM SWITCH passe en arrière-plan.", "当 STORM SWITCH 窗口失去焦点或最小化时自动静音。", "STORM SWITCH ウィンドウが非アクティブになった際に自動で音声をミュートします。", "Silencia automáticamente el audio cuando la ventana del emulador pasa a segundo plano.", "STORM SWITCH 창이 비활성화되면 자동으로 게임 소리를 음소거합니다.", "Disattiva automaticamente l'audio quando la finestra di STORM SWITCH è in secondo plano.", "Silencia automaticamente o áudio quando a janela do STORM SWITCH está em segundo plano.", "जब STORM SWITCH विंडो बैकग्राउंड में जाती है तो ऑडियो अपने आप म्यूट कर देता है।"},
        {"Selects the host network adapter used for local wireless (LDN) and LAN multiplayer.", "Выбирает сетевой адаптер ПК для локальной беспроводной сети (LDN) и LAN-мультиплеера.", "يختار محول الشبكة المستخدم للعب الجماعي المحلي (LDN و LAN).", "Wählt den Netzwerkadapter für lokales Multiplayer-Spiel (LDN und LAN).", "Sélectionne la carte réseau utilisée pour le multijoueur sans fil local (LDN) et LAN.", "选择用于本地无线联机（LDN）和局域网联机的主机网络适配器。", "ローカル通信（LDN）および LAN マルチプレイに使用するネットワークアダプターを選択します。", "Selecciona el adaptador de red usado para el multijugador inalámbrico local (LDN) y LAN.", "로컬 무선(LDN) 및 LAN 멀티플레이에 사용할 네트워크 어댑터를 선택합니다.", "Seleziona la scheda di rete usata per il multigiocatore wireless locale (LDN) e LAN.", "Seleciona o adaptador de rede usado para o multijogador local sem fio (LDN) e LAN.", "लोकल वायरलेस (LDN) और LAN मल्टीप्लेयर के लिए नेटवर्क एडाप्टर चुनता है।"},
        {"Comma-separated DSU/Cemuhook UDP server addresses (IP:port) for external motion controllers.", "Адреса UDP-серверов DSU/Cemuhook (IP:порт через запятую) для внешних датчиков движения и гироскопа.", "عناوين خوادم UDP لبروتوكول DSU/Cemuhook لأجهزة التحكم بالحركة الخارجية.", "Kommagetrennte DSU/Cemuhook-UDP-Serveradressen (IP:Port) für externe Bewegungscontroller.", "Adresses des serveurs UDP DSU/Cemuhook (IP:port) pour les capteurs de mouvement externes.", "用于外部体感控制器的 DSU/Cemuhook UDP 服务器地址列表（IP:端口，以逗号分隔）。", "外部モーションコントローラー用の DSU/Cemuhook UDP サーバーアドレス（IP:ポート）。", "Direcciones de servidores UDP DSU/Cemuhook (IP:puerto) para controles de movimiento externos.", "외부 모션 컨트롤러용 DSU/Cemuhook UDP 서버 주소 목록입니다 (IP:포트, 쉼표로 구분).", "Indirizzi dei server UDP DSU/Cemuhook (IP:porta) per controller di movimento esterni.", "Endereços de servidores UDP DSU/Cemuhook (IP:porta) para controles de movimento externos.", "बाहरी मोशन कंट्रोलर के लिए DSU/Cemuhook UDP सर्वर पते (IP:पोर्ट)।"},
        {"Generates a fresh random Amiibo UUID on every scan to bypass once-per-day scan limits in games.", "Генерирует случайный идентификатор Amiibo при каждом сканировании, обходя суточный лимит наград в играх.", "يولّد معرّف Amiibo عشوائياً عند كل مسح لتجاوز الحد اليومي في الألعاب.", "Erzeugt bei jedem Scan eine zufällige Amiibo-UUID, um Tageslimits in Spielen zu umgehen.", "Génère un identifiant Amiibo aléatoire à chaque scan pour contourner la limite quotidienne.", "每次扫描时生成随机 Amiibo UUID，从而绕过游戏内的每日扫描次数限制。", "スキャンごとにランダムな Amiibo UUID を生成し、1日1回の回数制限を回避します。", "Genera un UUID aleatorio en cada escaneo de Amiibo para evitar el límite diario en los juegos.", "스캔할 때마다 무작위 Amiibo UUID를 생성하여 게임 내 일일 스캔 제한을 우회합니다.", "Genera un UUID Amiibo casuale a ogni scansione per aggirare il limite giornaliero nei giochi.", "Gera um UUID de Amiibo aleatório a cada leitura para contornar o limite diário dos jogos.", "गेम्स में प्रतिदिन एक बार स्कैन की सीमा को बायपास करने के लिए हर स्कैन पर नया यादृच्छिक Amiibo UUID बनाता है।"},
        {"Simulates Nintendo Switch Airplane Mode, disabling virtual network connectivity inside games.", "Имитирует режим «В самолёте» на Switch, отключая виртуальные сетевые функции внутри игр.", "يحاكي وضع الطيران في Switch ويعطّل الاتصال الشبكي الافتراضي داخل الألعاب.", "Simuliert den Switch-Flugmodus und trennt virtuelle Netzwerkverbindungen in Spielen.", "Simule le mode avion de la Switch en désactivant la connectivité réseau dans les jeux.", "模拟 Switch 飞行模式，禁用游戏内的虚拟网络连接（可修复启动时卡联网检查的游戏）。", "Switch の機内モードをシミュレートし、ゲーム内の仮想ネットワーク接続を無効化します。", "Simula el modo avión de Switch, desactivando las funciones de red dentro de los juegos.", "Switch 비행기 모드를 시뮬레이션하여 게임 내 가상 네트워크 연결을 비활성화합니다.", "Simula la modalità aereo di Switch, disabilitando la rete virtuale nei giochi.", "Simula o modo avião do Switch, desativando a conectividade de rede nos jogos.", "Switch के हवाई जहाज मोड का अनुकरण करता है और गेम्स के भीतर वर्चुअल नेटवर्क बंद करता है।"},
        {"Forces strict sequential GPU command buffer submission order to troubleshoot rare rendering glitches.", "Принудительно сохраняет строгий порядок отправки буферов ГПУ для отладки редких графических сбоев.", "يفرض الترتيب التسلسلي الصارم لمخازن أوامر الرسوميات لإصلاح أعطال التصيير النادرة.", "Erzwingt die strikte Reihenfolge von GPU-Befehlspuffern zur Behebung seltener Grafikfehler.", "Force l'ordre séquentiel strict des tampons GPU pour corriger de rares bugs graphiques.", "强制按严格顺序提交 GPU 命令缓冲区，用于排查罕见的渲染异常。", "GPU コマンドバッファの順序入れ替えを無効化し、まれな描画不具合を回避します。", "Fuerza el orden secuencial estricto de búferes GPU para corregir fallos gráficos poco comunes.", "드문 렌더링 오류를 해결하기 위해 GPU 명령 버퍼의 재정렬을 비활성화합니다.", "Forza l'ordine sequenziale dei buffer comandi GPU per risolvere rari glitch grafici.", "Força a ordem sequencial estrita dos buffers da GPU para corrigir falhas gráficas raras.", "दुर्लभ रेंडरिंग त्रुटियों के निवारण के लिए GPU कमांड बफ़र के सख़्त अनुक्रम को बाध्य करता है।"},
        {"Disables high-level GPU macro emulation and falls back to low-level macro execution for debugging.", "Отключает высокоуровневую эмуляцию макросов ГПУ (Macro HLE) для отладки совместимости.", "يعطّل المحاكاة عالية المستوى لماكرو الرسوميات (Macro HLE) لأغراض التصحيح.", "Deaktiviert die High-Level-GPU-Makro-Emulation (Macro HLE) zu Debugging-Zwecken.", "Désactive l'émulation haut niveau des macros GPU (Macro HLE) pour le débogage.", "禁用高级 GPU 宏模拟（Macro HLE），回退到底层宏解释以排查兼容性问题。", "高水準 GPU マクロエミュレーション（Macro HLE）を無効化して互換性を検証します。", "Desactiva la emulación de macros GPU de alto nivel (Macro HLE) para depuración.", "호환성 디버깅을 위해 고수준 GPU 매크로 에뮬레이션(Macro HLE)을 비활성화합니다.", "Disabilita l'emulazione ad alto livello delle macro GPU (Macro HLE) per il debug.", "Desativa a emulação de alto nível de macros da GPU (Macro HLE) para depuração.", "डीबगिंग के लिए उच्च-स्तरीय GPU मैक्रो एमुलेशन (Macro HLE) को अक्षम करता है।"},
        {"Disables the JIT compiler for GPU Maxwell macros and uses the interpreter instead.", "Отключает JIT-компилятор макросов ГПУ Maxwell, переключаясь на интерпретатор.", "يعطّل مترجم JIT لماكرو الرسوميات ويستخدم المفسّر بدلاً منه.", "Deaktiviert den JIT-Compiler für GPU-Makros und nutzt stattdessen den Interpreter.", "Désactive le compilateur JIT des macros GPU et utilise l'interpréteur.", "禁用 GPU 宏 JIT 编译器并改用解释器运行。", "GPU マクロの JIT コンパイラを無効化し、インタープリタを使用します。", "Desactiva el compilador JIT de macros GPU y utiliza el intérprete.", "GPU 매크로 JIT 컴파일러를 비활성화하고 인터프리터를 사용합니다.", "Disabilita il compilatore JIT delle macro GPU usando l'interprete.", "Desativa o compilador JIT de macros da GPU e usa o interpretador.", "GPU मैक्रो के लिए JIT कंपाइलर को अक्षम कर इंटरप्रेटर का उपयोग करता है।"},
        {"Removes infinite-loop guard checks inside compiled shaders for slightly faster GPU execution.", "Отключает защитные проверки бесконечных циклов в шейдерах для небольшого ускорения ГПУ.", "يزيل فحوصات الأمان للحلقات اللانهائية داخل المظللات لتسريع التنفيذ قليلاً.", "Entfernt Endlosschleifen-Sicherheitsprüfungen in Shadern für etwas höhere GPU-Leistung.", "Supprime les vérifications de boucles infinies dans les shaders pour accélérer légèrement le GPU.", "移除编译着色器中的死循环安全检查，以换取轻微的 GPU 性能提升。", "シェーダー内の無限ループ安全検査を省略して GPU 実行をわずかに高速化します。", "Elimina las comprobaciones de seguridad de bucles infinitos en los shaders para ganar rendimiento.", "컴파일된 셰이더 내 무한 루프 안전 검사를 생략하여 GPU 실행 속도를 소폭 높입니다.", "Rimuove i controlli anti-loop infinito negli shader per velocizzare leggermente la GPU.", "Remove verificações contra loops infinitos nos shaders para ganhar leve desempenho na GPU.", "थोड़े तेज़ GPU निष्पादन के लिए संकलित शेडर्स में अनंत-लूप सुरक्षा जाँच हटाता है।"},
        {"Reads mouse and keyboard input directly from the OS hardware layer with minimal latency.", "Считывает ввод мыши и клавиатуры напрямую с аппаратного уровня ОС с минимальной задержкой.", "يقرأ إدخال الماوس ولوحة المفاتيح مباشرة من طبقة العتاد بأقل زمن انتقال.", "Liest Maus- und Tastatureingaben direkt auf Hardwareebene für minimale Eingabeverzögerung.", "Lit les entrées clavier et souris directement au niveau matériel pour une latence minimale.", "直接从操作系统硬件层读取鼠标和键盘输入，实现最低输入延迟。", "OS のハードウェア層からマウスとキーボード入力を直接読み取り、入力遅延を最小化します。", "Lee la entrada de ratón y teclado directamente a nivel de hardware con latencia mínima.", "최소 입력 지연을 위해 OS 하드웨어 계층에서 마우스 및 키보드 입력을 직접 읽습니다.", "Legge l'input di mouse e tastiera direttamente a livello hardware con latenza minima.", "Lê a entrada do mouse e teclado diretamente da camada de hardware com latência mínima.", "न्यूनतम विलंबता के लिए सीधे OS हार्डवेयर स्तर से माउस और कीबोर्ड इनपुट पढ़ता है।"},
        {"Maps desktop mouse movement to the right analog stick for camera control in 3D games.", "Преобразует движение мыши в наклон правого стика для удобного обзора камерой в 3D-играх.", "يحوّل حركة الماوس إلى العصا اليمنى للتحكم بالكاميرا في الألعاب ثلاثية الأبعاد.", "Wandelt Mausbewegungen für die Kamerasteuerung in Bewegungen des rechten Analogsticks um.", "Convertit les mouvements de la souris en stick droit pour contrôler la caméra 3D.", "将桌面鼠标移动映射为右摇杆，以便在 3D 游戏中用鼠标控制视角。", "マウス移動を右スティックに割り当て、3Dゲームのカメラ操作を可能にします。", "Convierte el movimiento del ratón en el stick derecho para controlar la cámara en juegos 3D.", "3D 게임 카메라 조작을 위해 마우스 이동을 오른쪽 아날로그 스틱에 매핑합니다.", "Mappa il movimento del mouse sullo stick destro per controllare la telecamera nei giochi 3D.", "Mapeia o movimento do mouse para o analógico direito para controlar a câmera em jogos 3D.", "3D गेम्स में कैमरा नियंत्रण के लिए माउस की गति को दाएँ एनालॉग स्टिक से जोड़ता है।"},
        {"Enables controller HD Rumble / vibration force feedback.", "Включает вибрацию и тактильную отдачу HD Rumble на геймпадах.", "يفعّل اهتزاز وحدة التحكم وميزة HD Rumble.", "Aktiviert Controller-Vibration und HD-Rumble-Force-Feedback.", "Active les vibrations de la manette et le retour haptique HD Rumble.", "启用手柄 HD 震动与力反馈效果。", "コントローラーの振動および HD 振動フィードバックを有効化します。", "Habilita la vibración del mando y la respuesta háptica HD Rumble.", "컨트롤러 진동 및 HD 진동 피드백을 활성화합니다.", "Abilita la vibrazione del controller e il feedback aptico HD Rumble.", "Ativa a vibração do controle e o retorno tátil HD Rumble.", "कंट्रोलर कंपन और HD रंबल फ़ीडबैक सक्षम करता है।"},
        {"Enables gyroscope and accelerometer motion controls for compatible controllers.", "Включает управление движением (гироскоп и акселерометр) на поддерживаемых геймпадах.", "يفعّل التحكم بالحركة (الجيروسكوب ومقياس التسارع) لوحدات التحكم المتوافقة.", "Aktiviert Bewegungssteuerung (Gyroskop und Beschleunigungssensor) für kompatible Controller.", "Active la détection de mouvement (gyroscope et accéléromètre) pour les manettes compatibles.", "为兼容手柄启用陀螺仪和加速度计体感控制。", "対応コントローラーのジャイロ・加速度モーション操作を有効化します。", "Habilita el control por movimiento (giroscopio y acelerómetro) en mandos compatibles.", "호환 컨트롤러의 자이로스코프 및 가속도계 모션 조작을 활성화합니다.", "Abilita i controlli di movimento (giroscopio e accelerometro) per i controller compatibili.", "Ativa os controles de movimento (giroscópio e acelerômetro) em controles compatíveis.", "संगत कंट्रोलर्स के लिए जायरोस्कोप और एक्सेलेरोमीटर मोशन कंट्रोल सक्षम करता है।"},
        {"Caps emulation speed to the configured percentage (100% = native console speed).", "Ограничивает скорость эмуляции заданным процентом (100% = штатная скорость консоли).", "يقيّد سرعة المحاكاة بالنسبة المئوية المحددة (100% = السرعة الأصلية).", "Begrenzt die Emulationsgeschwindigkeit auf den eingestellten Prozentwert (100 % = Originaltempo).", "Limite la vitesse d'émulation au pourcentage défini (100 % = vitesse normale).", "将模拟速度限制在设定的百分比（100% 为主机原生速度）。", "エミュレーション速度を設定したパーセンテージに制限します（100%＝実機等속）。", "Limita la velocidad de emulación al porcentaje configurado (100% = velocidad nativa).", "에뮬레이션 속도를 설정된 백분율로 제한합니다 (100% = 콘솔 기본 속도).", "Limita la velocità di emulazione alla percentuale impostata (100% = velocità nativa).", "Limita a velocidade da emulação à porcentagem configurada (100% = velocidade nativa).", "एमुलेशन गति को निर्धारित प्रतिशत तक सीमित करता है (100% = मूल कंसोल गति)।"},
        {"Selects the ARM64 CPU execution engine (Dynarmic JIT on PC, or NCE native execution on ARM64).", "Выбирает движок исполнения кода ЦП ARM64 (JIT-компилятор Dynarmic на ПК или NCE на ARM64).", "يختار محرك تنفيذ المعالج ARM64 (Dynarmic JIT أو التنفيذ الأصلي NCE).", "Wählt das ARM64-CPU-Backend (Dynarmic-JIT auf dem PC oder native NCE-Ausführung auf ARM64).", "Sélectionne le moteur d'exécution CPU ARM64 (Dynarmic JIT sur PC ou NCE natif sur ARM64).", "选择 ARM64 CPU 执行后端（PC 端使用 Dynarmic JIT，ARM64 设备可使用原生 NCE）。", "ARM64 CPU 実行バックエンドを選択します（PC は Dynarmic JIT、ARM64 は NCE 対応）。", "Selecciona el motor de ejecución de CPU ARM64 (Dynarmic JIT en PC o NCE nativo en ARM64).", "ARM64 CPU 실행 백엔드를 선택합니다 (PC는 Dynarmic JIT, ARM64 기기는 네이티브 NCE).", "Seleziona il motore di esecuzione CPU ARM64 (Dynarmic JIT su PC o NCE nativo su ARM64).", "Seleciona o motor de execução da CPU ARM64 (Dynarmic JIT no PC ou NCE nativo em ARM64).", "ARM64 CPU निष्पादन इंजन चुनता है (PC पर Dynarmic JIT, या ARM64 पर NCE)।"},
        {"Overrides the number of emulated CPU ticks executed per slice for fine-tuning timing.", "Задаёт фиксированное число тактов ЦП за квант времени для тонкой настройки таймингов.", "يتجاوز عدد نبضات المعالج المحاكاة لكل شريحة لضبط التوقيت بدقة.", "Überschreibt die Anzahl emulierter CPU-Takte pro Zeitscheibe zur Feinabstimmung.", "Remplace le nombre de cycles CPU émulés par tranche pour ajuster finement le timing.", "自定义每个时间片执行的模拟 CPU 周期数，用于微调时序。", "タイムスライスあたりのエミュレート CPU ティック数を手動指定します。", "Personaliza el número de ciclos de CPU emulados por intervalo para ajustar la sincronización.", "타이밍 미세 조정을 위해 타임슬라이스당 실행되는 에뮬레이션 CPU 틱 수를 수동 지정합니다.", "Personalizza il numero di cicli CPU emulati per intervallo per regolare il timing.", "Substitui o número de ciclos de CPU emulados por fatia para ajuste fino de tempo.", "टाइमिंग के सूक्ष्म समायोजन के लिए प्रति स्लाइस एमुलेटेड CPU टिक संख्या निर्धारित करता है।"},
        {"Uses a fixed pseudo-random number generator seed for deterministic TAS and debugging runs.", "Фиксирует начальное зерно генератора случайных чисел для воспроизводимых прогонов TAS и отладки.", "يستخدم بذرة ثابتة لمولد الأرقام العشوائية للتشغيل الحتمي والتصحيح.", "Verwendet einen festen Zufallsgenerator-Seed für reproduzierbare TAS- und Debug-Durchläufe.", "Utilise une graine fixe de générateur aléatoire pour des exécutions TAS reproductibles.", "使用固定的随机数生成器种子，以实现可重复的 TAS 与调试运行。", "TAS やデバッグの再現性を確保するため固定の乱数シードを使用します。", "Usa una semilla fija de números aleatorios para ejecuciones deterministas de TAS y depuración.", "재현 가능한 TAS 및 디버깅 실행을 위해 고정된 난수 생성기 시드를 사용합니다.", "Usa un seme fisso per il generatore casuale per sessioni TAS e debug riproducibili.", "Usa uma semente fixa de números aleatórios para execuções determinísticas de TAS e depuração.", "पुनरुत्पादनीय TAS और डीबगिंग रन के लिए निश्चित यादृच्छिक संख्या सीड का उपयोग करता है।"},
        {"Overrides the emulated Nintendo Switch system clock with a custom date and time.", "Заменяет системные часы консоли Switch произвольной пользовательской датой и временем.", "يتجاوز ساعة نظام Switch المحاكاة بتاريخ ووقت مخصصين.", "Überschreibt die emulierte Switch-Systemuhr mit einem benutzerdefinierten Datum und Uhrzeit.", "Remplace l'horloge système émulée de la Switch par une date et une heure personnalisées.", "使用自定义日期与时间覆盖模拟的 Switch 系统时钟（可用于游戏内时间事件）。", "エミュレートされた Switch 本体の時計を任意の日時で上書きします。", "Sustituye el reloj del sistema de Switch por una fecha y hora personalizadas.", "에뮬레이션된 Switch 시스템 시계를 사용자 지정 날짜 및 시간으로 덮어씁니다.", "Sostituisce l'orologio di sistema emulato di Switch con una data e un'ora personalizzate.", "Substitui o relógio do sistema emulado do Switch por uma data e hora personalizadas.", "एमुलेटेड Switch सिस्टम घड़ी को कस्टम तिथि और समय से बदलता है।"},
        {"Selects the console audio channel layout (Mono, Stereo, or 5.1 Surround).", "Выбирает конфигурацию звуковых каналов консоли (Моно, Стерео или объёмный звук 5.1).", "يختار تخطيط قنوات الصوت (أحادي أو ستيريو أو محيطي 5.1).", "Wählt das Audiokanal-Layout der Konsole (Mono, Stereo oder 5.1-Surround).", "Sélectionne la configuration des canaux audio (Mono, Stéréo ou Surround 5.1).", "选择主机的音频声道布局（单声道、立体声或 5.1 环绕声）。", "本体の音声チャンネル出力形式（モノラル、ステレオ、サラウンド）を選択します。", "Selecciona el modo de canales de audio de la consola (Mono, Estéreo o Envolvente 5.1).", "콘솔 오디오 채널 구성(모노, 스테레오 또는 5.1 서라운드)을 선택합니다.", "Seleziona la configurazione dei canali audio della console (Mono, Stereo o Surround 5.1).", "Seleciona o modo de canais de áudio do console (Mono, Estéreo ou Surround 5.1).", "कंसोल का ऑडियो चैनल लेआउट चुनता है (मोनो, स्टीरियो या 5.1 सराउंड)।"},
        {"Activates Feral GameMode on Linux to request high-performance CPU governor and I/O priority.", "Активирует Feral GameMode в Linux для перевода ЦП и ввода-вывода в режим высокой производительности.", "يفعّل وضع Feral GameMode على Linux لرفع أولوية المعالج والإدخال/الإخراج.", "Aktiviert Feral GameMode unter Linux für maximale CPU-Governor- und I/O-Priorität.", "Active Feral GameMode sous Linux pour maximiser la priorité CPU et E/S.", "在 Linux 上启用 Feral GameMode 以请求高性能 CPU 调度器与 I/O 优先级。", "Linux 環境で Feral GameMode を有効化し、高パフォーマンス CPU ガバナーを要求します。", "Activa Feral GameMode en Linux para priorizar el gobernador de CPU y la E/S.", "Linux에서 고성능 CPU 거버너 및 I/O 우선순위를 요청하기 위해 Feral GameMode를 활성화합니다.", "Attiva Feral GameMode su Linux per richiedere massima priorità CPU e I/O.", "Ativa o Feral GameMode no Linux para solicitar prioridade máxima de CPU e E/S.", "उच्च-प्रदर्शन CPU गवर्नर और I/O प्राथमिकता के लिए Linux पर Feral GameMode सक्रिय करता है।"},
        {"Forces Qt to run via XWayland (xcb) instead of native Wayland on Linux desktops.", "Принудительно запускает интерфейс Qt через XWayland (xcb) вместо нативного Wayland в Linux.", "يجبر واجهة Qt على العمل عبر XWayland بدلاً من Wayland الأصلي في Linux.", "Erzwingt unter Linux die Ausführung der Qt-Oberfläche über XWayland (xcb) statt Wayland.", "Force l'interface Qt à s'exécuter via XWayland (xcb) au lieu de Wayland natif sous Linux.", "在 Linux 桌面环境下强制 Qt 通过 XWayland (xcb) 运行而非原生 Wayland。", "Linux 環境でネイティブ Wayland の代わりに XWayland (xcb) バックエンドを強制します。", "Fuerza la ejecución de Qt mediante XWayland (xcb) en lugar de Wayland nativo en Linux.", "Linux 데스크톱에서 네이티브 Wayland 대신 XWayland(xcb)로 Qt를 강제 실행합니다.", "Forza l'esecuzione di Qt tramite XWayland (xcb) invece di Wayland nativo su Linux.", "Força a execução do Qt via XWayland (xcb) em vez de Wayland nativo no Linux.", "Linux डेस्कटॉप पर मूल Wayland के बजाय XWayland (xcb) के माध्यम से Qt चलाता है।"},
        {"Extreme", "Экстремальный", "فائق", "Extrem", "Extrême", "极限", "極限", "Extremo", "극한", "Estremo", "Extremo", "अत्यधिक"},
        {"FSR 2.0 Quality Mode:", "Режим качества FSR 2.0:", "وضع جودة FSR 2.0:", "FSR 2.0 Qualitätsmodus:", "Mode de qualité FSR 2.0 :", "FSR 2.0 质量模式:", "FSR 2.0 品質モード:", "Modo de calidad FSR 2.0:", "FSR 2.0 품질 모드:", "Modalità qualità FSR 2.0:", "Modo de qualidade FSR 2.0:", "FSR 2.0 गुणवत्ता मोड:"},
        {"FSR 2.0 / 3 Quality Mode:", "Режим качества FSR 2.0 / 3:", "وضع جودة FSR 2.0 / 3:", "FSR 2.0 / 3 Qualitätsmodus:", "Mode de qualité FSR 2.0 / 3 :", "FSR 2.0 / 3 质量模式:", "FSR 2.0 / 3 品質モード:", "Modo de calidad FSR 2.0 / 3:", "FSR 2.0 / 3 품질 모드:", "Modalità qualità FSR 2.0 / 3:", "Modo de qualidade FSR 2.0 / 3:", "FSR 2.0 / 3 गुणवत्ता मोड:"},
        {"Selects the quality preset for AMD FSR 2.0 / 3 Temporal Upscaling.", "Выбирает пресет качества для темпорального масштабирования AMD FSR 2.0 / 3.", "يختار إعداد الجودة المسبق للترقية الزمنية AMD FSR 2.0 / 3.", "Wählt das Qualitätsprofil für temporales AMD FSR 2.0 / 3 Upscaling.", "Sélectionne le préréglage de qualité pour la mise à l'échelle temporelle AMD FSR 2.0 / 3.", "选择 AMD FSR 2.0 / 3 时间超分辨率缩放的质量预设。", "AMD FSR 2.0 / 3 テンポラルアップスケーリングの品質プリセットを選択します。", "Selecciona el preajuste de calidad para el reescalado temporal AMD FSR 2.0 / 3.", "AMD FSR 2.0 / 3 시간적 업스케일링 품질 프리셋을 선택합니다.", "Seleziona il profilo di qualità per l'upscaling temporale AMD FSR 2.0 / 3.", "Seleciona a predefinição de qualidade para o upscaling temporal AMD FSR 2.0 / 3.", "AMD FSR 2.0 / 3 टेम्पोरल अपस्केलिंग के लिए गुणवत्ता प्रीसेट चुनता है।"},
        {"Quality (67% - 1.5x)", "Качество (67% - 1.5x)", "الجودة (67% - 1.5x)", "Qualität (67% - 1.5x)", "Qualité (67% - 1.5x)", "质量 (67% - 1.5x)", "クオリティ (67% - 1.5x)", "Calidad (67% - 1.5x)", "품질 (67% - 1.5x)", "Qualità (67% - 1.5x)", "Qualidade (67% - 1.5x)", "गुणवत्ता (67% - 1.5x)"},
        {"Balanced (59% - 1.7x)", "Баланс (59% - 1.7x)", "متوازن (59% - 1.7x)", "Ausgeglichen (59% - 1.7x)", "Équilibré (59% - 1.7x)", "平衡 (59% - 1.7x)", "バランス (59% - 1.7x)", "Equilibrado (59% - 1.7x)", "균형 (59% - 1.7x)", "Bilanciato (59% - 1.7x)", "Equilibrado (59% - 1.7x)", "संतुलित (59% - 1.7x)"},
        {"Performance (50% - 2.0x)", "Производительность (50% - 2.0x)", "الأداء (50% - 2.0x)", "Leistung (50% - 2.0x)", "Performance (50% - 2.0x)", "性能 (50% - 2.0x)", "パフォーマンス (50% - 2.0x)", "Rendimiento (50% - 2.0x)", "성능 (50% - 2.0x)", "Prestazioni (50% - 2.0x)", "Desempenho (50% - 2.0x)", "प्रदर्शन (50% - 2.0x)"},
        {"Ultra Performance (33% - 3.0x)", "Ультра-производительность (33% - 3.0x)", "أداء فائق (33% - 3.0x)", "Ultra-Leistung (33% - 3.0x)", "Ultra performance (33% - 3.0x)", "超高性能 (33% - 3.0x)", "ウルトラパフォーマンス (33% - 3.0x)", "Rendimiento ultra (33% - 3.0x)", "울트라 성능 (33% - 3.0x)", "Prestazioni ultra (33% - 3.0x)", "Desempenho ultra (33% - 3.0x)", "अल्ट्रा प्रदर्शन (33% - 3.0x)"},
        {"AMD FSR 3 (Temporal Upscaling + RCAS)", "AMD FSR 3 (Темпоральный апскейлинг + RCAS)", "AMD FSR 3 (ترقية زمنية + RCAS)", "AMD FSR 3 (Temporales Upscaling + RCAS)", "AMD FSR 3 (Mise à l'échelle temporelle + RCAS)", "AMD FSR 3 (时间超分辨率 + RCAS)", "AMD FSR 3 (テンポラルアップスケーリング + RCAS)", "AMD FSR 3 (Reescalado temporal + RCAS)", "AMD FSR 3 (시간적 업스케일링 + RCAS)", "AMD FSR 3 (Upscaling temporale + RCAS)", "AMD FSR 3 (Upscaling temporal + RCAS)", "AMD FSR 3 (टेम्पोरल अपस्केलिंग + RCAS)"},

    };

    for (const auto& item : s_table) {
        if (std::strcmp(text, item.key) == 0) {
            if (cur_lang.rfind("ru", 0) == 0 && item.ru) return QString::fromUtf8(item.ru);
            if (cur_lang.rfind("ar", 0) == 0 && item.ar) return QString::fromUtf8(item.ar);
            if (cur_lang.rfind("de", 0) == 0 && item.de) return QString::fromUtf8(item.de);
            if (cur_lang.rfind("fr", 0) == 0 && item.fr) return QString::fromUtf8(item.fr);
            if (cur_lang.rfind("zh", 0) == 0 && item.zh) return QString::fromUtf8(item.zh);
            if (cur_lang.rfind("ja", 0) == 0 && item.ja) return QString::fromUtf8(item.ja);
            if (cur_lang.rfind("es", 0) == 0 && item.es) return QString::fromUtf8(item.es);
            if (cur_lang.rfind("ko", 0) == 0 && item.ko) return QString::fromUtf8(item.ko);
            if (cur_lang.rfind("it", 0) == 0 && item.it) return QString::fromUtf8(item.it);
            if (cur_lang.rfind("pt", 0) == 0 && item.pt) return QString::fromUtf8(item.pt);
            if (cur_lang.rfind("hi", 0) == 0 && item.hi) return QString::fromUtf8(item.hi);
        }
    }

    return QCoreApplication::translate("ConfigurationShared", text, disambiguation);
}

std::unique_ptr<TranslationMap> InitializeTranslations(QObject* parent) {
    std::unique_ptr<TranslationMap> translations = std::make_unique<TranslationMap>();
    const auto& tr = [](const char* text, const char* disambiguation = nullptr) -> QString {
        return TranslateConfigText(text, disambiguation);
    };

#define INSERT(SETTINGS, ID, NAME, TOOLTIP)                                                        \
    translations->insert(std::pair{SETTINGS::values.ID.Id(), std::pair{(NAME), (TOOLTIP)}})

    // A setting can be ignored by giving it a blank name

    // Applets
    INSERT(Settings, cabinet_applet_mode, tr("Amiibo editor"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, controller_applet_mode, tr("Controller configuration"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, data_erase_applet_mode, tr("Data erase"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, error_applet_mode, tr("Error"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, net_connect_applet_mode, tr("Net connect"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, player_select_applet_mode, tr("Player select"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, swkbd_applet_mode, tr("Software keyboard"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, mii_edit_applet_mode, tr("Mii Edit"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, web_applet_mode, tr("Online web"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, shop_applet_mode, tr("Shop"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, photo_viewer_applet_mode, tr("Photo viewer"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, offline_web_applet_mode, tr("Offline web"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, login_share_applet_mode, tr("Login share"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, wifi_web_auth_applet_mode, tr("Wifi web auth"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, my_page_applet_mode, tr("My page"), tr("Selects whether to use STORM SWITCH's fast custom frontend or the real Nintendo Switch firmware applet."));
    INSERT(Settings, enable_overlay, tr("Enable Overlay Applet"),
           tr("Enables Horizon\'s built-in overlay applet. Press and hold the home button for 1 "
              "second to show it."));

    // Audio
    INSERT(Settings, sink_id, tr("Output Engine:"), tr("Selects the host audio output backend (cubeb, SDL2, or null). Auto selects the lowest-latency backend."));
    INSERT(Settings, audio_output_device_id, tr("Output Device:"), tr("Selects the physical headphones or speakers used for game audio playback."));
    INSERT(Settings, audio_input_device_id, tr("Input Device:"), tr("Selects the microphone device used by games that support voice input."));
    INSERT(Settings, audio_muted, tr("Mute audio"), tr("Completely mutes all emulated game audio output."));
    INSERT(Settings, volume, tr("Volume:"), tr("Adjusts the master output volume of emulated game audio (0-200%)."));
    INSERT(Settings, dump_audio_commands, QString(), QString());
    INSERT(UISettings, mute_when_in_background, tr("Mute audio when in background"), tr("Automatically mutes game audio whenever the STORM SWITCH window loses focus."));

    // Quick Save States
    INSERT(Settings, save_states_mode, tr("Save States Mode:"),
           tr("Controls quick save and quick load behavior. Guest RAM/CPU-only save states can crash unverified games."));

    // Core
    INSERT(Settings, use_multi_core, tr("Multicore CPU Emulation"),
           tr("This option increases CPU emulation thread use from 1 to the maximum of 4.\n"
              "This is mainly a debug option and shouldn't be disabled."));

    // CPU Optimizations (Item 18)
    INSERT(Settings, cpuopt_page_tables, tr("CPU Page Tables"),
           tr("Optimizes virtual memory translation by utilizing CPU hardware page tables when available."));
    INSERT(Settings, cpuopt_block_linking, tr("CPU Block Linking"),
           tr("Links adjacent JIT translation blocks together to eliminate dispatcher roundtrips."));
    INSERT(Settings, cpuopt_const_prop, tr("CPU Constant Propagation"),
           tr("Evaluates static arithmetic constants during JIT compilation for faster execution."));
    INSERT(Settings, cpuopt_context_elimination, tr("CPU Context Elimination"),
           tr("Removes unnecessary register saves and restores between compatible JIT blocks."));
    INSERT(Settings, cpuopt_fast_dispatcher, tr("CPU Fast Dispatcher"),
           tr("Uses direct jump tables for block execution instead of a global hash lookup."));
    INSERT(Settings, cpuopt_fastmem, tr("CPU Fastmem"),
           tr("Maps emulated guest Switch memory directly into host address space for near-native speeds."));
    INSERT(Settings, cpuopt_fastmem_exclusives, tr("CPU Fastmem Exclusives"),
           tr("Optimizes atomic memory access instructions using host atomic instructions."));
    INSERT(Settings, cpuopt_ignore_memory_aborts, tr("CPU Ignore Memory Aborts"),
           tr("Suppresses non-fatal guest memory abort faults to prevent unnecessary game crashes."));
    INSERT(Settings, cpuopt_misc_ir, tr("CPU Misc IR Optimizations"),
           tr("Performs dead code elimination and redundant instruction stripping in intermediate code."));
    INSERT(Settings, cpuopt_recompile_exclusives, tr("CPU Recompile Exclusives"),
           tr("Recompiles LDREX/STREX synchronization loops for optimal host thread performance."));
    INSERT(Settings, cpuopt_reduce_misalign_checks, tr("CPU Reduce Misalignment Checks"),
           tr("Reduces runtime checks for unaligned memory accesses on host architectures supporting them."));
    INSERT(Settings, cpuopt_return_stack_buffer, tr("CPU Return Stack Buffer"),
           tr("Optimizes subroutine return predictions in emulated code blocks."));

    // Network & Input & Extra (Item 18)
    INSERT(Settings, network_interface, tr("Network Interface"), tr("Selects the host network adapter used for local wireless (LDN) and LAN multiplayer."));
    INSERT(Settings, udp_input_servers, tr("UDP Input Servers"), tr("Comma-separated DSU/Cemuhook UDP server addresses (IP:port) for external motion controllers."));
    INSERT(Settings, random_amiibo_id, tr("Randomize Amiibo ID"), tr("Generates a fresh random Amiibo UUID on every scan to bypass once-per-day scan limits in games."));
    INSERT(Settings, airplane_mode, tr("Airplane Mode"), tr("Simulates Nintendo Switch Airplane Mode, disabling virtual network connectivity inside games."));
    INSERT(Settings, disable_buffer_reorder, tr("Disable Buffer Reorder"), tr("Forces strict sequential GPU command buffer submission order to troubleshoot rare rendering glitches."));
    INSERT(Settings, disable_macro_hle, tr("Disable Macro HLE"), tr("Disables high-level GPU macro emulation and falls back to low-level macro execution for debugging."));
    INSERT(Settings, disable_macro_jit, tr("Disable Macro JIT"), tr("Disables the JIT compiler for GPU Maxwell macros and uses the interpreter instead."));
    INSERT(Settings, disable_shader_loop_safety_checks, tr("Disable Shader Loop Safety Checks"), tr("Removes infinite-loop guard checks inside compiled shaders for slightly faster GPU execution."));
    INSERT(Settings, enable_raw_input, tr("Raw Input"), tr("Reads mouse and keyboard input directly from the OS hardware layer with minimal latency."));
    INSERT(Settings, mouse_panning, tr("Mouse Panning"), tr("Maps desktop mouse movement to the right analog stick for camera control in 3D games."));
    INSERT(Settings, vibration_enabled, tr("Enable Vibration"), tr("Enables controller HD Rumble / vibration force feedback."));
    INSERT(Settings, motion_enabled, tr("Enable Motion"), tr("Enables gyroscope and accelerometer motion controls for compatible controllers."));
INSERT(Settings, memory_layout_mode, tr("Memory Layout (DRAM)"),
           tr("Increases the amount of emulated DRAM.\nDoesn't affect performance/stability but may "
              "allow HD texture "
              "mods to load."));
    INSERT(Settings, use_speed_limit, tr("Limit Speed Percent"), tr("Caps emulation speed to the configured percentage (100% = native console speed)."));
    INSERT(Settings, current_speed_mode, QString(), QString());
    INSERT(Settings, speed_limit, tr("Limit Speed Percent"),
           tr("Controls the game's maximum rendering speed, but it's up to each game if it runs "
              "faster or not.\n200% for a 30 FPS game is 60 FPS, and for a "
              "60 FPS game it will be 120 FPS.\nDisabling it means unlocking the framerate to the "
              "maximum your PC can reach."));

    INSERT(Settings, turbo_speed_limit, tr("Turbo Speed"),
           tr("When the Turbo Speed hotkey is pressed, the speed will be limited to this "
              "percentage."));
    INSERT(Settings, slow_speed_limit, tr("Slow Speed"),
           tr("When the Slow Speed hotkey is pressed, the speed will be limited to this "
              "percentage."));

    INSERT(Settings, sync_core_speed, tr("Synchronize Core Speed"),
           tr("Synchronizes CPU core speed with the game's maximum rendering speed to boost FPS "
              "without affecting game speed (animations, physics, etc.).\n"
              "Can help reduce stuttering at lower framerates."));
    INSERT(Settings, enable_cheats_by_default, tr("Enable cheats by default"),
           tr("Enables regular cheats when a game starts. Master Code entries are always enabled. "
              "Individual cheats can still be toggled from Cheats in the menu bar while the game is "
              "running."));
    INSERT(Settings, current_program_id, QString(), QString());

    // Cpu
    INSERT(Settings, cpu_accuracy, tr("Accuracy:"),
           tr("Change the accuracy of the emulated CPU (for debugging only)."));
    INSERT(Settings, cpu_backend, tr("Backend:"), tr("Selects the ARM64 CPU execution engine (Dynarmic JIT on PC, or NCE native execution on ARM64)."));

    INSERT(Settings, cpu_clock, tr("CPU Clocks"),
           tr("Raises the clock the emulated CPU reports, which removes some FPS limiters.\n"
              "Weaker CPUs may see reduced performance, and certain games may behave improperly."));

    INSERT(Settings, use_custom_cpu_ticks, tr("Custom CPU Ticks"), tr("Overrides the number of emulated CPU ticks executed per slice for fine-tuning timing."));
    INSERT(Settings, cpu_ticks, tr("Custom CPU Ticks"),
           tr("Set a custom value of CPU ticks. Higher values can increase performance, but may "
              "cause deadlocks. A range of 77-21000 is recommended."));

    // Cpu Debug

    // Cpu Unsafe
    INSERT(
        Settings, cpuopt_unsafe_host_mmu, tr("Enable Host MMU Emulation (fastmem)"),
        tr("This optimization speeds up memory accesses by the guest program.\nEnabling it causes "
           "guest memory reads/writes to be done directly into memory and make use of Host's "
           "MMU.\nDisabling this forces all memory accesses to use Software MMU Emulation."));
    INSERT(
        Settings, cpuopt_unsafe_unfuse_fma,
        tr("Unfuse FMA (improve performance on CPUs without FMA)"),
        tr("This option improves speed by reducing accuracy of fused-multiply-add instructions on "
           "CPUs without native FMA support."));
    INSERT(
        Settings, cpuopt_unsafe_reduce_fp_error, tr("Faster FRSQRTE and FRECPE"),
        tr("This option improves the speed of some approximate floating-point functions by using "
           "less accurate native approximations."));
    INSERT(Settings, cpuopt_unsafe_ignore_standard_fpcr,
           tr("Faster ASIMD instructions (32 bits only)"),
           tr("This option improves the speed of 32 bits ASIMD floating-point functions by running "
              "with incorrect rounding modes."));
    INSERT(Settings, cpuopt_unsafe_inaccurate_nan, tr("Inaccurate NaN handling"),
           tr("This option improves speed by removing NaN checking.\nPlease note this also reduces "
              "accuracy of certain floating-point instructions."));
    INSERT(Settings, cpuopt_unsafe_fastmem_check, tr("Disable address space checks"),
           tr("This option improves speed by eliminating a safety check before every memory "
              "operation.\nDisabling it may allow arbitrary code execution."));
    INSERT(
        Settings, cpuopt_unsafe_ignore_global_monitor, tr("Ignore global monitor"),
        tr("This option improves speed by relying only on the semantics of cmpxchg to ensure "
           "safety of exclusive access instructions.\nPlease note this may result in deadlocks and "
           "other race conditions."));

    // Renderer
    INSERT(Settings, renderer_backend, tr("API:"),
           tr("Changes the output graphics API.\nVulkan is recommended."));
    INSERT(Settings, vulkan_device, tr("Device:"),
           tr("This setting selects the GPU to use (Vulkan only)."));
    INSERT(Settings, resolution_setup, tr("Resolution:"),
           tr("Forces to render at a different resolution.\n"
              "Higher resolutions require more VRAM and bandwidth.\n"
              "Options lower than 1X can cause artifacts."));
    INSERT(Settings, scaling_filter, tr("Window Adapting Filter:"),
           tr("Selects the scaling filter used when adapting the rendered image to the window size.\nBilinear and Bicubic are standard filters.\nAMD FidelityFX Super Resolution (FSR) and Qualcomm SGSR provide sharp upscaling."));
        INSERT(Settings, fsr2_quality_mode, tr("FSR 2.0 / 3 Quality Mode:"),
           tr("Selects the quality preset for AMD FSR 2.0 / 3 Temporal Upscaling."));
    INSERT(Settings, fsr_sharpening_slider, tr("FSR Sharpness:"),
           tr("Determines how sharpened the image will look using FSR's or SGSR's dynamic contrast."));
    INSERT(Settings, anti_aliasing, tr("Anti-Aliasing Method:"),
           tr("The anti-aliasing method to use.\nSMAA offers the best quality.\nFXAA "
              "can produce a more stable picture in lower resolutions."));
    INSERT(Settings, fullscreen_mode, tr("Fullscreen Mode:"),
           tr("The method used to render the window in fullscreen.\nBorderless offers the best "
              "compatibility with the on-screen keyboard that some games request for "
              "input.\nExclusive "
              "fullscreen may offer better performance and better Freesync/Gsync support."));
    INSERT(Settings, aspect_ratio, tr("Aspect Ratio:"),
           tr("Stretches the renderer to fit the specified aspect ratio.\nMost games only support "
              "16:9, so modifications are required to get other ratios.\nAlso controls the "
              "aspect ratio of captured screenshots."));
    INSERT(Settings, auto_aspect_ratio_patch, tr("Auto-Patch 3D Aspect Ratio (No Stretch)"),
           tr("Automatically patches 3D camera frustum constants in game executables to match the selected aspect ratio (4:3, 16:10, 18:9, 21:9, 32:9, 48:9, etc.) without stretching 3D geometry."));
    INSERT(Settings, auto_60fps_unlock, tr("Auto-Unlock 60/120 FPS"),
           tr("Automatically unlocks 30 FPS VSync/swap-interval caps to allow 60/120 FPS rendering."));
    INSERT(Settings, use_disk_shader_cache, tr("Use persistent pipeline cache"),
           tr("Allows saving shaders to storage for faster loading on following game "
              "boots.\nDisabling it is only intended for debugging."));
    INSERT(
        Settings, use_asynchronous_gpu_emulation, tr("Use asynchronous GPU emulation"),
        tr("Uses an extra CPU thread for rendering.\nThis option should always remain enabled."));
    INSERT(Settings, nvdec_emulation, tr("NVDEC emulation:"),
           tr("Specifies how videos should be decoded.\nIt can either use the CPU or the GPU for "
              "decoding, or perform no decoding at all (black screen on videos).\n"
              "In most cases, GPU decoding provides the best performance."));
    INSERT(Settings, accelerate_astc, tr("ASTC Decoding Method:"),
           tr("This option controls how ASTC textures should be decoded.\n"
              "CPU: Use the CPU for decoding.\n"
              "GPU: Use the GPU's compute shaders to decode ASTC textures (recommended).\n"
              "CPU Asynchronously: Use the CPU to decode ASTC textures on demand. Eliminates"
              "ASTC decoding\nstuttering but may present artifacts."));
    INSERT(Settings, astc_recompression, tr("ASTC Recompression Method:"),
           tr("Most GPUs lack support for ASTC textures and must decompress to an"
              "intermediate format: RGBA8.\n"
              "BC1/BC3: The intermediate format will be recompressed to BC1 or BC3 format,\n"
              " saving VRAM but degrading image quality."));
    INSERT(Settings, frame_pacing_mode, tr("Frame Pacing Mode (Vulkan only)"),
           tr("Controls how the emulator manages frame pacing to reduce stuttering and make the "
              "frame rate smoother and more consistent."));
    INSERT(Settings, vram_usage_mode, tr("VRAM Usage Mode:"),
           tr("Selects whether the emulator should prefer to conserve memory or make maximum usage "
              "of available video memory for performance.\nAggressive mode may impact performance "
              "of other applications such as recording software."));
    INSERT(Settings, skip_cpu_inner_invalidation, tr("Skip CPU Inner Invalidation"),
           tr("Skips certain cache invalidations during memory updates, reducing CPU usage and "
              "improving latency. This may cause soft-crashes."));
    INSERT(Settings, vsync_mode, tr("VSync Mode:"),
           tr("FIFO (VSync) does not drop frames or exhibit tearing but is limited by the screen "
              "refresh rate.\nFIFO Relaxed allows tearing as it recovers from a slow down.\n"
              "Mailbox can have lower latency than FIFO and does not tear but may drop "
              "frames.\nImmediate (no synchronization) presents whatever is available and can "
              "exhibit tearing."));
    INSERT(Settings, frame_gen, tr("Enable Frame Generation (LSFG)"),
           tr("Generates intermediate frames using Lossless Scaling optical flow to double or quadruple FPS."));
    INSERT(Settings, frame_gen_multiplier, tr("Frame Generation Multiplier:"),
           tr("Multiplier for generated frames (2x = double FPS, 3x = triple FPS, 4x = quadruple FPS)."));
    INSERT(Settings, frame_gen_target_rate, tr("Target Frame Rate (Hz):"),
           tr("Target output frame rate in Hz for Frame Generation."));
    INSERT(Settings, frame_gen_flow_scale, tr("Optical Flow Scale (%):"),
           tr("Manual resolution scale percentage for optical flow estimation."));
    INSERT(Settings, frame_gen_queue_target, tr("Frame Queue Depth:"),
           tr("Number of frames queued for optical flow interpolation."));
    INSERT(Settings, frame_gen_dump_flow, tr("Debug Optical Flow Vectors"),
           tr("Visualizes optical flow motion vectors on screen for debugging."));
    INSERT(Settings, bg_red, QString(), QString());
    INSERT(Settings, bg_green, QString(), QString());
    INSERT(Settings, bg_blue, QString(), QString());

    // Renderer (Advanced Graphics)
    INSERT(Settings, use_asynchronous_gpu_emulation, QString(), QString());

    INSERT(Settings, sync_memory_operations, tr("Sync Memory Operations"),
           tr("Ensures data consistency between compute and memory operations.\nThis option fixes "
              "issues in games, but may degrade performance.\nUnreal Engine 4 games often see the "
              "most significant changes thereof."));
    INSERT(Settings, async_presentation, tr("Enable asynchronous presentation (Vulkan only)"),
           tr("Slightly improves performance by moving presentation to a separate CPU thread."));
    INSERT(
        Settings, renderer_force_max_clock, tr("Force maximum clocks (Vulkan only)"),
        tr("Runs work in the background while waiting for graphics commands to keep the GPU from "
           "lowering its clock speed."));
    INSERT(Settings, max_anisotropy, tr("Anisotropic Filtering:"),
           tr("Controls the quality of texture rendering at oblique angles.\nSafe to set at 16x on "
              "most GPUs."));
    INSERT(Settings, gpu_accuracy, tr("GPU Mode:"),
           tr("Controls the GPU emulation mode.\nMost games render fine with Fast, but Accurate is still "
              "required for some.\nParticles tend to only render correctly with Accurate mode."));
    INSERT(Settings, dma_accuracy, tr("DMA Accuracy:"),
           tr("Controls the DMA read mode.\nUnsafe is faster, while Safe is more stable and can fix issues in some games.\nDefault follows the GPU Accuracy setting."));
    INSERT(Settings, gpu_fence_behavior, tr("GPU Fence Behavior:"),
           tr("Controls the GPU fence synchronization behavior.\nImmediate is the fastest option, but can introduce some issues.\nBalanced offers better compatibility and may fix issues in some games.\nAccurate further improves compatibility at the cost of some performance.\nStrict is the slowest option, but can fix issues that require stricter synchronization.\nDefault follows the GPU Accuracy setting."));
    INSERT(Settings, enable_gpu_buffer_readback, tr("Enable GPU buffer readback"),
           tr("Preserves GPU-modified data by reading it back before uploading.\nSome games require this to render certain effects properly."));
    INSERT(Settings, use_asynchronous_shaders, tr("Enable asynchronous shader compilation"),
           tr("May reduce shader stutter."));
    INSERT(Settings, gpu_clock, tr("GPU Clocks"),
           tr("Makes the game believe GPU work finishes faster than it does, so it stops lowering "
              "resolution and render distance to fit the Switch's clocks."));
    INSERT(Settings, gpu_unswizzle_enabled, tr("GPU Unswizzle"),
           tr("Accelerates BCn 3D texture decoding using GPU compute.\n"
              "Disable if experiencing crashes or graphical glitches."));
    INSERT(Settings, gpu_unswizzle_texture_size, tr("GPU Unswizzle Max Texture Size"),
           tr("Sets the maximum size (MiB) for GPU-based texture unswizzling.\n"
              "While the GPU is faster for medium and large textures, the CPU may be more "
              "efficient for very small ones.\n"
              "Adjust this to find the balance between GPU acceleration and CPU overhead."));
    INSERT(Settings, gpu_unswizzle_stream_size, tr("GPU Unswizzle Stream Size"),
           tr("Sets the maximum amount of texture data (in MiB) processed per frame.\n"
              "Higher values can reduce stutter during texture loading but may impact frame "
              "consistency."));
    INSERT(Settings, gpu_unswizzle_chunk_size, tr("GPU Unswizzle Chunk Size"),
           tr("Determines the number of depth slices processed in a single dispatch.\n"
              "Increasing this can improve throughput on high-end GPUs but may cause TDR or driver "
              "timeouts on weaker hardware."));

    INSERT(Settings, use_vulkan_driver_pipeline_cache, tr("Use Vulkan pipeline cache"),
           tr("Enables GPU vendor-specific pipeline cache.\nThis option can improve shader loading "
              "time significantly in cases where the Vulkan driver does not store pipeline cache "
              "files internally."));
    INSERT(Settings, enable_compute_pipelines, tr("Enable Compute Pipelines (Intel Vulkan Only)"),
           tr("Required by some games.\nThis setting only exists for Intel "
              "proprietary drivers and may crash if enabled.\nCompute pipelines are always enabled "
              "on all other drivers."));
    INSERT(Settings, enable_frame_interpolation, tr("Enable Frame Interpolation"),
           tr("Generates intermediate frames using Lossless Scaling / LSFG temporal interpolation."));
    INSERT(Settings, pipeline_worker_threads, tr("Pipeline Compilation Threads"),
           tr("Number of background worker threads for Vulkan pipeline compilation (0 = Auto)."));
    INSERT(
        Settings, use_reactive_flushing, tr("Enable Reactive Flushing"),
        tr("Uses reactive flushing instead of predictive flushing, allowing more accurate memory "
           "syncing."));
    INSERT(Settings, use_video_framerate, tr("Sync to framerate of video playback"),
           tr("Run the game at normal speed during video playback, even when the framerate is "
              "unlocked."));
    INSERT(Settings, barrier_feedback_loops, tr("Barrier feedback loops"),
           tr("Improves rendering of transparency effects in specific games."));
    INSERT(Settings, enable_buffer_history, tr("Enable buffer history"),
           tr("Enables access to previous buffer states.\nThis option may improve rendering "
              "quality and performance consistency in some games."));
    INSERT(Settings, fix_bloom_effects, tr("Fix bloom effects"), tr("Removes bloom in Burnout."));

    INSERT(Settings, post_processing_shader, tr("Post-Processing Shader:"),
           tr("Applies a RetroArch post-processing shader effect (CRT, scanlines, sharpening, color grading, bloom) to the final rendered image."));
#ifdef __ANDROID__
    INSERT(Settings, use_optimized_vertex_buffers, tr("Use Optimized Vertex Buffers"),
           tr("Optimizes vertex buffer memory allocations and uploads in Vulkan to reduce CPU overhead and improve geometry throughput."));
#endif
    INSERT(Settings, rescale_hack, tr("Enable Legacy Rescale Pass"),
           tr("May fix rescale issues in some games by relying on behavior from the previous "
              "implementation.\n"
              "Legacy behavior workaround that fixes line artifacts on AMD and Intel GPUs, and "
              "grey texture flicker on Nvidia GPUs in Luigis Mansion 3."));

    // Renderer (Extensions)
    INSERT(Settings, dyna_state, tr("Extended Dynamic State"),
           tr("Controls the number of features that can be used in Extended Dynamic State.\n"
              "Higher states allow for more features and can increase performance, but may cause "
              "additional graphical issues."));

    INSERT(Settings, vertex_input_dynamic_state, tr("Vertex Input Dynamic State"),
           tr("Enables vertex input dynamic state feature for better quality and performance."));

    INSERT(
        Settings, sample_shading, tr("Sample Shading"),
        tr("Allows the fragment shader to execute per sample in a multi-sampled fragment "
           "instead of once per fragment. Improves graphics quality at the cost of performance.\n"
           "Higher values improve quality but degrade performance."));

    // Renderer (Debug)

    // System
    INSERT(Settings, rng_seed, tr("RNG Seed"),
           tr("Controls the seed of the random number generator.\nMainly used for speedrunning."));
    INSERT(Settings, rng_seed_enabled, tr("Custom RNG Seed"), tr("Uses a fixed pseudo-random number generator seed for deterministic TAS and debugging runs."));
    INSERT(Settings, device_name, tr("Device Name"), tr("The name of the console."));
    INSERT(Settings, program_args, tr("Homebrew Args"),
           tr("Command-line arguments passed to homebrew at launch (e.g. -noglsl)."));
    INSERT(Settings, custom_rtc, tr("Custom RTC Date:"),
           tr("This option allows to change the clock of the console.\n"
              "Can be used to manipulate time in games."));
    INSERT(Settings, custom_rtc_enabled, tr("Custom RTC Date:"), tr("Overrides the emulated Nintendo Switch system clock with a custom date and time."));
    INSERT(Settings, custom_rtc_offset, tr("Custom RTC Offset:"),
           tr("The number of seconds from the current unix time"));
    INSERT(Settings, language_index, tr("Language:"),
           tr("This option can be overridden when region setting is auto-select"));
    INSERT(Settings, region_index, tr("Region:"), tr("The region of the console."));
    INSERT(Settings, time_zone_index, tr("Time Zone:"), tr("The time zone of the console."));
    INSERT(Settings, sound_index, tr("Sound Output Mode:"), tr("Selects the console audio channel layout (Mono, Stereo, or 5.1 Surround)."));
    INSERT(Settings, use_docked_mode, tr("Console Mode:"),
           tr("Selects if the console is in Docked or Handheld mode.\nGames will change "
              "their resolution, details and supported controllers and depending on this setting.\n"
              "Setting to Handheld can help improve performance for low end systems."));
    INSERT(Settings, current_user, QString(), QString());

    // Controls

    // Data Storage

    // Debugging

    // Debugging Graphics

    // Network

    // Web Service

    // Ui

    // Ui General
    INSERT(UISettings, select_user_on_boot, tr("Prompt for user profile on boot"),
           tr("Useful if multiple people use the same PC."));
    INSERT(UISettings, pause_when_in_background, tr("Pause when not in focus"),
           tr("Pauses emulation when focusing on other windows."));
    INSERT(UISettings, confirm_before_stopping, tr("Confirm before stopping emulation"),
           tr("Overrides prompts asking to confirm stopping the emulation.\nEnabling "
              "it bypasses such prompts and directly exits the emulation."));
    INSERT(UISettings, hide_mouse, tr("Hide mouse on inactivity"),
           tr("Hides the mouse after 2.5s of inactivity."));
    INSERT(UISettings, controller_applet_disabled, tr("Disable controller applet"),
           tr("Forcibly disables the use of the controller applet in emulated programs.\n"
              "When a program attempts to open the controller applet, it is immediately closed."));
    INSERT(UISettings, check_for_updates, tr("Check for updates"),
           tr("Whether or not to check for updates upon startup."));

    // Linux
    INSERT(UISettings, enable_gamemode, tr("Enable Gamemode"), tr("Activates Feral GameMode on Linux to request high-performance CPU governor and I/O priority."));
#ifdef __unix__
    INSERT(UISettings, gui_force_x11, tr("Force X11 as Graphics Backend"), tr("Forces Qt to run via XWayland (xcb) instead of native Wayland on Linux desktops."));
    INSERT(UISettings, gui_hide_backend_warning, QString(), QString());
#endif

    // Ui Debugging

    // Ui Multiplayer

    // Ui Games list

    // STORM Optimization Settings
    INSERT(Settings, cpu_affinity_pinning, tr("CPU Affinity Pinning"),
           tr("Pins emulation threads to physical performance cores."));
    INSERT(Settings, use_storm_tier2_compiler, tr("STORM Tier-2 JIT Compiler"),
           tr("Enables the zero-overhead STORM Tier-2 instruction pre-optimizer and superblock JIT tuner (canonicalizes ARMv8.3 PAC/BTI hints and accelerates CNTVCT_EL0 counter reads)."));
    INSERT(Settings, eco_frame_pacing, tr("Eco Frame Pacing"),
           tr("Reduces power draw and frame stuttering with adaptive delivery."));
    INSERT(Settings, drs_resolution_lock, tr("Lock Dynamic Resolution"),
           tr("Locks dynamic resolution scale to prevent resolution drops."));
    INSERT(Settings, smart_shader_throttle, tr("Smart Shader Throttle"),
           tr("Dynamically regulates priority of background shader compilation threads to prevent microstutters."));
    INSERT(Settings, vram_garbage_collection, tr("VRAM Garbage Collection"),
           tr("Periodic background collection of unused texture buffers and ASTC cache to prevent VRAM leaks."));
    INSERT(Settings, vram_budget_governor, tr("VRAM Budget Governor"),
           tr("Prevents out-of-memory crashes by unloading textures when VRAM exceeds 85%."));
    INSERT(Settings, storm_lowend_turbo, tr("Low-End Turbo"),
           tr("Maximizes smoothness on entry-level multi-core processors and weak GPUs."));
    INSERT(Settings, storm_thermal_governor, tr("Thermal Governor"),
           tr("Reduces CPU and GPU temperatures by 10-15°C during frame wait intervals without dropping target frame rate."));
    INSERT(Settings, renderer_force_max_clock, tr("Force Maximum Clocks (PC and Mobile)"),
           tr("Locks maximum performance for CPU and GPU, disables aggressive throttling and power saving."));
    INSERT(Settings, enable_hdr10, tr("Enable HDR10"),
           tr("Enables HDR10 (BT.2020 PQ / ST2084) color space for compatible HDR and OLED displays."));
    INSERT(Settings, low_latency_display, tr("Low Latency Display"),
           tr("Minimizes presentation queue depth to reduce display input latency."));
    INSERT(Settings, frame_gen_fp16, tr("FP16 Frame Generation"),
           tr("Uses 16-bit half precision floating point for frame generation."));
    INSERT(Settings, frame_gen_flow_scale_auto, tr("Auto Optical Flow Scale"),
           tr("Automatically calculates optical flow grid scale for frame generation."));
    INSERT(Settings, emulate_bgr565, tr("Emulate BGR565"),
           tr("Emulates BGR565 color format by software swapping red and blue channels."));
    INSERT(Settings, early_release_fences, tr("Early Release Fences"),
           tr("Releases synchronization fences earlier to reduce frame presentation latency."));
    INSERT(Settings, optimize_spirv_output, tr("Optimize SPIR-V Output"),
           tr("Runs optimization passes on compiled SPIR-V shaders."));
    INSERT(Settings, use_fast_gpu_time, tr("Fast GPU Time"),
           tr("Uses fast monotonic GPU timer queries for frame pacing."));
    INSERT(Settings, enable_frame_skipping, tr("Enable Frame Skipping"),
           tr("Skips rendering non-critical frames when emulation falls behind target rate."));
    INSERT(Settings, eco_thermal_mode, tr("Eco Thermal Mode"),
           tr("Reduces power consumption and fan noise on portable devices."));
    INSERT(Settings, dynamic_performance_scaler, tr("Dynamic performance scaler"),
           tr("Automatically adjusts rendering resolution to maintain the target frame rate. "
              "When frames take too long, resolution steps down; when frames are fast, it recovers. "
              "Inspired by Atmosphere's dynamic resolution system."));
    INSERT(UISettings, enable_floating_translate_button, tr("Floating Translate Button"),
           tr("Displays floating quick translation overlay button."));

#undef INSERT


#ifdef _DEBUG
    // Item 18: Startup validation that every Setting in linkage has a translation
    for (const auto& [category, category_settings] : Settings::values.linkage.by_category) {
        for (const auto* setting : category_settings) {
            if (setting && !setting->GetLabel().empty()) {
                if (!translations->contains(setting->Id())) {
                    LOG_WARNING(Frontend, "Setting '{}' (ID {}) lacks an explicit translation entry!",
                                setting->GetLabel(), setting->Id());
                }
            }
        }
    }
#endif

    return translations;
}

std::unique_ptr<ComboboxTranslationMap> ComboboxEnumeration(QObject* parent) {
    std::unique_ptr<ComboboxTranslationMap> translations =
        std::make_unique<ComboboxTranslationMap>();
    const auto& tr = [](const char* text, const char* disambiguation = nullptr) -> QString {
        return TranslateConfigText(text, disambiguation);
    };

#define PAIR(ENUM, VALUE, TRANSLATION) {static_cast<u32>(Settings::ENUM::VALUE), (TRANSLATION)}

    // Intentionally skipping VSyncMode to let the UI fill that one out
    translations->insert({Settings::EnumMetadata<Settings::SaveStatesMode>::Index(),
                          {
                              PAIR(SaveStatesMode, Off, tr("Off")),
                              PAIR(SaveStatesMode, VerifiedOnly, tr("Verified games only")),
                              PAIR(SaveStatesMode, AllGames, tr("All games (experimental)")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::AppletMode>::Index(),
                          {
                              PAIR(AppletMode, HLE, tr("Custom frontend")),
                              PAIR(AppletMode, LLE, tr("Real applet")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::SpirvOptimizeMode>::Index(),
                          {
                              PAIR(SpirvOptimizeMode, Never, tr("Never")),
                              PAIR(SpirvOptimizeMode, OnLoad, tr("On Load")),
                              PAIR(SpirvOptimizeMode, Always, tr("Always")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AstcDecodeMode>::Index(),
                          {
                              PAIR(AstcDecodeMode, Cpu, tr("CPU")),
                              PAIR(AstcDecodeMode, Gpu, tr("GPU")),
                              PAIR(AstcDecodeMode, CpuAsynchronous, tr("CPU Asynchronous")),
                              PAIR(AstcDecodeMode, Hybrid, tr("Hybrid")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::AstcRecompression>::Index(),
         {
             PAIR(AstcRecompression, Uncompressed, tr("Uncompressed (Best quality)")),
             PAIR(AstcRecompression, Bc1, tr("BC1 (Low quality)")),
             PAIR(AstcRecompression, Bc3, tr("BC3 (Medium quality)")),
             PAIR(AstcRecompression, Bc5, tr("BC5 (High quality)")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::FramePacingMode>::Index(),
                          {
                              PAIR(FramePacingMode, Target_Auto, tr("Auto")),
                              PAIR(FramePacingMode, Target_30, tr("30 FPS")),
                              PAIR(FramePacingMode, Target_60, tr("60 FPS")),
                              PAIR(FramePacingMode, Target_90, tr("90 FPS")),
                              PAIR(FramePacingMode, Target_120, tr("120 FPS")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::VramUsageMode>::Index(),
                          {
                              PAIR(VramUsageMode, Conservative, tr("Conservative")),
                              PAIR(VramUsageMode, Normal, tr("Normal")),
                              PAIR(VramUsageMode, Aggressive, tr("Aggressive")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::RendererBackend>::Index(),
         {PAIR(RendererBackend, Vulkan, tr("Vulkan")),
#ifdef HAS_OPENGL
          PAIR(RendererBackend, OpenGL_GLSL, tr("OpenGL GLSL")),
          PAIR(RendererBackend, OpenGL_GLASM, tr("OpenGL GLASM (Assembly Shaders, NVIDIA Only)")),
          PAIR(RendererBackend, OpenGL_SPIRV, tr("OpenGL SPIR-V (Experimental, AMD/Mesa Only)")),
#endif
          PAIR(RendererBackend, Null, tr("Null"))}});
    translations->insert({Settings::EnumMetadata<Settings::GpuAccuracy>::Index(),
                          {
                              PAIR(GpuAccuracy, Low, tr("Fast")),
                              PAIR(GpuAccuracy, Normal, tr("Normal")),
                              PAIR(GpuAccuracy, High, tr("Accurate")),
                              PAIR(GpuAccuracy, Extreme, tr("Extreme")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::DmaAccuracy>::Index(),
                          {
                              PAIR(DmaAccuracy, Default, tr("Default")),
                              PAIR(DmaAccuracy, Normal, tr("Normal")),
                              PAIR(DmaAccuracy, Unsafe, tr("Unsafe (fast)")),
                              PAIR(DmaAccuracy, Safe, tr("Safe (stable)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuFenceBehavior>::Index(),
                          {
                              PAIR(GpuFenceBehavior, Default, tr("Default")),
                              PAIR(GpuFenceBehavior, Immediate, tr("Immediate")),
                              PAIR(GpuFenceBehavior, Balanced, tr("Balanced")),
                              PAIR(GpuFenceBehavior, Accurate, tr("Accurate")),
                              PAIR(GpuFenceBehavior, Strict, tr("Strict")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::CpuAccuracy>::Index(),
         {
             PAIR(CpuAccuracy, Auto, tr("Auto")),
             PAIR(CpuAccuracy, Accurate, tr("Accurate")),
             PAIR(CpuAccuracy, Unsafe, tr("Unsafe")),
             PAIR(CpuAccuracy, Paranoid, tr("Paranoid (disables most optimizations)")),
             PAIR(CpuAccuracy, Debugging, tr("Debugging")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::CpuBackend>::Index(),
                          {
                              PAIR(CpuBackend, Dynarmic, tr("Dynarmic")),
                              PAIR(CpuBackend, Nce, tr("NCE")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::FullscreenMode>::Index(),
                          {
                              PAIR(FullscreenMode, Borderless, tr("Borderless Windowed")),
                              PAIR(FullscreenMode, Exclusive, tr("Exclusive Fullscreen")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::NvdecEmulation>::Index(),
                          {
                              PAIR(NvdecEmulation, Off, tr("No Video Output")),
                              PAIR(NvdecEmulation, Cpu, tr("CPU Video Decoding")),
                              PAIR(NvdecEmulation, Gpu, tr("GPU Video Decoding (Default)")),
                              PAIR(NvdecEmulation, Hybrid, tr("Hybrid Video Decoding")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::ResolutionSetup>::Index(),
         {
             PAIR(ResolutionSetup, Res1_4X, tr("0.25X (180p/270p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res1_2X, tr("0.5X (360p/540p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res3_4X, tr("0.75X (540p/810p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res1X, tr("1X (720p/1080p)")),
             PAIR(ResolutionSetup, Res5_4X, tr("1.25X (900p/1350p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res3_2X, tr("1.5X (1080p/1620p) [EXPERIMENTAL]")),
             PAIR(ResolutionSetup, Res2X, tr("2X (1440p/2160p)")),
             PAIR(ResolutionSetup, Res3X, tr("3X (2160p/3240p)")),
             PAIR(ResolutionSetup, Res4X, tr("4X (2880p/4320p)")),
             PAIR(ResolutionSetup, Res5X, tr("5X (3600p/5400p)")),
             PAIR(ResolutionSetup, Res6X, tr("6X (4320p/6480p)")),
             PAIR(ResolutionSetup, Res7X, tr("7X (5040p/7560p)")),
             PAIR(ResolutionSetup, Res8X, tr("8X (5760p/8640p)")),
         }});
        translations->insert({Settings::EnumMetadata<Settings::Fsr2QualityMode>::Index(),
                          {
                              PAIR(Fsr2QualityMode, Quality, tr("Quality (67% - 1.5x)")),
                              PAIR(Fsr2QualityMode, Balanced, tr("Balanced (59% - 1.7x)")),
                              PAIR(Fsr2QualityMode, Performance, tr("Performance (50% - 2.0x)")),
                              PAIR(Fsr2QualityMode, UltraPerformance, tr("Ultra Performance (33% - 3.0x)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::ScalingFilter>::Index(),
                          {
                              PAIR(ScalingFilter, NearestNeighbor, tr("Nearest Neighbor")),
                              PAIR(ScalingFilter, Bilinear, tr("Bilinear")),
                              PAIR(ScalingFilter, Bicubic, tr("Bicubic")),
                              PAIR(ScalingFilter, Gaussian, tr("Gaussian")),
                              PAIR(ScalingFilter, Lanczos, tr("Lanczos 3 (Anti-Ringing EWA)")),
                              PAIR(ScalingFilter, ScaleForce, tr("ScaleForce")),
                              PAIR(ScalingFilter, Fsr, tr("AMD FSR 2.0 (Temporal Upscaling)")),
                              PAIR(ScalingFilter, Fsr3, tr("AMD FSR 3 (Temporal Upscaling + RCAS)")),
                              PAIR(ScalingFilter, Area, tr("Area (Pixel Area Super-Sampling)")),
                              PAIR(ScalingFilter, ZeroTangent, tr("Zero-Tangent")),
                              PAIR(ScalingFilter, BSpline, tr("B-Spline")),
                              PAIR(ScalingFilter, Mitchell, tr("Mitchell")),
                              PAIR(ScalingFilter, Spline1, tr("Spline-1")),
                              PAIR(ScalingFilter, Mmpx, tr("MMPX")),
                              PAIR(ScalingFilter, Sgsr, tr("Snapdragon Game Super Resolution")),
                              PAIR(ScalingFilter, SgsrEdge, tr("Snapdragon Game Super Resolution EdgeDir")),
                              PAIR(ScalingFilter, Cas, tr("AMD FidelityFX CAS (Contrast Adaptive)")),
                              PAIR(ScalingFilter, OmniScale, tr("OmniScale HQ (Pixel-Art & 2D/3D)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AntiAliasing>::Index(),
                          {
                              PAIR(AntiAliasing, None, tr("None")),
                              PAIR(AntiAliasing, Fxaa, tr("FXAA")),
                              PAIR(AntiAliasing, Smaa, tr("SMAA")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AspectRatio>::Index(),
                          {
                              PAIR(AspectRatio, R16_9, tr("Default (16:9)")),
                              PAIR(AspectRatio, R4_3, tr("Force 4:3")),
                              PAIR(AspectRatio, R21_9, tr("Force 21:9")),
                              PAIR(AspectRatio, R16_10, tr("Force 16:10")),
                              PAIR(AspectRatio, Stretch, tr("Stretch to Window")),
                              PAIR(AspectRatio, R32_9, tr("Force 32:9 (Super UltraWide)")),
                              PAIR(AspectRatio, R1_1, tr("Force 1:1 (Square)")),
                              PAIR(AspectRatio, R3_2, tr("Force 3:2 (Portable / Surface)")),
                              PAIR(AspectRatio, R5_4, tr("Force 5:4 (SXGA)")),
                              PAIR(AspectRatio, R18_9, tr("Force 18:9 (2:1 Univisium)")),
                              PAIR(AspectRatio, R19_5_9, tr("Force 19.5:9 (Modern Mobile)")),
                              PAIR(AspectRatio, R20_9, tr("Force 20:9 (Cinema Mobile)")),
                              PAIR(AspectRatio, R21_10, tr("Force 21:10 (Wide 16:10)")),
                              PAIR(AspectRatio, R24_10, tr("Force 24:10 (3840x1600 UltraWide+)")),
                              PAIR(AspectRatio, R32_10, tr("Force 32:10 (Super UltraWide 16:10)")),
                              PAIR(AspectRatio, R48_9, tr("Force 48:9 (Triple Monitor 3x16:9)")),
                              PAIR(AspectRatio, Auto_Window, tr("Auto Window Aspect (True 3D Fit)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::AnisotropyMode>::Index(),
                          {
                              PAIR(AnisotropyMode, Automatic, tr("Automatic")),
                              PAIR(AnisotropyMode, Default, tr("Default")),
                              PAIR(AnisotropyMode, X2, tr("2x")),
                              PAIR(AnisotropyMode, X4, tr("4x")),
                              PAIR(AnisotropyMode, X8, tr("8x")),
                              PAIR(AnisotropyMode, X16, tr("16x")),
                              PAIR(AnisotropyMode, X32, tr("32x")),
                              PAIR(AnisotropyMode, X64, tr("64x")),
                              PAIR(AnisotropyMode, None, tr("None")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::Language>::Index(),
         {
             PAIR(Language, Japanese, tr("Japanese (日本語)")),
             PAIR(Language, EnglishAmerican, tr("American English")),
             PAIR(Language, French, tr("French (français)")),
             PAIR(Language, German, tr("German (Deutsch)")),
             PAIR(Language, Italian, tr("Italian (italiano)")),
             PAIR(Language, Spanish, tr("Spanish (español)")),
             PAIR(Language, Chinese, tr("Chinese")),
             PAIR(Language, Korean, tr("Korean (한국어)")),
             PAIR(Language, Dutch, tr("Dutch (Nederlands)")),
             PAIR(Language, Portuguese, tr("Portuguese (português)")),
             PAIR(Language, Russian, tr("Russian (Русский)")),
             PAIR(Language, Taiwanese, tr("Taiwanese")),
             PAIR(Language, EnglishBritish, tr("British English")),
             PAIR(Language, FrenchCanadian, tr("Canadian French")),
             PAIR(Language, SpanishLatin, tr("Latin American Spanish")),
             PAIR(Language, ChineseSimplified, tr("Simplified Chinese")),
             PAIR(Language, ChineseTraditional, tr("Traditional Chinese (正體中文)")),
             PAIR(Language, PortugueseBrazilian, tr("Brazilian Portuguese (português do Brasil)")),
             PAIR(Language, Polish, tr("Polish (polski)")),
             PAIR(Language, Thai, tr("Thai (แบบไทย)")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::Region>::Index(),
                          {
                              PAIR(Region, Japan, tr("Japan")),
                              PAIR(Region, Usa, tr("USA")),
                              PAIR(Region, Europe, tr("Europe")),
                              PAIR(Region, Australia, tr("Australia")),
                              PAIR(Region, China, tr("China")),
                              PAIR(Region, Korea, tr("Korea")),
                              PAIR(Region, Taiwan, tr("Taiwan")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::TimeZone>::Index(),
         {
             {static_cast<u32>(Settings::TimeZone::Auto),
              tr("Auto (%1)", "Auto select time zone")
                  .arg(QString::fromStdString(
                      Settings::GetTimeZoneString(Settings::TimeZone::Auto)))},
             {static_cast<u32>(Settings::TimeZone::Default),
              tr("Default (%1)", "Default time zone")
                  .arg(QString::fromStdString(Common::TimeZone::GetDefaultTimeZone()))},
             PAIR(TimeZone, Cet, tr("CET")),
             PAIR(TimeZone, Cst6Cdt, tr("CST6CDT")),
             PAIR(TimeZone, Cuba, tr("Cuba")),
             PAIR(TimeZone, Eet, tr("EET")),
             PAIR(TimeZone, Egypt, tr("Egypt")),
             PAIR(TimeZone, Eire, tr("Eire")),
             PAIR(TimeZone, Est, tr("EST")),
             PAIR(TimeZone, Est5Edt, tr("EST5EDT")),
             PAIR(TimeZone, Gb, tr("GB")),
             PAIR(TimeZone, GbEire, tr("GB-Eire")),
             PAIR(TimeZone, Gmt, tr("GMT")),
             PAIR(TimeZone, GmtPlusZero, tr("GMT+0")),
             PAIR(TimeZone, GmtMinusZero, tr("GMT-0")),
             PAIR(TimeZone, GmtZero, tr("GMT0")),
             PAIR(TimeZone, Greenwich, tr("Greenwich")),
             PAIR(TimeZone, Hongkong, tr("Hongkong")),
             PAIR(TimeZone, Hst, tr("HST")),
             PAIR(TimeZone, Iceland, tr("Iceland")),
             PAIR(TimeZone, Iran, tr("Iran")),
             PAIR(TimeZone, Israel, tr("Israel")),
             PAIR(TimeZone, Jamaica, tr("Jamaica")),
             PAIR(TimeZone, Japan, tr("Japan")),
             PAIR(TimeZone, Kwajalein, tr("Kwajalein")),
             PAIR(TimeZone, Libya, tr("Libya")),
             PAIR(TimeZone, Met, tr("MET")),
             PAIR(TimeZone, Mst, tr("MST")),
             PAIR(TimeZone, Mst7Mdt, tr("MST7MDT")),
             PAIR(TimeZone, Navajo, tr("Navajo")),
             PAIR(TimeZone, Nz, tr("NZ")),
             PAIR(TimeZone, NzChat, tr("NZ-CHAT")),
             PAIR(TimeZone, Poland, tr("Poland")),
             PAIR(TimeZone, Portugal, tr("Portugal")),
             PAIR(TimeZone, Prc, tr("PRC")),
             PAIR(TimeZone, Pst8Pdt, tr("PST8PDT")),
             PAIR(TimeZone, Roc, tr("ROC")),
             PAIR(TimeZone, Rok, tr("ROK")),
             PAIR(TimeZone, Singapore, tr("Singapore")),
             PAIR(TimeZone, Turkey, tr("Turkey")),
             PAIR(TimeZone, Uct, tr("UCT")),
             PAIR(TimeZone, Universal, tr("Universal")),
             PAIR(TimeZone, Utc, tr("UTC")),
             PAIR(TimeZone, WSu, tr("W-SU")),
             PAIR(TimeZone, Wet, tr("WET")),
             PAIR(TimeZone, Zulu, tr("Zulu")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::AudioMode>::Index(),
                          {
                              PAIR(AudioMode, Mono, tr("Mono")),
                              PAIR(AudioMode, Stereo, tr("Stereo")),
                              PAIR(AudioMode, Surround, tr("Surround")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::MemoryLayout>::Index(),
                          {
                              PAIR(MemoryLayout, Memory_4Gb, tr("4GB DRAM (Default)")),
                              PAIR(MemoryLayout, Memory_6Gb, tr("6GB DRAM (Unsafe)")),
                              PAIR(MemoryLayout, Memory_8Gb, tr("8GB DRAM")),
                              PAIR(MemoryLayout, Memory_10Gb, tr("10GB DRAM (Unsafe)")),
                              PAIR(MemoryLayout, Memory_12Gb, tr("12GB DRAM (Unsafe)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::ConsoleMode>::Index(),
                          {
                              PAIR(ConsoleMode, Docked, tr("Docked")),
                              PAIR(ConsoleMode, Handheld, tr("Handheld")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::CpuClock>::Index(),
                          {
                              PAIR(CpuClock, Normal, tr("Normal")),
                              PAIR(CpuClock, Boost, tr("Boost")),
                              PAIR(CpuClock, Overclock, tr("Overclock")),
                          }});
    translations->insert(
        {Settings::EnumMetadata<Settings::ConfirmStop>::Index(),
         {
             PAIR(ConfirmStop, Ask_Always, tr("Always ask (Default)")),
             PAIR(ConfirmStop, Ask_Based_On_Game, tr("Only if game specifies not to stop")),
             PAIR(ConfirmStop, Ask_Never, tr("Never ask")),
         }});
    translations->insert({Settings::EnumMetadata<Settings::GpuClock>::Index(),
                          {
                              PAIR(GpuClock, Normal, tr("Normal")),
                              PAIR(GpuClock, Boost, tr("Boost")),
                              PAIR(GpuClock, Overclock, tr("Overclock")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzleSize>::Index(),
                          {
                              PAIR(GpuUnswizzleSize, VerySmall, tr("Very Small (16 MB)")),
                              PAIR(GpuUnswizzleSize, Small, tr("Small (32 MB)")),
                              PAIR(GpuUnswizzleSize, Normal, tr("Normal (128 MB)")),
                              PAIR(GpuUnswizzleSize, Large, tr("Large (256 MB)")),
                              PAIR(GpuUnswizzleSize, VeryLarge, tr("Very Large (512 MB)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzle>::Index(),
                          {
                              PAIR(GpuUnswizzle, VeryLow, tr("Very Low (4 MB)")),
                              PAIR(GpuUnswizzle, Low, tr("Low (8 MB)")),
                              PAIR(GpuUnswizzle, Normal, tr("Normal (16 MB)")),
                              PAIR(GpuUnswizzle, Medium, tr("Medium (32 MB)")),
                              PAIR(GpuUnswizzle, High, tr("High (64 MB)")),
                          }});
    translations->insert({Settings::EnumMetadata<Settings::GpuUnswizzleChunk>::Index(),
                          {
                              PAIR(GpuUnswizzleChunk, VeryLow, tr("Very Low (32)")),
                              PAIR(GpuUnswizzleChunk, Low, tr("Low (64)")),
                              PAIR(GpuUnswizzleChunk, Normal, tr("Normal (128)")),
                              PAIR(GpuUnswizzleChunk, Medium, tr("Medium (256)")),
                              PAIR(GpuUnswizzleChunk, High, tr("High (512)")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::ExtendedDynamicState>::Index(),
                          {
                              PAIR(ExtendedDynamicState, Disabled, tr("Disabled")),
                              PAIR(ExtendedDynamicState, EDS1, tr("ExtendedDynamicState 1")),
                              PAIR(ExtendedDynamicState, EDS2, tr("ExtendedDynamicState 2")),
                              PAIR(ExtendedDynamicState, EDS3, tr("ExtendedDynamicState 3")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::GameListMode>::Index(),
                          {
                              PAIR(GameListMode, TreeView, tr("Tree View")),
                              PAIR(GameListMode, GridView, tr("Grid View")),
                              PAIR(GameListMode, CarouselView, tr("Carousel View")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::AudioEngine>::Index(),
                          {
                              PAIR(AudioEngine, Auto, tr("Auto")),
                              PAIR(AudioEngine, Cubeb, tr("Cubeb")),
                              PAIR(AudioEngine, Sdl3, tr("SDL3")),
                              PAIR(AudioEngine, Null, tr("Null")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::TemperatureUnits>::Index(),
                          {
                              PAIR(TemperatureUnits, Celsius, tr("Celsius (°C)")),
                              PAIR(TemperatureUnits, Fahrenheit, tr("Fahrenheit (°F)")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::GpuLogLevel>::Index(),
                          {
                              PAIR(GpuLogLevel, Off, tr("Off")),
                              PAIR(GpuLogLevel, Errors, tr("Errors Only")),
                              PAIR(GpuLogLevel, Standard, tr("Standard")),
                              PAIR(GpuLogLevel, Verbose, tr("Verbose")),
                              PAIR(GpuLogLevel, All, tr("All")),
                          }});

    translations->insert({Settings::EnumMetadata<Settings::SpeedMode>::Index(),
                          {
                              PAIR(SpeedMode, Standard, tr("Standard")),
                              PAIR(SpeedMode, Turbo, tr("Turbo")),
                              PAIR(SpeedMode, Slow, tr("Slow")),
                          }});

#undef PAIR
#undef CTX_PAIR

    return translations;
}
} // namespace ConfigurationShared
