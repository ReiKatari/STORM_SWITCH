// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project

// SPDX-License-Identifier: GPL-3.0-or-later



package org.yuzu.yuzu_emu.features.settings.utils



import android.content.Context

import android.content.Intent

import androidx.core.content.FileProvider

import org.json.JSONObject

import org.yuzu.yuzu_emu.R

import org.yuzu.yuzu_emu.YuzuApplication

import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting

import org.yuzu.yuzu_emu.features.settings.model.IntSetting

import org.yuzu.yuzu_emu.features.settings.model.ShortSetting

import org.yuzu.yuzu_emu.utils.DirectoryInitialization

import org.yuzu.yuzu_emu.utils.Log

import org.yuzu.yuzu_emu.utils.NativeConfig

import org.yuzu.yuzu_emu.utils.StormHardwareCalibrator

import java.io.File

import java.io.InputStream



data class SettingsProfile(

    val id: String,

    var name: String,

    var description: String,

    val isPreset: Boolean = false,

    val timestamp: Long = System.currentTimeMillis(),

    val file: File? = null,

    val tags: List<String> = emptyList()

)



object SettingsProfileManager {

    private const val PROFILES_DIR_NAME = "profiles"

    private const val FILE_EXTENSION = ".stormprofile"



    const val PRESET_FAST_ID = "preset_fast"

    const val PRESET_BALANCED_ID = "preset_balanced"

    const val PRESET_QUALITY_ID = "preset_quality"

    const val PRESET_ECO_ID = "preset_eco"



    private val trackedBooleanSettings = listOf(

        BooleanSetting.RENDERER_USE_SPEED_LIMIT,

        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS,

        BooleanSetting.RENDERER_REACTIVE_FLUSHING,

        BooleanSetting.VULKAN_PIPELINE_CACHE,

        BooleanSetting.VRAM_GARBAGE_COLLECTION,

        BooleanSetting.FASTMEM,

        BooleanSetting.FASTMEM_EXCLUSIVES,

        BooleanSetting.USE_DOCKED_MODE,

        BooleanSetting.AIRPLANE_MODE,

        BooleanSetting.ECO_THERMAL_MODE,

        BooleanSetting.ECO_FRAME_PACING,

        BooleanSetting.SMART_SHADER_THROTTLE,

        BooleanSetting.CPU_AFFINITY_PINNING,

        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE,

        BooleanSetting.RENDERER_FORCE_MAX_CLOCK,

        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION,

        BooleanSetting.RENDERER_ASYNC_PRESENTATION,

        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES,

        BooleanSetting.ENABLE_FRAME_SKIPPING,

        BooleanSetting.ENABLE_BUFFER_HISTORY,

        BooleanSetting.USE_OPTIMIZED_VERTEX_BUFFERS,

        BooleanSetting.SYNC_MEMORY_OPERATIONS,

        BooleanSetting.RENDERER_FRAME_GEN

    )



    private val trackedIntSettings = listOf(

        IntSetting.RENDERER_BACKEND,

        IntSetting.RENDERER_ACCURACY,

        IntSetting.RENDERER_RESOLUTION,

        IntSetting.RENDERER_SCALING_FILTER,

        IntSetting.RENDERER_ANTI_ALIASING,

        IntSetting.RENDERER_NVDEC_EMULATION,

        IntSetting.RENDERER_ASTC_DECODE_METHOD,

        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT,

        IntSetting.ASTC_RECOMPRESSION,

        IntSetting.RENDERER_DYNA_STATE,

        IntSetting.RENDERER_VRAM_USAGE_MODE,

        IntSetting.RENDERER_VSYNC,

        IntSetting.RENDERER_ASPECT_RATIO,

        IntSetting.DMA_ACCURACY,

        IntSetting.GPU_FENCE_BEHAVIOR,

        IntSetting.FRAME_PACING_MODE,

        IntSetting.FSR_SHARPENING_SLIDER,

        IntSetting.MAX_ANISOTROPY,

        IntSetting.RENDERER_SAMPLE_SHADING,

        IntSetting.CPU_ACCURACY,

        IntSetting.CPU_BACKEND,

        IntSetting.MEMORY_LAYOUT,

        IntSetting.FAST_GPU_TIME,

        IntSetting.FAST_CPU_TIME,

        IntSetting.AUDIO_OUTPUT_ENGINE,

        IntSetting.ANDROID_PIPELINE_WORKERS

    )



    private val trackedShortSettings = listOf(

        ShortSetting.RENDERER_SPEED_LIMIT

    )



    fun getProfilesDirectory(): File {

        val dir = File(DirectoryInitialization.userDirectory, PROFILES_DIR_NAME)

        if (!dir.exists()) {

            dir.mkdirs()

        }

        return dir

    }



    /**

     * 4 Always-Available Built-In Presets (Group 1: БАЗОВЫЕ ПРЕСЕТЫ STORM)

     */

    fun getBuiltInPresets(context: Context? = null): List<SettingsProfile> {

        val ctx = context ?: YuzuApplication.getLocalizedContext()

        return listOf(

            SettingsProfile(

                id = PRESET_FAST_ID,

                name = runCatching { ctx.getString(R.string.storm_prof_preset_fast_name) }.getOrDefault("Скорость и 60 FPS"),

                description = runCatching { ctx.getString(R.string.storm_prof_preset_fast_desc) }.getOrDefault("Максимальная частота кадров, пониженное разрешение и агрессивные оптимизации рендеринга."),

                isPreset = true,

                tags = listOf("0.75X", "Асинхр. шейдеры", "GPU: Нормально", "ОЗУ: 4 ГБ")

            ),

            SettingsProfile(

                id = PRESET_BALANCED_ID,

                name = runCatching { ctx.getString(R.string.storm_prof_preset_balanced_name) }.getOrDefault("Баланс (Рекомендуется)"),

                description = runCatching { ctx.getString(R.string.storm_prof_preset_balanced_desc) }.getOrDefault("Оптимальное сочетание плавности 30–60 FPS, качества картинки 1.0X и стабильности."),

                isPreset = true,

                tags = listOf("1.0X", "Асинхр. шейдеры", "GPU: Нормально", "FSR 80%")

            ),

            SettingsProfile(

                id = PRESET_QUALITY_ID,

                name = runCatching { ctx.getString(R.string.storm_prof_preset_quality_name) }.getOrDefault("Высокая точность графики"),

                description = runCatching { ctx.getString(R.string.storm_prof_preset_quality_desc) }.getOrDefault("Высокое разрешение до 1.5X, безопасная точность DMA, сглаживание и чистые текстуры."),

                isPreset = true,

                tags = listOf("1.5X / 1.0X", "SMAA / FXAA", "GPU: Высокая", "Safe DMA")

            ),

            SettingsProfile(

                id = PRESET_ECO_ID,

                name = runCatching { ctx.getString(R.string.storm_prof_preset_eco_name) }.getOrDefault("Эко и охлаждение"),

                description = runCatching { ctx.getString(R.string.storm_prof_preset_eco_desc) }.getOrDefault("Снижение нагрева и расхода батареи, умный термопакет, умеренное энергопотребление."),

                isPreset = true,

                tags = listOf("0.75X", "Эко-термопакет", "Ограничение нагрева", "ОЗУ: 4 ГБ")

            )

        )

    }



    /**

     * User-created Custom Profiles (Group 2: МОИ СОХРАНЁННЫЕ ПРОФИЛИ)

     */

    fun getCustomProfiles(): List<SettingsProfile> {

        val dir = getProfilesDirectory()

        val files = dir.listFiles { f -> f.isFile && f.name.endsWith(FILE_EXTENSION) } ?: return emptyList()

        val list = mutableListOf<SettingsProfile>()

        for (file in files) {

            try {

                val content = file.readText().trim()

                if (content.isEmpty()) continue

                val json = JSONObject(content)

                val id = json.optString("id", file.nameWithoutExtension)

                // Filter out any mock preset files from custom list

                if (id.startsWith("preset_") || id.startsWith("profile_default_")) continue



                val rawName = json.optString("name", file.nameWithoutExtension)

                val rawDesc = json.optString("description", "")

                val ts = json.optLong("timestamp", file.lastModified())

                val tags = extractTagsFromProfile(json)



                list.add(

                    SettingsProfile(

                        id = id,

                        name = rawName,

                        description = rawDesc,

                        isPreset = false,

                        timestamp = ts,

                        file = file,

                        tags = tags

                    )

                )

            } catch (e: Exception) {

                Log.error("[SettingsProfileManager] Error reading profile ${file.name}: ${e.message}")

            }

        }

        list.sortByDescending { it.timestamp }

        return list

    }



    /**

     * All Profiles: Built-in Presets first, then Custom Profiles.

     */

    fun getAllProfiles(context: Context? = null): List<SettingsProfile> {

        return getBuiltInPresets(context) + getCustomProfiles()

    }



    fun extractTagsFromProfile(json: JSONObject): List<String> {

        val tags = mutableListOf<String>()

        val integers = json.optJSONObject("integers")

        val booleans = json.optJSONObject("booleans")



        // Resolution

        if (integers != null && integers.has("resolution_setup")) {

            val res = integers.getInt("resolution_setup")

            val resStr = when (res) {

                0 -> "0.25X"

                1 -> "0.5X"

                2 -> "0.75X"

                3 -> "1.0X"

                4 -> "1.25X"

                5 -> "1.5X"

                6 -> "2.0X"

                7 -> "3.0X"

                8 -> "4.0X"

                else -> "${res}X"

            }

            tags.add("Разр: $resStr")

        }



        // Shaders

        if (booleans != null && booleans.has("use_asynchronous_shaders")) {

            val async = booleans.getBoolean("use_asynchronous_shaders")

            tags.add(if (async) "Шейдеры: Асинхр." else "Шейдеры: Синхр.")

        }



        // GPU Accuracy

        if (integers != null && integers.has("gpu_accuracy")) {

            val acc = integers.getInt("gpu_accuracy")

            val accStr = when (acc) {

                0 -> "GPU: Нормально"

                1 -> "GPU: Высокая"

                2 -> "GPU: Экстрим"

                else -> "GPU: Авто"

            }

            tags.add(accStr)

        }



        // Memory layout

        if (integers != null && integers.has("memory_layout_mode")) {

            val mem = integers.getInt("memory_layout_mode")

            val memStr = when (mem) {

                0 -> "ОЗУ: 4 ГБ"

                1 -> "ОЗУ: 6 ГБ"

                2 -> "ОЗУ: 8 ГБ"

                else -> "ОЗУ: 4 ГБ"

            }

            tags.add(memStr)

        }



        if (tags.isEmpty()) {

            tags.add("Пользовательский")

        }

        return tags

    }



    fun saveCurrentSettingsAsProfile(name: String, description: String = ""): SettingsProfile? {

        try {

            val safeName = name.trim().ifEmpty { "Profile_" + System.currentTimeMillis() }

            val id = "profile_" + System.currentTimeMillis()

            val file = File(getProfilesDirectory(), "$id$FILE_EXTENSION")



            val json = JSONObject()

            json.put("id", id)

            json.put("name", safeName)

            json.put("description", description.trim())

            json.put("version", "17.1.0")

            json.put("timestamp", System.currentTimeMillis())



            val needsGlobal = !NativeConfig.isPerGameConfigLoaded()



            val boolObj = JSONObject()

            for (setting in trackedBooleanSettings) {

                try {

                    boolObj.put(setting.key, setting.getBoolean(needsGlobal))

                } catch (e: Throwable) {

                    Log.warning("[SettingsProfileManager] Could not read boolean setting ${setting.key}: ${e.message}")

                }

            }

            json.put("booleans", boolObj)



            val intObj = JSONObject()

            for (setting in trackedIntSettings) {

                try {

                    intObj.put(setting.key, setting.getInt(needsGlobal))

                } catch (e: Throwable) {

                    Log.warning("[SettingsProfileManager] Could not read int setting ${setting.key}: ${e.message}")

                }

            }

            json.put("integers", intObj)



            val shortObj = JSONObject()

            for (setting in trackedShortSettings) {

                try {

                    shortObj.put(setting.key, setting.getShort(needsGlobal).toInt())

                } catch (e: Throwable) {

                    Log.warning("[SettingsProfileManager] Could not read short setting ${setting.key}: ${e.message}")

                }

            }

            json.put("shorts", shortObj)



            file.writeText(json.toString(2))

            Log.info("[SettingsProfileManager] Saved profile: $safeName to ${file.absolutePath}")

            val tags = extractTagsFromProfile(json)

            return SettingsProfile(id = id, name = safeName, description = description, isPreset = false, file = file, tags = tags)

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Failed to save profile: ${e.message}")

            return null

        }

    }



    fun duplicatePreset(profile: SettingsProfile, context: Context, customName: String? = null): SettingsProfile? {

        val targetName = customName?.trim()?.ifEmpty { null }

            ?: runCatching {

                String.format(context.getString(R.string.storm_prof_duplicate_name_fmt), profile.name)

            }.getOrDefault("${profile.name} (Копия)")



        // Apply preset settings to generate configuration

        applyProfile(profile, context)

        return saveCurrentSettingsAsProfile(targetName, profile.description)

    }



    fun applyProfile(profile: SettingsProfile, context: Context? = null): Boolean {

        try {

            if (profile.isPreset) {

                val ctx = context ?: YuzuApplication.getLocalizedContext()

                val stormPreset = when (profile.id) {

                    PRESET_FAST_ID -> StormHardwareCalibrator.StormPreset.FAST

                    PRESET_QUALITY_ID -> StormHardwareCalibrator.StormPreset.ACCURATE

                    PRESET_ECO_ID -> StormHardwareCalibrator.StormPreset.BATTERY

                    else -> StormHardwareCalibrator.StormPreset.NORMAL

                }

                StormHardwareCalibrator.applyPreset(stormPreset, ctx)

                if (org.yuzu.yuzu_emu.NativeLibrary.isRunning()) {

                    org.yuzu.yuzu_emu.NativeLibrary.applySettings()

                }

                Log.info("[SettingsProfileManager] Applied built-in preset: ${profile.name}")

                return true

            }



            val file = profile.file ?: return false

            if (!file.exists()) return false



            val json = JSONObject(file.readText())

            val booleans = json.optJSONObject("booleans")

            if (booleans != null) {

                for (setting in trackedBooleanSettings) {

                    try {

                        if (booleans.has(setting.key)) {

                            setting.setBoolean(booleans.getBoolean(setting.key))

                        }

                    } catch (e: Throwable) {

                        Log.warning("[SettingsProfileManager] Could not apply boolean setting ${setting.key}: ${e.message}")

                    }

                }

            }



            val integers = json.optJSONObject("integers")

            if (integers != null) {

                for (setting in trackedIntSettings) {

                    try {

                        if (integers.has(setting.key)) {

                            setting.setInt(integers.getInt(setting.key))

                        }

                    } catch (e: Throwable) {

                        Log.warning("[SettingsProfileManager] Could not apply int setting ${setting.key}: ${e.message}")

                    }

                }

            }



            val shorts = json.optJSONObject("shorts")

            if (shorts != null) {

                for (setting in trackedShortSettings) {

                    try {

                        if (shorts.has(setting.key)) {

                            setting.setShort(shorts.getInt(setting.key).toShort())

                        }

                    } catch (e: Throwable) {

                        Log.warning("[SettingsProfileManager] Could not apply short setting ${setting.key}: ${e.message}")

                    }

                }

            }



            if (NativeConfig.isPerGameConfigLoaded()) {

                NativeConfig.savePerGameConfig()

            } else {

                NativeConfig.saveGlobalConfig()

            }

            if (org.yuzu.yuzu_emu.NativeLibrary.isRunning()) {

                org.yuzu.yuzu_emu.NativeLibrary.applySettings()

            }

            Log.info("[SettingsProfileManager] Applied custom profile: ${profile.name}")

            return true

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Failed to apply profile: ${e.message}")

            return false

        }

    }



    fun renameProfile(profile: SettingsProfile, newName: String): Boolean {

        if (profile.isPreset) return false

        val file = profile.file ?: return false

        val cleanName = newName.trim()

        if (cleanName.isEmpty()) return false

        try {

            val json = JSONObject(file.readText())

            json.put("name", cleanName)

            file.writeText(json.toString(2))

            profile.name = cleanName

            Log.info("[SettingsProfileManager] Renamed profile to: $cleanName")

            return true

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Error renaming profile: ${e.message}")

            return false

        }

    }



    fun deleteProfile(profile: SettingsProfile): Boolean {

        if (profile.isPreset) return false

        val file = profile.file ?: return false

        return try {

            val deleted = file.delete()

            Log.info("[SettingsProfileManager] Deleted profile: ${profile.name} (success=$deleted)")

            deleted

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Error deleting profile: ${e.message}")

            false

        }

    }



    fun shareProfile(context: Context, profile: SettingsProfile) {

        val file = profile.file ?: return

        if (!file.exists()) return



        try {

            val uri = FileProvider.getUriForFile(

                context,

                context.packageName + ".provider",

                file

            )

            val intent = Intent(Intent.ACTION_SEND).apply {

                type = "application/octet-stream"

                putExtra(Intent.EXTRA_STREAM, uri)

                putExtra(Intent.EXTRA_SUBJECT, "STORM SWITCH Profile: ${profile.name}")

                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)

            }

            context.startActivity(Intent.createChooser(intent, context.getString(R.string.profile_share_title)))

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Error sharing profile: ${e.message}")

        }

    }



    fun importProfileFromStream(stream: InputStream, originalFileName: String? = null): SettingsProfile? {

        try {

            val text = stream.bufferedReader().use { it.readText() }

            val json = JSONObject(text)

            val name = json.optString("name", originalFileName?.removeSuffix(FILE_EXTENSION) ?: "Imported_Profile")

            val desc = json.optString("description", "")

            val id = "imported_" + System.currentTimeMillis()

            val targetFile = File(getProfilesDirectory(), "$id$FILE_EXTENSION")



            json.put("id", id)

            targetFile.writeText(json.toString(2))

            Log.info("[SettingsProfileManager] Successfully imported profile: $name")

            val tags = extractTagsFromProfile(json)

            return SettingsProfile(id = id, name = name, description = desc, isPreset = false, file = targetFile, tags = tags)

        } catch (e: Exception) {

            Log.error("[SettingsProfileManager] Failed to import profile: ${e.message}")

            return null

        }

    }

}
