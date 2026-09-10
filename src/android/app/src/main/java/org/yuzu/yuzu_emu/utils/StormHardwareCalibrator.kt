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
        val cpuCores: Int
    )

    fun detectHardware(context: Context): DeviceHardwareProfile {
        val am = context.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
        val memInfo = ActivityManager.MemoryInfo()
        am?.getMemoryInfo(memInfo)
        val totalRamGb = memInfo.totalMem / (1024.0 * 1024.0 * 1024.0)
        val availRamGb = memInfo.availMem / (1024.0 * 1024.0 * 1024.0)

        val hw = (Build.HARDWARE + " " + Build.BOARD + " " + Build.MODEL + " " + Build.MANUFACTURER).lowercase()
        val socModel = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) Build.SOC_MODEL.lowercase() else ""
        val isAdreno = GpuDriverHelper.isAdrenoGpu()

        // Form factor detection: tablet if large screen or smallest width >= 600dp
        val screenLayout = context.resources.configuration.screenLayout
        val isTablet = (screenLayout and Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_LARGE ||
            context.resources.configuration.smallestScreenWidthDp >= 600

        val isAdreno830 = isAdreno && (hw.contains("sun") || hw.contains("8750") || hw.contains("s938") || socModel.contains("8750"))
        val isAdreno7xx = isAdreno && (hw.contains("8650") || hw.contains("8550") || hw.contains("8450") || hw.contains("8475") || hw.contains("7475") || hw.contains("7675"))
        val isAdreno6xx = isAdreno && !isAdreno830 && !isAdreno7xx

        val isDimensity9400 = hw.contains("mt6991") || hw.contains("9400") || hw.contains("immortalis-g925") || socModel.contains("6991") || socModel.contains("9400")
        val isDimensity9300Or9200 = hw.contains("mt6989") || hw.contains("9300") || hw.contains("mt6985") || hw.contains("9200") || hw.contains("immortalis-g720") || hw.contains("immortalis-g715")
        val isExynosOrXclipse = hw.contains("s5e") || hw.contains("9945") || hw.contains("9925") || hw.contains("xclipse") || hw.contains("exynos")
        val isTensor = hw.contains("tensor") || hw.contains("zuma") || hw.contains("gs201") || hw.contains("gs101")
        val isMali = !isAdreno

        // Distinguish Snapdragon 700 series (Adreno 6xx) from Snapdragon 800 series (Adreno 6xx)
        // SD865/888 have Adreno 650/660 but are flagship-class; SD700 has Adreno 616-642L with weak 2+6 CPU
        val isSnapdragon8xxA6xx = isAdreno6xx && (hw.contains("8350") || hw.contains("8250") || socModel.contains("8350") || socModel.contains("8250"))
        val isSnapdragon7xxA6xx = isAdreno6xx && !isSnapdragon8xxA6xx

        val tier = when {
            isAdreno830 || (totalRamGb >= 14.0 && (isAdreno7xx || isDimensity9400)) -> HardwareTier.FLAGSHIP_ELITE
            (totalRamGb >= 11.0 && !isSnapdragon7xxA6xx) || isAdreno7xx || isDimensity9400 || isDimensity9300Or9200 -> HardwareTier.FLAGSHIP
            isSnapdragon8xxA6xx || (totalRamGb >= 7.5 && !isSnapdragon7xxA6xx) || isExynosOrXclipse -> HardwareTier.HIGH_MIDRANGE
            totalRamGb >= 5.5 || isTensor || isSnapdragon7xxA6xx -> HardwareTier.MIDRANGE
            else -> HardwareTier.BUDGET
        }

        val socName = when {
            isAdreno830 -> "Qualcomm Snapdragon 8 Elite (SM8750 / Adreno 830)"
            hw.contains("8650") || socModel.contains("8650") -> "Qualcomm Snapdragon 8 Gen 3 (SM8650 / Adreno 750)"
            hw.contains("8550") || socModel.contains("8550") -> "Qualcomm Snapdragon 8 Gen 2 (SM8550 / Adreno 740)"
            hw.contains("8450") || hw.contains("8475") || socModel.contains("8450") -> "Qualcomm Snapdragon 8 Gen 1 / 8+ Gen 1 (SM8450 / Adreno 730)"
            hw.contains("8350") || socModel.contains("8350") -> "Qualcomm Snapdragon 888 (SM8350 / Adreno 660)"
            hw.contains("8250") || socModel.contains("8250") -> "Qualcomm Snapdragon 865 / 870 (SM8250 / Adreno 650)"
            hw.contains("7325") || socModel.contains("7325") -> "Qualcomm Snapdragon 778G / 782G (SM7325 / Adreno 642L)"
            hw.contains("7350") || socModel.contains("7350") -> "Qualcomm Snapdragon 780G (SM7350 / Adreno 642)"
            hw.contains("7250") || socModel.contains("7250") -> "Qualcomm Snapdragon 765G / 768G (SM7250 / Adreno 620)"
            hw.contains("7225") || socModel.contains("7225") -> "Qualcomm Snapdragon 750G (SM7225 / Adreno 619)"
            hw.contains("7150") || socModel.contains("7150") -> "Qualcomm Snapdragon 730 / 732G (SM7150 / Adreno 618)"
            hw.contains("7125") || socModel.contains("7125") -> "Qualcomm Snapdragon 720G (SM7125 / Adreno 618)"
            isDimensity9400 -> "MediaTek Dimensity 9400 (MT6991 / Immortalis-G925)"
            isDimensity9300Or9200 -> "MediaTek Dimensity 9300 / 9200 (Immortalis GPU)"
            isExynosOrXclipse -> "Samsung Exynos (AMD Xclipse RDNA GPU)"
            isTensor -> "Google Tensor (ARM Mali GPU)"
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && Build.SOC_MODEL.isNotBlank() -> Build.SOC_MODEL
            else -> Build.HARDWARE
        }

        val gpuName = when {
            isAdreno830 -> "Qualcomm Adreno 830 (Snapdragon 8 Elite / Turnip Vulkan 1.3 / 1.4)"
            hw.contains("8650") -> "Qualcomm Adreno 750 (Snapdragon 8 Gen 3 / Turnip)"
            hw.contains("8550") -> "Qualcomm Adreno 740 (Snapdragon 8 Gen 2 / Turnip)"
            hw.contains("8450") || hw.contains("8475") -> "Qualcomm Adreno 730 (Snapdragon 8 Gen 1 / Turnip)"
            hw.contains("8350") -> "Qualcomm Adreno 660 (Snapdragon 888 / Turnip)"
            hw.contains("8250") -> "Qualcomm Adreno 650 (Snapdragon 865/870 / Turnip)"
            isDimensity9400 -> "ARM Immortalis-G925 (Dimensity 9400 Vulkan 1.3)"
            isDimensity9300Or9200 -> "ARM Immortalis-G720 / G715 (Dimensity 9300/9200)"
            hw.contains("9945") || hw.contains("2400") -> "Samsung Xclipse 940 (AMD RDNA3 Vulkan)"
            hw.contains("9925") || hw.contains("2200") -> "Samsung Xclipse 920 (AMD RDNA2 Vulkan)"
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
            cpuCores = cpuCores
        )
    }

    /**
     * Calibrate on first startup or when explicitly requested.
     */
    fun autoCalibrate(context: Context, force: Boolean = false) {
        val prefs = PreferenceManager.getDefaultSharedPreferences(context)
        if (!force && prefs.getBoolean(PREF_HARDWARE_CALIBRATED, false)) {
            return
        }

        // Apply device-tailored defaults on first launch
        applyPreset(StormPreset.DEFAULT, context)
        prefs.edit().putBoolean(PREF_HARDWARE_CALIBRATED, true).apply()
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

        try {
            NativeConfig.saveGlobalConfig()
        } catch (_: Exception) {
        }
    }

    /**
     * Individual tailored "По умолчанию (Сброс)" defaults for this specific device.
     */
    private fun applyDeviceDefaultPreset(profile: DeviceHardwareProfile) {
        // 1. Renderer / Video Defaults
        IntSetting.RENDERER_BACKEND.setInt(1) // Vulkan
        IntSetting.RENDERER_ACCURACY.setInt(0) // Normal

        // Resolution: 1.0X (3) for Flagship/Adreno 7xx+/Dimensity 9400, 0.75X (2) for Adreno 6xx and budget Mali/Tensor
        val defaultRes = if (profile.isAdreno6xx || profile.tier == HardwareTier.BUDGET || (profile.isMali && !profile.isDimensity9400 && profile.totalRamGb < 8.0)) 2 else 3
        IntSetting.RENDERER_RESOLUTION.setInt(defaultRes)

        // VSync: Mailbox (2) for Adreno 830/750 (silky smooth, lowest input latency), FIFO (0) for others
        IntSetting.RENDERER_VSYNC.setInt(if (profile.isAdreno830 || (profile.isAdreno && profile.tier == HardwareTier.FLAGSHIP_ELITE)) 2 else 0)
        IntSetting.RENDERER_ASPECT_RATIO.setInt(0) // 16:9
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0) // None

        // Scaling filter: FSR (6) on Dimensity 9400 / Mali, Bilinear (0) on Adreno
        IntSetting.RENDERER_SCALING_FILTER.setInt(if (profile.isDimensity9400) 6 else 0)
        if (profile.isDimensity9400) {
            IntSetting.FSR_SHARPENING_SLIDER.setInt(85)
        }

        // ASTC Decode: Hybrid (3) by default
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(3)
        IntSetting.ASTC_RECOMPRESSION.setInt(0) // Uncompressed for 100% texture fidelity
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(3) // Hybrid
        IntSetting.DMA_ACCURACY.setInt(1) // Normal

        // VRAM Usage Mode: Normal (1) for Flagships/High-midrange, Conservative (0) for Low RAM/Dimensity 9400/Adreno 6xx
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.tier >= HardwareTier.HIGH_MIDRANGE && !profile.isDimensity9400 && !profile.isAdreno6xx) 1 else 0)

        // GPU Fence Behavior: Balanced (1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(1)
        IntSetting.MAX_ANISOTROPY.setInt(0) // Automatic

        // Dynamic State: Disabled (0) on Adreno 6xx / Mali / Immortalis / Xclipse, Enabled EDS1 (1) on Adreno 7xx/8xx
        IntSetting.RENDERER_DYNA_STATE.setInt(if (profile.isAdreno && !profile.isAdreno6xx) 1 else 0)

        // 2. CPU / System Defaults
        // Form factor adaptation: Tablets use Docked mode (better thermals & screen), Smartphones use Handheld
        BooleanSetting.USE_DOCKED_MODE.setBoolean(profile.isTablet || (profile.tier == HardwareTier.FLAGSHIP_ELITE && profile.totalRamGb >= 16.0))
        IntSetting.CPU_BACKEND.setInt(1) // NCE
        IntSetting.CPU_ACCURACY.setInt(0) // Auto

        // Memory Layout: 6GB (1) if RAM >= 11GB on Adreno, 4GB (0) for others
        IntSetting.MEMORY_LAYOUT.setInt(if (profile.tier >= HardwareTier.FLAGSHIP && profile.isAdreno && profile.totalRamGb >= 11.0) 1 else 0)

        // Pipeline workers: 2 for Dimensity 9400/Adreno 6xx (avoids overloading limited perf cores), 4 for 8 Elite, 3-4 for others
        val optimalWorkers = when {
            profile.isDimensity9400 || profile.isAdreno6xx -> 2
            profile.isAdreno830 -> 4
            else -> (profile.cpuCores - 2).coerceIn(2, 4)
        }
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(optimalWorkers)

        // 3. Audio Defaults
        IntSetting.AUDIO_OUTPUT_ENGINE.setInt(0) // Auto
        BooleanSetting.AUDIO_MUTED.setBoolean(false)

        // 4. Renderer Booleans
        BooleanSetting.RENDERER_ASYNCHRONOUS_GPU_EMULATION.setBoolean(true)
        BooleanSetting.RENDERER_ASYNC_PRESENTATION.setBoolean(true)
        BooleanSetting.RENDERER_EARLY_RELEASE_FENCES.setBoolean(true)
        IntSetting.RENDERER_OPTIMIZE_SPIRV_OUTPUT.setInt(1)
        BooleanSetting.ENABLE_FRAME_SKIPPING.setBoolean(true)
        BooleanSetting.RENDERER_USE_DISK_SHADER_CACHE.setBoolean(true)
        BooleanSetting.FASTMEM.setBoolean(true)
        BooleanSetting.FASTMEM_EXCLUSIVES.setBoolean(profile.tier != HardwareTier.BUDGET)
        BooleanSetting.RENDERER_REACTIVE_FLUSHING.setBoolean(false)
        BooleanSetting.SYNC_MEMORY_OPERATIONS.setBoolean(false)
        BooleanSetting.RENDERER_ASYNCHRONOUS_SHADERS.setBoolean(true)
        BooleanSetting.SKIP_CPU_INNER_INVALIDATION.setBoolean(profile.tier == HardwareTier.BUDGET)
        BooleanSetting.RENDERER_FORCE_MAX_CLOCK.setBoolean(false)
        BooleanSetting.ENABLE_BUFFER_HISTORY.setBoolean(false)
        BooleanSetting.ENABLE_GPU_BUFFER_READBACK.setBoolean(false)
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(profile.isAdreno && !profile.isAdreno6xx)

        // 5. Frame Gen Defaults
        BooleanSetting.RENDERER_FRAME_GEN.setBoolean(false)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        // 6. Thermal & Pacing Flags
        BooleanSetting.ECO_THERMAL_MODE.setBoolean(true)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(true)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(true)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(true)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(true)
    }

    /**
     * Dynamically tailored "Быстрый" preset.
     */
    private fun applyDeviceFastPreset(profile: DeviceHardwareProfile) {
        IntSetting.RENDERER_BACKEND.setInt(1)
        // 1X for Snapdragon 8 Elite, 0.75X for midrange+, 0.5X for budget and Adreno 6xx (limited fillrate)
        val res = if (profile.isAdreno830) 3 else if (profile.tier >= HardwareTier.MIDRANGE && !profile.isAdreno6xx) 2 else 1
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(0)
        IntSetting.DMA_ACCURACY.setInt(1)
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(0) // Conservative
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(2) // Fast
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0)
        IntSetting.RENDERER_SCALING_FILTER.setInt(if (profile.isDimensity9400) 6 else 0)
        if (profile.isDimensity9400) {
            IntSetting.FSR_SHARPENING_SLIDER.setInt(80)
        }
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(3) // Hybrid
        // ASTC Recompression: BC3 (2) for Adreno 6xx to save 50% texture memory on LPDDR4X bus
        IntSetting.ASTC_RECOMPRESSION.setInt(if (profile.isAdreno6xx) 2 else 0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(3) // Hybrid
        IntSetting.MAX_ANISOTROPY.setInt(0)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(0)
        IntSetting.MEMORY_LAYOUT.setInt(0) // 4GB
        BooleanSetting.USE_DOCKED_MODE.setBoolean(false) // Handheld for speed/efficiency
        IntSetting.RENDERER_DYNA_STATE.setInt(if (profile.isAdreno && !profile.isAdreno6xx) 1 else 0)
        IntSetting.ANDROID_PIPELINE_WORKERS.setInt(2)

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
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(profile.isAdreno && !profile.isAdreno6xx)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(true)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(true)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(true)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(true)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(true)
    }

    /**
     * Dynamically tailored "Нормальный (Рекомендуется)" preset.
     */
    private fun applyDeviceNormalPreset(profile: DeviceHardwareProfile) {
        IntSetting.RENDERER_BACKEND.setInt(1)
        val res = if (profile.isDimensity9400 || profile.isAdreno6xx || (profile.tier == HardwareTier.BUDGET)) 2 else 3
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(0)
        IntSetting.DMA_ACCURACY.setInt(1)
        IntSetting.RENDERER_VSYNC.setInt(if (profile.isAdreno830 || (profile.isAdreno && profile.tier >= HardwareTier.FLAGSHIP)) 2 else 0)
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.totalRamGb < 8.0 || profile.isDimensity9400 || profile.isAdreno6xx) 0 else 1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(1) // Balanced
        IntSetting.RENDERER_ANTI_ALIASING.setInt(0)
        IntSetting.RENDERER_SCALING_FILTER.setInt(if (profile.isDimensity9400) 6 else 0)
        if (profile.isDimensity9400) {
            IntSetting.FSR_SHARPENING_SLIDER.setInt(85)
        }
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(3) // Hybrid
        IntSetting.ASTC_RECOMPRESSION.setInt(0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(3) // Hybrid
        IntSetting.MAX_ANISOTROPY.setInt(0)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(0)
        IntSetting.MEMORY_LAYOUT.setInt(if (profile.tier >= HardwareTier.FLAGSHIP && profile.totalRamGb >= 11.0 && !profile.isDimensity9400) 1 else 0)
        BooleanSetting.USE_DOCKED_MODE.setBoolean(profile.isTablet)
        IntSetting.RENDERER_DYNA_STATE.setInt(if (profile.isAdreno && !profile.isAdreno6xx) 1 else 0)
        val workers = if (profile.isDimensity9400 || profile.isAdreno6xx) 2 else if (profile.isAdreno830) 4 else (profile.cpuCores - 2).coerceIn(2, 4)
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
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(profile.isAdreno && !profile.isAdreno6xx)
        BooleanSetting.RENDERER_FRAME_GEN_FP16.setBoolean(true)
        BooleanSetting.RENDERER_FRAME_GEN_FLOW_SCALE_AUTO.setBoolean(true)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(true)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(true)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(true)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(true)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(true)
    }

    /**
     * Dynamically tailored "Точный" preset.
     */
    private fun applyDeviceAccuratePreset(profile: DeviceHardwareProfile) {
        IntSetting.RENDERER_BACKEND.setInt(1)
        // 1.5X for Flagship Adreno 7xx+, 0.75X for Adreno 6xx, 1X for others
        val res = if (profile.isAdreno6xx) 2 else if (profile.tier >= HardwareTier.FLAGSHIP && profile.isAdreno) 5 else 3
        IntSetting.RENDERER_RESOLUTION.setInt(res)
        IntSetting.RENDERER_ACCURACY.setInt(if (profile.isAdreno830) 2 else if (profile.isDimensity9400) 0 else 1)
        IntSetting.DMA_ACCURACY.setInt(3) // Safe
        IntSetting.RENDERER_VRAM_USAGE_MODE.setInt(if (profile.isAdreno6xx) 0 else if (profile.tier >= HardwareTier.FLAGSHIP && !profile.isDimensity9400) 2 else 1)
        IntSetting.GPU_FENCE_BEHAVIOR.setInt(0) // Strict
        IntSetting.RENDERER_ANTI_ALIASING.setInt(if (profile.isAdreno6xx) 0 else if (profile.tier >= HardwareTier.FLAGSHIP) 2 else 1)
        IntSetting.RENDERER_SCALING_FILTER.setInt(6) // AMD FSR
        IntSetting.FSR_SHARPENING_SLIDER.setInt(90)
        IntSetting.RENDERER_ASTC_DECODE_METHOD.setInt(3) // Hybrid
        IntSetting.ASTC_RECOMPRESSION.setInt(0)
        IntSetting.RENDERER_NVDEC_EMULATION.setInt(3) // Hybrid
        IntSetting.MAX_ANISOTROPY.setInt(if (profile.isAdreno6xx) 0 else if (profile.tier >= HardwareTier.FLAGSHIP) 4 else 2)
        IntSetting.CPU_BACKEND.setInt(1)
        IntSetting.CPU_ACCURACY.setInt(if (profile.isDimensity9400) 0 else 1) // Accurate or Auto
        IntSetting.MEMORY_LAYOUT.setInt(if (profile.isAdreno6xx) 0 else if (profile.tier >= HardwareTier.FLAGSHIP && profile.totalRamGb >= 11.0) 2 else 1)
        BooleanSetting.USE_DOCKED_MODE.setBoolean(profile.tier >= HardwareTier.FLAGSHIP && !profile.isDimensity9400 && !profile.isAdreno6xx)
        IntSetting.RENDERER_DYNA_STATE.setInt(if (profile.isAdreno && !profile.isAdreno6xx) 1 else 0)
        val workers = if (profile.isDimensity9400) 3 else if (profile.isAdreno6xx) 2 else (profile.cpuCores - 2).coerceIn(3, 4)
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
        BooleanSetting.RENDERER_VERTEX_INPUT_DYNAMIC_STATE.setBoolean(profile.isAdreno && !profile.isAdreno6xx)

        BooleanSetting.ECO_THERMAL_MODE.setBoolean(true)
        BooleanSetting.ECO_FRAME_PACING.setBoolean(true)
        BooleanSetting.SMART_SHADER_THROTTLE.setBoolean(true)
        BooleanSetting.CPU_AFFINITY_PINNING.setBoolean(true)
        BooleanSetting.VULKAN_PIPELINE_CACHE.setBoolean(true)
        BooleanSetting.VRAM_GARBAGE_COLLECTION.setBoolean(true)
    }
}
