// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.DialogInterface
import android.os.Bundle
import android.widget.Toast
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.databinding.DialogGameFixBinding
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.model.GameFixDatabase
import org.yuzu.yuzu_emu.utils.GameIconUtils
import java.util.Locale

class GameFixDialogFragment : DialogFragment() {

    private var _binding: DialogGameFixBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null
    private var onLaunchCallback: ((GameFixDatabase.LaunchMode) -> Unit)? = null

    companion object {
        const val TAG = "GameFixDialogFragment"

        fun newInstance(game: Game, onLaunch: (GameFixDatabase.LaunchMode) -> Unit): GameFixDialogFragment {
            val fragment = GameFixDialogFragment()
            fragment.game = game
            fragment.onLaunchCallback = onLaunch
            return fragment
        }
    }

    private fun sanitizeText(str: String): String {
        if (str.isEmpty()) return str
        if (str.contains("вЂ") || str.contains("вњ") || str.contains("Р") || str.contains("С")) {
            return try {
                val bytes = str.toByteArray(Charsets.ISO_8859_1)
                val decoded = String(bytes, Charsets.UTF_8)
                if (decoded.contains("•") || decoded.contains("✓") || decoded.any { it in 'а'..'я' || it in 'А'..'Я' }) {
                    decoded
                } else {
                    str
                }
            } catch (_: Exception) {
                str
            }
        }
        return str
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        _binding = DialogGameFixBinding.inflate(layoutInflater)

        val currentGame = game ?: return super.onCreateDialog(savedInstanceState)
        val profile = GameFixDatabase.getFix(currentGame)

        if (profile != null) {
            binding.textGameFixTitle.text = currentGame.title
            val hexId = GameFixDatabase.getProgramIdHex(currentGame)
            binding.textGameFixTitleId.text = "ID: $hexId"
            GameIconUtils.loadGameIcon(currentGame, binding.imageGameFixIcon)

            val isRu = Locale.getDefault().language == "ru"
            val issues = if (isRu) profile.issuesRu else profile.issuesEn
            val fixes = GameFixDatabase.getFormattedFixes(profile, isRu)
            binding.textGameFixIssues.text = sanitizeText(issues)
            binding.textGameFixRecommended.text = sanitizeText(fixes)
        }

        val isRu = Locale.getDefault().language == "ru"

        binding.cardLaunchAutoFix.setOnClickListener {
            val ctx = context
            try {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.AUTO_FIX
                GameFixDatabase.applyCleanFix(currentGame)
                if (ctx != null) {
                    val msg = if (isRu) {
                        "⚡ Авто-исправление: Применен чистый эталонный профиль"
                    } else {
                        "⚡ Auto-Fix: Clean profile applied"
                    }
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            } catch (_: Exception) {}
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.AUTO_FIX)
        }

        binding.cardLaunchAutoFixCustom.setOnClickListener {
            val ctx = context
            try {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.AUTO_FIX_WITH_CUSTOM
                GameFixDatabase.applyFixWithCustomOverrides(currentGame)
                if (ctx != null) {
                    val msg = if (isRu) {
                        "⚡ Авто-исправление: Индивидуальные настройки сохранены"
                    } else {
                        "⚡ Auto-Fix: Custom settings preserved"
                    }
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            } catch (_: Exception) {}
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.AUTO_FIX_WITH_CUSTOM)
        }

        binding.cardLaunchCustom.setOnClickListener {
            val ctx = context
            try {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CUSTOM
                GameFixDatabase.prepareCustomLaunch(currentGame)
                if (ctx != null) {
                    val msg = if (isRu) {
                        "🎮 Персональные настройки: Запуск с вашим профилем"
                    } else {
                        "🎮 Custom Settings: Launching with user profile"
                    }
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            } catch (_: Exception) {}
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.CUSTOM)
        }

        binding.cardLaunchGlobal.setOnClickListener {
            val ctx = context
            try {
                GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.GLOBAL
                GameFixDatabase.prepareGlobalLaunch(currentGame)
                if (ctx != null) {
                    val msg = if (isRu) {
                        "🌐 Глобальные настройки: Запуск с общими настройками"
                    } else {
                        "🌐 Global Settings: Launching with global profile"
                    }
                    Toast.makeText(ctx, msg, Toast.LENGTH_SHORT).show()
                }
            } catch (_: Exception) {}
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.GLOBAL)
        }

        binding.btnCancelLaunch.setOnClickListener {
            GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CANCEL
            val cb = onLaunchCallback
            dismissAllowingStateLoss()
            cb?.invoke(GameFixDatabase.LaunchMode.CANCEL)
        }

        val dialog = MaterialAlertDialogBuilder(requireContext())
            .setView(binding.root)
            .create()

        dialog.window?.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(android.graphics.Color.TRANSPARENT))
        return dialog
    }

    override fun onCancel(dialog: DialogInterface) {
        super.onCancel(dialog)
        GameFixDatabase.selectedLaunchMode = GameFixDatabase.LaunchMode.CANCEL
        onLaunchCallback?.invoke(GameFixDatabase.LaunchMode.CANCEL)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
