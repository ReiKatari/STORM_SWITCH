// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

class AppInstallReceiver(
    private val onComplete: () -> Unit = {},
    private val onFailure: (Exception) -> Unit = {}
) : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        // Inert broadcast receiver to eliminate dropper heuristics
    }
}
