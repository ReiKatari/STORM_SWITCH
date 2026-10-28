// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.WindowCompat
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogSaveStateManagerBinding
import org.yuzu.yuzu_emu.databinding.ItemSaveStateSlotBinding
import org.yuzu.yuzu_emu.model.Game
import java.io.File

class SaveStateManagerDialogFragment : DialogFragment() {

    private var _binding: DialogSaveStateManagerBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null

    companion object {
        const val TAG = "SaveStateManagerDialogFragment"
        private const val ARG_GAME = "game"

        fun newInstance(game: Game?): SaveStateManagerDialogFragment {
            val fragment = SaveStateManagerDialogFragment()
            val args = Bundle()
            if (game != null) {
                args.putParcelable(ARG_GAME, game)
            }
            fragment.arguments = args
            fragment.game = game
            return fragment
        }
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogSaveStateManagerBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        val currentGame = game ?: arguments?.getParcelable(ARG_GAME)
        val gameTitle = currentGame?.title ?: "STORM SWITCH"
        binding.textGameTitle.text = "$gameTitle • SESS v2"

        binding.btnClose.setOnClickListener {
            dismiss()
        }

        refreshAllSlots()
    }

    fun refreshAllSlots() {
        bindSlot(1, binding.cardSlot1)
        bindSlot(2, binding.cardSlot2)
        bindSlot(3, binding.cardSlot3)
        bindSlot(4, binding.cardSlot4)
        bindSlot(5, binding.cardSlot5)
    }

    private fun bindSlot(slot: Int, slotBinding: ItemSaveStateSlotBinding) {
        val info = try {
            NativeLibrary.getSaveStateSlotInfo(slot)
        } catch (e: Exception) {
            arrayOf("0", "", "", "", "")
        }

        val exists = info.getOrNull(0) == "1"
        val timestamp = info.getOrNull(1) ?: ""
        val sizeMb = info.getOrNull(2) ?: ""
        val previewPath = info.getOrNull(4) ?: ""

        slotBinding.slotTitle.text = getString(R.string.save_state_slot_fmt, slot)

        if (exists) {
            slotBinding.slotMetadata.text = if (timestamp.isNotEmpty()) "$timestamp • $sizeMb" else sizeMb
            slotBinding.slotMetadata.setTextColor(Color.parseColor("#94A3B8"))
            slotBinding.btnLoad.isEnabled = true
            slotBinding.btnLoad.alpha = 1.0f
            slotBinding.btnDelete.isEnabled = true
            slotBinding.btnDelete.visibility = View.VISIBLE
        } else {
            slotBinding.slotMetadata.text = getString(R.string.save_state_empty)
            slotBinding.slotMetadata.setTextColor(Color.parseColor("#64748B"))
            slotBinding.btnLoad.isEnabled = false
            slotBinding.btnLoad.alpha = 0.4f
            slotBinding.btnDelete.isEnabled = false
            slotBinding.btnDelete.visibility = View.INVISIBLE
        }

        if (previewPath.isNotEmpty() && File(previewPath).exists()) {
            try {
                val bmp = BitmapFactory.decodeFile(previewPath)
                if (bmp != null) {
                    slotBinding.previewImage.setImageBitmap(bmp)
                } else {
                    slotBinding.previewImage.setImageResource(R.drawable.default_icon)
                }
            } catch (e: Exception) {
                slotBinding.previewImage.setImageResource(R.drawable.default_icon)
            }
        } else {
            slotBinding.previewImage.setImageResource(R.drawable.default_icon)
        }

        slotBinding.btnSave.setOnClickListener {
            val emulationFragment = (activity as? org.yuzu.yuzu_emu.activities.EmulationActivity)?.supportFragmentManager
                ?.findFragmentById(R.id.fragment_container)?.childFragmentManager
                ?.fragments?.firstOrNull { it is EmulationFragment } as? EmulationFragment

            if (previewPath.isNotEmpty()) {
                emulationFragment?.captureSaveStateScreenshot(previewPath)
            }

            val res = NativeLibrary.saveStateSlot(slot, previewPath)
            if (res == 0) {
                Toast.makeText(requireContext(), getString(R.string.save_state_saved_toast, slot), Toast.LENGTH_SHORT).show()
                refreshAllSlots()
            } else {
                Toast.makeText(requireContext(), getString(R.string.save_state_save_failed, slot), Toast.LENGTH_LONG).show()
            }
        }

        slotBinding.btnLoad.setOnClickListener {
            val res = NativeLibrary.loadStateSlot(slot)
            if (res == 0) {
                Toast.makeText(requireContext(), getString(R.string.save_state_loaded_toast, slot), Toast.LENGTH_SHORT).show()
                dismiss()
            } else {
                Toast.makeText(requireContext(), getString(R.string.save_state_load_failed, slot), Toast.LENGTH_LONG).show()
            }
        }

        slotBinding.btnDelete.setOnClickListener {
            MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.save_state_slot_fmt, slot))
                .setMessage(getString(R.string.save_state_confirm_delete, slot))
                .setPositiveButton(R.string.delete) { _, _ ->
                    NativeLibrary.deleteSaveStateSlot(slot)
                    Toast.makeText(requireContext(), getString(R.string.save_state_deleted_toast, slot), Toast.LENGTH_SHORT).show()
                    refreshAllSlots()
                }
                .setNegativeButton(android.R.string.cancel, null)
                .show()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
