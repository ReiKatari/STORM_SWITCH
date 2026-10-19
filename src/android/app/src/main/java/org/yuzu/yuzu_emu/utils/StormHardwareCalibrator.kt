// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.ActivityManager
import android.content.Context
import android.content.res.Configuration
import android.os.Build
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import org.yuzu.yuzu_emu.features.settings.model.IntSetting
import org.yuzu.yuzu_emu.utils.NativeConfig

/**
 * Intelligent hardware detection and preset calibration engine.
 * Automatically scans SoC, GPU, RAM capacity, and form factor (tablet vs smartphone)
 * on first launch or reset, configuring tailored settings for performance and stability.
 */
object StormHardwareCalibrator {

    private const val PREF_HARDWARE_CALIBRATED = "storm_hardware_calibrated"
    const val PREF_LAST_CONFIGURED_VERSION = "storm_last_configured_version"

    enum class HardwareTier {
        FLAGSHIP_ELITE,
        FLAGSHIP,
        HIGH_MIDRANGE,
        MIDRANGE,
        BUDGET
    }

    enum class StormPreset {
        DEFAULT,
        FAST,
        NORMAL,
        ACCURATE
    }

    data class DeviceHardwareProfile(
        val tier: HardwareTier,
        val socName: String,
        val gpuName: String,
        val totalRamGb: Double,
        val availRamGb: Double,
        val isAdreno: Boolean,
        val isAdreno830: Boolean,
        val isAdreno7xx: Boolean,
        val isAdreno6xx: Boolean,
        val isDimensity9400: Boolean,
        val isDimensity9300Or9200: Boolean,
        val isExynosOrXclipse: Boolean,
        val isTensor: Boolean,
        val isMali: Boolean,
        val isTablet: Boolean,
        val isGamingDevice: Boolean,
        val cpuCores: Int
    )

    @Volatile
    private var cachedRawGpuModel: String? = null

    private fun queryRawGpuModelSafe(): String {
        cachedRawGpuModel?.let { if (it.isNotEmpty()) return it }
        return try {
            val hookPath = GpuDriverHelper.hookLibPath
            val model = if (!hookPath.isNullOrEmpty()) {
                GpuDriverHelper.getGpuModel(hookLibPath = hookPath)?.lowercase()?.trim() ?: ""
            } else {
                ""
            }
            if (model.isNotEmpty()) {
                cachedRawGpuModel = model
            }
            model
        } catch (_: Throwable) {
            ""
        }
    }

    fun detectHardware(context: Context): DeviceHardwareProfile {
        val am = context.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
        val memInfo = ActivityManager.MemoryInfo()
        am?.getMemoryInfo(memInfo)
        val totalRamGb = if (memInfo.totalMem > 0) memInfo.totalMem / (1024.0 * 1024.0 * 1024.0) else 8.0
        val availRamGb = if (memInfo.availMem > 0) memInfo.availMem / (1024.0 * 1024.0 * 1024.0) else 4.0

        val hw = (
            Build.HARDWARE + " " +
            Build.BOARD + " " +
            Build.MODEL + " " +
            Build.MANUFACTURER + " " +
            Build.DEVICE + " " +
            Build.PRODUCT
        ).lowercase()
        val socModel = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) Build.SOC_MODEL.lowercase() else ""
        val rawGpu = queryRawGpuModelSafe()
        val combined = "$hw $socModel $rawGpu"
        val isAdreno = GpuDriverHelper.isAdrenoGpu() || rawGpu.contains("adreno")

        // Form factor detection: tablet if large screen or smallest width >= 600dp
        val screenLayout = context.resources.configuration.screenLayout
        val isTablet = (screenLayout and Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_LARGE ||
            context.resources.configuration.smallestScreenWidthDp >= 600

        // Dedicated Android gaming handhelds & active-cooled gaming phones/tablets
        val isGamingDevice = hw.contains("odin") ||
            hw.contains("retroid") ||
            hw.contains("rp5") ||
            hw.contains("rpmini") ||
            hw.contains("ayaneo") ||
            hw.contains("pocket s") ||
            hw.contains("pocket dmg") ||
            hw.contains("pocket evo") ||
            hw.contains("razer edge") ||
            hw.contains("redmagic") ||
            hw.contains("nx729") || hw.contains("nx769") || hw.contains("nx789") ||
            hw.contains("rog phone") || hw.contains("ai220") || hw.contains("ai230") || hw.contains("ai240") || hw.contains("ai250") ||
            hw.contains("blackshark") ||
            hw.contains("legion") || hw.contains("tb320") || hw.contains("tb321")

        // Adreno 8xx flagship (Snapdragon 8 Elite SM8750 / Adreno 830, Snapdragon 8s Gen 4 SM8735 / Adreno 825)
        // Note: Snapdragon 7s Gen 3 (SM7635) has Adreno 810, which is midrange, not Elite!
        val isAdreno810 = isAdreno && (combined.contains("7635") || combined.contains("adreno (tm) 810") || combined.contains("adreno 810") || combined.contains("volcano"))
        val isAdreno830 = isAdreno && !isAdreno810 && (
            combined.contains("8750") ||
            combined.contains("8735") ||
            combined.contains("adreno (tm) 830") ||
            combined.contains("adreno 830") ||
            combined.contains("adreno (tm) 825") ||
            combined.contains("adreno 825") ||
            hw.contains("sun") ||
            hw.contains("pakala") ||
            hw.contains("s931") || hw.contains("s936") || hw.contains("s938")
        )

        // Adreno 7xx family (8 Gen 3 SM8650, G3x Gen 2 SG3250P, 8 Gen 2 SM8550, 8s Gen 3 SM8635,
        // 8+ Gen 1 SM8475, 8 Gen 1 SM8450, 7+ Gen 3 SM7675, 7+ Gen 2 SM7475, 7 Gen 3 SM7550,
        // 7s Gen 2 SM7435, 6 Gen 1/3 SM6450/SM6475, plus Adreno 810 on 7s Gen 3 SM7635)
        val isAdreno7xxFlagship = isAdreno && !isAdreno830 && (
            combined.contains("8650") || combined.contains("pineapple") ||
            combined.contains("sg3250") || combined.contains("g3x") || combined.contains("adreno a32") ||
            combined.contains("8550") || combined.contains("kalama") ||
            combined.contains("8635") || combined.contains("cliffs") ||
            combined.contains("8475") || combined.contains("8450") || combined.contains("taro") || combined.contains("cape") ||
            combined.contains("7675") || combined.contains("7475") || combined.contains("ukee") ||
            rawGpu.contains("adreno (tm) 750") || rawGpu.contains("adreno 750") ||
            rawGpu.contains("adreno (tm) 740") || rawGpu.contains("adreno 740") ||
            rawGpu.contains("adreno (tm) 735") || rawGpu.contains("adreno 735") ||
            rawGpu.contains("adreno (tm) 732") || rawGpu.contains("adreno 732") ||
            rawGpu.contains("adreno (tm) 730") || rawGpu.contains("adreno 730") ||
            rawGpu.contains("adreno (tm) 725") || rawGpu.contains("adreno 725") ||
            rawGpu.contains("fd7")
        )
        val isAdreno7xxMid = isAdreno && !isAdreno830 && !isAdreno7xxFlagship && (
            isAdreno810 ||
            combined.contains("7550") || combined.contains("crow") ||
            combined.contains("7435") || combined.contains("garnet") ||
            combined.contains("6450") || combined.contains("6475") ||
            rawGpu.contains("adreno (tm) 720") || rawGpu.contains("adreno 720") ||
            rawGpu.contains("adreno (tm) 710") || rawGpu.contains("adreno 710") ||
            rawGpu.contains("adreno (tm) 7") || rawGpu.contains("adreno 7")
        )
        val isAdreno7xx = isAdreno7xxFlagship || isAdreno7xxMid
        val isAdreno6xx = isAdreno && !isAdreno830 && !isAdreno7xx

        // MediaTek Dimensity & Helio detection
        val isDimensity9400 = combined.contains("mt6991") || combined.contains("dimensity 9400") || combined.contains("immortalis-g925") || socModel.contains("6991")
        val isDimensity9300Or9200 = !isDimensity9400 && (
            combined.contains("mt6989") || combined.contains("dimensity 9300") || combined.contains("immortalis-g720") ||
            combined.contains("mt6985") || combined.contains("dimensity 9200") || combined.contains("immortalis-g715") ||
            combined.contains("mt6983") || combined.contains("dimensity 9000") ||
            combined.contains("mt6899") || combined.contains("dimensity 8400") || combined.contains("mali-g720")
        )
        val isDimensity8xxx = !isDimensity9400 && !isDimensity9300Or9200 && (
            combined.contains("mt6897") || combined.contains("8300") || combined.contains("mali-g615") ||
            combined.contains("mt6896") || combined.contains("8200") || combined.contains("8250") ||
            combined.contains("mt6895") || combined.contains("8100") || combined.contains("8050") || combined.contains("mt6893")
        )

        // Samsung Exynos: strictly distinguish AMD RDNA Xclipse GPUs (2500/2400/2200/1580/1480) from Mali-based Exynos (2100/1380/1280/990)
        val isExynosOrXclipse = !isAdreno && (
            combined.contains("xclipse") ||
            combined.contains("9955") || combined.contains("exynos 2500") ||
            combined.contains("9945") || combined.contains("exynos 2400") ||
            combined.contains("9925") || combined.contains("exynos 2200") ||
            combined.contains("8855") || combined.contains("exynos 1580") ||
            combined.contains("8845") || combined.contains("exynos 1480")
        )
        val isExynosMali = !isAdreno && !isExynosOrXclipse && (
            combined.contains("exynos") || combined.contains("s5e")
        )
        val isExynos2100Or990 = isExynosMali && (combined.contains("2100") || combined.contains("9840") || combined.contains("990") || combined.contains("9830"))

        // Google Tensor G1 / G2 / G3 / G4
        val isTensor = !isAdreno && (
            combined.contains("tensor") ||
            combined.contains("zuma") ||
            combined.contains("gs201") ||
            combined.contains("gs101") ||
            hw.contains("tokay") || hw.contains("caiman") || hw.contains("komodo") ||
            hw.contains("shiba") || hw.contains("husky")
        )
        val isMali = !isAdreno && !isExynosOrXclipse

        // Distinguish Snapdragon 800-series (Adreno 660/650/640/630) from 700/600-series (Adreno 644/642L/620/619/618/610)
        val isSnapdragon8xxA6xx = isAdreno6xx && (
            combined.contains("8350") || combined.contains("lahaina") || combined.contains("adreno (tm) 660") || combined.contains("adreno 660") ||
            combined.contains("8250") || combined.contains("kona") || combined.contains("adreno (tm) 650") || combined.contains("adreno 650") ||
            combined.contains("8150") || combined.contains("msmnile") || combined.contains("adreno (tm) 640") || combined.contains("adreno 640") ||
            combined.contains("sdm845") || combined.contains("adreno (tm) 630") || combined.contains("adreno 630")
        )
        val isSnapdragon7xxHighA6xx = isAdreno6xx && !isSnapdragon8xxA6xx && (
            combined.contains("7450") || combined.contains("adreno (tm) 644") || combined.contains("adreno 644") ||
            combined.contains("7350") || combined.contains("cedros") || combined.contains("adreno (tm) 642") || combined.contains("adreno 642") ||
            combined.contains("7325") || combined.contains("yupik")
        )
        val isSnapdragon7xxA6xx = isAdreno6xx && !isSnapdragon8xxA6xx

        val tier = when {
            totalRamGb < 5.2 -> HardwareTier.BUDGET
            isAdreno830 || (totalRamGb >= 11.0 && (isAdreno7xxFlagship || isDimensity9400)) -> HardwareTier.FLAGSHIP_ELITE
            isAdreno7xxFlagship || isDimensity9400 || isDimensity9300Or9200 || (isExynosOrXclipse && (combined.contains("9955") || combined.contains("9945"))) -> HardwareTier.FLAGSHIP
            isSnapdragon8xxA6xx || isAdreno7xxMid || isDimensity8xxx || isExynosOrXclipse || isExynos2100Or990 || (isTensor && totalRamGb >= 7.5) || isSnapdragon7xxHighA6xx -> HardwareTier.HIGH_MIDRANGE
            totalRamGb >= 5.5 && (isTensor || isSnapdragon7xxA6xx || isMali) -> HardwareTier.MIDRANGE
            else -> HardwareTier.BUDGET
        }

        val socName = when {
            isAdreno830 && combined.contains("8735") -> "Qualcomm Snapdragon 8s Gen 4 (SM8735 / Adreno 825)"
            isAdreno830 -> "Qualcomm Snapdragon 8 Elite (SM8750 / Adreno 830)"
            combined.contains("sg3250") || combined.contains("g3x") -> "Qualcomm Snapdragon G3x Gen 2 (Adreno A32)"
            combined.contains("8650") || combined.contains("pineapple") -> "Qualcomm Snapdragon 8 Gen 3 (SM8650 / Adreno 750)"
            combined.contains("8635") -> "Qualcomm Snapdragon 8s Gen 3 (SM8635 / Adreno 735)"
            combined.contains("8550") || combined.contains("kalama") -> "Qualcomm Snapdragon 8 Gen 2 (SM8550 / Adreno 740)"
            combined.contains("8475") -> "Qualcomm Snapdragon 8+ Gen 1 (SM8475 / Adreno 730)"
            combined.contains("8450") || combined.contains("taro") -> "Qualcomm Snapdragon 8 Gen 1 (SM8450 / Adreno 730)"
            combined.contains("7675") -> "Qualcomm Snapdragon 7+ Gen 3 (SM7675 / Adreno 732)"
            combined.contains("7475") || combined.contains("ukee") -> "Qualcomm Snapdragon 7+ Gen 2 (SM7475 / Adreno 725)"
            combined.contains("7635") || isAdreno810 -> "Qualcomm Snapdragon 7s Gen 3 (SM7635 / Adreno 810)"
            combined.contains("7550") || combined.contains("crow") -> "Qualcomm Snapdragon 7 Gen 3 (SM7550 / Adreno 720)"
            combined.contains("7435") || combined.contains("garnet") -> "Qualcomm Snapdragon 7s Gen 2 (SM7435 / Adreno 710)"
            combined.contains("6450") || combined.contains("6475") -> "Qualcomm Snapdragon 6 Gen 1 / Gen 3 (Adreno 710)"
            combined.contains("8350") || combined.contains("lahaina") -> "Qualcomm Snapdragon 888 / 888+ (SM8350 / Adreno 660)"
            combined.contains("8250") || combined.contains("kona") -> "Qualcomm Snapdragon 865 / 870 (SM8250 / Adreno 650)"
            combined.contains("8150") || combined.contains("msmnile") -> "Qualcomm Snapdragon 855 / 860 (SM8150 / Adreno 640)"
            combined.contains("sdm845") -> "Qualcomm Snapdragon 845 (SDM845 / Adreno 630)"
            combined.contains("7450") -> "Qualcomm Snapdragon 7 Gen 1 (SM7450 / Adreno 644)"
            combined.contains("7325") || combined.contains("yupik") -> "Qualcomm Snapdragon 778G / 782G (SM7325 / Adreno 642L)"
            combined.contains("7350") || combined.contains("cedros") -> "Qualcomm Snapdragon 780G (SM7350 / Adreno 642)"
            combined.contains("7250") || combined.contains("lito") -> "Qualcomm Snapdragon 765G / 768G (SM7250 / Adreno 620)"
            combined.contains("7225") -> "Qualcomm Snapdragon 750G (SM7225 / Adreno 619)"
            combined.contains("6375") -> "Qualcomm Snapdragon 695 (SM6375 / Adreno 619)"
            combined.contains("7150") -> "Qualcomm Snapdragon 730G / 732G (SM7150 / Adreno 618)"
            combined.contains("7125") || combined.contains("atoll") -> "Qualcomm Snapdragon 720G (SM7125 / Adreno 618)"
            combined.contains("6225") -> "Qualcomm Snapdragon 680 / 685 (SM6225 / Adreno 610)"
            isDimensity9400 -> "MediaTek Dimensity 9400 (MT6991 / Immortalis-G925)"
            combined.contains("mt6989") || combined.contains("9300") -> "MediaTek Dimensity 9300 / 9300+ (MT6989 / Immortalis-G720)"
            combined.contains("mt6985") || combined.contains("9200") -> "MediaTek Dimensity 9200 / 9200+ (MT6985 / Immortalis-G715)"
            combined.contains("mt6983") || combined.contains("9000") -> "MediaTek Dimensity 9000 / 9000+ (MT6983 / Mali-G710)"
            combined.contains("mt6899") || combined.contains("8400") -> "MediaTek Dimensity 8400 (MT6899 / Mali-G720)"
            combined.contains("mt6897") || combined.contains("8300") -> "MediaTek Dimensity 8300 Ultra (MT6897 / Mali-G615)"
            isDimensity8xxx -> "MediaTek Dimensity 8100 / 8200 (Mali-G610)"
            combined.contains("mt6789") || combined.contains("g99") -> "MediaTek Helio G99 / G100 (MT6789 / Mali-G57)"
            combined.contains("9955") -> "Samsung Exynos 2500 (AMD Xclipse 950)"
            combined.contains("9945") -> "Samsung Exynos 2400 / 2400e (AMD Xclipse 940)"
            combined.contains("9925") -> "Samsung Exynos 2200 (AMD Xclipse 920)"
            combined.contains("8855") || combined.contains("8845") -> "Samsung Exynos 1480 / 1580 (AMD Xclipse 530/540)"
            isExynosOrXclipse -> "Samsung Exynos (AMD Xclipse RDNA GPU)"
            isExynosMali -> "Samsung Exynos (ARM Mali GPU)"
            isTensor -> "Google Tensor G1–G4 (ARM Mali GPU)"
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && Build.SOC_MODEL.isNotBlank() -> Build.SOC_MODEL
            else -> Build.HARDWARE
        }

        val gpuName = when {
            isAdreno830 && combined.contains("8735") -> "Qualcomm Adreno 825 (Snapdragon 8s Gen 4)"
            isAdreno830 -> "Qualcomm Adreno 830 (Snapdragon 8 Elite / Turnip Vulkan 1.3 / 1.4)"
            combined.contains("8650") -> "Qualcomm Adreno 750 (Snapdragon 8 Gen 3 / Turnip)"
            combined.contains("8635") -> "Qualcomm Adreno 735 (Snapdragon 8s Gen 3 / Turnip)"
            combined.contains("8550") -> "Qualcomm Adreno 740 (Snapdragon 8 Gen 2 / Turnip)"
            combined.contains("8450") || combined.contains("8475") -> "Qualcomm Adreno 730 (Snapdragon 8+ Gen 1 / Turnip)"
            combined.contains("7675") -> "Qualcomm Adreno 732 (Snapdragon 7+ Gen 3 / Turnip)"
            combined.contains("7475") -> "Qualcomm Adreno 725 (Snapdragon 7+ Gen 2 / Turnip)"
            isAdreno810 -> "Qualcomm Adreno 810 (Snapdragon 7s Gen 3)"
            isAdreno7xxMid -> "Qualcomm Adreno 710 / 720 (Snapdragon 7 Series)"
            combined.contains("8350") -> "Qualcomm Adreno 660 (Snapdragon 888 / Turnip)"
            combined.contains("8250") -> "Qualcomm Adreno 650 (Snapdragon 865/870 / Turnip)"
            combined.contains("8150") -> "Qualcomm Adreno 640 (Snapdragon 855/860 / Turnip)"
            isDimensity9400 -> "ARM Immortalis-G925 (Dimensity 9400 Vulkan 1.3)"
            isDimensity9300Or9200 -> "ARM Immortalis-G720 / G715 (Dimensity 9300/9200/8400)"
            isDimensity8xxx -> "ARM Mali-G615 / G610 (Dimensity 8300/8200/8100)"
            combined.contains("9955") -> "Samsung Xclipse 950 (AMD RDNA3.5 Vulkan)"
            combined.contains("9945") -> "Samsung Xclipse 940 (AMD RDNA3 Vulkan)"
            combined.contains("9925") -> "Samsung Xclipse 920 (AMD RDNA2 Vulkan)"
            isExynosOrXclipse -> "Samsung Xclipse (AMD RDNA Mobile Vulkan)"
            isTensor -> "ARM Mali-G715 / G710 (Google Tensor)"
            isAdreno -> "Qualcomm Adreno (Turnip Vulkan 1.3 / 1.4)"
            else -> "ARM Mali / Mobile Vulkan GPU"
        }

        val cpuCores = Runtime.getRuntime().availableProcessors()

        return DeviceHardwareProfile(
            tier = tier,
            socName = socName,
            gpuName = gpuName,
            totalRamGb = totalRamGb,
            availRamGb = availRamGb,
            isAdreno = isAdreno,
            isAdreno830 = isAdreno830,
            isAdreno7xx = isAdreno7xx,
            isAdreno6xx = isAdreno6xx,
            isDimensity9400 = isDimensity9400,
            isDimensity9300Or9200 = isDimensity9300Or9200,
            isExynosOrXclipse = isExynosOrXclipse,
            isTensor = isTensor,
            isMali = isMali,
            isTablet = isTablet,
            isGamingDevice = isGamingDevice,
            cpuCores = cpuCores
        )
    }

    /**
     * Calibrate on first startup, version update, or when explicitly requested.
     * On version update, preserves user settings while running sanitizeSettingsForHardware
     * to automatically heal any unsafe/incompatible settings for the device's SoC, GPU, driver, and RAM.
     */
    fun autoCalibrate(context: Context, force: Boolean = false) {
        val prefs = PreferenceManager.getDefaultSharedPreferences(context)
        val currentVersion = org.yuzu.yuzu_emu.BuildConfig.VERSION_NAME

        if (!force && prefs.getBoolean(PREF_HARDWARE_CALIBRATED, false)) {
            if (prefs.getString(PREF_LAST_CONFIGURED_VERSION, null) != currentVersion) {
                sanitizeSettingsForHardware(context)
                prefs.edit().putString(PREF_LAST_CONFIGURED_VERSION, currentVersion).apply()
            } else {
                sanitizeSettingsForHardware(context)
            }
            return
        }

        // Apply device-tailored defaults on first launch / clean install or explicit reset
        applyPreset(StormPreset.DEFAULT, context)
        prefs.edit()
            .putBoolean(PREF_HARDWARE_CALIBRATED, true)
            .putString(PREF_LAST_CONFIGURED_VERSION, currentVersion)
            .putBoolean(org.yuzu.yuzu_emu.features.settings.model.Settings.PREF_APP_FULLSCREEN, true)
            .apply()
    }

    /**
     * Automatically inspects active global/per-game settings and corrects hardware-incompatible
     * combinations (e.g., 8GB DRAM on <7GB RAM devices, ExtendedDynamicState on Adreno 6xx / Mali /
     * Xclipse / stock Qualcomm system driver, uncompressed ASTC on <=4GB RAM devices, excessive
     * parallel shader workers on low-RAM or Mali devices).
     */
    fun sanitizeSettingsForHardware(context: Context = org.yuzu.yuzu_emu.YuzuApplication.appContext) {
        try {
            val profile = detectHardware(context)
            val isTurnip = GpuDriverHelper.isTurnipDriverActive()
            var changed = false

            // 1. RAM / Memory Layout Auto-Protection against Android LMKD (Low Memory Killer)
            val currentMemLayout = IntSetting.MEMORY_LAYOUT.getInt()
            if (profile.totalRamGb < 5.2 && currentMemLayout > 0) {
                IntSetting.MEMORY_LAYOUT.setInt(0) // Force 4GB DRAM on <=4GB RAM phones
                changed = true
            } else if (profile.totalRamGb < 7.2 && currentMemLayout > 1) {
                IntSetting.MEMORY_LAYOUT.setInt(1) // Clamp 8GB -> 6GB DRAM on 6GB RAM phones
                changed = true
            }

            // 2. VRAM Usage Mode & ASTC Recompression on Low-RAM / Budget Devices
            if (profile.totalRamGb < 6.5 || profile.tier == HardwareTier.BUDGET) {
                if (IntSetting.RENDERER_VRAM_USAGE_MODE.getInt() > 0) {
                    IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(0) // Conservative
                    changed = true
                }
            }
            if (profile.totalRamGb < 5.5 && IntSetting.ASTC_RECOMPRESSION.getInt() == 0) {
                IntSetting.ASTC_RECOMPRESSION.setInt(2) // BC3 recompression to prevent OOM on 4GB RAM phones
                changed = true
            }

            // 3. Extended Dynamic State (EDS) & Vertex Input Dynamic State Hardware/Driver Guard
            val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip
            val currentDyna = IntSetting.RENDERER_DYNA_STATE.getInt()
            if (!canUseEds1 && currentDyna != 0) {
                IntSetting.RENDERER_DYNA_STATE.setInt(0)
                changed = true
            } else if (canUseEds1 && currentDyna > 1) {
                IntSetting.RENDERER_DYNA_STATE.setInt(1)
                changed = true
            }

            if (!canUseEds1 && BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.getBoolean()) {
                BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(false)
                changed = true
            }

            // 4. Parallel Pipeline Workers Clamp for RAM & Big.LITTLE Core Topology
            val maxSafeWorkers = when {
                profile.totalRamGb < 6.0 || profile.tier == HardwareTier.BUDGET || profile.isAdreno6xx -> 2
                profile.isMali || profile.isExynosOrXclipse || profile.isTensor -> 2
                profile.isAdreno830 && profile.totalRamGb >= 11.0 -> 4
                else -> (profile.cpuCores - 2).coerceIn(2, 4)
            }
            if (IntSetting.ANDROID_PIPELINE_WORKERS.getInt() > maxSafeWorkers) {
                IntSetting.ANDROID_PIPELINE_WORKERS.setInt(maxSafeWorkers)
                changed = true
            }

            if (changed) {
                Log.info("[StormHardwareCalibrator] Auto-sanitized settings for ${profile.socName} (${String.format("%.1f", profile.totalRamGb)} GB RAM, Turnip=$isTurnip)")
            }
        } catch (e: Exception) {
            Log.warning("[StormHardwareCalibrator] Failed to sanitize settings: ${e.message}")
        }
    }

    /**
     * Adapts a GameFix settings map to the current device's exact SoC, GPU, driver, and RAM tier
     * before writing to the per-game INI file.
     */
    fun sanitizeFixMapForHardware(
        settings: MutableMap<String, String>,
        context: Context = org.yuzu.yuzu_emu.YuzuApplication.appContext
    ) {
        try {
            val profile = detectHardware(context)
            val isTurnip = GpuDriverHelper.isTurnipDriverActive()
            val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip

            // 1. Dynamic State protection per GPU / Driver
            val dynaVal = settings["Renderer\\dyna_state"]
            if (!canUseEds1) {
                settings["Renderer\\dyna_state"] = "0"
                settings["Renderer\\vertex_input_dynamic_state"] = "false"
            } else if (dynaVal == "2" || dynaVal == "3") {
                settings["Renderer\\dyna_state"] = "1"
            }

            // 2. Memory Layout protection per physical RAM capacity
            val memVal = (settings["Core\\memory_layout_mode"] ?: settings["System\\memory_layout_mode"])?.toIntOrNull() ?: 0
            if (profile.totalRamGb < 5.2 && memVal > 0) {
                settings["Core\\memory_layout_mode"] = "0"
                settings["System\\memory_layout_mode"] = "0"
            } else if (profile.totalRamGb < 7.2 && memVal > 1) {
                settings["Core\\memory_layout_mode"] = "1"
                settings["System\\memory_layout_mode"] = "1"
            }

            // 3. Low-RAM & Adreno 6xx VRAM / ASTC OOM protection
            if (profile.totalRamGb < 6.5 || profile.isAdreno6xx || !profile.isAdreno) {
                settings["Renderer\\vram_usage_mode"] = "0"
            }
            if (profile.totalRamGb < 5.5 && settings["Renderer\\astc_recompression"] == "0") {
                settings["Renderer\\astc_recompression"] = "2" // BC3 prevents OOM on 4GB RAM devices
            }
        } catch (_: Exception) {
        }
    }

    /**
     * Apply a specific preset dynamically tailored for this exact device hardware.
     */
    fun applyPreset(preset: StormPreset, context: Context) {
        val profile = detectHardware(context)

        when (preset) {
            StormPreset.DEFAULT -> applyDeviceDefaultPreset(profile)
            StormPreset.FAST -> applyDeviceFastPreset(profile)
            StormPreset.NORMAL -> applyDeviceNormalPreset(profile)
            StormPreset.ACCURATE -> applyDeviceAccuratePreset(profile)
        }

        sanitizeSettingsForHardware(context)

        try {
            NativeConfig.saveGlobalConfig()
        } catch (_: Exception) {
        }
    }

    /**
     * Individual tailored "По умолчанию (Сброс)" defaults for this specific device.
     */
    private fun applyDeviceDefaultPreset(profile: DeviceHardwareProfile) {
        val isTurnip = GpuDriverHelper.isTurnipDriverActive()
        val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip

        // 1. Renderer / Video Defaults
        IntSetting.RENDERER_BACKEND.setInt(1) // Vulkan
        IntSetting.RENDERER_ACCURACY.setInt(0) // Normal

        // Resolution: 1.0X (3) for Flagships & High-Midrange with >=8GB RAM, 0.75X (2) for Midrange/Adreno 6xx, 0.5X (1) for <5GB Budget
        val defaultRes = when {
            profile.totalRamGb < 5.0 -> 1 // 0.5X for 4GB RAM budget phones
            profile.tier >= HardwareTier.FLAGSHIP -> 3 // 1.0X
            profile.tier == HardwareTier.HIGH_MIDRANGE && profile.totalRamGb >= 7.5 && !profile.isAdreno6xx -> 3 // 1.0X
            else -> 2 // 0.75X
        }
        IntSetting.RENDERER_RESOLUTION.setInt(defaultRes)

        // VSync: Mailbox (1) for flagship Adreno 830/7xx, FIFO (2) for Mali / Dimensity / Exynos / budget
        IntSetting.RENDERER_VSYNC.setInt(if (profile.isAdreno && profile.tier >= HardwareTier.FLAGSHIP) 1 else 2)
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0) // None

        // Scaling filter: AMD FSR (6) with 80% Sharpening
        IntSetting.RENDERER_SCALING_FILTER.setInt(6)
        IntSetting.FSR_SHARPENING_SLIDER.setInt(80)

        // ASTC Decode: GPU (1) by default; BC3 (2) on <5.5GB RAM devices, Uncompressed (0) otherwise
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1)
        IntSetting.ASTC_RECOMPRESSION.setInt(if (profile.totalRamGb < 5.5) 2 else 0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(2) // GPU
        IntSetting.DMA_ACCURACY.setInt(1) // Normal

        // VRAM Usage Mode: Normal (1) for Flagships/High-midrange on Adreno with >=8GB RAM, Conservative (0) otherwise
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.isAdreno && profile.tier >= HardwareTier.HIGH_MIDRANGE && profile.totalRamGb >= 8.0 && !profile.isAdreno6xx) 1 else 0)

        // GPU Fence Behavior: Balanced (1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(1)
        IntSetting.MAX_ANISOTROPY.setInt(0) // Automatic

        // Dynamic State: EDS1 (1) only on Adreno 7xx/8xx with Turnip, Disabled (0) on Adreno 6xx / Qualcomm system / Mali / Xclipse
        IntSetting.RENDERER_DYNA_STATE.setInt(if (canUseEds1) 1 else 0)

        // 2. CPU / System Defaults
        // Form factor adaptation: Tablets and active-cooled gaming handhelds with >=8GB RAM can use Docked mode
        BooleanSetting.USE_DOCKED_MODE.setBoolean((profile.isTablet || (profile.isGamingDevice && profile.tier >= HardwareTier.FLAGSHIP)) && profile.totalRamGb >= 7.5)
        IntSetting.CPU_BACKEND.setInt(1) // NCE
        IntSetting.CPU_ACCURACY.setInt(0) // Auto

        // Memory Layout: 4GB (0)
        IntSetting.MEMORY_LAYOUT.setInt(0)

        // Pipeline workers: 2 for Dimensity / Mali / Xclipse / Adreno 6xx / <6GB RAM, 4 for 8 Elite, 3-4 for Flagship Adreno 7xx
        val optimalWorkers = when {
            profile.totalRamGb < 6.0 || profile.isMali || profile.isExynosOrXclipse || profile.isTensor || profile.isAdreno6xx -> 2
            profile.isAdreno830 && profile.totalRamGb >= 11.0 -> 4
            else -> (profile.cpuCores - 2).coerceIn(2, 4)
        }
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(optimalWorkers)

        // 3. Audio Defaults
        IntSetting.AUDIO_OUTPUT_ENGINE.setInt(0) // Auto
        BooleanSetting.AUDIO_MUTED.setBoolean(false)
        org.yuzu.yuzu_emu.features.settings.model.ByteSetting.AUDIO_VOLUME.setByte(100.toByte())

        // 4. Renderer Booleans
        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
        BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
        BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
        org.yuzu.yuzu_emu.features.settings.model.ShortSetting.RENDERER_SPEED_LIMIT.setShort(100.toShort())
        BooleanSetting.RENDERER_USE_SPEED_LIMIT.setBoolean(true)
        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
        BooleanSetting.FASTMEM.setBoolean(true)
        BooleanSetting.FASTMEM_EXCLUSIVES.setBoolean(true)
        BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
        BooleanSetting.SYNC_MEMORY_OPERATIONS.setBoolean(false)
        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.setBoolean(true)
        BooleanSetting.SKIP_CPU_INNER_INVALIDATION.setBoolean(false)
        BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
        BooleanSetting.ENABLE_BUFFER_HISTORY.setBoolean(false)
        BooleanSetting.ENABLE_GPU_BUFFER_READBACK.setBoolean(false)
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(canUseEds1)

        // 5. Frame Gen Defaults
        BooleanSetting.RENDERER_FRAME_GEN.setBoolean(false)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        // 6. Thermal & Pacing Flags
        BooleanSetting.ECO_THERMAL_MODE.setBoolean(false)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(false)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(false)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(false)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(profile.totalRamGb < 6.5)
    }

    /**
     * Dynamically tailored "Быстрый" preset.
     */
    private fun applyDeviceFastPreset(profile: DeviceHardwareProfile) {
        val isTurnip = GpuDriverHelper.isTurnipDriverActive()
        val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip

        IntSetting.RENDERER_BACKEND.setInt(1)
        val res = if (profile.isAdreno830) 3 else if (profile.tier >= HardwareTier.MIDRANGE && !profile.isAdreno6xx && profile.totalRamGb >= 6.0) 2 else 1
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(0)
        IntSetting.DMA_ACCURACY.setInt(1)
        IntSetting.RENDERER_VSYNC.setInt(if (profile.isAdreno && profile.tier >= HardwareTier.FLAGSHIP) 1 else 2)
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(0) // Conservative
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(2) // Fast
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0)
        IntSetting.RENDERER_SCALING_FILTER.setInt(6)
        IntSetting.FSR_SHARPENING_SLIDER.setInt(80)
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1) // GPU
        IntSetting.ASTC_RECOMPRESSION.setInt(if (profile.isAdreno6xx || profile.totalRamGb < 6.5) 2 else 0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(2) // GPU
        IntSetting.MAX_ANISOTROPY.setInt(0)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(0)
        IntSetting.MEMORY_LAYOUT.setInt(0) // 4GB
        BooleanSetting.USE_DOCKED_MODE.setBoolean(false) // Handheld for speed/efficiency
        IntSetting.RENDERER_DYNA_STATE.setInt(if (canUseEds1) 1 else 0)
        val workers = if (profile.isMali || profile.isExynosOrXclipse || profile.isTensor || profile.isAdreno6xx || profile.totalRamGb < 6.0) 2 else if (profile.isAdreno830) 4 else 2
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(workers)

        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
        BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
        BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
        BooleanSetting.FASTMEM.setBoolean(true)
        BooleanSetting.FASTMEM_EXCLUSIVES.setBoolean(true)
        BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
        BooleanSetting.SYNC_MEMORY_OPERATIONS.setBoolean(false)
        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.setBoolean(true)
        BooleanSetting.SKIP_CPU_INNER_INVALIDATION.setBoolean(profile.tier != HardwareTier.FLAGSHIP_ELITE)
        BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
        BooleanSetting.ENABLE_BUFFER_HISTORY.setBoolean(false)
        BooleanSetting.ENABLE_GPU_BUFFER_READBACK.setBoolean(false)
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(canUseEds1)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(false)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(false)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(false)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(false)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(profile.totalRamGb < 6.5)
    }

    /**
     * Dynamically tailored "Нормальный (Рекомендуется)" preset.
     */
    private fun applyDeviceNormalPreset(profile: DeviceHardwareProfile) {
        val isTurnip = GpuDriverHelper.isTurnipDriverActive()
        val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip

        IntSetting.RENDERER_BACKEND.setInt(1)
        val res = if (profile.totalRamGb < 5.0) 1 else if (profile.isDimensity9400 || profile.isDimensity9300Or9200 || profile.isAdreno6xx || (profile.tier <= HardwareTier.MIDRANGE)) 2 else 3
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(0)
        IntSetting.DMA_ACCURACY.setInt(1)
        IntSetting.RENDERER_VSYNC.setInt(if (profile.isAdreno && profile.tier >= HardwareTier.FLAGSHIP) 1 else 2)
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.totalRamGb < 8.0 || !profile.isAdreno || profile.isAdreno6xx) 0 else 1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(1) // Balanced
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0)
        IntSetting.RENDERER_SCALING_FILTER.setInt(6)
        IntSetting.FSR_SHARPENING_SLIDER.setInt(80)
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1) // GPU
        IntSetting.ASTC_RECOMPRESSION.setInt(if (profile.totalRamGb < 5.5) 2 else 0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(2) // GPU
        IntSetting.MAX_ANISOTROPY.setInt(0)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(0)
        IntSetting.MEMORY_LAYOUT.setInt(0)
        BooleanSetting.USE_DOCKED_MODE.setBoolean((profile.isTablet || (profile.isGamingDevice && profile.tier >= HardwareTier.FLAGSHIP)) && profile.totalRamGb >= 7.5)
        IntSetting.RENDERER_DYNA_STATE.setInt(if (canUseEds1) 1 else 0)
        val workers = if (profile.isMali || profile.isExynosOrXclipse || profile.isTensor || profile.isAdreno6xx || profile.totalRamGb < 6.0) 2 else if (profile.isAdreno830) 4 else (profile.cpuCores - 2).coerceIn(2, 4)
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(workers)

        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
        BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
        BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
        BooleanSetting.FASTMEM.setBoolean(true)
        BooleanSetting.FASTMEM_EXCLUSIVES.setBoolean(true)
        BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
        BooleanSetting.SYNC_MEMORY_OPERATIONS.setBoolean(false)
        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.setBoolean(true)
        BooleanSetting.SKIP_CPU_INNER_INVALIDATION.setBoolean(false)
        BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
        BooleanSetting.ENABLE_BUFFER_HISTORY.setBoolean(false)
        BooleanSetting.ENABLE_GPU_BUFFER_READBACK.setBoolean(false)
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(canUseEds1)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(false)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(false)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(false)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(false)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(profile.totalRamGb < 6.5)
    }

    /**
     * Dynamically tailored "Точный" preset.
     */
    private fun applyDeviceAccuratePreset(profile: DeviceHardwareProfile) {
        val isTurnip = GpuDriverHelper.isTurnipDriverActive()
        val canUseEds1 = profile.isAdreno && !profile.isAdreno6xx && isTurnip

        IntSetting.RENDERER_BACKEND.setInt(1)
        // 1.5X for Flagship Adreno 7xx+ with >=12GB RAM, 0.75X for Adreno 6xx/Low-RAM, 1X for others
        val res = if (profile.isAdreno6xx || profile.totalRamGb < 6.5) 2 else if (profile.tier >= HardwareTier.FLAGSHIP && profile.isAdreno && profile.totalRamGb >= 11.0) 5 else 3
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(if (profile.isDimensity9400) 0 else 1)
        IntSetting.DMA_ACCURACY.setInt(3) // Safe
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.isAdreno6xx || profile.totalRamGb < 8.0) 0 else if (profile.tier >= HardwareTier.FLAGSHIP && !profile.isDimensity9400 && profile.isAdreno) 2 else 1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(0) // Strict
        IntSetting.RENDERER_ANTI_ALIASING.setInt(if (profile.isAdreno6xx || profile.totalRamGb < 7.0) 0 else if (profile.tier >= HardwareTier.FLAGSHIP) 2 else 1)
        IntSetting.RENDERER_SCALING_FILTER.setInt(6) // AMD FSR
        IntSetting.FSR_SHARPENING_SLIDER.setInt(90)
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(1) // GPU
        IntSetting.ASTC_RECOMPRESSION.setInt(if (profile.totalRamGb < 5.5) 2 else 0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(2) // GPU
        IntSetting.MAX_ANISOTROPY.setInt(if (profile.isAdreno6xx || profile.totalRamGb < 7.0) 0 else if (profile.tier >= HardwareTier.FLAGSHIP) 4 else 2)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(if (profile.isDimensity9400) 0 else 1) // Accurate or Auto
        IntSetting.MEMORY_LAYOUT.setInt(if (profile.isAdreno6xx || profile.totalRamGb < 5.5) 0 else if (profile.tier >= HardwareTier.FLAGSHIP && profile.totalRamGb >= 11.0) 2 else 1)
        BooleanSetting.USE_DOCKED_MODE.setBoolean(profile.tier >= HardwareTier.FLAGSHIP && !profile.isDimensity9400 && !profile.isAdreno6xx && profile.totalRamGb >= 7.5)
        IntSetting.RENDERER_DYNA_STATE.setInt(if (canUseEds1) 1 else 0)
        val workers = if (profile.isDimensity9400) 3 else if (profile.isMali || profile.isExynosOrXclipse || profile.isTensor || profile.isAdreno6xx || profile.totalRamGb < 6.5) 2 else (profile.cpuCores - 2).coerceIn(3, 4)
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(workers)

        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
        BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
        BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
        BooleanSetting.FASTMEM.setBoolean(true)
        BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(true)
        BooleanSetting.SYNC_MEMORY_OPERATIONS.setBoolean(true)
        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.setBoolean(true)
        BooleanSetting.SKIP_CPU_INNER_INVALIDATION.setBoolean(false)
        BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
        BooleanSetting.ENABLE_BUFFER_HISTORY.setBoolean(true)
        BooleanSetting.ENABLE_GPU_BUFFER_READBACK.setBoolean(false)
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(canUseEds1)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(false)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(false)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(false)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(false)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(profile.totalRamGb < 6.5)
    }
}
