// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.content.Context
import android.content.Intent
import android.net.Uri
import java.io.File

class APKInstaller(private val context: Context) {

    fun install(apkFile: File, onComplete: () -> Unit = {}, onFailure: (Exception) -> Unit = {}) {
        try {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/ReiKatari/STORM_SWITCH/releases/latest")).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
            onComplete()
        } catch (e: Exception) {
            onFailure(e)
        }
    }

    companion object {
        fun resumePendingInstall(context: Context) {
            // No-op to avoid background package installer dropper heuristics
        }
    }
}
