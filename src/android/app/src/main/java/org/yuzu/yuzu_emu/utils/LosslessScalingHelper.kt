// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.net.Uri
import android.os.Build
import android.os.Environment
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting
import java.io.File

object LosslessScalingHelper {
    const val RESULT_OK = 0
    const val RESULT_NOT_INSTALLED = 1

    private val _statusText = MutableStateFlow("")
    val statusText: StateFlow<String> = _statusText.asStateFlow()

    @Volatile
    private var installed: Boolean? = null

    @Volatile
    private var lastCheckedPermGranted: Boolean? = null

    private fun hasStorageAccess(): Boolean =
        Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager()

    private fun resolveSafeUserDir(): String {
        val raw = DirectoryInitialization.rawUserPath
        if (!raw.isNullOrBlank()) return raw
        return File(Environment.getExternalStorageDirectory(), "STORM SWITCH").absolutePath
    }

    fun invalidateCache() {
        installed = null
    }

    fun isInstalled(): Boolean {
        val permNow = hasStorageAccess()
        val cached = installed
        if (cached != null && lastCheckedPermGranted == permNow) {
            return cached
        }
        return refreshStatus()
    }

    fun isSupportedByGpu(): Boolean = true

    fun refreshStatus(): Boolean {
        val permNow = hasStorageAccess()
        migrateLegacyDll()
        val result = try {
            NativeLibrary.validateLosslessDll() == RESULT_OK
        } catch (_: Exception) {
            false
        }
        installed = result
        lastCheckedPermGranted = permNow
        if (!result && DirectoryInitialization.areDirectoriesReady) {
            try {
                BooleanSetting.RENDERER_FRAME_GEN.setBoolean(false)
            } catch (_: Exception) {}
        }

        try {
            val context = YuzuApplication.appContext
            _statusText.value = if (result) {
                context.getString(R.string.lossless_scaling_installed)
            } else {
                context.getString(R.string.lossless_scaling_not_installed)
            }
        } catch (_: Exception) {}
        return result
    }

    fun install(source: Uri): Int {
        return try {
            var rawPath = try { NativeLibrary.getLosslessDllPath() } catch (_: Exception) { "" }
            if (rawPath.isBlank()) {
                val userDir = resolveSafeUserDir()
                rawPath = if (userDir.isNotBlank()) {
                    File(userDir, "lossless/Lossless.dll").absolutePath
                } else {
                    val fallbackBase = YuzuApplication.appContext.getExternalFilesDir(null)?.absolutePath
                        ?: YuzuApplication.appContext.filesDir.absolutePath
                    File(fallbackBase, "lossless/Lossless.dll").absolutePath
                }
            }
            val destination = File(rawPath)
            val fallbackUserDir = resolveSafeUserDir().ifBlank { YuzuApplication.appContext.filesDir.absolutePath }
            val parentDir = destination.parentFile ?: File(fallbackUserDir, "lossless")
            parentDir.mkdirs()

            val copied = FileUtil.copyUriToInternalStorage(
                source,
                parentDir.absolutePath,
                destination.name.ifEmpty { "Lossless.dll" }
            )
            if (copied == null) {
                refreshStatus()
                return RESULT_NOT_INSTALLED
            }

            val result = try { NativeLibrary.prepareLosslessDll() } catch (_: Exception) { RESULT_NOT_INSTALLED }
            if (result != RESULT_OK) {
                try { NativeLibrary.removeLosslessDll() } catch (_: Exception) {}
            } else {
                BooleanSetting.RENDERER_FRAME_GEN.setBoolean(true)
                NativeConfig.saveGlobalConfig()
            }
            refreshStatus()
            result
        } catch (e: Exception) {
            Log.error("[LosslessScalingHelper] Error during install: ${e.message}")
            refreshStatus()
            RESULT_NOT_INSTALLED
        }
    }

    fun remove(): Boolean {
        val removed = NativeLibrary.removeLosslessDll()
        BooleanSetting.RENDERER_FRAME_GEN.setBoolean(false)
        NativeConfig.saveGlobalConfig()
        cleanCustomConfigsFrameGen()
        refreshStatus()
        return removed
    }

    private fun migrateLegacyDll() {
        try {
            val userDir = resolveSafeUserDir()
            val parentDir = if (userDir.isNotBlank()) {
                File(userDir, "lossless")
            } else {
                val fallbackBase = YuzuApplication.appContext.getExternalFilesDir(null)?.absolutePath
                    ?: YuzuApplication.appContext.filesDir.absolutePath
                File(fallbackBase, "lossless")
            }
            val oldDll = File(parentDir, "LosslessScaling.dll")
            val newDll = File(parentDir, "Lossless.dll")
            if (oldDll.exists() && oldDll.isFile && !newDll.exists()) {
                oldDll.copyTo(newDll, overwrite = true)
                Log.info("[LosslessScalingHelper] Auto-migrated LosslessScaling.dll -> Lossless.dll")
            }
        } catch (e: Exception) {
            Log.error("[LosslessScalingHelper] Error during legacy DLL migration: ${e.message}")
        }
    }

    private fun cleanCustomConfigsFrameGen() {
        try {
            val customDir = File(resolveSafeUserDir(), "config/custom")
            if (customDir.exists() && customDir.isDirectory) {
                customDir.listFiles { file -> file.name.endsWith(".ini") }?.forEach { iniFile ->
                    try {
                        val lines = iniFile.readLines()
                        var modified = false
                        val newLines = lines.map { line ->
                            val trimmed = line.trim()
                            if (trimmed.startsWith("frame_gen=", ignoreCase = true) ||
                                trimmed.startsWith("frame_gen =", ignoreCase = true)
                            ) {
                                modified = true
                                "frame_gen=false"
                            } else if (trimmed.startsWith("frame_gen\\use_global=", ignoreCase = true) ||
                                trimmed.startsWith("frame_gen\\use_global =", ignoreCase = true)
                            ) {
                                modified = true
                                "frame_gen\\use_global=true"
                            } else {
                                line
                            }
                        }
                        if (modified) {
                            iniFile.writeText(newLines.joinToString("\n"))
                            Log.info("[LosslessScalingHelper] Disabled frame_gen in custom config: ${iniFile.name}")
                        }
                    } catch (_: Exception) {}
                }
            }
        } catch (e: Exception) {
            Log.error("[LosslessScalingHelper] Failed to clean custom configs: ${e.message}")
        }
    }
}
