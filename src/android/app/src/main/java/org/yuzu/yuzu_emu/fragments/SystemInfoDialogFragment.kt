// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.ActivityManager
import android.app.Dialog
import android.content.Context
import android.os.Build
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.DialogSystemInfoBinding

class SystemInfoDialogFragment : DialogFragment() {
    private var _binding: DialogSystemInfoBinding? = null
    private val binding get() = _binding!!

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, org.yuzu.yuzu_emu.utils.ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onCreateView(
        inflater: android.view.LayoutInflater,
        container: android.view.ViewGroup?,
        savedInstanceState: Bundle?
    ): android.view.View {
        _binding = DialogSystemInfoBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: android.view.View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        androidx.core.view.ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(
                androidx.core.view.WindowInsetsCompat.Type.systemBars() or
                androidx.core.view.WindowInsetsCompat.Type.displayCutout()
            )
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        binding.root.requestApplyInsets()

        populateSystemInfo()

        binding.btnCloseDialog.setOnClickListener {
            dismiss()
        }

        binding.btnCopyInfo.setOnClickListener {
            copyDiagnosticsToClipboard()
        }
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            window.setLayout(
                android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                android.view.ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawableResource(android.R.color.transparent)
            androidx.core.view.WindowCompat.setDecorFitsSystemWindows(window, false)
            org.yuzu.yuzu_emu.utils.ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onConfigurationChanged(newConfig: android.content.res.Configuration) {
        super.onConfigurationChanged(newConfig)
        dialog?.window?.let { window ->
            window.setLayout(
                android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                android.view.ViewGroup.LayoutParams.MATCH_PARENT
            )
            window.setBackgroundDrawableResource(android.R.color.transparent)
            androidx.core.view.WindowCompat.setDecorFitsSystemWindows(window, false)
            org.yuzu.yuzu_emu.utils.ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    private fun populateSystemInfo() {
        val buildVer = try { NativeLibrary.getBuildVersion() } catch (_: Throwable) { "9.1.0" }
        binding.textAppBuild.text = "STORM SWITCH $buildVer"

        // 1. Device Info
        val deviceInfo = buildString {
            append(getString(R.string.sysinfo_lbl_manufacturer)).append(Build.MANUFACTURER).append("\n")
            append(getString(R.string.sysinfo_lbl_model)).append(Build.MODEL).append(" (").append(Build.DEVICE).append(")\n")
            append(getString(R.string.sysinfo_lbl_product)).append(Build.PRODUCT).append("\n")
            append(getString(R.string.sysinfo_lbl_abi)).append(Build.SUPPORTED_ABIS.joinToString(", ")).append("\n")
            append(getString(R.string.sysinfo_lbl_android_ver)).append(Build.VERSION.RELEASE).append(" (API ").append(Build.VERSION.SDK_INT).append(")\n")
            append(getString(R.string.sysinfo_lbl_sec_patch)).append(Build.VERSION.SECURITY_PATCH).append("\n")
            append(getString(R.string.sysinfo_lbl_build_id)).append(Build.ID)
        }
        binding.textDeviceInfo.text = deviceInfo

        // 2. CPU / SoC Info
        val cpuInfo = buildString {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && Build.SOC_MODEL.isNotBlank()) {
                append(getString(R.string.sysinfo_lbl_soc_platform)).append(Build.SOC_MODEL).append("\n")
            }
            val cpuSummary = try {
                NativeLibrary.getCpuSummary()
            } catch (_: Throwable) {
                ""
            }
            if (cpuSummary.isNotEmpty() && cpuSummary != "Unknown") {
                append(getString(R.string.sysinfo_lbl_processor)).append(cpuSummary)
            } else {
                append(getString(R.string.sysinfo_lbl_arch_arm64))
            }
        }
        binding.textCpuInfo.text = cpuInfo

        // 3. GPU Info
        val gpuInfo = buildString {
            try {
                val gpuModel = NativeLibrary.getGpuModel()
                append(getString(R.string.sysinfo_lbl_gpu_model)).append(if (gpuModel.isNotEmpty()) gpuModel else "Adreno / Mali GPU").append("\n")

                val vulkanApi = NativeLibrary.getVulkanApiVersion()
                append("• Vulkan API: ").append(if (vulkanApi.isNotEmpty()) vulkanApi else "1.3.x").append("\n")

                val vulkanDriver = NativeLibrary.getVulkanDriverVersion()
                append(getString(R.string.sysinfo_lbl_driver_ver)).append(if (vulkanDriver.isNotEmpty()) vulkanDriver else "Vulkan Hardware Driver").append("\n")

                val frameGen = try { NativeLibrary.supportsFrameGeneration() } catch (_: Throwable) { false }
                append(getString(R.string.sysinfo_lbl_frame_gen)).append(if (frameGen) getString(R.string.sysinfo_val_supported) else getString(R.string.sysinfo_val_not_supported))
            } catch (e: Throwable) {
                append(getString(R.string.sysinfo_lbl_status)).append(e.message)
            }
        }
        binding.textGpuInfo.text = gpuInfo

        // 4. Memory Info
        val ramInfo = buildString {
            val activityManager =
                requireContext().getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
            val memInfo = ActivityManager.MemoryInfo()
            activityManager.getMemoryInfo(memInfo)
            val totalDeviceRam = memInfo.totalMem / (1024 * 1024)
            val availDeviceRam = memInfo.availMem / (1024 * 1024)

            append(getString(R.string.sysinfo_lbl_total_ram)).append(totalDeviceRam).append(" MB\n")
            append(getString(R.string.sysinfo_lbl_avail_ram)).append(availDeviceRam).append(" MB\n")
            append(getString(R.string.sysinfo_lbl_low_mem_thresh)).append(memInfo.threshold / (1024 * 1024)).append(" MB")
        }
        binding.textRamInfo.text = ramInfo
    }

    private fun copyDiagnosticsToClipboard() {
        val buildVer = try { NativeLibrary.getBuildVersion() } catch (_: Throwable) { "9.1.0" }
        val fullInfo = buildString {
            appendLine("=== STORM SWITCH v$buildVer System Diagnostics ===")
            appendLine("Date: ${java.util.Date()}")
            appendLine("Device: ${Build.MANUFACTURER} ${Build.MODEL} (${Build.DEVICE})")
            appendLine("Android: ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT}), Patch: ${Build.VERSION.SECURITY_PATCH}")
            appendLine("CPU: ${binding.textCpuInfo.text}")
            appendLine("GPU: ${binding.textGpuInfo.text}")
            appendLine("RAM: ${binding.textRamInfo.text}")
        }
        val clipboard = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
        val clip = android.content.ClipData.newPlainText("STORM SWITCH System Info", fullInfo)
        clipboard.setPrimaryClip(clip)
        android.widget.Toast.makeText(requireContext(), R.string.copied_to_clipboard, android.widget.Toast.LENGTH_SHORT).show()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    companion object {
        const val TAG = "SystemInfoDialogFragment"

        fun newInstance(): SystemInfoDialogFragment {
            return SystemInfoDialogFragment()
        }
    }
}
