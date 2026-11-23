// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import android.graphics.SurfaceTexture
import android.net.Uri
import android.os.Build
import android.view.Surface
import java.io.File
import java.io.IOException
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.settings.model.StringSetting
import java.io.FileNotFoundException
import java.util.zip.ZipException
import java.util.zip.ZipFile
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.features.settings.utils.SettingsFile

object GpuDriverHelper {
    private const val META_JSON_FILENAME = "meta.json"
    private const val ACTIVE_DRIVER_MARKER = "active_driver_zip.txt"
    private const val PREF_DRIVER_MIGRATED_12_V1 = "storm_driver_migrated_12_v3"
    const val PREF_USER_EXPLICIT_SYSTEM_DRIVER = "storm_user_explicit_system_driver"
    private val driverLock = Any()
    @Volatile
    var activeGameProgramId: String = ""
    @Volatile
    var activeGameTitle: String = ""
    @Volatile
    private var lastNativeDriverInitSignature: String? = null
    private var fileRedirectionPath: String? = null
    var driverInstallationPath: String? = null
        get() {
            if (field == null) {
                try {
                    field = YuzuApplication.appContext.filesDir.canonicalPath + "/gpu_driver/"
                } catch (_: Throwable) {}
            }
            return field
        }
    internal var hookLibPath: String? = null
        get() {
            if (field == null) {
                try {
                    field = YuzuApplication.appContext.applicationInfo.nativeLibraryDir + "/"
                } catch (_: Throwable) {}
            }
            return field
        }

    val driverStoragePath get() = (DirectoryInitialization.userDirectory ?: YuzuApplication.appContext.filesDir.absolutePath) + "/gpu_drivers/"

    fun initializeFreedrenoConfigEarly() {
        NativeFreedrenoConfig.setFreedrenoBasePath(YuzuApplication.appContext.cacheDir.absolutePath)
        NativeFreedrenoConfig.initializeFreedrenoConfig()
        NativeFreedrenoConfig.reloadFreedrenoConfig()
    }


    fun isMaliGpu(): Boolean {
        if (File("/dev/kgsl-3d0").exists() || File("/sys/class/kgsl/kgsl-3d0/gpu_model").exists()) {
            return false
        }
        if (File("/dev/mali0").exists() || File("/dev/panfrost").exists() || File("/dev/panthor").exists()) {
            return true
        }
        val model = (hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: "").lowercase()
        return model.contains("mali") || model.contains("immortalis")
    }

    fun isXclipseGpu(): Boolean {
        val model = (hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: "").lowercase()
        return model.contains("xclipse")
    }

    fun isStormDriver8OrNewer(): Boolean {
        val meta = installedCustomDriverData
        val name = meta.name ?: ""
        val ver = meta.version ?: ""
        val isStorm = name.contains("STORM", ignoreCase = true) || ver.contains("STORM", ignoreCase = true)
        if (!isStorm) return false
        val regex = Regex("""\d+(\.\d+)*""")
        val verNum = regex.find(ver)?.value ?: regex.find(name)?.value ?: "0"
        val major = verNum.split(".").firstOrNull()?.toIntOrNull() ?: 0
        return major >= 8
    }

    fun isStormDriver10OrNewer(): Boolean {
        val meta = installedCustomDriverData
        val name = meta.name ?: ""
        val ver = meta.version ?: ""
        val isStorm = name.contains("STORM", ignoreCase = true) || ver.contains("STORM", ignoreCase = true)
        if (!isStorm) return false
        val regex = Regex("""\d+(\.\d+)*""")
        val verNum = regex.find(ver)?.value ?: regex.find(name)?.value ?: "0"
        val major = verNum.split(".").firstOrNull()?.toIntOrNull() ?: 0
        return major >= 10
    }

    fun isStormDriver12OrNewer(): Boolean {
        val meta = installedCustomDriverData
        val name = meta.name ?: ""
        val ver = meta.version ?: ""
        val isStorm = name.contains("STORM", ignoreCase = true) || ver.contains("STORM", ignoreCase = true)
        if (!isStorm) return false
        val regex = Regex("""\d+(\.\d+)*""")
        val verNum = regex.find(ver)?.value ?: regex.find(name)?.value ?: "0"
        val major = verNum.split(".").firstOrNull()?.toIntOrNull() ?: 0
        return major >= 12
    }

    fun isAdreno8xx(): Boolean {
        val soc = if (android.os.Build.VERSION.SDK_INT >= 31) android.os.Build.SOC_MODEL else ""
        if (soc.contains("7635", ignoreCase = true) || android.os.Build.HARDWARE.contains("volcano", ignoreCase = true)) {
            return false // Snapdragon 7s Gen 3 (Adreno 810) uses A7xx ISA/driver family
        }
        if (android.os.Build.MODEL.contains("S931", ignoreCase = true) ||
            android.os.Build.MODEL.contains("S936", ignoreCase = true) ||
            android.os.Build.MODEL.contains("S938", ignoreCase = true) ||
            android.os.Build.HARDWARE.contains("sun", ignoreCase = true) ||
            android.os.Build.HARDWARE.contains("pakala", ignoreCase = true) ||
            android.os.Build.BOARD.contains("sun", ignoreCase = true) ||
            android.os.Build.DEVICE.contains("sun", ignoreCase = true) ||
            android.os.Build.PRODUCT.contains("sun", ignoreCase = true) ||
            soc.contains("8850", ignoreCase = true) ||
            soc.contains("8835", ignoreCase = true) ||
            soc.contains("8750", ignoreCase = true) ||
            soc.contains("8735", ignoreCase = true)
        ) {
            return true
        }
        try {
            val model = hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: ""
            if (model.contains("810")) {
                return false
            }
            if (model.contains("840") || model.contains("850") || model.contains("830") || model.contains("825") ||
                model.contains("Adreno (TM) 8", ignoreCase = true) || model.contains("Adreno 8", ignoreCase = true)) {
                return true
            }
        } catch (_: Throwable) {}
        return false
    }

    /**
     * Detects Snapdragon 8 Elite Gen 5+ (SM8850 / SM8835 - Adreno 840 / 850+).
     * Legacy A830-only Turnip / PurpleVK builds report 'Adreno (TM) 840 (...)/unknown' and write
     * A830 UBWC macrotile headers into A840 swapchain AHardwareBuffers, squishing the live viewport
     * to 2/3 top-left with bottom UBWC stripes unless the driver explicitly supports Adreno 840 / SM8850.
     */
    fun isAdreno840OrNewer(): Boolean {
        val soc = if (android.os.Build.VERSION.SDK_INT >= 31) android.os.Build.SOC_MODEL else ""
        if (soc.contains("SM8850", ignoreCase = true) ||
            soc.contains("SM8845", ignoreCase = true) ||
            soc.contains("SM8835", ignoreCase = true) ||
            soc.equals("8850", ignoreCase = true) ||
            android.os.Build.HARDWARE.contains("canoe", ignoreCase = true)
        ) {
            return true
        }
        try {
            val model = hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: ""
            if (model.contains("840") || model.contains("850") ||
                model.contains("Adreno (TM) 84", ignoreCase = true) || model.contains("Adreno 84", ignoreCase = true) ||
                model.contains("Adreno (TM) 85", ignoreCase = true) || model.contains("Adreno 85", ignoreCase = true) ||
                model.contains("Adreno (TM) 9", ignoreCase = true) || model.contains("Adreno 9", ignoreCase = true)
            ) {
                return true
            }
        } catch (_: Throwable) {}
        return false
    }

    fun isTurnipDriverActive(): Boolean {
        val metadata = installedCustomDriverData
        val libName = metadata.libraryName ?: ""
        val vendor = metadata.vendor ?: ""
        val name = metadata.name ?: ""
        val driverVersion = metadata.version ?: ""
        if (libName.isEmpty()) {
            return false // System driver
        }
        return vendor.contains("Freedreno", ignoreCase = true) ||
               vendor.contains("Mesa", ignoreCase = true) ||
               name.contains("Turnip", ignoreCase = true) ||
               name.contains("STORM", ignoreCase = true) ||
               driverVersion.contains("Turnip", ignoreCase = true) ||
               libName.contains("freedreno", ignoreCase = true) ||
               libName.contains("adreno", ignoreCase = true) ||
               libName.contains("purple", ignoreCase = true)
    }

    fun isMalisx2Metadata(metadata: GpuDriverMetadata): Boolean {
        val libName = metadata.libraryName ?: ""
        val name = metadata.name ?: ""
        val vendor = metadata.vendor ?: ""
        return libName.contains("malisx2", ignoreCase = true) ||
               name.contains("malisx2", ignoreCase = true) ||
               vendor.contains("malisx2", ignoreCase = true)
    }

    fun isPanVkMetadata(metadata: GpuDriverMetadata): Boolean {
        if (isMalisx2Metadata(metadata)) return false
        val libName = metadata.libraryName ?: ""
        val name = metadata.name ?: ""
        return libName.contains("panvk", ignoreCase = true) ||
               libName.contains("panfrost", ignoreCase = true) ||
               name.contains("PanVK", ignoreCase = true)
    }

    fun isMalisx2DriverActive(): Boolean {
        // Qualcomm Adreno hardware can NEVER use malisx2 (requires Arm Mali kbase /dev/mali0)
        if (File("/sys/class/kgsl/kgsl-3d0/gpu_model").exists()) {
            return false
        }
        return isMalisx2Metadata(installedCustomDriverData)
    }

    fun isPanVkDriverActive(): Boolean {
        // Qualcomm Adreno hardware can NEVER use PanVK!
        if (File("/sys/class/kgsl/kgsl-3d0/gpu_model").exists()) {
            return false
        }
        return isPanVkMetadata(installedCustomDriverData)
    }

    fun isMaliCompatibleCustomMetadata(metadata: GpuDriverMetadata): Boolean {
        if (metadata.name == null || metadata.libraryName.isNullOrEmpty()) return false
        if (isMalisx2Metadata(metadata)) {
            return Build.VERSION.SDK_INT >= 29
        }
        if (isPanVkMetadata(metadata)) return true
        val name = metadata.name ?: ""
        val ver = metadata.version ?: ""
        val isStorm = name.contains("STORM", ignoreCase = true) || ver.contains("STORM", ignoreCase = true)
        if (isStorm) {
            val regex = Regex("""\d+(\.\d+)*""")
            val verNum = regex.find(ver)?.value ?: regex.find(name)?.value ?: "0"
            val major = verNum.split(".").firstOrNull()?.toIntOrNull() ?: 0
            return major >= 8
        }
        return false
    }

    fun isDriverCompatibleWithAdreno8xx(metadata: GpuDriverMetadata): Boolean {
        // System driver is always 100% compatible
        if (metadata.name == null) {
            return true
        }
        val desc = metadata.description ?: ""
        val name = metadata.name ?: ""
        val version = metadata.packageVersion ?: ""
        val drvVer = metadata.version ?: ""
        val combinedMeta = "$name $desc $version $drvVer"

        // Adreno 840+ (Snapdragon 8 Elite Gen 5 / SM8850) requires explicit A840 / SM8850 driver support.
        // A830-only Turnip / PurpleVK builds report '/unknown' and corrupt A840 UBWC swapchains.
        if (isAdreno840OrNewer()) {
            return combinedMeta.contains("840", ignoreCase = true) ||
                   combinedMeta.contains("8850", ignoreCase = true) ||
                   combinedMeta.contains("Gen 5", ignoreCase = true) ||
                   combinedMeta.contains("A840", ignoreCase = true)
        }

        // Verified STORM DRIVER 3.2.0+ for Adreno 830 / 825 (SM8750 / SM8735)
        if (name.contains("STORM", ignoreCase = true)) {
            val verNum = version.replace("[^0-9.]".toRegex(), "")
            if (verNum.isNotEmpty()) {
                val parts = verNum.split(".").mapNotNull { it.toIntOrNull() }
                if (parts.isNotEmpty() && (parts[0] > 3 || (parts[0] == 3 && parts.getOrElse(1) { 0 } >= 2))) {
                    return true
                }
            }
        }

        if (desc.contains("Adreno 8", ignoreCase = true) || desc.contains("830", ignoreCase = true) || desc.contains("825", ignoreCase = true) || desc.contains("Snapdragon 8 Elite", ignoreCase = true)) {
            return true
        }

        return false
    }

    fun initializeDriverParameters() = synchronized(driverLock) {
        try {
            val extDir = YuzuApplication.appContext.getExternalFilesDir(null)
            val baseDir = extDir ?: YuzuApplication.appContext.filesDir
            fileRedirectionPath = baseDir.canonicalPath + "/gpu/vk_file_redirect/"
            driverInstallationPath = YuzuApplication.appContext.filesDir.canonicalPath + "/gpu_driver/"
        } catch (_: Exception) {
            fileRedirectionPath = YuzuApplication.appContext.filesDir.canonicalPath + "/gpu/vk_file_redirect/"
            driverInstallationPath = YuzuApplication.appContext.filesDir.canonicalPath + "/gpu_driver/"
        }

        initializeDirectories()
        hookLibPath = YuzuApplication.appContext.applicationInfo.nativeLibraryDir + "/"

        // Read active selected zip metadata BEFORE deciding whether a non-Adreno GPU may proceed,
        // fixing the chicken-and-egg bug where gpu_driver/ was wiped before unzipping malisx2/PanVK.
        val activeDriverZipPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Exception) { "" }
        val activeZipMetadata = if (activeDriverZipPath.isNotEmpty()) {
            val f = File(activeDriverZipPath)
            if (f.exists()) getMetadataFromZip(f) else GpuDriverMetadata()
        } else {
            GpuDriverMetadata()
        }
        val isNonAdrenoAllowed = isMalisx2DriverActive() ||
            isPanVkDriverActive() ||
            (isMaliGpu() && isStormDriver8OrNewer()) ||
            (isMaliGpu() && isMaliCompatibleCustomMetadata(activeZipMetadata)) ||
            (!isAdrenoGpu() && (isMalisx2Metadata(activeZipMetadata) && Build.VERSION.SDK_INT >= 29 || isPanVkMetadata(activeZipMetadata)))

        if (!isAdrenoGpu() && !isNonAdrenoAllowed) {
            // On non-Adreno GPUs without a valid Mali/malisx2/PanVK driver, wipe any stray Turnip files
            try {
                driverInstallationPath?.let { path ->
                    val installDir = File(path)
                    if (installDir.exists()) {
                        installDir.deleteRecursively()
                        installDir.mkdirs()
                    }
                }
            } catch (_: Exception) {}
            NativeLibrary.initializeGpuDriver(
                hookLibPath,
                driverInstallationPath,
                null,
                fileRedirectionPath
            )
            return@synchronized
        }

        NativeFreedrenoConfig.reloadFreedrenoConfig()

        // Keep driverInstallationPath strictly synchronized with StringSetting.DRIVER_PATH
        val installDir = File(driverInstallationPath!!)
        val markerFile = File(installDir, ACTIVE_DRIVER_MARKER)
        val recordedZip = if (markerFile.exists()) {
            try { markerFile.readText(Charsets.UTF_8).trim() } catch (_: Exception) { "" }
        } else {
            ""
        }

        if (activeDriverZipPath.isEmpty()) {
            if (!installedCustomDriverData.libraryName.isNullOrEmpty() ||
                markerFile.exists() ||
                File(installDir, META_JSON_FILENAME).exists()
            ) {
                try {
                    installDir.deleteRecursively()
                    installDir.mkdirs()
                } catch (_: Exception) {}
            }
        } else {
            val activeDriverFile = File(activeDriverZipPath)
            val currentInstalled = installedCustomDriverData
            val needsSync = when {
                currentInstalled.libraryName.isNullOrEmpty() -> true
                recordedZip.isNotEmpty() -> !recordedZip.equals(activeDriverZipPath, ignoreCase = true)
                activeDriverFile.exists() -> {
                    val targetMeta = getMetadataFromZip(activeDriverFile)
                    val mismatch = targetMeta.name != null &&
                        (currentInstalled.name != targetMeta.name ||
                            currentInstalled.version != targetMeta.version ||
                            currentInstalled.libraryName != targetMeta.libraryName)
                    if (!mismatch && targetMeta.name != null) {
                        writeActiveDriverMarker(activeDriverFile.absolutePath)
                    }
                    mismatch
                }
                else -> false
            }

            if (needsSync && activeDriverFile.exists()) {
                val metadata = getMetadataFromZip(activeDriverFile)
                if (metadata.name != null && metadata.minApi <= Build.VERSION.SDK_INT &&
                    (!isAdreno8xx() || isDriverCompatibleWithAdreno8xx(metadata))) {
                    try {
                        installDir.deleteRecursively()
                        installDir.mkdirs()
                        FileUtil.unzipToInternalStorage(
                            activeDriverFile.path,
                            installDir
                        )
                        extractAllDriverFiles(activeDriverFile, installDir)
                        writeActiveDriverMarker(activeDriverFile.absolutePath)
                    } catch (_: Exception) {
                    }
                }
            }
        }

        // Guard Adreno hardware against Mali-only drivers (malisx2 / PanVK)
        if (isAdrenoGpu() && (isMalisx2Metadata(installedCustomDriverData) || isPanVkMetadata(installedCustomDriverData))) {
            Log.warning("[GpuDriverHelper] Auto-fallback: Mali-only driver '${installedCustomDriverData.name}' cannot run on Adreno; clearing install dir.")
            try {
                driverInstallationPath?.let { path ->
                    val installDir = File(path)
                    if (installDir.exists()) {
                        installDir.deleteRecursively()
                        installDir.mkdirs()
                    }
                }
            } catch (_: Exception) {}
        }

        // Guard Mali/non-Adreno hardware against incompatible Turnip or API<29 malisx2 drivers
        if (!isAdrenoGpu() && !installedCustomDriverData.libraryName.isNullOrEmpty() &&
            !isMaliCompatibleCustomMetadata(installedCustomDriverData)) {
            Log.warning("[GpuDriverHelper] Auto-fallback: Driver '${installedCustomDriverData.name}' is incompatible with this Mali GPU/API; clearing install dir.")
            try {
                driverInstallationPath?.let { path ->
                    val installDir = File(path)
                    if (installDir.exists()) {
                        installDir.deleteRecursively()
                        installDir.mkdirs()
                    }
                }
            } catch (_: Exception) {}
        }

        // Guard Adreno 8xx (8 Elite / 8s Gen 4) against incompatible legacy A6xx/A7xx Turnip drivers
        if (isAdreno8xx() && !isDriverCompatibleWithAdreno8xx(installedCustomDriverData)) {
            Log.warning("[GpuDriverHelper] Auto-fallback: Installed custom driver '${installedCustomDriverData.name}' is incompatible with Adreno 8xx; reverting to compatible driver.")
            try {
                driverInstallationPath?.let { path ->
                    val installDir = File(path)
                    if (installDir.exists()) {
                        installDir.deleteRecursively()
                        installDir.mkdirs()
                    }
                }
            } catch (_: Exception) {}
        }

        val customLib = installedCustomDriverData.libraryName
        val malisx2Active = isMalisx2DriverActive()
        if (!customLib.isNullOrEmpty() && malisx2Active) {
            // malisx2 is a clean-room Mesa kraid/Bifrost driver for Mali kbase:
            // clear all Turnip and PanVK env vars so they never interfere, and cap Mesa disk cache.
            NativeFreedrenoConfig.clearFreedrenoEnv("DRIRC_CONFIGDIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_DIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
            NativeFreedrenoConfig.clearFreedrenoEnv("TU_DEBUG")
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_MESA_DEBUG")
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_NO_ERROR")
            NativeFreedrenoConfig.setFreedrenoEnv("MESA_SHADER_CACHE_MAX_SIZE", "128M")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DISK_CACHE_SINGLE_FILE")
        } else if (!customLib.isNullOrEmpty() && (isTurnipDriverActive() || isPanVkDriverActive())) {
            val installDir = File(driverInstallationPath!!)
            val isStorm8 = isStormDriver8OrNewer()
            val panVkActive = isPanVkDriverActive()
            val turnipActive = isTurnipDriverActive()

            if (isStorm8 || panVkActive) {
                // Generate clean per-game driconf XML rather than copying raw zip 00-storm.conf (which may carry tu_dont_care_as_load=true)
                val generatedFile = PerGameDrircGenerator.generateForGame(activeGameProgramId, activeGameTitle)
                val configBytes = generatedFile.readBytes()
                try {
                    val redirDir = File(fileRedirectionPath!!)
                    if (!redirDir.exists()) redirDir.mkdirs()
                    File(redirDir, "drirc.xml").writeBytes(configBytes)
                    File(redirDir, "drirc").writeBytes(configBytes)
                    File(redirDir, "00-storm.conf").writeBytes(configBytes)
                    File(redirDir, "drirc.conf").writeBytes(configBytes)

                    File(installDir, "drirc.xml").writeBytes(configBytes)
                    File(installDir, "00-storm.conf").writeBytes(configBytes)
                    File(installDir, "drirc.conf").writeBytes(configBytes)
                    File(installDir, "drirc").writeBytes(configBytes)

                    val homeDir = YuzuApplication.appContext.filesDir
                    File(homeDir, ".drirc").writeBytes(configBytes)
                } catch (_: Throwable) {}

                val targetDrircDir = if (activeGameProgramId.isNotBlank()) {
                    generatedFile.parentFile ?: installDir
                } else {
                    installDir
                }
                val drircXmlFile = if (activeGameProgramId.isNotBlank()) {
                    generatedFile
                } else {
                    File(installDir, "drirc.xml")
                }
                NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", targetDrircDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", targetDrircDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_FILE", drircXmlFile.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)
            } else if (turnipActive) {
                // Issue #94: Third-party Turnip driver (e.g. MrPurple T30, Kimchi, Weab-chan)
                // Set DRIRC_CONFIGDIR and MESA_DRIRC_DIR to installDir (preserving bundled .conf / sysmem config),
                // clear MESA_DRIRC_FILE, and delete any stale .drirc in filesDir
                NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", installDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", installDir.absolutePath)
                NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
                NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)

                val staleDrirc = File(YuzuApplication.appContext.filesDir, ".drirc")
                if (staleDrirc.exists()) {
                    try { staleDrirc.delete() } catch (_: Throwable) {}
                }
            }

            // Sparse multi-file Mesa shader cache capped at 128M (bounds RAM mmap footprint)
            NativeFreedrenoConfig.setFreedrenoEnv("MESA_SHADER_CACHE_MAX_SIZE", "128M")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DISK_CACHE_SINGLE_FILE")

            var model = ""
            try {
                model = hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: ""
            } catch (e: Throwable) {
                // Ignore fallback
            }

            // ARM Mali / PanVK stability and driver flags
            if (isPanVkDriverActive() || model.contains("Mali", ignoreCase = true)) {
                NativeFreedrenoConfig.setFreedrenoEnv("PAN_MESA_DEBUG", "nocrc,noafbc")
                NativeFreedrenoConfig.setFreedrenoEnv("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER", "1")
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_NO_ERROR", "1")
            }
        } else {
            // Reverting to system driver or non-turnip: clear all freedreno drirc variables
            NativeFreedrenoConfig.clearFreedrenoEnv("DRIRC_CONFIGDIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_DIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
            NativeFreedrenoConfig.clearFreedrenoEnv("TU_DEBUG")
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_MESA_DEBUG")
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_NO_ERROR")
        }

        // Preserve per_game_drirc and per-TID TU_DEBUG whenever a game TID is already active
        if (!customLib.isNullOrEmpty() && (isTurnipDriverActive() || isPanVkDriverActive()) &&
            activeGameProgramId.isNotBlank()) {
            applyPerGameDriverConfig(activeGameProgramId, activeGameTitle)
        }

        val initSignature = listOf(
            hookLibPath ?: "",
            driverInstallationPath ?: "",
            customLib ?: "",
            fileRedirectionPath ?: ""
        ).joinToString("|")
        if (initSignature != lastNativeDriverInitSignature) {
            lastNativeDriverInitSignature = initSignature
            NativeLibrary.initializeGpuDriver(
                hookLibPath,
                driverInstallationPath,
                customLib,
                fileRedirectionPath
            )
        }
    }

    // Per-TID Turnip driver hints (verified real TU_DEBUG flags only: nolrz, sysmem, gmem, nobin, noubwc, flushall, syncdraw)
    val PER_TID_TURNIP_DEBUG_HINTS: Map<String, List<String>> = mapOf(
        "0100726014352000" to listOf("nolrz", "noubwc"), // Diablo II: Resurrected
        "01002A801458A000" to listOf("nolrz", "noubwc"), // Diablo II: Resurrected (US)
        "0100916014D8C000" to listOf("nolrz", "noubwc"), // Diablo II: Resurrected (EU)
        "0100A83022A22000" to listOf("nolrz"), // Absolum (nolrz fixes Gen8 coarse LRZ cutouts; GMEM preserved)
        "010067300059A000" to listOf("nolrz", "noubwc"), // Mario + Rabbids: Kingdom Battle
        "010067300B762000" to listOf("nolrz", "noubwc")  // Mario + Rabbids: Kingdom Battle (EU)
    )

    private fun resolvePerTidTurnipHints(programIdHex: String, isStorm8: Boolean): List<String> {
        var rawId = programIdHex.trim().uppercase()
        if (rawId.startsWith("0X")) {
            rawId = rawId.substring(2)
        }
        val cleanId = if (rawId.length == 16) rawId.substring(0, 13) + "000" else rawId
        if (!isStorm8 && cleanId == "01007EF00011E000") {
            return emptyList()
        }
        val baseFlags = PER_TID_TURNIP_DEBUG_HINTS[cleanId] ?: emptyList()
        if (baseFlags.isEmpty()) {
            return emptyList()
        }
        if (!isAdreno8xx()) {
            return listOf("nolrz")
        }
        return baseFlags
    }

    fun applyPerGameDriverConfig(programIdHex: String, gameTitle: String) = synchronized(driverLock) {
        activeGameProgramId = programIdHex
        activeGameTitle = gameTitle
        val customLib = installedCustomDriverData.libraryName
        val turnipActive = isTurnipDriverActive()
        val panVkActive = isPanVkDriverActive()
        if (customLib.isNullOrEmpty() || driverInstallationPath.isNullOrEmpty() || (!turnipActive && !panVkActive)) {
            NativeFreedrenoConfig.clearFreedrenoEnv("DRIRC_CONFIGDIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_DIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
            return@synchronized
        }

        try {
            val installDir = File(driverInstallationPath!!)
            val isStorm8 = isStormDriver8OrNewer()

            if (isStorm8 || panVkActive) {
                val profileType = PerGameDrircGenerator.detectProfileType(programIdHex, gameTitle)
                val generatedConfigFile = PerGameDrircGenerator.generateForGame(programIdHex, gameTitle)
                val configBytes = generatedConfigFile.readBytes()
                val stormConfFile = File(installDir, "00-storm.conf")
                val isMarioRabbids = profileType == PerGameDrircGenerator.GameProfileType.MARIO_RABBIDS ||
                    programIdHex.equals("010067300059A000", ignoreCase = true) ||
                    programIdHex.equals("010067300B762000", ignoreCase = true) ||
                    programIdHex.equals("0100317013770000", ignoreCase = true) ||
                    programIdHex.equals("010032C00AC58000", ignoreCase = true) ||
                    programIdHex.equals("01005CA00F966000", ignoreCase = true) ||
                    gameTitle.contains("rabbids", ignoreCase = true)

                val selectedDriverPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Exception) { "" }

                val perGameDir = File(installDir, "per_game_drirc")
                if (!perGameDir.exists()) perGameDir.mkdirs()
                val perGameDrircFile = File(perGameDir, "drirc.xml")
                perGameDrircFile.writeBytes(configBytes)
                File(perGameDir, "00-storm.conf").writeBytes(configBytes)
                File(perGameDir, "drirc.conf").writeBytes(configBytes)
                stormConfFile.writeBytes(configBytes)
                File(installDir, "drirc.xml").writeBytes(configBytes)
                File(installDir, "drirc.conf").writeBytes(configBytes)
                File(installDir, "drirc").writeBytes(configBytes)

                val redirPath = fileRedirectionPath
                if (!redirPath.isNullOrEmpty()) {
                    val redirDir = File(redirPath)
                    if (!redirDir.exists()) redirDir.mkdirs()
                    File(redirDir, "drirc.xml").writeBytes(configBytes)
                    File(redirDir, "drirc").writeBytes(configBytes)
                    File(redirDir, "00-storm.conf").writeBytes(configBytes)
                    File(redirDir, "drirc.conf").writeBytes(configBytes)
                }

                val homeDir = YuzuApplication.appContext.filesDir
                File(homeDir, ".drirc").writeBytes(configBytes)

                NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", perGameDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", perGameDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_FILE", perGameDrircFile.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)

                if (NativeFreedrenoConfig.isFreedrenoEnvSet("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")) {
                    NativeFreedrenoConfig.clearFreedrenoEnv("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")
                }
                Log.info("[GpuDriverHelper] Successfully applied per-game driver config for $gameTitle ($programIdHex) -> ${perGameDrircFile.absolutePath}")
            } else {
                // Issue #94: Third-party Turnip driver (turnipActive && !isStorm8, e.g. MrPurple T30, Kimchi, Weab-chan)
                // Set DRIRC_CONFIGDIR and MESA_DRIRC_DIR to installDir (preserving bundled .conf / sysmem config),
                // clear MESA_DRIRC_FILE, and delete any stale .drirc in filesDir
                NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", installDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", installDir.absolutePath)
                NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
                NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)

                val staleDrirc = File(YuzuApplication.appContext.filesDir, ".drirc")
                if (staleDrirc.exists()) {
                    try { staleDrirc.delete() } catch (_: Throwable) {}
                }
                if (NativeFreedrenoConfig.isFreedrenoEnvSet("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")) {
                    NativeFreedrenoConfig.clearFreedrenoEnv("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")
                }
                Log.info("[GpuDriverHelper] Preserved third-party Turnip driver native config for $gameTitle ($programIdHex) in ${installDir.absolutePath}")
            }

            // Synchronize detected vendor with GameFixDatabase
            val isMaliOrPanVk = panVkActive || isMaliGpu()
            org.yuzu.yuzu_emu.model.GameFixDatabase.setDetectedGpuVendor(if (isMaliOrPanVk) "mali" else "adreno")
            if (!turnipActive) {
                return@synchronized
            }

            val normalizedTid = programIdHex.uppercase()
            val perTidHints = resolvePerTidTurnipHints(normalizedTid, isStorm8)
            val flags = linkedSetOf<String>()
            if (isAdreno840OrNewer()) {
                flags.add("noubwc")
            }
            flags.addAll(perTidHints)

            if (flags.isNotEmpty()) {
                val tuDebugStr = flags.joinToString(",")
                NativeFreedrenoConfig.setFreedrenoEnv("TU_DEBUG", tuDebugStr)
                Log.info("[GpuDriverHelper] Applied clean per-TID Turnip TU_DEBUG for $normalizedTid ($gameTitle): TU_DEBUG=$tuDebugStr")
            } else {
                NativeFreedrenoConfig.clearFreedrenoEnv("TU_DEBUG")
                Log.info("[GpuDriverHelper] Cleared TU_DEBUG for $normalizedTid ($gameTitle)")
            }
        } catch (t: Throwable) {
            Log.error("[GpuDriverHelper] Failed to apply per-game driver config: ${t.message}")
        }
    }

    fun getDrivers(): MutableList<Pair<String, GpuDriverMetadata>> {
        val driverZips = File(driverStoragePath).listFiles()
        val drivers: MutableList<Pair<String, GpuDriverMetadata>> =
            driverZips
                ?.mapNotNull {
                    val metadata = getMetadataFromZip(it)
                    metadata.name?.let { _ -> Pair(it.path, metadata) }
                }
                ?.sortedByDescending { it: Pair<String, GpuDriverMetadata> -> it.second.name }
                ?.distinct()
                ?.toMutableList() ?: mutableListOf()
        return drivers
    }

    private fun resetDriverInstallDirOnly() {
        lastNativeDriverInitSignature = null
        driverInstallationPath?.let { path ->
            val dir = File(path)
            dir.deleteRecursively()
            dir.mkdirs()
        }
        clearVulkanPipelineCaches()
    }

    fun installDefaultDriver() = synchronized(driverLock) {
        resetDriverInstallDirOnly()
        initializeDriverParameters()
    }

    fun copyDriverToInternalStorage(driverUri: Uri): Boolean {
        // Ensure we have directories.
        initializeDirectories()

        // Copy the zip file URI to user data
        val copiedFile =
            FileUtil.copyUriToInternalStorage(driverUri, driverStoragePath) ?: return false

        // Validate driver
        val metadata = getMetadataFromZip(copiedFile)
        if (metadata.name == null) {
            copiedFile.delete()
            return false
        }

        if (metadata.minApi > Build.VERSION.SDK_INT) {
            copiedFile.delete()
            return false
        }
        return true
    }

    /**
     * Copies driver zip into user data directory so that it can be exported along with
     * other user data and also unzipped into the installation directory
     */
    fun installCustomDriver(driverUri: Uri): Boolean = synchronized(driverLock) {
        // Ensure we have directories and clean old files without resetting native driver to null mid-install.
        initializeDirectories()
        resetDriverInstallDirOnly()

        // Copy the zip file URI to user data
        val copiedFile =
            FileUtil.copyUriToInternalStorage(driverUri, driverStoragePath) ?: return@synchronized false

        // Validate driver
        val metadata = getMetadataFromZip(copiedFile)
        if (metadata.name == null) {
            copiedFile.delete()
            return@synchronized false
        }

        if (metadata.minApi > Build.VERSION.SDK_INT) {
            copiedFile.delete()
            return@synchronized false
        }

        // Unzip the driver.
        try {
            FileUtil.unzipToInternalStorage(
                copiedFile.path,
                File(driverInstallationPath!!)
            )
            extractAllDriverFiles(copiedFile, File(driverInstallationPath!!))
            writeActiveDriverMarker(copiedFile.absolutePath)
            clearInflightDriverMarker()
            clearVulkanPipelineCaches()
        } catch (e: SecurityException) {
            return@synchronized false
        }

        // Initialize the driver parameters.
        initializeDriverParameters()

        return@synchronized true
    }

    /**
     * Unzips driver into installation directory
     */
    fun installCustomDriver(driver: File): Boolean = synchronized(driverLock) {
        // Validate driver BEFORE wiping existing installation directory so failed installs keep prior state intact.
        val metadata = getMetadataFromZip(driver)
        if (metadata.name == null) {
            driver.delete()
            return@synchronized false
        }

        if (metadata.minApi > Build.VERSION.SDK_INT) {
            Log.warning("[GpuDriverHelper] Driver '${metadata.name}' requires Android API ${metadata.minApi}, device is API ${Build.VERSION.SDK_INT}")
            return@synchronized false
        }

        // Ensure we have directories and clean old files without resetting native driver to null mid-install.
        initializeDirectories()
        resetDriverInstallDirOnly()

        // Unzip the driver to the private installation directory
        try {
            FileUtil.unzipToInternalStorage(
                driver.path,
                File(driverInstallationPath!!)
            )
            extractAllDriverFiles(driver, File(driverInstallationPath!!))
            writeActiveDriverMarker(driver.absolutePath)
            clearInflightDriverMarker()
            clearVulkanPipelineCaches()
        } catch (e: SecurityException) {
            return@synchronized false
        }

        // Initialize the driver parameters.
        initializeDriverParameters()

        return@synchronized true
    }

    /**
     * Extracts all .so libraries (including vulkan.adreno_gen8.so, vulkan.adreno_a7xx.so,
     * vulkan.freedreno.so) and driver config files directly into destDir.
     */
    fun extractAllDriverFiles(zipFile: File, destDir: File) {
        if (!destDir.exists()) destDir.mkdirs()
        try {
            // 1. Flatten any .so and config files that might be in subfolders of destDir
            destDir.walkTopDown().forEach { file ->
                if (file.isFile && file.parentFile != destDir) {
                    val baseName = file.name
                    if (baseName.endsWith(".so", ignoreCase = true) ||
                        baseName.endsWith(".conf", ignoreCase = true) ||
                        baseName.endsWith(".xml", ignoreCase = true) ||
                        baseName.equals("meta.json", ignoreCase = true) ||
                        baseName.startsWith("drirc", ignoreCase = true)
                    ) {
                        val outFile = File(destDir, baseName)
                        if (!outFile.exists() || outFile.length() != file.length()) {
                            try { file.copyTo(outFile, overwrite = true) } catch (_: Throwable) {}
                        }
                    }
                }
            }

            // 2. Scan zip entries directly to ensure all .so files and configs are placed directly in destDir
            ZipFile(zipFile).use { zf ->
                val entries = zf.entries()
                while (entries.hasMoreElements()) {
                    val entry = entries.nextElement()
                    if (entry.isDirectory) continue
                    val baseName = entry.name.substringAfterLast('/').substringAfterLast('\\')
                    if (baseName.endsWith(".so", ignoreCase = true) ||
                        baseName.endsWith(".conf", ignoreCase = true) ||
                        baseName.endsWith(".xml", ignoreCase = true) ||
                        baseName.equals("meta.json", ignoreCase = true) ||
                        baseName.startsWith("drirc", ignoreCase = true)
                    ) {
                        val outFile = File(destDir, baseName)
                        if (!outFile.exists() || outFile.length() != entry.size) {
                            zf.getInputStream(entry).use { inStream ->
                                java.io.FileOutputStream(outFile).use { outStream ->
                                    inStream.copyTo(outStream)
                                }
                            }
                        }
                    }
                }
            }
        } catch (_: Throwable) {}
    }

    private fun writeActiveDriverMarker(zipPath: String) {
        try {
            val installPath = driverInstallationPath ?: return
            val installDir = File(installPath)
            if (!installDir.exists()) installDir.mkdirs()
            File(installDir, ACTIVE_DRIVER_MARKER).writeText(zipPath, Charsets.UTF_8)
        } catch (_: Throwable) {}
    }

    fun isDriverBlacklisted(driverNameOrPath: String): Boolean {
        if (driverNameOrPath.isBlank()) return false
        return try {
            val installPath = driverInstallationPath ?: return false
            val blacklistFile = File(installPath, ".driver_blacklisted")
            if (!blacklistFile.exists()) return false
            val recorded = blacklistFile.readText(Charsets.UTF_8)
            val baseName = File(driverNameOrPath).name
            recorded.contains(driverNameOrPath, ignoreCase = true) ||
                (baseName.isNotEmpty() && recorded.contains(baseName, ignoreCase = true))
        } catch (_: Throwable) {
            false
        }
    }

    fun clearDriverBlacklist() {
        try {
            val installPath = driverInstallationPath ?: return
            File(installPath, ".driver_blacklisted").delete()
            File(installPath, ".driver_inflight").delete()
            File(installPath, ".driver_inflight_dlopen").delete()
            File(installPath, ".driver_inflight_vkinit").delete()
        } catch (_: Throwable) {}
    }

    fun prepareInflightDriverMarker(): File? {
        return try {
            val installPath = driverInstallationPath ?: return null
            val activePath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Throwable) { "" }
            if (activePath.isBlank()) return null
            val installDir = File(installPath)
            if (!installDir.exists()) installDir.mkdirs()
            val marker = File(installDir, ".driver_inflight")
            val libName = installedCustomDriverData.libraryName ?: ""
            marker.writeText("$activePath\n$libName\n", Charsets.UTF_8)
            marker
        } catch (_: Throwable) {
            null
        }
    }

    fun clearInflightDriverMarker() {
        try {
            val installPath = driverInstallationPath ?: return
            File(installPath, ".driver_inflight").delete()
            File(installPath, ".driver_inflight_dlopen").delete()
            File(installPath, ".driver_inflight_vkinit").delete()
        } catch (_: Throwable) {}
    }

    fun recoverFromInflightDriverCrashIfNeeded(context: Context = YuzuApplication.appContext): Boolean = synchronized(driverLock) {
        return@synchronized try {
            val installPath = driverInstallationPath ?: return@synchronized false
            val installDir = File(installPath)
            // Legacy session-wide .driver_inflight markers can remain after am force-stop; purge silently.
            val legacyMarker = File(installDir, ".driver_inflight")
            if (legacyMarker.exists()) {
                runCatching { legacyMarker.delete() }
            }
            val mDlopen = File(installDir, ".driver_inflight_dlopen")
            val mVkInit = File(installDir, ".driver_inflight_vkinit")
            if (!mDlopen.exists() && !mVkInit.exists()) return@synchronized false

            val failedInfo = buildString {
                if (mDlopen.exists()) appendLine(runCatching { mDlopen.readText(Charsets.UTF_8) }.getOrDefault(""))
                if (mVkInit.exists()) appendLine(runCatching { mVkInit.readText(Charsets.UTF_8) }.getOrDefault(""))
                val currentPath = runCatching { StringSetting.DRIVER_PATH.getString() }.getOrDefault("")
                if (currentPath.isNotBlank()) appendLine(currentPath)
            }.trim()

            mDlopen.delete()
            mVkInit.delete()
            Log.warning("[GpuDriverHelper] Detected previous custom driver init crash; blacklisting failing driver and falling back to System Vulkan: $failedInfo")
            StringSetting.DRIVER_PATH.global = true
            StringSetting.DRIVER_PATH.setString("")
            NativeConfig.saveGlobalConfig()
            installDefaultDriver()
            if (!installDir.exists()) installDir.mkdirs()
            val blacklistFile = File(installDir, ".driver_blacklisted")
            if (failedInfo.isNotEmpty()) {
                blacklistFile.writeText(failedInfo, Charsets.UTF_8)
            } else {
                blacklistFile.writeText("crashed_custom_driver", Charsets.UTF_8)
            }
            true
        } catch (t: Throwable) {
            Log.warning("[GpuDriverHelper] recoverFromInflightDriverCrashIfNeeded failed: ${t.message}")
            false
        }
    }

    fun clearVulkanPipelineCaches(context: Context = YuzuApplication.appContext) {
        val targets = linkedSetOf<File>()
        val roots = listOfNotNull(
            DirectoryInitialization.rawUserPath?.let { File(it) },
            DirectoryInitialization.userDirectory?.let { File(it) },
            context.getExternalFilesDir(null),
            context.cacheDir,
            context.filesDir
        ).distinct()

        for (root in roots) {
            targets.add(File(root, "vulkan_pipelines.bin"))
            targets.add(File(root, "vulkan_static_pipelines.bin"))
            targets.add(File(root, "cache/vulkan_pipelines.bin"))
            targets.add(File(root, "cache/vulkan_static_pipelines.bin"))
            targets.add(File(root, "shader/vulkan_static_pipelines.bin"))
            targets.add(File(root, "cache/shader/vulkan_static_pipelines.bin"))
            for (shaderBase in listOf(File(root, "shader"), File(root, "cache/shader"))) {
                if (shaderBase.exists() && shaderBase.isDirectory) {
                    shaderBase.listFiles()?.forEach { sub ->
                        if (sub.isDirectory) {
                            targets.add(File(sub, "vulkan_pipelines.bin"))
                        }
                    }
                }
            }
        }

        for (target in targets) {
            try {
                if (target.exists() && target.isFile) {
                    target.delete()
                }
            } catch (_: Throwable) {}
        }
    }

    fun clearShaderAndPipelineCaches(context: Context = YuzuApplication.appContext) {
        val targets = linkedSetOf<File>()
        val userDir = DirectoryInitialization.rawUserPath ?: DirectoryInitialization.userDirectory
        if (!userDir.isNullOrEmpty()) {
            targets.add(File(userDir, "cache/shader"))
            targets.add(File(userDir, "shader"))
            targets.add(File(userDir, "cache/mesa_shader_cache"))
            targets.add(File(userDir, "mesa_shader_cache"))
            targets.add(File(userDir, "cache/vulkan_pipelines.bin"))
            targets.add(File(userDir, "cache/vulkan_static_pipelines.bin"))
        }
        val extFiles = context.getExternalFilesDir(null)
        if (extFiles != null) {
            targets.add(File(extFiles, "shader"))
            targets.add(File(extFiles, "cache/shader"))
            targets.add(File(extFiles, "mesa_shader_cache"))
            targets.add(File(extFiles, "cache/mesa_shader_cache"))
            targets.add(File(extFiles, "vulkan_pipelines.bin"))
            targets.add(File(extFiles, "vulkan_static_pipelines.bin"))
        }
        val cacheDir = context.cacheDir
        if (cacheDir != null) {
            targets.add(File(cacheDir, "shader"))
            targets.add(File(cacheDir, "mesa_shader_cache"))
            targets.add(File(cacheDir, "vulkan_pipelines.bin"))
            targets.add(File(cacheDir, "vulkan_static_pipelines.bin"))
        }
        val filesDir = context.filesDir
        if (filesDir != null) {
            targets.add(File(filesDir, "shader"))
            targets.add(File(filesDir, "cache/shader"))
            targets.add(File(filesDir, "mesa_shader_cache"))
            targets.add(File(filesDir, "gpu/vk_file_redirect/mesa_shader_cache"))
            targets.add(File(filesDir, "vulkan_pipelines.bin"))
            targets.add(File(filesDir, "vulkan_static_pipelines.bin"))
        }

        for (target in targets) {
            try {
                if (target.exists()) {
                    if (target.isDirectory) {
                        target.deleteRecursively()
                    } else {
                        target.delete()
                    }
                    Log.info("[GpuDriverHelper] Deleted shader cache target: ${target.absolutePath}")
                }
            } catch (e: Throwable) {
                Log.error("[GpuDriverHelper] Failed to delete shader cache target ${target.absolutePath}: ${e.message}")
            }
        }
    }

    /**
     * Checks if STORM_DRIVER_12.0.0.zip is present or if user had 10.0.0/11.0.0 selected,
     * running at most once per device migration so manual driver selection is never overridden.
     */
    fun checkAndAutoUpgradeStormDriver(context: Context = YuzuApplication.appContext) = synchronized(driverLock) {
        try {
            initializeDirectories()
            if (recoverFromInflightDriverCrashIfNeeded(context)) {
                return@synchronized
            }
            val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(context)
            val migratedV3 = prefs.getBoolean(PREF_DRIVER_MIGRATED_12_V1, false)
            if (!migratedV3 && !isMaliGpu() && !isXclipseGpu()) {
                val storageDirCheck = File(driverStoragePath)
                val driver12Present = listOf(
                    File("/storage/emulated/0/STORM SWITCH/gpu_drivers", "STORM_DRIVER_12.0.0.zip"),
                    File("/sdcard/STORM SWITCH/gpu_drivers", "STORM_DRIVER_12.0.0.zip"),
                    File(storageDirCheck, "STORM_DRIVER_12.0.0.zip")
                ).any { it.exists() && it.length() > 1024L }
                if (driver12Present) {
                    clearDriverBlacklist()
                    prefs.edit().putBoolean(PREF_USER_EXPLICIT_SYSTEM_DRIVER, false).apply()
                }
            }
            if (prefs.getBoolean(PREF_USER_EXPLICIT_SYSTEM_DRIVER, false)) {
                return@synchronized
            }
            if (migratedV3) {
                return@synchronized
            }

            // Guard against Mali / Samsung Xclipse devices - never activate Turnip custom drivers
            if (isMaliGpu() || isXclipseGpu()) {
                Log.info("[GpuDriverHelper] Mali or Samsung Xclipse GPU detected; skipping Turnip custom driver auto-upgrade.")
                prefs.edit().putBoolean(PREF_DRIVER_MIGRATED_12_V1, true).apply()
                return
            }

            initializeDirectories()
            val storagePathStr = driverStoragePath
            val storageDir = File(storagePathStr)
            val driver12FileName = "STORM_DRIVER_12.0.0.zip"
            val targetDriver12 = File(storageDir, driver12FileName)

            val currentSelectedPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Throwable) { "" }
            val hadOlderStormSelected = currentSelectedPath.contains("10.0.0", ignoreCase = true) ||
                currentSelectedPath.contains("11.0.0", ignoreCase = true) ||
                currentSelectedPath.contains("STORM_DRIVER_10", ignoreCase = true) ||
                currentSelectedPath.contains("STORM_DRIVER_11", ignoreCase = true)

            // Check if device is Qualcomm / Adreno OR user previously had a STORM_DRIVER_*.zip installed
            val kgslExists = File("/sys/class/kgsl/kgsl-3d0/gpu_model").exists()
            val hardwareQcom = Build.HARDWARE.contains("qcom", ignoreCase = true) ||
                Build.BOARD.contains("qcom", ignoreCase = true) ||
                Build.MANUFACTURER.contains("qualcomm", ignoreCase = true)
            val socQcom = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                try { Build.SOC_MANUFACTURER.contains("qualcomm", ignoreCase = true) } catch (_: Throwable) { false }
            } else false
            val adrenoGpu = try { isAdrenoGpu() } catch (_: Throwable) { false }
            val hadStormInstalled = hadOlderStormSelected ||
                currentSelectedPath.contains("STORM_DRIVER", ignoreCase = true) ||
                try {
                    val meta = installedCustomDriverData
                    meta.name?.contains("STORM", ignoreCase = true) == true || meta.version?.contains("STORM", ignoreCase = true) == true
                } catch (_: Throwable) { false }

            val isEligible = kgslExists || hardwareQcom || socQcom || adrenoGpu || hadStormInstalled
            if (!isEligible) {
                return@synchronized
            }

            // If /storage/emulated/0/STORM SWITCH/gpu_drivers/STORM_DRIVER_12.0.0.zip exists, sync to internal storage
            val primaryExternalDir = File("/storage/emulated/0/STORM SWITCH/gpu_drivers")
            val primaryDriver12 = File(primaryExternalDir, driver12FileName)

            if (!targetDriver12.exists() || targetDriver12.length() < 1024L) {
                if (primaryDriver12.exists() && primaryDriver12.length() > 1024L) {
                    try {
                        if (!storageDir.exists()) storageDir.mkdirs()
                        primaryDriver12.copyTo(targetDriver12, overwrite = true)
                        Log.info("[GpuDriverHelper] Synced existing $driver12FileName from external storage to internal storage")
                    } catch (_: Throwable) {}
                }
            }

            val candidateDirs = listOfNotNull(
                storageDir,
                DirectoryInitialization.userDirectory?.let { File(it, "gpu_drivers") },
                primaryExternalDir,
                File("/sdcard/STORM SWITCH/gpu_drivers")
            ).distinct()

            var foundDriver12: File? = null
            for (dir in candidateDirs) {
                if (!dir.exists() || !dir.isDirectory) continue
                val candidate = File(dir, driver12FileName)
                if (candidate.exists() && candidate.length() > 1024L) {
                    foundDriver12 = candidate
                    break
                }
                val any12 = dir.listFiles { f -> f.isFile && f.name.startsWith("STORM_DRIVER_12", ignoreCase = true) && f.name.endsWith(".zip", ignoreCase = true) }
                if (!any12.isNullOrEmpty()) {
                    foundDriver12 = any12.first()
                    break
                }
            }

            val isSystemDefault = currentSelectedPath.isEmpty() || !File(currentSelectedPath).exists()
            val installedMeta = installedCustomDriverData
            val is12Installed = (installedMeta.version?.contains("12.0") == true || installedMeta.name?.contains("12.0") == true) &&
                File(driverInstallationPath!!, "meta.json").exists()

            if (foundDriver12 != null) {
                if (isDriverBlacklisted(foundDriver12.absolutePath) ||
                    isDriverBlacklisted("STORM_DRIVER_12") ||
                    isDriverBlacklisted("vulkan.adreno_gen8.so")
                ) {
                    Log.warning("[GpuDriverHelper] Skipping auto-upgrade to STORM DRIVER 12.0.0 because it previously crashed and is blacklisted")
                    return@synchronized
                }
                val shouldActivate = hadOlderStormSelected || isSystemDefault || !is12Installed
                if (shouldActivate) {
                    val finalDriverFile = if (foundDriver12.canonicalPath == targetDriver12.canonicalPath) {
                        foundDriver12
                    } else {
                        try {
                            foundDriver12.copyTo(targetDriver12, overwrite = true)
                            targetDriver12
                        } catch (_: Throwable) {
                            foundDriver12
                        }
                    }

                    if (installCustomDriver(finalDriverFile)) {
                        val destDir = File(driverInstallationPath!!)
                        extractAllDriverFiles(finalDriverFile, destDir)
                        writeActiveDriverMarker(finalDriverFile.absolutePath)
                        StringSetting.DRIVER_PATH.global = true
                        StringSetting.DRIVER_PATH.setString(finalDriverFile.absolutePath)
                        NativeConfig.saveGlobalConfig()
                        try {
                            prefs
                                .edit()
                                .putString("driver_path", finalDriverFile.absolutePath)
                                .putString("driver_data", finalDriverFile.absolutePath)
                                .putBoolean(PREF_DRIVER_MIGRATED_12_V1, true)
                                .apply()
                        } catch (_: Throwable) {}
                        Log.info("[GpuDriverHelper] Auto-upgraded to STORM DRIVER 12.0.0 at ${finalDriverFile.absolutePath}")
                    }
                } else if (is12Installed && !isSystemDefault) {
                    prefs.edit().putBoolean(PREF_DRIVER_MIGRATED_12_V1, true).apply()
                }
            }
        } catch (e: Throwable) {
            Log.warning("[GpuDriverHelper] checkAndAutoUpgradeStormDriver failed: ${e.message}")
        }
    }

    /**
     * Verifies driver extraction & inflight crash protection immediately before launching a game.
     */
    fun ensureStormDriverReadyForLaunch(context: Context = YuzuApplication.appContext): Boolean = synchronized(driverLock) {
        return@synchronized try {
            initializeDirectories()
            if (recoverFromInflightDriverCrashIfNeeded(context)) {
                return@synchronized false
            }

            val prefs = androidx.preference.PreferenceManager.getDefaultSharedPreferences(context)
            var currentPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Throwable) { "" }
            if (currentPath.isBlank() &&
                (!prefs.getBoolean(PREF_USER_EXPLICIT_SYSTEM_DRIVER, false) ||
                    !prefs.getBoolean(PREF_DRIVER_MIGRATED_12_V1, false)) &&
                !isMaliGpu() && !isXclipseGpu()
            ) {
                checkAndAutoUpgradeStormDriver(context)
                currentPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Throwable) { "" }
            }

            if (currentPath.isBlank()) {
                initializeDriverParameters()
                return@synchronized false
            }

            if (isDriverBlacklisted(currentPath)) {
                Log.warning("[GpuDriverHelper] Active driver is blacklisted after previous crash; falling back to System Vulkan")
                StringSetting.DRIVER_PATH.global = true
                StringSetting.DRIVER_PATH.setString("")
                NativeConfig.saveGlobalConfig()
                installDefaultDriver()
                return@synchronized false
            }

            val driverFile = File(currentPath)
            if (!driverFile.exists()) {
                initializeDriverParameters()
                return@synchronized false
            }

            val installDir = driverInstallationPath?.let { File(it) }
            val hasSo = installDir?.listFiles()?.any { f -> f.isFile && f.name.endsWith(".so", ignoreCase = true) } == true
            val markerMatches = installDir?.let {
                val m = File(it, ACTIVE_DRIVER_MARKER)
                m.exists() && runCatching { m.readText(Charsets.UTF_8).trim().equals(driverFile.absolutePath, ignoreCase = true) }.getOrDefault(false)
            } == true

            if (!hasSo || !markerMatches || installedCustomDriverData.libraryName.isNullOrEmpty()) {
                installCustomDriver(driverFile)
            } else {
                initializeDriverParameters()
            }
            true
        } catch (t: Throwable) {
            Log.warning("[GpuDriverHelper] ensureStormDriverReadyForLaunch failed: ${t.message}")
            false
        }
    }

    /**
     * Takes in a zip file and reads the meta.json file for presentation to the UI
     *
     * @param driver Zip containing driver and meta.json file
     * @return A non-null [GpuDriverMetadata] instance that may have null members
     */
    fun getMetadataFromZip(driver: File): GpuDriverMetadata {
        if (!driver.exists()) {
            return GpuDriverMetadata()
        }

        try {
            ZipFile(driver).use { zf ->
                val entries = zf.entries()
                while (entries.hasMoreElements()) {
                    val entry = entries.nextElement()
                    val entryName = entry.name.substringAfterLast('/')
                    if (!entry.isDirectory && (entryName.equals("meta.json", ignoreCase = true) || (entryName.endsWith(".json", ignoreCase = true) && !entryName.contains(".metadata.")))) {
                        zf.getInputStream(entry).use {
                            val meta = GpuDriverMetadata(it, entry.size)
                            if (meta.name != null) {
                                return meta
                            }
                        }
                    }
                }
            }
        } catch (_: ZipException) {
        } catch (_: FileNotFoundException) {
        }
        return GpuDriverMetadata()

    }

    external fun supportsCustomDriverLoading(): Boolean

    external fun getSystemDriverInfo(
        surface: Surface = Surface(SurfaceTexture(true)),
        hookLibPath: String = GpuDriverHelper.hookLibPath ?: (YuzuApplication.appContext.applicationInfo.nativeLibraryDir + "/")
    ): Array<String>?

    external fun getGpuModel(
        surface: Surface = Surface(SurfaceTexture(true)),
        hookLibPath: String
    ): String?

    fun isAdrenoGpu(): Boolean {
        return try {
            if (File("/dev/kgsl-3d0").exists() || File("/sys/class/kgsl/kgsl-3d0/gpu_model").exists()) {
                return true
            }
            if (File("/dev/mali0").exists() || File("/dev/panfrost").exists() || File("/dev/panthor").exists()) {
                return false
            }
            val model = (hookLibPath?.let { getGpuModel(hookLibPath = it) } ?: "").lowercase()
            model.contains("adreno")
        } catch (e: Throwable) {
            false
        }
    }

    // Parse the custom driver metadata to retrieve the name.
    val installedCustomDriverData: GpuDriverMetadata
        get() = GpuDriverMetadata(File(driverInstallationPath + META_JSON_FILENAME))

    val customDriverSettingData: GpuDriverMetadata
        get() = getMetadataFromZip(File(StringSetting.DRIVER_PATH.getString()))

    fun initializeDirectories() {
        try {
            fileRedirectionPath?.let { File(it).mkdirs() }
            driverInstallationPath?.let { File(it).mkdirs() }
            val storagePath = try { driverStoragePath } catch (_: Exception) { null }
            storagePath?.let { File(it).mkdirs() }
        } catch (_: Exception) {}
    }

    /**
     * Checks if a driver zip with the given filename is already present and valid in the
     * internal driver storage directory. Validation requires a readable meta.json with a name.
     */
    fun isDriverZipInstalledByName(fileName: String): Boolean {
        // Normalize separators in case upstream sent a path
        val baseName = fileName.substringAfterLast('/')
            .substringAfterLast('\\')
        val candidate = File("$driverStoragePath$baseName")
        if (!candidate.exists() || candidate.length() == 0L) return false
        val metadata = getMetadataFromZip(candidate)
        return metadata.name != null
    }

    /**
     * Updates all per-game custom configs to use the global driver.
     */
    fun applyDriverGloballyToAllCustomConfigs(driverPath: String? = null) {
        val userDir = DirectoryInitialization.userDirectory ?: return
        val customDir = File(userDir, "config/custom")
        if (!customDir.exists() || !customDir.isDirectory) return

        val iniFiles = customDir.listFiles { file -> file.isFile && file.extension.equals("ini", ignoreCase = true) } ?: return
        for (iniFile in iniFiles) {
            try {
                val lines = iniFile.readLines()
                val newLines = mutableListOf<String>()
                var inGpuDriverSection = false
                var hasGpuDriverSection = false

                for (line in lines) {
                    val trimmed = line.trim()
                    if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                        if (inGpuDriverSection) {
                            newLines.add("driver_path\\use_global=true")
                            inGpuDriverSection = false
                        }
                        val isGpuDriver = trimmed.equals("[GpuDriver]", ignoreCase = true)
                        if (isGpuDriver) {
                            hasGpuDriverSection = true
                            inGpuDriverSection = true
                            newLines.add("[GpuDriver]")
                            continue
                        }
                    }

                    // Strip any old driver_path settings (overrides, use_global flags, defaults)
                    if (trimmed.startsWith("driver_path", ignoreCase = true)) {
                        continue
                    }

                    newLines.add(line)
                }

                if (inGpuDriverSection) {
                    newLines.add("driver_path\\use_global=true")
                } else if (!hasGpuDriverSection) {
                    newLines.add("")
                    newLines.add("[GpuDriver]")
                    newLines.add("driver_path\\use_global=true")
                }

                iniFile.writeText(newLines.joinToString("\n"))
            } catch (_: Exception) {
            }
        }
    }

    fun savePerGameDriver(game: Game, driverPath: String) {
        try {
            val iniFile = SettingsFile.getCustomSettingsFile(game)
            if (!iniFile.exists()) {
                iniFile.parentFile?.mkdirs()
                iniFile.createNewFile()
            }
            val lines = if (iniFile.length() > 0) iniFile.readLines() else emptyList()
            val newLines = mutableListOf<String>()
            var inGpuDriverSection = false
            var hasGpuDriverSection = false

            for (line in lines) {
                val trimmed = line.trim()
                if (trimmed.startsWith("# STORM_AUTO_GAME_FIX_TEMPORARY") ||
                    trimmed.contains("Auto-generated by STORM SWITCH GameFix")) {
                    continue
                }

                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    if (inGpuDriverSection) {
                        newLines.add("driver_path = $driverPath")
                        newLines.add("driver_path\\use_global = false")
                        newLines.add("driver_path\\default = false")
                        inGpuDriverSection = false
                    }
                    val isGpuDriver = trimmed.equals("[GpuDriver]", ignoreCase = true)
                    if (isGpuDriver) {
                        hasGpuDriverSection = true
                        inGpuDriverSection = true
                        newLines.add("[GpuDriver]")
                        continue
                    }
                }

                if (inGpuDriverSection && trimmed.startsWith("driver_path", ignoreCase = true)) {
                    continue
                }

                newLines.add(line)
            }

            if (inGpuDriverSection) {
                newLines.add("driver_path = $driverPath")
                newLines.add("driver_path\\use_global = false")
                newLines.add("driver_path\\default = false")
            } else if (!hasGpuDriverSection) {
                if (newLines.isNotEmpty() && newLines.last().isNotBlank()) {
                    newLines.add("")
                }
                newLines.add("[GpuDriver]")
                newLines.add("driver_path = $driverPath")
                newLines.add("driver_path\\use_global = false")
                newLines.add("driver_path\\default = false")
            }

            iniFile.writeText(newLines.joinToString("\n"))
            Log.info("[GpuDriverHelper] Explicitly saved per-game driver for ${game.title}: $driverPath")
        } catch (e: Exception) {
            Log.error("[GpuDriverHelper] Failed to save per-game driver for ${game.title}: ${e.message}")
        }
    }

    fun resetPerGameDriverToGlobal(game: Game) {
        try {
            val iniFile = SettingsFile.getCustomSettingsFile(game)
            if (!iniFile.exists() || iniFile.length() == 0L) return
            val lines = iniFile.readLines()
            val newLines = mutableListOf<String>()
            var inGpuDriverSection = false
            var hasGpuDriverSection = false

            for (line in lines) {
                val trimmed = line.trim()
                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    if (inGpuDriverSection) {
                        newLines.add("driver_path\\use_global = true")
                        inGpuDriverSection = false
                    }
                    val isGpuDriver = trimmed.equals("[GpuDriver]", ignoreCase = true)
                    if (isGpuDriver) {
                        hasGpuDriverSection = true
                        inGpuDriverSection = true
                        newLines.add("[GpuDriver]")
                        continue
                    }
                }

                if (inGpuDriverSection && trimmed.startsWith("driver_path", ignoreCase = true)) {
                    continue
                }

                newLines.add(line)
            }

            if (inGpuDriverSection) {
                newLines.add("driver_path\\use_global = true")
            } else if (!hasGpuDriverSection) {
                if (newLines.isNotEmpty() && newLines.last().isNotBlank()) {
                    newLines.add("")
                }
                newLines.add("[GpuDriver]")
                newLines.add("driver_path\\use_global = true")
            }

            iniFile.writeText(newLines.joinToString("\n"))
            Log.info("[GpuDriverHelper] Explicitly reset per-game driver to global for ${game.title}")
        } catch (e: Exception) {
            Log.error("[GpuDriverHelper] Failed to reset per-game driver for ${game.title}: ${e.message}")
        }
    }

    fun getPerGameDriver(game: Game): Pair<Boolean, String?> {
        try {
            val iniFile = SettingsFile.getCustomSettingsFile(game)
            if (!iniFile.exists() || iniFile.length() == 0L) {
                return Pair(true, null)
            }
            var inGpuDriver = false
            var useGlobal = true
            var pathInIni: String? = null
            for (line in iniFile.readLines()) {
                val trimmed = line.trim()
                if (trimmed.isEmpty() || trimmed.startsWith("#") || trimmed.startsWith(";")) {
                    continue
                }
                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    inGpuDriver = trimmed.equals("[GpuDriver]", ignoreCase = true)
                    continue
                }
                if (inGpuDriver && trimmed.contains("=")) {
                    val key = trimmed.substringBefore("=").trim()
                    val value = trimmed.substringAfter("=").trim().removeSurrounding("\"", "\"")
                    if (key.equals("driver_path\\use_global", ignoreCase = true)) {
                        useGlobal = value.equals("true", ignoreCase = true)
                    } else if (key.equals("driver_path", ignoreCase = true)) {
                        pathInIni = value
                    }
                }
            }
            return Pair(useGlobal, pathInIni)
        } catch (e: Exception) {
            Log.error("[GpuDriverHelper] Failed to read per-game driver for ${game.title}: ${e.message}")
            return Pair(true, null)
        }
    }
}
