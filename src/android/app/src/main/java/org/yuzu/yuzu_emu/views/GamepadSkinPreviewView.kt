// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.views

import android.content.Context
import android.graphics.Camera
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RadialGradient
import android.graphics.RectF
import android.graphics.Shader
import android.graphics.Typeface
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import java.util.Locale
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.sin

/**
 * Highly detailed 3D Vector Canvas Switch 2 Pro Controller Replica.
 * Features 20 collectible gaming skins, interactive 3D perspective tilt,
 * Front / Rear View (GL / GR grip paddles) toggling, and 6-Axis Gyro reticle.
 */
class GamepadSkinPreviewView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    data class SkinDefinition(
        val id: Int,
        val name: String,
        val subtitleResId: Int,
        val baseShellStart: Int,
        val baseShellEnd: Int,
        val gripColor: Int,
        val accentColor: Int,
        val secondaryAccent: Int,
        val buttonColors: IntArray, // A, B, X, Y
        val dpadColor: Int,
        val stickBaseColor: Int,
        val stickCapColor: Int,
        val patternType: Int, // 0: None, 1: Carbon, 2: Zonai, 3: Wonder, 4: PokeSplit, 5: Smash, 6: Splat, 7: Runes, 8: Snes
        val rearShellColor: Int,
        val rearPaddleColor: Int
    )

    companion object {
        const val PATTERN_NONE = 0
        const val PATTERN_CARBON = 1
        const val PATTERN_ZONAI = 2
        const val PATTERN_WONDER = 3
        const val PATTERN_POKE_SPLIT = 4
        const val PATTERN_SMASH = 5
        const val PATTERN_SPLAT = 6
        const val PATTERN_RUNES = 7
        const val PATTERN_SNES = 8

        val SKINS = arrayOf(
            // 0: Carbon Stealth
            SkinDefinition(0, "Carbon Stealth", R.string.controller_skin_sub_0, 0xFF1E2024.toInt(), 0xFF121316.toInt(), 0xFF0D0E10.toInt(), 0xFF00F0FF.toInt(), 0xFF0077B6.toInt(), intArrayOf(0xFF1B1C20.toInt(), 0xFF1B1C20.toInt(), 0xFF1B1C20.toInt(), 0xFF1B1C20.toInt()), 0xFF181A1E.toInt(), 0xFF15161A.toInt(), 0xFF00F0FF.toInt(), PATTERN_CARBON, 0xFF121316.toInt(), 0xFF00F0FF.toInt()),
            // 1: STORM Aurora
            SkinDefinition(1, "STORM Aurora", R.string.controller_skin_sub_1, 0xFF1B143F.toInt(), 0xFF0A081A.toInt(), 0xFF2A1B5E.toInt(), 0xFF00E5FF.toInt(), 0xFF8B5CF6.toInt(), intArrayOf(0xFF8B5CF6.toInt(), 0xFF8B5CF6.toInt(), 0xFF00E5FF.toInt(), 0xFF00E5FF.toInt()), 0xFF24164C.toInt(), 0xFF191136.toInt(), 0xFF00E5FF.toInt(), PATTERN_NONE, 0xFF160F33.toInt(), 0xFF8B5CF6.toInt()),
            // 2: Zelda Royal Zonai
            SkinDefinition(2, "Zelda Royal Zonai", R.string.controller_skin_sub_2, 0xFF0A2E24.toInt(), 0xFF041712.toInt(), 0xFF07211A.toInt(), 0xFFD4AF37.toInt(), 0xFF00FFCC.toInt(), intArrayOf(0xFFD4AF37.toInt(), 0xFFD4AF37.toInt(), 0xFFD4AF37.toInt(), 0xFFD4AF37.toInt()), 0xFF07211A.toInt(), 0xFF041712.toInt(), 0xFF00FFCC.toInt(), PATTERN_ZONAI, 0xFF061B15.toInt(), 0xFFD4AF37.toInt()),
            // 3: Super Mario Bros. Wonder
            SkinDefinition(3, "Super Mario Bros. Wonder", R.string.controller_skin_sub_3, 0xFF5B1863.toInt(), 0xFF2A0830.toInt(), 0xFFE52521.toInt(), 0xFF00E5FF.toInt(), 0xFFFFD700.toInt(), intArrayOf(0xFFE52521.toInt(), 0xFFFFD700.toInt(), 0xFF0051BA.toInt(), 0xFF43B047.toInt()), 0xFF350B3D.toInt(), 0xFF2A0830.toInt(), 0xFF00E5FF.toInt(), PATTERN_WONDER, 0xFF380D40.toInt(), 0xFFE52521.toInt()),
            // 4: Sonic
            SkinDefinition(4, "Sonic", R.string.controller_skin_sub_4, 0xFF0051BA.toInt(), 0xFF002255.toInt(), 0xFFE50000.toInt(), 0xFFFFD700.toInt(), 0xFFFFFFFF.toInt(), intArrayOf(0xFFFFD700.toInt(), 0xFFFFD700.toInt(), 0xFFFFFFFF.toInt(), 0xFFFFFFFF.toInt()), 0xFF003882.toInt(), 0xFF002255.toInt(), 0xFFFFD700.toInt(), PATTERN_NONE, 0xFF002F75.toInt(), 0xFFFFD700.toInt()),
            // 5: Diablo II: Resurrected
            SkinDefinition(5, "Diablo II: Resurrected", R.string.controller_skin_sub_5, 0xFF140707.toInt(), 0xFF050101.toInt(), 0xFF4A0A0A.toInt(), 0xFFFF4500.toInt(), 0xFFC5A059.toInt(), intArrayOf(0xFFC5A059.toInt(), 0xFFC5A059.toInt(), 0xFFFF4500.toInt(), 0xFFFF4500.toInt()), 0xFF200909.toInt(), 0xFF140707.toInt(), 0xFFFF4500.toInt(), PATTERN_RUNES, 0xFF1A0808.toInt(), 0xFFFF4500.toInt()),
            // 6: Assassin's Creed
            SkinDefinition(6, "Assassin\'s Creed", R.string.controller_skin_sub_6, 0xFFE8ECEF.toInt(), 0xFFB0BEC5.toInt(), 0xFF2C3E50.toInt(), 0xFF990000.toInt(), 0xFFD4AF37.toInt(), intArrayOf(0xFF990000.toInt(), 0xFF990000.toInt(), 0xFFD4AF37.toInt(), 0xFFD4AF37.toInt()), 0xFF2C3E50.toInt(), 0xFF1E2833.toInt(), 0xFF990000.toInt(), PATTERN_NONE, 0xFFCFD8DC.toInt(), 0xFF990000.toInt()),
            // 7: Donkey Kong Bananza
            SkinDefinition(7, "Donkey Kong Bananza", R.string.controller_skin_sub_7, 0xFFF5A623.toInt(), 0xFF995E05.toInt(), 0xFF4A2711.toInt(), 0xFF2ECC71.toInt(), 0xFFE74C3C.toInt(), intArrayOf(0xFFE74C3C.toInt(), 0xFFE74C3C.toInt(), 0xFF2ECC71.toInt(), 0xFF2ECC71.toInt()), 0xFF4A2711.toInt(), 0xFF351B0B.toInt(), 0xFF2ECC71.toInt(), PATTERN_NONE, 0xFF42220D.toInt(), 0xFFF5A623.toInt()),
            // 8: Metroid Dread
            SkinDefinition(8, "Metroid Dread", R.string.controller_skin_sub_8, 0xFFEDEDED.toInt(), 0xFFB8C2CC.toInt(), 0xFF2D3748.toInt(), 0xFFD63031.toInt(), 0xFF00CEC9.toInt(), intArrayOf(0xFFD63031.toInt(), 0xFFD63031.toInt(), 0xFF00CEC9.toInt(), 0xFF00CEC9.toInt()), 0xFF2D3748.toInt(), 0xFF1A202C.toInt(), 0xFF00CEC9.toInt(), PATTERN_NONE, 0xFFCBD5E0.toInt(), 0xFFD63031.toInt()),
            // 9: Castlevania
            SkinDefinition(9, "Castlevania", R.string.controller_skin_sub_9, 0xFF150B16.toInt(), 0xFF050105.toInt(), 0xFF52071A.toInt(), 0xFFE74C3C.toInt(), 0xFFBDC3C7.toInt(), intArrayOf(0xFFBDC3C7.toInt(), 0xFFBDC3C7.toInt(), 0xFFE74C3C.toInt(), 0xFFE74C3C.toInt()), 0xFF260D20.toInt(), 0xFF150B16.toInt(), 0xFFE74C3C.toInt(), PATTERN_NONE, 0xFF210A1D.toInt(), 0xFFE74C3C.toInt()),
            // 10: Xenoblade
            SkinDefinition(10, "Xenoblade", R.string.controller_skin_sub_10, 0xFFC0392B.toInt(), 0xFF6E1810.toInt(), 0xFF2C3E50.toInt(), 0xFF00A8FF.toInt(), 0xFFF1C40F.toInt(), intArrayOf(0xFF00A8FF.toInt(), 0xFF00A8FF.toInt(), 0xFFF1C40F.toInt(), 0xFFF1C40F.toInt()), 0xFF2C3E50.toInt(), 0xFF1A252F.toInt(), 0xFF00A8FF.toInt(), PATTERN_NONE, 0xFF5A150F.toInt(), 0xFF00A8FF.toInt()),
            // 11: Smash Bros
            SkinDefinition(11, "Smash Bros", R.string.controller_skin_sub_11, 0xFF18181A.toInt(), 0xFF08080A.toInt(), 0xFF2E2E33.toInt(), 0xFFFF6B00.toInt(), 0xFFFFD700.toInt(), intArrayOf(0xFFFF6B00.toInt(), 0xFFFF6B00.toInt(), 0xFFFFD700.toInt(), 0xFFFFD700.toInt()), 0xFF242429.toInt(), 0xFF18181A.toInt(), 0xFFFF6B00.toInt(), PATTERN_SMASH, 0xFF1E1E22.toInt(), 0xFFFF6B00.toInt()),
            // 12: Splatoon 3
            SkinDefinition(12, "Splatoon 3", R.string.controller_skin_sub_12, 0xFF1E1E22.toInt(), 0xFF0C0C0E.toInt(), 0xFF6C00FF.toInt(), 0xFFEFFF04.toInt(), 0xFF6C00FF.toInt(), intArrayOf(0xFFEFFF04.toInt(), 0xFF6C00FF.toInt(), 0xFFEFFF04.toInt(), 0xFF6C00FF.toInt()), 0xFF2E2E36.toInt(), 0xFF18181C.toInt(), 0xFFEFFF04.toInt(), PATTERN_SPLAT, 0xFF18181C.toInt(), 0xFFEFFF04.toInt()),
            // 13: Monster Hunter
            SkinDefinition(13, "Monster Hunter", R.string.controller_skin_sub_13, 0xFF7A1818.toInt(), 0xFF3D0909.toInt(), 0xFF29221C.toInt(), 0xFFCD7F32.toInt(), 0xFFE67E22.toInt(), intArrayOf(0xFFCD7F32.toInt(), 0xFFCD7F32.toInt(), 0xFFE67E22.toInt(), 0xFFE67E22.toInt()), 0xFF29221C.toInt(), 0xFF1C1612.toInt(), 0xFFCD7F32.toInt(), PATTERN_NONE, 0xFF381212.toInt(), 0xFFCD7F32.toInt()),
            // 14: Pokémon
            SkinDefinition(14, "Pokémon", R.string.controller_skin_sub_14, 0xFFEE1515.toInt(), 0xFF8A0808.toInt(), 0xFF222226.toInt(), 0xFFFFFFFF.toInt(), 0xFF00D2D3.toInt(), intArrayOf(0xFFFFFFFF.toInt(), 0xFFFFFFFF.toInt(), 0xFFFFFFFF.toInt(), 0xFFFFFFFF.toInt()), 0xFF222226.toInt(), 0xFF141416.toInt(), 0xFF00D2D3.toInt(), PATTERN_POKE_SPLIT, 0xFF333338.toInt(), 0xFFEE1515.toInt()),
            // 15: Hollow Knight: Silksong
            SkinDefinition(15, "Hollow Knight: Silksong", R.string.controller_skin_sub_15, 0xFFA92222.toInt(), 0xFF540C0C.toInt(), 0xFF1C1C24.toInt(), 0xFFFFFFFF.toInt(), 0xFFF39C12.toInt(), intArrayOf(0xFFFFFFFF.toInt(), 0xFFFFFFFF.toInt(), 0xFFF39C12.toInt(), 0xFFF39C12.toInt()), 0xFF1C1C24.toInt(), 0xFF101016.toInt(), 0xFFFFFFFF.toInt(), PATTERN_NONE, 0xFF400E0E.toInt(), 0xFFFFFFFF.toInt()),
            // 16: Persona 5 Royal
            SkinDefinition(16, "Persona 5 Royal", R.string.controller_skin_sub_16, 0xFF141416.toInt(), 0xFF050506.toInt(), 0xFFD60012.toInt(), 0xFFFFFFFF.toInt(), 0xFFFFD700.toInt(), intArrayOf(0xFFD60012.toInt(), 0xFFD60012.toInt(), 0xFFFFFFFF.toInt(), 0xFFFFD700.toInt()), 0xFF222226.toInt(), 0xFF121214.toInt(), 0xFFD60012.toInt(), PATTERN_NONE, 0xFF1E1E22.toInt(), 0xFFD60012.toInt()),
            // 17: DOOM
            SkinDefinition(17, "DOOM", R.string.controller_skin_sub_17, 0xFF3D4C38.toInt(), 0xFF1C2419.toInt(), 0xFF1F241E.toInt(), 0xFFFF3300.toInt(), 0xFFB22222.toInt(), intArrayOf(0xFFFF3300.toInt(), 0xFFFF3300.toInt(), 0xFFB22222.toInt(), 0xFFB22222.toInt()), 0xFF1F241E.toInt(), 0xFF151A14.toInt(), 0xFFFF3300.toInt(), PATTERN_NONE, 0xFF2A3326.toInt(), 0xFFFF3300.toInt()),
            // 18: Animal Crossing
            SkinDefinition(18, "Animal Crossing", R.string.controller_skin_sub_18, 0xFFA8E6CF.toInt(), 0xFF65B89C.toInt(), 0xFFFDEDB2.toInt(), 0xFF88D4D6.toInt(), 0xFF77DD77.toInt(), intArrayOf(0xFF88D4D6.toInt(), 0xFF77DD77.toInt(), 0xFF88D4D6.toInt(), 0xFF77DD77.toInt()), 0xFF54A187.toInt(), 0xFF438A72.toInt(), 0xFF88D4D6.toInt(), PATTERN_NONE, 0xFF81CCAE.toInt(), 0xFF88D4D6.toInt()),
            // 19: Retro SNES
            SkinDefinition(19, "Retro SNES", R.string.controller_skin_sub_19, 0xFFC4C3C8.toInt(), 0xFF9E9DA4.toInt(), 0xFF8D8D94.toInt(), 0xFF5856D6.toInt(), 0xFF34C759.toInt(), intArrayOf(0xFFFF3B30.toInt(), 0xFFFFCC00.toInt(), 0xFF007AFF.toInt(), 0xFF34C759.toInt()), 0xFF5E5E66.toInt(), 0xFF4A4A52.toInt(), 0xFF5856D6.toInt(), PATTERN_SNES, 0xFF98979E.toInt(), 0xFF5856D6.toInt())
        )

        /**
         * Auto-detects matching skin index (0..19) for a running game.
         */
        @JvmStatic
        fun getAutoSkinForGame(title: String, programIdHex: String): Int {
            val t = title.lowercase(Locale.ROOT)
            val tid = programIdHex.uppercase(Locale.ROOT)

            return when {
                t.contains("zelda") || tid.startsWith("0100F2C") || tid.startsWith("01007EF") || tid.startsWith("01004DA") -> 2
                t.contains("mario") || tid.startsWith("0100151") || tid.startsWith("0100000") || tid.startsWith("0100EC3") -> 3
                t.contains("sonic") || tid.startsWith("0100067") || tid.startsWith("0100D16") -> 4
                t.contains("diablo") || tid.startsWith("0100B1A") || tid.startsWith("01001B3") -> 5
                t.contains("assassin") -> 6
                t.contains("donkey kong") || t.contains("dk bananza") -> 7
                t.contains("metroid") || tid.startsWith("0100DAF") || tid.startsWith("0100984") -> 8
                t.contains("castlevania") -> 9
                t.contains("xenoblade") || tid.startsWith("0100E95") || tid.startsWith("0100746") || tid.startsWith("0100F34") -> 10
                t.contains("smash") || tid.startsWith("01006A8") -> 11
                t.contains("splatoon") || tid.startsWith("0100C25") -> 12
                t.contains("monster hunter") || tid.startsWith("0100FBC") -> 13
                t.contains("pokémon") || t.contains("pokemon") || tid.startsWith("0100ABF") || tid.startsWith("01008F6") || tid.startsWith("0100F43") -> 14
                t.contains("hollow knight") || t.contains("silksong") -> 15
                t.contains("persona") || tid.startsWith("0100125") -> 16
                t.contains("doom") || tid.startsWith("0100F6B") -> 17
                t.contains("animal crossing") || tid.startsWith("01006F8") -> 18
                t.contains("snes") || t.contains("retro") || t.contains("classic") -> 19
                else -> 0
            }
        }
    }

    var skinIndex: Int = 0
        set(value) {
            field = value.coerceIn(0, SKINS.size - 1)
            invalidate()
        }

    var showRearView: Boolean = false
        set(value) {
            field = value
            invalidate()
        }

    var isInteractive: Boolean = true

    // 3D Tilt parameters
    private var tiltX: Float = 0f
    private var tiltY: Float = 0f
    private val camera3D = Camera()
    private val matrix3D = Matrix()

    // Paints
    private val bodyPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val shadowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val strokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
    }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.CENTER
        typeface = Typeface.DEFAULT_BOLD
    }

    // Touch handling
    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var touchDownX = 0f
    private var touchDownY = 0f

    init {
        skinIndex = try {
            IntSetting.CONTROLLER_SKIN_STYLE.getInt(false).coerceIn(0, 19)
        } catch (_: Throwable) {
            0
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (!isInteractive) return super.onTouchEvent(event)

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                touchDownX = event.x
                touchDownY = event.y
                lastTouchX = event.x
                lastTouchY = event.y
                parent?.requestDisallowInterceptTouchEvent(true)
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                val dx = event.x - lastTouchX
                val dy = event.y - lastTouchY
                tiltY = (tiltY + dx * 0.15f).coerceIn(-18f, 18f)
                tiltX = (tiltX - dy * 0.15f).coerceIn(-18f, 18f)
                lastTouchX = event.x
                lastTouchY = event.y
                invalidate()
                return true
            }

            MotionEvent.ACTION_UP -> {
                val totalDist = abs(event.x - touchDownX) + abs(event.y - touchDownY)
                if (totalDist < 12f) {
                    // Tap toggles Front/Rear view
                    showRearView = !showRearView
                    performClick()
                } else {
                    // Smooth bounce back
                    animateTiltReset()
                }
                parent?.requestDisallowInterceptTouchEvent(false)
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                animateTiltReset()
                parent?.requestDisallowInterceptTouchEvent(false)
            }
        }
        return super.onTouchEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }

    private fun animateTiltReset() {
        animate().setDuration(240).setUpdateListener {
            tiltX *= 0.7f
            tiltY *= 0.7f
            if (abs(tiltX) < 0.2f && abs(tiltY) < 0.2f) {
                tiltX = 0f
                tiltY = 0f
            }
            invalidate()
        }.start()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (width <= 0 || height <= 0) return

        val skin = SKINS[skinIndex]
        val cx = width / 2f
        val cy = height / 2f

        // Apply 3D perspective matrix
        canvas.save()
        camera3D.save()
        camera3D.rotateX(tiltX)
        camera3D.rotateY(tiltY)
        camera3D.getMatrix(matrix3D)
        camera3D.restore()

        matrix3D.preTranslate(-cx, -cy)
        matrix3D.postTranslate(cx, cy)
        canvas.concat(matrix3D)

        // Draw Controller Model
        val baseW = minOf(width * 0.90f, height * 1.55f)
        val baseH = baseW * 0.62f
        val left = cx - baseW / 2f
        val top = cy - baseH / 2f

        if (showRearView) {
            drawRearController(canvas, skin, left, top, baseW, baseH)
        } else {
            drawFrontController(canvas, skin, left, top, baseW, baseH)
        }

        // Draw View Badge (FRONT / REAR) at bottom center
        drawViewBadge(canvas, skin, cx, top + baseH + 20f)

        canvas.restore()
    }

    private fun drawFrontController(canvas: Canvas, skin: SkinDefinition, left: Float, top: Float, w: Float, h: Float) {
        val cx = left + w / 2f
        val cy = top + h / 2f

        // 1. Drop Shadow
        shadowPaint.color = 0x66000000
        canvas.drawRoundRect(left + 8f, top + 14f, left + w - 8f, top + h + 14f, 40f, 40f, shadowPaint)

        // 2. Controller Body Shell Path
        val bodyPath = Path().apply {
            moveTo(left + w * 0.22f, top + h * 0.12f)
            // Top bridge between bumpers
            lineTo(left + w * 0.78f, top + h * 0.12f)
            // Right shoulder curve
            quadTo(left + w * 0.92f, top + h * 0.12f, left + w * 0.98f, top + h * 0.35f)
            // Right wing grip flare
            quadTo(left + w * 1.02f, top + h * 0.72f, left + w * 0.88f, top + h * 0.96f)
            // Right wing bottom tip
            quadTo(left + w * 0.80f, top + h * 1.02f, left + w * 0.72f, top + h * 0.92f)
            // Inner bottom arch
            quadTo(cx, top + h * 0.78f, left + w * 0.28f, top + h * 0.92f)
            // Left wing bottom tip
            quadTo(left + w * 0.20f, top + h * 1.02f, left + w * 0.12f, top + h * 0.96f)
            // Left wing grip flare
            quadTo(left - w * 0.02f, top + h * 0.72f, left + w * 0.02f, top + h * 0.35f)
            // Left shoulder curve
            quadTo(left + w * 0.08f, top + h * 0.12f, left + w * 0.22f, top + h * 0.12f)
            close()
        }

        // Draw Shell Base with Volumetric Gradient
        bodyPaint.shader = LinearGradient(
            left, top, left, top + h,
            skin.baseShellStart, skin.baseShellEnd,
            Shader.TileMode.CLAMP
        )
        canvas.drawPath(bodyPath, bodyPaint)

        // Pattern overlays (Pokéball split, Carbon weave, Smash cross, etc.)
        drawSkinPattern(canvas, skin, bodyPath, left, top, w, h)

        // Shell Edge Accent Highlight
        strokePaint.strokeWidth = 3.5f
        strokePaint.shader = LinearGradient(
            left, top, left + w, top + h,
            skin.accentColor, skin.secondaryAccent,
            Shader.TileMode.CLAMP
        )
        canvas.drawPath(bodyPath, strokePaint)

        // Top Bumpers (L & R)
        drawBumpers(canvas, skin, left, top, w, h)

        // Left Analog Stick
        val leftStickX = left + w * 0.28f
        val leftStickY = top + h * 0.40f
        val stickRadius = w * 0.09f
        drawStick(canvas, skin, leftStickX, leftStickY, stickRadius, isLeft = true)

        // D-Pad (Cross) below Left Stick
        val dpadX = left + w * 0.36f
        val dpadY = top + h * 0.64f
        val dpadSize = w * 0.12f
        drawDPad(canvas, skin, dpadX, dpadY, dpadSize)

        // Right Action Diamond (ABXY)
        val abxyX = left + w * 0.72f
        val abxyY = top + h * 0.38f
        val abxyRadius = w * 0.095f
        drawABXY(canvas, skin, abxyX, abxyY, abxyRadius)

        // Right Analog Stick (Offset Lower)
        val rightStickX = left + w * 0.62f
        val rightStickY = top + h * 0.62f
        drawStick(canvas, skin, rightStickX, rightStickY, stickRadius, isLeft = false)

        // Center Controls: Minus, Plus, Capture, Home
        val centerBtnRadius = w * 0.022f
        drawCenterButtons(canvas, skin, cx, cy, w, h, centerBtnRadius)

        // Central Gyroscope Reticle / Sensor Horizon Level
        drawGyroReticle(canvas, skin, cx, top + h * 0.38f, w * 0.055f)

        // Player LED 1..4 Indicator
        drawPlayerLeds(canvas, skin, cx, top + h * 0.72f, w * 0.08f)
    }

    private fun drawRearController(canvas: Canvas, skin: SkinDefinition, left: Float, top: Float, w: Float, h: Float) {
        val cx = left + w / 2f
        val cy = top + h / 2f

        // Rear Shell Path (Mirrored contours)
        val rearPath = Path().apply {
            moveTo(left + w * 0.22f, top + h * 0.12f)
            lineTo(left + w * 0.78f, top + h * 0.12f)
            quadTo(left + w * 0.92f, top + h * 0.12f, left + w * 0.98f, top + h * 0.35f)
            quadTo(left + w * 1.02f, top + h * 0.72f, left + w * 0.88f, top + h * 0.96f)
            quadTo(left + w * 0.80f, top + h * 1.02f, left + w * 0.72f, top + h * 0.92f)
            quadTo(cx, top + h * 0.78f, left + w * 0.28f, top + h * 0.92f)
            quadTo(left + w * 0.20f, top + h * 1.02f, left + w * 0.12f, top + h * 0.96f)
            quadTo(left - w * 0.02f, top + h * 0.72f, left + w * 0.02f, top + h * 0.35f)
            quadTo(left + w * 0.08f, top + h * 0.12f, left + w * 0.22f, top + h * 0.12f)
            close()
        }

        bodyPaint.shader = LinearGradient(
            left, top, left, top + h,
            skin.rearShellColor, 0xFF0D0E10.toInt(),
            Shader.TileMode.CLAMP
        )
        canvas.drawPath(rearPath, bodyPaint)

        // Rear Edge Accent
        strokePaint.strokeWidth = 3f
        strokePaint.color = skin.accentColor
        strokePaint.shader = null
        canvas.drawPath(rearPath, strokePaint)

        // Programmable Grip Buttons: GL (Left Rear) & GR (Right Rear)
        val paddlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.rearPaddleColor
            style = Paint.Style.FILL
        }
        val paddleStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }

        // GL Paddle
        val glPath = Path().apply {
            moveTo(left + w * 0.26f, top + h * 0.42f)
            quadTo(left + w * 0.34f, top + h * 0.50f, left + w * 0.28f, top + h * 0.68f)
            lineTo(left + w * 0.22f, top + h * 0.62f)
            quadTo(left + w * 0.24f, top + h * 0.48f, left + w * 0.26f, top + h * 0.42f)
            close()
        }
        canvas.drawPath(glPath, paddlePaint)
        canvas.drawPath(glPath, paddleStroke)

        // GR Paddle
        val grPath = Path().apply {
            moveTo(left + w * 0.74f, top + h * 0.42f)
            quadTo(left + w * 0.66f, top + h * 0.50f, left + w * 0.72f, top + h * 0.68f)
            lineTo(left + w * 0.78f, top + h * 0.62f)
            quadTo(left + w * 0.76f, top + h * 0.48f, left + w * 0.74f, top + h * 0.42f)
            close()
        }
        canvas.drawPath(grPath, paddlePaint)
        canvas.drawPath(grPath, paddleStroke)

        // Paddle Labels: "GL" and "GR"
        textPaint.color = Color.WHITE
        textPaint.textSize = w * 0.026f
        canvas.drawText("GL", left + w * 0.27f, top + h * 0.56f, textPaint)
        canvas.drawText("GR", left + w * 0.73f, top + h * 0.56f, textPaint)

        // USB-C Charging Port & Sync Button (Top center)
        val usbcPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = 0xFF22242A.toInt()
            style = Paint.Style.FILL
        }
        canvas.drawRoundRect(cx - w * 0.05f, top + h * 0.14f, cx + w * 0.05f, top + h * 0.18f, 6f, 6f, usbcPaint)

        // Sync Button (circle)
        val syncPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            style = Paint.Style.FILL
        }
        canvas.drawCircle(cx - w * 0.09f, top + h * 0.16f, w * 0.012f, syncPaint)

        // Center Model Plate & Certification Engraving
        textPaint.color = 0xAAFFFFFF.toInt()
        textPaint.textSize = w * 0.022f
        canvas.drawText("SWITCH 2 PRO CONTROLLER", cx, cy - 10f, textPaint)
        textPaint.textSize = w * 0.018f
        textPaint.color = 0x77FFFFFF.toInt()
        canvas.drawText("STORM CERTIFIED • MODEL HAC-013 • 5V 500mA", cx, cy + 14f, textPaint)
    }

    private fun drawSkinPattern(canvas: Canvas, skin: SkinDefinition, bodyPath: Path, left: Float, top: Float, w: Float, h: Float) {
        val cx = left + w / 2f
        val cy = top + h / 2f

        canvas.save()
        canvas.clipPath(bodyPath)

        when (skin.patternType) {
            PATTERN_POKE_SPLIT -> {
                // Top half Pokéball red, bottom half pure white, center obsidian belt
                val splitPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFFF5F5F5.toInt() }
                canvas.drawRect(left, cy, left + w, top + h, splitPaint)

                // Black belt
                val beltPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFF141416.toInt() }
                canvas.drawRect(left, cy - h * 0.06f, left + w, cy + h * 0.06f, beltPaint)

                // Central circle
                val pokeOuter = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFF141416.toInt() }
                canvas.drawCircle(cx, cy, w * 0.07f, pokeOuter)
                val pokeInner = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE }
                canvas.drawCircle(cx, cy, w * 0.045f, pokeInner)
                val pokeCore = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFF00D2D3.toInt() }
                canvas.drawCircle(cx, cy, w * 0.025f, pokeCore)
            }

            PATTERN_SMASH -> {
                // Crossed fiery Smash logo
                val smashPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = skin.accentColor
                    strokeWidth = w * 0.035f
                    style = Paint.Style.STROKE
                }
                canvas.drawLine(left + w * 0.35f, top + h * 0.20f, left + w * 0.65f, top + h * 0.80f, smashPaint)
                canvas.drawLine(left + w * 0.65f, top + h * 0.30f, left + w * 0.35f, top + h * 0.70f, smashPaint)
            }

            PATTERN_CARBON -> {
                // Diagonal carbon fiber lines
                val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = 0x22FFFFFF
                    strokeWidth = 2f
                }
                var curX = left - h
                while (curX < left + w + h) {
                    canvas.drawLine(curX, top, curX + h, top + h, linePaint)
                    curX += 16f
                }
            }

            PATTERN_ZONAI -> {
                // Zonai Ancient Spiral Glyphs
                val zonaiPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = 0x4400FFCC.toInt()
                    style = Paint.Style.STROKE
                    strokeWidth = 3f
                }
                canvas.drawCircle(cx, top + h * 0.32f, w * 0.08f, zonaiPaint)
                canvas.drawCircle(cx, top + h * 0.32f, w * 0.04f, zonaiPaint)
            }

            PATTERN_WONDER -> {
                // Wonder floral stars
                val wonderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = 0x3300E5FF.toInt()
                    style = Paint.Style.STROKE
                    strokeWidth = 3f
                }
                canvas.drawCircle(left + w * 0.32f, top + h * 0.32f, w * 0.05f, wonderPaint)
                canvas.drawCircle(left + w * 0.68f, top + h * 0.55f, w * 0.05f, wonderPaint)
            }

            PATTERN_SPLAT -> {
                // Splatoon ink split
                val splatPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0x66EFFF04.toInt() }
                canvas.drawCircle(left + w * 0.25f, top + h * 0.5f, w * 0.15f, splatPaint)
                splatPaint.color = 0x666C00FF.toInt()
                canvas.drawCircle(left + w * 0.75f, top + h * 0.5f, w * 0.15f, splatPaint)
            }

            else -> {}
        }

        canvas.restore()
    }

    private fun drawBumpers(canvas: Canvas, skin: SkinDefinition, left: Float, top: Float, w: Float, h: Float) {
        val bumperPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.gripColor
            style = Paint.Style.FILL
        }
        val bumperStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }

        // Left Bumper (L)
        val lRect = RectF(left + w * 0.18f, top + h * 0.06f, left + w * 0.38f, top + h * 0.13f)
        canvas.drawRoundRect(lRect, 8f, 8f, bumperPaint)
        canvas.drawRoundRect(lRect, 8f, 8f, bumperStroke)

        // Right Bumper (R)
        val rRect = RectF(left + w * 0.62f, top + h * 0.06f, left + w * 0.82f, top + h * 0.13f)
        canvas.drawRoundRect(rRect, 8f, 8f, bumperPaint)
        canvas.drawRoundRect(rRect, 8f, 8f, bumperStroke)

        textPaint.color = Color.WHITE
        textPaint.textSize = w * 0.03f
        canvas.drawText("L", lRect.centerX(), lRect.centerY() + w * 0.01f, textPaint)
        canvas.drawText("R", rRect.centerX(), rRect.centerY() + w * 0.01f, textPaint)
    }

    private fun drawStick(canvas: Canvas, skin: SkinDefinition, x: Float, y: Float, radius: Float, isLeft: Boolean) {
        // Base Well Ring
        val wellPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = 0xFF121418.toInt()
            style = Paint.Style.FILL
        }
        canvas.drawCircle(x, y, radius, wellPaint)

        // Accent Ring
        strokePaint.strokeWidth = 3f
        strokePaint.color = skin.accentColor
        strokePaint.shader = null
        canvas.drawCircle(x, y, radius - 2f, strokePaint)

        // Cap Dome (concave 3D texture)
        val capPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(
                x, y, radius * 0.75f,
                skin.stickBaseColor, 0xFF0A0B0E.toInt(),
                Shader.TileMode.CLAMP
            )
        }
        canvas.drawCircle(x, y, radius * 0.75f, capPaint)

        // Directional Grip Dots (North, South, East, West)
        val dotPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.stickCapColor
        }
        val d = radius * 0.45f
        canvas.drawCircle(x, y - d, 2.5f, dotPaint)
        canvas.drawCircle(x, y + d, 2.5f, dotPaint)
        canvas.drawCircle(x - d, y, 2.5f, dotPaint)
        canvas.drawCircle(x + d, y, 2.5f, dotPaint)
    }

    private fun drawDPad(canvas: Canvas, skin: SkinDefinition, x: Float, y: Float, size: Float) {
        val half = size / 2f
        val thick = size * 0.34f
        val halfThick = thick / 2f

        val dpadPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.dpadColor
            style = Paint.Style.FILL
        }
        val dpadStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }

        // Horizontal bar
        canvas.drawRoundRect(x - half, y - halfThick, x + half, y + halfThick, 6f, 6f, dpadPaint)
        canvas.drawRoundRect(x - half, y - halfThick, x + half, y + halfThick, 6f, 6f, dpadStroke)

        // Vertical bar
        canvas.drawRoundRect(x - halfThick, y - half, x + halfThick, y + half, 6f, 6f, dpadPaint)
        canvas.drawRoundRect(x - halfThick, y - half, x + halfThick, y + half, 6f, 6f, dpadStroke)

        // Center concave pivot
        val pivotPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = 0xFF14161A.toInt()
        }
        canvas.drawCircle(x, y, thick * 0.45f, pivotPaint)
    }

    private fun drawABXY(canvas: Canvas, skin: SkinDefinition, x: Float, y: Float, radius: Float) {
        val btnRadius = radius * 0.32f
        val offset = radius * 0.65f

        val btnPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.FILL
        }
        val btnStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }

        textPaint.textSize = btnRadius * 1.15f
        textPaint.color = Color.WHITE

        // X (Top)
        btnPaint.color = skin.buttonColors[2]
        canvas.drawCircle(x, y - offset, btnRadius, btnPaint)
        canvas.drawCircle(x, y - offset, btnRadius, btnStroke)
        canvas.drawText("X", x, y - offset + btnRadius * 0.4f, textPaint)

        // A (Right)
        btnPaint.color = skin.buttonColors[0]
        canvas.drawCircle(x + offset, y, btnRadius, btnPaint)
        canvas.drawCircle(x + offset, y, btnRadius, btnStroke)
        canvas.drawText("A", x + offset, y + btnRadius * 0.4f, textPaint)

        // B (Bottom)
        btnPaint.color = skin.buttonColors[1]
        canvas.drawCircle(x, y + offset, btnRadius, btnPaint)
        canvas.drawCircle(x, y + offset, btnRadius, btnStroke)
        canvas.drawText("B", x, y + offset + btnRadius * 0.4f, textPaint)

        // Y (Left)
        btnPaint.color = skin.buttonColors[3]
        canvas.drawCircle(x - offset, y, btnRadius, btnPaint)
        canvas.drawCircle(x - offset, y, btnRadius, btnStroke)
        canvas.drawText("Y", x - offset, y + btnRadius * 0.4f, textPaint)
    }

    private fun drawCenterButtons(canvas: Canvas, skin: SkinDefinition, cx: Float, cy: Float, w: Float, h: Float, radius: Float) {
        val btnPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = 0xFF2B2E36.toInt()
            style = Paint.Style.FILL
        }
        textPaint.color = Color.WHITE
        textPaint.textSize = radius * 1.5f

        // Minus (-) Left
        val minusX = cx - w * 0.12f
        val minusY = cy - h * 0.12f
        canvas.drawCircle(minusX, minusY, radius, btnPaint)
        canvas.drawText("-", minusX, minusY + radius * 0.45f, textPaint)

        // Plus (+) Right
        val plusX = cx + w * 0.12f
        val plusY = cy - h * 0.12f
        canvas.drawCircle(plusX, plusY, radius, btnPaint)
        canvas.drawText("+", plusX, plusY + radius * 0.45f, textPaint)

        // Capture (Square) Lower Left
        val capX = cx - w * 0.08f
        val capY = cy + h * 0.08f
        canvas.drawRoundRect(capX - radius, capY - radius, capX + radius, capY + radius, 4f, 4f, btnPaint)
        canvas.drawCircle(capX, capY, radius * 0.4f, Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE })

        // Home (Circle) Lower Right
        val homeX = cx + w * 0.08f
        val homeY = cy + h * 0.08f
        canvas.drawCircle(homeX, homeY, radius, btnPaint)
        val homeGlow = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }
        canvas.drawCircle(homeX, homeY, radius, homeGlow)
        textPaint.textSize = radius * 1.1f
        canvas.drawText("⌂", homeX, homeY + radius * 0.35f, textPaint)
    }

    private fun drawGyroReticle(canvas: Canvas, skin: SkinDefinition, cx: Float, cy: Float, radius: Float) {
        // Gyro horizon circle
        val reticlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }
        canvas.drawCircle(cx, cy, radius, reticlePaint)

        // Dynamic tilt horizon bar (responds to 3D tilt)
        val angleRad = Math.toRadians((tiltY * 1.5f).toDouble())
        val dx = (radius * 0.7f * cos(angleRad)).toFloat()
        val dy = (radius * 0.7f * sin(angleRad)).toFloat()
        val barPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE
            strokeWidth = 2.5f
        }
        canvas.drawLine(cx - dx, cy - dy, cx + dx, cy + dy, barPaint)
        canvas.drawCircle(cx, cy, 3f, Paint(Paint.ANTI_ALIAS_FLAG).apply { color = skin.accentColor })
    }

    private fun drawPlayerLeds(canvas: Canvas, skin: SkinDefinition, cx: Float, cy: Float, width: Float) {
        val ledSpacing = width / 3f
        val ledStartX = cx - width / 2f
        val ledRadius = 3f

        val offLed = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = 0xFF22262C.toInt() }
        val onLed = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = skin.accentColor }

        for (i in 0 until 4) {
            val lx = ledStartX + i * ledSpacing
            canvas.drawCircle(lx, cy, ledRadius, if (i == 0) onLed else offLed)
        }
    }

    private fun drawViewBadge(canvas: Canvas, skin: SkinDefinition, cx: Float, y: Float) {
        val badgeText = if (showRearView) {
            context.getString(R.string.controller_view_rear)
        } else {
            context.getString(R.string.controller_view_front)
        }

        textPaint.textSize = 28f
        textPaint.color = Color.WHITE
        val textW = textPaint.measureText(badgeText)

        val pillW = textW + 50f
        val pillH = 44f
        val pillRect = RectF(cx - pillW / 2f, y - pillH / 2f, cx + pillW / 2f, y + pillH / 2f)

        val pillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = 0xDD121418.toInt()
            style = Paint.Style.FILL
        }
        val pillStroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = skin.accentColor
            strokeWidth = 2f
            style = Paint.Style.STROKE
        }

        canvas.drawRoundRect(pillRect, pillH / 2f, pillH / 2f, pillPaint)
        canvas.drawRoundRect(pillRect, pillH / 2f, pillH / 2f, pillStroke)
        canvas.drawText(badgeText, cx, y + 10f, textPaint)
    }
}
