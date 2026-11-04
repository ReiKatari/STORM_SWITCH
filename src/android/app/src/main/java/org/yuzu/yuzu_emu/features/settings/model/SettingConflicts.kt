// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.features.settings.model

data class AndroidSettingConflict(
    val ruleId: String,
    val titleRu: String,
    val titleEn: String,
    val messageRu: String,
    val messageEn: String,
    val applyFix: () -> Unit
) {
    fun localizedTitle(isRu: Boolean): String = if (isRu) titleRu else titleEn
    fun localizedMessage(isRu: Boolean): String = if (isRu) messageRu else messageEn
}

object SettingConflicts {
    fun detectConflicts(isPerGame: Boolean = false): List<AndroidSettingConflict> {
        val list = mutableListOf<AndroidSettingConflict>()

        // 1. Async shaders without disk shader cache
        val asyncShaders = BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.getBoolean()
        val diskCache = BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.getBoolean()
        if (asyncShaders && !diskCache) {
            list.add(
                AndroidSettingConflict(
                    "async_disk_cache",
                    "Конфликт компиляции шейдеров",
                    "Shader Compilation Conflict",
                    "Асинхронные шейдеры включены без дискового кэша: микрофризы и пропадание текстур будут повторяться при каждом запуске.",
                    "Asynchronous shaders are enabled without disk shader cache: stutter and pop-in will recur on every game launch.",
                    applyFix = {
                        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
                    }
                )
            )
        }

        // 2. Frame generation with unlocked FPS or immediate vsync
        val frameGen = BooleanSetting.RENDERER_FRAME_GEN.getBoolean()
        val vsync = IntSetting.RENDERER_VSYNC.getInt()
        val speedLimit = BooleanSetting.RENDERER_USE_SPEED_LIMIT.getBoolean()
        if (frameGen && (vsync == 0 || !speedLimit)) {
            list.add(
                AndroidSettingConflict(
                    "frame_gen_pacing",
                    "Конфликт генерации кадров (LSFG)",
                    "Frame Generation Pacing Conflict",
                    "Генерация кадров активна с выключенным VSync или снятым лимитом FPS: нарушается расчет векторов движения.",
                    "Frame generation is active with VSync off or unlocked FPS: optical flow vector estimation becomes erratic.",
                    applyFix = {
                        IntSetting.RENDERER_VSYNC.setInt(2) // FIFO
                        BooleanSetting.RENDERER_USE_SPEED_LIMIT.setBoolean(true)
                    }
                )
            )
        }

        // 3. Unsafe CPU flags enabled when accuracy is not Unsafe
        val cpuAccuracy = IntSetting.CPU_ACCURACY.getInt()
        val unsafeMmu = BooleanSetting.CPUOPT_UNSAFE_HOST_MMU.getBoolean()
        if (cpuAccuracy != 2 && unsafeMmu) {
            list.add(
                AndroidSettingConflict(
                    "cpu_unsafe_flags",
                    "Игнорирование флагов процессора",
                    "CPU Unsafe Flags Ignored",
                    "Флаги «Небезопасно» включены, но точность ЦП не Unsafe: движок Dynarmic игнорирует их в режиме Авто/Точно.",
                    "Unsafe CPU flags are active but CPU accuracy is not Unsafe: Dynarmic ignores these flags in Auto/Accurate mode.",
                    applyFix = {
                        IntSetting.CPU_ACCURACY.setInt(2) // Unsafe
                    }
                )
            )
        }

        // 4. Resolution >= 3x on mobile device
        val resolution = IntSetting.RENDERER_RESOLUTION.getInt()
        if (resolution >= 7) { // 3x or higher
            list.add(
                AndroidSettingConflict(
                    "high_resolution_mobile",
                    "Высокая нагрузка по видеопамяти",
                    "Extreme Mobile VRAM Workload",
                    "Масштаб разрешения 3x и выше на мобильном устройстве приводит к перегреву, троттлингу и крашам по нехватке памяти.",
                    "Resolution scale 3x or higher on mobile devices causes overheating, thermal throttling, and out-of-memory crashes.",
                    applyFix = {
                        IntSetting.RENDERER_RESOLUTION.setInt(3) // 1x
                    }
                )
            )
        }

        return list
    }
}
