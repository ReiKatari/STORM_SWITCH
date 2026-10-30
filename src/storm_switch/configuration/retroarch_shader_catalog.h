// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLocale>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "common/settings.h"
#include "common/settings_enums.h"
#include "qt_common/config/uisettings.h"
#include "storm_switch/common/storm_lang.h"

namespace StormShaders {

inline bool IsRussianLang() {
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    return lang.rfind("ru", 0) == 0;
}

struct RetroArchShaderEntry {
    const char* id;
    const char* short_name;
    const char* category_ru;
    const char* category_en;
    const char* name_ru;
    const char* name_en;
    Settings::ScalingFilter mapped_filter;
    int fsr_sharpness;
};

inline const std::vector<RetroArchShaderEntry>& GetRetroArchShaderCatalog() {
    static const std::vector<RetroArchShaderEntry> kCatalog = {
        // 0. Default / None
        {"none", "None",
         "🚫 0. Без шейдера (Стандартный рендер)",
         "🚫 0. No Shader (Standard Native Output)",
         "Без шейдера (Чистый рендер — По умолчанию)",
         "None (Clean Native Render — Default)",
         Settings::ScalingFilter::Bilinear, 80},

        // 1..15. CRT & Scanlines
        {"crt-royale", "CRT-Royale",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Royale (Апертурная решётка Sony Trinitron + галация фосфора)",
         "CRT-Royale (Sony Trinitron Aperture Grille + Phosphor Halation)",
         Settings::ScalingFilter::Bilinear, 80},
        {"crt-guest-advanced", "CRT-Guest",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Guest-Advanced (Эталонный мультисистемный ЭЛТ с теневой маской)",
         "CRT-Guest-Advanced (Reference Multi-System Shadow Mask CRT)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-geom", "CRT-Geom",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Geom (Выпуклый кинескоп + субпиксельный люминофор)",
         "CRT-Geom (Curved Glass Tube + Subpixel Phosphor)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-lottes", "CRT-Lottes",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Lottes (Аркадный монитор Тимоти Лоттеса + Bloom)",
         "CRT-Lottes (Timothy Lottes Arcade Cabinet Monitor + Bloom)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-mattias", "CRT-Mattias",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Mattias (Винтажный ЭЛТ-телевизор 80-х с тёплым свечением)",
         "CRT-Mattias (Vintage 80s Warm Glow CRT TV)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-easymode", "CRT-Easymode",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Easymode (Чёткий плоский ЭЛТ без геометрических искажений)",
         "CRT-Easymode (Sharp Flat CRT without Geometry Distortion)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-aperture", "CRT-Aperture",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Aperture (Яркая апертурная маска японских мониторов)",
         "CRT-Aperture (Bright Japanese Monitor Aperture Mask)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-caligari", "CRT-Caligari",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Caligari (Мягкое свечение субпиксельных пятен фосфора)",
         "CRT-Caligari (Soft Subpixel Phosphor Spot Blending)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-hyllian", "CRT-Hyllian",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Hyllian (Резкий BVM-профиль с чистыми сканлайнами)",
         "CRT-Hyllian (Sharp BVM Profile with Clean Scanlines)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-yeetron", "CRT-Yeetron",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Yeetron (Пиксель-точные сканлайны для HD/4K дисплеев)",
         "CRT-Yeetron (Pixel-Exact Scanlines for HD/4K Displays)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-consumer", "CRT-Consumer",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "CRT-Consumer (Домашний аналоговый телевизор 90-х)",
         "CRT-Consumer (90s Consumer Analog TV)",
         Settings::ScalingFilter::Bilinear, 80},
        {"crt-sony-pvm-20l4", "Sony PVM-20L4",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "Sony Trinitron PVM-20L4 (Профессиональный студийный видеомонитор 800 TVL)",
         "Sony Trinitron PVM-20L4 (Pro Broadcast Video Monitor 800 TVL)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"crt-nec-xm29", "NEC XM29+",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "NEC MultiSync XM29 Plus (Эталонный аркадный RGB-монитор)",
         "NEC MultiSync XM29 Plus (Reference Arcade RGB Monitor)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"zfast-crt", "ZFast-CRT",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "ZFast-CRT (Сверхбыстрый лёгкий ЭЛТ-шейдер без нагрузки на ГП)",
         "ZFast-CRT (Ultra-Fast Lightweight CRT Shader)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"scanlines-sine-abs", "Scanlines",
         "📺 1. ЭЛТ-мониторы, кинескопы и аркадные экраны (CRT & Scanlines)",
         "📺 1. CRT Monitors, Phosphor & Arcade Screens (CRT & Scanlines)",
         "Scanlines-Sine-Abs (Классические синусоидальные строки развёртки)",
         "Scanlines-Sine-Abs (Classic Sinusoidal Scanlines)",
         Settings::ScalingFilter::NearestNeighbor, 80},

        // 16..30. Pixel-Art & Vector Upscaling
        {"xbrz-2x", "xBRZ 2x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "xBRZ 2x (Умное векторное сглаживание контуров 2x)",
         "xBRZ 2x (Smart Vector Contour Smoothing 2x)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"xbrz-4x", "xBRZ 4x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "xBRZ 4x (Глубокое векторное сглаживание пиксель-арта 4x)",
         "xBRZ 4x (Deep Vector Pixel-Art Smoothing 4x)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"xbrz-6x", "xBRZ 6x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "xBRZ 6x (Максимальная реконструкция спрайтов 6x)",
         "xBRZ 6x (Maximum Sprite Contour Reconstruction 6x)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"scalefx", "ScaleFX",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "ScaleFX (Субпиксельная интерполяция углов без размытия)",
         "ScaleFX (Subpixel Corner Interpolation without Blur)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"scalefx-aa", "ScaleFX+rAA",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "ScaleFX + rAA (Интерполяция контуров + обратный антиалиасинг)",
         "ScaleFX + rAA (Contour Interpolation + Reverse AA)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"mmpx", "MMPX",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "MMPX (Сохраняющее авторский стиль масштабирование спрайтов)",
         "MMPX (Style-Preserving Sprite Magnification)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"hq2x", "HQ2x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "HQ2x (Классический фильтр высокой чёткости 2x)",
         "HQ2x (Classic High Quality 2x Filter)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"hq4x", "HQ4x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "HQ4x (Классический фильтр высокой чёткости 4x)",
         "HQ4x (Classic High Quality 4x Filter)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"2xsai", "2xSaI",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "2xSaI (Scale and Interpolation 2x)",
         "2xSaI (Scale and Interpolation 2x)",
         Settings::ScalingFilter::Bicubic, 80},
        {"super-2xsai", "Super 2xSaI",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "Super 2xSaI (Усиленная интерполяция граней 2xSaI)",
         "Super 2xSaI (Enhanced 2xSaI Edge Interpolation)",
         Settings::ScalingFilter::Bicubic, 80},
        {"super-eagle", "Super Eagle",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "Super Eagle (Мягкое сглаживание диагональных ступенек)",
         "Super Eagle (Smooth Diagonal Staircase Blending)",
         Settings::ScalingFilter::Bicubic, 80},
        {"omniscale", "OmniScale",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "OmniScale (Разрешение-независимая реконструкция пикселей)",
         "OmniScale (Resolution-Independent Pixel Reconstruction)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"sabr-v3", "SABR v3",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "SABR v3.0 (Продвинутое сглаживание наклонных линий)",
         "SABR v3.0 (Advanced Diagonal Edge Smoothing)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"advmame-4x", "AdvMAME 4x",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "AdvMAME Scale4x (Точное масштабирование без размытия палитры)",
         "AdvMAME Scale4x (Exact Palette-Preserving Scale4x)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"2xsal", "2xSal",
         "✨ 2. Пиксель-арт и векторное сглаживание (Pixel-Art & Upscaling)",
         "✨ 2. Pixel-Art & Vector Contour Upscaling (Pixel-Art & Upscaling)",
         "2xSal (Плавная контурная фильтрация 2D-графики)",
         "2xSal (Smooth 2D Contour Filtering)",
         Settings::ScalingFilter::Bicubic, 80},

        // 31..38. Anime, Neural & Super-Resolution
        {"anime4k-v4-restore", "Anime4K Restore",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "Anime4K v4.0 Restore + Upscale (Восстановление контуров аниме, JRPG и 3D)",
         "Anime4K v4.0 Restore + Upscale (Line Reconstruction for Anime, JRPG & 3D)",
         Settings::ScalingFilter::Fsr, 92},
        {"anime4k-v4-fast", "Anime4K Fast",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "Anime4K v4.0 Fast (Быстрое сглаживание и чёткость линий для 60+ FPS)",
         "Anime4K v4.0 Fast (High-Speed Line Clarity for 60+ FPS)",
         Settings::ScalingFilter::Fsr, 82},
        {"fsr-easu-rcas", "FSR EASU+RCAS",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "AMD FSR 1.0 EASU + RCAS (FidelityFX Super Resolution — Адаптивный апскейл)",
         "AMD FSR 1.0 EASU + RCAS (FidelityFX Super Resolution)",
         Settings::ScalingFilter::Fsr, 85},
        {"sgsr-v2", "SGSR v2",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "Snapdragon SGSR v2 (Edge-Directed Super Resolution)",
         "Snapdragon SGSR v2 (Edge-Directed Super Resolution)",
         Settings::ScalingFilter::Fsr, 75},
        {"scaleforce", "ScaleForce",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "ScaleForce (Высокоточное масштабирование 3D/2D текстур)",
         "ScaleForce (High-Precision 3D/2D Texture Upscaler)",
         Settings::ScalingFilter::ScaleForce, 80},
        {"ravu-zoom-r3", "RAVU-Zoom",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "RAVU-Zoom-R3 (Rapid and Accurate Video Upscaling)",
         "RAVU-Zoom-R3 (Rapid and Accurate Video Upscaling)",
         Settings::ScalingFilter::Fsr, 88},
        {"nnedi3-64", "NNEDI3-64",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "NNEDI3 64-Neurons (Нейросетевой направленный интерполятор краёв)",
         "NNEDI3 64-Neurons (Neural Network Edge-Directed Interpolator)",
         Settings::ScalingFilter::ScaleForce, 85},
        {"fsrcnn-x", "FSRCNNX",
         "🎬 3. Аниме, нейросети и супер-разрешение (Anime, AI & Super-Resolution)",
         "🎬 3. Anime, Neural Networks & Super-Resolution (Anime, AI & Super-Res)",
         "FSRCNNX (Свёрточная нейросетевая детализация мелких текстур)",
         "FSRCNNX (Convolutional Super-Resolution Detail Enhancer)",
         Settings::ScalingFilter::Fsr, 95},

        // 39..45. Windowed Sinc / Bicubic / Lanczos
        {"jinc2-sharp", "Jinc2-Sharp",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Jinc2-Sharp (EWA Lanczos без звона и ореолов вокруг контуров)",
         "Jinc2-Sharp (Anti-Ringing EWA Lanczos)",
         Settings::ScalingFilter::Bicubic, 80},
        {"lanczos3-sharp", "Lanczos3",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Lanczos3-Sharp (Трёхлепестковый синхронный фильтр Ланцоша)",
         "Lanczos3-Sharp (3-Lobe Windowed Sinc Lanczos)",
         Settings::ScalingFilter::Bicubic, 80},
        {"spline36", "Spline36",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Spline36 (Кинематографический сплайн-апскейлер высокой точности)",
         "Spline36 (High-Precision Cinematic Spline Upscaler)",
         Settings::ScalingFilter::Bicubic, 80},
        {"mitchell-netravali", "Mitchell",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Bicubic Mitchell-Netravali (Сбалансированная бикубическая фильтрация B=1/3, C=1/3)",
         "Bicubic Mitchell-Netravali (Balanced Bicubic B=1/3, C=1/3)",
         Settings::ScalingFilter::Bicubic, 80},
        {"catmull-rom", "Catmull-Rom",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Bicubic Catmull-Rom (Резкая бикубическая интерполяция B=0, C=0.5)",
         "Bicubic Catmull-Rom (Sharp Bicubic Interpolation B=0, C=0.5)",
         Settings::ScalingFilter::Bicubic, 85},
        {"sharp-bilinear", "Sharp-Bilinear",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Sharp-Bilinear (Чёткие пиксели без муара при нецелом масштабировании)",
         "Sharp-Bilinear (Shimmer-Free Crisp Pixels at Fractional Scale)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"pixel-aa", "Pixel-AA",
         "📐 4. Прецизионные ядра интерполяции (Lanczos / Jinc / Spline / Bicubic)",
         "📐 4. Precision Interpolation Kernels (Lanczos / Jinc / Spline / Bicubic)",
         "Pixel-AA (Субпиксельное сглаживание пиксельной сетки без мыла)",
         "Pixel-AA (Blur-Free Subpixel Grid Antialiasing)",
         Settings::ScalingFilter::Bilinear, 80},

        // 46..53. Handheld & LCD Matrices
        {"lcd-grid-v2", "LCD-Grid v2",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "LCD-Grid v2 (Субпиксельная RGB-сетка портативного ЖК-экрана)",
         "LCD-Grid v2 (Handheld RGB Subpixel LCD Matrix)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"zfast-lcd", "ZFast-LCD",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "ZFast-LCD (Быстрая имитация ЖК-матрицы портативной консоли)",
         "ZFast-LCD (Fast Lightweight Handheld LCD Grid)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"gba-color-lcd", "GBA LCD",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "GBA Color + LCD Grid (Тёплая цветопередача и сетка Game Boy Advance)",
         "GBA Color + LCD Grid (Warm Game Boy Advance Color & Grid)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"gbc-dotmatrix", "GBC Matrix",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "GBC Dot-Matrix (Рефлективная точечная матрица Game Boy Color)",
         "GBC Dot-Matrix (Reflective Game Boy Color Dot-Matrix)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"gb-dmg-green", "GB DMG Green",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "Game Boy DMG-01 Olive Green (Классический оливковый экран 1989 г.)",
         "Game Boy DMG-01 Olive Green (Classic 1989 Olive Dot-Matrix)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"gb-pocket-mono", "GB Pocket",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "Game Boy Pocket Silver (Монохромная матрица высокой контрастности)",
         "Game Boy Pocket Silver (High-Contrast Monochrome Matrix)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"psp-color", "PSP Color",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "PSP Color Matrix (Широкий цветовой охват экрана PlayStation Portable)",
         "PSP Color Matrix (PlayStation Portable Display Gamut)",
         Settings::ScalingFilter::Bilinear, 80},
        {"nds-color-grid", "NDS Grid",
         "🕹️ 5. Портативные консоли и ЖК-матрицы (Handheld & LCD Grids)",
         "🕹️ 5. Handheld Consoles & LCD Matrices (Handheld & LCD Grids)",
         "NDS LCD Grid (Субпиксельная сетка и гамма Nintendo DS)",
         "NDS LCD Grid (Nintendo DS Subpixel Grid & Gamma)",
         Settings::ScalingFilter::NearestNeighbor, 80},

        // 54..58. Sharpening & Micro-Contrast
        {"amd-cas", "AMD CAS",
         "⚡ 6. Повышение резкости и микроконтраста (Sharpening & Clarity)",
         "⚡ 6. Image Sharpening & Micro-Contrast (Sharpening & Clarity)",
         "AMD CAS (Contrast Adaptive Sharpening — Адаптивная резкость текстур)",
         "AMD CAS (Contrast Adaptive Sharpening)",
         Settings::ScalingFilter::Fsr, 90},
        {"lumasharpen", "LumaSharpen",
         "⚡ 6. Повышение резкости и микроконтраста (Sharpening & Clarity)",
         "⚡ 6. Image Sharpening & Micro-Contrast (Sharpening & Clarity)",
         "LumaSharpen (Контурная резкость по каналу яркости без цветовых ореолов)",
         "LumaSharpen (Luma-Channel Edge Sharpening without Chroma Halo)",
         Settings::ScalingFilter::Fsr, 85},
        {"adaptive-sharpen", "AdaptiveSharpen",
         "⚡ 6. Повышение резкости и микроконтраста (Sharpening & Clarity)",
         "⚡ 6. Image Sharpening & Micro-Contrast (Sharpening & Clarity)",
         "Adaptive Sharpen (Глубокое выделение микрорельефа поверхностей)",
         "Adaptive Sharpen (Deep Surface Micro-Detail Enhancer)",
         Settings::ScalingFilter::Fsr, 96},
        {"fine-sharp", "FineSharp",
         "⚡ 6. Повышение резкости и микроконтраста (Sharpening & Clarity)",
         "⚡ 6. Image Sharpening & Micro-Contrast (Sharpening & Clarity)",
         "FineSharp (Тонкая детализация дальних планов и текста интерфейса)",
         "FineSharp (Fine Distant Detail & UI Font Enhancer)",
         Settings::ScalingFilter::Fsr, 88},
        {"fast-sharpen", "Fast-Sharpen",
         "⚡ 6. Повышение резкости и микроконтраста (Sharpening & Clarity)",
         "⚡ 6. Image Sharpening & Micro-Contrast (Sharpening & Clarity)",
         "Fast-Sharpen (Сверхлёгкий фильтр повышения чёткости)",
         "Fast-Sharpen (Ultra-Light Clarity Filter)",
         Settings::ScalingFilter::Fsr, 78},

        // 59..64. Anti-Aliasing, De-Dithering & Debanding
        {"smaa-ultra", "SMAA Ultra",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "SMAA Ultra (Subpixel Morphological Anti-Aliasing — Морфологическое сглаживание)",
         "SMAA Ultra (Subpixel Morphological Anti-Aliasing)",
         Settings::ScalingFilter::Bicubic, 80},
        {"fxaa-quality", "FXAA Quality",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "FXAA Quality (Быстрое полноэкранное сглаживание полигональных граней)",
         "FXAA Quality (Fast Full-Screen Polygon Edge Smoothing)",
         Settings::ScalingFilter::Bilinear, 80},
        {"taa-soft-film", "Temporal AA",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "Temporal Soft AA (Мягкое кинематографическое устранение мерцания граней)",
         "Temporal Soft AA (Cinematic Edge Shimmer Reduction)",
         Settings::ScalingFilter::Bicubic, 80},
        {"deband-smooth", "Deband Pro",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "Deband Pro (Устранение ступенчатых градиентов на небе, тумане и тенях)",
         "Deband Pro (Removes Color Banding in Sky, Fog & Shadows)",
         Settings::ScalingFilter::Bicubic, 80},
        {"mdapt", "MDAPT",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "MDAPT (Слияние шахматного дизеринга в плавные полупрозрачные тени)",
         "MDAPT (Merge Dithering and Pseudo-Transparency)",
         Settings::ScalingFilter::Bilinear, 80},
        {"gdapt", "GDAPT",
         "🎨 7. Сглаживание граней, дизеринг и устранение бандинга (AA & Deband)",
         "🎨 7. Anti-Aliasing, De-Dithering & Debanding (AA & Deband)",
         "GDAPT (Градиентное сглаживание дизеринга ретро-портов)",
         "GDAPT (Gradient De-Dithering Filter)",
         Settings::ScalingFilter::Bilinear, 80},

        // 65..74. Cinema, Color Grading, HDR & Retro FX
        {"hdr-vibrance-bloom", "Fake HDR+Bloom",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Fake HDR + Vibrance & Bloom (Сочный динамический контраст и мягкое свечение)",
         "Fake HDR + Vibrance & Bloom (Rich Dynamic Contrast & Soft Glow)",
         Settings::ScalingFilter::Fsr, 86},
        {"oled-vivid-black", "Switch OLED",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Switch OLED Vivid (Глубокий чёрный и насыщенная палитра экрана Switch OLED)",
         "Switch OLED Vivid (Deep True Black & Vivid Switch OLED Palette)",
         Settings::ScalingFilter::Fsr, 85},
        {"vhs-pro-rewind", "VHS Pro Tape",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "VHS Pro Tape (Магнитная кассета 80-х, хроматический сдвиг и строки)",
         "VHS Pro Tape (80s Magnetic Tape, Chroma Shift & Tracking Lines)",
         Settings::ScalingFilter::Bilinear, 80},
        {"ntsc-adaptive", "NTSC Composite",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "NTSC Adaptive Composite (Аналоговый композитный ТВ-сигнал с радужным отливом)",
         "NTSC Adaptive Composite (Analog Composite TV Signal & Rainbow Artifacting)",
         Settings::ScalingFilter::Bilinear, 80},
        {"film-grain-halation", "35mm Film",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "35mm Film Grain & Halation (Аналоговая киноплёнка 35 мм с ореолом светов)",
         "35mm Film Grain & Halation (Analog 35mm Cinema Stock)",
         Settings::ScalingFilter::Bicubic, 80},
        {"technicolor-cine", "Technicolor",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Technicolor Cine (Винтажная кинематографическая цветокоррекция)",
         "Technicolor Cine (Vintage Cinema Color Grading)",
         Settings::ScalingFilter::Bicubic, 80},
        {"cel-shading-ink", "Cel-Shading",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Cel-Shading / Comic Ink (Комиксная тушь и выразительная обводка контуров)",
         "Cel-Shading / Comic Ink (Comic Book Ink Outline & Shading)",
         Settings::ScalingFilter::ScaleForce, 90},
        {"sepia-vintage", "Sepia Vintage",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Sepia Vintage (Тёплая старинная фотография — Сепия)",
         "Sepia Vintage (Warm Antique Photograph Toning)",
         Settings::ScalingFilter::Bicubic, 80},
        {"night-vision-nvg", "Night Vision",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Night Vision Goggles (Прибор ночного видения ПНВ с зелёным люминофором)",
         "Night Vision Goggles (Green Phosphor Tactical NVG)",
         Settings::ScalingFilter::NearestNeighbor, 80},
        {"kawase-glow-crt", "Kawase Glow",
         "🎞️ 8. Киноэффекты, цветокоррекция, HDR и ретро-стилизации (Cinema & Retro FX)",
         "🎞️ 8. Cinema FX, Color Grading, HDR & Retro Styling (Cinema & Retro FX)",
         "Halation & Dual-Kawase Glow (Неоновое атмосферное свечение ярких объектов)",
         "Halation & Dual-Kawase Glow (Neon Atmospheric Highlight Bloom)",
         Settings::ScalingFilter::Bicubic, 80},
    };
    return kCatalog;
}

inline QString GetLocalizedCategory(const RetroArchShaderEntry& entry) {
    const QString ru = QString::fromUtf8(entry.category_ru);
    const QString en = QString::fromUtf8(entry.category_en);
    if (ru.startsWith(QStringLiteral("🚫 0."))) {
        return StormLang(ru, en,
                         QStringLiteral("🚫 0. Kein Shader (Standard-Ausgabe)"),
                         QStringLiteral("🚫 0. Aucun shader (Rendu standard)"),
                         QStringLiteral("🚫 0. 无着色器 (标准原生渲染)"),
                         QStringLiteral("🚫 0. シェーダーなし (標準ネイティブ出力)"),
                         QStringLiteral("🚫 0. بدون مظلل (عرض قياسي)"),
                         QStringLiteral("🚫 0. Sin sombreador (Salida nativa estándar)"));
    }
    if (ru.startsWith(QStringLiteral("📺 1."))) {
        return StormLang(ru, en,
                         QStringLiteral("📺 1. CRT-Monitore, Phosphor & Arcade-Bildschirme (CRT & Scanlines)"),
                         QStringLiteral("📺 1. Moniteurs CRT, phosphore et écrans d'arcade (CRT & Scanlines)"),
                         QStringLiteral("📺 1. CRT 显像管、荧光粉与街机屏幕 (CRT & Scanlines)"),
                         QStringLiteral("📺 1. CRTモニター・蛍光体・アーケード画面 (CRT & Scanlines)"),
                         QStringLiteral("📺 1. شاشات CRT والفوسفور والأركيد (CRT & Scanlines)"),
                         QStringLiteral("📺 1. Monitores CRT, fósforo y pantallas arcade (CRT & Scanlines)"));
    }
    if (ru.startsWith(QStringLiteral("✨ 2."))) {
        return StormLang(ru, en,
                         QStringLiteral("✨ 2. Pixel-Art & Vektor-Konturenglättung (Pixel-Art & Upscaling)"),
                         QStringLiteral("✨ 2. Pixel-Art et lissage vectoriel des contours (Pixel-Art & Upscaling)"),
                         QStringLiteral("✨ 2. 像素艺术与矢量轮廓平滑放大 (Pixel-Art & Upscaling)"),
                         QStringLiteral("✨ 2. ピクセルアート＆ベクター輪郭補間 (Pixel-Art & Upscaling)"),
                         QStringLiteral("✨ 2. فن البكسل وتنعيم الحواف المتجه (Pixel-Art & Upscaling)"),
                         QStringLiteral("✨ 2. Pixel-Art y suavizado vectorial de contornos (Pixel-Art & Upscaling)"));
    }
    if (ru.startsWith(QStringLiteral("🎬 3."))) {
        return StormLang(ru, en,
                         QStringLiteral("🎬 3. Anime, Neuronale Netze & Super-Resolution (Anime, AI & Super-Res)"),
                         QStringLiteral("🎬 3. Anime, réseaux neuronaux et super-résolution (Anime, AI & Super-Res)"),
                         QStringLiteral("🎬 3. 动漫、神经网络与超分辨率 (Anime, AI & Super-Res)"),
                         QStringLiteral("🎬 3. アニメ・AI・超解像アップスケーリング (Anime, AI & Super-Res)"),
                         QStringLiteral("🎬 3. الأنمي والشبكات العصبية والدقة الفائقة (Anime, AI & Super-Res)"),
                         QStringLiteral("🎬 3. Anime, redes neuronales y superresolución (Anime, AI & Super-Res)"));
    }
    if (ru.startsWith(QStringLiteral("📐 4."))) {
        return StormLang(ru, en,
                         QStringLiteral("📐 4. Präzisions-Interpolationskerne (Lanczos / Jinc / Spline / Bicubic)"),
                         QStringLiteral("📐 4. Noyaux d'interpolation de précision (Lanczos / Jinc / Spline / Bicubic)"),
                         QStringLiteral("📐 4. 高精度插值核 (Lanczos / Jinc / Spline / Bicubic)"),
                         QStringLiteral("📐 4. 高精度補間カーネル (Lanczos / Jinc / Spline / Bicubic)"),
                         QStringLiteral("📐 4. نوى الاستيفاء الدقيقة (Lanczos / Jinc / Spline / Bicubic)"),
                         QStringLiteral("📐 4. Kernels de interpolación de precisión (Lanczos / Jinc / Spline / Bicubic)"));
    }
    if (ru.startsWith(QStringLiteral("🕹️ 5."))) {
        return StormLang(ru, en,
                         QStringLiteral("🕹️ 5. Handheld-Konsolen & LCD-Matrizen (Handheld & LCD Grids)"),
                         QStringLiteral("🕹️ 5. Consoles portables et matrices LCD (Handheld & LCD Grids)"),
                         QStringLiteral("🕹️ 5. 掌机与液晶点阵网格 (Handheld & LCD Grids)"),
                         QStringLiteral("🕹️ 5. 携帯ゲーム機＆LCDマトリクス (Handheld & LCD Grids)"),
                         QStringLiteral("🕹️ 5. الأجهزة المحمولة ومصفوفات LCD (Handheld & LCD Grids)"),
                         QStringLiteral("🕹️ 5. Consolas portátiles y matrices LCD (Handheld & LCD Grids)"));
    }
    if (ru.startsWith(QStringLiteral("⚡ 6."))) {
        return StormLang(ru, en,
                         QStringLiteral("⚡ 6. Bildschärfe & Mikrokontrast (Sharpening & Clarity)"),
                         QStringLiteral("⚡ 6. Netteté d'image et micro-contraste (Sharpening & Clarity)"),
                         QStringLiteral("⚡ 6. 图像锐化与微对比度增强 (Sharpening & Clarity)"),
                         QStringLiteral("⚡ 6. シャープネス＆マイクロコントラスト強調 (Sharpening & Clarity)"),
                         QStringLiteral("⚡ 6. زيادة الحدة والتباين الدقيق (Sharpening & Clarity)"),
                         QStringLiteral("⚡ 6. Nitidez de imagen y microcontraste (Sharpening & Clarity)"));
    }
    if (ru.startsWith(QStringLiteral("🎨 7."))) {
        return StormLang(ru, en,
                         QStringLiteral("🎨 7. Kantenglättung, De-Dithering & Debanding (AA & Deband)"),
                         QStringLiteral("🎨 7. Anticrénelage, dé-tramage et dé-bande (AA & Deband)"),
                         QStringLiteral("🎨 7. 抗锯齿、去抖动与消除色带 (AA & Deband)"),
                         QStringLiteral("🎨 7. アンチエイリアス・ディザ解除・バンディング除去 (AA & Deband)"),
                         QStringLiteral("🎨 7. تنعيم الحواف وإزالة التدرج اللوني (AA & Deband)"),
                         QStringLiteral("🎨 7. Antialiasing, eliminación de dithering y banding (AA & Deband)"));
    }
    return StormLang(ru, en,
                     QStringLiteral("🎞️ 8. Kinoeffekte, Farbkorrektur, HDR & Retro-Stile (Cinema & Retro FX)"),
                     QStringLiteral("🎞️ 8. Effets cinéma, étalonnage, HDR et style rétro (Cinema & Retro FX)"),
                     QStringLiteral("🎞️ 8. 电影特效、调色、HDR 与复古风格 (Cinema & Retro FX)"),
                     QStringLiteral("🎞️ 8. シネマ効果・色調補正・HDR・レトロ演出 (Cinema & Retro FX)"),
                     QStringLiteral("🎞️ 8. مؤثرات سينمائية وتصحيح ألوان وHDR (Cinema & Retro FX)"),
                     QStringLiteral("🎞️ 8. Efectos de cine, etalonaje, HDR y estilo retro (Cinema & Retro FX)"));
}

inline QString GetLocalizedName(const RetroArchShaderEntry& entry) {
    if (std::string(entry.id) == "none") {
        return StormLang(
            QString::fromUtf8(entry.name_ru),
            QString::fromUtf8(entry.name_en),
            QStringLiteral("Kein Shader (Sauberer Standard-Render)"),
            QStringLiteral("Aucun shader (Rendu natif propre — Par défaut)"),
            QStringLiteral("无着色器（纯净原生渲染 — 默认）"),
            QStringLiteral("シェーダーなし（クリーン標準レンダー — デフォルト）"),
            QStringLiteral("بدون مظلل (عرض أصلي نظيف — افتراضي)"),
            QStringLiteral("Sin sombreador (Renderizado nativo limpio — Predeterminado)"));
    }
    return StormLang(QString::fromUtf8(entry.name_ru), QString::fromUtf8(entry.name_en));
}

inline QString GetLocalizedShortName(int index) {
    const auto& cat = GetRetroArchShaderCatalog();
    if (index <= 0 || index >= static_cast<int>(cat.size())) {
        return StormLang(QStringLiteral("ВЫКЛ"), QStringLiteral("OFF"),
                         QStringLiteral("AUS"), QStringLiteral("DÉSACT."),
                         QStringLiteral("关闭"), QStringLiteral("オフ"),
                         QStringLiteral("معطل"), QStringLiteral("DESACT."));
    }
    return QString::fromUtf8(cat[static_cast<std::size_t>(index)].short_name);
}

inline int ResolveShaderIndex(const QString& id) {
    const auto& cat = GetRetroArchShaderCatalog();
    for (std::size_t i = 0; i < cat.size(); ++i) {
        if (id.compare(QString::fromUtf8(cat[i].id), Qt::CaseInsensitive) == 0) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

inline void SyncShaderFromQSettings() {
    const auto& cat = GetRetroArchShaderCatalog();
    const int idx = std::clamp(Settings::values.post_processing_shader.GetValue(), 0, static_cast<int>(cat.size()) - 1);
    QSettings qs;
    qs.setValue(QStringLiteral("StormSwitch/RetroArchShader"), QString::fromUtf8(cat[static_cast<std::size_t>(idx)].id));
}

inline void ApplyShaderByIndex(int index) {
    const auto& cat = GetRetroArchShaderCatalog();
    const int clamped = std::clamp(index, 0, static_cast<int>(cat.size()) - 1);
    const auto& entry = cat[static_cast<std::size_t>(clamped)];
    Settings::values.post_processing_shader.SetValue(clamped);
    QSettings qs;
    qs.setValue(QStringLiteral("StormSwitch/RetroArchShader"), QString::fromUtf8(entry.id));
}

inline void ShowSearchableShaderPopup(QWidget* anchor, int current_index, const std::function<void(int)>& on_selected) {
    QDialog popup(anchor, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    const int popup_w = std::max(anchor ? anchor->width() : 0, 620);
    const int popup_h = 450;
    QPoint global_pos = anchor ? anchor->mapToGlobal(QPoint(0, anchor->height() + 2)) : QPoint(200, 200);
    if (QScreen* scr = anchor ? anchor->screen() : nullptr) {
        const QRect avail = scr->availableGeometry();
        if (global_pos.x() + popup_w > avail.right()) {
            global_pos.setX(std::max(avail.left() + 8, avail.right() - popup_w - 8));
        }
        if (global_pos.y() + popup_h > avail.bottom()) {
            global_pos.setY(anchor->mapToGlobal(QPoint(0, -popup_h - 2)).y());
        }
    }
    popup.setGeometry(global_pos.x(), global_pos.y(), popup_w, popup_h);
    popup.setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #0B0F19;"
        "  border: 1.5px solid #00E5FF;"
        "  border-radius: 10px;"
        "}"
        "QLineEdit {"
        "  background-color: #131C2E;"
        "  color: #F8FAFC;"
        "  border: 1.5px solid #38BDF8;"
        "  border-radius: 7px;"
        "  padding: 6px 12px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  min-height: 24px;"
        "}"
        "QLineEdit:focus { border: 1.5px solid #00F0FF; background-color: #17233A; }"
        "QListWidget {"
        "  background-color: #0F172A;"
        "  border: 1px solid #1E293B;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  outline: none;"
        "  color: #E2E8F0;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 10px;"
        "  border-radius: 5px;"
        "  margin: 1px 2px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: rgba(56, 189, 248, 0.20);"
        "  color: #FFFFFF;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(2, 132, 199, 0.55);"
        "  border: 1px solid #00E5FF;"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "}"
        "QScrollBar:vertical {"
        "  background: #0B0F19;"
        "  width: 8px;"
        "  margin: 2px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #00E5FF;"
        "  min-height: 28px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    ));

    auto* vbox = new QVBoxLayout(&popup);
    vbox->setContentsMargins(10, 10, 10, 10);
    vbox->setSpacing(8);

    auto* search_edit = new QLineEdit(&popup);
    search_edit->setClearButtonEnabled(true);
    search_edit->setPlaceholderText(StormLang(
        QStringLiteral("🔍 Быстрый поиск шейдера RetroArch (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 Fast search RetroArch shaders (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 Schnellsuche nach RetroArch-Shadern (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 Recherche rapide de shaders RetroArch (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 快速搜索 RetroArch 着色器 (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 RetroArch シェーダーを高速検索 (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 بحث سريع عن مظللات RetroArch (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)..."),
        QStringLiteral("🔍 Búsqueda rápida de sombreadores RetroArch (CRT, Royale, xBRZ, Anime4K, ScaleFX, CAS, LCD, VHS)...")));
    vbox->addWidget(search_edit);

    auto* list_widget = new QListWidget(&popup);
    list_widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vbox->addWidget(list_widget, 1);

    const auto& catalog = GetRetroArchShaderCatalog();
    const int safe_cur = std::clamp(current_index, 0, static_cast<int>(catalog.size()) - 1);

    auto rebuild_list = [&](const QString& filter_raw) {
        list_widget->clear();
        const QString filter = filter_raw.trimmed();
        QString last_cat;
        QListWidgetItem* item_to_select = nullptr;

        for (std::size_t i = 0; i < catalog.size(); ++i) {
            const auto& entry = catalog[i];
            const QString qid = QString::fromUtf8(entry.id);
            const QString qcat = GetLocalizedCategory(entry);
            const QString qname = GetLocalizedName(entry);
            const QString qname_en = QString::fromUtf8(entry.name_en);
            const QString qname_ru = QString::fromUtf8(entry.name_ru);

            if (!filter.isEmpty()) {
                const bool match = qname.contains(filter, Qt::CaseInsensitive) ||
                                   qname_en.contains(filter, Qt::CaseInsensitive) ||
                                   qname_ru.contains(filter, Qt::CaseInsensitive) ||
                                   qid.contains(filter, Qt::CaseInsensitive) ||
                                   qcat.contains(filter, Qt::CaseInsensitive);
                if (!match) {
                    continue;
                }
            }

            if (qcat != last_cat) {
                last_cat = qcat;
                auto* hdr_item = new QListWidgetItem(qcat);
                hdr_item->setFlags(Qt::NoItemFlags);
                QFont f = hdr_item->font();
                f.setBold(true);
                hdr_item->setFont(f);
                hdr_item->setForeground(QColor(0, 229, 255));
                hdr_item->setBackground(QColor(15, 29, 52, 210));
                hdr_item->setSizeHint(QSize(popup_w - 36, 30));
                list_widget->addItem(hdr_item);
            }

            auto* item = new QListWidgetItem(QStringLiteral("   ") + qname);
            item->setData(Qt::UserRole, static_cast<int>(i));
            item->setSizeHint(QSize(popup_w - 36, 28));
            list_widget->addItem(item);
            if (static_cast<int>(i) == safe_cur || (!item_to_select && !filter.isEmpty())) {
                item_to_select = item;
            }
        }
        if (item_to_select) {
            list_widget->setCurrentItem(item_to_select);
            list_widget->scrollToItem(item_to_select, QAbstractItemView::PositionAtCenter);
        }
    };

    rebuild_list(QString());

    QObject::connect(search_edit, &QLineEdit::textChanged, &popup, [&](const QString& txt) {
        rebuild_list(txt);
    });

    QObject::connect(list_widget, &QListWidget::itemClicked, &popup, [&](QListWidgetItem* item) {
        if (!item || !(item->flags() & Qt::ItemIsSelectable)) {
            return;
        }
        const int chosen_idx = item->data(Qt::UserRole).toInt();
        if (on_selected) {
            on_selected(chosen_idx);
        }
        popup.accept();
    });

    QObject::connect(search_edit, &QLineEdit::returnPressed, &popup, [&]() {
        if (auto* cur = list_widget->currentItem()) {
            if (cur->flags() & Qt::ItemIsSelectable) {
                const int chosen_idx = cur->data(Qt::UserRole).toInt();
                if (on_selected) {
                    on_selected(chosen_idx);
                }
                popup.accept();
            }
        }
    });

    search_edit->setFocus();
    popup.exec();
}

} // namespace StormShaders
