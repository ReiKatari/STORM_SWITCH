---
name: "storm-graphics-upscaler-shaders"
description: "Обеспечивает нейросетевой AI-апскейлинг обложек и текстур через Real-ESRGAN Vulkan до 4K, а также интеграцию библиотеки Libretro Slang Shaders и алгоритмов AMD FidelityFX CAS в эмуляторы."
---
# STORM Graphics Upscaler, Shaders & FidelityFX

## Назначение
Премиальное визуальное улучшение графики в играх и эмуляторах (`STORM EDEN 3`, `STORM DS`, `STORM RETRO`, `STORM GAME TRANSLATOR`):
1. **Автономный AI-апскейлинг (Real-ESRGAN-ncnn-vulkan)**:
   - Постеры и фото: `realesrgan-ncnn-vulkan.exe -i in.png -o out.png -s 4 -n realesrgan-x4plus`
   - Аниме и рисованный арт: `realesrgan-ncnn-vulkan.exe -i in.png -o out.png -s 4 -n realesrgan-x4plus-anime`
   - Работает автономно на любом GPU (Vulkan) без Python и внешних зависимостей.
2. **Библиотека шейдеров Libretro Slang (`E:\TOOLS\Shaders`)**:
   - CRT-мониторы: `crt-royale`, `crt-geom`, `Mega_Bezel`.
   - Сглаживание пикселей: `xBRZ`, `ScaleFX`, `Bilateral`.
3. **AMD FidelityFX CAS (Contrast Adaptive Sharpening)**:
   - Шейдер `sharpen/shaders/rcas.slang` для мгновенного устранения мыла при апскейле без падения FPS.
