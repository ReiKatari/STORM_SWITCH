// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.ui

import android.animation.AnimatorInflater
import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.content.res.ColorStateList
import android.graphics.Color
import android.view.Gravity
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import com.google.android.material.button.MaterialButton
import org.yuzu.yuzu_emu.R

/**
 * STORM AURORA FLUID 3D Tactile Button Helper.
 *
 * Implements 100% compliance with STORM Ecosystem Law #3 & Android Tactile Button Standard:
 * - 3D faceted convex volumetric geometry (огранка, выпуклый вид, фаски глубины)
 * - Physical press micro-animation via [R.animator.storm_btn_press_animator]
 * - Tactile haptic feedback on touch down (HapticFeedbackConstants.VIRTUAL_KEY)
 * - Zero truncated / clipped button text (multi-line wrapping, auto-expansion, generous padding)
 * - Universal automatic lifecycle injection across all Activities, Fragments, and AlertDialogs.
 */
object StormButton3dHelper {

    enum class ButtonStyle3D {
        PRIMARY,
        SECONDARY,
        SUCCESS,
        DANGER,
        TONAL
    }

    /**
     * Install automatic 3D tactile button enhancement for this activity and all fragments/dialogs.
     */
    @JvmStatic
    fun install(activity: AppCompatActivity) {
        // Style existing decor view
        activity.window?.decorView?.let { decor ->
            decor.post { apply3dStyleToHierarchy(decor) }
        }

        // Intercept every fragment and dialog automatically
        activity.supportFragmentManager.registerFragmentLifecycleCallbacks(
            object : FragmentManager.FragmentLifecycleCallbacks() {
                override fun onFragmentViewCreated(
                    fm: FragmentManager,
                    f: Fragment,
                    v: View,
                    savedInstanceState: android.os.Bundle?
                ) {
                    v.post { apply3dStyleToHierarchy(v) }
                }

                override fun onFragmentStarted(fm: FragmentManager, f: Fragment) {
                    if (f is DialogFragment) {
                        f.dialog?.let { dialog ->
                            enhanceDialog(dialog)
                        }
                    }
                }
            },
            true
        )
    }

    /**
     * Enhance an arbitrary dialog (including MaterialAlertDialog and custom DialogFragments).
     */
    @JvmStatic
    fun enhanceDialog(dialog: Dialog) {
        dialog.window?.decorView?.let { decor ->
            decor.post {
                apply3dStyleToHierarchy(decor)
                if (dialog is AlertDialog) {
                    fixAlertDialogButtons(dialog)
                }
            }
        }
    }

    /**
     * Fix standard AlertDialog buttons (Positive, Negative, Neutral) to 3D faceted buttons
     * and ensure the button container does not clip translated text.
     */
    @JvmStatic
    fun fixAlertDialogButtons(dialog: AlertDialog) {
        val posBtn = dialog.getButton(DialogInterface.BUTTON_POSITIVE)
        val negBtn = dialog.getButton(DialogInterface.BUTTON_NEGATIVE)
        val neuBtn = dialog.getButton(DialogInterface.BUTTON_NEUTRAL)

        posBtn?.let { apply3d(it, ButtonStyle3D.PRIMARY) }
        negBtn?.let { apply3d(it, ButtonStyle3D.SECONDARY) }
        neuBtn?.let { apply3d(it, ButtonStyle3D.TONAL) }

        // Ensure button bar parent wraps properly without clipping
        val buttonBar = posBtn?.parent as? ViewGroup ?: negBtn?.parent as? ViewGroup
        if (buttonBar is LinearLayout) {
            val totalButtons = (if (posBtn != null) 1 else 0) +
                    (if (negBtn != null) 1 else 0) +
                    (if (neuBtn != null) 1 else 0)
            if (totalButtons >= 3) {
                buttonBar.orientation = LinearLayout.VERTICAL
            }
        }
    }

    /**
     * Recursively traverses a View hierarchy and upgrades all buttons to 3D faceted convex buttons.
     */
    @JvmStatic
    fun apply3dStyleToHierarchy(root: View) {
        // Protect on-screen emulation overlay and translation views from being modified
        val className = root.javaClass.name
        if (className.contains("InputOverlay") ||
            className.contains("GameTranslationOverlay") ||
            className.contains("OverlayLayout") ||
            className.contains("TouchOverlay")
        ) {
            return
        }

        if (root is Button) {
            autoStyleButton(root)
            return
        }

        if (root is ViewGroup) {
            for (i in 0 until root.childCount) {
                apply3dStyleToHierarchy(root.getChildAt(i))
            }
        }
    }

    /**
     * Automatically detect the button's semantic role and apply the matching 3D style.
     */
    @JvmStatic
    fun autoStyleButton(button: Button) {
        val text = button.text?.toString()?.trim() ?: ""
        val idName = try {
            if (button.id != View.NO_ID) button.resources.getResourceEntryName(button.id).lowercase() else ""
        } catch (_: Exception) {
            ""
        }
        val lowerText = text.lowercase()

        // Determine 3D style role
        val style = when {
            idName.contains("delete") || idName.contains("remove") ||
            lowerText.contains("delete") || lowerText.contains("удалить") ||
            lowerText.contains("remove") || lowerText.contains("стереть") -> ButtonStyle3D.DANGER

            idName.contains("apply") || idName.contains("save") || idName.contains("confirm") ||
            idName.contains("wizard") || idName.contains("primary") ||
            lowerText.contains("apply") || lowerText.contains("применить") ||
            lowerText.contains("save") || lowerText.contains("сохранить") ||
            lowerText.contains("готово") || lowerText.contains("done") -> ButtonStyle3D.PRIMARY

            idName.contains("install") || lowerText.contains("install") ||
            lowerText.contains("установить") -> ButtonStyle3D.SUCCESS

            idName.contains("share") || idName.contains("export") ||
            lowerText.contains("share") || lowerText.contains("поделиться") ||
            lowerText.contains("экспорт") -> ButtonStyle3D.TONAL

            else -> ButtonStyle3D.SECONDARY
        }

        apply3d(button, style)
    }

    /**
     * Core 3D Tactile Styling implementation.
     */
    @JvmStatic
    fun apply3d(button: Button, style: ButtonStyle3D = ButtonStyle3D.PRIMARY) {
        val context = button.context
        val density = context.resources.displayMetrics.density
        val hasText = !button.text.isNullOrBlank()

        // 1. Clear backgroundTintList so MaterialButton won't flatten multi-stop gradient into solid color
        button.backgroundTintList = null

        if (button is MaterialButton) {
            button.cornerRadius = (14 * density).toInt()
            button.strokeWidth = 0
            button.iconGravity = MaterialButton.ICON_GRAVITY_TEXT_START
            button.iconPadding = (6 * density).toInt()
        }

        // 2. Select 3D faceted convex drawable
        val bgRes = when (style) {
            ButtonStyle3D.PRIMARY -> R.drawable.storm_btn_3d_primary
            ButtonStyle3D.SECONDARY -> R.drawable.storm_btn_3d_secondary
            ButtonStyle3D.SUCCESS -> R.drawable.storm_btn_3d_success
            ButtonStyle3D.DANGER -> R.drawable.storm_btn_3d_danger
            ButtonStyle3D.TONAL -> R.drawable.storm_btn_3d_tonal
        }
        button.setBackgroundResource(bgRes)

        // 3. High-contrast typography & Zero-Truncation rules
        if (hasText) {
            button.isSingleLine = false
            button.maxLines = 3
            button.ellipsize = null
            button.isAllCaps = false
            button.gravity = Gravity.CENTER

            val minHeightPx = (46 * density).toInt()
            if (button.minHeight < minHeightPx) {
                button.minHeight = minHeightPx
            }

            val padH = (16 * density).toInt()
            val padV = (8 * density).toInt()
            button.setPadding(padH, padV, padH, padV)

            // Crisp legible text color
            val textColor = when (style) {
                ButtonStyle3D.PRIMARY,
                ButtonStyle3D.SUCCESS,
                ButtonStyle3D.DANGER,
                ButtonStyle3D.TONAL -> Color.parseColor("#FFFFFF")
                ButtonStyle3D.SECONDARY -> Color.parseColor("#F1F5F9")
            }
            button.setTextColor(textColor)
        }

        // 4. Attach 3D Tactile Press Animator
        try {
            val animator = AnimatorInflater.loadStateListAnimator(context, R.animator.storm_btn_press_animator)
            button.stateListAnimator = animator
        } catch (_: Exception) {
            // Fallback gracefully if animator is not supported
        }

        // 5. Tactile Haptic Feedback on Touch Down without consuming click event
        button.setOnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                v.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)
            }
            false
        }
    }

    @JvmStatic
    fun applyPrimary(button: Button) = apply3d(button, ButtonStyle3D.PRIMARY)

    @JvmStatic
    fun applySecondary(button: Button) = apply3d(button, ButtonStyle3D.SECONDARY)

    @JvmStatic
    fun applySuccess(button: Button) = apply3d(button, ButtonStyle3D.SUCCESS)

    @JvmStatic
    fun applyDanger(button: Button) = apply3d(button, ButtonStyle3D.DANGER)

    @JvmStatic
    fun applyTonal(button: Button) = apply3d(button, ButtonStyle3D.TONAL)
}
