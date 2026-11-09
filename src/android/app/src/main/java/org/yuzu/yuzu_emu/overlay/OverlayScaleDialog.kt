// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay

import android.app.Dialog
import android.content.Context
import android.view.Gravity
import android.view.LayoutInflater
import android.view.WindowManager
import android.widget.TextView
import com.google.android.material.button.MaterialButton
import com.google.android.material.slider.Slider
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.overlay.model.OverlayControl
import org.yuzu.yuzu_emu.overlay.model.OverlayControlData

class OverlayScaleDialog(
    context: Context,
    private val overlayControlData: OverlayControlData,
    private val onConfigChanged: (scale: Float, opacity: Float) -> Unit
) : Dialog(context) {

    private var currentScale = overlayControlData.individualScale
    private val originalScale = overlayControlData.individualScale
    private var currentOpacity = overlayControlData.individualOpacity
    private val originalOpacity = overlayControlData.individualOpacity

    private lateinit var dialogTitleText: TextView
    private lateinit var scaleValueText: TextView
    private lateinit var scaleSlider: Slider
    private lateinit var opacityValueText: TextView
    private lateinit var opacitySlider: Slider

    init {
        setupDialog()
    }

    private fun getControlTitle(): String {
        return when (overlayControlData.id) {
            OverlayControl.BUTTON_A.id -> context.getString(R.string.overlay_ctrl_button_a)
            OverlayControl.BUTTON_B.id -> context.getString(R.string.overlay_ctrl_button_b)
            OverlayControl.BUTTON_X.id -> context.getString(R.string.overlay_ctrl_button_x)
            OverlayControl.BUTTON_Y.id -> context.getString(R.string.overlay_ctrl_button_y)
            OverlayControl.BUTTON_PLUS.id -> context.getString(R.string.overlay_ctrl_button_plus)
            OverlayControl.BUTTON_MINUS.id -> context.getString(R.string.overlay_ctrl_button_minus)
            OverlayControl.BUTTON_HOME.id -> context.getString(R.string.overlay_ctrl_button_home)
            OverlayControl.BUTTON_CAPTURE.id -> context.getString(R.string.overlay_ctrl_button_capture)
            OverlayControl.BUTTON_L.id -> context.getString(R.string.overlay_ctrl_button_l)
            OverlayControl.BUTTON_R.id -> context.getString(R.string.overlay_ctrl_button_r)
            OverlayControl.BUTTON_ZL.id -> context.getString(R.string.overlay_ctrl_button_zl)
            OverlayControl.BUTTON_ZR.id -> context.getString(R.string.overlay_ctrl_button_zr)
            OverlayControl.BUTTON_STICK_L.id -> context.getString(R.string.overlay_ctrl_stick_l3)
            OverlayControl.BUTTON_STICK_R.id -> context.getString(R.string.overlay_ctrl_stick_r3)
            OverlayControl.STICK_L.id -> context.getString(R.string.overlay_ctrl_stick_l)
            OverlayControl.STICK_R.id -> context.getString(R.string.overlay_ctrl_stick_r)
            OverlayControl.COMBINED_DPAD.id -> context.getString(R.string.overlay_ctrl_dpad)
            else -> context.getString(R.string.emulation_control_adjust)
        }
    }

    private fun setupDialog() {
        val view = LayoutInflater.from(context).inflate(R.layout.dialog_overlay_scale, null)
        setContentView(view)

        window?.setBackgroundDrawable(null)

        window?.apply {
            attributes = attributes.apply {
                flags = flags and WindowManager.LayoutParams.FLAG_DIM_BEHIND.inv()
                flags = flags or WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
            }
        }

        dialogTitleText = view.findViewById(R.id.dialogTitleText)
        scaleValueText = view.findViewById(R.id.scaleValueText)
        scaleSlider = view.findViewById(R.id.scaleSlider)
        opacityValueText = view.findViewById(R.id.opacityValueText)
        opacitySlider = view.findViewById(R.id.opacitySlider)

        val resetButton = view.findViewById<MaterialButton>(R.id.resetButton)
        val confirmButton = view.findViewById<MaterialButton>(R.id.confirmButton)
        val cancelButton = view.findViewById<MaterialButton>(R.id.cancelButton)

        dialogTitleText.text = getControlTitle()

        scaleValueText.text = String.format("%.1fx", currentScale)
        scaleSlider.value = currentScale.coerceIn(0.5f, 4.0f)

        opacityValueText.text = String.format("%d%%", (currentOpacity * 100).toInt())
        opacitySlider.value = currentOpacity.coerceIn(0.0f, 1.0f)

        scaleSlider.addOnChangeListener { _, value, input ->
            if (input) {
                currentScale = value
                scaleValueText.text = String.format("%.1fx", currentScale)
            }
        }

        scaleSlider.addOnSliderTouchListener(object : Slider.OnSliderTouchListener {
            override fun onStartTrackingTouch(slider: Slider) {}
            override fun onStopTrackingTouch(slider: Slider) {
                onConfigChanged(currentScale, currentOpacity)
            }
        })

        opacitySlider.addOnChangeListener { _, value, input ->
            if (input) {
                currentOpacity = value
                opacityValueText.text = String.format("%d%%", (currentOpacity * 100).toInt())
            }
        }

        opacitySlider.addOnSliderTouchListener(object : Slider.OnSliderTouchListener {
            override fun onStartTrackingTouch(slider: Slider) {}
            override fun onStopTrackingTouch(slider: Slider) {
                onConfigChanged(currentScale, currentOpacity)
            }
        })

        resetButton.setOnClickListener {
            currentScale = 1.0f
            currentOpacity = 1.0f
            scaleSlider.value = 1.0f
            opacitySlider.value = 1.0f
            scaleValueText.text = String.format("%.1fx", currentScale)
            opacityValueText.text = "100%"
            onConfigChanged(currentScale, currentOpacity)
        }

        confirmButton.setOnClickListener {
            overlayControlData.individualScale = currentScale
            overlayControlData.individualOpacity = currentOpacity
            onConfigChanged(currentScale, currentOpacity)
            dismiss()
        }

        cancelButton.setOnClickListener {
            onConfigChanged(originalScale, originalOpacity)
            dismiss()
        }

        setOnCancelListener {
            onConfigChanged(originalScale, originalOpacity)
            dismiss()
        }
    }

    fun showDialog(anchorX: Int, anchorY: Int, anchorHeight: Int, anchorWidth: Int) {
        show()

        window?.let { window ->
            val layoutParams = window.attributes
            layoutParams.gravity = Gravity.TOP or Gravity.START

            val density = context.resources.displayMetrics.density
            val dialogWidthPx = (340 * density).toInt()
            val dialogHeightPx = (400 * density).toInt()

            val screenWidth = context.resources.displayMetrics.widthPixels
            val screenHeight = context.resources.displayMetrics.heightPixels

            var targetX = anchorX + anchorWidth / 2 - dialogWidthPx / 2
            var targetY = anchorY + anchorHeight / 2 - dialogHeightPx / 2

            // Constrain inside screen bounds
            targetX = targetX.coerceIn(16, (screenWidth - dialogWidthPx - 16).coerceAtLeast(16))
            targetY = targetY.coerceIn(16, (screenHeight - dialogHeightPx - 16).coerceAtLeast(16))

            layoutParams.x = targetX
            layoutParams.y = targetY
            layoutParams.width = dialogWidthPx

            window.attributes = layoutParams
        }
    }
}