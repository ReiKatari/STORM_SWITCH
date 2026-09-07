#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STORM SWITCH Auto-Tuner and Performance Preset Manager
Configures and optimizes general STORM SWITCH emulator settings for Low, Balanced, or High Quality hardware.
"""

import os
import sys
import shutil
import argparse
from datetime import datetime
from pathlib import Path

# Metadata mapping each internal config key to exact UI parameter name, human-readable values, and explanation why it is changed
SETTINGS_METADATA = {
    ("Cpu", "cpu_accuracy"): {
        "name": "Точность ЦП",
        "desc": "максимальная скорость и совместимость JIT-компилятора Dynarmic",
        "values": {"0": "Авто", "1": "Точный", "2": "Небезопасный"}
    },
    ("Cpu", "cpuopt_fastmem"): {
        "name": "Эмуляция Host MMU (fastmem)",
        "desc": "прямой маппинг виртуальной памяти для стабильных 60 FPS",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Cpu", "cpuopt_ignore_memory_aborts"): {
        "name": "Игнорировать прерывания памяти",
        "desc": "защита от аварийных вылетов при обращениях за границы буфера",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "resolution_setup"): {
        "name": "Разрешение рендеринга",
        "desc": "масштабирование базового разрешения для оптимизации нагрузки на видеокарту",
        "values": {"0": "0.5X (360p/540p)", "1": "0.75X (540p/810p)", "2": "1X (720p/1080p)", "5": "2X (1440p/2160p)"}
    },
    ("Renderer", "gpu_accuracy"): {
        "name": "Точность ГПУ",
        "desc": "баланс скорости и точности графического процессора",
        "values": {"0": "Быстрый", "1": "Высокая точность"}
    },
    ("Renderer", "astc_recompression"): {
        "name": "Пересжатие текстур ASTC",
        "desc": "сжатие текстур для экономии видеопамяти VRAM на слабых ГПУ",
        "values": {"0": "Без сжатия (Лучшее качество)", "1": "BC1 (Низкое качество)", "2": "BC3 (Среднее качество)", "3": "BC5 (Высокое качество)"}
    },
    ("Renderer", "astc_decode_mode"): {
        "name": "Метод декодирования ASTC",
        "desc": "распределение декодирования текстур между ЦП и ГПУ",
        "values": {"0": "ЦП", "1": "ГПУ", "2": "ЦП Асинхронно", "3": "Гибридный"}
    },
    ("Renderer", "use_asynchronous_shaders"): {
        "name": "Асинхронная компиляция шейдеров",
        "desc": "фоновая компиляция шейдеров исключает внутриигровые микрофризы",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "async_presentation"): {
        "name": "Асинхронный вывод",
        "desc": "отдельный поток вывода кадров для снижения задержек ввода",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "use_reactive_flushing"): {
        "name": "Реактивный сброс памяти",
        "desc": "управление точностью сброса кэшированных поверхностей",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "sync_memory_operations"): {
        "name": "Синхронизация операций памяти",
        "desc": "синхронизация потоков памяти между процессором и видеокартой",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "use_fast_gpu_time"): {
        "name": "Тайминги ГПУ",
        "desc": "ускоренная синхронизация таймингов кадров для высокого FPS",
        "values": {"true": "Ускоренный", "false": "Стандартный"}
    },
    ("Renderer", "eco_frame_pacing"): {
        "name": "Эко-выравнивание кадров",
        "desc": "устранение холостой нагрузки ядер ЦП в ожидании кадров",
        "values": {"true": "Включено", "false": "Отключено"}
    },
    ("Renderer", "max_anisotropy"): {
        "name": "Анизотропная фильтрация",
        "desc": "четкость текстур под острым углом к камере",
        "values": {"0": "По умолчанию", "1": "2x", "2": "4x", "3": "8x", "5": "16x"}
    },
    ("Renderer", "anti_aliasing"): {
        "name": "Метод сглаживания",
        "desc": "устранение ступенчатости и неровностей краев 3D-геометрии",
        "values": {"0": "Отключено", "1": "FXAA", "2": "SMAA"}
    },
    ("Renderer", "scaling_filter"): {
        "name": "Фильтр масштабирования",
        "desc": "алгоритм интерполяции и повышения четкости картинки",
        "values": {"1": "Билинейный", "5": "AMD FidelityFX Super Resolution"}
    },
    ("Renderer", "fsr_sharpening_slider"): {
        "name": "Резкость FSR",
        "desc": "уровень резкости алгоритма AMD FSR для четкости деталей",
        "values": {}
    },
    ("System", "use_docked_mode"): {
        "name": "Режим док-станции",
        "desc": "переключение разрешения и графического профиля консоли",
        "values": {"0": "Портативный", "1": "В док-станции"}
    },
}

PRESETS = {
    "low": {
        "name": "Низкий / Энергосбережение (Low / Eco)",
        "description": "Оптимизировано для слабых ПК, встроенной графики и экономии батареи (0.75X, Быстрый ГПУ, BC3 сжатие)",
        "settings": {
            ("Renderer", "resolution_setup"): "0",
            ("Renderer", "gpu_accuracy"): "0",
            ("Renderer", "astc_recompression"): "2",
            ("Renderer", "astc_decode_mode"): "0",
            ("Renderer", "use_asynchronous_shaders"): "true",
            ("Renderer", "async_presentation"): "true",
            ("Renderer", "use_reactive_flushing"): "false",
            ("Renderer", "sync_memory_operations"): "false",
            ("Renderer", "use_fast_gpu_time"): "true",
            ("Renderer", "eco_frame_pacing"): "true",
            ("Renderer", "max_anisotropy"): "0",
            ("Renderer", "anti_aliasing"): "0",
            ("Renderer", "scaling_filter"): "1",
            ("Cpu", "cpu_accuracy"): "0",
            ("Cpu", "cpuopt_fastmem"): "true",
            ("Cpu", "cpuopt_ignore_memory_aborts"): "true",
            ("System", "use_docked_mode"): "0",
        }
    },
    "balanced": {
        "name": "Сбалансированный (Balanced - 60 FPS)",
        "description": "Рекомендуемый общий профиль для плавной игры в 1080p при оптимальном балансе качества и скорости (1X, Быстрый ГПУ, FSR)",
        "settings": {
            ("Renderer", "resolution_setup"): "2",
            ("Renderer", "gpu_accuracy"): "0",
            ("Renderer", "astc_recompression"): "0",
            ("Renderer", "astc_decode_mode"): "0",
            ("Renderer", "use_asynchronous_shaders"): "true",
            ("Renderer", "async_presentation"): "true",
            ("Renderer", "use_reactive_flushing"): "false",
            ("Renderer", "sync_memory_operations"): "false",
            ("Renderer", "use_fast_gpu_time"): "true",
            ("Renderer", "eco_frame_pacing"): "false",
            ("Renderer", "max_anisotropy"): "0",
            ("Renderer", "anti_aliasing"): "1",
            ("Renderer", "scaling_filter"): "5",
            ("Renderer", "fsr_sharpening_slider"): "80",
            ("Cpu", "cpu_accuracy"): "0",
            ("Cpu", "cpuopt_fastmem"): "true",
            ("Cpu", "cpuopt_ignore_memory_aborts"): "true",
            ("System", "use_docked_mode"): "1",
        }
    },
    "high": {
        "name": "Максимальное качество (High Quality / Ultra)",
        "description": "Максимальная детализация для мощных ПК и дискретных видеокарт (2X 1440p/4K, Высокая точность ГПУ, SMAA, 16x Aniso)",
        "settings": {
            ("Renderer", "resolution_setup"): "5",
            ("Renderer", "gpu_accuracy"): "1",
            ("Renderer", "astc_recompression"): "0",
            ("Renderer", "astc_decode_mode"): "0",
            ("Renderer", "use_asynchronous_shaders"): "true",
            ("Renderer", "async_presentation"): "true",
            ("Renderer", "use_reactive_flushing"): "true",
            ("Renderer", "sync_memory_operations"): "true",
            ("Renderer", "use_fast_gpu_time"): "true",
            ("Renderer", "eco_frame_pacing"): "false",
            ("Renderer", "max_anisotropy"): "5",
            ("Renderer", "anti_aliasing"): "2",
            ("Renderer", "scaling_filter"): "5",
            ("Renderer", "fsr_sharpening_slider"): "85",
            ("Cpu", "cpu_accuracy"): "0",
            ("Cpu", "cpuopt_fastmem"): "true",
            ("Cpu", "cpuopt_ignore_memory_aborts"): "true",
            ("System", "use_docked_mode"): "1",
        }
    }
}

def format_param_entry(sec, key, val):
    meta = SETTINGS_METADATA.get((sec, key))
    if meta:
        param_name = meta["name"]
        val_name = meta["values"].get(val, val)
        reason = meta["desc"]
        return f"{param_name}: {val_name} ({reason})"
    return f"{sec}\\{key}: {val}"

def find_config_file():
    candidates = [
        Path("user/config/qt-config.ini"),
        Path("../user/config/qt-config.ini"),
        Path(os.path.expandvars(r"%APPDATA%\storm_switch\config\qt-config.ini")),
        Path(os.path.expandvars(r"%APPDATA%\yuzu\config\qt-config.ini")),
        Path(os.path.expandvars(r"%LOCALAPPDATA%\storm_switch\config\qt-config.ini")),
        Path(os.path.expanduser("~/.config/storm_switch/qt-config.ini")),
        Path(os.path.expanduser("~/.config/yuzu/qt-config.ini")),
    ]
    for c in candidates:
        if c.is_file():
            return c.resolve()
    # Fallback to first location if nothing exists yet
    return candidates[0].resolve()

def load_ini(path):
    sections = {}
    current_section = None
    if not path.is_file():
        return sections
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line_str = line.strip()
            if not line_str or line_str.startswith(";") or line_str.startswith("#"):
                continue
            if line_str.startswith("[") and line_str.endswith("]"):
                current_section = line_str[1:-1]
                if current_section not in sections:
                    sections[current_section] = {}
            elif "=" in line_str and current_section is not None:
                k, v = line_str.split("=", 1)
                sections[current_section][k.strip()] = v.strip()
    return sections

def save_ini(path, sections):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        for sec, kvs in sections.items():
            f.write(f"[{sec}]\n")
            for k, v in kvs.items():
                f.write(f"{k}={v}\n")
            f.write("\n")

def apply_preset(config_path, preset_key):
    preset = PRESETS[preset_key]
    sections = load_ini(config_path)

    # Backup existing configuration
    if config_path.is_file():
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = config_path.with_suffix(f".ini.bak_{timestamp}")
        shutil.copy2(config_path, backup_path)
        print(f"[+] Создана резервная копия конфига: {backup_path}")

    print(f"\nПрименение ОБЩИХ параметров авто-настройки '{preset['name']}':")
    changes_count = 0
    for (sec, key), val in preset["settings"].items():
        if sec not in sections:
            sections[sec] = {}
        old_val = sections[sec].get(key, "<не задано>")
        sections[sec][key] = val
        sections[sec][f"{key}\\default"] = "false"
        sections[sec][f"{key}\\use_global"] = "true"
        changes_count += 1
        entry_text = format_param_entry(sec, key, val)
        print(f"  ✓ {entry_text}")

    save_ini(config_path, sections)
    print(f"\n[OK] Пресет '{preset['name']}' успешно сохранен! Обновлено общих параметров: {changes_count}")
    print(f"Общая конфигурация записана в: {config_path}\n")

def main():
    if sys.platform == "win32":
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass

    parser = argparse.ArgumentParser(description="STORM SWITCH Auto-Tuner / Общие авто-настройки производительности")
    parser.add_argument("-p", "--preset", choices=["low", "balanced", "high"], help="Ключ профиля: low, balanced, high")
    parser.add_argument("-c", "--config", help="Пользовательский путь к qt-config.ini")
    parser.add_argument("-l", "--list", action="store_true", help="Показать доступные профили и их параметры")
    args = parser.parse_args()

    config_path = Path(args.config).resolve() if args.config else find_config_file()

    print("=" * 65)
    print("      STORM SWITCH AUTO-TUNER И ОПТИМИЗАЦИЯ ПРОИЗВОДИТЕЛЬНОСТИ")
    print("=" * 65)
    print(f"Целевой файл общей конфигурации: {config_path}")

    if args.list:
        print("\nДоступные общие профили оптимизации:")
        for k, p in PRESETS.items():
            print(f"\n[{k.upper()}] - {p['name']}")
            print(f"  Описание: {p['description']}")
            print("  Параметры:")
            for (sec, param), val in p["settings"].items():
                entry_text = format_param_entry(sec, param, val)
                print(f"    ✓ {entry_text}")
        return

    selected = args.preset
    if not selected:
        print("\nВыберите профиль производительности (меняет ОБЩИЕ параметры):")
        print("  1) LOW      - Низкий / Энергосбережение (0.75X, Быстрый ГПУ, BC3 сжатие)")
        print("  2) BALANCED - Сбалансированный (1X, Быстрый ГПУ, FSR, 60 FPS)")
        print("  3) HIGH     - Высокое качество (2X, Высокая точность ГПУ, SMAA, 16x Aniso)")
        print("  q) Выход")
        try:
            choice = input("\nВведите номер профиля (1-3) [по умолчанию 2]: ").strip()
        except (KeyboardInterrupt, EOFError):
            print("\nОтменено.")
            return

        if choice in ["1", "low", "l"]:
            selected = "low"
        elif choice in ["3", "high", "h"]:
            selected = "high"
        elif choice in ["q", "quit", "exit"]:
            print("Выход.")
            return
        else:
            selected = "balanced"

    apply_preset(config_path, selected)

if __name__ == "__main__":
    main()
