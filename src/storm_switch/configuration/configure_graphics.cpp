// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <functional>
#include <vector>
#include <QBoxLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPixmap>
#include <QPushButton>
#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QKeyEvent>
#include <QListWidget>
#include <QScreen>
#include <QSettings>
#include <QSlider>
#include <QStringLiteral>
#include <QtCore/qobjectdefs.h>
#include <qabstractbutton.h>
#include <qboxlayout.h>
#include <qcombobox.h>
#include <qcoreevent.h>
#include <qglobal.h>
#include <qgridlayout.h>
#include <vulkan/vulkan_core.h>

#include "common/common_types.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "core/core.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
#include "ui_configure_graphics.h"
#include "storm_switch/configuration/configuration_shared.h"
#include "storm_switch/configuration/configure_graphics.h"
#include "storm_switch/configuration/retroarch_shader_catalog.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/configuration/shared_widget.h"


namespace {

[[maybe_unused]] bool IsStormRussianLang() {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    return lang.rfind("ru", 0) == 0;
}

class SearchableRetroArchCombo : public QComboBox {
public:
    explicit SearchableRetroArchCombo(QWidget* parent = nullptr) : QComboBox(parent) {
        setMinimumHeight(32);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void showPopup() override {
        hidePopup();
        const int cur_idx = std::max(0, currentIndex());
        StormShaders::ShowSearchableShaderPopup(this, cur_idx, [this](int chosen_idx) {
            if (chosen_idx >= 0 && chosen_idx < count()) {
                setCurrentIndex(chosen_idx);
                emit activated(chosen_idx);
            }
        });
    }
};

} // anonymous namespace

ConfigureGraphics::ConfigureGraphics(
    const Core::System& system_, std::vector<VkDeviceInfo::Record>& records_,
    const std::function<void()>& expose_compute_option_,
    const std::function<void(Settings::AspectRatio, Settings::ResolutionSetup)>&
        update_aspect_ratio_,
    std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group_,
    const ConfigurationShared::Builder& builder, QWidget* parent)
    : ConfigurationShared::Tab(group_, parent), ui{std::make_unique<Ui::ConfigureGraphics>()},
      records{records_}, expose_compute_option{expose_compute_option_},
      update_aspect_ratio{update_aspect_ratio_}, system{system_},
      combobox_translations{builder.ComboboxTranslations()} {
    vulkan_device = Settings::values.vulkan_device.GetValue();
    RetrieveVulkanDevices();

    ui->setupUi(this);

    Setup(builder);

    for (const auto& device : vulkan_devices) {
        vulkan_device_combobox->addItem(device);
    }

    UpdateBackgroundColorButton(QColor::fromRgb(Settings::values.bg_red.GetValue(),
                                                Settings::values.bg_green.GetValue(),
                                                Settings::values.bg_blue.GetValue()));
    UpdateAPILayout();
    PopulateVSyncModeSelection(false); //< must happen after UpdateAPILayout

    // VSync setting needs to be determined after populating the VSync combobox
    const auto vsync_mode_setting = Settings::values.vsync_mode.GetValue();
    const auto vsync_mode = VSyncSettingToMode(vsync_mode_setting);
    int index{};
    for (const auto mode : vsync_mode_combobox_enum_map) {
        if (mode == vsync_mode) {
            break;
        }
        index++;
    }
    if (static_cast<unsigned long>(index) < vsync_mode_combobox_enum_map.size()) {
        vsync_mode_combobox->setCurrentIndex(index);
    }

    connect(api_combobox, qOverload<int>(&QComboBox::activated), this, [this] {
        UpdateAPILayout();
        PopulateVSyncModeSelection(false);
        ConfigurationShared::NotifyGlobalSettingChanged();
    });
    connect(vulkan_device_combobox, qOverload<int>(&QComboBox::activated), this,
            [this](int device) {
                UpdateDeviceSelection(device);
                PopulateVSyncModeSelection(false);
            });
    connect(ui->bg_button, &QPushButton::clicked, this, [this] {
        const QColor new_bg_color = QColorDialog::getColor(bg_color);
        if (!new_bg_color.isValid()) {
            return;
        }
        UpdateBackgroundColorButton(new_bg_color);
    });

    const auto& update_screenshot_info = [this, &builder]() {
        const auto& combobox_enumerations = builder.ComboboxTranslations().at(
            Settings::EnumMetadata<Settings::AspectRatio>::Index());
        const auto ratio_index = aspect_ratio_combobox->currentIndex();
        const auto ratio =
            static_cast<Settings::AspectRatio>(combobox_enumerations[ratio_index].first);

        const auto& combobox_enumerations_resolution = builder.ComboboxTranslations().at(
            Settings::EnumMetadata<Settings::ResolutionSetup>::Index());
        const auto res_index = resolution_combobox->currentIndex();
        const auto setup = static_cast<Settings::ResolutionSetup>(
            combobox_enumerations_resolution[res_index].first);

        update_aspect_ratio(ratio, setup);
    };

    connect(aspect_ratio_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            update_screenshot_info);
    connect(resolution_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            update_screenshot_info);

    api_combobox->setEnabled(!UISettings::values.has_broken_vulkan && api_combobox->isEnabled());
    ui->api_widget->setEnabled(
        (!UISettings::values.has_broken_vulkan || Settings::IsConfiguringGlobal()) &&
        ui->api_widget->isEnabled());

    if (Settings::IsConfiguringGlobal()) {
        ui->bg_widget->setEnabled(Settings::values.bg_red.UsingGlobal());
    }
    RetranslateUI();
}

void ConfigureGraphics::PopulateVSyncModeSelection(bool use_setting) {
    if (!vsync_mode_combobox) {
        return;
    }
    const Settings::RendererBackend backend{GetCurrentGraphicsBackend()};
    if (backend == Settings::RendererBackend::Null) {
        vsync_mode_combobox->setEnabled(false);
        return;
    }
    vsync_mode_combobox->setEnabled(true);

    const int current_index = vsync_mode_combobox->currentIndex();
    const auto current_mode =
        current_index < 0 || static_cast<size_t>(current_index) >= vsync_mode_combobox_enum_map.size() || use_setting
            ? VSyncSettingToMode(Settings::values.vsync_mode.GetValue())
            : vsync_mode_combobox_enum_map[current_index];
    int index{};
    const int device{vulkan_device_combobox ? vulkan_device_combobox->currentIndex() : -1};

    const auto& present_modes =
        backend == Settings::RendererBackend::Vulkan && device > -1 && static_cast<size_t>(device) < device_present_modes.size()
            ? device_present_modes[device]
            : default_present_modes;

    const bool blocked = vsync_mode_combobox->blockSignals(true);
    vsync_mode_combobox->clear();
    vsync_mode_combobox_enum_map.clear();
    vsync_mode_combobox_enum_map.reserve(present_modes.size());
    for (const auto present_mode : present_modes) {
        const auto mode_name = TranslateVSyncMode(present_mode, backend);
        if (mode_name.isEmpty()) {
            continue;
        }

        vsync_mode_combobox->insertItem(index, mode_name);
        vsync_mode_combobox_enum_map.push_back(present_mode);
        if (present_mode == current_mode) {
            vsync_mode_combobox->setCurrentIndex(index);
        }
        index++;
    }
    vsync_mode_combobox->blockSignals(blocked);

    if (!Settings::IsConfiguringGlobal() && vsync_restore_global_button) {
        vsync_restore_global_button->setVisible(!Settings::values.vsync_mode.UsingGlobal());

        const Settings::VSyncMode global_vsync_mode = Settings::values.vsync_mode.GetValue(true);
        vsync_restore_global_button->setEnabled(
            ((backend == Settings::RendererBackend::OpenGL_GLSL ||
              backend == Settings::RendererBackend::OpenGL_GLASM ||
              backend == Settings::RendererBackend::OpenGL_SPIRV) &&
             (global_vsync_mode == Settings::VSyncMode::Immediate ||
              global_vsync_mode == Settings::VSyncMode::Fifo)) ||
            backend == Settings::RendererBackend::Vulkan);
    }
}

void ConfigureGraphics::UpdateVsyncSetting() const {
    const Settings::RendererBackend backend{GetCurrentGraphicsBackend()};
    if (backend == Settings::RendererBackend::Null || !vsync_mode_combobox) {
        return;
    }

    const int current_index = vsync_mode_combobox->currentIndex();
    if (current_index < 0 || static_cast<size_t>(current_index) >= vsync_mode_combobox_enum_map.size()) {
        return;
    }

    const auto mode = vsync_mode_combobox_enum_map[current_index];
    const auto vsync_mode = PresentModeToSetting(mode);
    if (Settings::IsConfiguringGlobal()) {
        Settings::values.vsync_mode.SetGlobal(true);
    }
    Settings::values.vsync_mode.SetValue(vsync_mode);
}

void ConfigureGraphics::UpdateDeviceSelection(int device) {
    if (device == -1) {
        return;
    }
    if (GetCurrentGraphicsBackend() == Settings::RendererBackend::Vulkan) {
        vulkan_device = device;
    }
}

ConfigureGraphics::~ConfigureGraphics() = default;

void ConfigureGraphics::SetConfiguration() {
    if (api_combobox) {
        const auto backend = Settings::values.renderer_backend.GetValue();
        const int index = FindIndex(Settings::EnumMetadata<Settings::RendererBackend>::Index(), static_cast<int>(backend));
        if (index >= 0 && index < api_combobox->count()) {
            const bool blocked = api_combobox->blockSignals(true);
            api_combobox->setCurrentIndex(index);
            api_combobox->blockSignals(blocked);
        }
    }
    UpdateAPILayout();
    PopulateVSyncModeSelection(true);
}

void ConfigureGraphics::Setup(const ConfigurationShared::Builder& builder) {
    QLayout* api_layout = ui->api_widget->layout();
    QWidget* api_grid_widget = new QWidget(this);
    QVBoxLayout* api_grid_layout = new QVBoxLayout(api_grid_widget);
    api_grid_layout->setContentsMargins(0, 0, 0, 0);
    api_layout->addWidget(api_grid_widget);

    // 1-Click Performance Presets Bar
    auto* presets_frame = new QFrame(api_grid_widget);
    presets_frame->setStyleSheet(QStringLiteral(
        "QFrame {"
        "    background: rgba(15, 23, 42, 0.7);"
        "    border: 1px solid rgba(0, 210, 255, 0.3);"
        "    border-radius: 8px;"
        "    margin-bottom: 6px;"
        "}"
    ));
    auto* presets_vbox = new QVBoxLayout(presets_frame);
    presets_vbox->setContentsMargins(10, 6, 10, 8);
    presets_vbox->setSpacing(6);

    auto* presets_title = new QLabel(StormLang(
        QStringLiteral("🚀 <b>Быстрые профили графики и производительности (в 1 клик):</b>"),
        QStringLiteral("🚀 <b>1-Click Graphics & Performance Presets:</b>"),
        QStringLiteral("🚀 <b>1-Klick-Grafik- & Leistungsprofile:</b>"),
        QStringLiteral("🚀 <b>Profils graphiques et performances en 1 clic :</b>"),
        QStringLiteral("🚀 <b>一键图形与性能预设配置：</b>"),
        QStringLiteral("🚀 <b>ワンクリック描画・パフォーマンスプリセット:</b>"),
        QStringLiteral("🚀 <b>ملفات تعريف الرسوميات والأداء بنقرة واحدة:</b>"),
        QStringLiteral("🚀 <b>Perfiles de gráficos y rendimiento en 1 clic:</b>")), presets_frame);
    presets_title->setObjectName(QStringLiteral("presetsTitleLabel"));
    presets_title->setStyleSheet(QStringLiteral("color: #00F0FF; font-size: 11.5px; border: none; background: transparent;"));
    presets_vbox->addWidget(presets_title);

    auto* presets_hbox = new QHBoxLayout();
    presets_hbox->setSpacing(8);

    const auto make_preset_btn = [&](const QString& obj_name, const QString& text, const QString& tip, auto apply_fn) {
        auto* btn = new QPushButton(text, presets_frame);
        btn->setObjectName(obj_name);
        btn->setToolTip(tip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1E293B, stop:1 #0F172A);"
            "    color: #F8FAFC;"
            "    font-weight: 600;"
            "    font-size: 11.5px;"
            "    padding: 5px 10px;"
            "    border-radius: 5px;"
            "    border: 1px solid rgba(0, 210, 255, 0.35);"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0284C7, stop:1 #0369A1);"
            "    color: #FFFFFF;"
            "    border: 1px solid #00F0FF;"
            "}"
        ));
        connect(btn, &QPushButton::clicked, this, [this, apply_fn]() {
            apply_fn();
            Settings::UpdateGPUAccuracy();
            Settings::UpdateRescalingInfo();
            SetConfiguration();
            ConfigurationShared::ReloadAllActiveWidgets();
            ConfigurationShared::NotifyGlobalSettingChanged();
        });
        presets_hbox->addWidget(btn);
    };

    const auto set_preset = [](auto& setting, auto val) {
        setting.SetGlobal(Settings::IsConfiguringGlobal());
        setting.SetValue(val);
    };

    make_preset_btn(
        QStringLiteral("presetBtnMaxFps"),
        StormLang(QStringLiteral("⚡ Макс. FPS"), QStringLiteral("⚡ Max FPS"),
                  QStringLiteral("⚡ Max. FPS"), QStringLiteral("⚡ FPS Max"),
                  QStringLiteral("⚡ 最高帧率"), QStringLiteral("⚡ 最大 FPS"),
                  QStringLiteral("⚡ أقصى إطارات"), QStringLiteral("⚡ Máx. FPS")),
        StormLang(QStringLiteral("Максимальная скорость: точность ГП «Нормальный», асинхронные шейдеры, быстрый таймер ГП"),
                  QStringLiteral("Maximum speed: Low GPU accuracy, async shaders, fast GPU time"),
                  QStringLiteral("Maximale Geschwindigkeit: Niedrige GPU-Genauigkeit, asynchrone Shader, schnelle GPU-Zeit"),
                  QStringLiteral("Vitesse maximale : Précision GPU basse, shaders asynchrones, temps GPU rapide"),
                  QStringLiteral("最高速度：低 GPU 精度、异步着色器、快速 GPU 时间"),
                  QStringLiteral("最高速度：GPU 精度（低）、非同期シェーダー、高速 GPU 時間"),
                  QStringLiteral("أقصى سرعة: دقة GPU منخفضة، تظليل غير متزامن، وقت GPU سريع"),
                  QStringLiteral("Velocidad máxima: precisión GPU baja, sombreadores asíncronos, tiempo GPU rápido")),
        [set_preset]() {
            set_preset(Settings::values.gpu_accuracy, Settings::GpuAccuracy::Low);
            set_preset(Settings::values.use_asynchronous_shaders, true);
            set_preset(Settings::values.async_presentation, true);
            set_preset(Settings::values.use_reactive_flushing, false);
            set_preset(Settings::values.use_fast_gpu_time, true);
            set_preset(Settings::values.vram_garbage_collection, true);
            set_preset(Settings::values.storm_lowend_turbo, false);
        });

    make_preset_btn(
        QStringLiteral("presetBtnBalanced"),
        StormLang(QStringLiteral("⚖️ Баланс (Рекомендуется)"), QStringLiteral("⚖️ Balanced (Recommended)"),
                  QStringLiteral("⚖️ Ausgewogen (Empfohlen)"), QStringLiteral("⚖️ Équilibré (Recommandé)"),
                  QStringLiteral("⚖️ 平衡（推荐）"), QStringLiteral("⚖️ バランス（推奨）"),
                  QStringLiteral("⚖️ متوازن (موصى به)"), QStringLiteral("⚖️ Equilibrado (Recomendado)")),
        StormLang(QStringLiteral("Идеальный баланс стабильности и FPS: точность ГП «Высокий», сжатие BC1, асинхронная презентация"),
                  QStringLiteral("Optimal stability & FPS balance: High GPU accuracy, BC1, async presentation"),
                  QStringLiteral("Optimale Balance aus Stabilität & FPS: Hohe GPU-Genauigkeit, BC1, asynchrone Präsentation"),
                  QStringLiteral("Équilibre optimal stabilité & FPS : Précision GPU élevée, BC1, présentation asynchrone"),
                  QStringLiteral("最佳稳定性与帧率平衡：高 GPU 精度、BC1 压缩、异步呈现"),
                  QStringLiteral("安定性と FPS の最適バランス：GPU 精度（高）、BC1、非同期プレゼンテーション"),
                  QStringLiteral("توازن مثالي بين الاستقرار والإطارات: دقة GPU عالية، BC1، عرض غير متزامن"),
                  QStringLiteral("Equilibrio óptimo entre estabilidad y FPS: precisión GPU alta, BC1, presentación asíncrona")),
        [set_preset]() {
            set_preset(Settings::values.gpu_accuracy, Settings::GpuAccuracy::High);
            set_preset(Settings::values.resolution_setup, Settings::ResolutionSetup::Res1X);
            set_preset(Settings::values.astc_recompression, Settings::AstcRecompression::Bc1);
            set_preset(Settings::values.use_asynchronous_shaders, true);
            set_preset(Settings::values.async_presentation, true);
            set_preset(Settings::values.use_reactive_flushing, false);
            set_preset(Settings::values.use_fast_gpu_time, true);
            set_preset(Settings::values.storm_lowend_turbo, false);
        });

    make_preset_btn(
        QStringLiteral("presetBtn4k"),
        StormLang(QStringLiteral("💎 4K Качество"), QStringLiteral("💎 4K Quality"),
                  QStringLiteral("💎 4K-Qualität"), QStringLiteral("💎 Qualité 4K"),
                  QStringLiteral("💎 4K 极致画质"), QStringLiteral("💎 4K 高画質"),
                  QStringLiteral("💎 جودة 4K"), QStringLiteral("💎 Calidad 4K")),
        StormLang(QStringLiteral("Максимальное качество: 2X (4K), фильтр FSR, сглаживание SMAA, анизотропия 16x, ASTC без сжатия"),
                  QStringLiteral("Ultra visual quality: 2X (4K), FSR scaling, SMAA anti-aliasing, 16x AF, Uncompressed ASTC"),
                  QStringLiteral("Maximale Bildqualität: 2X (4K), FSR-Skalierung, SMAA-Kantenglättung, 16x AF, unkomprimiertes ASTC"),
                  QStringLiteral("Qualité visuelle ultra : 2X (4K), mise à l'échelle FSR, anticrénelage SMAA, AF 16x, ASTC non compressé"),
                  QStringLiteral("极致视觉画质：2X (4K)、FSR 缩放、SMAA 抗锯齿、16x 各向异性过滤、未压缩 ASTC"),
                  QStringLiteral("最高画質：2X (4K)、FSR スケーリング、SMAA アンチエイリアス、16x 異方性フィルタリング、非圧縮 ASTC"),
                  QStringLiteral("جودة بصرية فائقة: 2X (4K)، تحجيم FSR، تنعيم SMAA، تصفية 16x، ASTC غير مضغوط"),
                  QStringLiteral("Calidad visual ultra: 2X (4K), escalado FSR, antialiasing SMAA, AF 16x, ASTC sin comprimir")),
        [set_preset]() {
            set_preset(Settings::values.gpu_accuracy, Settings::GpuAccuracy::High);
            set_preset(Settings::values.resolution_setup, Settings::ResolutionSetup::Res2X);
            set_preset(Settings::values.scaling_filter, Settings::ScalingFilter::Fsr);
            set_preset(Settings::values.anti_aliasing, Settings::AntiAliasing::Smaa);
            set_preset(Settings::values.max_anisotropy, Settings::AnisotropyMode::X16);
            set_preset(Settings::values.astc_recompression, Settings::AstcRecompression::Uncompressed);
            set_preset(Settings::values.use_reactive_flushing, true);
            set_preset(Settings::values.storm_lowend_turbo, false);
        });

    make_preset_btn(
        QStringLiteral("presetBtnEco"),
        StormLang(QStringLiteral("🔋 Эко / Портатив"), QStringLiteral("🔋 Eco / Handheld"),
                  QStringLiteral("🔋 Eco / Handheld"), QStringLiteral("🔋 Éco / Portable"),
                  QStringLiteral("🔋 节能 / 掌机"), QStringLiteral("🔋 エコ / 携帯機"),
                  QStringLiteral("🔋 اقتصادي / محمول"), QStringLiteral("🔋 Eco / Portátil")),
        StormLang(QStringLiteral("Для слабых ПК и портативных консолей: экономия видеопамяти, Турбо для слабых ПК и термоконтроль"),
                  QStringLiteral("For low-end PCs and handhelds: VRAM governor, Low-End Turbo, thermal control"),
                  QStringLiteral("Für schwache PCs und Handhelds: VRAM-Begrenzer, Low-End-Turbo, Temperaturregelung"),
                  QStringLiteral("Pour PC modestes et consoles portables : Économie VRAM, Turbo bas de gamme, contrôle thermique"),
                  QStringLiteral("适用于低配电脑与掌机：显存预算控制、低端设备加速与温度控制"),
                  QStringLiteral("低スペック PC・携帯機向け：VRAM 節約、低スペック高速化、温度管理"),
                  QStringLiteral("للأجهزة الضعيفة والمحمولة: توفير ذاكرة الفيديو، توربو للأجهزة الضعيفة، التحكم الحراري"),
                  QStringLiteral("Para PCs de gama baja y portátiles: ahorro de VRAM, Turbo de gama baja y control térmico")),
        [set_preset]() {
            set_preset(Settings::values.gpu_accuracy, Settings::GpuAccuracy::Low);
            set_preset(Settings::values.resolution_setup, Settings::ResolutionSetup::Res1X);
            set_preset(Settings::values.astc_recompression, Settings::AstcRecompression::Bc1);
            set_preset(Settings::values.eco_thermal_mode, true);
            set_preset(Settings::values.eco_frame_pacing, true);
            set_preset(Settings::values.smart_shader_throttle, true);
            set_preset(Settings::values.vram_budget_governor, true);
            set_preset(Settings::values.storm_lowend_turbo, true);
        });

    presets_vbox->addLayout(presets_hbox);
    api_grid_layout->addWidget(presets_frame);

    QLayout& graphics_layout = *ui->graphics_widget->layout();

    std::map<u32, QWidget*> hold_graphics;
    std::map<u32, QWidget*> hold_frame_gen;
    std::vector<QWidget*> hold_api;

    const auto is_frame_gen_setting = [](u32 id) {
        return id == Settings::values.frame_gen.Id() ||
               id == Settings::values.frame_gen_multiplier.Id() ||
               id == Settings::values.frame_gen_target_rate.Id() ||
               id == Settings::values.frame_gen_flow_scale_auto.Id() ||
               id == Settings::values.frame_gen_flow_scale.Id() ||
               id == Settings::values.frame_gen_queue_target.Id() ||
               id == Settings::values.frame_gen_fp16.Id() ||
               id == Settings::values.frame_gen_dump_flow.Id();
    };

    for (const auto setting : Settings::values.linkage.by_category[Settings::Category::Renderer]) {
        ConfigurationShared::Widget* widget = [&]() {
            if (setting->Id() == Settings::values.fsr_sharpening_slider.Id()) {
                // FSR needs a reversed slider and a 0.5 multiplier
                return builder.BuildWidget(
                    setting, apply_funcs, ConfigurationShared::RequestType::ReverseSlider, true,
                    0.5f, nullptr, tr("%", "FSR/SGSR sharpening percentage (e.g. 50%)"));
            } else {
                return builder.BuildWidget(setting, apply_funcs);
            }
        }();

        if (widget == nullptr) {
            continue;
        }
        if (!widget->Valid()) {
            widget->deleteLater();
            continue;
        }

        if (setting->Id() == Settings::values.renderer_backend.Id()) {
            // Add the renderer combobox now so it's at the top
            api_grid_layout->addWidget(widget);
            api_combobox = widget->combobox;
            api_restore_global_button = widget->restore_button;
            if (!Settings::IsConfiguringGlobal()) {
                api_restore_global_button->connect(api_restore_global_button,
                                                   &QAbstractButton::clicked,
                                                   [this](bool) { UpdateAPILayout(); });
                // Detach API's restore button and place it where we want
                // Lets us put it on the side, and it will automatically scale if there's a
                // second combobox (vulkan_device)
                widget->layout()->removeWidget(api_restore_global_button);
                api_layout->addWidget(api_restore_global_button);
            }
        } else if (setting->Id() == Settings::values.vulkan_device.Id()) {
            // Keep track of vulkan_device's combobox so we can populate it
            hold_api.push_back(widget);
            vulkan_device_combobox = widget->combobox;
            vulkan_device_widget = widget;
        } else if (setting->Id() == Settings::values.vsync_mode.Id()) {
            // Keep track of vsync_mode's combobox so we can populate it
            vsync_mode_combobox = widget->combobox;
            vsync_mode_combobox->connect(
                vsync_mode_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int) {
                    if (vsync_mode_combobox && vsync_mode_combobox->currentIndex() >= 0 &&
                        static_cast<size_t>(vsync_mode_combobox->currentIndex()) <
                            vsync_mode_combobox_enum_map.size()) {
                        if (Settings::IsConfiguringGlobal() && Settings::values.vsync_mode.UsingGlobal()) {
                            UpdateVsyncSetting();
                            ConfigurationShared::NotifyGlobalSettingChanged();
                        }
                    }
                });

            // Since vsync is populated at runtime, we have to manually set up the button for
            // restoring the global setting.
            if (!Settings::IsConfiguringGlobal()) {
                QPushButton* restore_button =
                    ConfigurationShared::Widget::CreateRestoreGlobalButton(
                        Settings::values.vsync_mode.UsingGlobal(), widget);
                restore_button->setEnabled(true);
                widget->layout()->addWidget(restore_button);

                restore_button->connect(restore_button, &QAbstractButton::clicked,
                                        [restore_button, this](bool) {
                                            Settings::values.vsync_mode.SetGlobal(true);
                                            PopulateVSyncModeSelection(true);

                                            restore_button->setVisible(false);
                                        });

                std::function<void()> set_non_global = [restore_button, this]() {
                    Settings::values.vsync_mode.SetGlobal(false);
                    UpdateVsyncSetting();
                    restore_button->setVisible(true);
                };
                widget->combobox->connect(widget->combobox,
                                          QOverload<int>::of(&QComboBox::activated),
                                          [set_non_global]() { set_non_global(); });
                vsync_restore_global_button = restore_button;
            }
            hold_graphics.emplace(setting->Id(), widget);
        } else if (setting->Id() == Settings::values.aspect_ratio.Id()) {
            // Keep track of the aspect ratio combobox to update other UI tabs that need it
            aspect_ratio_combobox = widget->combobox;
            hold_graphics.emplace(setting->Id(), widget);
        } else if (setting->Id() == Settings::values.resolution_setup.Id()) {
            // Keep track of the resolution combobox to update other UI tabs that need it
            resolution_combobox = widget->combobox;
            hold_graphics.emplace(setting->Id(), widget);
        } else if (is_frame_gen_setting(setting->Id())) {
            hold_frame_gen.emplace(setting->Id(), widget);
        } else {
            hold_graphics.emplace(setting->Id(), widget);
        }
    }

    for (const auto& [id, widget] : hold_graphics) {
        auto* w = qobject_cast<ConfigurationShared::Widget*>(widget);
        if (w && w->IsInputOrSelectionControl()) {
            graphics_layout.addWidget(widget);
        }
    }

    // Add RetroArch Shaders Selector Row with grouped categories & fast search at top of dropdown
    {
        auto* shader_row = new QWidget(ui->graphics_widget);
        shader_row->setObjectName(QStringLiteral("StormSettingRow"));
        shader_row->setMinimumHeight(38);
        shader_row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        auto* shader_row_layout = new QHBoxLayout(shader_row);
        shader_row_layout->setContentsMargins(2, 3, 4, 3);

        auto* shader_label = new QLabel(shader_row);
        shader_label->setObjectName(QStringLiteral("retroarchShaderLabel"));
        shader_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto* shader_combo = new SearchableRetroArchCombo(shader_row);
        shader_combo->setObjectName(QStringLiteral("retroarchShaderCombo"));

        for (const auto& entry : StormShaders::GetRetroArchShaderCatalog()) {
            shader_combo->addItem(
                StormShaders::GetLocalizedName(entry),
                QString::fromUtf8(entry.id));
        }

        int saved_idx = Settings::values.post_processing_shader.GetValue();
        if (saved_idx <= 0) {
            QSettings qs;
            const QString saved_shader = qs.value(QStringLiteral("StormSwitch/RetroArchShader"), QStringLiteral("none")).toString();
            saved_idx = StormShaders::ResolveShaderIndex(saved_shader);
        }
        if (saved_idx >= 0 && saved_idx < shader_combo->count()) {
            shader_combo->setCurrentIndex(saved_idx);
        }

        connect(shader_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [](int idx) {
            if (idx < 0) return;
            StormShaders::ApplyShaderByIndex(idx);
            ConfigurationShared::NotifyGlobalSettingChanged();
        });

        apply_funcs.push_back([shader_combo](bool) {
            if (shader_combo && shader_combo->currentIndex() >= 0) {
                StormShaders::ApplyShaderByIndex(shader_combo->currentIndex());
            }
        });

        shader_row_layout->addWidget(shader_label, 1);
        shader_row_layout->addWidget(shader_combo, 1);
        graphics_layout.addWidget(shader_row);
    }
    for (const auto& [id, widget] : hold_graphics) {
        auto* w = qobject_cast<ConfigurationShared::Widget*>(widget);
        if (!w || !w->IsInputOrSelectionControl()) {
            graphics_layout.addWidget(widget);
        }
    }

    graphics_layout.setSpacing(6);
    if (ui->verticalLayout_4) {
        ui->verticalLayout_4->setSpacing(8);
    }
    if (ui->verticalLayout_2) {
        ui->verticalLayout_2->setSpacing(10);
    }
    if (ui->bg_widget) {
        ui->bg_widget->setMinimumHeight(38);
    }
    if (ui->bg_button) {
        ui->bg_button->setMinimumHeight(32);
    }
    if (ui->scrollArea) {
        ui->scrollArea->setWidgetResizable(true);
        ui->scrollArea->setFrameShape(QFrame::NoFrame);
        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui->scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        ui->scrollArea->setMinimumSize(240, 180);
    }

    if (!hold_frame_gen.empty()) {
        auto* fg_group = new QGroupBox(ui->scrollAreaWidgetContents ? ui->scrollAreaWidgetContents : this);
        fg_group->setObjectName(QStringLiteral("frameGenGroupBox"));
        fg_group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        auto get_fg_title = []() {
            std::string lang = UISettings::values.language.GetValue();
            if (lang.empty()) {
                lang = QLocale::system().name().toStdString();
            }
            if (lang.rfind("ru", 0) == 0) return QStringLiteral("Генерация кадров (Lossless Scaling / LSFG)");
            if (lang.rfind("zh", 0) == 0) return QStringLiteral("帧生成 (Lossless Scaling / LSFG)");
            if (lang.rfind("ja", 0) == 0) return QStringLiteral("フレーム生成 (Lossless Scaling / LSFG)");
            if (lang.rfind("ko", 0) == 0) return QStringLiteral("프레임 생성 (Lossless Scaling / LSFG)");
            if (lang.rfind("de", 0) == 0) return QStringLiteral("Frame-Generierung (Lossless Scaling / LSFG)");
            if (lang.rfind("fr", 0) == 0) return QStringLiteral("Génération d'images (Lossless Scaling / LSFG)");
            if (lang.rfind("es", 0) == 0) return QStringLiteral("Generación de fotogramas (Lossless Scaling / LSFG)");
            if (lang.rfind("it", 0) == 0) return QStringLiteral("Generazione di fotogrammi (Lossless Scaling / LSFG)");
            if (lang.rfind("pt", 0) == 0) return QStringLiteral("Geração de quadros (Lossless Scaling / LSFG)");
            if (lang.rfind("ar", 0) == 0) return QStringLiteral("توليد الإطارات (Lossless Scaling / LSFG)");
            return QStringLiteral("Frame Generation (Lossless Scaling / LSFG)");
        };
        fg_group->setTitle(get_fg_title());
        auto* fg_layout = new QVBoxLayout(fg_group);
        fg_layout->setContentsMargins(10, 16, 10, 10);
        fg_layout->setSpacing(6);

        const std::vector<u32> fg_order = {
            Settings::values.frame_gen.Id(),
            Settings::values.frame_gen_multiplier.Id(),
            Settings::values.frame_gen_target_rate.Id(),
            Settings::values.frame_gen_queue_target.Id(),
            Settings::values.frame_gen_flow_scale_auto.Id(),
            Settings::values.frame_gen_flow_scale.Id(),
            Settings::values.frame_gen_fp16.Id(),
            Settings::values.frame_gen_dump_flow.Id(),
        };
        for (const u32 fg_id : fg_order) {
            auto it = hold_frame_gen.find(fg_id);
            if (it != hold_frame_gen.end()) {
                fg_layout->addWidget(it->second);
            }
        }
        for (const auto& [id, widget] : hold_frame_gen) {
            if (std::find(fg_order.begin(), fg_order.end(), id) == fg_order.end()) {
                fg_layout->addWidget(widget);
            }
        }
        if (ui->verticalLayout_2) {
            ui->verticalLayout_2->addWidget(fg_group);
        } else {
            graphics_layout.addWidget(fg_group);
        }
    }

    UpdateResolutionItems();

    for (auto widget : hold_api) {
        api_grid_layout->addWidget(widget);
    }

    // Background color is too specific to build into the new system, so we manage it here
    // (3 settings, all collected into a single widget with a QColor to manage on top)
    if (Settings::IsConfiguringGlobal()) {
        apply_funcs.push_back([this](bool powered_on) {
            Settings::values.bg_red.SetValue(static_cast<u8>(bg_color.red()));
            Settings::values.bg_green.SetValue(static_cast<u8>(bg_color.green()));
            Settings::values.bg_blue.SetValue(static_cast<u8>(bg_color.blue()));
        });
    } else {
        QPushButton* bg_restore_button = ConfigurationShared::Widget::CreateRestoreGlobalButton(
            Settings::values.bg_red.UsingGlobal(), ui->bg_widget);
        ui->bg_widget->layout()->addWidget(bg_restore_button);

        bg_restore_button->connect(bg_restore_button, &QAbstractButton::clicked,
                                   [bg_restore_button, this](bool) {
                                       const int r = Settings::values.bg_red.GetValue(true);
                                       const int g = Settings::values.bg_green.GetValue(true);
                                       const int b = Settings::values.bg_blue.GetValue(true);
                                       UpdateBackgroundColorButton(QColor::fromRgb(r, g, b));

                                       bg_restore_button->setVisible(false);
                                       bg_restore_button->setEnabled(false);
                                   });

        ui->bg_button->connect(ui->bg_button, &QAbstractButton::clicked, [bg_restore_button](bool) {
            bg_restore_button->setVisible(true);
            bg_restore_button->setEnabled(true);
        });

        apply_funcs.push_back([bg_restore_button, this](bool powered_on) {
            const bool using_global = !bg_restore_button->isEnabled();
            Settings::values.bg_red.SetGlobal(using_global);
            Settings::values.bg_green.SetGlobal(using_global);
            Settings::values.bg_blue.SetGlobal(using_global);
            if (!using_global) {
                Settings::values.bg_red.SetValue(static_cast<u8>(bg_color.red()));
                Settings::values.bg_green.SetValue(static_cast<u8>(bg_color.green()));
                Settings::values.bg_blue.SetValue(static_cast<u8>(bg_color.blue()));
            }
        });
    }
}

const QString ConfigureGraphics::TranslateVSyncMode(VkPresentModeKHR mode,
                                                    Settings::RendererBackend backend) const {
    const bool is_ogl = (backend == Settings::RendererBackend::OpenGL_GLSL ||
                         backend == Settings::RendererBackend::OpenGL_GLASM ||
                         backend == Settings::RendererBackend::OpenGL_SPIRV);
    switch (mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return is_ogl
                   ? StormLang(QStringLiteral("Выкл."), QStringLiteral("Off"),
                               QStringLiteral("Aus"), QStringLiteral("Désactivé"),
                               QStringLiteral("关闭"), QStringLiteral("オフ"),
                               QStringLiteral("إيقاف"), QStringLiteral("Desactivado"))
                   : StormLang(QStringLiteral("Immediate (VSync Выкл.)"), QStringLiteral("Immediate (VSync Off)"),
                               QStringLiteral("Immediate (VSync Aus)"), QStringLiteral("Immediate (VSync Désactivé)"),
                               QStringLiteral("Immediate (垂直同步关闭)"), QStringLiteral("Immediate (VSync オフ)"),
                               QStringLiteral("Immediate (إيقاف VSync)"), QStringLiteral("Immediate (VSync Desactivado)"));
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return StormLang(QStringLiteral("Mailbox (Рекомендуется)"), QStringLiteral("Mailbox (Recommended)"),
                         QStringLiteral("Mailbox (Empfohlen)"), QStringLiteral("Mailbox (Recommandé)"),
                         QStringLiteral("Mailbox (推荐)"), QStringLiteral("Mailbox (推奨)"),
                         QStringLiteral("Mailbox (موصى به)"), QStringLiteral("Mailbox (Recomendado)"));
    case VK_PRESENT_MODE_FIFO_KHR:
        return is_ogl
                   ? StormLang(QStringLiteral("Вкл."), QStringLiteral("On"),
                               QStringLiteral("Ein"), QStringLiteral("Activé"),
                               QStringLiteral("开启"), QStringLiteral("オン"),
                               QStringLiteral("تشغيل"), QStringLiteral("Activado"))
                   : StormLang(QStringLiteral("FIFO (VSync Вкл.)"), QStringLiteral("FIFO (VSync On)"),
                               QStringLiteral("FIFO (VSync Ein)"), QStringLiteral("FIFO (VSync Activé)"),
                               QStringLiteral("FIFO (垂直同步开启)"), QStringLiteral("FIFO (VSync オン)"),
                               QStringLiteral("FIFO (تشغيل VSync)"), QStringLiteral("FIFO (VSync Activado)"));
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return StormLang(QStringLiteral("FIFO Relaxed (Адаптивный VSync)"), QStringLiteral("FIFO Relaxed"),
                         QStringLiteral("FIFO Relaxed (Adaptives VSync)"), QStringLiteral("FIFO Relaxed (VSync adaptatif)"),
                         QStringLiteral("FIFO Relaxed (自适应垂直同步)"), QStringLiteral("FIFO Relaxed (適応型 VSync)"),
                         QStringLiteral("FIFO Relaxed (مزامنة متكيفة)"), QStringLiteral("FIFO Relaxed (VSync adaptativo)"));
    default:
        return {};
    }
}

int ConfigureGraphics::FindIndex(u32 enumeration, int value) const {
    const auto it = combobox_translations.find(enumeration);
    if (it != combobox_translations.end()) {
        for (u32 i = 0; i < it->second.size(); i++) {
            if (it->second[i].first == u32(value)) {
                return i;
            }
        }
    }
    return -1;
}

void ConfigureGraphics::ApplyConfiguration() {
    const bool powered_on = system.IsPoweredOn();
    for (const auto& func : apply_funcs) {
        func(powered_on);
    }

    UpdateVsyncSetting();

    Settings::values.vulkan_device.SetGlobal(true);
    auto const index = Settings::EnumMetadata<Settings::RendererBackend>::Index();
    if (Settings::IsConfiguringGlobal() ||
        (!Settings::IsConfiguringGlobal() && api_restore_global_button && api_restore_global_button->isEnabled())) {
        Settings::RendererBackend backend = Settings::values.renderer_backend.GetValue();
        const auto it = combobox_translations.find(index);
        if (it != combobox_translations.end() && api_combobox && api_combobox->currentIndex() >= 0 &&
            static_cast<size_t>(api_combobox->currentIndex()) < it->second.size()) {
            backend = Settings::RendererBackend(it->second[api_combobox->currentIndex()].first);
        }
        switch (backend) {
        case Settings::RendererBackend::Vulkan:
            Settings::values.vulkan_device.SetGlobal(Settings::IsConfiguringGlobal());
            if (vulkan_device_combobox && vulkan_device_combobox->currentIndex() >= 0) {
                Settings::values.vulkan_device.SetValue(vulkan_device_combobox->currentIndex());
            }
            break;
        case Settings::RendererBackend::OpenGL_GLSL:
        case Settings::RendererBackend::OpenGL_SPIRV:
        case Settings::RendererBackend::OpenGL_GLASM:
        case Settings::RendererBackend::Null:
            break;
        }
    }
}

void ConfigureGraphics::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureGraphics::RetranslateUI() {
    ui->retranslateUi(this);
    ui->groupBox_2->setTitle(StormLang(
        QStringLiteral("Настройки API"), QStringLiteral("API Settings"),
        QStringLiteral("API-Einstellungen"), QStringLiteral("Paramètres de l'API"),
        QStringLiteral("API 设置"), QStringLiteral("API 設定"),
        QStringLiteral("إعدادات API"), QStringLiteral("Configuración de API")));
    ui->groupBox->setTitle(StormLang(
        QStringLiteral("Настройки графики"), QStringLiteral("Graphics Settings"),
        QStringLiteral("Grafikeinstellungen"), QStringLiteral("Paramètres graphiques"),
        QStringLiteral("图形设置"), QStringLiteral("グラフィック設定"),
        QStringLiteral("إعدادات الرسوميات"), QStringLiteral("Configuración gráfica")));
    ui->label->setText(StormLang(
        QStringLiteral("Цвет фона:"), QStringLiteral("Background Color:"),
        QStringLiteral("Hintergrundfarbe:"), QStringLiteral("Couleur d'arrière-plan :"),
        QStringLiteral("背景颜色："), QStringLiteral("背景色:"),
        QStringLiteral("لون الخلفية:"), QStringLiteral("Color de fondo:")));
    const QString bg_tip = StormLang(
        QStringLiteral("Выбирает цвет фона за пределами игрового изображения (при чёрных полосах по краям экрана)."),
        QStringLiteral("Selects the background color shown behind the game image (letterbox/pillarbox borders)."),
        QStringLiteral("Wählt die Hintergrundfarbe hinter dem Spielbild (Letterbox-/Pillarbox-Ränder)."),
        QStringLiteral("Sélectionne la couleur d'arrière-plan affichée derrière l'image du jeu (bordures)."),
        QStringLiteral("选择游戏画面周围显示的背景颜色（黑边区域）。"),
        QStringLiteral("ゲーム画面の余白に表示される背景色を選択します。"),
        QStringLiteral("يحدد لون الخلفية المعروض خلف صورة اللعبة."),
        QStringLiteral("Selecciona el color de fondo mostrado detrás de la imagen del juego."));
    ui->bg_widget->setToolTip(bg_tip);
    ui->label->setToolTip(bg_tip);
    ui->bg_button->setToolTip(bg_tip);

    if (auto* presets_title = findChild<QLabel*>(QStringLiteral("presetsTitleLabel"))) {
        presets_title->setText(StormLang(
            QStringLiteral("🚀 <b>Быстрые профили графики и производительности (в 1 клик):</b>"),
            QStringLiteral("🚀 <b>1-Click Graphics & Performance Presets:</b>"),
            QStringLiteral("🚀 <b>1-Klick-Grafik- & Leistungsprofile:</b>"),
            QStringLiteral("🚀 <b>Profils graphiques et performances en 1 clic :</b>"),
            QStringLiteral("🚀 <b>一键图形与性能预设配置：</b>"),
            QStringLiteral("🚀 <b>ワンクリック描画・パフォーマンスプリセット:</b>"),
            QStringLiteral("🚀 <b>ملفات تعريف الرسوميات والأداء بنقرة واحدة:</b>"),
            QStringLiteral("🚀 <b>Perfiles de gráficos y rendimiento en 1 clic:</b>")));
    }
    if (auto* btn = findChild<QPushButton*>(QStringLiteral("presetBtnMaxFps"))) {
        btn->setText(StormLang(QStringLiteral("⚡ Макс. FPS"), QStringLiteral("⚡ Max FPS"),
                               QStringLiteral("⚡ Max. FPS"), QStringLiteral("⚡ FPS Max"),
                               QStringLiteral("⚡ 最高帧率"), QStringLiteral("⚡ 最大 FPS"),
                               QStringLiteral("⚡ أقصى إطارات"), QStringLiteral("⚡ Máx. FPS")));
        btn->setToolTip(StormLang(
            QStringLiteral("Максимальная скорость: точность ГП «Нормальный», асинхронные шейдеры, быстрый таймер ГП"),
            QStringLiteral("Maximum speed: Low GPU accuracy, async shaders, fast GPU time"),
            QStringLiteral("Maximale Geschwindigkeit: Niedrige GPU-Genauigkeit, asynchrone Shader, schnelle GPU-Zeit"),
            QStringLiteral("Vitesse maximale : Précision GPU basse, shaders asynchrones, temps GPU rapide"),
            QStringLiteral("最高速度：低 GPU 精度、异步着色器、快速 GPU 时间"),
            QStringLiteral("最高速度：GPU 精度（低）、非同期シェーダー、高速 GPU 時間"),
            QStringLiteral("أقصى سرعة: دقة GPU منخفضة، تظليل غير متزامن، وقت GPU سريع"),
            QStringLiteral("Velocidad máxima: precisión GPU baja, sombreadores asíncronos, tiempo GPU rápido")));
    }
    if (auto* btn = findChild<QPushButton*>(QStringLiteral("presetBtnBalanced"))) {
        btn->setText(StormLang(QStringLiteral("⚖️ Баланс (Рекомендуется)"), QStringLiteral("⚖️ Balanced (Recommended)"),
                               QStringLiteral("⚖️ Ausgewogen (Empfohlen)"), QStringLiteral("⚖️ Équilibré (Recommandé)"),
                               QStringLiteral("⚖️ 平衡（推荐）"), QStringLiteral("⚖️ バランス（推奨）"),
                               QStringLiteral("⚖️ متوازن (موصى به)"), QStringLiteral("⚖️ Equilibrado (Recomendado)")));
        btn->setToolTip(StormLang(
            QStringLiteral("Идеальный баланс стабильности и FPS: точность ГП «Высокий», сжатие BC1, асинхронная презентация"),
            QStringLiteral("Optimal stability & FPS balance: High GPU accuracy, BC1, async presentation"),
            QStringLiteral("Optimale Balance aus Stabilität & FPS: Hohe GPU-Genauigkeit, BC1, asynchrone Präsentation"),
            QStringLiteral("Équilibre optimal stabilité & FPS : Précision GPU élevée, BC1, présentation asynchrone"),
            QStringLiteral("最佳稳定性与帧率平衡：高 GPU 精度、BC1 压缩、异步呈现"),
            QStringLiteral("安定性と FPS の最適バランス：GPU 精度（高）、BC1、非同期プレゼンテーション"),
            QStringLiteral("توازن مثالي بين الاستقرار والإطارات: دقة GPU عالية، BC1، عرض غير متزامن"),
            QStringLiteral("Equilibrio óptimo entre estabilidad y FPS: precisión GPU alta, BC1, presentación asíncrona")));
    }
    if (auto* btn = findChild<QPushButton*>(QStringLiteral("presetBtn4k"))) {
        btn->setText(StormLang(QStringLiteral("💎 4K Качество"), QStringLiteral("💎 4K Quality"),
                               QStringLiteral("💎 4K-Qualität"), QStringLiteral("💎 Qualité 4K"),
                               QStringLiteral("💎 4K 极致画质"), QStringLiteral("💎 4K 高画質"),
                               QStringLiteral("💎 جودة 4K"), QStringLiteral("💎 Calidad 4K")));
        btn->setToolTip(StormLang(
            QStringLiteral("Максимальное качество: 2X (4K), фильтр FSR, сглаживание SMAA, анизотропия 16x, ASTC без сжатия"),
            QStringLiteral("Ultra visual quality: 2X (4K), FSR scaling, SMAA anti-aliasing, 16x AF, Uncompressed ASTC"),
            QStringLiteral("Maximale Bildqualität: 2X (4K), FSR-Skalierung, SMAA-Kantenglättung, 16x AF, unkomprimiertes ASTC"),
            QStringLiteral("Qualité visuelle ultra : 2X (4K), mise à l'échelle FSR, anticrénelage SMAA, AF 16x, ASTC non compressé"),
            QStringLiteral("极致视觉画质：2X (4K)、FSR 缩放、SMAA 抗锯齿、16x 各向异性过滤、未压缩 ASTC"),
            QStringLiteral("最高画質：2X (4K)、FSR スケーリング、SMAA アンチエイリアス、16x 異方性フィルタリング、非圧縮 ASTC"),
            QStringLiteral("جودة بصرية فائقة: 2X (4K)، تحجيم FSR، تنعيم SMAA، تصفية 16x، ASTC غير مضغوط"),
            QStringLiteral("Calidad visual ultra: 2X (4K), escalado FSR, antialiasing SMAA, AF 16x, ASTC sin comprimir")));
    }
    if (auto* btn = findChild<QPushButton*>(QStringLiteral("presetBtnEco"))) {
        btn->setText(StormLang(QStringLiteral("🔋 Эко / Портатив"), QStringLiteral("🔋 Eco / Handheld"),
                               QStringLiteral("🔋 Eco / Handheld"), QStringLiteral("🔋 Éco / Portable"),
                               QStringLiteral("🔋 节能 / 掌机"), QStringLiteral("🔋 エコ / 携帯機"),
                               QStringLiteral("🔋 اقتصادي / محمول"), QStringLiteral("🔋 Eco / Portátil")));
        btn->setToolTip(StormLang(
            QStringLiteral("Для слабых ПК и портативных консолей: экономия видеопамяти, Турбо для слабых ПК и термоконтроль"),
            QStringLiteral("For low-end PCs and handhelds: VRAM governor, Low-End Turbo, thermal control"),
            QStringLiteral("Für schwache PCs und Handhelds: VRAM-Begrenzer, Low-End-Turbo, Temperaturregelung"),
            QStringLiteral("Pour PC modestes et consoles portables : Économie VRAM, Turbo bas de gamme, contrôle thermique"),
            QStringLiteral("适用于低配电脑与掌机：显存预算控制、低端设备加速与温度控制"),
            QStringLiteral("低スペック PC・携帯機向け：VRAM 節約、低スペック高速化、温度管理"),
            QStringLiteral("للأجهزة الضعيفة والمحمولة: توفير ذاكرة الفيديو، توربو للأجهزة الضعيفة، التحكم الحراري"),
            QStringLiteral("Para PCs de gama baja y portátiles: ahorro de VRAM, Turbo de gama baja y control térmico")));
    }

    if (auto* fg_group = findChild<QGroupBox*>(QStringLiteral("frameGenGroupBox"))) {
        fg_group->setTitle(StormLang(
            QStringLiteral("Генерация кадров (Lossless Scaling / LSFG)"),
            QStringLiteral("Frame Generation (Lossless Scaling / LSFG)"),
            QStringLiteral("Frame-Generierung (Lossless Scaling / LSFG)"),
            QStringLiteral("Génération d'images (Lossless Scaling / LSFG)"),
            QStringLiteral("帧生成 (Lossless Scaling / LSFG)"),
            QStringLiteral("フレーム生成 (Lossless Scaling / LSFG)"),
            QStringLiteral("توليد الإطارات (Lossless Scaling / LSFG)"),
            QStringLiteral("Generación de fotogramas (Lossless Scaling / LSFG)")));
    }
    if (auto* sh_lbl = findChild<QLabel*>(QStringLiteral("retroarchShaderLabel"))) {
        sh_lbl->setText(StormLang(
            QStringLiteral("Шейдеры:"),
            QStringLiteral("Shaders:"),
            QStringLiteral("Shader:"),
            QStringLiteral("Shaders :"),
            QStringLiteral("着色器："),
            QStringLiteral("シェーダー:"),
            QStringLiteral("المظللات:"),
            QStringLiteral("Sombreadores:")));
        const QString sh_tip = StormLang(
            QStringLiteral("Выбор постобрабатывающего шейдера из полной коллекции RetroArch (ЭЛТ-мониторы CRT-Royale/Trinitron, сглаживание пиксель-арта xBRZ/ScaleFX/MMPX/HQ4x, нейросети Anime4K/NNEDI3, чёткость AMD CAS/LumaSharpen, матрицы портативных консолей LCD-Grid и киноэффекты VHS/HDR).\nВ выпадающем списке сверху доступен быстрый поиск по названию и категории."),
            QStringLiteral("Select a post-processing shader preset from the complete RetroArch collection (CRT-Royale/Trinitron, xBRZ/ScaleFX/MMPX/HQ4x pixel-art upscalers, Anime4K/NNEDI3, AMD CAS/LumaSharpen, handheld LCD grids, and VHS/HDR cinema effects).\nIncludes a fast search bar at the top of the dropdown list."),
            QStringLiteral("Wählt einen Post-Processing-Shader aus der kompletten RetroArch-Sammlung (CRT-Royale, xBRZ, ScaleFX, Anime4K, AMD CAS, LCD-Grid, VHS/HDR) mit Schnellsuche oben in der Liste."),
            QStringLiteral("Sélectionne un shader de post-traitement de la collection RetroArch (CRT-Royale, xBRZ, ScaleFX, Anime4K, AMD CAS, LCD-Grid, VHS/HDR) avec recherche rapide en haut de la liste."),
            QStringLiteral("从完整的 RetroArch 着色器集合中选择后处理预设（CRT-Royale、xBRZ、ScaleFX、Anime4K、AMD CAS、掌机 LCD 网格、VHS/HDR 电影特效），下拉列表顶部提供快速搜索。"),
            QStringLiteral("RetroArch の全シェーダーコレクション（CRT-Royale、xBRZ、ScaleFX、Anime4K、AMD CAS、携帯機 LCD グリッド、VHS/HDR 効果）から選択します。ドロップダウン上部に高速検索バーがあります。"),
            QStringLiteral("اختر مظلل معالجة لاحقة من مجموعة RetroArch الكاملة مع شريط بحث سريع أعلى القائمة المنسدلة."),
            QStringLiteral("Selecciona un sombreador de postprocesado de la colección completa de RetroArch con búsqueda rápida en la parte superior de la lista desplegable."));
        sh_lbl->setToolTip(sh_tip);
        if (auto* sh_combo = findChild<QComboBox*>(QStringLiteral("retroarchShaderCombo"))) {
            sh_combo->setToolTip(sh_tip);
            const auto& catalog = StormShaders::GetRetroArchShaderCatalog();
            for (int i = 0; i < sh_combo->count() && i < static_cast<int>(catalog.size()); ++i) {
                sh_combo->setItemText(i, StormShaders::GetLocalizedName(catalog[static_cast<std::size_t>(i)]));
            }
        }
    }
    PopulateVSyncModeSelection(false);
    UpdateResolutionItems();
}

void ConfigureGraphics::UpdateResolutionItems() {
    if (!resolution_combobox) {
        return;
    }
    const bool is_docked = Settings::values.use_docked_mode.GetValue() == Settings::ConsoleMode::Docked;
    const std::vector<QString> res_texts = {
        is_docked ? tr("0.25X (270p)") : tr("0.25X (180p)"),
        is_docked ? tr("0.5X (540p)") : tr("0.5X (360p)"),
        is_docked ? tr("0.75X (810p)") : tr("0.75X (540p)"),
        is_docked ? tr("1X (1080p)") : tr("1X (720p)"),
        is_docked ? tr("1.25X (1350p)") : tr("1.25X (900p)"),
        is_docked ? tr("1.5X (1620p)") : tr("1.5X (1080p)"),
        is_docked ? tr("2X (2160p / 4K)") : tr("2X (1440p / 2K)"),
        is_docked ? tr("3X (3240p / 6K)") : tr("3X (2160p / 4K)"),
        is_docked ? tr("4X (4320p / 8K)") : tr("4X (2880p)"),
        is_docked ? tr("5X (5400p)") : tr("5X (3600p)"),
        is_docked ? tr("6X (6480p)") : tr("6X (4320p)"),
        is_docked ? tr("7X (7560p)") : tr("7X (5040p)"),
        is_docked ? tr("8X (8640p)") : tr("8X (5760p)"),
    };
    for (int i = 0; i < resolution_combobox->count() && i < static_cast<int>(res_texts.size()); ++i) {
        resolution_combobox->setItemText(i, res_texts[i]);
    }
}

void ConfigureGraphics::UpdateBackgroundColorButton(QColor color) {
    bg_color = color;

    QPixmap pixmap(ui->bg_button->size());
    pixmap.fill(bg_color);

    const QIcon color_icon(pixmap);
    ui->bg_button->setIcon(color_icon);
}

void ConfigureGraphics::UpdateAPILayout() {
    bool runtime_lock = !system.IsPoweredOn();
    bool need_global = !(Settings::IsConfiguringGlobal() || api_restore_global_button->isEnabled());
    vulkan_device = Settings::values.vulkan_device.GetValue(need_global);
    vulkan_device_widget->setEnabled(!need_global && runtime_lock);

    const auto current_backend = GetCurrentGraphicsBackend();
    const bool is_vulkan = current_backend == Settings::RendererBackend::Vulkan;
    vulkan_device_widget->setVisible(is_vulkan);
    if (is_vulkan && int(vulkan_device) < vulkan_device_combobox->count()) {
        vulkan_device_combobox->setCurrentIndex(vulkan_device);
    }
}

void ConfigureGraphics::RetrieveVulkanDevices() {
    vulkan_devices.clear();
    vulkan_devices.reserve(records.size());
    device_present_modes.clear();
    device_present_modes.reserve(records.size());
    for (const auto& record : records) {
        vulkan_devices.push_back(QString::fromStdString(record.name));
        device_present_modes.push_back(record.vsync_support);

        if (record.has_broken_compute) {
            expose_compute_option();
        }
    }
}

Settings::RendererBackend ConfigureGraphics::GetCurrentGraphicsBackend() const {
    const auto selected_backend = [&]() {
        auto const index = Settings::EnumMetadata<Settings::RendererBackend>::Index();
        if (!Settings::IsConfiguringGlobal() && api_restore_global_button && !api_restore_global_button->isEnabled())
            return Settings::values.renderer_backend.GetValue(true);
        const auto it = combobox_translations.find(index);
        if (it != combobox_translations.end() && api_combobox && api_combobox->currentIndex() >= 0 &&
            static_cast<size_t>(api_combobox->currentIndex()) < it->second.size()) {
            return Settings::RendererBackend(it->second[api_combobox->currentIndex()].first);
        }
        return Settings::values.renderer_backend.GetValue();
    }();

    if (selected_backend == Settings::RendererBackend::Vulkan &&
        UISettings::values.has_broken_vulkan)
        return Settings::RendererBackend::OpenGL_GLSL;
    return selected_backend;
}
