---
name: "storm-asset-optimizer"
description: "Обеспечивает автоматизированную оптимизацию графических ассетов без потерь: сжатие PNG через Oxipng и конвертацию игровых текстур в Direct3D форматы (DDS BC1-BC7) через Microsoft texconv."
---
# STORM Asset & Texture Optimizer

## Назначение
Максимальная оптимизация веса дистрибутивов и графической производительности:
1. **Сжатие PNG без потерь (Oxipng)**:
   - Команда: `rtk oxipng -o max --strip all <files>`
   - Уменьшение размера иконок, сплэшей и ассетов на 20-45% без потери единого пикселя.
2. **Конвейер текстур Direct3D (DirectXTex / texconv)**:
   - Высококачественные текстуры: `rtk texconv -f BC7_UNORM -m 0 <input_file>`
   - Базовые диффузные карты: `rtk texconv -f BC1_UNORM -m 0 <input_file>`
   - Генерация mipmap-уровней для сглаживания текстур на расстоянии в 3D/эмуляторах.
