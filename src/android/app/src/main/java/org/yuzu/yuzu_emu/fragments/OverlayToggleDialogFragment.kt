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
