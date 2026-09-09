// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Activity
import android.content.Context

/**
 * Backwards-compatible wrapper delegating to UniversalGameModeManager.
 */
object SamsungGameBoosterHelper {

    fun isSamsungDevice(): Boolean {
        return UniversalGameModeManager.currentVendor == UniversalGameModeManager.Vendor.SAMSUNG
    }

    fun onGameStart(activity: Activity, gameTitle: String?) {
        UniversalGameModeManager.onGameStart(activity, gameTitle)
    }

    fun onGameResume(activity: Activity, gameTitle: String?) {
        UniversalGameModeManager.onGameResume(activity, gameTitle)
    }

    fun onGamePause(activity: Activity) {
        UniversalGameModeManager.onGamePause(activity)
    }

    fun onGameStop(activity: Activity) {
        UniversalGameModeManager.onGameStop(activity)
    }

    fun onGameStart(context: Context, gameTitle: String?) {
        if (context is Activity) {
            UniversalGameModeManager.onGameStart(context, gameTitle)
        }
    }

    fun onGameResume(context: Context, gameTitle: String?) {
        if (context is Activity) {
            UniversalGameModeManager.onGameResume(context, gameTitle)
        }
    }

    fun onGamePause(context: Context) {
        if (context is Activity) {
            UniversalGameModeManager.onGamePause(context)
        }
    }

    fun onGameStop(context: Context) {
        if (context is Activity) {
            UniversalGameModeManager.onGameStop(context)
        }
    }
}
