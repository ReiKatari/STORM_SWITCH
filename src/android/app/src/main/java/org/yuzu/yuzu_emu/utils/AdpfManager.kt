// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import android.os.Build
import android.os.PerformanceHintManager
import org.yuzu.yuzu_emu.NativeLibrary

/**
 * Android Dynamic Performance Framework (ADPF) manager.
 * Uses PerformanceHintManager HintSession (API 31+) to inform the Android OS scheduler
 * of CPU/GPU work targets for emulation and render threads.
 */
object AdpfManager {
    private var hintSession: PerformanceHintManager.Session? = null
    private var currentTargetNanos: Long = 16_666_666L // Default 60 FPS target
    private var isInitialized = false

    fun isSessionActive(): Boolean = isInitialized

    fun initSession(context: Context, targetFps: Float = 60f) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return
        }
        try {
            close()
            val phm = context.getSystemService(PerformanceHintManager::class.java) ?: return
            val tids = NativeLibrary.getEmulationTids()
            if (tids.isEmpty()) {
                Log.info("[ADPF] No emulation TIDs available yet, deferring session creation")
                return
            }
            val fps = targetFps.coerceIn(15f, 120f)
            currentTargetNanos = (1_000_000_000L / fps.toLong())
            hintSession = phm.createHintSession(tids, currentTargetNanos)
            isInitialized = true
            Log.info("[ADPF] Created PerformanceHintManager HintSession for ${tids.size} threads (target: ${currentTargetNanos}ns, ${fps} FPS)")
        } catch (e: Throwable) {
            Log.warning("[ADPF] Failed to create HintSession: ${e.message}")
        }
    }

    fun reportActualWorkDuration(actualDurationNanos: Long) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S || !isInitialized) {
            return
        }
        try {
            if (actualDurationNanos > 0L) {
                // Bound the reported duration to at most 4x target to prevent scheduler over-reaction
                val clamped = actualDurationNanos.coerceAtMost(currentTargetNanos * 4)
                hintSession?.reportActualWorkDuration(clamped)
            }
        } catch (_: Throwable) {}
    }

    fun updateTargetFps(targetFps: Float) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S || !isInitialized) {
            return
        }
        try {
            val fps = targetFps.coerceIn(15f, 120f)
            currentTargetNanos = (1_000_000_000L / fps.toLong())
            hintSession?.updateTargetWorkDuration(currentTargetNanos)
        } catch (_: Throwable) {}
    }

    fun close() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            try {
                hintSession?.close()
            } catch (_: Throwable) {}
            hintSession = null
            isInitialized = false
        }
    }
}
