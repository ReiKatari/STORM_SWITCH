// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.views

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.Shader
import android.util.AttributeSet
import com.google.android.material.card.MaterialCardView
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.IntSetting

/**
 * High-performance 5-Design System Adaptive CardView for STORM SWITCH.
 * Provides 5 distinct volumetric 3D styling modes, tactile animations,
 * and high-contrast dual-tone gradient borders:
 * 0: TITANIUM CYBER DECK (#00E5FF -> #0077B6)
 * 1: HYPERION DOCKED CONSOLE (#C084FC -> #8B5CF6)
 * 2: VALKYRIE TACTICAL RIG (#10B981 -> #F59E0B)
 * 3: SOLARIS SWITCH 2 PRO (#FF4B3A -> #00C3E3)
 * 4: AETHER FROSTED STUDIO (#14B8A6 -> #6366F1)
 */
class GradientBorderCardView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : MaterialCardView(context, attrs, defStyleAttr) {

    companion object {
        const val STYLE_TITANIUM_CYBER_DECK = 0
        const val STYLE_HYPERION_DOCKED_CONSOLE = 1
        const val STYLE_VALKYRIE_TACTICAL_RIG = 2
        const val STYLE_SOLARIS_SWITCH_2_PRO = 3
        const val STYLE_AETHER_FROSTED_STUDIO = 4
    }

    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 5f
    }

    private val borderPath = Path()
    private val borderRect = RectF()
    private var showGradientBorder = false
    private var currentDesignStyle = STYLE_TITANIUM_CYBER_DECK

    init {
        setWillNotDraw(false)
        updateDesignState()
    }

    private fun updateBorderState() {
        val shouldShow = isPressed || isFocused || isSelected
        if (showGradientBorder != shouldShow) {
            showGradientBorder = shouldShow
            invalidate()
        }
    }

    /**
     * Reads the active UI Design Style (0..4) and updates corners and shaders.
     */
    fun updateDesignState() {
        currentDesignStyle = try {
            IntSetting.UI_DESIGN_STYLE.getInt(false).coerceIn(0, 4)
        } catch (_: Throwable) {
            STYLE_TITANIUM_CYBER_DECK
        }
        applyDesignCorners()
        invalidate()
    }

    private fun applyDesignCorners() {
        val density = context.resources.displayMetrics.density
        val targetRadius = when (currentDesignStyle) {
            STYLE_TITANIUM_CYBER_DECK -> 6f * density
            STYLE_HYPERION_DOCKED_CONSOLE -> 14f * density
            STYLE_VALKYRIE_TACTICAL_RIG -> 8f * density
            STYLE_SOLARIS_SWITCH_2_PRO -> 18f * density
            STYLE_AETHER_FROSTED_STUDIO -> 24f * density
            else -> 12f * density
        }
        radius = targetRadius
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (w <= 0 || h <= 0) return

        val density = context.resources.displayMetrics.density
        applyDesignCorners()

        // 5 Dynamic Shader Palettes
        val (startColor, endColor, strokeW) = when (currentDesignStyle) {
            STYLE_TITANIUM_CYBER_DECK -> Triple(0xFF00E5FF.toInt(), 0xFF0077B6.toInt(), 4f * density)
            STYLE_HYPERION_DOCKED_CONSOLE -> Triple(0xFFC084FC.toInt(), 0xFF8B5CF6.toInt(), 5f * density)
            STYLE_VALKYRIE_TACTICAL_RIG -> Triple(0xFF10B981.toInt(), 0xFFF59E0B.toInt(), 4.5f * density)
            STYLE_SOLARIS_SWITCH_2_PRO -> Triple(0xFFFF4B3A.toInt(), 0xFF00C3E3.toInt(), 5f * density)
            STYLE_AETHER_FROSTED_STUDIO -> Triple(0xFF14B8A6.toInt(), 0xFF6366F1.toInt(), 5.5f * density)
            else -> Triple(0xFF00E5FF.toInt(), 0xFF8B5CF6.toInt(), 4f * density)
        }

        borderPaint.strokeWidth = strokeW
        borderPaint.shader = LinearGradient(
            0f,
            0f,
            w.toFloat(),
            h.toFloat(),
            startColor,
            endColor,
            Shader.TileMode.CLAMP
        )

        val halfStroke = strokeW / 2f
        borderRect.set(
            halfStroke,
            halfStroke,
            w - halfStroke,
            h - halfStroke
        )

        borderPath.reset()
        borderPath.addRoundRect(
            borderRect,
            radius,
            radius,
            Path.Direction.CW
        )
    }

    override fun onFocusChanged(gainFocus: Boolean, direction: Int, previouslyFocusedRect: Rect?) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect)
        updateBorderState()
    }

    override fun setSelected(selected: Boolean) {
        super.setSelected(selected)
        updateBorderState()
    }

    override fun setPressed(pressed: Boolean) {
        super.setPressed(pressed)
        updateBorderState()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (showGradientBorder) {
            canvas.drawPath(borderPath, borderPaint)
        }
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        updateDesignState()
    }
}
