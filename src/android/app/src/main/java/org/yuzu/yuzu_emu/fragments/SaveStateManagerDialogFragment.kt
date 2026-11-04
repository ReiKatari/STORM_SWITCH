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
import com.google.android.material.snackbar.Snackbar
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogSaveStateManagerBinding
import org.yuzu.yuzu_emu.databinding.ItemSaveStateSlotBinding
import org.yuzu.yuzu_emu.features.quickresume.QuickResumeStore
import org.yuzu.yuzu_emu.model.Game
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import kotlin.math.log10
import kotlin.math.pow

class SaveStateManagerDialogFragment : DialogFragment() {

    private var _binding: DialogSaveStateManagerBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null

    data class SaveStateUndoBackup(
        val slot: Int,
        val stateBackupFile: File,
        val previewBackupFile: File?,
        val originalStatePath: String,
        val originalPreviewPath: String,
        val timestamp: Long = System.currentTimeMillis()
    )

    companion object {
        const val TAG = "SaveStateManagerDialogFragment"
        private const val ARG_GAME = "game"
        private var lastUndoBackup: SaveStateUndoBackup? = null

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
        _binding = DialogSaveStateManagerBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        androidx.core.view.ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                androidx.core.view.WindowInsetsCompat.Type.systemBars() or
                androidx.core.view.WindowInsetsCompat.Type.displayCutout()
            )
            val density = resources.displayMetrics.density
            val topInset = maxOf(systemBars.top, (24 * density).toInt())
            v.setPadding(systemBars.left, topInset, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()
        val currentGame = game ?: arguments?.getParcelable(ARG_GAME)
        val gameTitle = currentGame?.title ?: "STORM SWITCH"
        binding.textGameTitle.text = "$gameTitle • SESS v2"

        // Remember last played game in QuickResumeStore
        currentGame?.let {
            QuickResumeStore.rememberLastGame(requireContext(), it)
        }

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

    private fun formatRelativeTime(timestampMs: Long): String {
        val diff = System.currentTimeMillis() - timestampMs
        return when {
            diff < 60_000L -> "Только что"
            diff < 3600_000L -> "${diff / 60_000L} мин назад"
            diff < 86400_000L -> "${diff / 3600_000L} ч назад"
            diff < 172800_000L -> "Вчера"
            else -> SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault()).format(Date(timestampMs))
        }
    }

    private fun formatFileSize(bytes: Long): String {
        if (bytes <= 0) return "0 Б"
        val units = arrayOf("Б", "КБ", "МБ", "ГБ")
        val digitGroups = (log10(bytes.toDouble()) / log10(1024.0)).toInt().coerceIn(0, 3)
        return String.format(Locale.US, "%.1f %s", bytes / 1024.0.pow(digitGroups.toDouble()), units[digitGroups])
    }

    private fun createUndoBackup(slot: Int, statePath: String, previewPath: String) {
        val stateFile = File(statePath)
        if (!stateFile.exists() || stateFile.length() == 0L) return

        val cacheDir = File(requireContext().cacheDir, "savestate_undo").apply { mkdirs() }
        val stateBackup = File(cacheDir, "slot_${slot}_undo.cst.bak")
        val previewBackup = if (previewPath.isNotEmpty() && File(previewPath).exists()) {
            File(cacheDir, "slot_${slot}_undo.png.bak")
        } else null

        runCatching {
            stateFile.copyTo(stateBackup, overwrite = true)
            if (previewBackup != null) {
                File(previewPath).copyTo(previewBackup, overwrite = true)
            }
            lastUndoBackup = SaveStateUndoBackup(
                slot = slot,
                stateBackupFile = stateBackup,
                previewBackupFile = previewBackup,
                originalStatePath = statePath,
                originalPreviewPath = previewPath
            )
        }
    }

    private fun restoreUndoBackup(backup: SaveStateUndoBackup) {
        if (!backup.stateBackupFile.exists()) return
        runCatching {
            val targetState = File(backup.originalStatePath)
            backup.stateBackupFile.copyTo(targetState, overwrite = true)
            if (backup.previewBackupFile != null && backup.previewBackupFile.exists()) {
                val targetPreview = File(backup.originalPreviewPath)
                backup.previewBackupFile.copyTo(targetPreview, overwrite = true)
            }
            Toast.makeText(
                requireContext(),
                "Слот ${backup.slot} успешно восстановлен из резервной копии!",
                Toast.LENGTH_SHORT
            ).show()
            lastUndoBackup = null
            refreshAllSlots()
        }.onFailure {
            Toast.makeText(requireContext(), "Ошибка восстановления слота: ${it.message}", Toast.LENGTH_SHORT).show()
        }
    }

    private fun showUndoSnackbar(slot: Int, message: String) {
        val backup = lastUndoBackup ?: return
        Snackbar.make(binding.root, message, Snackbar.LENGTH_LONG)
            .setAction("ОТМЕНИТЬ") {
                restoreUndoBackup(backup)
            }
            .setActionTextColor(Color.parseColor("#00F0FF"))
            .show()
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
        val statePath = info.getOrNull(3) ?: ""
        val previewPath = info.getOrNull(4) ?: ""

        val stateFile = File(statePath)
        val fileLastModified = if (stateFile.exists()) stateFile.lastModified() else 0L
        val relTime = if (fileLastModified > 0) formatRelativeTime(fileLastModified) else timestamp
        val exactSize = if (stateFile.exists() && stateFile.length() > 0) formatFileSize(stateFile.length()) else sizeMb

        slotBinding.slotTitle.text = getString(R.string.save_state_slot_fmt, slot)

        if (exists) {
            val displayMeta = if (relTime.isNotEmpty() && exactSize.isNotEmpty()) {
                "$relTime • $exactSize"
            } else if (exactSize.isNotEmpty()) {
                exactSize
            } else {
                relTime
            }
            slotBinding.slotMetadata.text = displayMeta
            slotBinding.slotMetadata.setTextColor(org.yuzu.yuzu_emu.utils.ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorOnSurfaceVariant))
            slotBinding.btnLoad.isEnabled = true
            slotBinding.btnLoad.alpha = 1.0f
            slotBinding.btnDelete.isEnabled = true
            slotBinding.btnDelete.visibility = View.VISIBLE
        } else {
            val backup = lastUndoBackup
            if (backup != null && backup.slot == slot && backup.stateBackupFile.exists()) {
                slotBinding.slotMetadata.text = "Пусто • Доступна отмена (удерживайте карточку)"
                slotBinding.slotMetadata.setTextColor(Color.parseColor("#38BDF8"))
            } else {
                slotBinding.slotMetadata.text = getString(R.string.save_state_empty)
                slotBinding.slotMetadata.setTextColor(org.yuzu.yuzu_emu.utils.ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorOutline))
            }
            slotBinding.btnLoad.isEnabled = false
            slotBinding.btnLoad.alpha = 0.4f
            slotBinding.btnDelete.isEnabled = false
            slotBinding.btnDelete.visibility = View.INVISIBLE
        }

        // Long press on card allows 1-tap Undo restore
        slotBinding.root.setOnLongClickListener {
            val backup = lastUndoBackup
            if (backup != null && backup.slot == slot && backup.stateBackupFile.exists()) {
                MaterialAlertDialogBuilder(requireContext())
                    .setTitle("Восстановление слота $slot")
                    .setMessage("Восстановить сохранение слота $slot из резервной копии (.undo.bak)?")
                    .setPositiveButton("Восстановить") { _, _ ->
                        restoreUndoBackup(backup)
                    }
                    .setNegativeButton(R.string.cancel, null)
                    .show()
                true
            } else {
                false
            }
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

            // Before overwriting existing save state: automatically backup to .undo.bak
            if (exists && statePath.isNotEmpty()) {
                createUndoBackup(slot, statePath, previewPath)
            }

            val res = NativeLibrary.saveStateSlot(slot, previewPath)
            if (res == 0) {
                androidx.preference.PreferenceManager.getDefaultSharedPreferences(requireContext()).edit().putInt("last_active_save_slot", slot).apply()
                refreshAllSlots()
                showUndoSnackbar(slot, getString(R.string.save_state_saved_toast, slot))
            } else if (res == 9) {
                Toast.makeText(requireContext(), getString(R.string.save_state_unsupported_toast), Toast.LENGTH_LONG).show()
            } else if (res == 10) {
                Toast.makeText(requireContext(), getString(R.string.save_states_mode_disabled_toast), Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(requireContext(), getString(R.string.save_state_save_failed, slot), Toast.LENGTH_LONG).show()
            }
        }

        slotBinding.btnLoad.setOnClickListener {
            val saveMode = org.yuzu.yuzu_emu.features.settings.model.IntSetting.SAVE_STATES_MODE.getInt()
            if (saveMode == 2) {
                Toast.makeText(requireContext(), getString(R.string.save_state_experimental_warning), Toast.LENGTH_LONG).show()
            }
            val res = NativeLibrary.loadStateSlot(slot)
            if (res == 0) {
                Toast.makeText(requireContext(), getString(R.string.save_state_loaded_toast, slot), Toast.LENGTH_SHORT).show()
                dismiss()
            } else if (res == 9) {
                Toast.makeText(requireContext(), getString(R.string.save_state_unsupported_toast), Toast.LENGTH_LONG).show()
            } else if (res == 10) {
                Toast.makeText(requireContext(), getString(R.string.save_states_mode_disabled_toast), Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(requireContext(), getString(R.string.save_state_load_failed, slot), Toast.LENGTH_LONG).show()
            }
        }

        slotBinding.btnDelete.setOnClickListener {
            val confirmDialog = MaterialAlertDialogBuilder(requireContext())
                .setTitle(getString(R.string.save_state_slot_fmt, slot))
                .setMessage(getString(R.string.save_state_confirm_delete, slot))
                .setPositiveButton(R.string.delete) { _, _ ->
                    // Before deleting: backup to .undo.bak for 1-tap undo recovery
                    if (statePath.isNotEmpty()) {
                        createUndoBackup(slot, statePath, previewPath)
                    }
                    NativeLibrary.deleteSaveStateSlot(slot)
                    refreshAllSlots()
                    showUndoSnackbar(slot, getString(R.string.save_state_deleted_toast, slot))
                }
                .setNegativeButton(R.string.cancel, null)
                .show()
            confirmDialog.getButton(androidx.appcompat.app.AlertDialog.BUTTON_NEGATIVE)?.let { negBtn ->
                org.yuzu.yuzu_emu.utils.ThemeHelper.styleCancelButton(negBtn)
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
