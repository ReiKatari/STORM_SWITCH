// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

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
     * Detects Snapdragon 8 Elite Gen 5+ (SM8850 / SM8835 — Adreno 840 / 850+).
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

    fun isPanVkDriverActive(): Boolean {
        val metadata = installedCustomDriverData
        val libName = metadata.libraryName ?: ""
        val vendor = metadata.vendor ?: ""
        val name = metadata.name ?: ""
        val description = metadata.description ?: ""
        val activeDriverZipPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Exception) { "" }
        return name.contains("PanVK", ignoreCase = true) ||
               name.contains("Panfrost", ignoreCase = true) ||
               vendor.contains("Panfrost", ignoreCase = true) ||
               vendor.contains("PanVK", ignoreCase = true) ||
               libName.contains("panvk", ignoreCase = true) ||
               libName.contains("panfrost", ignoreCase = true) ||
               libName.contains("libvulkan_panfrost", ignoreCase = true) ||
               description.contains("PanVK", ignoreCase = true) ||
               activeDriverZipPath.contains("panvk", ignoreCase = true) ||
               activeDriverZipPath.contains("panfrost", ignoreCase = true) ||
               activeDriverZipPath.contains("libvulkan_panfrost", ignoreCase = true)
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

    fun initializeDriverParameters() {
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

        val isPanVkOrStorm = isPanVkDriverActive() || (isMaliGpu() && isStormDriver8OrNewer())
        if (!isAdrenoGpu() && !isPanVkOrStorm) {
            // On non-Adreno GPUs without a PanVK custom driver, custom drivers cannot be loaded.
            // Clean up any stray Turnip driver files to prevent startup crashes.
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
            return
        }

        NativeFreedrenoConfig.reloadFreedrenoConfig()

        // Auto-restore selected driver from config if installation folder is missing files
        val activeDriverZipPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Exception) { "" }
        if (installedCustomDriverData.libraryName.isNullOrEmpty() && activeDriverZipPath.isNotEmpty()) {
            val activeDriverFile = File(activeDriverZipPath)
            if (activeDriverFile.exists()) {
                val metadata = getMetadataFromZip(activeDriverFile)
                if (metadata.name != null && metadata.minApi <= Build.VERSION.SDK_INT &&
                    (!isAdreno8xx() || isDriverCompatibleWithAdreno8xx(metadata))) {
                    try {
                        FileUtil.unzipToInternalStorage(
                            activeDriverFile.path,
                            File(driverInstallationPath!!)
                        )
                    } catch (_: Exception) {
                    }
                }
            }
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
        if (!customLib.isNullOrEmpty() && (isTurnipDriverActive() || isPanVkDriverActive())) {
            val installDir = File(driverInstallationPath!!)
            val isStorm8 = isStormDriver8OrNewer()
            val panVkActive = isPanVkDriverActive()
            val turnipActive = isTurnipDriverActive()

            if (isStorm8 || panVkActive) {
                val drircFile = File(installDir, "drirc.xml")
                val drircConfFile = File(installDir, "00-storm.conf")
                val fallbackConf = if (drircConfFile.exists()) drircConfFile else if (drircFile.exists()) drircFile else null

                if (fallbackConf != null && fallbackConf.exists()) {
                    val configBytes = fallbackConf.readBytes()
                    try {
                        val redirDir = File(fileRedirectionPath!!)
                        if (!redirDir.exists()) redirDir.mkdirs()
                        File(redirDir, "drirc.xml").writeBytes(configBytes)
                        File(redirDir, "drirc").writeBytes(configBytes)
                        File(redirDir, "00-storm.conf").writeBytes(configBytes)
                        File(redirDir, "drirc.conf").writeBytes(configBytes)

                        File(installDir, "00-storm.conf").writeBytes(configBytes)
                        File(installDir, "drirc.conf").writeBytes(configBytes)
                        File(installDir, "drirc").writeBytes(configBytes)

                        // Also write to $HOME/.drirc for Mesa driconf loader
                        val homeDir = YuzuApplication.appContext.filesDir
                        File(homeDir, ".drirc").writeBytes(configBytes)
                    } catch (_: Throwable) {}

                    NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", driverInstallationPath!!)
                    NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", driverInstallationPath!!)
                    NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_FILE", fallbackConf.absolutePath)
                    NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)
                }
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

            // 4GB Monolithic Shader Cache for Mesa Turnip and PanVK
            NativeFreedrenoConfig.setFreedrenoEnv("MESA_SHADER_CACHE_MAX_SIZE", "4294967296")
            NativeFreedrenoConfig.setFreedrenoEnv("MESA_DISK_CACHE_SINGLE_FILE", "1")

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
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_MESA_DEBUG")
            NativeFreedrenoConfig.clearFreedrenoEnv("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_NO_ERROR")
        }

        NativeLibrary.initializeGpuDriver(
            hookLibPath,
            driverInstallationPath,
            customLib,
            fileRedirectionPath
        )
    }

    // Per-TID Turnip driver hints (verified real TU_DEBUG flags only: nolrz, sysmem, gmem, nobin, noubwc, flushall, syncdraw)
    val PER_TID_TURNIP_DEBUG_HINTS: Map<String, List<String>> = emptyMap()

    fun applyPerGameDriverConfig(programIdHex: String, gameTitle: String) {
        val customLib = installedCustomDriverData.libraryName
        val turnipActive = isTurnipDriverActive()
        val panVkActive = isPanVkDriverActive()
        if (customLib.isNullOrEmpty() || driverInstallationPath.isNullOrEmpty() || (!turnipActive && !panVkActive)) {
            NativeFreedrenoConfig.clearFreedrenoEnv("DRIRC_CONFIGDIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_DIR")
            NativeFreedrenoConfig.clearFreedrenoEnv("MESA_DRIRC_FILE")
            return
        }

        try {
            val installDir = File(driverInstallationPath!!)
            val isStorm8 = isStormDriver8OrNewer()

            if (isStorm8 || panVkActive) {
                val profileType = PerGameDrircGenerator.detectProfileType(programIdHex, gameTitle)
                val generatedConfigFile = PerGameDrircGenerator.generateForGame(programIdHex, gameTitle)
                val configBytes = generatedConfigFile.readBytes()
                val stormConfFile = File(installDir, "00-storm.conf")
                val isDiablo = profileType == PerGameDrircGenerator.GameProfileType.DIABLO ||
                    programIdHex.equals("0100726014352000", ignoreCase = true) ||
                    gameTitle.contains("diablo", ignoreCase = true)
                val isMarioRabbids = profileType == PerGameDrircGenerator.GameProfileType.MARIO_RABBIDS ||
                    programIdHex.equals("010067300059A000", ignoreCase = true) ||
                    programIdHex.equals("0100760012E4A000", ignoreCase = true) ||
                    programIdHex.equals("010032C00AC58000", ignoreCase = true) ||
                    programIdHex.equals("01005CA00F966000", ignoreCase = true) ||
                    gameTitle.contains("rabbids", ignoreCase = true)

                val selectedDriverPath = try { StringSetting.DRIVER_PATH.getString() } catch (_: Exception) { "" }

                if (isStorm8 && stormConfFile.exists()) {
                    val currentConf = try { stormConfFile.readText() } catch (_: Exception) { "" }
                    if (!currentConf.contains("<application name=\"Tier ")) {
                        if (selectedDriverPath.isNotEmpty()) {
                            val activeZip = File(selectedDriverPath)
                            if (activeZip.exists()) {
                                try {
                                    java.util.zip.ZipFile(activeZip).use { zf ->
                                        val entry = zf.getEntry("00-storm.conf") ?: zf.entries().asSequence().firstOrNull { it.name.endsWith("00-storm.conf") }
                                        if (entry != null) {
                                            zf.getInputStream(entry).use { input ->
                                                stormConfFile.writeBytes(input.readBytes())
                                            }
                                            Log.info("[GpuDriverHelper] Self-healed 00-storm.conf from ${activeZip.name}")
                                        }
                                    }
                                } catch (e: Exception) {
                                    Log.error("[GpuDriverHelper] Failed to self-heal 00-storm.conf: ${e.message}")
                                }
                            }
                        }
                    }
                }

                val perGameDir = File(installDir, "per_game_drirc")
                if (!perGameDir.exists()) perGameDir.mkdirs()
                val perGameDrircFile = File(perGameDir, "drirc.xml")
                perGameDrircFile.writeBytes(configBytes)

                val homeDir = YuzuApplication.appContext.filesDir
                File(homeDir, ".drirc").writeBytes(configBytes)

                NativeFreedrenoConfig.setFreedrenoEnv("DRIRC_CONFIGDIR", perGameDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_DIR", perGameDir.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("MESA_DRIRC_FILE", perGameDrircFile.absolutePath)
                NativeFreedrenoConfig.setFreedrenoEnv("HOME", YuzuApplication.appContext.filesDir.absolutePath)

                if (isDiablo || isMarioRabbids) {
                    NativeFreedrenoConfig.clearFreedrenoEnv("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")
                } else {
                    if (NativeFreedrenoConfig.isFreedrenoEnvSet("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")) {
                        NativeFreedrenoConfig.clearFreedrenoEnv("MESA_VK_DYNAMIC_ATTACHMENT_FEEDBACK_LOOP_ENABLE")
                    }
                }
                Log.info("[GpuDriverHelper] Successfully applied per-game driver config for $gameTitle ($programIdHex) -> ${generatedConfigFile.absolutePath}")
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
                return
            }

            val normalizedTid = programIdHex.uppercase()
            // On third-party Turnip drivers for BOTW (01007EF00011E000), let the driver's native 00-mesa-defaults.conf / user sysmem config run cleanly without forced nolrz
            val perTidHints = if (!isStorm8 && normalizedTid == "01007EF00011E000") {
                null
            } else {
                PER_TID_TURNIP_DEBUG_HINTS[normalizedTid]
            }

            val existingTuDebug = if (NativeFreedrenoConfig.isFreedrenoEnvSet("TU_DEBUG")) NativeFreedrenoConfig.getFreedrenoEnv("TU_DEBUG") else ""
            val flags = existingTuDebug.split(",").map { it.trim() }.filter { it.isNotEmpty() && it != "noconcurrentunresolves" }.toMutableSet()
            if (isAdreno840OrNewer()) {
                flags.add("noubwc")
            }

            if (perTidHints != null && perTidHints.isNotEmpty()) {
                flags.addAll(perTidHints)
                NativeFreedrenoConfig.setFreedrenoEnv("TU_DEBUG", flags.joinToString(","))
                Log.info("[GpuDriverHelper] Applied verified per-TID Turnip driver hints for $normalizedTid ($gameTitle): TU_DEBUG=${flags.joinToString(",")}")
            } else {
                // Do NOT overwrite or strip sysmem / gmem from TU_DEBUG!
                // Only clean transient per-game flags: nobin, nolrz, noubwc (if not A840+), flushall, syncdraw
                val specialFlags = mutableSetOf("nobin", "nolrz", "flushall", "syncdraw")
                if (!isAdreno840OrNewer()) {
                    specialFlags.add("noubwc")
                }
                val cleanedFlags = flags.filter { !specialFlags.contains(it) }
                if (cleanedFlags.isEmpty()) {
                    NativeFreedrenoConfig.clearFreedrenoEnv("TU_DEBUG")
                } else {
                    NativeFreedrenoConfig.setFreedrenoEnv("TU_DEBUG", cleanedFlags.joinToString(","))
                }
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

    fun installDefaultDriver() {
        // Removing the installed driver will result in the backend using the default system driver.
        File(driverInstallationPath!!).deleteRecursively()
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
    fun installCustomDriver(driverUri: Uri): Boolean {
        // Revert to system default in the event the specified driver is bad.
        installDefaultDriver()

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

        // Unzip the driver.
        try {
            FileUtil.unzipToInternalStorage(
                copiedFile.path,
                File(driverInstallationPath!!)
            )
        } catch (e: SecurityException) {
            return false
        }

        // Initialize the driver parameters.
        initializeDriverParameters()

        return true
    }

    /**
     * Unzips driver into installation directory
     */
    fun installCustomDriver(driver: File): Boolean {
        // Revert to system default in the event the specified driver is bad.
        installDefaultDriver()

        // Ensure we have directories.
        initializeDirectories()

        // Validate driver
        val metadata = getMetadataFromZip(driver)
        if (metadata.name == null) {
            driver.delete()
            return false
        }

        // Unzip the driver to the private installation directory
        try {
            FileUtil.unzipToInternalStorage(
                driver.path,
                File(driverInstallationPath!!)
            )
        } catch (e: SecurityException) {
            return false
        }

        // Initialize the driver parameters.
        initializeDriverParameters()

        return true
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
            supportsCustomDriverLoading()
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
