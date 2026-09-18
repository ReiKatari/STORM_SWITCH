// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Activity
import android.content.Context
import android.view.View
import android.view.Window
import androidx.core.content.edit
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.features.settings.model.Settings

object FullscreenHelper {
    fun isFullscreenEnabled(context: Context): Boolean {
        return PreferenceManager.getDefaultSharedPreferences(context).getBoolean(
            Settings.PREF_APP_FULLSCREEN,
            Settings.APP_FULLSCREEN_DEFAULT
        )
    }

    fun setFullscreenEnabled(context: Context, enabled: Boolean) {
        PreferenceManager.getDefaultSharedPreferences(context).edit {
            putBoolean(Settings.PREF_APP_FULLSCREEN, enabled)
        }
    }

    fun shouldHideSystemBars(activity: Activity): Boolean {
        return isFullscreenEnabled(activity)
    }

    fun applyToWindow(window: Window, hideSystemBars: Boolean) {
        try {
            WindowCompat.setDecorFitsSystemWindows(window, false)
            val controller = WindowInsetsControllerCompat(window, window.decorView)

            if (hideSystemBars) {
                controller.hide(WindowInsetsCompat.Type.systemBars())
                controller.systemBarsBehavior =
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE

                @Suppress("DEPRECATION")
                window.decorView.systemUiVisibility =
                    View.SYSTEM_UI_FLAG_FULLSCREEN or
                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            } else {
                controller.show(WindowInsetsCompat.Type.systemBars())
            }
        } catch (_: Throwable) {
        }
    }

    fun applyToActivity(activity: Activity) {
        applyToWindow(activity.window, isFullscreenEnabled(activity))
    }
}
