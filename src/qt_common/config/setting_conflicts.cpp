// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#include "setting_conflicts.h"

#include "common/settings.h"
#include "common/settings_enums.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"

namespace ConfigurationShared {

std::vector<SettingConflict> SettingConflicts::DetectConflicts(bool is_per_game, u64 title_id) {
    std::vector<SettingConflict> conflicts;

    // Rule 1: Asynchronous shaders without disk shader cache
    if (Settings::values.use_asynchronous_shaders.GetValue() &&
        !Settings::values.use_disk_shader_cache.GetValue()) {
        conflicts.push_back({
            "async_shaders_no_disk_cache",
            StormLang11(
                QStringLiteral("Конфликт компиляции шейдеров"),
                QStringLiteral("Shader Compilation Conflict"),
                QStringLiteral("Shader-Kompilierungskonflikt"),
                QStringLiteral("Conflit de compilation des shaders"),
                QStringLiteral("Conflicto de compilación de sombreadores"),
                QStringLiteral("Conflitto di compilazione degli shader"),
                QStringLiteral("Conflito de compilação de sombreadores"),
                QStringLiteral("着色器编译冲突"),
                QStringLiteral("シェーダーコンパイルの競合"),
                QStringLiteral("셰이더 컴파일 충돌"),
                QStringLiteral("تعارض في تصريف التظليل"),
                QStringLiteral("शेडर संकलन टकराव")),
            StormLang11(
                QStringLiteral("Асинхронные шейдеры включены без дискового кэша: микрофризы и пропадание текстур будут повторяться при каждом запуске."),
                QStringLiteral("Asynchronous shaders are enabled without disk shader cache: stutter and pop-in will recur on every game launch."),
                QStringLiteral("Asynchrone Shader ohne Festplatten-Cache aktiviert: Ruckler und Pop-in treten bei jedem Start auf."),
                QStringLiteral("Shaders asynchrones activés sans cache disque : saccades et disparitions se répéteront à chaque lancement."),
                QStringLiteral("Sombreadores asíncronos activados sin caché en disco: los tirones y el pop-in se repetirán en cada inicio."),
                QStringLiteral("Shader asincroni abilitati senza cache su disco: stuttering e pop-in si ripeteranno ad ogni avvio."),
                QStringLiteral("Sombreadores assíncronos ativados sem cache em disco: travamentos e pop-in ocorrerão a cada inicialização."),
                QStringLiteral("在未启用磁盘着色器缓存的情况下启用了异步着色器：每次启动时都会出现卡顿和纹理加载延迟。"),
                QStringLiteral("ディスクシェーダーキャッシュなしで非同期シェーダーが有効です: 起動ごとにスタッターが発生します。"),
                QStringLiteral("디스크 셰이더 캐시 없이 비동기 셰이더가 활성화됨: 매 실행 시 끊김과 팝인이 반복됩니다."),
                QStringLiteral("تم تمكين التظليل غير المتزامن بدون ذاكرة التخزين المؤقت للقرص: سيتكرر التقطيع في كل تشغيل."),
                QStringLiteral("डिस्क शेडर कैश के बिना एसिंक्रोनस शेडर सक्षम हैं: हर गेम लॉन्च पर रुकावट दोहराई जाएगी।")),
            []() {
                Settings::values.use_disk_shader_cache.SetValue(true);
            }
        });
    }

    // Rule 2: Frame generation with unlocked FPS or VSync Immediate
    if (Settings::values.frame_gen.GetValue() &&
        (Settings::values.vsync_mode.GetValue() == Settings::VSyncMode::Immediate ||
         !Settings::values.use_speed_limit.GetValue())) {
        conflicts.push_back({
            "frame_gen_pacing_mismatch",
            StormLang11(
                QStringLiteral("Конфликт генерации кадров (LSFG)"),
                QStringLiteral("Frame Generation Pacing Conflict"),
                QStringLiteral("Bildratengenerierungs-Konflikt"),
                QStringLiteral("Conflit de génération d'images"),
                QStringLiteral("Conflicto de generación de fotogramas"),
                QStringLiteral("Conflitto generazione fotogrammi"),
                QStringLiteral("Conflito de geração de quadros"),
                QStringLiteral("帧生成同步冲突"),
                QStringLiteral("フレーム生成同期の競合"),
                QStringLiteral("프레임 생성 동기화 충돌"),
                QStringLiteral("تعارض توليد الإطارات"),
                QStringLiteral("फ्रेम जनरेशन पेसिंग टकराव")),
            StormLang11(
                QStringLiteral("Генерация кадров активна с выключенным VSync или снятым лимитом FPS: нарушается расчет векторов движения."),
                QStringLiteral("Frame generation is active with VSync off or unlocked FPS: optical flow vector estimation becomes erratic."),
                QStringLiteral("Frame-Generierung ist aktiv mit VSync AUS oder unbegrenzten FPS: Optical-Flow-Vektorberechnung wird ungenau."),
                QStringLiteral("Génération d'images active avec VSync désactivé ou FPS débloqué : le calcul des vecteurs optiques devient instable."),
                QStringLiteral("Generación de fotogramas activa con VSync desactivado o FPS desbloqueado: el cálculo de flujo óptico se vuelve errático."),
                QStringLiteral("Generazione fotogrammi attiva con VSync disattivato o FPS sbloccati: il calcolo dei vettori ottici diventa instabile."),
                QStringLiteral("Geração de quadros ativa com VSync desativado ou FPS desbloqueado: o cálculo de fluxo óptico se torna errático."),
                QStringLiteral("帧生成已启用但在 VSync 关闭或帧率未限制时：光流运动矢量估算将出现异常。"),
                QStringLiteral("VSync 無効または FPS 無制限でフレーム生成が有効です: オプティカルフロー計算が不安定になります。"),
                QStringLiteral("VSync가 꺼져 있거나 FPS 제한이 해제된 상태에서 프레임 생성이 활성화됨: 모션 벡터 계산이 불안정해집니다."),
                QStringLiteral("توليد الإطارات نشط مع إيقاف VSync أو معدل إطارات غير مقيد: يصبح تقدير التدفق البصري غير مستقر."),
                QStringLiteral("VSync बंद या अनलॉक किए गए FPS के साथ फ्रेम जनरेशन सक्रिय है: ऑप्टिकल फ्लो वेक्टर गणना त्रुटिपूर्ण हो जाती है।")),
            []() {
                Settings::values.vsync_mode.SetValue(Settings::VSyncMode::Fifo);
                Settings::values.use_speed_limit.SetValue(true);
            }
        });
    }

    // Rule 3: GPU accuracy Low without reactive flushing
    if (Settings::values.gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low &&
        !Settings::values.use_reactive_flushing.GetValue()) {
        conflicts.push_back({
            "gpu_accuracy_low_no_reactive_flushing",
            StormLang11(
                QStringLiteral("Риск сбоя синхронизации GPU"),
                QStringLiteral("GPU Synchronization Risk"),
                QStringLiteral("GPU-Synchronisationsrisiko"),
                QStringLiteral("Risque de synchronisation GPU"),
                QStringLiteral("Riesgo de sincronización GPU"),
                QStringLiteral("Rischio sincronizzazione GPU"),
                QStringLiteral("Risco de sincronização da GPU"),
                QStringLiteral("GPU 同步异常风险"),
                QStringLiteral("GPU 同期の危険性"),
                QStringLiteral("GPU 동기화 오류 위험"),
                QStringLiteral("خطر مزامنة وحدة معالجة الرسوم"),
                QStringLiteral("GPU सिंक्रोनाइज़ेशन जोखिम")),
            StormLang11(
                QStringLiteral("Низкая точность GPU без реактивного сброса кэша вызывает графические артефакты в играх с чтением из GPU."),
                QStringLiteral("Low GPU accuracy without reactive flushing causes graphical corruption in games requiring GPU readbacks."),
                QStringLiteral("Niedrige GPU-Genauigkeit ohne reaktives Flushing verursacht Grafikfehler bei Spielen mit GPU-Rücklesungen."),
                QStringLiteral("Précision GPU faible sans vidage réactif provoquant des corruptions graphiques sur les jeux avec relecture GPU."),
                QStringLiteral("Baja precisión de GPU sin vaciado reactivo provoca corrupción gráfica en juegos que leen de la GPU."),
                QStringLiteral("Bassa precisione GPU senza svuotamento reattivo causa artefatti nei giochi con rilettura GPU."),
                QStringLiteral("Baixa precisão da GPU sem liberação reativa causa corrupção gráfica em jogos que leem da GPU."),
                QStringLiteral("在未开启被动刷新时使用低 GPU 精度会导致需要 GPU 回读的游戏出现画面渲染损坏。"),
                QStringLiteral("リアクティブフラッシュなしの低 GPU 精度は、GPU リードバックを必要とするゲームでグラフィック異常を引き起こします。"),
                QStringLiteral("반응형 플러시 없는 낮은 GPU 정밀도는 GPU 판독이 필요한 게임에서 그래픽 깨짐을 유발합니다."),
                QStringLiteral("تؤدي دقة GPU المنخفضة دون تفريغ تفاعلي إلى تشوهات رسومية في الألعاب التي تتطلب قراءة من وحدة الرسوم."),
                QStringLiteral("रिएक्टिव फ्लशिंग के बिना कम GPU सटीकता GPU रीडबैक वाले गेम्स में ग्राफिकल विकृति का कारण बनती है।")),
            []() {
                Settings::values.use_reactive_flushing.SetValue(true);
            }
        });
    }

    // Rule 4: Unsafe CPU flags enabled when CPU accuracy is not Unsafe
    if (Settings::values.cpu_accuracy.GetValue() != Settings::CpuAccuracy::Unsafe) {
        const bool has_unsafe_flags =
            Settings::values.cpuopt_unsafe_unfuse_fma.GetValue() ||
            Settings::values.cpuopt_unsafe_reduce_fp_error.GetValue() ||
            Settings::values.cpuopt_unsafe_ignore_standard_fpcr.GetValue() ||
            Settings::values.cpuopt_unsafe_inaccurate_nan.GetValue() ||
            Settings::values.cpuopt_unsafe_fastmem_check.GetValue();

        if (has_unsafe_flags) {
            conflicts.push_back({
                "cpu_unsafe_flags_ignored",
                StormLang11(
                    QStringLiteral("Игнорирование флагов процессора"),
                    QStringLiteral("CPU Unsafe Flags Ignored"),
                    QStringLiteral("Unsichere CPU-Flags ignoriert"),
                    QStringLiteral("Drapeaux CPU non sécurisés ignorés"),
                    QStringLiteral("Opciones no seguras de CPU ignoradas"),
                    QStringLiteral("Flag CPU non sicuri ignorati"),
                    QStringLiteral("Sinalizadores inseguros de CPU ignorados"),
                    QStringLiteral("不安全 CPU 标记被忽略"),
                    QStringLiteral("非安全な CPU フラグが無視されています"),
                    QStringLiteral("안전하지 않은 CPU 플래그 무시됨"),
                    QStringLiteral("تم تجاهل علامات المعالج غير الآمنة"),
                    QStringLiteral("असुरक्षित CPU फ़्लैग अनदेखे किए गए")),
                StormLang11(
                    QStringLiteral("Флаги «Небезопасно» включены, но точность ЦП не Unsafe: движок Dynarmic игнорирует их в режиме Авто/Точно."),
                    QStringLiteral("Unsafe CPU flags are active but CPU accuracy is not Unsafe: Dynarmic ignores these flags in Auto/Accurate mode."),
                    QStringLiteral("Unsichere CPU-Flags sind aktiv, aber die CPU-Genauigkeit ist nicht 'Unsicher': Dynarmic ignoriert sie im Auto/Genau-Modus."),
                    QStringLiteral("Les options CPU non sécurisées sont actives mais la précision n'est pas 'Non sécurisée' : Dynarmic les ignore en mode Auto/Précis."),
                    QStringLiteral("Opciones no seguras de CPU activadas pero precisión no es 'No seguro': Dynarmic las ignora en modo Auto/Preciso."),
                    QStringLiteral("I flag CPU non sicuri sono attivi ma la precisione non è 'Non sicuro': Dynarmic li ignora in modalità Auto/Preciso."),
                    QStringLiteral("Sinalizadores de CPU inseguros ativos mas a precisão não é 'Inseguro': Dynarmic os ignora no modo Auto/Preciso."),
                    QStringLiteral("已勾选不安全 CPU 选项但 CPU 精度并非“不安全”：Dynarmic 引擎在自动/准确模式下将忽略这些标记。"),
                    QStringLiteral("非安全な CPU フラグが有効ですが精度が「非安全」ではありません: Dynarmic は自動/高精度モードでこれらを無視します。"),
                    QStringLiteral("안전하지 않은 CPU 플래그가 켜져 있지만 정확도가 '안전하지 않음'이 아닙니다: Dynarmic은 자동/정확 모드에서 이를 무시합니다."),
                    QStringLiteral("علامات المعالج غير الآمنة نشطة لكن الدقة ليست 'غير آمن': يتجاهلها محرك Dynarmic في الوضع التلقائي/الدقيق."),
                    QStringLiteral("असुरक्षित CPU फ़्लैग सक्रिय हैं लेकिन CPU सटीकता 'असुरक्षित' नहीं है: Dynarmic ऑटो/सटीक मोड में इन्हें अनदेखा करता है।")),
                []() {
                    Settings::values.cpu_accuracy.SetValue(Settings::CpuAccuracy::Unsafe);
                }
            });
        }
    }

    // Rule 5: Memory layout > 4GB on standard Switch 1 title
    if (Settings::values.memory_layout_mode.GetValue() != Settings::MemoryLayout::Memory_4Gb) {
        conflicts.push_back({
            "switch1_memory_layout_oversize",
            StormLang11(
                QStringLiteral("Предупреждение размера DRAM гостя"),
                QStringLiteral("Guest DRAM Size Warning"),
                QStringLiteral("Warnung zur Gast-DRAM-Größe"),
                QStringLiteral("Avertissement de taille DRAM invitée"),
                QStringLiteral("Advertencia de tamaño DRAM invitado"),
                QStringLiteral("Avviso dimensione DRAM ospite"),
                QStringLiteral("Aviso de tamanho de DRAM convidada"),
                QStringLiteral("客户机 DRAM 大小警告"),
                QStringLiteral("ゲスト DRAM サイズの警告"),
                QStringLiteral("게스트 DRAM 크기 경고"),
                QStringLiteral("تحذير حجم ذاكرة DRAM للضيف"),
                QStringLiteral("गेस्ट DRAM आकार चेतावनी")),
            StormLang11(
                QStringLiteral("Выделение памяти > 4 ГБ может вызывать падение игр Switch 1 при старте (используйте только с модами на память)."),
                QStringLiteral("Allocating >4GB guest RAM may crash standard Switch 1 games on launch (use only with memory-expanding mods)."),
                QStringLiteral("Zuweisung von >4 GB Gast-RAM kann Standard-Spiele der Switch 1 beim Start zum Absturz bringen."),
                QStringLiteral("Allouer >4 Go de RAM invitée peut faire planter les jeux Switch 1 standard au lancement."),
                QStringLiteral("Asignar >4GB de RAM puede bloquear juegos estándar de Switch 1 al iniciar."),
                QStringLiteral("L'allocazione di >4GB di RAM può causare il crash dei giochi standard di Switch 1 all'avvio."),
                QStringLiteral("Alocar >4GB de RAM convidada pode travar jogos padrão do Switch 1 na inicialização."),
                QStringLiteral("分配大于 4GB 的客户机内存可能会导致标准 Switch 1 游戏在启动时崩溃（请仅在装有大内存补丁时使用）。"),
                QStringLiteral("4GB を超えるゲスト RAM の割り当ては、メモリ拡張 MOD がない限り標準の Switch 1 ゲームをクラッシュさせる可能性があります。"),
                QStringLiteral("4GB 이상의 게스트 RAM 할당은 메모리 확장 모드가 없는 일반 Switch 1 게임을 실행 시 충돌시킬 수 있습니다."),
                QStringLiteral("قد يؤدي تخصيص أكثر من 4 جيجابايت من ذاكرة RAM إلى تعطل ألعاب Switch 1 القياسية عند التشغيل."),
                QStringLiteral(">4GB गेस्ट रैम आवंटित करने से लॉन्च पर मानक स्विच 1 गेम्स क्रैश हो सकते हैं।")),
            []() {
                Settings::values.memory_layout_mode.SetValue(Settings::MemoryLayout::Memory_4Gb);
            }
        });
    }

    // Rule 6: Extreme resolution scale (>= 3x)
    if (static_cast<int>(Settings::values.resolution_setup.GetValue()) >=
        static_cast<int>(Settings::ResolutionSetup::Res3X)) {
        conflicts.push_back({
            "extreme_resolution_scale",
            StormLang11(
                QStringLiteral("Высокая нагрузка по видеопамяти"),
                QStringLiteral("Extreme VRAM Workload"),
                QStringLiteral("Extreme VRAM-Auslastung"),
                QStringLiteral("Charge VRAM extrême"),
                QStringLiteral("Carga extrema de VRAM"),
                QStringLiteral("Carico VRAM estremo"),
                QStringLiteral("Carga extrema de VRAM"),
                QStringLiteral("极高显存负载"),
                QStringLiteral("極度の VRAM 負荷"),
                QStringLiteral("극심한 VRAM 부하"),
                QStringLiteral("حمولة ذاكرة VRAM شديدة"),
                QStringLiteral("अत्यधिक VRAM कार्यभार")),
            StormLang11(
                QStringLiteral("Масштаб разрешения 3x и выше требует от 8 ГБ VRAM и может приводить к сбросу видеодрайвера на слабых GPU."),
                QStringLiteral("Resolution scale 3x or higher requires >=8GB VRAM and may cause GPU driver resets on modest hardware."),
                QStringLiteral("Auflösungsskalierung 3x oder höher erfordert >=8 GB VRAM und kann zu Treiberabstürzen auf schwächeren GPUs führen."),
                QStringLiteral("Une échelle de résolution de 3x ou plus nécessite >=8 Go de VRAM et peut causer un plantage du pilote graphique."),
                QStringLiteral("Una escala de resolución de 3x o superior requiere >=8GB de VRAM y puede causar reinicios del controlador de GPU."),
                QStringLiteral("Una scala di risoluzione 3x o superiore richiede >=8GB di VRAM e può causare il ripristino dei driver GPU."),
                QStringLiteral("A escala de resolução 3x ou superior requer >=8GB de VRAM e pode causar reinicialização do driver da GPU."),
                QStringLiteral("3x 或更高的分辨率缩放需要至少 8GB 显存，在性能较弱的 GPU 上可能导致显卡驱动重置或崩溃。"),
                QStringLiteral("3x 以上の解像度スケールは 8GB 以上の VRAM を必要とし、低スペック GPU でドライバーのリセットを引き起こす可能性があります。"),
                QStringLiteral("3x 이상의 해상도 스케일링은 8GB 이상의 VRAM을 요구하며 보급형 GPU에서 드라이버 응답 없음을 유발할 수 있습니다."),
                QStringLiteral("يتطلب مقياس الدقة 3x أو أعلى 8 جيجابايت على الأقل من VRAM وقد يتسبب في إعادة ضبط برنامج تشغيل وحدة الرسوم."),
                QStringLiteral("3x या उच्च रिज़ॉल्यूशन स्केल के लिए >=8GB VRAM की आवश्यकता होती है और कमजोर GPU पर ड्राइवर रीसेट का कारण बन सकता है।")),
            []() {
                Settings::values.resolution_setup.SetValue(Settings::ResolutionSetup::Res2X);
            }
        });
    }

    return conflicts;
}

bool SettingConflicts::HasConflicts(bool is_per_game, u64 title_id) {
    return !DetectConflicts(is_per_game, title_id).empty();
}

} // namespace ConfigurationShared
