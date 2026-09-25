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
        const char* key;
        const char* ru;
        const char* ar;
        const char* de;
        const char* fr;
        const char* zh;
        const char* ja;
        const char* es;
    };

    static const LangItem s_table[] = {
        // Screenshot 2 & System settings
        {"CPU Clocks", "Частоты ЦП", "ترددات المعالج", "CPU-Takte", "Fréquences CPU", "CPU 时钟", "CPU クロック", "Frecuencias CPU"},
        {"GPU Clocks", "Частоты ГПУ", "ترددات معالج الرسوميات", "GPU-Takte", "Fréquences GPU", "GPU 时钟", "GPU クロック", "Frecuencias GPU"},
        {"Eco Thermal Mode", "Эко-термальный режим", "الوضع الحراري الاقتصادي", "Eco-Thermal-Modus", "Mode thermique éco", "环保节能模式", "エコ熱管理モード", "Modo térmico ecológico"},
        {"Low-End Turbo", "Турбо для слабых ПК", "وضع التوربو للأجهزة الضعيفة", "Low-End-Turbo", "Turbo pour machines modestes", "低端设备加速", "低スペック高速化", "Turbo para gama baja"},
        {"Thermal Governor", "Термоконтроллер", "متحكم الحرارة", "Thermal Governor", "Régulateur thermique", "温度调节器", "温度ガバナー", "Regulador térmico"},
        {"Limit Speed Percent", "Ограничение скорости в процентах", "نسبة تحديد السرعة", "Geschwindigkeitsbegrenzung in Prozent", "Pourcentage de limite de vitesse", "运行速度百分比限制", "速度制限パーセント", "Límite de velocidad en porcentaje"},
        {"Turbo Speed", "Турбо скорость", "سرعة التوربو", "Turbo-Geschwindigkeit", "Vitesse Turbo", "加速速度", "ターボ速度", "Velocidad turbo"},
        {"Slow Speed", "Замедленная скорость", "السرعة البطيئة", "Verlangsamte Geschwindigkeit", "Vitesse lente", "减速速度", "低速", "Velocidad lenta"},
        {"Synchronize Core Speed", "Синхронизировать скорость ядра", "مزامنة سرعة النواة", "Kerngeschwindigkeit synchronisieren", "Synchroniser la vitesse du cœur", "同步核心速度", "コア速度を同期", "Sincronizar velocidad del núcleo"},
        {"Multicore CPU Emulation", "Многоядерная эмуляция ЦП", "محاكاة المعالج متعدد النواة", "Mehrkern-CPU-Emulation", "Émulation CPU multicœur", "多核 CPU 模拟", "マルチコア CPU エミュレーション", "Emulación de CPU multinúcleo"},
        {"Memory Layout", "Схема памяти", "مخطط الذاكرة", "Speicher-Layout", "Disposition de la mémoire", "内存布局", "メモリレイアウト", "Distribución de memoria"},
        {"Custom RTC Date:", "Пользовательское время RTC:", "تاريخ RTC مخصص:", "Benutzerdefinierte RTC-Zeit:", "Date RTC personnalisée :", "自定义 RTC 时间:", "カスタム RTC 日時:", "Fecha RTC personalizada:"},
        {"Custom RTC Offset:", "Смещение RTC:", "إزاحة RTC:", "RTC-Offset:", "Décalage RTC :", "RTC 偏移量:", "RTC オフセット:", "Compensación RTC:"},
        {"Custom RTC Offset", "Смещение RTC", "إزاحة RTC", "RTC-Offset", "Décalage RTC", "RTC 偏移量", "RTC オフセット", "Compensación RTC"},
        {"Custom RTC", "Пользовательское время RTC", "تاريخ RTC مخصص", "Benutzerdefinierte RTC-Zeit", "Date RTC personnalisée", "自定义 RTC 时间", "カスタム RTC 日時", "Fecha RTC personalizada"},
        {"Custom RNG Seed", "Пользовательский сид RNG", "بذرة عشوائية مخصصة (RNG)", "Benutzerdefinierter RNG-Seed", "Graine RNG personnalisée", "自定义随机数种子", "カスタム RNG シード", "Semilla RNG personalizada"},

        // Screenshot 3 & CPU settings
        {"Accuracy:", "Точность:", "الدقة:", "Genauigkeit:", "Précision :", "精确度:", "精度:", "Precisión:"},
        {"Accurate", "Точный", "دقيق", "Präzise", "Précis", "高精度", "正確", "Preciso"},
        {"Auto", "Авто", "تلقائي", "Auto", "Auto", "自动", "自動", "Automático"},
        {"Unsafe", "Небезопасно", "غير آمن", "Unsicher", "Non sécurisé", "不安全", "非安全", "Inseguro"},
        {"Unsafe (fast)", "Небезопасно (быстро)", "غير آمن (سريع)", "Unsicher (schnell)", "Non sécurisé (rapide)", "不安全 (极速)", "非安全 (高速)", "Inseguro (rápido)"},
        {"Safe (stable)", "Безопасно (стабильно)", "آمن (مستقر)", "Sicher (stabil)", "Sécurisé (stable)", "安全 (稳定)", "安全 (安定)", "Seguro (estable)"},
        {"Paranoid (disables most optimizations)", "Параноидальный (отключает оптимизации)", "مفرط في الدقة (يعطل التحسينات)", "Paranoid (deaktiviert die meisten Optimierungen)", "Paranoïaque (désactive la plupart des optimisations)", "极致安全 (禁用大部分优化)", "パラノイド (大半の最適化を無効化)", "Paranoico (desactiva la mayoría de optimizaciones)"},
        {"Debugging", "Отладка", "تصحيح الأخطاء", "Debugging", "Débogage", "调试", "デバッグ", "Depuración"},
        {"Custom CPU Ticks", "Пользовательские такты ЦП", "ترددات مخصصة للمعالج", "Benutzerdefinierte CPU-Ticks", "Ticks CPU personnalisés", "自定义 CPU 时钟节拍", "カスタム CPU ティック", "Ticks de CPU personalizados"},
        {"CPU Affinity Pinning", "Привязка потоков ЦП", "تثبيت أنوية المعالج", "CPU-Affinity Pinning", "Affinité des cœurs CPU", "CPU 核心亲和性绑定", "CPU アフィニティ固定", "Fijación de afinidad de CPU"},

        // Screenshot 4 & Graphics settings
        {"API:", "Графический API:", "واجهة برمجة الرسوميات:", "Grafik-API:", "API graphique :", "图形 API:", "グラフィックス API:", "API de gráficos:"},
        {"Device:", "Устройство:", "الجهاز:", "Gerät:", "Appareil :", "设备:", "デバイス:", "Dispositivo:"},
        {"VSync Mode:", "Режим VSync:", "وضع VSync:", "VSync-Modus:", "Mode VSync :", "垂直同步模式:", "VSync モード:", "Modo VSync:"},
        {"Resolution:", "Разрешение:", "الدقة:", "Auflösung:", "Résolution :", "分辨率:", "解像度:", "Resolución:"},
        {"Window Adapting Filter:", "Фильтр масштабирования окна:", "مرشح ملاءمة النافذة:", "Fensteranpassungsfilter:", "Filtre d'adaptation de fenêtre :", "窗口自适应滤镜:", "ウィンドウ適応フィルター:", "Filtro de adaptación de ventana:"},
        {"FSR Sharpness:", "Резкость FSR:", "حدة FSR:", "FSR-Schärfe:", "Netteté FSR :", "FSR 锐度:", "FSR シャープネス:", "Nitidez FSR:"},
        {"Anti-Aliasing Method:", "Метод сглаживания:", "طريقة منع التعرج:", "Kantenglättungsmethode:", "Méthode anticrénelage :", "抗锯齿方法:", "アンチエイリアシング方式:", "Método de suavizado:"},
        {"Fullscreen Mode:", "Полноэкранный режим:", "وضع ملء الشاشة:", "Vollbildmodus:", "Mode plein écran :", "全屏模式:", "全画面モード:", "Modo de pantalla completa:"},
        {"Aspect Ratio:", "Соотношение сторон:", "نسبة الأبعاد:", "Seitenverhältnis:", "Format d'image :", "宽高比:", "アスペクト比:", "Relación de aspecto:"},
        {"Use persistent pipeline cache", "Использовать постоянный кэш конвейера", "استخدام ذاكرة خطوط الأنابيب الدائمة", "Dauerhaften Pipeline-Cache verwenden", "Utiliser le cache de pipeline persistant", "使用持久化管线缓存", "永続パイプラインキャッシュを使用", "Usar caché de tuberías persistente"},
        {"Use asynchronous GPU emulation", "Асинхронная эмуляция ГПУ", "محاكاة غير متزامنة لمعالج الرسوميات", "Asynchrone GPU-Emulation", "Émulation GPU asynchrone", "异步 GPU 模拟", "非同期 GPU エミュレーション", "Emulación asíncrona de GPU"},
        {"Eco Frame Pacing", "Эко-выравнивание кадров", "تنظيم الإطارات الاقتصادي", "Eco-Frame-Pacing", "Régulation d'images éco", "环保帧平滑", "エコフレームペーシング", "Compensación de fotogramas ecológica"},

        // Screenshot 5 & Advanced Graphics settings
        {"GPU Fence Behavior:", "Барьеры ГПУ:", "سلوك حواجز معالج الرسوميات:", "GPU-Fence-Verhalten:", "Comportement des barrières GPU :", "GPU 栅栏同步行为:", "GPU フェンス動作:", "Comportamiento de barreras GPU:"},
        {"VRAM Usage Mode:", "Использование видеопамяти:", "وضع استخدام ذاكرة الفيديو:", "VRAM-Nutzungsmodus:", "Mode d'utilisation VRAM :", "显存使用模式:", "VRAM 使用モード:", "Modo de uso de VRAM:"},
        {"NVDEC emulation:", "Эмуляция NVDEC:", "محاكاة NVDEC:", "NVDEC-Emulation:", "Émulation NVDEC :", "NVDEC 视频解码模拟:", "NVDEC エミュレーション:", "Emulación NVDEC:"},
        {"Anisotropic Filtering:", "Анизотропная фильтрация:", "التصفية متباينة الخواص:", "Anisotrope Filterung:", "Filtrage anisotrope :", "各向异性过滤:", "異方性フィルタリング:", "Filtrado anisotrópico:"},
        {"ASTC Decoding Method:", "Метод декодирования ASTC:", "طريقة فك تشفير ASTC:", "ASTC-Dekodierungsmethode:", "Méthode de décodage ASTC :", "ASTC 解码方法:", "ASTC デコード方式:", "Método de decodificación ASTC:"},
        {"Frame Pacing Mode (Vulkan only)", "Режим стабилизации кадров (только Vulkan)", "وضع ضبط توقيت الإطارات (Vulkan فقط)", "Frame-Pacing-Modus (nur Vulkan)", "Mode de calage d'images (Vulkan uniquement)", "帧同步模式 (仅限 Vulkan)", "フレームペーシングモード (Vulkan のみ)", "Modo de sincronización de fotogramas (solo Vulkan)"},
        {"ASTC Recompression Method:", "Метод пересжатия ASTC:", "طريقة إعادة ضغط ASTC:", "ASTC-Rekompression:", "Méthode de recompression ASTC :", "ASTC 重压缩方式:", "ASTC 再圧縮方式:", "Método de recompresión ASTC:"},
        {"Lock Dynamic Resolution", "Блокировка динамического разрешения", "قفل الدقة الديناميكية", "Dynamische Auflösung sperren", "Verrouiller la résolution dynamique", "锁定动态分辨率", "動的解像度ロック", "Bloquear resolución dinámica"},
        {"Sync Memory Operations", "Синхронизация операций памяти", "مزامنة عمليات الذاكرة", "Speicheroperationen synchronisieren", "Synchroniser les opérations de mémoire", "同步内存操作", "メモリオペレーションの同期", "Sincronizar operaciones de memoria"},
        {"Force Maximum Clocks (PC and Mobile)", "Принудительная максимальная частота", "فرض أقصى ترددات", "Maximale Taktraten erzwingen", "Forcer les fréquences maximales", "强制最高时钟频率", "最大クロックを強制", "Forzar frecuencias máximas"},
        {"Force maximum clocks (Vulkan only)", "Принудительная максимальная частота (только Vulkan)", "فرض أقصى ترددات (Vulkan فقط)", "Maximale Taktraten erzwingen (nur Vulkan)", "Forcer les fréquences maximales (Vulkan uniquement)", "强制最高时钟频率 (仅限 Vulkan)", "最大クロックを強制 (Vulkan のみ)", "Forzar frecuencias máximas (solo Vulkan)"},
        {"Early Release Fences", "Раннее освобождение барьеров", "تحرير المزامنة المبكر (Fences)", "Frühes Freigeben von Fences", "Libération anticipée des barrières", "提前释放同步栅栏 (Fences)", "フェンスの早期解放", "Liberación temprana de barreras"},
        {"Optimize SPIR-V Output", "Оптимизировать SPIR-V вывод", "تحسين مخرجات SPIR-V", "SPIR-V-Ausgabe optimieren", "Optimiser la sortie SPIR-V", "优化 SPIR-V 输出", "SPIR-V 出力を最適化", "Optimizar salida SPIR-V"},
        {"Fast GPU Time", "Быстрое время ГПУ", "توقيت GPU السريع", "Schnelle GPU-Zeit", "Temps GPU rapide", "快速 GPU 时间", "高速 GPU タイマー", "Tiempo rápido de GPU"},
        {"Enable Frame Skipping", "Включить пропуск кадров", "تفعيل تخطي الإطارات", "Frame-Skipping aktivieren", "Activer le saut d'images", "启用跳帧", "フレームスキップを有効化", "Habilitar salto de fotogramas"},
        {"Use Vulkan pipeline cache", "Кэш конвейеров Vulkan", "استخدام ذاكرة خطوط أنابيب Vulkan", "Vulkan-Pipeline-Cache verwenden", "Utiliser le cache de pipeline Vulkan", "使用 Vulkan 管线缓存", "Vulkan パイプラインキャッシュを使用", "Usar caché de tuberías de Vulkan"},
        {"Enable asynchronous shader compilation", "Включить асинхронную компиляцию шейдеров", "تجميع الشيدر غير المتزامن", "Asynchrone Shader-Kompilierung aktivieren", "Activer la compilation asynchrone des shaders", "启用异步着色器编译", "非同期シェーダーコンパイルを有効化", "Habilitar compilación asíncrona de shaders"},
        {"Sync to framerate of video playback", "Синхронизировать с частотой кадров видео", "المزامنة مع معدل إطارات تشغيل الفيديو", "Mit Video-Framerate synchronisieren", "Synchroniser avec le framerate de la vidéo", "与视频播放帧率同步", "動画再生のフレームレートに同期", "Sincronizar con tasa de fotogramas de video"},
        {"Enable Reactive Flushing", "Включить реактивный сброс", "تفعيل التفريغ التفاعلي", "Reaktives Flushing aktivieren", "Activer la vidange réactive", "启用响应式刷新 (Reactive Flushing)", "リアクティブフラッシュを有効化", "Habilitar vaciado reactivo"},
        {"Barrier feedback loops", "Циклы обратной связи барьеров", "حلقات التغذية الراجعة للحواجز", "Barrieren-Feedbackschleifen", "Boucles de rétroaction des barrières", "栅栏反馈循环", "バリアフィードバックループ", "Bucles de retroalimentación de barreras"},
        {"Enable buffer history", "Включить историю буфера", "تفعيل سجل التخزين المؤقت", "Pufferverlauf aktivieren", "Activer l'historique du tampon", "启用缓冲区历史记录", "バッファ履歴を有効化", "Habilitar historial de búfer"},
        {"Smart Shader Throttle", "Умный троттлинг шейдеров", "التحكم الذكي بمترجم الشيدر", "Smart Shader Throttle", "Régulation intelligente des shaders", "智能着色器节流", "スマートシェーダースロットル", "Regulador inteligente de shaders"},
        {"VRAM Garbage Collection", "Очистка видеопамяти", "تنظيف ذاكرة الفيديو (VRAM)", "VRAM-Speicherbereinigung", "Nettoyage de la mémoire VRAM", "显存垃圾回收", "VRAM ガベージコレクション", "Recolección de basura VRAM"},
        {"Enable HDR10", "Включить HDR10", "تفعيل HDR10", "HDR10 aktivieren", "Activer HDR10", "启用 HDR10", "HDR10 を有効化", "Habilitar HDR10"},
        {"VRAM Budget Governor", "Контроллер бюджета видеопамяти", "منظم ميزانية VRAM", "VRAM-Budget-Governor", "Régulateur de budget VRAM", "显存预算调度器", "VRAM 予算ガバナー", "Regulador de presupuesto VRAM"},
        {"Enable GPU buffer readback", "Обратное чтение буфера ГПУ", "تفعيل إعادة قراءة مخزن GPU", "GPU-Puffer-Rücklesung aktivieren", "Activer la relecture du tampon GPU", "启用 GPU 缓冲区回读", "GPU バッファのリードバックを有効化", "Habilitar relectura del búfer de GPU"},
        {"Floating Translate Button", "Плавающая кнопка перевода", "زر الترجمة العائم", "Schwebender Übersetzungs-Button", "Bouton de traduction flottant", "悬浮翻译按钮", "フローティング翻訳ボタン", "Botón de traducción flotante"},

        // Combobox enums
        {"Conservative", "Экономный", "اقتصادي", "Konservativ", "Économe", "保守", "控えめ", "Conservador"},
        {"Normal", "Нормальный", "عادي", "Normal", "Normal", "正常", "標準", "Normal"},
        {"Aggressive", "Агрессивный", "عدواني", "Aggressiv", "Agressif", "激进", "積極的", "Agresivo"},
        {"Default", "По умолчанию", "افتراضي", "Standard", "Par défaut", "默认", "デフォルト", "Por defecto"},
        {"Default (16:9)", "По умолчанию (16:9)", "افتراضي (16:9)", "Standard (16:9)", "Par défaut (16:9)", "默认 (16:9)", "デフォルト (16:9)", "Por defecto (16:9)"},
        {"Stretch to Window", "Растянуть до окна", "تمديد إلى النافذة", "Auf Fenstergröße strecken", "Étirer à la fenêtre", "拉伸至窗口", "ウィンドウに合わせる", "Estirar a la ventana"},
        {"Borderless Windowed", "Безрамочный режим", "نافذة بلا حدود", "Rahmenloses Fenster", "Fenêtré sans bordure", "无边框窗口", "ボーダーレスウィンドウ", "Ventana sin bordes"},
        {"Exclusive Fullscreen", "Эксклюзивный полноэкранный режим", "ملء الشاشة الحصري", "Exklusives Vollbild", "Plein écran exclusif", "独占全屏", "排他フルスクリーン", "Pantalla completa exclusiva"},
        {"None", "Нет", "لا شيء", "Keine", "Aucun", "无", "なし", "Ninguno"},
        {"Off", "Выкл", "إيقاف", "Aus", "Désactivé", "关闭", "オフ", "Desactivado"},
        {"Never", "Никогда", "أبداً", "Nie", "Jamais", "从不", "なし", "Nunca"},
        {"On Load", "При загрузке", "عند التحميل", "Beim Laden", "Au chargement", "加载时", "読み込み時", "Al cargar"},
        {"Always", "Всегда", "دائماً", "Immer", "Toujours", "总是", "常に", "Siempre"},
        {"GPU Video Decoding (Default)", "Декодирование видео на ГПУ (По умолчанию)", "فك تشفير الفيديو عبر GPU (افتراضي)", "GPU-Videodekodierung (Standard)", "Décodage vidéo GPU (par défaut)", "GPU 视频解码 (默认)", "GPU ビデオデコード (デフォルト)", "Decodificación de video por GPU (predeterminado)"},
        {"CPU Video Decoding", "Декодирование видео на ЦП", "فك تشفير الفيديو عبر CPU", "CPU-Videodekodierung", "Décodage vidéo CPU", "CPU 视频解码", "CPU ビデオデコード", "Decodificación de video por CPU"},
        {"No Video Output", "Без вывода видео", "بدون إخراج فيديو", "Keine Videoausgabe", "Aucune sortie vidéo", "无视频输出", "ビデオ出力なし", "Sin salida de video"},
        {"Uncompressed (Best quality)", "Без сжатия (наилучшее качество)", "غير مضغوط (أفضل جودة)", "Unkomprimiert (Beste Qualität)", "Non compressé (Meilleure qualité)", "未压缩 (最高质量)", "非圧縮 (最高品質)", "Sin comprimir (mejor calidad)"},
        {"BC1 (Low quality)", "BC1 (Низкое качество)", "BC1 (جودة منخفضة)", "BC1 (Geringe Qualität)", "BC1 (Basse qualité)", "BC1 (低质量)", "BC1 (低品質)", "BC1 (baja calidad)"},
        {"BC3 (Medium quality)", "BC3 (Среднее качество)", "BC3 (جودة متوسطة)", "BC3 (Mittlere Qualität)", "BC3 (Qualité moyenne)", "BC3 (中等质量)", "BC3 (中品質)", "BC3 (calidad media)"},
        {"BC5 (High quality)", "BC5 (Высокое качество)", "BC5 (جودة عالية)", "BC5 (Hohe Qualität)", "BC5 (Haute qualité)", "BC5 (高质量)", "BC5 (高品質)", "BC5 (alta calidad)"},
        {"Hybrid Video Decoding", "Гибридное декодирование видео", "فك تشفير الفيديو الهجين", "Hybride Videodekodierung", "Décodage vidéo hybride", "混合视频解码", "ハイブリッド動画デコード", "Decodificación de video híbrida"},
        {"Hybrid", "Гибридный", "هجين", "Hybrid", "Hybride", "混合", "ハイブリッド", "Híbrido"},
        {"Carousel View", "Карусель", "عرض دوار", "Karussellansicht", "Vue carrousel", "旋转木马视图", "カルーセル表示", "Vista de carrusel"},
        {"Fast", "Быстро", "سريع", "Schnell", "Rapide", "快速", "高速", "Rápido"},
        {"Strict", "Строго", "صارم", "Strikt", "Strict", "严格", "厳格", "Estricto"},
        {"Immediate", "Немедленно", "فوري", "Sofort", "Immédiat", "立即", "即時", "Inmediato"},
        {"Balanced", "Сбалансированно", "متوازن", "Ausgewogen", "Équilibré", "平衡", "バランス", "Equilibrado"},
        {"Automatic", "Автоматически", "تلقائي", "Automatisch", "Automatique", "自动", "自動", "Automático"},
        {"Custom frontend", "Пользовательский интерфейс", "واجهة أمامية مخصصة", "Benutzerdefiniertes Frontend", "Interface personnalisée", "自定义前端", "カスタムフロントエンド", "Frontend personalizado"},
        {"Real applet", "Настоящий апплет", "بريمج حقيقي", "Echtes Applet", "Véritable applet", "真实小程序", "実際のアプレット", "Applet real"},
        {"CPU Asynchronous", "ЦП (Асинхронно)", "المعالج (غير متزامن)", "CPU asynchron", "CPU asynchrone", "CPU 异步", "CPU 非同期", "CPU asíncrono"},
        {"Force 4:3", "Принудительно 4:3", "إجبار 4:3", "4:3 erzwingen", "Forcer 4:3", "强制 4:3", "強制 4:3", "Forzar 4:3"},
        {"Force 21:9", "Принудительно 21:9", "إجبار 21:9", "21:9 erzwingen", "Forcer 21:9", "强制 21:9", "強制 21:9", "Forzar 21:9"},
        {"Force 16:10", "Принудительно 16:10", "إجبار 16:10", "16:10 erzwingen", "Forcer 16:10", "强制 16:10", "強制 16:10", "Forzar 16:10"},
        {"Nearest Neighbor", "Ближайший сосед", "أقرب جار", "Nächster Nachbar", "Plus proche voisin", "最近邻", "最近傍", "Vecino más cercano"},
        {"Bilinear", "Билинейный", "ثنائي الخطي", "Bilinear", "Bilinéaire", "双线性", "バイリニア", "Bilineal"},
        {"Bicubic", "Бикубический", "ثنائي التكعيب", "Bikubisch", "Bicubique", "双三次", "バイキュービック", "Bicúbico"},
        {"Gaussian", "Гауссов", "جاوسي", "Gauß", "Gaussien", "高斯", "ガウス", "Gaussiano"},
        {"Lanczos", "Ланцош", "لانكزوس", "Lanczos", "Lanczos", "兰索斯", "ランチョス", "Lanczos"},
        {"Japanese (日本語)", "Японский (日本語)", "اليابانية (日本語)", "Japanisch (日本語)", "Japonais (日本語)", "日语 (日本語)", "日本語", "Japonés (日本語)"},
        {"American English", "Американский английский", "الإنجليزية الأمريكية", "Amerikanisches Englisch", "Anglais américain", "美式英语", "アメリカ英語", "Inglés estadounidense"},
        {"French (français)", "Французский (français)", "الفرنسية (français)", "Französisch (français)", "Français (français)", "法语 (français)", "フランス語 (français)", "Francés (français)"},
        {"German (Deutsch)", "Немецкий (Deutsch)", "الألمانية (Deutsch)", "Deutsch", "Allemand (Deutsch)", "德语 (Deutsch)", "ドイツ語 (Deutsch)", "Alemán (Deutsch)"},
        {"Italian (italiano)", "Итальянский (italiano)", "الإيطالية (italiano)", "Italienisch (italiano)", "Italien (italiano)", "意大利语 (italiano)", "イタリア語 (italiano)", "Italiano (italiano)"},
        {"Spanish (español)", "Испанский (español)", "الإسبانية (español)", "Spanisch (español)", "Espagnol (español)", "西班牙语 (español)", "スペイン語 (español)", "Español (español)"},
        {"Chinese", "Китайский", "الصينية", "Chinesisch", "Chinois", "中文", "中国語", "Chino"},
        {"Korean (한국어)", "Корейский (한국어)", "الكورية (한국어)", "Koreanisch (한국어)", "Coréen (한국어)", "韩语 (한국어)", "韓国語 (한국어)", "Coreano (한국어)"},
        {"Dutch (Nederlands)", "Нидерландский (Nederlands)", "الهولندية (Nederlands)", "Niederländisch (Nederlands)", "Néerlandais (Nederlands)", "荷兰语 (Nederlands)", "オランダ語 (Nederlands)", "Holandés (Nederlands)"},
        {"Portuguese (Português)", "Португальский (Português)", "البرتغالية (Português)", "Portugiesisch (Português)", "Portugais (Português)", "葡萄牙语 (Português)", "ポルトガル語 (Português)", "Portugués (Português)"},
        {"Russian (Русский)", "Русский (Русский)", "الروسية (Русский)", "Russisch (Русский)", "Russe (Русский)", "俄语 (Русский)", "ロシア語 (Русский)", "Ruso (Русский)"},
        {"Taiwanese", "Тайваньский", "التايوانية", "Taiwanisch", "Taïwanais", "繁体中文 (台湾)", "台湾語", "Taiwanés"},
        {"British English", "Британский английский", "الإنجليزية البريطانية", "Britisches Englisch", "Anglais britannique", "英式英语", "イギリス英語", "Inglés británico"},
        {"Simplified Chinese", "Упрощенный китайский", "الصينية المبسطة", "Vereinfachtes Chinesisch", "Chinois simplifié", "简体中文", "簡体字中国語", "Chino simplificado"},
        {"Traditional Chinese", "Традиционный китайский", "الصينية التقليدية", "Traditionelles Chinesisch", "Chinois traditionnel", "繁体中文", "繁体字中国語", "Chino tradicional"},
        {"Brazilian Portuguese", "Бразильский португальский", "البرتغالية البرازيلية", "Brasilianisches Portugiesisch", "Portugais brésilien", "巴西葡萄牙语", "ブラジルポルトガル語", "Portugués brasileño"},
        {"Latin American Spanish", "Латиноамериканский испанский", "الإسبانية الأمريكية اللاتينية", "Lateinamerikanisches Spanisch", "Espagnol d'Amérique latine", "拉美西班牙语", "ラテンアメリカスペイン語", "Español latinoamericano"},

        // Extensions and titles
        {"CPU", "ЦП", "معالج", "CPU", "CPU", "CPU", "CPU", "CPU"},
        {"GPU", "ГПУ", "معالج الرسوميات", "GPU", "GPU", "GPU", "GPU", "GPU"},
        {"FP16 Frame Generation", "Генерация кадров FP16", "توليد الإطارات بدقة FP16", "FP16-Frame-Generierung", "Génération d'images FP16", "FP16 帧生成", "FP16 フレーム生成", "Generación de fotogramas FP16"},
        {"Auto Optical Flow Scale", "Авто-масштаб оптического потока", "مقياس التدفق البصري التلقائي", "Automatische Skalierung des optischen Flusses", "Échelle de flux optique automatique", "自动光流缩放", "自動オプティカルフロースケール", "Escala de flujo óptico automático"},
        {"Emulate BGR565", "Эмуляция BGR565", "محاكاة BGR565", "BGR565 emulieren", "Émuler BGR565", "模拟 BGR565", "BGR565 をエミュレート", "Emular BGR565"},
        {"Extended Dynamic State", "Расширенное динамическое состояние", "الحالة الديناميكية الموسعة", "Extended Dynamic State", "Extended Dynamic State", "扩展动态状态", "拡張ダイナミックステート", "Estado dinámico extendido"},
        {"Vertex Input Dynamic State", "Динамическое состояние ввода вершин", "الحالة الديناميكية لإدخال الرؤوس", "Vertex Input Dynamic State", "Vertex Input Dynamic State", "顶点输入动态状态", "頂点入力ダイナミックステート", "Estado dinámico de entrada de vértices"},
        {"Sample Shading", "Множественное затенение", "تظليل العينات", "Sample-Shading", "Ombrage d'échantillon", "采样着色", "サンプルシェーディング", "Sombreado de muestras"},
        {"GPU Unswizzle", "Десвиззлинг ГПУ", "فك التفاف معالج الرسوميات", "GPU-Unswizzle", "GPU Unswizzle", "GPU 反交织", "GPU アンスウィズル", "Desentrelazado de GPU"},
        {"GPU Unswizzle Max Texture Size", "Максимальный размер текстур для десвиззлинга ГПУ", "أقصى حجم للقوام لفك التفاف GPU", "Maximale Texturgröße für GPU-Unswizzle", "Taille max de texture pour GPU Unswizzle", "GPU 反交织最大纹理尺寸", "GPU アンスウィズル最大テクスチャサイズ", "Tamaño máximo de textura para GPU Unswizzle"},
        {"GPU Unswizzle Stream Size", "Размер потока десвиззлинга ГПУ", "حجم تدفق فك التفاف GPU", "Stream-Größe für GPU-Unswizzle", "Taille du flux GPU Unswizzle", "GPU 反交织流大小", "GPU アンスウィズルストリームサイズ", "Tamaño de flujo de GPU Unswizzle"},
        {"GPU Unswizzle Chunk Size", "Размер блока десвиззлинга ГПУ", "حجم كتلة فك التفاف GPU", "Blockgröße für GPU-Unswizzle", "Taille de bloc GPU Unswizzle", "GPU 反交织块大小", "GPU アンスウィズルチャンクサイズ", "Tamaño de fragmento de GPU Unswizzle"},

        // Tooltips
        {"Enables Horizon's built-in overlay applet. Press and hold the home button for 1 second to show it.",
         "Включает встроенный оверлей Horizon. Нажмите и удерживайте кнопку Home в течение 1 секунды для отображения.",
         "تفعيل تراكب Horizon المدمج. اضغط مع الاستمرار على زر الصفحة الرئيسية لمدة ثانية واحدة لإظهاره.",
         "Aktiviert das integrierte Horizon-Overlay-Applet. Halten Sie die Home-Taste 1 Sekunde lang gedrückt, um es anzuzeigen.",
         "Active l'applet de superposition intégrée d'Horizon. Maintenez le bouton Home enfoncé pendant 1 seconde pour l'afficher.",
         "启用 Horizon 内置叠加小程序。按住 Home 键 1 秒即可显示。",
         "Horizon の内蔵オーバーレイアプレットを有効化します。Home ボタンを1秒長押しして表示します。",
         "Habilita la superposición integrada de Horizon. Mantenga presionado el botón de inicio durante 1 segundo para mostrarlo."},

        {"Synchronizes CPU core speed with the game's maximum rendering speed to boost FPS without affecting game speed (animations, physics, etc.).\nCan help reduce stuttering at lower framerates.",
         "Синхронизирует скорость ядер ЦП с максимальной скоростью рендеринга игры для повышения FPS без влияния на скорость игры (анимации, физика и т. д.).\nПомогает уменьшить рывки при низкой частоте кадров.",
         "مزامنة سرعة أنوية المعالج مع أقصى سرعة تصيير للعبة لتعزيز الإطارات دون التأثير على سرعة اللعبة.\nتساعد في تقليل التقطيع عند معدلات الإطارات المنخفضة.",
         "Synchronisiert die CPU-Kerngeschwindigkeit mit der maximalen Rendergeschwindigkeit, um FPS ohne Beeinflussung der Spielgeschwindigkeit zu steigern.\nKann Ruckler bei niedrigen Bildraten reduzieren.",
         "Synchronise la vitesse des cœurs CPU avec la vitesse maximale de rendu du jeu pour améliorer les FPS sans affecter la vitesse du jeu.\nPeut aider à réduire les saccades à faible fréquence d'images.",
         "将 CPU 核心速度与游戏最大渲染速度同步，在不影响游戏内部运行速度的前提下提升帧率。\n有助于减少低帧率时的微卡顿。",
         "ゲームの最大描画速度に CPU コア速度を同期させ、ゲーム自体の速度に影響を与えずに FPS を向上させます。\n低フレームレート時のカクつきを抑えます。",
         "Sincroniza la velocidad de los núcleos de CPU con la velocidad máxima de renderizado del juego para mejorar los FPS sin afectar la velocidad del juego.\nAyuda a reducir tirones a bajas tasas de fotogramas."},

        {"Raises the clock the emulated CPU reports, which removes some FPS limiters.\nWeaker CPUs may see reduced performance, and certain games may behave improperly.",
         "Повышает тактовую частоту, которую сообщает эмулируемый ЦП, снимая некоторые ограничители FPS.\nНа слабых ЦП производительность может снизиться, а некоторые игры могут работать некорректно.",
         "يرفع تردد المعالج الذي يتم الإبلاغ عنه لمحاكاة أسرع، مما يزيل بعض محددات الإطارات.\nقد ينخفض الأداء على المعالجات الضعيفة وتتصرف بعض الألعاب بشكل غير صحيح.",
         "Erhöht den Takt, den die emulierte CPU meldet, was einige FPS-Begrenzungen aufhebt.\nSchwächere CPUs können Leistungseinbußen erleiden und manche Spiele reagieren fehlerhaft.",
         "Augmente la fréquence signalée par le processeur émulé, supprimant ainsi certains limiteurs de FPS.\nLes processeurs moins puissants peuvent voir leurs performances réduites.",
         "提高模拟 CPU 报告的时钟频率，以解除部分游戏的帧率限制。\n较弱的 CPU 可能会降低性能，某些游戏可能出现异常。",
         "エミュレートされた CPU が報告するクロックを引き上げ、一部の FPS 制限を解除します。\n低性能な CPU ではパフォーマンスが低下する場合があり、一部のゲームで不具合が生じる可能性があります。",
         "Aumenta la frecuencia de reloj que informa la CPU emulada, lo que elimina algunos limitadores de FPS.\nLas CPUs menos potentes pueden ver reducido su rendimiento."},

        {"Specifies how videos should be decoded.\nIt can either use the CPU or the GPU for decoding, or perform no decoding at all (black screen on videos).\nIn most cases, GPU decoding provides the best performance.",
         "Определяет способ декодирования видео.\nМожет использовать ЦП или ГПУ для декодирования, либо отключить его полностью (черный экран в видеороликах).\nВ большинстве случаев декодирование на ГПУ обеспечивает наилучшую производительность.",
         "يحدد كيفية فك تشفير الفيديو.\nيمكن استخدام المعالج أو معالج الرسوميات، أو عدم فك التشفير إطلاقاً.\nفي معظم الحالات، يوفر فك التشفير عبر الرسوميات أفضل أداء.",
         "Gibt an, wie Videos dekodiert werden sollen.\nKann CPU oder GPU nutzen oder die Dekodierung deaktivieren.\nIn den meisten Fällen bietet die GPU-Dekodierung die beste Leistung.",
         "Indique le mode de décodage des vidéos.\nPeut utiliser le processeur ou le processeur graphique, ou désactiver le décodage.\nDans la plupart des cas, le décodage GPU offre les meilleures performances.",
         "指定视频解码方式。\n可选择使用 CPU 或 GPU 进行解码，或完全不进行解码 (视频黑屏)。\n大多数情况下，GPU 解码性能最佳。",
         "動画のデコード方法を指定します。\nCPU または GPU を使用するか、デコードを行わない（動画が黒画面になります）かを選択できます。\n通常は GPU デコードが最も高いパフォーマンスを発揮します。",
         "Especifica cómo deben decodificarse los videos.\nPuede usar la CPU o la GPU, o no realizar decodificación.\nEn la mayoría de los casos, la decodificación por GPU ofrece el mejor rendimiento."},

        {"This option controls how ASTC textures should be decoded.\nCPU: Use the CPU for decoding.\nGPU: Use the GPU's compute shaders to decode ASTC textures (recommended).\nCPU Asynchronously: Use the CPU to decode ASTC textures on demand. EliminatesASTC decoding\nstuttering but may present artifacts.",
         "Определяет метод декодирования текстур ASTC.\nЦП: декодирование силами ЦП.\nГПУ: декодирование вычислительными шейдерами ГПУ (рекомендуется).\nЦП (Асинхронно): декодирование на ЦП по требованию. Устраняет заикания, но возможны временные артефакты.",
         "يحدد هذا الخيار كيفية فك تشفير قوام ASTC.\nالمعالج: فك التشفير عبر المعالج.\nمعالج الرسوميات: استخدام شيدرات الحساب (موصى به).\nغير متزامن: فك التشفير حسب الطلب.",
         "Steuert, wie ASTC-Texturen dekodiert werden.\nCPU: Dekodierung durch CPU.\nGPU: Compute-Shader der GPU verwenden (empfohlen).\nCPU asynchron: Dekodierung bei Bedarf.",
         "Contrôle la méthode de décodage des textures ASTC.\nCPU : Décodage par le processeur.\nGPU : Shaders de calcul GPU (recommandé).\nCPU asynchrone : Décodage à la demande.",
         "控制 ASTC 纹理的解码方式。\nCPU: 使用 CPU 进行解码。\nGPU: 使用 GPU 计算着色器解码 (推荐)。\nCPU 异步: 按需在 CPU 上解码，消除卡顿但可能出现短暂画面瑕疵。",
         "ASTC テクスチャのデコード方法を制御します。\nCPU: CPU でデコードします。\nGPU: GPU コンピュートシェーダーを使用します (推奨)。\nCPU 非同期: 必要に応じてデコードし、スタッターを解消しますが一時的な乱れが生じる場合があります。",
         "Controla cómo se decodifican las texturas ASTC.\nCPU: Usa la CPU para decodificar.\nGPU: Usa shaders de cómputo de la GPU (recomendado).\nCPU asíncrona: Decodifica bajo demanda."},

        {"Makes the game believe GPU work finishes faster than it does, so it stops lowering resolution and render distance to fit the Switch's clocks.",
         "Заставляет игру считать, что задачи ГПУ выполняются быстрее, предотвращая снижение динамического разрешения и дальности прорисовки.",
         "يجعل اللعبة تعتقد أن عمل معالج الرسوميات ينتهي أسرع، مما يمنعها من خفض الدقة ومسافة الرؤية.",
         "Täuscht dem Spiel vor, dass die GPU-Arbeit schneller beendet ist, wodurch das Absenken von Auflösung und Sichtweite verhindert wird.",
         "Fait croire au jeu que le rendu GPU est plus rapide, évitant ainsi la baisse de résolution et de distance d'affichage.",
         "使游戏认为 GPU 工作比实际更快完成，从而防止其自动降低动态分辨率和渲染视距。",
         "GPU の処理が実際よりも速く完了したとゲームに認識させ、動的解像度や描画距離の低下を防ぎます。",
         "Hace que el juego crea que el trabajo de la GPU termina más rápido, evitando que baje la resolución dinámica y la distancia de dibujado."},

        {"Pins emulation threads to physical performance cores.",
         "Привязывает потоки эмуляции к физическим производительным ядрам ЦП.",
         "تثبيت خيوط المحاكاة على أنوية الأداء الفعلية.",
         "Pinnt Emulations-Threads an physische Leistungskerne.",
         "Assigne les threads d'émulation aux cœurs de performance physiques.",
         "将模拟线程绑定到物理性能核心 (P-Cores)。",
         "エミュレーションスレッドを物理的な高性能コアに固定します。",
         "Fija los hilos de emulación a núcleos de rendimiento físicos."},

        {"Reduces power draw and frame stuttering with adaptive delivery.",
         "Снижает энергопотребление и рывки кадров за счет адаптивной подачи.",
         "يقلل استهلاك الطاقة وتقطيع الإطارات عبر التقديم التكيفي.",
         "Reduziert Stromverbrauch und Frame-Ruckler durch adaptive Bereitstellung.",
         "Réduit la consommation d'énergie et les saccades grâce à une distribution adaptative des images.",
         "通过自适应帧递送降低能耗并消除画面微卡顿。",
         "適応型フレーム配信により消費電力を抑え、画面のカクつきを低減します。",
         "Reduce el consumo de energía y los tirones con entrega adaptativa de fotogramas."},

        {"Locks dynamic resolution scale to prevent resolution drops.",
         "Блокирует динамическое масштабирование разрешения, предотвращая падение четкости.",
         "قفل مقياس الدقة الديناميكي لمنع هبوط الجودة.",
         "Sperrt die dynamische Auflösungsskalierung, um ein Absinken der Schärfe zu verhindern.",
         "Verrouille l'échelle de résolution dynamique pour éviter les chutes de netteté.",
         "锁定动态分辨率缩放，防止画质分辨率下降。",
         "動的解像度スケーリングを固定し、解像度の低下を防止します。",
         "Bloquea la escala de resolución dinámica para evitar caídas de resolución."},

        {"Dynamically regulates priority of background shader compilation threads to prevent microstutters.",
         "Динамически регулирует приоритет фоновых потоков компиляции шейдеров для предотвращения микрофризов.",
         "تنظيم أولوية خيوط تجميع الشيدر في الخلفية ديناميكياً لمنع التقطيع البسيط.",
         "Regelt dynamisch die Priorität von Shader-Kompilierungs-Threads im Hintergrund zur Vermeidung von Mikrorucklern.",
         "Régule dynamiquement la priorité des threads de compilation de shaders en arrière-plan pour éviter les micro-saccades.",
         "动态调节后台着色器编译线程的优先级，消除微卡顿。",
         "バックグラウンドのシェーダーコンパイルスレッドの優先度を動的に調整し、マイクロスタッターを防ぎます。",
         "Regula dinámicamente la prioridad de los hilos de compilación de shaders para evitar microtirones."},

        {"Periodic background collection of unused texture buffers and ASTC cache to prevent VRAM leaks.",
         "Периодическая фоновая очистка неиспользуемых текстурных буферов и кэша ASTC для предотвращения утечек видеопамяти.",
         "تنظيف دوري في الخلفية لمخازن القوام غير المستخدمة وذاكرة ASTC لتجنب تسرب ذاكرة الرسوميات.",
         "Regelmäßige Hintergrundbereinigung ungenutzter Texturpuffer und des ASTC-Caches gegen VRAM-Lecks.",
         "Nettoyage périodique en arrière-plan des tampons de texture inutilisés et du cache ASTC pour éviter les fuites de VRAM.",
         "定期在后台回收未使用的纹理缓冲区和 ASTC 缓存，防止显存泄漏。",
         "未使用のテクスチャバッファと ASTC キャッシュをバックグラウンドで定期解放し、VRAM リークを防ぎます。",
         "Limpieza periódica en segundo plano de búferes de textura no utilizados y caché ASTC para evitar fugas de VRAM."},

        {"Prevents out-of-memory crashes by unloading textures when VRAM exceeds 85%.",
         "Предотвращает вылеты из-за нехватки памяти, выгружая неактивные текстуры при заполнении видеопамяти выше 85%.",
         "يمنع الانهيارات بسبب نفاد الذاكرة عن طريق تفريغ القوام عندما يتجاوز استخدام VRAM نسبة 85%.",
         "Verhindert Abstürze durch Speichermangel, indem Texturen entladen werden, wenn der VRAM zu über 85 % belegt ist.",
         "Empêche les plantages par manque de mémoire en déchargeant les textures lorsque l'utilisation de la VRAM dépasse 85 %.",
         "当显存占用超过 85% 时自动卸载非活动纹理，防止内存不足导致闪退崩溃。",
         "VRAM の使用率が85%を超えた際に非アクティブなテクスチャをアンロードし、メモリ不足によるクラッシュを防ぎます。",
         "Evita cierres por falta de memoria descargando texturas inactivas cuando el uso de VRAM supera el 85%."},

        {"Maximizes smoothness on entry-level multi-core processors and weak GPUs.",
         "Максимизирует плавность работы на начальных многоядерных процессорах и слабых ГПУ.",
         "زيادة السلاسة إلى أقصى حد على المعالجات متعددة الأنوية الاقتصادية ومعالجات الرسوميات الضعيفة.",
         "Maximiert die Flüssigkeit auf Einstiegs-Mehrkernprozessoren und schwachen GPUs.",
         "Maximise la fluidité sur les processeurs multicœurs d'entrée de gamme et les cartes graphiques modestes.",
         "在入门级多核处理器和弱 GPU 上最大化帧率平滑度。",
         "エントリー向けマルチコア CPU や低スペック GPU での滑らかさを最大化します。",
         "Maximiza la fluidez en procesadores multinúcleo básicos y GPUs de gama baja."},

        {"Reduces CPU and GPU temperatures by 10-15°C during frame wait intervals without dropping target frame rate.",
         "Снижает температуру ЦП и ГПУ на 10-15°C во время интервалов ожидания кадров без падения целевого фреймрейта.",
         "تقليل حرارة المعالج والرسوميات بمقدار 10-15 درجة أثناء فترات انتظار الإطارات دون انخفاض معدل الإطارات.",
         "Senkt CPU- und GPU-Temperaturen in Frame-Warteintervallen um 10–15 °C, ohne die Zielbildrate zu verringern.",
         "Réduit les températures CPU et GPU de 10 à 15 °C pendant les temps d'attente d'images sans baisser la fréquence cible.",
         "在帧等待间隙将 CPU 和 GPU 温度降低 10-15°C，且完全不降低目标帧率。",
         "フレーム待機中にターゲットフレームレートを維持したまま、CPU と GPU の温度を 10〜15℃ 低減させます。",
         "Reduce la temperatura de CPU y GPU en 10-15 °C durante los intervalos de espera de fotogramas sin perder fluidez."},

        {"Locks maximum performance for CPU and GPU, disables aggressive throttling and power saving.",
         "Фиксирует максимальную производительность для ЦП и ГПУ, отключает агрессивный троттлинг и энергосбережение.",
         "تثبيت أقصى أداء للمعالج والرسوميات وتعطيل وضع توفير الطاقة والحد من التردد.",
         "Sperrt die maximale Leistung für CPU und GPU und deaktiviert Drosselung sowie Energiesparmodi.",
         "Verrouille les performances maximales pour le CPU et le GPU, désactivant l'économie d'énergie agressive.",
         "锁定 CPU 和 GPU 的最高性能状态，禁用激进降频和节能限制。",
         "CPU と GPU を最高パフォーマンス状態に固定し、省電力やサーマルスロットリングを無効化します。",
         "Fija el rendimiento máximo para CPU y GPU, deshabilitando el ahorro de energía agresivo."},

        {"Enables HDR10 (BT.2020 PQ / ST2084) color space for compatible HDR and OLED displays.",
         "Включает цветовое пространство HDR10 (BT.2020 PQ / ST2084) для совместимых дисплеев HDR и OLED.",
         "تفعيل مساحة الألوان HDR10 للشاشات المتوافقة.",
         "Aktiviert den HDR10-Farbraum (BT.2020 PQ / ST2084) für kompatible HDR- und OLED-Bildschirme.",
         "Active l'espace colorimétrique HDR10 (BT.2020 PQ / ST2084) pour les écrans compatibles.",
         "为兼容的 HDR 和 OLED 显示器启用 HDR10 (BT.2020 PQ / ST2084) 色彩空间。",
         "対応する HDR および OLED ディスプレイ向けに HDR10 (BT.2020 PQ / ST2084) 色空間を有効化します。",
         "Habilita el espacio de color HDR10 (BT.2020 PQ / ST2084) para pantallas compatibles."},

        {"Uses 16-bit half precision floating point for frame generation.",
         "Использует 16-битные вычисления с плавающей запятой половинной точности для генерации кадров.",
         "استخدام عمليات الفاصلة العائمة بدقة نصفية 16 بت لتوليد الإطارات.",
         "Verwendet 16-Bit-Gleitkomma mit halber Genauigkeit für die Frame-Generierung.",
         "Utilise la virgule flottante en demi-précision 16 bits pour la génération d'images.",
         "使用 16 位半精度浮点计算进行帧生成加速。",
         "フレーム生成に 16 ビット半精度浮動小数点 (FP16) を使用して高速化します。",
         "Utiliza coma flotante de media precisión de 16 bits para la generación de fotogramas."},

        {"Automatically calculates optical flow grid scale for frame generation.",
         "Автоматически рассчитывает масштаб сетки оптического потока для генерации кадров.",
         "حساب مقياس شبكة التدفق البصري تلقائياً لتوليد الإطارات.",
         "Berechnet automatisch die Skalierung des optischen Flussrasters für die Frame-Generierung.",
         "Calcule automatiquement l'échelle de la grille de flux optique pour la génération d'images.",
         "自动计算适合当前分辨率的光流网格缩放比例以进行帧生成。",
         "フレーム生成用のオプティカルフローグリッドスケールを自動計算します。",
         "Calcula automáticamente la escala de cuadrícula de flujo óptico para la generación de fotogramas."},

        {"Emulates BGR565 color format by software swapping red and blue channels.",
         "Эмулирует цветовой формат BGR565 путем программной перестановки красного и синего каналов.",
         "محاكاة تنسيق الألوان BGR565 عن طريق تبديل القنوات الحمراء والزرقاء برمجياً.",
         "Emuliert das BGR565-Farbformat durch softwarebasiertes Vertauschen von Rot- und Blaukanal.",
         "Émule le format de couleur BGR565 par permutation logicielle des canaux rouge et bleu.",
         "通过软件置换红蓝通道来模拟 BGR565 颜色格式。",
         "赤と青のチャンネルをソフトウェアで入れ替えることで BGR565 カラーフォーマットを再現します。",
         "Emula el formato de color BGR565 intercambiando por software los canales rojo y azul."},

        {"Releases synchronization fences earlier to reduce frame presentation latency.",
         "Освобождает барьеры синхронизации раньше для снижения задержки вывода кадров.",
         "تحرير حواجز المزامنة مبكراً لتقليل زمن استجابة عرض الإطارات.",
         "Gibt Synchronisations-Fences früher frei, um die Latenz bei der Bildausgabe zu verringern.",
         "Libère les barrières de synchronisation plus tôt pour réduire la latence d'affichage des images.",
         "更早释放同步栅栏 (Fences)，以降低画面呈现的输入延迟。",
         "同期フェンスを早期に解放し、フレーム表示の遅延を低減します。",
         "Libera las barreras de sincronización antes para reducir la latencia de presentación de fotogramas."},

        {"Runs optimization passes on compiled SPIR-V shaders.",
         "Выполняет проходы оптимизации для скомпилированных шейдеров SPIR-V.",
         "تشغيل مراحل التحسين على شيدرات SPIR-V المجمعة.",
         "Führt Optimierungsdurchläufe auf kompilierten SPIR-V-Shadern aus.",
         "Exécute des passes d'optimisation sur les shaders SPIR-V compilés.",
         "对编译后的 SPIR-V 着色器执行优化通道。",
         "コンパイルされた SPIR-V シェーダーに対して最適化パスを実行します。",
         "Ejecuta pasadas de optimización en shaders SPIR-V compilados."},

        {"Uses fast monotonic GPU timer queries for frame pacing.",
         "Использует быстрые монотонные запросы таймера ГПУ для выравнивания кадров.",
         "استخدام استعلامات توقيت GPU رتيبة سريعة لضبط الإطارات.",
         "Verwendet schnelle monotone GPU-Timer-Abfragen für die Frame-Taktung.",
         "Utilise des requêtes de minuterie GPU monotones et rapides pour le calage des images.",
         "使用快速单调 GPU 计时器查询进行帧平滑。",
         "高速な単調増加 GPU タイマークエリを使用してフレームペーシングを行います。",
         "Utiliza consultas de temporizador GPU monótonas y rápidas para el ritmo de fotogramas."},

        {"Skips rendering non-critical frames when emulation falls behind target rate.",
         "Пропускает рендеринг некритичных кадров при отставании эмуляции от целевой скорости.",
         "تخطي تصيير الإطارات غير الحرجة عندما يتأخر المحاكي عن السرعة المطلوبة.",
         "Überspringt das Rendern unkritischer Frames, wenn die Emulation hinter die Zielrate zurückfällt.",
         "Ignore le rendu des images non critiques lorsque l'émulation prend du retard sur la cadence cible.",
         "当模拟进度落后于目标帧率时，跳过非关键帧的渲染。",
         "エミュレーションが目標速度に追いつかない場合、重要度の低いフレームの描画をスキップします。",
         "Omite el renderizado de fotogramas no críticos cuando la emulación se retrasa respecto a la velocidad objetivo."},

        {"Reduces power consumption and fan noise on portable devices.",
         "Снижает энергопотребление и шум вентилятора на портативных устройствах.",
         "تقليل استهلاك الطاقة وضوضاء المروحة على الأجهزة المحمولة.",
         "Reduziert Stromverbrauch und Lüftergeräusche auf tragbaren Geräten.",
         "Réduit la consommation d'énergie et le bruit du ventilateur sur les appareils portables.",
         "降低便携式设备的功耗与风扇噪音。",
         "ポータブルデバイスでの消費電力とファンの騒音を抑制します。",
         "Reduce el consumo de energía y el ruido del ventilador en dispositivos portátiles."},

        {"Displays floating quick translation overlay button.",
         "Отображает плавающую кнопку быстрого перевода.",
         "عرض زر تراكب الترجمة السريعة العائم.",
         "Zeigt einen schwebenden Schnellübersetzungs-Button an.",
         "Affiche un bouton flottant de traduction rapide.",
         "在界面上显示悬浮快速翻译按钮。",
         "フローティングクイック翻訳ボタンを表示します。",
         "Muestra un botón flotante de traducción rápida."},

        {"Dynamic performance scaler",
         "Динамическое масштабирование производительности",
         "مقياس الأداء الديناميكي",
         "Dynamische Leistungsskalierung",
         "Mise à l'échelle dynamique des performances",
         "动态性能缩放",
         "動的パフォーマンススケーラー",
         "Escalador dinámico de rendimiento"},

        {"Automatically adjusts rendering resolution to maintain the target frame rate. When frames take too long, resolution steps down; when frames are fast, it recovers. Inspired by Atmosphere's dynamic resolution system.",
         "Автоматически регулирует разрешение рендеринга для поддержания целевой частоты кадров. При падении производительности разрешение снижается, а при стабильном фреймрейте — восстанавливается.",
         "يضبط دقة العرض تلقائياً للحفاظ على معدل الإطارات المستهدف. عندما تستغرق الإطارات وقتاً طويلاً، تنخفض الدقة، وعندما تكون سريعة، تستعيد عافيتها.",
         "Passt die Renderauflösung automatisch an, um die Zielbildrate zu halten. Wenn Frames zu lange dauern, sinkt die Auflösung; wenn Frames schnell sind, wird sie wiederhergestellt.",
         "Ajuste automatiquement la résolution de rendu pour maintenir la fréquence d'images cible. Lorsque les images prennent trop de temps, la résolution diminue ; lorsque les images sont rapides, elle est rétablie.",
         "自动调整渲染分辨率以保持目标帧率。当帧生成时间过长时降低分辨率，当帧率充裕时恢复分辨率。",
         "ターゲットフレームレートを維持するために描画解像度を自動的に調整します。負荷が高い時は解像度を下げ、余裕がある時は復元します。",
         "Ajusta automáticamente la resolución de renderizado para mantener los FPS objetivo. Cuando los fotogramas tardan más, la resolución se reduce; cuando van rápido, se restablece."},

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
    INSERT(Settings, cabinet_applet_mode, tr("Amiibo editor"), QString());
    INSERT(Settings, controller_applet_mode, tr("Controller configuration"), QString());
    INSERT(Settings, data_erase_applet_mode, tr("Data erase"), QString());
    INSERT(Settings, error_applet_mode, tr("Error"), QString());
    INSERT(Settings, net_connect_applet_mode, tr("Net connect"), QString());
    INSERT(Settings, player_select_applet_mode, tr("Player select"), QString());
    INSERT(Settings, swkbd_applet_mode, tr("Software keyboard"), QString());
    INSERT(Settings, mii_edit_applet_mode, tr("Mii Edit"), QString());
    INSERT(Settings, web_applet_mode, tr("Online web"), QString());
    INSERT(Settings, shop_applet_mode, tr("Shop"), QString());
    INSERT(Settings, photo_viewer_applet_mode, tr("Photo viewer"), QString());
    INSERT(Settings, offline_web_applet_mode, tr("Offline web"), QString());
    INSERT(Settings, login_share_applet_mode, tr("Login share"), QString());
    INSERT(Settings, wifi_web_auth_applet_mode, tr("Wifi web auth"), QString());
    INSERT(Settings, my_page_applet_mode, tr("My page"), QString());
    INSERT(Settings, enable_overlay, tr("Enable Overlay Applet"),
           tr("Enables Horizon\'s built-in overlay applet. Press and hold the home button for 1 "
              "second to show it."));

    // Audio
    INSERT(Settings, sink_id, tr("Output Engine:"), QString());
    INSERT(Settings, audio_output_device_id, tr("Output Device:"), QString());
    INSERT(Settings, audio_input_device_id, tr("Input Device:"), QString());
    INSERT(Settings, audio_muted, tr("Mute audio"), QString());
    INSERT(Settings, volume, tr("Volume:"), QString());
    INSERT(Settings, dump_audio_commands, QString(), QString());
    INSERT(UISettings, mute_when_in_background, tr("Mute audio when in background"), QString());

    // Core
    INSERT(Settings, use_multi_core, tr("Multicore CPU Emulation"),
           tr("This option increases CPU emulation thread use from 1 to the maximum of 4.\n"
              "This is mainly a debug option and shouldn't be disabled."));
    INSERT(Settings, memory_layout_mode, tr("Memory Layout"),
           tr("Increases the amount of emulated RAM.\nDoesn't affect performance/stability but may "
              "allow HD texture "
              "mods to load."));
    INSERT(Settings, use_speed_limit, tr("Limit Speed Percent"), QString());
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

    // Cpu
    INSERT(Settings, cpu_accuracy, tr("Accuracy:"),
           tr("Change the accuracy of the emulated CPU (for debugging only)."));
    INSERT(Settings, cpu_backend, tr("Backend:"), QString());

    INSERT(Settings, cpu_clock, tr("CPU Clocks"),
           tr("Raises the clock the emulated CPU reports, which removes some FPS limiters.\n"
              "Weaker CPUs may see reduced performance, and certain games may behave improperly."));

    INSERT(Settings, use_custom_cpu_ticks, tr("Custom CPU Ticks"), QString());
    INSERT(Settings, cpu_ticks, tr("Custom CPU Ticks"),
           tr("Set a custom value of CPU ticks. Higher values can increase performance, but may "
              "cause deadlocks. A range of 77-21000 is recommended."));
    INSERT(Settings, cpu_backend, tr("Backend:"), QString());

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
    INSERT(Settings, scaling_filter, tr("Window Adapting Filter:"), QString());
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
    INSERT(Settings, rng_seed_enabled, tr("Custom RNG Seed"), QString());
    INSERT(Settings, device_name, tr("Device Name"), tr("The name of the console."));
    INSERT(Settings, program_args, tr("Homebrew Args"),
           tr("Command-line arguments passed to homebrew at launch (e.g. -noglsl)."));
    INSERT(Settings, custom_rtc, tr("Custom RTC Date:"),
           tr("This option allows to change the clock of the console.\n"
              "Can be used to manipulate time in games."));
    INSERT(Settings, custom_rtc_enabled, tr("Custom RTC Date:"), QString());
    INSERT(Settings, custom_rtc_offset, tr("Custom RTC Offset:"),
           tr("The number of seconds from the current unix time"));
    INSERT(Settings, language_index, tr("Language:"),
           tr("This option can be overridden when region setting is auto-select"));
    INSERT(Settings, region_index, tr("Region:"), tr("The region of the console."));
    INSERT(Settings, time_zone_index, tr("Time Zone:"), tr("The time zone of the console."));
    INSERT(Settings, sound_index, tr("Sound Output Mode:"), QString());
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
    INSERT(UISettings, enable_gamemode, tr("Enable Gamemode"), QString());
#ifdef __unix__
    INSERT(UISettings, gui_force_x11, tr("Force X11 as Graphics Backend"), QString());
    INSERT(UISettings, gui_hide_backend_warning, QString(), QString());
#endif

    // Ui Debugging

    // Ui Multiplayer

    // Ui Games list

    // STORM Optimization Settings
    INSERT(Settings, cpu_affinity_pinning, tr("CPU Affinity Pinning"),
           tr("Pins emulation threads to physical performance cores."));
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
                              PAIR(GpuAccuracy, High, tr("Accurate")),
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
    translations->insert({Settings::EnumMetadata<Settings::ScalingFilter>::Index(),
                          {
                              PAIR(ScalingFilter, NearestNeighbor, tr("Nearest Neighbor")),
                              PAIR(ScalingFilter, Bilinear, tr("Bilinear")),
                              PAIR(ScalingFilter, Bicubic, tr("Bicubic")),
                              PAIR(ScalingFilter, Gaussian, tr("Gaussian")),
                              PAIR(ScalingFilter, Lanczos, tr("Lanczos")),
                              PAIR(ScalingFilter, ScaleForce, tr("ScaleForce")),
                              PAIR(ScalingFilter, Fsr, tr("AMD FidelityFX Super Resolution")),
                              PAIR(ScalingFilter, Area, tr("Area")),
                              PAIR(ScalingFilter, Mmpx, tr("MMPX")),
                              PAIR(ScalingFilter, ZeroTangent, tr("Zero-Tangent")),
                              PAIR(ScalingFilter, BSpline, tr("B-Spline")),
                              PAIR(ScalingFilter, Mitchell, tr("Mitchell")),
                              PAIR(ScalingFilter, Spline1, tr("Spline-1")),
                              PAIR(ScalingFilter, Sgsr, tr("Snapdragon Game Super Resolution")),
                              PAIR(ScalingFilter, SgsrEdge, tr("Snapdragon Game Super Resolution EdgeDir")),
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

#undef PAIR
#undef CTX_PAIR

    return translations;
}
} // namespace ConfigurationShared
