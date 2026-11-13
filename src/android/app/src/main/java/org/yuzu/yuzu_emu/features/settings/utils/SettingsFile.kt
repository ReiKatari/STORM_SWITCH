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
                val rBloom = text.replace(Regex("fix_bloom_effects\\s*=\\s*false", RegexOption.IGNORE_CASE), "fix_bloom_effects = true")
                if (rBloom != text) { text = rBloom; modified = true }
                val rReadback = text.replace(Regex("enable_gpu_buffer_readback\\s*=\\s*true", RegexOption.IGNORE_CASE), "enable_gpu_buffer_readback = false")
                if (rReadback != text) { text = rReadback; modified = true }
            }

            // Diablo II: Resurrected (0100726014352*, 01002A801458A*, 0100916014D8C*)
            if (cleanProg.startsWith("0100726014352") ||
                cleanProg.startsWith("01002A801458A") ||
                cleanProg.startsWith("0100916014D8C")
            ) {
                val r1 = text.replace(Regex("memory_layout_mode\\s*=\\s*[12]"), "memory_layout_mode = 0")
                if (r1 != text) { text = r1; modified = true }
                val r1Global = text.replace(Regex("memory_layout_mode\\\\use_global\\s*=\\s*false", RegexOption.IGNORE_CASE), "memory_layout_mode\\use_global = true")
                if (r1Global != text) { text = r1Global; modified = true }
                val rDyna = text.replace(Regex("(?m)^\\s*dyna_state\\s*=\\s*0\\s*$"), "")
                if (rDyna != text) { text = rDyna; modified = true }
                val rDynaGlobal = text.replace(Regex("dyna_state\\\\use_global\\s*=\\s*false", RegexOption.IGNORE_CASE), "dyna_state\\use_global = true")
                if (rDynaGlobal != text) { text = rDynaGlobal; modified = true }
                val r2 = text.replace(Regex("gpu_accuracy\\s*=\\s*[01]"), "gpu_accuracy = 2")
                if (r2 != text) { text = r2; modified = true }
                val r3 = text.replace(Regex("vram_budget_governor\\s*=\\s*true", RegexOption.IGNORE_CASE), "vram_budget_governor = false")
                if (r3 != text) { text = r3; modified = true }
                val r4 = text.replace(Regex("early_release_fences\\s*=\\s*true", RegexOption.IGNORE_CASE), "early_release_fences = false")
                if (r4 != text) { text = r4; modified = true }
                val r5 = text.replace(Regex("enable_frame_skipping\\s*=\\s*true", RegexOption.IGNORE_CASE), "enable_frame_skipping = false")
                if (r5 != text) { text = r5; modified = true }
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
        val PREF_SANITIZED_KEY = "storm_config_sanitized_17_9_1_tot"
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
                                if (parts.size == 2 && (parts[1].trim() == "0" || parts[1].trim() == "3")) {
                                    newLines.add("dyna_state=1")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("dyna_state\\default", ignoreCase = true)) {
                                hasDynaDefault = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("true", ignoreCase = true)) {
                                    newLines.add("dyna_state\\default=false")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("sync_memory_operations", ignoreCase = true) && !trimmed.startsWith("sync_memory_operations\\default", ignoreCase = true)) {
                                hasSyncMem = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("false", ignoreCase = true)) {
                                    newLines.add("sync_memory_operations=true")
                                    modified = true
                                    continue
                                }
                            } else if (trimmed.startsWith("sync_memory_operations\\default", ignoreCase = true)) {
                                hasSyncMemDefault = true
                                val parts = trimmed.split("=", limit = 2)
                                if (parts.size == 2 && parts[1].trim().equals("true", ignoreCase = true)) {
                                    newLines.add("sync_memory_operations\\default=false")
                                    modified = true
                                    continue
                                }
                            }
                            newLines.add(line)
                        }

                        if (!hasDynaState) {
                            newLines.add("dyna_state=1")
                            newLines.add("dyna_state\\default=false")
                            modified = true
                        } else if (!hasDynaDefault) {
                            newLines.add("dyna_state\\default=false")
                            modified = true
                        }

                        if (!hasSyncMem) {
                            newLines.add("sync_memory_operations=true")
                            newLines.add("sync_memory_operations\\default=false")
                            modified = true
                        } else if (!hasSyncMemDefault) {
                            newLines.add("sync_memory_operations\\default=false")
                            modified = true
                        }

                        if (modified) {
                            globalConfigFile.writeText(newLines.joinToString("\n"), Charsets.UTF_8)
                            Log.info("[SettingsFile] Sanitized global config.ini: dyna_state=1, sync_memory_operations=true")
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
                                val content = customFile.readText(Charsets.UTF_8)
                                if (content.contains("STORM_AUTO_GAME_FIX_TEMPORARY", ignoreCase = true)) {
                                    customFile.delete()
                                    Log.info("[SettingsFile] Deleted temporary per-game fix profile for $nameLower to regenerate clean 14.4.0 settings")
                                } else {
                                    customFile.delete()
                                    Log.info("[SettingsFile] Reset per-game config for $nameLower to clean profile")
                                }
                            } catch (e: Exception) {
                                Log.warning("[SettingsFile] Failed to sanitize target game profile $nameLower: ${e.message}")
                            }
                        }
                    }
                }
            }

            prefs.edit().putBoolean(PREF_SANITIZED_KEY, true).apply()
            Log.info("[SettingsFile] Successfully completed storm_config_sanitized_17_9_0 migration")
        } catch (e: Exception) {
            Log.warning("[SettingsFile] Migration storm_config_sanitized_17_9_0 failed: ${e.message}")
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
