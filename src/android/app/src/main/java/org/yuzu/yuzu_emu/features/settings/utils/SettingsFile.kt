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

    fun loadCustomConfig(game: Game) {
        val fileName = FileUtil.getFilename(Uri.parse(game.path))
        NativeConfig.initializePerGameConfig(game.programId, fileName)
    }
}
