// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.SharedPreferences
import android.content.res.Configuration
import android.graphics.Color
import android.os.Build
import androidx.annotation.ColorInt
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsControllerCompat
import kotlin.math.roundToInt
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import org.yuzu.yuzu_emu.ui.main.ThemeProvider
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.settings.model.Settings

object ThemeHelper {
    const val SYSTEM_BAR_ALPHA = 0.9f

    // Listener that detects if the theme keys are being changed from the setting menu and recreates the activity
    private var listener: SharedPreferences.OnSharedPreferenceChangeListener? = null
    private val preferences = PreferenceManager.getDefaultSharedPreferences(
        YuzuApplication.appContext
    )

    fun setTheme(activity: AppCompatActivity) {
        setThemeMode(activity)
        activity.setTheme(getSelectedStaticThemeColor())
    }

    fun getSelectedStaticThemeColor(): Int {
        val themeIndex = IntSetting.THEME_MODE.getInt()
        return when (themeIndex) {
            0 -> R.style.Theme_Storm_Dark
            1 -> R.style.Theme_Storm_Night
            2 -> R.style.Theme_Storm_Day
            3 -> R.style.Theme_Storm_Midnight
            4 -> R.style.Theme_Storm_Matrix
            5 -> R.style.Theme_Storm_Cyberpunk
            6 -> R.style.Theme_Storm_Fantasy
            7 -> R.style.Theme_Storm_Warhammer40k
            else -> R.style.Theme_Storm_Dark
        }
    }

    fun getCurrentTheme(): Int = getSelectedStaticThemeColor()

    @ColorInt
    fun getColorWithOpacity(@ColorInt color: Int, alphaFactor: Float): Int {
        return Color.argb(
            (alphaFactor * Color.alpha(color)).roundToInt(),
            Color.red(color),
            Color.green(color),
            Color.blue(color)
        )
    }

    @ColorInt
    fun getColor(context: android.content.Context, @androidx.annotation.AttrRes attrRes: Int): Int {
        return getColorFromAttr(context, attrRes)
    }

    @ColorInt
    fun getColorFromAttr(context: android.content.Context, @androidx.annotation.AttrRes attrRes: Int): Int {
        val typedValue = android.util.TypedValue()
        context.theme.resolveAttribute(attrRes, typedValue, true)
        return typedValue.data
    }

    fun setCorrectTheme(activity: AppCompatActivity) {
        val currentTheme = (activity as ThemeProvider).themeId
        setTheme(activity)
        if (currentTheme != (activity as ThemeProvider).themeId) {
            activity.recreate()
        }
    }

    fun applySystemBarsTheme(window: android.view.Window?, context: android.content.Context? = null) {
        if (window == null) return
        try {
            WindowCompat.setDecorFitsSystemWindows(window, false)
            val windowController = WindowCompat.getInsetsController(window, window.decorView)
            val themeMode = IntSetting.THEME_MODE.getInt()
            val isLight = themeMode == 2
            windowController.isAppearanceLightStatusBars = isLight
            windowController.isAppearanceLightNavigationBars = isLight
            window.statusBarColor = Color.TRANSPARENT
            window.navigationBarColor = Color.TRANSPARENT
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                window.isNavigationBarContrastEnforced = false
                window.isStatusBarContrastEnforced = false
            }
            val ctx = context ?: window.context
            if (FullscreenHelper.isFullscreenEnabled(ctx)) {
                FullscreenHelper.applyToWindow(window, true)
            }
        } catch (_: Throwable) {
        }
    }

    fun setThemeMode(activity: AppCompatActivity) {
        val themeMode = IntSetting.THEME_MODE.getInt()
        val targetNightMode = if (themeMode == 2) AppCompatDelegate.MODE_NIGHT_NO else AppCompatDelegate.MODE_NIGHT_YES
        if (AppCompatDelegate.getDefaultNightMode() != targetNightMode) {
            AppCompatDelegate.setDefaultNightMode(targetNightMode)
        }
        if (activity.delegate.localNightMode != targetNightMode) {
            activity.delegate.localNightMode = targetNightMode
        }
        applySystemBarsTheme(activity.window, activity)
    }

    private fun isNightMode(activity: AppCompatActivity): Boolean {
        return when (activity.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK) {
            Configuration.UI_MODE_NIGHT_NO -> false
            Configuration.UI_MODE_NIGHT_YES -> true
            else -> false
        }
    }

    private fun setLightModeSystemBars(windowController: WindowInsetsControllerCompat) {
        windowController.isAppearanceLightStatusBars = true
        windowController.isAppearanceLightNavigationBars = true
    }

    private fun setDarkModeSystemBars(windowController: WindowInsetsControllerCompat) {
        windowController.isAppearanceLightStatusBars = false
        windowController.isAppearanceLightNavigationBars = false
    }

    fun showThemedSnackbar(
        view: android.view.View,
        message: CharSequence,
        duration: Int = com.google.android.material.snackbar.Snackbar.LENGTH_SHORT,
        actionText: CharSequence? = null,
        onActionClick: android.view.View.OnClickListener? = null
    ): com.google.android.material.snackbar.Snackbar {
        val context = view.context
        val snackbar = com.google.android.material.snackbar.Snackbar.make(view, message, duration)
        val snackbarView = snackbar.view
        val density = context.resources.displayMetrics.density
        val surfaceColor = getColor(context, com.google.android.material.R.attr.colorSurfaceVariant)
        val strokeColor = getColor(context, com.google.android.material.R.attr.colorOutline)
        val textColor = getColor(context, com.google.android.material.R.attr.colorOnSurface)
        val primaryColor = getColor(context, com.google.android.material.R.attr.colorPrimary)

        val shape = com.google.android.material.shape.MaterialShapeDrawable().apply {
            shapeAppearanceModel = com.google.android.material.shape.ShapeAppearanceModel.builder()
                .setAllCornerSizes(12f * density)
                .build()
            fillColor = android.content.res.ColorStateList.valueOf(surfaceColor)
            setStroke(1.5f * density, strokeColor)
            elevation = 6f * density
        }
        snackbarView.background = shape
        snackbar.setTextColor(textColor)
        snackbar.setActionTextColor(primaryColor)

        (snackbarView.layoutParams as? android.view.ViewGroup.MarginLayoutParams)?.let { params ->
            val marginH = (16f * density).toInt()
            val marginB = (16f * density).toInt()
            params.setMargins(marginH, params.topMargin, marginH, marginB)
            snackbarView.layoutParams = params
        }

        if (actionText != null && onActionClick != null) {
            snackbar.setAction(actionText, onActionClick)
        }
        snackbar.show()
        return snackbar
    }

    fun ThemeChangeListener(activity: AppCompatActivity) {
        listener = SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
            val relevantKeys = listOf(
                Settings.PREF_STATIC_THEME_COLOR,
                Settings.PREF_THEME_MODE,
                Settings.PREF_BLACK_BACKGROUNDS
            )
            if (key in relevantKeys) {
                activity.recreate()
            }
        }
        preferences.registerOnSharedPreferenceChangeListener(listener)
    }
}

enum class Theme(val int: Int) {
    Default(0),
    MaterialYou(1);

    companion object {
        fun from(int: Int): Theme = entries.firstOrNull { it.int == int } ?: Default
    }
}
