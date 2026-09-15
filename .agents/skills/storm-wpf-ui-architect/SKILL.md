---
name: "storm-wpf-ui-architect"
description: "Проектирует современные WPF-интерфейсы на базе WPF-UI (.NET 9), векторных сеток Lucide/Tabler Icons, аппаратных Lottie-анимаций SkiaSharp и 8 фирменных тем STORM SOFT."
---
# STORM WPF UI Architect & Visual Design System

## Назначение
Обеспечивает создание премиального визуального опыта приложений экосистемы STORM SOFT на базе .NET 9 и WPF:
- Использование компонентов **WPF-UI (`Wpf.Ui`)** с плавной прокруткой и современной геометрией.
- Интеграция 8 фирменных тем (`STORM DARK`, `STORM NIGHT`, `STORM DAY`, `STORM MIDNIGHT`, `STORM MATRIX`, `STORM CYBERPUNK`, `STORM FANTASY`, `STORM WARHAMMER 40K`).
- Объёмный 3D-стиль: тени `DropShadowEffect` (`Direction=270`), тактильный прессинг (`TranslateTransform`), сплошные чекбоксы без галочек, стилизованные слайдеры.
- Замена растровых иконок на векторные XAML `Path` по сеткам **Lucide Icons** и **Tabler Icons**.
- Интеграция векторных анимаций **Lottie** через **SkiaSharp** (120 FPS, нулевая нагрузка на CPU) в инсталляторах `StormInstaller` и лоадерах.
