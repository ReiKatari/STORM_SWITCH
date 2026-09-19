// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.edit
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.navigation.findNavController
import androidx.navigation.fragment.navArgs
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.GridLayoutManager
import com.google.android.material.transition.MaterialSharedAxis
import org.yuzu.yuzu_emu.HomeNavigationDirections
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.adapters.DriverAdapter
import org.yuzu.yuzu_emu.databinding.FragmentDriverManagerBinding
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.Settings
import org.yuzu.yuzu_emu.features.settings.model.StringSetting
import org.yuzu.yuzu_emu.features.settings.ui.SettingsSubscreen
import org.yuzu.yuzu_emu.model.DriverViewModel
import org.yuzu.yuzu_emu.model.HomeViewModel
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.GpuDriverHelper
import org.yuzu.yuzu_emu.utils.NativeConfig
import org.yuzu.yuzu_emu.utils.ViewUtils.updateMargins
import org.yuzu.yuzu_emu.utils.collect
import java.io.File
import java.io.IOException

class DriverManagerFragment : Fragment() {
    private var _binding: FragmentDriverManagerBinding? = null
    private val binding get() = _binding!!

    private val homeViewModel: HomeViewModel by activityViewModels()
    private val driverViewModel: DriverViewModel by activityViewModels()

    private val args by navArgs<DriverManagerFragmentArgs>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enterTransition = MaterialSharedAxis(MaterialSharedAxis.X, true)
        returnTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)
        reenterTransition = MaterialSharedAxis(MaterialSharedAxis.X, false)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentDriverManagerBinding.inflate(inflater)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        homeViewModel.setStatusBarShadeVisibility(visible = false)

        driverViewModel.onOpenDriverManager(args.game)
        if (NativeConfig.isPerGameConfigLoaded()) {
            binding.toolbarDrivers.inflateMenu(R.menu.menu_driver_manager)
            driverViewModel.showClearButton(!StringSetting.DRIVER_PATH.global)
            binding.toolbarDrivers.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_driver_use_global -> {
                        StringSetting.DRIVER_PATH.global = true
                        NativeConfig.savePerGameConfig()
                        val targetGame = args.game
                        if (targetGame != null) {
                            GpuDriverHelper.resetPerGameDriverToGlobal(targetGame)
                            driverViewModel.wipeGameShaders(targetGame)
                        }
                        driverViewModel.updateDriverList()
                        (binding.listDrivers.adapter as DriverAdapter)
                            .replaceList(driverViewModel.driverList.value)
                        driverViewModel.showClearButton(false)
                        homeViewModel.reloadPropertiesList(true)
                        true
                    }

                    else -> false
                }
            }

            driverViewModel.showClearButton.collect(viewLifecycleOwner) {
                binding.toolbarDrivers.menu.findItem(R.id.menu_driver_use_global).isVisible = it
            }
        }

        driverViewModel.shouldShowDriverShaderDialog.collect(viewLifecycleOwner) { shouldShow ->
            if (shouldShow) {
                showDriverShaderWipeDialog()
            }
        }

        if (!driverViewModel.isInteractionAllowed.value) {
            DriversLoadingDialogFragment().show(
                childFragmentManager,
                DriversLoadingDialogFragment.TAG
            )
        }

        binding.toolbarDrivers.setNavigationOnClickListener {
            requireActivity().onBackPressedDispatcher.onBackPressed()
        }

        binding.buttonInstall.setOnClickListener {
            getDriver.launch(arrayOf("application/zip"))
        }

        binding.buttonFetch.setOnClickListener {
            val action = HomeNavigationDirections.actionGlobalSettingsSubscreenActivity(
                SettingsSubscreen.DRIVER_FETCHER,
                null
            )
            binding.root.findNavController().navigate(action)
        }

        binding.listDrivers.apply {
            layoutManager = GridLayoutManager(
                requireContext(),
                resources.getInteger(R.integer.grid_columns)
            )
            adapter = DriverAdapter(driverViewModel) { driver, position ->
                val path = if (position == 0) "" else {
                    driverViewModel.driverData.getOrNull(position - 1)?.first ?: ""
                }
                if (args.game != null) {
                    // Personal game settings: ALWAYS apply directly for this game without prompting
                    applyDriver(path, globallyToAll = false, selectedPosition = position)
                } else {
                    promptDriverApplication(path, position)
                }
            }
        }

        setInsets()

        if (!GpuDriverHelper.supportsCustomDriverLoading()) {
            showDriverWarningDialog()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        driverViewModel.onCloseDriverManager(args.game)
    }

    override fun onResume() {
        super.onResume()
        refreshDriverList()
    }

    private fun refreshDriverList() {
        if (_binding == null) return
        driverViewModel.reloadDriverData()
        driverViewModel.updateDriverList()
        (binding.listDrivers.adapter as? DriverAdapter)
            ?.replaceList(driverViewModel.driverList.value)
    }

    private fun setInsets() =
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.root
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())

            val leftInsets = barInsets.left + cutoutInsets.left
            val rightInsets = barInsets.right + cutoutInsets.right

            binding.toolbarDrivers.updateMargins(left = leftInsets, right = rightInsets)
            binding.listDrivers.updateMargins(left = leftInsets, right = rightInsets)

            val fabSpacing = resources.getDimensionPixelSize(R.dimen.spacing_fab)
            binding.buttonInstall.updateMargins(
                left = leftInsets + fabSpacing,
                right = rightInsets + fabSpacing,
                bottom = barInsets.bottom + fabSpacing
            )

            binding.buttonFetch.updateMargins(
                left = leftInsets + fabSpacing,
                right = rightInsets + fabSpacing,
                bottom = barInsets.bottom + fabSpacing
            )

            binding.listDrivers.updatePadding(
                bottom = barInsets.bottom +
                    resources.getDimensionPixelSize(R.dimen.spacing_bottom_list_fab)
            )

            windowInsets
        }

    private val getDriver =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { result ->
            if (result == null) {
                return@registerForActivityResult
            }

            ProgressDialogFragment.newInstance(
                requireActivity(),
                R.string.installing_driver,
                false
            ) { _, _ ->
                val driverPath =
                    "${GpuDriverHelper.driverStoragePath}${FileUtil.getFilename(result)}"
                val driverFile = File(driverPath)

                // Ignore file exceptions when a user selects an invalid zip
                try {
                    if (!GpuDriverHelper.copyDriverToInternalStorage(result)) {
                        throw IOException("Driver failed validation!")
                    }
                } catch (_: IOException) {
                    if (driverFile.exists()) {
                        driverFile.delete()
                    }
                    return@newInstance getString(R.string.select_gpu_driver_error)
                }

                val driverData = GpuDriverHelper.getMetadataFromZip(driverFile)
                val driverInList =
                    driverViewModel.driverData.firstOrNull {
                        it.first == driverPath || it.second == driverData
                    }
                val isForGame = args.game != null
                if (driverInList != null) {
                    val existingIndex = driverViewModel.driverData.indexOf(driverInList)
                    if (isForGame) {
                        withContext(Dispatchers.Main) {
                            applyDriver(driverInList.first, globallyToAll = false, selectedPosition = existingIndex + 1)
                        }
                        return@newInstance Any()
                    } else {
                        return@newInstance getString(R.string.driver_already_installed)
                    }
                } else {
                    driverViewModel.addDriverOnly(Pair(driverPath, driverData))
                    val newPosition = driverViewModel.driverData.size
                    withContext(Dispatchers.Main) {
                        if (isForGame) {
                            applyDriver(driverPath, globallyToAll = false, selectedPosition = newPosition)
                        } else {
                            promptDriverApplication(driverPath, newPosition)
                        }
                    }
                }
                return@newInstance Any()
            }.show(childFragmentManager, ProgressDialogFragment.TAG)
        }


    private fun promptDriverApplication(driverPath: String, selectedPosition: Int) {
        if (_binding == null) return
        refreshDriverList()

        if (args.game != null) {
            // Per-game settings: NEVER prompt with global/game/cancel dialog. Always apply directly for this game.
            applyDriver(driverPath, globallyToAll = false, selectedPosition = selectedPosition)
            return
        }

        com.google.android.material.dialog.MaterialAlertDialogBuilder(requireContext())
            .setTitle(R.string.apply_driver_title)
            .setMessage(R.string.apply_driver_message)
            .setPositiveButton(R.string.apply_driver_globally) { _, _ ->
                applyDriver(driverPath, globallyToAll = true, selectedPosition = selectedPosition)
            }
            .setNeutralButton(R.string.apply_driver_keep_custom) { _, _ ->
                applyDriver(driverPath, globallyToAll = false, selectedPosition = selectedPosition)
            }
            .setNegativeButton(R.string.cancel) { _, _ ->
                updateDriverSelectionUi()
            }
            .show()
    }

    private fun applyDriver(driverPath: String, globallyToAll: Boolean, selectedPosition: Int) {
        val driverFile = File(driverPath)
        val isForGame = args.game != null

        if (globallyToAll) {
            StringSetting.DRIVER_PATH.global = true
            StringSetting.DRIVER_PATH.setString(driverPath)
            if (driverPath.isEmpty() || !driverFile.exists()) {
                GpuDriverHelper.installDefaultDriver()
            } else {
                GpuDriverHelper.installCustomDriver(driverFile)
            }
            GpuDriverHelper.applyDriverGloballyToAllCustomConfigs()
            driverViewModel.wipeAllShaders()
            NativeConfig.saveGlobalConfig()
        } else {
            if (isForGame) {
                val targetGame = args.game!!
                if (!NativeConfig.isPerGameConfigLoaded()) {
                    org.yuzu.yuzu_emu.features.settings.utils.SettingsFile.loadCustomConfig(targetGame)
                }
                StringSetting.DRIVER_PATH.global = false
                StringSetting.DRIVER_PATH.setString(driverPath)
                if (driverPath.isEmpty() || !driverFile.exists()) {
                    GpuDriverHelper.installDefaultDriver()
                } else {
                    GpuDriverHelper.installCustomDriver(driverFile)
                }
                GpuDriverHelper.savePerGameDriver(targetGame, driverPath)
                driverViewModel.wipeGameShaders(targetGame)
                NativeConfig.savePerGameConfig()
                org.yuzu.yuzu_emu.model.GameFixDatabase.markConfigAsUserCustom(targetGame)
            } else {
                StringSetting.DRIVER_PATH.global = true
                StringSetting.DRIVER_PATH.setString(driverPath)
                if (driverPath.isEmpty() || !driverFile.exists()) {
                    GpuDriverHelper.installDefaultDriver()
                } else {
                    GpuDriverHelper.installCustomDriver(driverFile)
                }
                driverViewModel.wipeAllShaders()
                NativeConfig.saveGlobalConfig()
            }
        }

        driverViewModel.onDriverSelected(selectedPosition, skipShaderWipe = true)
        driverViewModel.reloadDriverData()
        refreshDriverList()
        updateDriverSelectionUi()
        homeViewModel.reloadPropertiesList(true)

        if (!BooleanSetting.DONT_SHOW_DRIVER_SHADER_WARNING.getBoolean(needsGlobal = true)) {
            showDriverShaderWipeDialog()
        }
    }

    private fun updateDriverSelectionUi() {
        if (_binding == null) return
        val adapter = binding.listDrivers.adapter as? DriverAdapter ?: return
        val selectedPosition = adapter.currentList
            .indexOfFirst { it.selected }
            .let { if (it == -1) 0 else it }
        driverViewModel.showClearButton(!StringSetting.DRIVER_PATH.global)
        binding.listDrivers.smoothScrollToPosition(selectedPosition)
    }

    fun showDriverWarningDialog() {
        val shouldDisplayGpuWarning =
            PreferenceManager.getDefaultSharedPreferences(requireContext())
                .getBoolean(Settings.PREF_SHOULD_SHOW_DRIVER_WARNING, true)
        if (shouldDisplayGpuWarning) {
            MessageDialogFragment.newInstance(
                activity,
                titleId = R.string.unsupported_gpu,
                descriptionId = R.string.unsupported_gpu_warning,
                positiveButtonTitleId = R.string.dont_show_again,
                negativeButtonTitleId = R.string.close,
                showNegativeButton = true,
                positiveAction = {
                    PreferenceManager.getDefaultSharedPreferences(requireContext())
                        .edit() {
                            putBoolean(Settings.PREF_SHOULD_SHOW_DRIVER_WARNING, false)
                        }
                }
            ).show(requireActivity().supportFragmentManager, MessageDialogFragment.TAG)
        }
    }

    private fun showDriverShaderWipeDialog() {
        com.google.android.material.dialog.MaterialAlertDialogBuilder(requireContext())
            .setTitle(R.string.driver_shader_wipe_dialog_title)
            .setMessage(R.string.driver_shader_wipe_dialog_message)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                driverViewModel.onDriverShaderDialogDismissed(dontShowAgain = false)
            }
            .setNegativeButton(R.string.dont_show_again) { _, _ ->
                driverViewModel.onDriverShaderDialogDismissed(dontShowAgain = true)
            }
            .setCancelable(false)
            .show()
    }
}
