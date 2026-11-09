// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.DialogFragment
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogOverlayToggleBinding
import org.yuzu.yuzu_emu.databinding.ItemOverlayToggleControlBinding
import org.yuzu.yuzu_emu.overlay.model.OverlayControl
import org.yuzu.yuzu_emu.overlay.model.OverlayControlData
import org.yuzu.yuzu_emu.utils.NativeConfig

class OverlayToggleDialogFragment : DialogFragment() {

    private var _binding: DialogOverlayToggleBinding? = null
    private val binding get() = _binding!!

    private var onControlsChangedCallback: (() -> Unit)? = null
    private lateinit var overlayControlData: MutableList<OverlayControlData>
    private lateinit var adapter: ToggleControlAdapter

    data class ControlDisplayItem(
        val control: OverlayControl,
        val name: String,
        val description: String,
        val glyph: String?,
        val iconRes: Int?,
        var enabled: Boolean
    )

    companion object {
        const val TAG = "OverlayToggleDialogFragment"

        fun newInstance(onControlsChanged: (() -> Unit)? = null): OverlayToggleDialogFragment {
            return OverlayToggleDialogFragment().apply {
                this.onControlsChangedCallback = onControlsChanged
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NO_TITLE, 0)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.setOnKeyListener { _, keyCode, event ->
            if (keyCode == android.view.KeyEvent.KEYCODE_ESCAPE && event.action == android.view.KeyEvent.ACTION_UP) {
                dismissAllowingStateLoss()
                true
            } else {
                false
            }
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogOverlayToggleBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        overlayControlData = NativeConfig.getOverlayControlData().toMutableList()

        val displayItems = buildDisplayList()

        adapter = ToggleControlAdapter(displayItems) { item, isChecked ->
            item.enabled = isChecked
            overlayControlData.firstOrNull { it.id == item.control.id }?.enabled = isChecked
        }

        val isLandscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        binding.recyclerControls.layoutManager = if (isLandscape) {
            GridLayoutManager(requireContext(), 2)
        } else {
            LinearLayoutManager(requireContext())
        }
        binding.recyclerControls.adapter = adapter

        binding.btnToggleAll.setOnClickListener {
            val anyDisabled = displayItems.any { !it.enabled }
            val newState = anyDisabled
            displayItems.forEach { item ->
                item.enabled = newState
                overlayControlData.firstOrNull { it.id == item.control.id }?.enabled = newState
            }
            adapter.notifyDataSetChanged()
        }

        binding.btnClose.setOnClickListener {
            saveAndDismiss()
        }

        binding.btnConfirm.setOnClickListener {
            saveAndDismiss()
        }
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val isLandscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
            val density = resources.displayMetrics.density
            val width = if (isLandscape) {
                (640 * density).toInt()
            } else {
                (resources.displayMetrics.widthPixels * 0.92f).toInt()
            }
            val height = (resources.displayMetrics.heightPixels * 0.85f).toInt()
            window.setLayout(width, height)
            window.setGravity(Gravity.CENTER)
        }
    }

    private fun saveAndDismiss() {
        NativeConfig.setOverlayControlData(overlayControlData.toTypedArray())
        NativeConfig.saveGlobalConfig()
        onControlsChangedCallback?.invoke()
        dismiss()
    }

    private fun buildDisplayList(): MutableList<ControlDisplayItem> {
        val list = mutableListOf<ControlDisplayItem>()
        for (ctrl in OverlayControl.entries) {
            val isEnabled = overlayControlData.firstOrNull { it.id == ctrl.id }?.enabled ?: ctrl.defaultVisibility
            val (name, desc, glyph, icon) = when (ctrl) {
                OverlayControl.BUTTON_A -> Quad(getString(R.string.overlay_ctrl_button_a), getString(R.string.overlay_ctrl_button_a_desc), "A", null)
                OverlayControl.BUTTON_B -> Quad(getString(R.string.overlay_ctrl_button_b), getString(R.string.overlay_ctrl_button_b_desc), "B", null)
                OverlayControl.BUTTON_X -> Quad(getString(R.string.overlay_ctrl_button_x), getString(R.string.overlay_ctrl_button_x_desc), "X", null)
                OverlayControl.BUTTON_Y -> Quad(getString(R.string.overlay_ctrl_button_y), getString(R.string.overlay_ctrl_button_y_desc), "Y", null)
                OverlayControl.COMBINED_DPAD -> Quad(getString(R.string.overlay_ctrl_dpad), getString(R.string.overlay_ctrl_dpad_desc), null, R.drawable.ic_options)
                OverlayControl.STICK_L -> Quad(getString(R.string.overlay_ctrl_stick_l), getString(R.string.overlay_ctrl_stick_l_desc), null, R.drawable.ic_controller)
                OverlayControl.STICK_R -> Quad(getString(R.string.overlay_ctrl_stick_r), getString(R.string.overlay_ctrl_stick_r_desc), null, R.drawable.ic_controller)
                OverlayControl.BUTTON_STICK_L -> Quad(getString(R.string.overlay_ctrl_stick_l3), getString(R.string.overlay_ctrl_stick_l3_desc), "L3", null)
                OverlayControl.BUTTON_STICK_R -> Quad(getString(R.string.overlay_ctrl_stick_r3), getString(R.string.overlay_ctrl_stick_r3_desc), "R3", null)
                OverlayControl.BUTTON_L -> Quad(getString(R.string.overlay_ctrl_button_l), getString(R.string.overlay_ctrl_button_l_desc), "L", null)
                OverlayControl.BUTTON_R -> Quad(getString(R.string.overlay_ctrl_button_r), getString(R.string.overlay_ctrl_button_r_desc), "R", null)
                OverlayControl.BUTTON_ZL -> Quad(getString(R.string.overlay_ctrl_button_zl), getString(R.string.overlay_ctrl_button_zl_desc), "ZL", null)
                OverlayControl.BUTTON_ZR -> Quad(getString(R.string.overlay_ctrl_button_zr), getString(R.string.overlay_ctrl_button_zr_desc), "ZR", null)
                OverlayControl.BUTTON_PLUS -> Quad(getString(R.string.overlay_ctrl_button_plus), getString(R.string.overlay_ctrl_button_plus_desc), "+", null)
                OverlayControl.BUTTON_MINUS -> Quad(getString(R.string.overlay_ctrl_button_minus), getString(R.string.overlay_ctrl_button_minus_desc), "−", null)
                OverlayControl.BUTTON_HOME -> Quad(getString(R.string.overlay_ctrl_button_home), getString(R.string.overlay_ctrl_button_home_desc), "⌂", null)
                OverlayControl.BUTTON_CAPTURE -> Quad(getString(R.string.overlay_ctrl_button_capture), getString(R.string.overlay_ctrl_button_capture_desc), "◉", null)
                OverlayControl.BUTTON_GL -> Quad(getString(R.string.storm_ov20_ctrl_gl), getString(R.string.storm_ov20_ctrl_gl_desc), "GL", null)
                OverlayControl.BUTTON_GR -> Quad(getString(R.string.storm_ov20_ctrl_gr), getString(R.string.storm_ov20_ctrl_gr_desc), "GR", null)
                OverlayControl.BUTTON_SL -> Quad(getString(R.string.storm_ov20_ctrl_sl), getString(R.string.storm_ov20_ctrl_sl_desc), "SL", null)
                OverlayControl.BUTTON_SR -> Quad(getString(R.string.storm_ov20_ctrl_sr), getString(R.string.storm_ov20_ctrl_sr_desc), "SR", null)
                OverlayControl.BUTTON_REWIND -> Quad(getString(R.string.storm_ov20_ctrl_rewind), getString(R.string.storm_ov20_ctrl_rewind_desc), "«", null)
                OverlayControl.BUTTON_FAST_FORWARD -> Quad(getString(R.string.storm_ov20_ctrl_fast_forward), getString(R.string.storm_ov20_ctrl_fast_forward_desc), "»", null)
                OverlayControl.BUTTON_FRAME_STEP -> Quad(getString(R.string.storm_ov20_ctrl_frame_step), getString(R.string.storm_ov20_ctrl_frame_step_desc), "▶|", null)
                OverlayControl.BUTTON_QUICK_SAVE -> Quad(getString(R.string.storm_ov20_ctrl_quick_save), getString(R.string.storm_ov20_ctrl_quick_save_desc), null, R.drawable.ic_storm_hud_quicksave)
                OverlayControl.BUTTON_QUICK_LOAD -> Quad(getString(R.string.storm_ov20_ctrl_quick_load), getString(R.string.storm_ov20_ctrl_quick_load_desc), null, R.drawable.ic_storm_hud_quickload)
                OverlayControl.BUTTON_AI_TRANSLATOR -> Quad(getString(R.string.storm_ov20_ctrl_ai_translator), getString(R.string.storm_ov20_ctrl_ai_translator_desc), null, R.drawable.ic_storm_hud_translate)
                OverlayControl.BUTTON_AUTO_FIX -> Quad(getString(R.string.storm_ov20_ctrl_auto_fix), getString(R.string.storm_ov20_ctrl_auto_fix_desc), "FIX", null)
                OverlayControl.BUTTON_TURBO_HOLD -> Quad(getString(R.string.storm_ov20_ctrl_turbo_hold), getString(R.string.storm_ov20_ctrl_turbo_hold_desc), "TRB", null)
                OverlayControl.BUTTON_PAUSE_COOLING -> Quad(getString(R.string.storm_ov20_ctrl_pause_cooling), getString(R.string.storm_ov20_ctrl_pause_cooling_desc), null, R.drawable.ic_storm_hud_temp)
                OverlayControl.BUTTON_GAME_CHAT -> Quad(getString(R.string.storm_ov20_ctrl_game_chat), getString(R.string.storm_ov20_ctrl_game_chat_desc), "C", null)
                OverlayControl.BUTTON_GYRO_TOGGLE -> Quad(getString(R.string.storm_ov20_ctrl_gyro_toggle), getString(R.string.storm_ov20_ctrl_gyro_toggle_desc), "GYR", null)
                OverlayControl.BUTTON_MOUSE_MODE -> Quad(getString(R.string.storm_ov20_ctrl_mouse_mode), getString(R.string.storm_ov20_ctrl_mouse_mode_desc), "PTR", null)
                OverlayControl.BUTTON_MACRO_M1 -> Quad(getString(R.string.storm_ov20_ctrl_macro_m1), getString(R.string.storm_ov20_ctrl_macro_m1_desc), "M1", null)
                OverlayControl.BUTTON_HIDE_HUD -> Quad(getString(R.string.storm_ov20_ctrl_hide_hud), getString(R.string.storm_ov20_ctrl_hide_hud_desc), "HUD", null)
            }
            list.add(ControlDisplayItem(ctrl, name, desc, glyph, icon, isEnabled))
        }
        return list
    }

    private data class Quad(val name: String, val desc: String, val glyph: String?, val icon: Int?)

    private inner class ToggleControlAdapter(
        private val items: List<ControlDisplayItem>,
        private val onToggle: (ControlDisplayItem, Boolean) -> Unit
    ) : RecyclerView.Adapter<ToggleControlAdapter.ViewHolder>() {

        inner class ViewHolder(val b: ItemOverlayToggleControlBinding) : RecyclerView.ViewHolder(b.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val b = ItemOverlayToggleControlBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            return ViewHolder(b)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val item = items[position]
            holder.b.textControlName.text = item.name
            holder.b.textControlDesc.text = item.description
            holder.b.switchControl.isChecked = item.enabled

            if (item.glyph != null) {
                holder.b.textControlGlyph.visibility = View.VISIBLE
                holder.b.textControlGlyph.text = item.glyph
                holder.b.iconControl.visibility = View.GONE
            } else {
                holder.b.textControlGlyph.visibility = View.GONE
                holder.b.iconControl.visibility = View.VISIBLE
                if (item.iconRes != null) {
                    holder.b.iconControl.setImageResource(item.iconRes)
                }
            }

            holder.b.cardControl.setOnClickListener {
                val newState = !item.enabled
                item.enabled = newState
                holder.b.switchControl.isChecked = newState
                onToggle(item, newState)
            }
        }

        override fun getItemCount(): Int = items.size
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
