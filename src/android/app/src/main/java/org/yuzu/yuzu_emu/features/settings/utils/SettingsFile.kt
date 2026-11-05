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

    fun sanitizeMarioWonderConfig(file: File) {
        try {
            if (!file.exists() || file.length() == 0L) return
            var text = file.readText(Charsets.UTF_8)
            var modified = false
            // Force memory_layout_mode to 0 (4GB DRAM)
            if (text.contains("memory_layout_mode")) {
                val replaced = text.replace(Regex("memory_layout_mode\\s*=\\s*[1-9]"), "memory_layout_mode = 0")
                if (replaced != text) {
                    text = replaced
                    modified = true
                }
            }
            // Force barrier_feedback_loops to false
            if (text.contains("barrier_feedback_loops")) {
                val replaced = text.replace(Regex("barrier_feedback_loops\\s*=\\s*true", RegexOption.IGNORE_CASE), "barrier_feedback_loops = false")
                if (replaced != text) {
                    text = replaced
                    modified = true
                }
            }
            // Force cpuopt_ignore_memory_aborts to false so NCE/Dynarmic handles Wonder's page faults cleanly
            if (text.contains("cpuopt_ignore_memory_aborts")) {
                val replaced = text.replace(Regex("cpuopt_ignore_memory_aborts\\s*=\\s*true", RegexOption.IGNORE_CASE), "cpuopt_ignore_memory_aborts = false")
                if (replaced != text) {
                    text = replaced
                    modified = true
                }
            }
            if (modified) {
                file.writeText(text, Charsets.UTF_8)
            }
        } catch (_: Exception) {}
    }

    fun loadCustomConfig(game: Game) {
        val customFile = getCustomSettingsFile(game)
        val cleanProg = game.programIdHex.trim().uppercase()
        if (cleanProg == "010015100B514000" || cleanProg.startsWith("010015100B514")) {
            sanitizeMarioWonderConfig(customFile)
        }
        val fileName = customFile.nameWithoutExtension.ifEmpty {
            try { FileUtil.getFilename(Uri.parse(game.path)) } catch (_: Exception) { "" }
        }
        val programIdArg = game.programIdHex.trim().ifEmpty { game.programId }
        NativeConfig.initializePerGameConfig(programIdArg, fileName)
    }
}
