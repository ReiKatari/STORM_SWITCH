// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Activity
import android.app.Application
import android.app.Dialog
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.util.TypedValue
import android.view.Gravity
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.ViewTreeObserver
import android.view.Window
import android.view.animation.DecelerateInterpolator
import android.view.animation.LinearInterpolator
import android.view.animation.OvershootInterpolator
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
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import java.util.Collections
import java.util.WeakHashMap
import kotlin.math.roundToInt

/**
 * Clean 17.1.0 Design Engine for STORM SWITCH with zero border-stroke glow.
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

    @JvmStatic
    fun applyToActivity(activity: Activity) {
        runCatching {
            val design = getDesignStyle()
            val themeIndex = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
            val window = activity.window ?: return

            if (!activity.javaClass.name.contains("EmulationActivity", ignoreCase = true)) {
                applyWindowDecorBackground(activity, window, design)
            }

            val decor = window.decorView
            styleViewHierarchy(decor, design, themeIndex)

            if (!activity.javaClass.name.contains("EmulationActivity", ignoreCase = true) && !activeListeners.containsKey(activity)) {
                var isTraversing = false
                val listener = ViewTreeObserver.OnGlobalLayoutListener {
                    if (isTraversing) return@OnGlobalLayoutListener
                    isTraversing = true
                    try {
                        val currentDesign = getDesignStyle()
                        val currentTheme = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
                        styleViewHierarchy(decor, currentDesign, currentTheme)
                    } catch (_: Throwable) {
                    } finally {
                        isTraversing = false
                    }
                }
                decor.viewTreeObserver.addOnGlobalLayoutListener(listener)
                activeListeners[activity] = listener
            }
        }
    }

    @JvmStatic
    @JvmOverloads
    fun applyToDialog(dialog: Dialog, context: Context? = null) {
        runCatching {
            val window = dialog.window ?: return
            val decor = window.decorView
            val design = getDesignStyle()
            val themeIndex = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
            styleViewHierarchy(decor, design, themeIndex)
            decor.post {
                runCatching {
                    styleViewHierarchy(decor, design, themeIndex)
                }
            }
        }
    }

    @JvmStatic
    @JvmOverloads
    fun applyToViewTree(root: View, context: Context? = null) {
        runCatching {
            val design = getDesignStyle()
            val themeIndex = runCatching { IntSetting.THEME_MODE.getInt() }.getOrDefault(3)
            styleViewHierarchy(root, design, themeIndex)
        }
    }

    @JvmStatic
    fun reapply(activity: Activity) {
        runCatching {
            val decor = activity.window?.decorView ?: return
            clearStyleTags(decor)
            applyToActivity(activity)
        }
    }

    private fun cleanup(activity: Activity) {
        runCatching {
            val listener = activeListeners.remove(activity) ?: return
            val vto = activity.window?.decorView?.viewTreeObserver
            if (vto != null && vto.isAlive) {
                vto.removeOnGlobalLayoutListener(listener)
            }
        }
    }

    private fun getDesignStyle(): Int {
        return runCatching { IntSetting.UI_DESIGN_STYLE.getInt() }
            .getOrDefault(DESIGN_HYPERION_GLASS_3D)
            .coerceIn(DESIGN_HYPERION_GLASS_3D, DESIGN_CHRONO_NEXUS_COMMANDER)
    }

    private fun clearStyleTags(view: View) {
        view.setTag(R.id.tag_storm_design_applied, null)
        if (view is ViewGroup) {
            for (i in 0 until view.childCount) {
                clearStyleTags(view.getChildAt(i))
            }
        }
    }

    private fun styleViewHierarchy(root: View, design: Int, themeIndex: Int) {
        val signature = (design shl 8) or (themeIndex and 0xFF)
        if (root.getTag(R.id.tag_storm_design_applied) != signature) {
            root.setTag(R.id.tag_storm_design_applied, signature)
            styleSingleView(root, design)
        }

        if (root is ViewGroup) {
            for (i in 0 until root.childCount) {
                val child = root.getChildAt(i) ?: continue
                styleViewHierarchy(child, design, themeIndex)
            }
        }
    }

    private fun styleSingleView(view: View, design: Int) {
        val idName = runCatching {
            if (view.id != View.NO_ID) view.resources.getResourceEntryName(view.id) else ""
        }.getOrDefault("")

        if (idName.contains("surface_emulation", ignoreCase = true) ||
            idName.contains("overlay", ignoreCase = true) ||
            view.javaClass.name.contains("Overlay", ignoreCase = true)
        ) {
            return
        }

        when (view) {
            is MaterialCardView -> styleCard(view, design, idName)
            is MaterialButton -> styleButton(view, design)
            is TextInputLayout -> styleTextInputLayout(view, design)
            is Chip -> styleChip(view, design)
            is EditText -> styleEditText(view, design)
        }
    }

    private fun styleCard(card: MaterialCardView, design: Int, idName: String) {
        val context = card.context
        val density = context.resources.displayMetrics.density

        val isCarouselOrCompactCover =
            idName == "card_game_carousel" || idName == "card_game_grid_compact"
        if (isCarouselOrCompactCover) {
            card.strokeWidth = 0
            return
        }

        val surfaceContainer = resolveColor(
            context,
            com.google.android.material.R.attr.colorSurfaceVariant,
            resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF161E2E.toInt())
        )

        // Zero border-stroke glow across all designs
        card.strokeWidth = 0

        when (design) {
            DESIGN_HYPERION_GLASS_3D -> {
                card.radius = 14f * density
                card.cardElevation = 4f * density
                card.setCardBackgroundColor(setAlpha(surfaceContainer, 0xEC))
            }
            DESIGN_CYBER_DECK_TACTILE -> {
                card.radius = 6f * density
                card.cardElevation = 2f * density
                card.setCardBackgroundColor(surfaceContainer)
            }
            DESIGN_AURORA_QUANTUM -> {
                card.radius = 18f * density
                card.cardElevation = 4f * density
                card.setCardBackgroundColor(setAlpha(surfaceContainer, 0xF2))
            }
            DESIGN_ORBITAL_SPATIAL_UI -> {
                card.radius = 16f * density
                card.cardElevation = 5f * density
                card.setCardBackgroundColor(setAlpha(surfaceContainer, 0xF0))
            }
            DESIGN_CHRONO_NEXUS_COMMANDER -> {
                card.radius = 8f * density
                card.cardElevation = 3f * density
                card.setCardBackgroundColor(surfaceContainer)
            }
        }

        val isExpandableContainer =
            card.findViewById<View>(R.id.radio_group) != null ||
                card.findViewById<View>(R.id.slider_container) != null ||
                card.findViewById<View>(R.id.switch_container) != null
        if (!isExpandableContainer && (card.isClickable || card.isFocusable)) {
            applyTouchFeedback(card, design, density)
        }
    }

    private fun styleButton(button: MaterialButton, design: Int) {
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

        // Zero faceted/glowing border stroke on all buttons
        button.strokeWidth = 0
        button.insetTop = 0
        button.insetBottom = 0

        when (design) {
            DESIGN_HYPERION_GLASS_3D -> {
                button.cornerRadius = (12f * density).roundToInt()
                button.elevation = 2f * density
            }
            DESIGN_CYBER_DECK_TACTILE -> {
                button.cornerRadius = (6f * density).roundToInt()
                button.elevation = 2f * density
            }
            DESIGN_AURORA_QUANTUM -> {
                button.cornerRadius = (18f * density).roundToInt()
                button.elevation = 2f * density
            }
            DESIGN_ORBITAL_SPATIAL_UI -> {
                button.cornerRadius = (14f * density).roundToInt()
                button.elevation = 3f * density
            }
            DESIGN_CHRONO_NEXUS_COMMANDER -> {
                button.cornerRadius = (8f * density).roundToInt()
                button.elevation = 2f * density
            }
        }

        applyTouchFeedback(button, design, density)
    }

    private fun styleTextInputLayout(layout: TextInputLayout, design: Int) {
        val context = layout.context
        val density = context.resources.displayMetrics.density

        val radius = when (design) {
            DESIGN_HYPERION_GLASS_3D -> 12f * density
            DESIGN_CYBER_DECK_TACTILE -> 6f * density
            DESIGN_AURORA_QUANTUM -> 16f * density
            DESIGN_ORBITAL_SPATIAL_UI -> 14f * density
            DESIGN_CHRONO_NEXUS_COMMANDER -> 8f * density
            else -> 10f * density
        }

        layout.setBoxCornerRadii(radius, radius, radius, radius)
        layout.boxStrokeWidth = 0
        layout.boxStrokeWidthFocused = 0
    }

    private fun styleChip(chip: Chip, design: Int) {
        val context = chip.context
        val density = context.resources.displayMetrics.density

        chip.chipStrokeWidth = 0f
        when (design) {
            DESIGN_HYPERION_GLASS_3D -> chip.chipCornerRadius = 12f * density
            DESIGN_CYBER_DECK_TACTILE -> chip.chipCornerRadius = 6f * density
            DESIGN_AURORA_QUANTUM -> chip.chipCornerRadius = 18f * density
            DESIGN_ORBITAL_SPATIAL_UI -> chip.chipCornerRadius = 14f * density
            DESIGN_CHRONO_NEXUS_COMMANDER -> chip.chipCornerRadius = 8f * density
        }

        applyTouchFeedback(chip, design, density)
    }

    private fun isEmbeddedOrSearchEditText(editText: EditText): Boolean {
        if (editText is TextInputEditText) {
            return true
        }
        val idName = runCatching {
            if (editText.id != View.NO_ID) editText.resources.getResourceEntryName(editText.id) else ""
        }.getOrDefault("")
        if (idName.contains("search", ignoreCase = true)) {
            return true
        }

        var parent = editText.parent
        var depth = 0
        while (parent is View && depth < 4) {
            val parentView = parent as View
            if (parentView is TextInputLayout ||
                parentView.javaClass.name.contains("SearchView") ||
                parentView is androidx.appcompat.widget.SearchView ||
                parentView is android.widget.SearchView
            ) {
                return true
            }
            val parentIdName = runCatching {
                if (parentView.id != View.NO_ID) parentView.resources.getResourceEntryName(parentView.id) else ""
            }.getOrDefault("")
            if (parentIdName.contains("search", ignoreCase = true)) {
                return true
            }
            parent = parentView.parent
            depth++
        }
        return false
    }

    private fun styleEditText(editText: EditText, design: Int) {
        val context = editText.context
        val density = context.resources.displayMetrics.density

        if (isEmbeddedOrSearchEditText(editText)) {
            var isInsideSearchContainer = false
            var p = editText.parent
            var d = 0
            while (p is View && d < 4) {
                val pv = p as View
                val pid = runCatching {
                    if (pv.id != View.NO_ID) pv.resources.getResourceEntryName(pv.id) else ""
                }.getOrDefault("")
                if (pid.contains("search_container", ignoreCase = true) ||
                    pid.contains("search_background", ignoreCase = true)
                ) {
                    isInsideSearchContainer = true
                    break
                }
                p = pv.parent
                d++
            }
            if (isInsideSearchContainer) {
                editText.background = null
                editText.gravity = Gravity.CENTER_VERTICAL or Gravity.START
                editText.includeFontPadding = false
                editText.setPadding(0, 0, 0, 0)
            }

            val onSurfaceColor = resolveColor(context, com.google.android.material.R.attr.colorOnSurface, Color.WHITE)
            val onSurfaceVariantColor = resolveColor(context, com.google.android.material.R.attr.colorOnSurfaceVariant, 0xFF94A3B8.toInt())
            editText.setTextColor(onSurfaceColor)
            editText.setHintTextColor(onSurfaceVariantColor)
            return
        }

        val surfaceColor = resolveColor(
            context,
            com.google.android.material.R.attr.colorSurfaceVariant,
            resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF182234.toInt())
        )
        val r = when (design) {
            DESIGN_HYPERION_GLASS_3D -> 12f * density
            DESIGN_CYBER_DECK_TACTILE -> 6f * density
            DESIGN_AURORA_QUANTUM -> 16f * density
            DESIGN_ORBITAL_SPATIAL_UI -> 14f * density
            DESIGN_CHRONO_NEXUS_COMMANDER -> 8f * density
            else -> 10f * density
        }

        runCatching {
            val padH = (12f * density).roundToInt()
            val padV = (8f * density).roundToInt()
            val bg = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = r
                setColor(setAlpha(surfaceColor, 0xF2))
            }
            editText.background = bg
            if (editText.paddingLeft == 0 && editText.paddingRight == 0) {
                editText.setPadding(padH, padV, padH, padV)
            }
        }
    }

    private fun applyWindowDecorBackground(context: Context, window: Window, design: Int) {
        val surfaceColor = resolveColor(context, com.google.android.material.R.attr.colorSurface, 0xFF121826.toInt())
        val primaryColor = resolveColor(context, com.google.android.material.R.attr.colorPrimary, 0xFF8B5CF6.toInt())

        val gradient = when (design) {
            DESIGN_HYPERION_GLASS_3D -> GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(
                    blend(surfaceColor, Color.WHITE, 0.03f),
                    surfaceColor,
                    blend(surfaceColor, Color.BLACK, 0.12f)
                )
            )
            DESIGN_CYBER_DECK_TACTILE -> GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.04f),
                    blend(surfaceColor, Color.BLACK, 0.18f)
                )
            )
            DESIGN_AURORA_QUANTUM -> GradientDrawable(
                GradientDrawable.Orientation.BL_TR,
                intArrayOf(
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.08f),
                    blend(surfaceColor, Color.BLACK, 0.08f)
                )
            )
            DESIGN_ORBITAL_SPATIAL_UI -> GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(
                    blend(surfaceColor, Color.WHITE, 0.04f),
                    surfaceColor,
                    blend(surfaceColor, primaryColor, 0.04f)
                )
            )
            DESIGN_CHRONO_NEXUS_COMMANDER -> GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                intArrayOf(
                    blend(surfaceColor, Color.BLACK, 0.08f),
                    surfaceColor,
                    blend(surfaceColor, Color.BLACK, 0.14f)
                )
            )
            else -> null
        }

        gradient?.let {
            window.setBackgroundDrawable(it)
        }
    }

    private fun applyTouchFeedback(view: View, design: Int, density: Float) {
        if (!view.isClickable && !view.isFocusable) return
        val originalTranslationZ = view.translationZ
        val originalTranslationY = view.translationY

        view.setOnTouchListener { v, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    when (design) {
                        DESIGN_HYPERION_GLASS_3D -> {
                            v.animate().scaleX(0.97f).scaleY(0.97f).setDuration(100)
                                .setInterpolator(DecelerateInterpolator()).start()
                        }
                        DESIGN_CYBER_DECK_TACTILE -> {
                            v.animate().scaleX(0.96f).scaleY(0.96f).setDuration(80)
                                .setInterpolator(LinearInterpolator()).start()
                            runCatching {
                                v.performHapticFeedback(
                                    HapticFeedbackConstants.KEYBOARD_TAP,
                                    HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING
                                )
                            }
                        }
                        DESIGN_AURORA_QUANTUM -> {
                            v.animate().scaleX(0.97f).scaleY(0.97f)
                                .translationZ(originalTranslationZ + 2f * density)
                                .setDuration(110).setInterpolator(DecelerateInterpolator()).start()
                        }
                        DESIGN_ORBITAL_SPATIAL_UI -> {
                            v.animate().scaleX(0.97f).scaleY(0.97f)
                                .translationZ(originalTranslationZ + 3f * density)
                                .setDuration(120).setInterpolator(DecelerateInterpolator()).start()
                        }
                        DESIGN_CHRONO_NEXUS_COMMANDER -> {
                            v.animate().translationY(originalTranslationY + 1.5f * density)
                                .setDuration(70).setInterpolator(LinearInterpolator()).start()
                        }
                    }
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    v.animate()
                        .scaleX(1.0f)
                        .scaleY(1.0f)
                        .translationZ(originalTranslationZ)
                        .translationY(originalTranslationY)
                        .setDuration(160)
                        .setInterpolator(OvershootInterpolator(1.1f))
                        .start()
                }
            }
            false
        }
    }

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
