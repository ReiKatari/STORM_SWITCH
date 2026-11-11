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

import com.google.android.material.dialog.MaterialAlertDialogBuilder

import kotlinx.coroutines.Dispatchers

import kotlinx.coroutines.launch

import kotlinx.coroutines.withContext

import org.yuzu.yuzu_emu.R

import org.yuzu.yuzu_emu.databinding.DialogAutoOptimizationBinding

import org.yuzu.yuzu_emu.databinding.DialogSaveProfileBinding

import org.yuzu.yuzu_emu.features.settings.utils.SettingsProfileManager

import org.yuzu.yuzu_emu.utils.StormDesignEngine

import org.yuzu.yuzu_emu.utils.StormHardwareCalibrator

import org.yuzu.yuzu_emu.utils.ThemeHelper

import java.util.Locale



class AutoOptimizationDialogFragment : DialogFragment() {



    private var _binding: DialogAutoOptimizationBinding? = null

    private val binding get() = _binding!!



    private var selectedMode = MODE_NORMAL

    private var detectedProfile: StormHardwareCalibrator.DeviceHardwareProfile? = null



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

        _binding = DialogAutoOptimizationBinding.inflate(inflater, container, false)

        return binding.root

    }



    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {

        super.onViewCreated(view, savedInstanceState)



        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, windowInsets ->

            val systemBars = windowInsets.getInsets(

                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout()

            )

            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)

            windowInsets

        }

        binding.root.requestApplyInsets()



        binding.btnClose.setOnClickListener { dismiss() }

        binding.btnCloseWizard?.setOnClickListener { dismiss() }

        binding.btnCancel?.setOnClickListener { dismiss() }

        binding.btnCancelWizard?.setOnClickListener { dismiss() }



        binding.btnResetDefaults.setOnClickListener {

            restoreDefaults()

        }



        setupModeSelectors()

        detectAndDisplayHardwareInfo()



        binding.btnSaveAsProfile.setOnClickListener {

            showSaveAsProfileDialog()

        }



        binding.btnApplyWizard.setOnClickListener {

            applyOptimization(selectedMode)

        }

    }



    override fun onStart() {

        super.onStart()

        dialog?.let { d ->

            applyWindowDimensions()

            StormDesignEngine.applyToDialog(d, requireContext())



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

        applyWindowDimensions()

        dialog?.let { StormDesignEngine.applyToDialog(it, requireContext()) }

    }



    private fun applyWindowDimensions() {

        dialog?.window?.let { window ->

            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)

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

            detectedProfile = profile



            withContext(Dispatchers.Main) {

                if (_binding != null) {

                    renderHardwareInfo(profile)

                    updateParametersPreview(selectedMode, profile)

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



        // Localized Russian decimal separator ',' for RAM capacity

        val ruLocale = Locale.forLanguageTag("ru")

        val isRussian = runCatching { resources.configuration.locales[0].language.startsWith("ru") }.getOrDefault(true)

        val ramTotalStr = if (isRussian) {

            String.format(ruLocale, "%.1f ГБ", profile.totalRamGb)

        } else {

            String.format(Locale.US, "%.1f GB", profile.totalRamGb)

        }

        val ramAvailStr = if (isRussian) {

            String.format(ruLocale, "%.1f ГБ", profile.availRamGb)

        } else {

            String.format(Locale.US, "%.1f GB", profile.availRamGb)

        }

        binding.textHwRam.text = getString(R.string.storm_auto_hw_ram_fmt, ramTotalStr, ramAvailStr)



        val tierName = when (profile.tier) {

            StormHardwareCalibrator.HardwareTier.FLAGSHIP_ELITE -> getString(R.string.tier_flagship_elite)

            StormHardwareCalibrator.HardwareTier.FLAGSHIP -> getString(R.string.tier_flagship)

            StormHardwareCalibrator.HardwareTier.HIGH_MIDRANGE -> getString(R.string.tier_high_midrange)

            StormHardwareCalibrator.HardwareTier.MIDRANGE -> getString(R.string.tier_midrange)

            StormHardwareCalibrator.HardwareTier.BUDGET -> getString(R.string.tier_budget)

        }

        binding.textHwTier.text = tierName



        val isNeonCyan = profile.tier == StormHardwareCalibrator.HardwareTier.FLAGSHIP_ELITE || profile.tier == StormHardwareCalibrator.HardwareTier.FLAGSHIP

        binding.textHwTier.setTextColor(if (isNeonCyan) org.yuzu.yuzu_emu.utils.ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorPrimary) else org.yuzu.yuzu_emu.utils.ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorSecondary))



        binding.tvHardwareInfo.text = getString(R.string.storm_auto_hw_summary)

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



        // Check icons visibility

        binding.radioModeFast.visibility = if (mode == MODE_FAST) View.VISIBLE else View.GONE

        binding.radioModeNormal.visibility = if (mode == MODE_NORMAL) View.VISIBLE else View.GONE

        binding.radioModeAccurate.visibility = if (mode == MODE_ACCURATE) View.VISIBLE else View.GONE

        binding.radioModeDefault.visibility = if (mode == MODE_BATTERY || mode == MODE_DEFAULT) View.VISIBLE else View.GONE



        // Zero border stroke mandate: app:strokeWidth="0dp" ALWAYS!

        // Show active state exclusively via surface elevation background tint

        val activeBg = ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorSecondaryContainer)

        val defaultBg = ThemeHelper.getColorFromAttr(requireContext(), com.google.android.material.R.attr.colorSurfaceVariant)



        binding.cardPresetMaxFps.setCardBackgroundColor(if (mode == MODE_FAST) activeBg else defaultBg)

        binding.cardPresetBalanced.setCardBackgroundColor(if (mode == MODE_NORMAL) activeBg else defaultBg)

        binding.cardPresetQuality.setCardBackgroundColor(if (mode == MODE_ACCURATE) activeBg else defaultBg)

        binding.cardPresetBattery.setCardBackgroundColor(if (mode == MODE_BATTERY || mode == MODE_DEFAULT) activeBg else defaultBg)



        // Update live reactive parameters preview

        detectedProfile?.let { profile ->

            updateParametersPreview(mode, profile)

        }



        try {

            binding.root.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)

        } catch (_: Exception) {}

    }



    private fun updateParametersPreview(mode: Int, profile: StormHardwareCalibrator.DeviceHardwareProfile) {

        val stormPreset = when (mode) {

            MODE_FAST -> StormHardwareCalibrator.StormPreset.FAST

            MODE_ACCURATE -> StormHardwareCalibrator.StormPreset.ACCURATE

            MODE_BATTERY -> StormHardwareCalibrator.StormPreset.BATTERY

            else -> StormHardwareCalibrator.StormPreset.NORMAL

        }



        val preview = StormHardwareCalibrator.getPresetPreview(stormPreset, profile, requireContext())



        binding.textPreviewResVal.text = preview.resolution

        binding.textPreviewScalingVal.text = preview.scaling

        binding.textPreviewGpuAccVal.text = preview.gpuAccuracy

        binding.textPreviewAstcVal.text = preview.astc



        binding.textPreviewCpuBackendVal.text = preview.cpuBackend

        binding.textPreviewCpuAccVal.text = preview.cpuAccuracy

        binding.textPreviewDramVal.text = preview.dram



        binding.textPreviewAsyncVal.text = preview.asyncShaders

        binding.textPreviewFlushingVal.text = preview.reactiveFlushing

        binding.textPreviewThermalsVal.text = preview.thermals

    }



    private fun showSaveAsProfileDialog() {

        val modeName = when (selectedMode) {

            MODE_FAST -> getString(R.string.storm_auto_mode_fast_title)

            MODE_ACCURATE -> getString(R.string.storm_auto_mode_accurate_title)

            MODE_BATTERY -> getString(R.string.storm_auto_mode_battery_title)

            else -> getString(R.string.storm_auto_mode_normal_title)

        }



        val dialogBinding = DialogSaveProfileBinding.inflate(layoutInflater)

        dialogBinding.textSaveProfileTitle.text = getString(R.string.storm_auto_save_dialog_title)

        dialogBinding.textSaveProfileMsg.text = getString(R.string.storm_prof_save_subtitle)

        val defaultName = getString(R.string.storm_auto_save_default_name_fmt, modeName)

        dialogBinding.editProfileName.setText(defaultName)



        val dialog = MaterialAlertDialogBuilder(requireContext())

            .setView(dialogBinding.root)

            .create()



        dialog.window?.let { window ->

            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

        }



        dialogBinding.btnCancelSaveProfile.setOnClickListener { dialog.dismiss() }

        dialogBinding.btnConfirmSaveProfile.setOnClickListener {

            val name = dialogBinding.editProfileName.text?.toString()?.trim().orEmpty()

            if (name.isNotEmpty()) {

                val preset = when (selectedMode) {

                    MODE_FAST -> StormHardwareCalibrator.StormPreset.FAST

                    MODE_ACCURATE -> StormHardwareCalibrator.StormPreset.ACCURATE

                    MODE_BATTERY -> StormHardwareCalibrator.StormPreset.BATTERY

                    else -> StormHardwareCalibrator.StormPreset.NORMAL

                }

                StormHardwareCalibrator.applyPreset(preset, requireContext())

                val saved = SettingsProfileManager.saveCurrentSettingsAsProfile(name, modeName)

                if (saved != null) {

                    Toast.makeText(

                        requireContext(),

                        getString(R.string.profile_saved_toast, name),

                        Toast.LENGTH_SHORT

                    ).show()

                    dialog.dismiss()

                }

            } else {

                dialogBinding.inputLayoutProfileName.error = getString(R.string.profile_name_hint)

            }

        }



        dialog.show()

        StormDesignEngine.applyToDialog(dialog, requireContext())

    }



    private fun restoreDefaults() {

        val context = requireContext()

        val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(context)

        prefs.edit().putInt("selected_auto_optimization_mode", MODE_DEFAULT).apply()



        StormHardwareCalibrator.applyPreset(StormHardwareCalibrator.StormPreset.DEFAULT, context)



        Toast.makeText(

            context,

            getString(R.string.storm_auto_reset_success),

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

            MODE_FAST -> getString(R.string.storm_auto_mode_fast_title)

            MODE_NORMAL -> getString(R.string.storm_auto_mode_normal_title)

            MODE_ACCURATE -> getString(R.string.storm_auto_mode_accurate_title)

            MODE_BATTERY -> getString(R.string.storm_auto_mode_battery_title)

            else -> getString(R.string.auto_opt_mode_default_short)

        }



        Toast.makeText(

            context,

            getString(R.string.storm_auto_applied_fmt, modeName),

            Toast.LENGTH_LONG

        ).show()



        dismiss()

    }



    override fun onDestroyView() {

        super.onDestroyView()

        _binding = null

    }

}
