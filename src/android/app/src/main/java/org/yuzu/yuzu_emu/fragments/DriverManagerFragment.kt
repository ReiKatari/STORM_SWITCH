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
import androidx.lifecycle.lifecycleScope
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

        updateRecommendedBanner()

        binding.buttonInstallStormDriver.setOnClickListener {
            installStormDriver10()
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
        updateRecommendedBanner()
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

    private fun updateRecommendedBanner() {
        if (_binding == null) return
        val model = (GpuDriverHelper.hookLibPath?.let { GpuDriverHelper.getGpuModel(hookLibPath = it) } ?: "").trim()
        val modelLower = model.lowercase()
        val (vendor, rec) = when {
            modelLower.contains("mali") || modelLower.contains("immortalis") -> {
                val name = if (model.isNotEmpty()) model else "Arm Mali"
                name to "PanVK / STORM DRIVER 11.0.0"
            }
            modelLower.contains("xclipse") -> {
                val name = if (model.isNotEmpty()) model else "Samsung Xclipse"
                name to "STORM DRIVER 11.0.0 / System Xclipse RDNA"
            }
            modelLower.contains("adreno") -> {
                val name = if (model.isNotEmpty()) model else "Qualcomm Adreno"
                name to "Turnip / STORM DRIVER 11.0.0"
            }
            else -> {
                val name = if (model.isNotEmpty()) model else "Universal GPU"
                name to "STORM DRIVER 11.0.0"
            }
        }
        binding.textGpuVendorRecommendation.text = getString(R.string.storm_driver_10_recommendation, vendor, rec)

        val activePath = StringSetting.DRIVER_PATH.getString()
        val customMeta = GpuDriverHelper.customDriverSettingData
        val is11Active = customMeta.version?.contains("11.0") == true ||
                         customMeta.name?.contains("11.0") == true ||
                         activePath.contains("11.0", ignoreCase = true)

        if (is11Active) {
            binding.buttonInstallStormDriver.text = "STORM DRIVER 11.0.0 (Активен)"
        } else {
            binding.buttonInstallStormDriver.text = getString(R.string.install_storm_driver_10)
        }
    }

    private fun installStormDriver10() {
        val fileName = "STORM_DRIVER_11.0.0.zip"
        val storagePath = "${GpuDriverHelper.driverStoragePath}$fileName"
        val existingFile = File(storagePath)

        if (existingFile.exists() && existingFile.length() > 0L) {
            val meta = GpuDriverHelper.getMetadataFromZip(existingFile)
            if (meta.name != null) {
                val currentPath = StringSetting.DRIVER_PATH.getString()
                if (currentPath.equals(storagePath, ignoreCase = true)) {
                    android.widget.Toast.makeText(requireContext(), "STORM DRIVER 11.0.0 уже активен!", android.widget.Toast.LENGTH_SHORT).show()
                    return
                }
                promptDriverApplication(storagePath, 1)
                return
            }
        }

        val dialogBinding = org.yuzu.yuzu_emu.databinding.DialogProgressBinding.inflate(layoutInflater)
        dialogBinding.progressBar.isIndeterminate = true
        dialogBinding.title.text = getString(R.string.installing_driver)
        dialogBinding.status.text = getString(R.string.storm_driver_downloading)

        val progressDialog = com.google.android.material.dialog.MaterialAlertDialogBuilder(requireContext())
            .setView(dialogBinding.root)
            .setCancelable(false)
            .create()

        progressDialog.show()

        viewLifecycleOwner.lifecycleScope.launch(Dispatchers.Main) {
            val downloadMirrors = listOf(
                "https://github.com/ReiKatari/STORM_DRIVER/releases/download/11.0.0/STORM_DRIVER_11.0.0.zip",
                "https://media.githubusercontent.com/media/ReiKatari/STORM_GAMES_WORLD/main/Files/STORM_DRIVER_11.0.0.zip",
                "https://github.com/ReiKatari/STORM_GAMES_WORLD/raw/main/Files/STORM_DRIVER_11.0.0.zip"
            )

            var downloadedFile: File? = null
            var downloadSuccess = false

            val client = okhttp3.OkHttpClient.Builder()
                .dns(org.yuzu.yuzu_emu.utils.SmartDns)
                .connectTimeout(15, java.util.concurrent.TimeUnit.SECONDS)
                .readTimeout(30, java.util.concurrent.TimeUnit.SECONDS)
                .followRedirects(true)
                .followSslRedirects(true)
                .retryOnConnectionFailure(true)
                .build()

            withContext(Dispatchers.IO) {
                val tempTarget = File(requireContext().cacheDir, fileName)
                for ((idx, url) in downloadMirrors.withIndex()) {
                    try {
                        withContext(Dispatchers.Main) {
                            dialogBinding.status.text = if (idx > 0) {
                                getString(R.string.release_dl_mirror_fmt, idx)
                            } else {
                                getString(R.string.downloading)
                            }
                        }

                        val req = okhttp3.Request.Builder()
                            .url(url)
                            .header("User-Agent", "Mozilla/5.0 (Linux; Android 14) StormSwitch/16.2.4")
                            .header("Accept", "application/octet-stream, */*")
                            .build()

                        client.newCall(req).execute().use { resp ->
                            if (!resp.isSuccessful) throw IOException("HTTP ${resp.code}")
                            val body = resp.body ?: throw IOException("Empty body")
                            body.byteStream().use { input ->
                                java.io.FileOutputStream(tempTarget).use { output ->
                                    input.copyTo(output)
                                }
                            }
                        }

                        if (tempTarget.exists() && tempTarget.length() > 1024L) {
                            downloadedFile = tempTarget
                            downloadSuccess = true
                            break
                        }
                    } catch (_: Exception) {}
                }
            }

            progressDialog.dismiss()

            if (!downloadSuccess || downloadedFile == null) {
                android.widget.Toast.makeText(requireContext(), getString(R.string.driver_empty), android.widget.Toast.LENGTH_SHORT).show()
                return@launch
            }

            try {
                val uri = android.net.Uri.fromFile(downloadedFile!!)
                if (GpuDriverHelper.copyDriverToInternalStorage(uri)) {
                    refreshDriverList()
                    val targetPath = "${GpuDriverHelper.driverStoragePath}$fileName"
                    promptDriverApplication(targetPath, 1)
                } else {
                    android.widget.Toast.makeText(requireContext(), getString(R.string.select_gpu_driver_error), android.widget.Toast.LENGTH_SHORT).show()
                }
            } catch (e: Exception) {
                android.widget.Toast.makeText(requireContext(), e.localizedMessage ?: "Error", android.widget.Toast.LENGTH_SHORT).show()
            }
        }
    }

}
