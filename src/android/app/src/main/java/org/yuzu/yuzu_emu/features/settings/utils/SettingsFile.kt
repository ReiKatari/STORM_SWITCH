// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.features.settings.utils

import android.net.Uri
import org.yuzu.yuzu_emu.model.Game
import java.io.*
import org.yuzu.yuzu_emu.utils.DirectoryInitialization
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.NativeConfig

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

            // Diablo II: Resurrected (0100726014352*, 01002A801458A*, 0100916014D8C*)
            if (cleanProg.startsWith("0100726014352") ||
                cleanProg.startsWith("01002A801458A") ||
                cleanProg.startsWith("0100916014D8C")
            ) {
                val r1 = text.replace(Regex("memory_layout_mode\\s*=\\s*[01]"), "memory_layout_mode = 2")
                if (r1 != text) { text = r1; modified = true }
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

                val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(
                    org.yuzu.yuzu_emu.YuzuApplication.appContext
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

    fun sanitizeMarioWonderConfig(file: File) {
        // Backwards compatibility stub
    }

    fun loadCustomConfig(game: Game) {
        val customFile = getCustomSettingsFile(game)
        sanitizeKnownBrokenCustomConfigs(game, customFile)
        val fileName = customFile.nameWithoutExtension.ifEmpty {
            try { FileUtil.getFilename(Uri.parse(game.path)) } catch (_: Exception) { "" }
        }
        val programIdArg = game.programIdHex.trim().ifEmpty { game.programId }
        NativeConfig.initializePerGameConfig(programIdArg, fileName)
    }
}
