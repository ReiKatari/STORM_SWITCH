// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import java.util.Locale

/**
 * Universal Game Metadata, Decimal Separator & Typography Helper for STORM SWITCH.
 * Enforces strict 16-HEX Title IDs (%016X), clean un-prefixed versions (zero 'v'/'V'),
 * and locale-dependent decimal separators (',' for Russian, '.' for other languages).
 */
object StormFormatHelper {

    /**
     * Cleans and formats version string, stripping any leading 'v' or 'V' prefixes.
     */
    @JvmStatic
    fun formatVersion(rawVersion: String?): String {
        if (rawVersion.isNullOrBlank()) return "1.0.0"
        var clean = rawVersion.trim()
        while (clean.startsWith("v", ignoreCase = true)) {
            clean = clean.substring(1).trim()
        }
        return if (clean.isNotEmpty()) clean else "1.0.0"
    }

    /**
     * Strictly formats Title ID as a 16-character uppercase HEX string (%016X).
     */
    @JvmStatic
    fun formatTitleId(programId: String?): String {
        if (programId.isNullOrBlank()) return "0000000000000000"
        val clean = programId.trim().removePrefix("0x").removePrefix("0X")
        val parsed = clean.toULongOrNull(16) ?: clean.toULongOrNull(10)
        return if (parsed != null && parsed != 0uL) {
            String.format(Locale.ROOT, "%016X", parsed.toLong())
        } else if (clean.isNotEmpty() && clean != "0") {
            clean.padStart(16, '0').uppercase(Locale.ROOT)
        } else {
            "0000000000000000"
        }
    }

    /**
     * Returns true if current active display language is Russian.
     */
    @JvmStatic
    fun isRussianLocale(): Boolean {
        return try {
            val appLocale = YuzuApplication.getLocalizedContext().resources.configuration.locales[0]
            appLocale.language.equals("ru", ignoreCase = true)
        } catch (_: Throwable) {
            Locale.getDefault().language.equals("ru", ignoreCase = true)
        }
    }

    /**
     * Formats file size in bytes to a human-readable string with locale-dependent decimal separator:
     * - Russian (ru): comma ',' separator (e.g. "8,64 ГБ")
     * - Other locales: dot '.' separator (e.g. "8.64 GB")
     */
    @JvmStatic
    fun formatFileSize(bytes: Long, context: Context? = null): String {
        if (bytes <= 0L) return if (isRussianLocale()) "0 Б" else "0 B"
        val isRu = isRussianLocale()
        val sep = if (isRu) "," else "."

        val kb = 1024.0
        val mb = kb * 1024.0
        val gb = mb * 1024.0
        val tb = gb * 1024.0

        val (value, unit) = when {
            bytes >= tb -> Pair(bytes / tb, if (isRu) "ТБ" else "TB")
            bytes >= gb -> Pair(bytes / gb, if (isRu) "ГБ" else "GB")
            bytes >= mb -> Pair(bytes / mb, if (isRu) "МБ" else "MB")
            bytes >= kb -> Pair(bytes / kb, if (isRu) "КБ" else "KB")
            else -> Pair(bytes.toDouble(), if (isRu) "Б" else "B")
        }

        val formattedVal = String.format(Locale.ROOT, "%.2f", value).replace(".", sep)
        return "$formattedVal $unit"
    }

    /**
     * Localizes an existing raw size string (e.g. "8.64 GB"):
     * Replaces decimal dot with comma in Russian and updates units to Cyrillic.
     */
    @JvmStatic
    fun localizeSizeString(rawSize: String?, context: Context? = null): String {
        if (rawSize.isNullOrBlank()) return ""
        val isRu = isRussianLocale()
        var s = rawSize.trim()

        if (isRu) {
            // Replace dot between digits with comma
            s = s.replace(Regex("(\\d+)\\.(\\d+)"), "$1,$2")
            s = s.replace(Regex("(?i)\\bGB\\b"), "ГБ")
            s = s.replace(Regex("(?i)\\bMB\\b"), "МБ")
            s = s.replace(Regex("(?i)\\bKB\\b"), "КБ")
            s = s.replace(Regex("(?i)\\bB\\b"), "Б")
        } else {
            // Replace comma between digits with dot
            s = s.replace(Regex("(\\d+),(\\d+)"), "$1.$2")
            s = s.replace(Regex("(?i)\\bГБ\\b"), "GB")
            s = s.replace(Regex("(?i)\\bМБ\\b"), "MB")
            s = s.replace(Regex("(?i)\\bКБ\\b"), "KB")
            s = s.replace(Regex("(?i)\\bБ\\b"), "B")
        }
        return s
    }

    /**
     * Formats add-ons badge string without truncation ("Дополнений: %d" in RU / "Add-ons: %d" in EN).
     */
    @JvmStatic
    fun formatAddons(count: Int, context: Context?): String {
        val ctx = context ?: YuzuApplication.appContext
        return try {
            ctx.getString(R.string.game_badge_dlc_count_fmt, count)
        } catch (_: Throwable) {
            if (isRussianLocale()) "Дополнений: $count" else "Add-ons: $count"
        }
    }
}
