// SPDX-FileCopyrightText: Copyright 2026 STORM TEAM
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.content.Context
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.HapticFeedbackConstants
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.fragment.app.DialogFragment
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogAutoOptimizationBinding
import org.yuzu.yuzu_emu.ui.StormButton3dHelper
import org.yuzu.yuzu_emu.utils.StormHardwareCalibrator
import org.yuzu.yuzu_emu.utils.ThemeHelper

class AutoOptimizationDialogFragment : DialogFragment() {

    private var _binding: DialogAutoOptimizationBinding? = null
    private val binding get() = _binding!!

    private var selectedMode = MODE_NORMAL

    companion object {
        const val TAG = "AutoOptimizationDialogFragment"

        const val MODE_FAST = 0
        const val MODE_NORMAL = 1
        const val MODE_ACCURATE = 2
        const val MODE_BATTERY = 3
        const val MODE_DEFAULT = 4

        @Volatile
        private var cachedProfile: StormHardwareCalibrator.DeviceHardwareProfile? = null

        fun newInstance(): AutoOptimizationDialogFragment {
            return AutoOptimizationDialogFragment()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
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
        _binding = DialogAutoOptimizationBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(view) { v, windowInsets ->
            val insets = windowInsets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()
            )
            val density = resources.displayMetrics.density
            val dp16 = (16 * density).toInt()
            val topInset = maxOf(insets.top, (28 * density).toInt()) + (16 * density).toInt()
            v.setPadding(insets.left + dp16, topInset, insets.right + dp16, insets.bottom + dp16)
            windowInsets
        }
        view.requestApplyInsets()

        binding.btnClose.setOnClickListener { dismiss() }
        binding.btnCloseWizard.setOnClickListener { dismiss() }
        binding.btnCancel.setOnClickListener { dismiss() }
        binding.btnCancelWizard.setOnClickListener { dismiss() }

        binding.btnResetDefaults.setOnClickListener {
            restoreDefaults()
        }

        // Apply 3D Faceted Tactile Buttons
        StormButton3dHelper.applyPrimary(binding.btnApplyWizard)
        StormButton3dHelper.applySecondary(binding.btnResetDefaults)
        StormButton3dHelper.applySecondary(binding.btnCancel)

        setupModeSelectors()
        detectAndDisplayHardwareInfo()

        binding.btnApplyWizard.setOnClickListener {
            applyOptimization(selectedMode)
        }
    }

    override fun onStart() {
        super.onStart()
        dialog?.let { d ->
            d.window?.let { window ->
                window.setLayout(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT
                )
                window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
                WindowCompat.setDecorFitsSystemWindows(window, false)
                ThemeHelper.applySystemBarsTheme(window, requireContext())
            }
            // Mandatory ESC Modal Dismissal
            d.setOnKeyListener { _, keyCode, event ->
                if (keyCode == KeyEvent.KEYCODE_ESCAPE && event.action == KeyEvent.ACTION_UP) {
                    dismiss()
                    true
                } else {
                    false
                }
            }
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

    private fun detectAndDisplayHardwareInfo() {
        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.IO) {
            val profile = cachedProfile ?: StormHardwareCalibrator.detectHardware(requireContext().applicationContext).also {
                cachedProfile = it
            }

            withContext(Dispatchers.Main) {
                if (_binding != null) {
                    renderHardwareInfo(profile)
                }
            }
        }
    }

    private fun renderHardwareInfo(profile: StormHardwareCalibrator.DeviceHardwareProfile) {
        val formFactor = if (profile.isTablet) {
            getString(R.string.auto_opt_form_tablet)
        } else {
            getString(R.string.auto_opt_form_phone)
        }

        binding.textHwSoc.text = getString(R.string.auto_opt_soc_fmt, profile.socName, profile.cpuCores, formFactor)
        binding.textHwGpu.text = getString(R.string.auto_opt_gpu_fmt, profile.gpuName)
        binding.textHwRam.text = getString(R.string.auto_opt_ram_fmt, profile.totalRamGb, profile.availRamGb)

        val tierName = when (profile.tier) {
            StormHardwareCalibrator.HardwareTier.FLAGSHIP_ELITE -> "ULTRA-FLAGSHIP"
            StormHardwareCalibrator.HardwareTier.FLAGSHIP -> "FLAGSHIP"
            StormHardwareCalibrator.HardwareTier.HIGH_MIDRANGE -> "HIGH-END"
            StormHardwareCalibrator.HardwareTier.MIDRANGE -> "MID-RANGE"
            StormHardwareCalibrator.HardwareTier.BUDGET -> "BUDGET"
        }
        binding.textHwTier.text = tierName

        val isNeonGreen = profile.tier == StormHardwareCalibrator.HardwareTier.FLAGSHIP_ELITE || profile.tier == StormHardwareCalibrator.HardwareTier.FLAGSHIP
        binding.textHwTier.setTextColor(if (isNeonGreen) Color.parseColor("#00F2FE") else Color.parseColor("#00E676"))

        binding.tvHardwareInfo.text = "✓ Dynamic State • NCE • Reactive Flushing • CPU Affinity Pinning • Fast GPU Time"
    }

    private fun setupModeSelectors() {
        binding.cardPresetMaxFps.setOnClickListener { selectMode(MODE_FAST) }
        binding.cardPresetBalanced.setOnClickListener { selectMode(MODE_NORMAL) }
        binding.cardPresetQuality.setOnClickListener { selectMode(MODE_ACCURATE) }
        binding.cardPresetBattery.setOnClickListener { selectMode(MODE_BATTERY) }

        val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(requireContext())
        val savedMode = prefs.getInt("selected_auto_optimization_mode", MODE_NORMAL)
        selectMode(savedMode)
    }

    private fun selectMode(mode: Int) {
        selectedMode = mode

        binding.radioModeFast.isChecked = (mode == MODE_FAST)
        binding.radioModeNormal.isChecked = (mode == MODE_NORMAL)
        binding.radioModeAccurate.isChecked = (mode == MODE_ACCURATE)
        binding.radioModeDefault.isChecked = (mode == MODE_BATTERY || mode == MODE_DEFAULT)

        val density = resources.displayMetrics.density
        val selectedBorderPx = (2.5f * density).toInt()
        val defaultBorderPx = (1.2f * density).toInt()

        val selectedColor = Color.parseColor("#8B5CF6")
        val defaultBorderColor = Color.parseColor("#358B5CF6")

        binding.cardPresetMaxFps.strokeColor = if (mode == MODE_FAST) selectedColor else defaultBorderColor
        binding.cardPresetMaxFps.strokeWidth = if (mode == MODE_FAST) selectedBorderPx else defaultBorderPx

        binding.cardPresetBalanced.strokeColor = if (mode == MODE_NORMAL) selectedColor else defaultBorderColor
        binding.cardPresetBalanced.strokeWidth = if (mode == MODE_NORMAL) selectedBorderPx else defaultBorderPx

        binding.cardPresetQuality.strokeColor = if (mode == MODE_ACCURATE) selectedColor else defaultBorderColor
        binding.cardPresetQuality.strokeWidth = if (mode == MODE_ACCURATE) selectedBorderPx else defaultBorderPx

        binding.cardPresetBattery.strokeColor = if (mode == MODE_BATTERY || mode == MODE_DEFAULT) selectedColor else defaultBorderColor
        binding.cardPresetBattery.strokeWidth = if (mode == MODE_BATTERY || mode == MODE_DEFAULT) selectedBorderPx else defaultBorderPx

        try {
            binding.root.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)
        } catch (_: Exception) {}
    }

    private fun restoreDefaults() {
        val context = requireContext()
        val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(context)
        prefs.edit().putInt("selected_auto_optimization_mode", MODE_DEFAULT).apply()

        StormHardwareCalibrator.applyPreset(StormHardwareCalibrator.StormPreset.DEFAULT, context)

        Toast.makeText(
            context,
            getString(R.string.auto_optimization_defaults_restored_toast),
            Toast.LENGTH_LONG
        ).show()

        dismiss()
    }

    private fun applyOptimization(mode: Int) {
        val context = requireContext()
        val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(context)
        prefs.edit().putInt("selected_auto_optimization_mode", mode).apply()

        val preset = when (mode) {
            MODE_FAST -> StormHardwareCalibrator.StormPreset.FAST
            MODE_NORMAL -> StormHardwareCalibrator.StormPreset.NORMAL
            MODE_ACCURATE -> StormHardwareCalibrator.StormPreset.ACCURATE
            MODE_BATTERY -> StormHardwareCalibrator.StormPreset.BATTERY
            else -> StormHardwareCalibrator.StormPreset.DEFAULT
        }

        StormHardwareCalibrator.applyPreset(preset, context)

        val modeName = when (mode) {
            MODE_FAST -> getString(R.string.auto_opt_mode_fast_short)
            MODE_NORMAL -> getString(R.string.auto_opt_mode_normal_short)
            MODE_ACCURATE -> getString(R.string.auto_opt_mode_accurate_short)
            MODE_BATTERY -> getString(R.string.auto_opt_mode_battery_short)
            else -> getString(R.string.auto_opt_mode_default_short)
        }

        Toast.makeText(
            context,
            getString(R.string.auto_optimization_applied_toast, modeName),
            Toast.LENGTH_LONG
        ).show()

        dismiss()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
