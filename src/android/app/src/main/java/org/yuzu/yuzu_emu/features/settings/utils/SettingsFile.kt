// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.features.settings.utils

import android.content.Context
import android.net.Uri
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import java.io.File

/**
 * Contains static methods for interacting with .ini files in which settings are stored.
 */
object SettingsFile {
    const val FILE_NAME_CONFIG = "config.ini"

    fun getSettingsFile(fileName: String): File =
        File(DirectoryInitialization.userDirectory + "/config/" + fileName)

    fun getCustomSettingsFile(game: Game): File {
        val configDir = File(DirectoryInitialization.userDirectory, "config/custom")
        val cleanProgHex = game.programIdHex.trim().uppercase(java.util.Locale.ROOT)
        val settingsName = game.settingsName.trim()
        val fileName = try { FileUtil.getFilename(Uri.parse(game.path)) } catch (_: Exception) { "" }

        val byHex = File(configDir, "$cleanProgHex.ini")
        if (cleanProgHex.isNotEmpty() && cleanProgHex != "0" && cleanProgHex != "0000000000000000" && byHex.exists()) return byHex
        val bySettings = File(configDir, "$settingsName.ini")
        if (bySettings.exists()) return bySettings
        if (fileName.isNotEmpty()) {
            val byFileName = File(configDir, "$fileName.ini")
            if (byFileName.exists()) return byFileName
        }

        return if (cleanProgHex.isNotEmpty() && cleanProgHex != "0" && cleanProgHex != "0000000000000000") {
            byHex
        } else if (settingsName.isNotEmpty() && settingsName != "0000000000000000") {
            bySettings
        } else {
            File(configDir, "$fileName.ini")
        }
    }

    fun sanitizeKnownBrokenCustomConfigs(game: Game, file: File) {
        try {
            if (!file.exists() || file.length() == 0L) return
            val cleanProg = game.programIdHex.trim().uppercase()
            var text = file.readText(Charsets.UTF_8)
            var modified = false

            // Mario Wonder (010015100B514*)
            if (cleanProg.startsWith("010015100B514")) {
                val r1 = text.replace(Regex("barrier_feedback_loops\\s*=\\s*false", RegexOption.IGNORE_CASE), "barrier_feedback_loops = true")
                if (r1 != text) { text = r1; modified = true }
                val r2 = text.replace(Regex("early_release_fences\\s*=\\s*true", RegexOption.IGNORE_CASE), "early_release_fences = false")
                if (r2 != text) { text = r2; modified = true }
                val r3 = text.replace(Regex("enable_frame_skipping\\s*=\\s*true", RegexOption.IGNORE_CASE), "enable_frame_skipping = false")
                if (r3 != text) { text = r3; modified = true }
                val r4 = text.replace(Regex("cpuopt_ignore_memory_aborts\\s*=\\s*false", RegexOption.IGNORE_CASE), "cpuopt_ignore_memory_aborts = true")
                if (r4 != text) { text = r4; modified = true }
            }

            // Mario + Rabbids: Kingdom Battle (010067300059A*, 010067300B762*)
            if (cleanProg.startsWith("010067300059A") || cleanProg.startsWith("010067300B762")) {
                val rMem = text.replace(Regex("(?m)^\\s*memory_layout_mode\\s*=\\s*[12]\\s*$"), "memory_layout_mode=0")
                if (rMem != text) { text = rMem; modified = true }
                val rBloom = text.replace(Regex("(?m)^\\s*fix_bloom_effects\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "fix_bloom_effects=true")
                if (rBloom != text) { text = rBloom; modified = true }
                val rReadback = text.replace(Regex("(?m)^\\s*enable_gpu_buffer_readback\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "enable_gpu_buffer_readback=true")
                if (rReadback != text) { text = rReadback; modified = true }
                val rSync = text.replace(Regex("(?m)^\\s*sync_memory_operations\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "sync_memory_operations=true")
                if (rSync != text) { text = rSync; modified = true }
                val rDma = text.replace(Regex("(?m)^\\s*dma_accuracy\\s*=\\s*[012]\\s*$"), "dma_accuracy=3")
                if (rDma != text) { text = rDma; modified = true }
                val rGpu = text.replace(Regex("(?m)^\\s*gpu_accuracy\\s*=\\s*[013]\\s*$"), "gpu_accuracy=2")
                if (rGpu != text) { text = rGpu; modified = true }
                val rAsyncSh = text.replace(Regex("(?m)^\\s*use_asynchronous_shaders\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_asynchronous_shaders=false")
                if (rAsyncSh != text) { text = rAsyncSh; modified = true }
                val rAsyncP = text.replace(Regex("(?m)^\\s*async_presentation\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "async_presentation=false")
                if (rAsyncP != text) { text = rAsyncP; modified = true }
                val rFast = text.replace(Regex("(?m)^\\s*use_fast_gpu_time\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_fast_gpu_time=false")
                if (rFast != text) { text = rFast; modified = true }
                val rBfl = text.replace(Regex("(?m)^\\s*barrier_feedback_loops\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "barrier_feedback_loops=true")
                if (rBfl != text) { text = rBfl; modified = true }
                val rFlush = text.replace(Regex("(?m)^\\s*use_reactive_flushing\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "use_reactive_flushing=true")
                if (rFlush != text) { text = rFlush; modified = true }
                val rGc = text.replace(Regex("(?m)^\\s*vram_garbage_collection\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_garbage_collection=true")
                if (rGc != text) { text = rGc; modified = true }
                val rGov = text.replace(Regex("(?m)^\\s*vram_budget_governor\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_budget_governor=true")
                if (rGov != text) { text = rGov; modified = true }
                val rFences = text.replace(Regex("(?m)^\\s*early_release_fences\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "early_release_fences=false")
                if (rFences != text) { text = rFences; modified = true }
            }

            // Diablo II: Resurrected (0100726014352*, 01002A801458A*, 0100916014D8C*)
            if (cleanProg.startsWith("0100726014352") ||
                cleanProg.startsWith("01002A801458A") ||
                cleanProg.startsWith("0100916014D8C")
            ) {
                // Heal corrupted keys from earlier unescaped backslash replacement
                val rHealMem = text.replace(Regex("(?m)^memory_layout_modeuse_global\\s*=.*$"), Regex.escapeReplacement("memory_layout_mode\\use_global=false"))
                if (rHealMem != text) { text = rHealMem; modified = true }
                val rHealDyna = text.replace(Regex("(?m)^dyna_stateuse_global\\s*=.*$"), Regex.escapeReplacement("dyna_state\\use_global=false"))
                if (rHealDyna != text) { text = rHealDyna; modified = true }
                val r1 = text.replace(Regex("(?m)^\\s*memory_layout_mode\\s*=\\s*[02]\\s*$"), "memory_layout_mode=1")
                if (r1 != text) { text = r1; modified = true }
                val r2 = text.replace(Regex("(?m)^\\s*gpu_accuracy\\s*=\\s*[023]\\s*$"), "gpu_accuracy=1")
                if (r2 != text) { text = r2; modified = true }
                val rDyna = text.replace(Regex("(?m)^\\s*dyna_state\\s*=\\s*[023]\\s*$"), "dyna_state=1")
                if (rDyna != text) { text = rDyna; modified = true }
                val rBloom = text.replace(Regex("(?m)^\\s*fix_bloom_effects\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "fix_bloom_effects=false")
                if (rBloom != text) { text = rBloom; modified = true }
                val rAsyncSh = text.replace(Regex("(?m)^\\s*use_asynchronous_shaders\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "use_asynchronous_shaders=true")
                if (rAsyncSh != text) { text = rAsyncSh; modified = true }
                val rAsyncP = text.replace(Regex("(?m)^\\s*async_presentation\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "async_presentation=true")
                if (rAsyncP != text) { text = rAsyncP; modified = true }
                val rSync = text.replace(Regex("(?m)^\\s*sync_memory_operations\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "sync_memory_operations=true")
                if (rSync != text) { text = rSync; modified = true }
                val rReadback = text.replace(Regex("(?m)^\\s*enable_gpu_buffer_readback\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "enable_gpu_buffer_readback=false")
                if (rReadback != text) { text = rReadback; modified = true }
                val rDma = text.replace(Regex("(?m)^\\s*dma_accuracy\\s*=\\s*[012]\\s*$"), "dma_accuracy=3")
                if (rDma != text) { text = rDma; modified = true }
                val rBfl = text.replace(Regex("(?m)^\\s*barrier_feedback_loops\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "barrier_feedback_loops=false")
                if (rBfl != text) { text = rBfl; modified = true }
                val rFlush = text.replace(Regex("(?m)^\\s*use_reactive_flushing\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_reactive_flushing=false")
                if (rFlush != text) { text = rFlush; modified = true }
                val rFast = text.replace(Regex("(?m)^\\s*use_fast_gpu_time\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_fast_gpu_time=false")
                if (rFast != text) { text = rFast; modified = true }
                val rVkCache = text.replace(Regex("(?m)^\\s*use_vulkan_driver_pipeline_cache\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_vulkan_driver_pipeline_cache=false")
                if (rVkCache != text) { text = rVkCache; modified = true }
                val r3 = text.replace(Regex("(?m)^\\s*vram_budget_governor\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_budget_governor=true")
                if (r3 != text) { text = r3; modified = true }
                val rGc = text.replace(Regex("(?m)^\\s*vram_garbage_collection\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_garbage_collection=true")
                if (rGc != text) { text = rGc; modified = true }
                val r4 = text.replace(Regex("(?m)^\\s*early_release_fences\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "early_release_fences=false")
                if (r4 != text) { text = r4; modified = true }
                val r5 = text.replace(Regex("(?m)^\\s*enable_frame_skipping\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "enable_frame_skipping=false")
                if (r5 != text) { text = r5; modified = true }
            }

            // Absolum (0100A83022A22*)
            if (cleanProg.startsWith("0100A83022A22")) {
                val rMem = text.replace(Regex("(?m)^\\s*memory_layout_mode\\s*=\\s*[12]\\s*$"), "memory_layout_mode=0")
                if (rMem != text) { text = rMem; modified = true }
                val rGpu = text.replace(Regex("(?m)^\\s*gpu_accuracy\\s*=\\s*[013]\\s*$"), "gpu_accuracy=2")
                if (rGpu != text) { text = rGpu; modified = true }
                val rBloom = text.replace(Regex("(?m)^\\s*fix_bloom_effects\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "fix_bloom_effects=true")
                if (rBloom != text) { text = rBloom; modified = true }
                val rNvdec = text.replace(Regex("(?m)^\\s*nvdec_emulation\\s*=\\s*[01]\\s*$"), "nvdec_emulation=2")
                if (rNvdec != text) { text = rNvdec; modified = true }
                val rAsyncP = text.replace(Regex("(?m)^\\s*async_presentation\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "async_presentation=false")
                if (rAsyncP != text) { text = rAsyncP; modified = true }
                val rAsyncSh = text.replace(Regex("(?m)^\\s*use_asynchronous_shaders\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "use_asynchronous_shaders=true")
                if (rAsyncSh != text) { text = rAsyncSh; modified = true }
                val rFast = text.replace(Regex("(?m)^\\s*use_fast_gpu_time\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "use_fast_gpu_time=false")
                if (rFast != text) { text = rFast; modified = true }
                val rSync = text.replace(Regex("(?m)^\\s*sync_memory_operations\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "sync_memory_operations=true")
                if (rSync != text) { text = rSync; modified = true }
                val rReadback = text.replace(Regex("(?m)^\\s*enable_gpu_buffer_readback\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "enable_gpu_buffer_readback=true")
                if (rReadback != text) { text = rReadback; modified = true }
                val rFlush = text.replace(Regex("(?m)^\\s*use_reactive_flushing\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "use_reactive_flushing=true")
                if (rFlush != text) { text = rFlush; modified = true }
                val rDma = text.replace(Regex("(?m)^\\s*dma_accuracy\\s*=\\s*[012]\\s*$"), "dma_accuracy=3")
                if (rDma != text) { text = rDma; modified = true }
                val rBfl = text.replace(Regex("(?m)^\\s*barrier_feedback_loops\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "barrier_feedback_loops=true")
                if (rBfl != text) { text = rBfl; modified = true }
                val rGov = text.replace(Regex("(?m)^\\s*vram_budget_governor\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_budget_governor=true")
                if (rGov != text) { text = rGov; modified = true }
                val rGc = text.replace(Regex("(?m)^\\s*vram_garbage_collection\\s*=\\s*false\\s*$", RegexOption.IGNORE_CASE), "vram_garbage_collection=true")
                if (rGc != text) { text = rGc; modified = true }
                val rFences = text.replace(Regex("(?m)^\\s*early_release_fences\\s*=\\s*true\\s*$", RegexOption.IGNORE_CASE), "early_release_fences=false")
                if (rFences != text) { text = rFences; modified = true }
            }

            // Unravel Two (01008C3005A32*, 0100E5D00CC0C*, 010099900B22E*)
            if (cleanProg.startsWith("01008C3005A32") ||
                cleanProg.startsWith("0100E5D00CC0C") ||
                cleanProg.startsWith("010099900B22E")
            ) {
                val rFences = text.replace(Regex("early_release_fences\\s*=\\s*true", RegexOption.IGNORE_CASE), "early_release_fences = false")
                if (rFences != text) { text = rFences; modified = true }

                val prefs = PreferenceManager.getDefaultSharedPreferences(
                    YuzuApplication.appContext
                )
                val healKey = "storm_unravel2_1635_healed_$cleanProg"
                if (!prefs.getBoolean(healKey, false) && !text.contains("storm_unravel2_1635_healed=1")) {
                    val rCpu = text.replace(Regex("cpu_backend\\s*=\\s*0"), "cpu_backend = 1")
                    if (rCpu != text) { text = rCpu; modified = true }
                    val rNvdec = text.replace(Regex("nvdec_emulation\\s*=\\s*1"), "nvdec_emulation = 2")
                    if (rNvdec != text) { text = rNvdec; modified = true }
                    val rBfl = text.replace(Regex("barrier_feedback_loops\\s*=\\s*true", RegexOption.IGNORE_CASE), "barrier_feedback_loops = false")
                    if (rBfl != text) { text = rBfl; modified = true }
                    val rGpu = text.replace(Regex("gpu_accuracy\\s*=\\s*2"), "gpu_accuracy = 1")
                    if (rGpu != text) { text = rGpu; modified = true }
                    text = text.trimEnd() + "\n# storm_unravel2_1635_healed=1\n"
                    modified = true
                    prefs.edit().putBoolean(healKey, true).apply()
                }
            }

            // Switch 2 (0400*)
            if (cleanProg.startsWith("0400")) {
                val r1 = text.replace(Regex("cpu_backend\\s*=\\s*1"), "cpu_backend = 0")
                if (r1 != text) { text = r1; modified = true }
            }

            // One-time heal for pre-16.3.2 JNI enum virtual-base cast bug that poisoned
            // aspect_ratio\use_global=false + aspect_ratio=0 inside per-game .ini files.
            if (!text.contains("storm_jni_enum_healed=1")) {
                if (Regex("aspect_ratio\\\\use_global\\s*=\\s*false", RegexOption.IGNORE_CASE).containsMatchIn(text) &&
                    Regex("(?m)^\\s*aspect_ratio\\s*=\\s*0\\s*$").containsMatchIn(text)
                ) {
                    text = text.replace(
                        Regex("aspect_ratio\\\\use_global\\s*=\\s*false", RegexOption.IGNORE_CASE),
                        Regex.escapeReplacement("aspect_ratio\\use_global=true")
                    )
                }
                text = text.trimEnd() + "\n# storm_jni_enum_healed=1\n"
                modified = true
            }

            if (modified) {
                file.writeText(text, Charsets.UTF_8)
            }
        } catch (_: Exception) {}
    }

    fun sanitizePoisonedConfigs(context: Context = YuzuApplication.appContext) {
        val prefs = PreferenceManager.getDefaultSharedPreferences(context)
        val PREF_SANITIZED_KEY = "storm_config_sanitized_18_0_9"
        if (prefs.getBoolean(PREF_SANITIZED_KEY, false)) {
            return
        }

        try {
            val userDir = DirectoryInitialization.userDirectory ?: "/storage/emulated/0/STORM SWITCH"
            val configDirs = listOfNotNull(
                File(userDir, "config"),
                File("/storage/emulated/0/STORM SWITCH/config"),
                File("/sdcard/STORM SWITCH/config"),
                File(context.filesDir, "config")
            ).distinct()

            for (cfgDir in configDirs) {
                if (!cfgDir.exists() || !cfgDir.isDirectory) continue

                // 1. Global config.ini
                val globalConfigFile = File(cfgDir, FILE_NAME_CONFIG)
                if (globalConfigFile.exists() && globalConfigFile.isFile) {
                    try {
                        val lines = globalConfigFile.readLines(Charsets.UTF_8)
                        val newLines = mutableListOf<String>()
                        var modified = false
                        var hasDynaState = false
                        var hasDynaDefault = false
                        var hasSyncMem = false
                        var hasSyncMemDefault = false

                        for (line in lines) {
                            val trimmed = line.trim()
                            if (trimmed.startsWith("dyna_state", ignoreCase = true) && !trimmed.startsWith("dyna_state\\default", ignoreCase = true)) {
                                hasDynaState = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim() != "0") {
                                    newLines.add("dyna_state=0")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("dyna_state\\default", ignoreCase = true)) {
                                hasDynaDefault = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("false", ignoreCase = true)) {
                                    newLines.add("dyna_state\\default=true")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("sync_memory_operations", ignoreCase = true) && !trimmed.startsWith("sync_memory_operations\\default", ignoreCase = true)) {
                                hasSyncMem = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("true", ignoreCase = true)) {
                                    newLines.add("sync_memory_operations=false")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("sync_memory_operations\\default", ignoreCase = true)) {
                                hasSyncMemDefault = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("false", ignoreCase = true)) {
                                    newLines.add("sync_memory_operations\\default=true")
                                    modified = true
                                    continue
                                }
                            }
                            newLines.add(line)
                        }

                        if (!hasDynaState) {
                            newLines.add("dyna_state=0")
                            newLines.add("dyna_state\\default=true")
                            modified = true
                        } else if (!hasDynaDefault) {
                            newLines.add("dyna_state\\default=true")
                            modified = true
                        }

                        if (!hasSyncMem) {
                            newLines.add("sync_memory_operations=false")
                            newLines.add("sync_memory_operations\\default=true")
                            modified = true
                        } else if (!hasSyncMemDefault) {
                            newLines.add("sync_memory_operations\\default=true")
                            modified = true
                        }

                        if (modified) {
                            globalConfigFile.writeText(newLines.joinToString("\n"), Charsets.UTF_8)
                            Log.info("[SettingsFile] Sanitized global config.ini for 18.0.2: dyna_state=0, sync_memory_operations=false (bloom preserved)")
                        }
                    } catch (e: Exception) {
                        Log.warning("[SettingsFile] Failed to sanitize global config: ${e.message}")
                    }
                }

                // 2. Custom per-game config files
                val customDir = File(cfgDir, "custom")
                if (customDir.exists() && customDir.isDirectory) {
                    val customFiles = customDir.listFiles { f -> f.isFile && f.extension.equals("ini", ignoreCase = true) }
                    val targetGameIds = setOf(
                        "0100726014352000",
                        "01002a801458a000",
                        "0100916014d8c000",
                        "0100ad5013bb6000",
                        "010067300059a000",
                        "010067300b762000",
                        "0100a83022a22000"
                    )

                    customFiles?.forEach { customFile ->
                        val nameLower = customFile.nameWithoutExtension.lowercase()
                        val isTargetGame = targetGameIds.any { nameLower.contains(it) }

                        if (isTargetGame) {
                            try {
                                customFile.delete()
                                Log.info("[SettingsFile] Deleted stale per-game config $nameLower for clean 17.9.4 regeneration")
                            } catch (e: Exception) {
                                Log.warning("[SettingsFile] Failed to sanitize target game profile $nameLower: ${e.message}")
                            }
                        }
                    }
                }
            }

            try {
                org.yuzu.yuzu_emu.utils.GpuDriverHelper.clearShaderAndPipelineCaches(context)
                org.yuzu.yuzu_emu.utils.GpuDriverHelper.clearVulkanPipelineCaches(context)
                File(context.filesDir, ".drirc").delete()
                File(context.filesDir, "gpu_drivers").listFiles()?.forEach { sub ->
                    if (sub.isDirectory) {
                        File(sub, "per_game_drirc").deleteRecursively()
                    }
                }
            } catch (e: Throwable) {
                Log.warning("[SettingsFile] Cache/drirc cleanup during 18_0_3 migration failed: ${e.message}")
            }

            prefs.edit().putBoolean(PREF_SANITIZED_KEY, true).apply()
            Log.info("[SettingsFile] Successfully completed storm_config_sanitized_18_0_3 migration")
        } catch (e: Exception) {
            Log.warning("[SettingsFile] Migration storm_config_sanitized_18_0_3 failed: ${e.message}")
        }
    }

    fun sanitizeMarioWonderConfig(file: File) {
        // Backwards compatibility stub
    }

    fun loadCustomConfig(game: Game) {
        sanitizePoisonedConfigs()
        val customFile = getCustomSettingsFile(game)
        sanitizeKnownBrokenCustomConfigs(game, customFile)
        val fileName = customFile.nameWithoutExtension.ifEmpty {
            try { FileUtil.getFilename(Uri.parse(game.path)) } catch (_: Exception) { "" }
        }
        val programIdArg = game.programIdHex.trim().ifEmpty { game.programId }
        NativeConfig.initializePerGameConfig(programIdArg, fileName)
    }
}
