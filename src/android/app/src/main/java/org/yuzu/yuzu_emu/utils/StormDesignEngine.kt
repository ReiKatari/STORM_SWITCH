// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Activity
import android.app.Application
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.LayerDrawable
import android.graphics.drawable.RippleDrawable
import android.os.Build
import android.os.Bundle
import android.util.TypedValue
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.ViewTreeObserver
import android.view.Window
import android.view.animation.DecelerateInterpolator
import android.view.animation.LinearInterpolator
import android.view.animation.OvershootInterpolator
import android.widget.Button
import android.widget.EditText
import android.widget.Switch
import androidx.annotation.AttrRes
import androidx.annotation.ColorInt
import androidx.appcompat.widget.Toolbar
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.card.MaterialCardView
import com.google.android.material.chip.Chip
import com.google.android.material.materialswitch.MaterialSwitch
import com.google.android.material.textfield.TextInputLayout
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import java.util.Collections
import java.util.WeakHashMap
import kotlin.math.roundToInt

/**
 * High-performance 5-Design System Engine for STORM SWITCH.
 * Provides 5 distinct volumetric 3D styling modes and tactile animations
 * operating 100% orthogonally across all 9 STORM color themes at 60/120 FPS.
 */
object StormDesignEngine {

    const val DESIGN_HYPERION_GLASS_3D = 0
    const val DESIGN_CYBER_DECK_TACTILE = 1
    const val DESIGN_AURORA_QUANTUM = 2
    const val DESIGN_ORBITAL_SPATIAL_UI = 3
    const val DESIGN_CHRONO_NEXUS_COMMANDER = 4

    private val activeListeners =
        Collections.synchronizedMap(WeakHashMap<Activity, ViewTreeObserver.OnGlobalLayoutListener>())
    private var isInstalled = false

    /**
     * Installs lifecycle callbacks on the Application instance.
     */
    @JvmStatic
    fun install(application: Application) {
        if (isInstalled) return
        isInstalled = true

        application.registerActivityLifecycleCallbacks(object : Application.ActivityLifecycleCallbacks {
            override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {
                applyToActivity(activity)
            }

            override fun onActivityStarted(activity: Activity) {}

            override fun onActivityResumed(activity: Activity) {
                applyToActivity(activity)
            }

            override fun onActivityPaused(activity: Activity) {}
            override fun onActivityStopped(activity: Activity) {}
            override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {}

            override fun onActivityDestroyed(activity: Activity) {
                cleanup(activity)
            }
        })
    }

    /**
     * Applies the active design system to the given Activity.
     */
    @JvmStatic
    fun applyToActivity(activity: Activity) {
        runCatching {
            val design = getDesignStyle()
            val themeIndex = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
            val window = activity.window ?: return

            // Style root window decor background gradient for non-emulation activities
            if (!activity.javaClass.name.contains("EmulationActivity", ignoreCase = true)) {
                applyWindowDecorBackground(activity, window, design)
            }

            val decor = window.decorView
            styleViewHierarchy(decor, design, themeIndex)

            // Attach debounced layout listener if not already registered
            if (!activeListeners.containsKey(activity)) {
                var isTraversing = false
                val listener = ViewTreeObserver.OnGlobalLayoutListener {
                    if (isTraversing) return@OnGlobalLayoutListener
                    isTraversing = true
                    try {
                        val currentDesign = getDesignStyle()
                        val currentTheme = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
                        styleViewHierarchy(decor, currentDesign, currentTheme)
                    } finally {
                        isTraversing = false
                    }
                }
                decor.viewTreeObserver?.let { observer ->
                    if (observer.isAlive) {
                        observer.addOnGlobalLayoutListener(listener)
                        activeListeners[activity] = listener
                    }
                }
            }
        }
    }

    private fun cleanup(activity: Activity) {
        runCatching {
            activeListeners.remove(activity)?.let { listener ->
                val decor = activity.window?.decorView
                if (decor != null && decor.viewTreeObserver.isAlive) {
                    decor.viewTreeObserver.removeOnGlobalLayoutListener(listener)
                }
            }
        }
    }

    /**
     * Gets the currently active UI Design Style (0..4).
     */
    fun getDesignStyle(): Int = runCatching {
        IntSetting.UI_DESIGN_STYLE.getInt().coerceIn(0, 4)
    }.getOrDefault(0)

    /**
     * Recursively styles the view hierarchy using debounced tag checks.
     */
    fun styleViewHierarchy(view: View, design: Int, themeIndex: Int) {
        val desiredTag = (design shl 16) or (themeIndex and 0xFFFF)
        styleView(view, design, themeIndex, desiredTag)

        if (view is ViewGroup) {
            val childCount = view.childCount
            for (i in 0 until childCount) {
                val child = view.getChildAt(i) ?: continue
                styleViewHierarchy(child, design, themeIndex)
            }
        }
    }

    private fun styleView(view: View, design: Int, themeIndex: Int, desiredTag: Int) {
        if (view.getTag(R.id.tag_storm_design_applied) == desiredTag) {
            return
        }

        runCatching {
            when (view) {
                is MaterialCardView -> styleCardView(view, design)
                is MaterialButton -> styleButton(view, design)
                is TextInputLayout -> styleTextInputLayout(view, design)
                is Chip -> styleChip(view, design)
                is EditText -> styleEditText(view, design)
            }
            view.setTag(R.id.tag_storm_design_applied, desiredTag)
        }
    }

    // =========================================================================
    // Component Styling
    // =========================================================================

    private fun styleCardView(card: MaterialCardView, design: Int) {
        val context = card.context
        val density = context.resources.displayMetrics.density
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())
        val surfaceColor = resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF121826.toInt())
        val surfaceContainer = resolveColor(context, com.google.android.material.R.attr.colorSurfaceVariant, surfaceColor)
        val outlineColor = resolveColor(context, com.google.android.material.R.attr.colorOutline, 0xFF334155.toInt())

        when (design) {
            DESIGN_HYPERION_GLASS_3D -> {
                // Frosted glass specular top rim (1.5dp), 14dp rounded corners, deep elevated shadow (8dp)
                card.radius = 14f * density
                card.cardElevation = 8f * density
                card.strokeWidth = maxOf(1, (1.5f * density).roundToInt())
                val specularStroke = blend(Color.WHITE, primaryColor, 0.25f)
                card.strokeColor = setAlpha(specularStroke, 0x66)
                card.setCardBackgroundColor(setAlpha(surfaceContainer, 0xDF))
            }

            DESIGN_CYBER_DECK_TACTILE -> {
                // Sharp precision tactical 4dp corners, dual-tone cybernetic border, recessed input wells
                card.radius = 4f * density
                card.cardElevation = 4f * density
                card.strokeWidth = maxOf(1, (2.5f * density).roundToInt())
                card.strokeColor = primaryColor
                card.setCardBackgroundColor(surfaceContainer)
            }

            DESIGN_AURORA_QUANTUM -> {
                // Fluid organic 24dp pill/capsule corners, luminous accent rim, plasma sheen
                card.radius = 24f * density
                card.cardElevation = 6f * density
                card.strokeWidth = maxOf(1, (2f * density).roundToInt())
                card.strokeColor = setAlpha(primaryColor, 0xCC)
                val blendedSurface = blend(surfaceContainer, primaryColor, 0.08f)
                card.setCardBackgroundColor(blendedSurface)
            }

            DESIGN_ORBITAL_SPATIAL_UI -> {
                // VisionOS spatial floating lens (18dp corners), high Z-depth floating cards (12dp)
                card.radius = 18f * density
                card.cardElevation = 12f * density
                card.strokeWidth = maxOf(1, (2f * density).roundToInt())
                card.strokeColor = setAlpha(blend(primaryColor, Color.WHITE, 0.35f), 0x99)
                card.setCardBackgroundColor(setAlpha(surfaceContainer, 0xF0))
            }

            DESIGN_CHRONO_NEXUS_COMMANDER -> {
                // Heavy flagship command-deck 8dp armored bevel, high-contrast command cards
                card.radius = 8f * density
                card.cardElevation = 6f * density
                card.strokeWidth = maxOf(1, (3f * density).roundToInt())
                card.strokeColor = outlineColor
                card.setCardBackgroundColor(surfaceContainer)
            }
        }

        applyTouchFeedback(card, design, density)
    }

    private fun styleButton(button: MaterialButton, design: Int) {
        // Guard against styling switches or toolbar buttons
        if (button is MaterialSwitch || button is Switch) return
        var parentView = button.parent
        while (parentView != null) {
            if (parentView is Toolbar || parentView is MaterialToolbar) {
                return
            }
            parentView = parentView.parent
        }

        val context = button.context
        val density = context.resources.displayMetrics.density
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())
        val outlineColor = resolveColor(context, com.google.android.material.R.attr.colorOutline, 0xFF334155.toInt())

        when (design) {
            DESIGN_HYPERION_GLASS_3D -> {
                button.cornerRadius = (14f * density).roundToInt()
                button.strokeWidth = maxOf(1, (1.5f * density).roundToInt())
                button.strokeColor = ColorStateList.valueOf(setAlpha(Color.WHITE, 0x88))
                button.elevation = 6f * density
            }

            DESIGN_CYBER_DECK_TACTILE -> {
                button.cornerRadius = (4f * density).roundToInt()
                button.strokeWidth = maxOf(1, (2f * density).roundToInt())
                button.strokeColor = ColorStateList.valueOf(primaryColor)
                button.elevation = 3f * density
            }

            DESIGN_AURORA_QUANTUM -> {
                button.cornerRadius = (24f * density).roundToInt()
                button.strokeWidth = maxOf(1, (1.5f * density).roundToInt())
                button.strokeColor = ColorStateList.valueOf(setAlpha(primaryColor, 0xDD))
                button.elevation = 5f * density
            }

            DESIGN_ORBITAL_SPATIAL_UI -> {
                button.cornerRadius = (18f * density).roundToInt()
                button.strokeWidth = maxOf(1, (2f * density).roundToInt())
                button.strokeColor = ColorStateList.valueOf(setAlpha(blend(primaryColor, Color.WHITE, 0.3f), 0xAA))
                button.elevation = 8f * density
            }

            DESIGN_CHRONO_NEXUS_COMMANDER -> {
                button.cornerRadius = (8f * density).roundToInt()
                button.strokeWidth = maxOf(1, (2.5f * density).roundToInt())
                button.strokeColor = ColorStateList.valueOf(outlineColor)
                button.elevation = 4f * density
            }
        }

        applyTouchFeedback(button, design, density)
    }

    private fun styleTextInputLayout(layout: TextInputLayout, design: Int) {
        val context = layout.context
        val density = context.resources.displayMetrics.density
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())

        val (radius, stroke) = when (design) {
            DESIGN_HYPERION_GLASS_3D -> Pair(14f * density, maxOf(1, (1.5f * density).roundToInt()))
            DESIGN_CYBER_DECK_TACTILE -> Pair(4f * density, maxOf(1, (2.5f * density).roundToInt()))
            DESIGN_AURORA_QUANTUM -> Pair(20f * density, maxOf(1, (2f * density).roundToInt()))
            DESIGN_ORBITAL_SPATIAL_UI -> Pair(16f * density, maxOf(1, (2f * density).roundToInt()))
            DESIGN_CHRONO_NEXUS_COMMANDER -> Pair(8f * density, maxOf(1, (2.5f * density).roundToInt()))
            else -> Pair(8f * density, maxOf(1, (1.5f * density).roundToInt()))
        }

        layout.setBoxCornerRadii(radius, radius, radius, radius)
        layout.boxStrokeWidth = stroke
        layout.boxStrokeColor = primaryColor
    }

    private fun styleChip(chip: Chip, design: Int) {
        val context = chip.context
        val density = context.resources.displayMetrics.density
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())

        when (design) {
            DESIGN_HYPERION_GLASS_3D -> {
                chip.chipCornerRadius = 14f * density
                chip.chipStrokeWidth = 1.5f * density
                chip.chipStrokeColor = ColorStateList.valueOf(setAlpha(Color.WHITE, 0x66))
            }
            DESIGN_CYBER_DECK_TACTILE -> {
                chip.chipCornerRadius = 4f * density
                chip.chipStrokeWidth = 2f * density
                chip.chipStrokeColor = ColorStateList.valueOf(primaryColor)
            }
            DESIGN_AURORA_QUANTUM -> {
                chip.chipCornerRadius = 20f * density
                chip.chipStrokeWidth = 1.5f * density
                chip.chipStrokeColor = ColorStateList.valueOf(setAlpha(primaryColor, 0xBB))
            }
            DESIGN_ORBITAL_SPATIAL_UI -> {
                chip.chipCornerRadius = 16f * density
                chip.chipStrokeWidth = 2f * density
                chip.chipStrokeColor = ColorStateList.valueOf(setAlpha(blend(primaryColor, Color.WHITE, 0.3f), 0x99))
            }
            DESIGN_CHRONO_NEXUS_COMMANDER -> {
                chip.chipCornerRadius = 8f * density
                chip.chipStrokeWidth = 2.5f * density
                chip.chipStrokeColor = ColorStateList.valueOf(primaryColor)
            }
        }

        applyTouchFeedback(chip, design, density)
    }

    private fun styleEditText(editText: EditText, design: Int) {
        val context = editText.context
        val density = context.resources.displayMetrics.density
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())
        val surfaceColor = resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF121826.toInt())

        val r = when (design) {
            DESIGN_HYPERION_GLASS_3D -> 14f * density
            DESIGN_CYBER_DECK_TACTILE -> 4f * density
            DESIGN_AURORA_QUANTUM -> 20f * density
            DESIGN_ORBITAL_SPATIAL_UI -> 16f * density
            DESIGN_CHRONO_NEXUS_COMMANDER -> 8f * density
            else -> 8f * density
        }

        runCatching {
            val bg = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = r
                setColor(setAlpha(surfaceColor, 0xF0))
                setStroke(maxOf(1, (1.5f * density).roundToInt()), setAlpha(primaryColor, 0x88))
            }
            editText.background = bg
        }
    }

    // =========================================================================
    // Root Window Decor Background Gradient
    // =========================================================================

    private fun applyWindowDecorBackground(context: Context, window: Window, design: Int) {
        val surfaceColor = resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF121826.toInt())
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())

        val gradient = when (design) {
            DESIGN_HYPERION_GLASS_3D -> GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(
                    blend(surfaceColor, Color.WHITE, 0.04f),
                    surfaceColor,
                    blend(surfaceColor, Color.BLACK, 0.15f)
                )
            )

            DESIGN_CYBER_DECK_TACTILE -> GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.05f),
                    blend(surfaceColor, Color.BLACK, 0.22f)
                )
            )

            DESIGN_AURORA_QUANTUM -> GradientDrawable(
                GradientDrawable.Orientation.BL_TR,
                intArrayOf(
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.10f),
                    blend(surfaceColor, Color.BLACK, 0.08f)
                )
            )

            DESIGN_ORBITAL_SPATIAL_UI -> GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(
                    blend(surfaceColor, Color.WHITE, 0.05f),
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.04f)
                )
            )

            DESIGN_CHRONO_NEXUS_COMMANDER -> GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                intArrayOf(
                    blend(surfaceColor, Color.BLACK, 0.10f),
                    surfaceColor,
                    blend(surfaceColor, Color.BLACK, 0.16f)
                )
            )

            else -> null
        }

        gradient?.let {
            window.setBackgroundDrawable(it)
        }
    }

    // =========================================================================
    // Tactile 3D Touch Animations
    // =========================================================================

    private fun applyTouchFeedback(view: View, design: Int, density: Float) {
        if (!view.isClickable && !view.isFocusable) return
        val originalTranslationZ = view.translationZ
        val originalTranslationY = view.translationY

        view.setOnTouchListener { v, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    when (design) {
                        DESIGN_HYPERION_GLASS_3D -> {
                            v.animate()
                                .scaleX(0.96f)
                                .scaleY(0.96f)
                                .setDuration(110)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                        DESIGN_CYBER_DECK_TACTILE -> {
                            v.animate()
                                .scaleX(0.94f)
                                .scaleY(0.94f)
                                .setDuration(80)
                                .setInterpolator(LinearInterpolator())
                                .start()
                            runCatching {
                                v.performHapticFeedback(
                                    HapticFeedbackConstants.KEYBOARD_TAP,
                                    HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING
                                )
                            }
                        }
                        DESIGN_AURORA_QUANTUM -> {
                            v.animate()
                                .scaleX(0.95f)
                                .scaleY(0.95f)
                                .translationZ(originalTranslationZ + 4f * density)
                                .setDuration(130)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                        DESIGN_ORBITAL_SPATIAL_UI -> {
                            v.animate()
                                .scaleX(0.97f)
                                .scaleY(0.97f)
                                .translationZ(originalTranslationZ + 6f * density)
                                .setDuration(150)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                        DESIGN_CHRONO_NEXUS_COMMANDER -> {
                            v.animate()
                                .translationY(originalTranslationY + 2f * density)
                                .setDuration(70)
                                .setInterpolator(LinearInterpolator())
                                .start()
                            runCatching {
                                v.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)
                            }
                        }
                    }
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    when (design) {
                        DESIGN_HYPERION_GLASS_3D -> {
                            v.animate()
                                .scaleX(1.0f)
                                .scaleY(1.0f)
                                .setDuration(220)
                                .setInterpolator(OvershootInterpolator(1.4f))
                                .start()
                        }
                        DESIGN_CYBER_DECK_TACTILE -> {
                            v.animate()
                                .scaleX(1.0f)
                                .scaleY(1.0f)
                                .setDuration(90)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                        DESIGN_AURORA_QUANTUM -> {
                            v.animate()
                                .scaleX(1.0f)
                                .scaleY(1.0f)
                                .translationZ(originalTranslationZ)
                                .setDuration(180)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                        DESIGN_ORBITAL_SPATIAL_UI -> {
                            v.animate()
                                .scaleX(1.0f)
                                .scaleY(1.0f)
                                .translationZ(originalTranslationZ)
                                .setDuration(240)
                                .setInterpolator(OvershootInterpolator(1.2f))
                                .start()
                        }
                        DESIGN_CHRONO_NEXUS_COMMANDER -> {
                            v.animate()
                                .translationY(originalTranslationY)
                                .setDuration(100)
                                .setInterpolator(DecelerateInterpolator())
                                .start()
                        }
                    }
                }
            }
            false // Always return false so click and scroll listeners receive the event
        }
    }

    // =========================================================================
    // Utilities
    // =========================================================================

    @ColorInt
    private fun resolveColor(context: Context, @AttrRes attrRes: Int, fallback: Int): Int {
        val tv = TypedValue()
        return if (context.theme.resolveAttribute(attrRes, tv, true)) {
            if (tv.type >= TypedValue.TYPE_FIRST_COLOR_INT && tv.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                tv.data
            } else {
                androidx.core.content.ContextCompat.getColor(context, tv.resourceId)
            }
        } else {
            fallback
        }
    }

    @ColorInt
    private fun setAlpha(@ColorInt color: Int, alpha: Int): Int {
        return (color and 0x00FFFFFF) or ((alpha.coerceIn(0, 255)) shl 24)
    }

    @ColorInt
    private fun blend(@ColorInt c1: Int, @ColorInt c2: Int, ratio: Float): Int {
        val clampedRatio = ratio.coerceIn(0f, 1f)
        val inverse = 1f - clampedRatio
        val a = (Color.alpha(c1) * inverse + Color.alpha(c2) * clampedRatio).roundToInt()
        val r = (Color.red(c1) * inverse + Color.red(c2) * clampedRatio).roundToInt()
        val g = (Color.green(c1) * inverse + Color.green(c2) * clampedRatio).roundToInt()
        val b = (Color.blue(c1) * inverse + Color.blue(c2) * clampedRatio).roundToInt()
        return Color.argb(a, r, g, b)
    }
}
