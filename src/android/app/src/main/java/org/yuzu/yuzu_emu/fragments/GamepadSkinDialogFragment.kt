// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.fragment.app.DialogFragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.card.MaterialCardView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogGamepadSkinBinding
import org.yuzu.yuzu_emu.databinding.ItemGamepadSkinCardBinding
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import org.yuzu.yuzu_emu.views.GamepadSkinPreviewView

class GamepadSkinDialogFragment : DialogFragment() {

    companion object {
        const val TAG = "GamepadSkinDialogFragment"

        fun newInstance(onDismiss: (() -> Unit)? = null): GamepadSkinDialogFragment {
            val fragment = GamepadSkinDialogFragment()
            fragment.onDismissCallback = onDismiss
            return fragment
        }
    }

    private var onDismissCallback: (() -> Unit)? = null
    private var _binding: DialogGamepadSkinBinding? = null
    private val binding get() = _binding!!

    private var selectedIndex = 0

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        _binding = DialogGamepadSkinBinding.inflate(layoutInflater)

        selectedIndex = try {
            IntSetting.CONTROLLER_SKIN_STYLE.getInt(false).coerceIn(0, 19)
        } catch (_: Throwable) {
            0
        }

        binding.gamepadPreview.skinIndex = selectedIndex

        // Flip view button (Front <-> Rear)
        binding.btnFlipView.setOnClickListener {
            binding.gamepadPreview.showRearView = !binding.gamepadPreview.showRearView
            binding.btnFlipView.text = if (binding.gamepadPreview.showRearView) {
                getString(R.string.controller_view_front)
            } else {
                getString(R.string.controller_flip_view)
            }
        }

        // Auto-match switch
        binding.switchAutoSkin.isChecked = try {
            BooleanSetting.CONTROLLER_AUTO_GAME_SKIN.getBoolean()
        } catch (_: Throwable) {
            true
        }
        binding.switchAutoSkin.setOnCheckedChangeListener { _, isChecked ->
            BooleanSetting.CONTROLLER_AUTO_GAME_SKIN.setBoolean(isChecked)
        }

        // Recycler View of 20 Skins
        binding.recyclerSkins.layoutManager = LinearLayoutManager(requireContext())
        val adapter = SkinCardsAdapter()
        binding.recyclerSkins.adapter = adapter
        binding.recyclerSkins.scrollToPosition(selectedIndex)

        // Done button
        binding.btnDone.setOnClickListener {
            dismiss()
        }

        return MaterialAlertDialogBuilder(requireContext())
            .setView(binding.root)
            .create()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        onDismissCallback?.invoke()
        _binding = null
    }

    private inner class SkinCardsAdapter : RecyclerView.Adapter<SkinCardsAdapter.ViewHolder>() {

        inner class ViewHolder(val b: ItemGamepadSkinCardBinding) : RecyclerView.ViewHolder(b.root)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val b = ItemGamepadSkinCardBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            return ViewHolder(b)
        }

        override fun getItemCount(): Int = GamepadSkinPreviewView.SKINS.size

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            val skin = GamepadSkinPreviewView.SKINS[position]
            holder.b.textSkinTitle.text = skin.name
            holder.b.textSkinSubtitle.setText(skin.subtitleResId)

            // Mini gradient dot preview
            val dotBg = GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(skin.baseShellStart, skin.accentColor)
            ).apply {
                shape = GradientDrawable.OVAL
            }
            holder.b.viewSkinColorDot.background = dotBg

            val isSelected = position == selectedIndex
            holder.b.imageSelectedCheck.visibility = if (isSelected) View.VISIBLE else View.GONE

            if (isSelected) {
                holder.b.cardSkinItem.strokeColor = skin.accentColor
                holder.b.cardSkinItem.strokeWidth = 4
                holder.b.cardSkinItem.setCardBackgroundColor(0x3300E5FF.toInt())
            } else {
                holder.b.cardSkinItem.strokeColor = 0x33888888.toInt()
                holder.b.cardSkinItem.strokeWidth = 2
                holder.b.cardSkinItem.setCardBackgroundColor(Color.TRANSPARENT)
            }

            holder.b.cardSkinItem.setOnClickListener {
                val oldIndex = selectedIndex
                selectedIndex = position
                binding.gamepadPreview.skinIndex = position
                IntSetting.CONTROLLER_SKIN_STYLE.setInt(position)
                notifyItemChanged(oldIndex)
                notifyItemChanged(selectedIndex)
                val toastMsg = getString(R.string.controller_applied_toast, skin.name)
                Toast.makeText(requireContext(), toastMsg, Toast.LENGTH_SHORT).show()
            }
        }
    }
}
