// SPDX-FileCopyrightText: 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.model

import org.yuzu.yuzu_emu.utils.GpuDriverMetadata

data class Driver(
    override var selected: Boolean,
    val title: String,
    val version: String = "",
    val description: String = ""
) : SelectableItem {
    override fun onSelectionStateChanged(selected: Boolean) {
        this.selected = selected
    }

    companion object {
        fun GpuDriverMetadata.toDriver(selected: Boolean = false): Driver {
            val ver = packageVersion?.takeIf { it.isNotBlank() } ?: version?.takeIf { it.isNotBlank() } ?: ""
            val rawName = name?.takeIf { it.isNotBlank() } ?: ""

            // Clean title: strip parenthesis subtitle from name if redundant with description
            val cleanName = if (rawName.contains("(") && rawName.contains(")")) {
                rawName.substringBefore("(").trim()
            } else {
                rawName.trim()
            }

            val mainVerNumber = ver.substringBefore("-").trim()
            val displayTitle = when {
                cleanName.isNotEmpty() && mainVerNumber.isNotEmpty() && cleanName.contains(mainVerNumber) -> cleanName
                cleanName.isNotEmpty() && ver.isNotEmpty() && cleanName.contains(ver) -> cleanName
                cleanName.isNotEmpty() && ver.isNotEmpty() && !cleanName.endsWith(ver) -> "$cleanName $ver"
                cleanName.isNotEmpty() -> cleanName
                ver.isNotEmpty() -> ver
                else -> ""
            }

            val cleanDesc = description?.trim() ?: ""

            return Driver(
                selected,
                displayTitle,
                ver,
                cleanDesc
            )
        }
    }
}
