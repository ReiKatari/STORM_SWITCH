// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.model

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.StringSetting
import org.yuzu.yuzu_emu.features.settings.utils.SettingsFile
import org.yuzu.yuzu_emu.model.Driver.Companion.toDriver
import org.yuzu.yuzu_emu.utils.GpuDriverHelper
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.utils.GpuDriverMetadata
import org.yuzu.yuzu_emu.utils.NativeConfig
import android.os.Handler
import android.os.Looper
import android.widget.Toast
import java.io.File

class DriverViewModel : ViewModel() {
    private val _areDriversLoading = MutableStateFlow(false)
    private val _isDriverReady = MutableStateFlow(true)
    private val _isDeletingDrivers = MutableStateFlow(false)


    val isInteractionAllowed: StateFlow<Boolean> =
        combine(
            _areDriversLoading,
            _isDriverReady,
            _isDeletingDrivers
        ) { loading, ready, deleting ->
            !loading && ready && !deleting
        }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(), initialValue = false)

    var driverData = GpuDriverHelper.getDrivers()

    private val _driverList = MutableStateFlow(emptyList<Driver>())
    val driverList: StateFlow<List<Driver>> get() = _driverList

    // Used for showing which driver is currently installed within the driver manager card
    private val _selectedDriverTitle = MutableStateFlow("")
    val selectedDriverTitle: StateFlow<String> get() = _selectedDriverTitle

    private val _selectedDriverVersion = MutableStateFlow("")
    val selectedDriverVersion: StateFlow<String> get() = _selectedDriverVersion

    private val _showClearButton = MutableStateFlow(false)
    val showClearButton = _showClearButton.asStateFlow()

    private val driversToDelete = mutableListOf<String>()

    private var previousDriverPath: String = ""
    private var activeGame: Game? = null

    private val _shouldShowDriverShaderDialog = MutableStateFlow(false)
    val shouldShowDriverShaderDialog: StateFlow<Boolean> get() = _shouldShowDriverShaderDialog

    init {
        updateDriverList()
        updateDriverNameForGame(null)
        previousDriverPath = StringSetting.DRIVER_PATH.getString()
    }

    fun reloadDriverData() {
        _areDriversLoading.value = true
        driverData = GpuDriverHelper.getDrivers()
            .filterNot { driversToDelete.contains(it.first) }
            .toMutableList()
        updateDriverList()
        _areDriversLoading.value = false
    }

    fun updateDriverList() {
        val selectedDriver = GpuDriverHelper.customDriverSettingData
        val currentDriverPath = StringSetting.DRIVER_PATH.getString()
        val systemDriverData = try {
            GpuDriverHelper.getSystemDriverInfo()
        } catch (_: Throwable) {
            null
        }
        val systemDriverTitle = YuzuApplication.appContext.getString(R.string.system_gpu_driver)

        val isSystemSelected = currentDriverPath.isEmpty() || (!File(currentDriverPath).exists() && selectedDriver.name == null)

        val newDriverList = mutableListOf(
            Driver(
                selected = isSystemSelected,
                title = systemDriverTitle,
                version = NativeLibrary.getVulkanDriverVersion().takeIf { !it.isNullOrEmpty() } ?: systemDriverTitle,
                description = systemDriverData?.get(1) ?: ""
            )
        )
        driverData.forEach {
            val isSelected = !isSystemSelected && (
                it.first.equals(currentDriverPath, ignoreCase = true) ||
                File(it.first).name.equals(File(currentDriverPath).name, ignoreCase = true) ||
                (it.second.name != null && it.second.name == selectedDriver.name && it.second.version == selectedDriver.version)
            )
            newDriverList.add(it.second.toDriver(isSelected))
        }

        if (!isSystemSelected && newDriverList.none { it.selected }) {
            val matchedIndex = driverData.indexOfFirst {
                File(it.first).name.equals(File(currentDriverPath).name, ignoreCase = true)
            }
            if (matchedIndex != -1) {
                newDriverList[matchedIndex + 1].selected = true
            } else if (newDriverList.isNotEmpty()) {
                newDriverList[0].selected = true
            }
        }

        _driverList.value = newDriverList
        previousDriverPath = StringSetting.DRIVER_PATH.getString()
    }

    fun onOpenDriverManager(game: Game?) {
        activeGame = game
        if (game != null) {
            SettingsFile.loadCustomConfig(game)
        }
        updateDriverList()
    }

    fun showClearButton(value: Boolean) {
        _showClearButton.value = value
    }

    fun onDriverSelected(position: Int, skipShaderWipe: Boolean = false) {

        val newDriverPath = if (position == 0) {
            ""
        } else {
            driverData[position - 1].first
        }

        if (!skipShaderWipe && newDriverPath != previousDriverPath) {
            if (activeGame != null) {
                wipeGameShaders(activeGame!!)
            } else {
                wipeAllShaders()
            }

            if (!BooleanSetting.DONT_SHOW_DRIVER_SHADER_WARNING.getBoolean(needsGlobal = true)) {
                _shouldShowDriverShaderDialog.value = true
            }
        }

        if (position == 0) {
            StringSetting.DRIVER_PATH.setString("")
            if (activeGame == null) {
                GpuDriverHelper.installDefaultDriver()
            }
        } else {
            val driverFile = File(driverData[position - 1].first)
            StringSetting.DRIVER_PATH.setString(driverFile.path)
            if (activeGame == null && driverFile.exists()) {
                GpuDriverHelper.installCustomDriver(driverFile)
            }
        }
        if (activeGame == null) {
            NativeConfig.saveGlobalConfig()
        } else {
            NativeConfig.savePerGameConfig()
        }
        previousDriverPath = newDriverPath
        updateName()
    }

    fun onDriverShaderDialogDismissed(dontShowAgain: Boolean) {
        if (dontShowAgain) {
            BooleanSetting.DONT_SHOW_DRIVER_SHADER_WARNING.setBoolean(true)
            NativeConfig.saveGlobalConfig()
        }
        _shouldShowDriverShaderDialog.value = false
    }

    fun wipeGameShaders(game: Game) {
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val externalFilesDir = YuzuApplication.appContext.getExternalFilesDir(null)
                    ?: return@withContext
                val shaderDir = File(
                    externalFilesDir.absolutePath +
                    "/shader/" + game.settingsName.lowercase()
                )
                if (shaderDir.exists()) {
                    shaderDir.deleteRecursively()
                }
                val cacheDir = YuzuApplication.appContext.cacheDir
                if (cacheDir != null) {
                    val vulkanCache = File(cacheDir, "vulkan_pipelines.bin")
                    if (vulkanCache.exists()) {
                        vulkanCache.delete()
                    }
                }
            }
        }
    }

    fun wipeAllShaders() {
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val externalFilesDir = YuzuApplication.appContext.getExternalFilesDir(null)
                if (externalFilesDir != null) {
                    val shaderDir = File(externalFilesDir.absolutePath + "/shader")
                    if (shaderDir.exists()) {
                        shaderDir.deleteRecursively()
                    }
                }
                val cacheDir = YuzuApplication.appContext.cacheDir
                if (cacheDir != null) {
                    val vulkanCache = File(cacheDir, "vulkan_pipelines.bin")
                    if (vulkanCache.exists()) {
                        vulkanCache.delete()
                    }
                }
            }
        }
    }

    val isPerGame: Boolean
        get() = activeGame != null || NativeConfig.isPerGameConfigLoaded()

    fun onDriverRemoved(removedPosition: Int, selectedPosition: Int) {
        if (isPerGame) {
            // Per-game settings must NEVER delete driver packages from storage or global settings.
            // Reset this game's driver override to use the global driver instead.
            StringSetting.DRIVER_PATH.global = true
            updateDriverList()
            updateName()
            showClearButton(false)
            return
        }

        val driverIndex = removedPosition - 1
        if (driverIndex !in driverData.indices) {
            updateDriverList()
            return
        }

        val pathToDelete = driverData[driverIndex].first
        deleteDriver(pathToDelete)
    }

    fun deleteDriver(driverPath: String) {
        val file = File(driverPath)
        if (file.exists()) {
            file.delete()
        }
        driversToDelete.remove(driverPath)

        val currentSelectedPath = StringSetting.DRIVER_PATH.getString()
        val isCurrentSelected = currentSelectedPath.equals(driverPath, ignoreCase = true) ||
            (file.name.isNotEmpty() && File(currentSelectedPath).name.equals(file.name, ignoreCase = true))

        if (isCurrentSelected) {
            StringSetting.DRIVER_PATH.setString("")
            if (activeGame == null) {
                GpuDriverHelper.installDefaultDriver()
                NativeConfig.saveGlobalConfig()
            } else {
                NativeConfig.savePerGameConfig()
            }
            wipeAllShaders()
        }

        reloadDriverData()
        updateDriverList()
        updateName()

        Toast.makeText(
            YuzuApplication.appContext,
            R.string.driver_deleted,
            Toast.LENGTH_SHORT
        ).show()
    }

    fun onDriverAdded(driver: Pair<String, GpuDriverMetadata>) {
        if (driversToDelete.contains(driver.first)) {
            driversToDelete.remove(driver.first)
        }

        val existingDriverIndex = driverData.indexOfFirst {
            it.first == driver.first || it.second == driver.second
        }
        if (existingDriverIndex != -1) {
            onDriverSelected(existingDriverIndex + 1)
            return
        }
        driverData.add(driver)
        onDriverSelected(driverData.size)
    }

    fun onCloseDriverManager(game: Game?) {
        _isDeletingDrivers.value = true
        try {
            updateDriverNameForGame(game)
            if (game == null) {
                NativeConfig.saveGlobalConfig()
                val globalDriverPath = StringSetting.DRIVER_PATH.getString(needsGlobal = true)
                val globalDriverFile = File(globalDriverPath)
                if (globalDriverPath.isEmpty() || !globalDriverFile.exists()) {
                    GpuDriverHelper.installDefaultDriver()
                } else {
                    val installedMeta = GpuDriverHelper.installedCustomDriverData
                    val targetMeta = GpuDriverHelper.getMetadataFromZip(globalDriverFile)
                    if (installedMeta != targetMeta) {
                        GpuDriverHelper.installCustomDriver(globalDriverFile)
                    }
                }

                // Drivers are physically deleted from storage ONLY from global settings
                driversToDelete.forEach {
                    val driver = File(it)
                    if (driver.exists()) {
                        driver.delete()
                    }
                }
            } else {
                NativeConfig.savePerGameConfig()
                NativeConfig.unloadPerGameConfig()
                NativeConfig.reloadGlobalConfig()
            }

            driversToDelete.clear()
        } finally {
            activeGame = null
            _isDeletingDrivers.value = false
        }
    }

    // It is the Emulation Fragment's responsibility to load per-game settings so that this function
    fun onLaunchGame(game: Game? = null) {
        _isDriverReady.value = false

        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val selectedDriverPath = StringSetting.DRIVER_PATH.getString()
                val selectedDriverFile = File(selectedDriverPath)
                val selectedDriverMetadata = GpuDriverHelper.customDriverSettingData
                val installedMetadata = GpuDriverHelper.installedCustomDriverData

                org.yuzu.yuzu_emu.utils.Log.info(
                    "[DriverViewModel] onLaunchGame for '${game?.title}': selected='$selectedDriverPath' (${selectedDriverMetadata.name}), installed='${installedMetadata.name}'"
                )

                if (selectedDriverPath.isEmpty() || selectedDriverMetadata.name == null) {
                    if (installedMetadata.name != null) {
                        org.yuzu.yuzu_emu.utils.Log.info("[DriverViewModel] Reverting to default system driver")
                        GpuDriverHelper.installDefaultDriver()
                    } else {
                        GpuDriverHelper.initializeDriverParameters()
                    }
                } else if (selectedDriverFile.exists()) {
                    val libName = installedMetadata.libraryName
                    val libFile = if (!libName.isNullOrEmpty()) File(GpuDriverHelper.driverInstallationPath, libName) else null
                    if (installedMetadata != selectedDriverMetadata || libFile == null || !libFile.exists()) {
                        org.yuzu.yuzu_emu.utils.Log.info("[DriverViewModel] Installing custom driver: ${selectedDriverFile.name}")
                        GpuDriverHelper.installCustomDriver(selectedDriverFile)
                    } else {
                        org.yuzu.yuzu_emu.utils.Log.info("[DriverViewModel] Driver already active: ${installedMetadata.name}")
                        GpuDriverHelper.initializeDriverParameters()
                    }
                } else {
                    org.yuzu.yuzu_emu.utils.Log.info("[DriverViewModel] Selected driver file missing, falling back to default")
                    GpuDriverHelper.installDefaultDriver()
                }

                // Crucial: ALWAYS apply per-game driver config (00-storm.conf / drirc) AFTER the driver is unzipped and parameters initialized!
                if (game != null) {
                    GpuDriverHelper.applyPerGameDriverConfig(game.programIdHex, game.title)
                }

                setDriverReady()
            }
        }
    }

    fun updateDriverNameForGame(game: Game?) {
        if (!GpuDriverHelper.supportsCustomDriverLoading()) {
            return
        }

        try {
            if (game == null || NativeConfig.isPerGameConfigLoaded()) {
                updateName()
            } else {
                SettingsFile.loadCustomConfig(game)
                updateName()
                NativeConfig.unloadPerGameConfig()
                NativeConfig.reloadGlobalConfig()
            }
        } catch (e: Throwable) {
            org.yuzu.yuzu_emu.utils.Log.error("[DriverViewModel] Error updating driver name: ${e.message}")
        }
    }

    private fun updateName() {
        val systemDriverTitle = YuzuApplication.appContext.getString(R.string.system_gpu_driver)
        val systemDriverVersion = NativeLibrary.getVulkanDriverVersion().takeIf { !it.isNullOrEmpty() } ?: systemDriverTitle
        val currentDriverPath = StringSetting.DRIVER_PATH.getString()
        val customDriver = if (currentDriverPath.isNotEmpty() && File(currentDriverPath).exists()) {
            val fromSetting = GpuDriverHelper.customDriverSettingData
            if (fromSetting.name != null) fromSetting else GpuDriverHelper.getMetadataFromZip(File(currentDriverPath))
        } else {
            GpuDriverHelper.customDriverSettingData
        }

        val effectiveVer = customDriver.packageVersion?.takeIf { it.isNotBlank() }
            ?: customDriver.version?.takeIf { it.isNotBlank() } ?: ""
        val rawName = customDriver.name?.takeIf { it.isNotBlank() } ?: ""
        val baseName = if (rawName.contains("(") && rawName.contains(")")) {
            rawName.substringBefore("(").trim()
        } else {
            rawName.trim()
        }

        val mainVerNumber = effectiveVer.substringBefore("-").trim()
        val customDisplayTitle = when {
            baseName.isNotEmpty() && mainVerNumber.isNotEmpty() && baseName.contains(mainVerNumber) -> baseName
            baseName.isNotEmpty() && effectiveVer.isNotEmpty() && baseName.contains(effectiveVer) -> baseName
            baseName.isNotEmpty() && effectiveVer.isNotEmpty() && !baseName.endsWith(effectiveVer) -> "$baseName $effectiveVer"
            baseName.isNotEmpty() -> baseName
            effectiveVer.isNotEmpty() -> effectiveVer
            else -> null
        }

        _selectedDriverTitle.value = customDisplayTitle ?: systemDriverTitle
        _selectedDriverVersion.value = if (effectiveVer.isNotEmpty()) effectiveVer else systemDriverVersion
    }

    private fun setDriverReady() {
        _isDriverReady.value = true
        updateName()
    }
}
