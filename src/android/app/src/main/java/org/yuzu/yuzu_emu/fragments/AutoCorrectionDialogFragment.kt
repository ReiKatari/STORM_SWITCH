// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.DialogFragment
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogAutoCorrectionBinding
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.utils.GameIconUtils
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.ThemeHelper

class AutoCorrectionDialogFragment : DialogFragment() {

    private var _binding: DialogAutoCorrectionBinding? = null
    private val binding get() = _binding!!

    private var game: Game? = null

    companion object {
        const val TAG = "AutoCorrectionDialogFragment"
        private const val ARG_GAME = "game"

        fun newInstance(game: Game): AutoCorrectionDialogFragment {
            val fragment = AutoCorrectionDialogFragment()
            val args = Bundle()
            args.putParcelable(ARG_GAME, game)
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
        _binding = DialogAutoCorrectionBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()
            )
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()

        val currentGame = game ?: arguments?.getParcelable(ARG_GAME) ?: run {
            dismissAllowingStateLoss()
            return
        }

        val prefs = PreferenceManager.getDefaultSharedPreferences(requireContext())
        val prefKey = "auto_corrected_thermal_${currentGame.programId}"
        val isAutoCorrected = prefs.getBoolean(prefKey, false)

        binding.textAutoCorrectionGameTitle.text = currentGame.title
        GameIconUtils.loadGameIcon(currentGame, binding.imageAutoCorrectionIcon)

        if (isAutoCorrected) {
            binding.textAutoCorrectionStatus.text = getString(R.string.auto_correction_status_active)
            binding.textAutoCorrectionStatus.setTextColor(Color.parseColor("#00F0FF"))
        } else {
            binding.textAutoCorrectionStatus.text = getString(R.string.auto_correction_status_inactive)
            binding.textAutoCorrectionStatus.setTextColor(Color.parseColor("#EF4444"))
        }

        binding.textAutoCorrectionIssues.text = getString(R.string.auto_correction_issues_content)
        binding.textAutoCorrectionRecommended.text = getString(R.string.auto_correction_recommendations_content)
        binding.cardAutoCorrectionIssues.visibility = View.VISIBLE

        binding.btnApplyAutoCorrection.setOnClickListener {
            try {
                // Apply thermal cooling and FPS boost profile
                IntSetting.RENDERER_RESOLUTION.setInt(2) // 0.75X (540p / 810p)
                IntSetting.ASTC_RECOMPRESSION.setInt(2) // BC3 (Fast)
                BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
                BooleanSetting.FASTMEM.setBoolean(true)
                BooleanSetting.FASTMEM_EXCLUSIVES.setBoolean(true)
                BooleanSetting.ECO_THERMAL_MODE.setBoolean(true)
                BooleanSetting.ECO_FRAME_PACING.setBoolean(true)
                BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(true)
                BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(true)
                BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
                BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(true)
                BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
                IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
                BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
                BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
                BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
                IntSetting.RENDERER_NVDEC_EMULATION.setInt(2)
                IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1)
                BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
                IntSetting.MAX_ANISOTROPY.setInt(1) // 1X

                if (NativeConfig.isPerGameConfigLoaded()) {
                    NativeConfig.savePerGameConfig()
                } else {
                    NativeConfig.saveGlobalConfig()
                }

                prefs.edit().putBoolean(prefKey, true).apply()
                Toast.makeText(requireContext(), getString(R.string.auto_correction_applied_toast), Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(requireContext(), "Error: ${e.message}", Toast.LENGTH_SHORT).show()
            }
            dismissAllowingStateLoss()
        }

        binding.btnResetAutoCorrection.setOnClickListener {
            try {
                // Reset thermal cooling profile to standard values
                IntSetting.RENDERER_RESOLUTION.setInt(3) // 1.0X (720p / 1080p)
                IntSetting.ASTC_RECOMPRESSION.setInt(0) // Uncompressed
                BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
                BooleanSetting.ECO_THERMAL_MODE.setBoolean(false)
                BooleanSetting.ECO_FRAME_PACING.setBoolean(false)
                BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(false)
                BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(false)
                BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
                BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(false)
                BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
                IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
                BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
                BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
                BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
                IntSetting.RENDERER_NVDEC_EMULATION.setInt(2)
                IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1)

                if (NativeConfig.isPerGameConfigLoaded()) {
                    NativeConfig.savePerGameConfig()
                } else {
                    NativeConfig.saveGlobalConfig()
                }

                prefs.edit().putBoolean(prefKey, false).apply()
                Toast.makeText(requireContext(), getString(R.string.auto_correction_reset_toast), Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                Toast.makeText(requireContext(), "Error: ${e.message}", Toast.LENGTH_SHORT).show()
            }
            dismissAllowingStateLoss()
        }

        binding.btnCloseAutoCorrection.setOnClickListener {
            dismissAllowingStateLoss()
        }
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        dialog?.window?.let { window ->
            window.setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
