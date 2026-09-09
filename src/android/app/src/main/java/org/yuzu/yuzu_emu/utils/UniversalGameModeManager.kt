// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Activity
import android.app.GameManager
import android.app.GameState
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.PowerManager
import android.view.Window

/**
 * Universal Game Mode and Game Booster Manager for Android devices.
 * Integrates with:
 *  - Samsung: Game Booster, Game Tools, Game Optimizing Service (GOS), Gaming Hub
 *  - Xiaomi / POCO / Redmi: Game Turbo, Joyose, SecurityCenter Game Space
 *  - OnePlus / OPPO / Realme: ColorOS Game Space, HyperBoost, OPlus Games
 *  - Vivo / iQOO: Ultra Game Mode, Multi-Turbo, GameWatch
 *  - ASUS ROG / Zenfone: Armoury Crate, ROG GameCenter
 *  - Huawei / Honor: Game Suite, Game Assistant
 *  - Android 12+ / 13+ / 14+ / 15+: Standard GameManager & GameState API
 */
object UniversalGameModeManager {

    enum class Vendor {
        SAMSUNG,
        XIAOMI,
        OPPO_ONEPLUS_REALME,
        VIVO,
        ASUS,
        HUAWEI_HONOR,
        GENERIC
    }

    val currentVendor: Vendor by lazy {
        val manufacturer = Build.MANUFACTURER.lowercase()
        val brand = Build.BRAND.lowercase()
        when {
            manufacturer.contains("samsung") || brand.contains("samsung") -> Vendor.SAMSUNG
            manufacturer.contains("xiaomi") || brand.contains("xiaomi") ||
            manufacturer.contains("redmi") || brand.contains("redmi") ||
            manufacturer.contains("poco") || brand.contains("poco") ||
            manufacturer.contains("blackshark") -> Vendor.XIAOMI
            manufacturer.contains("oppo") || brand.contains("oppo") ||
            manufacturer.contains("oneplus") || brand.contains("oneplus") ||
            manufacturer.contains("realme") || brand.contains("realme") -> Vendor.OPPO_ONEPLUS_REALME
            manufacturer.contains("vivo") || brand.contains("vivo") ||
            manufacturer.contains("iqoo") || brand.contains("iqoo") -> Vendor.VIVO
            manufacturer.contains("asus") || brand.contains("asus") -> Vendor.ASUS
            manufacturer.contains("huawei") || brand.contains("huawei") ||
            manufacturer.contains("honor") || brand.contains("honor") -> Vendor.HUAWEI_HONOR
            else -> Vendor.GENERIC
        }
    }

    private fun isPackageInstalled(context: Context, packageName: String): Boolean {
        return try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                context.packageManager.getPackageInfo(packageName, PackageManager.PackageInfoFlags.of(0))
            } else {
                @Suppress("DEPRECATION")
                context.packageManager.getPackageInfo(packageName, 0)
            }
            true
        } catch (_: Throwable) {
            false
        }
    }

    private fun safeSendExplicitBroadcast(context: Context, targetPackage: String, intent: Intent) {
        try {
            intent.setPackage(targetPackage)
            intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                // FLAG_RECEIVER_INCLUDE_BACKGROUND = 0x01000000
                intent.addFlags(0x01000000)
            }
            context.sendBroadcast(intent)
        } catch (_: Throwable) {}
    }

    /**
     * Call when emulation game starts or loads.
     */
    fun onGameStart(activity: Activity, gameTitle: String?) {
        val context = activity.applicationContext
        val packageName = context.packageName
        val title = gameTitle ?: "Nintendo Switch Game"

        // 1. AOSP Android GameManager API (API 31+ / API 33+ GameState)
        // Supported by Android 12+, Samsung One UI 5+, Xiaomi HyperOS, ColorOS 13+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    gameManager?.setGameState(GameState(false, GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
                }
                val mode = gameManager?.gameMode ?: 0
                Log.info("[UniversalGameModeManager] AOSP Game Mode: $mode for $title")
            } catch (t: Throwable) {
                Log.warning("[UniversalGameModeManager] GameManager error: ${t.message}")
            }
        }

        // 2. Window display performance flags
        applyWindowPerformanceFlags(activity.window, context, isGaming = true)

        // 3. Dispatch vendor-specific game start signals
        dispatchVendorGameStart(context, packageName, title)
    }

    /**
     * Call when emulation activity is resumed.
     */
    fun onGameResume(activity: Activity, gameTitle: String?) {
        val context = activity.applicationContext
        val packageName = context.packageName
        val title = gameTitle ?: "Nintendo Switch Game"

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
            } catch (_: Throwable) {}
        }

        applyWindowPerformanceFlags(activity.window, context, isGaming = true)
        dispatchVendorGameResume(context, packageName, title)
    }

    /**
     * Call when emulation activity is paused.
     */
    fun onGamePause(activity: Activity) {
        val context = activity.applicationContext
        val packageName = context.packageName

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_NONE))
            } catch (_: Throwable) {}
        }

        applyWindowPerformanceFlags(activity.window, context, isGaming = false)
        dispatchVendorGamePause(context, packageName)
    }

    /**
     * Call when emulation activity is destroyed / game stopped.
     */
    fun onGameStop(activity: Activity) {
        val context = activity.applicationContext
        val packageName = context.packageName

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_NONE))
            } catch (_: Throwable) {}
        }

        applyWindowPerformanceFlags(activity.window, context, isGaming = false)
        dispatchVendorGameStop(context, packageName)
    }

    private fun applyWindowPerformanceFlags(window: Window, context: Context, isGaming: Boolean) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                window.setPreferMinimalPostProcessing(isGaming)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                val powerManager = context.getSystemService(Context.POWER_SERVICE) as? PowerManager
                if (powerManager?.isSustainedPerformanceModeSupported == true) {
                    window.setSustainedPerformanceMode(isGaming)
                }
            }
        } catch (_: Throwable) {}
    }

    // ==========================================
    // Vendor-specific dispatch implementations
    // ==========================================

    private fun dispatchVendorGameStart(context: Context, packageName: String, title: String) {
        // --- Samsung Game Booster, Game Tools, GOS ---
        if (currentVendor == Vendor.SAMSUNG || isPackageInstalled(context, "com.samsung.android.game.gos") || isPackageInstalled(context, "com.samsung.android.game.gametools")) {
            val samsungPackages = listOf(
                "com.samsung.android.game.gos",
                "com.samsung.android.game.gametools",
                "com.samsung.android.game.gamehome"
            )
            for (pkg in samsungPackages) {
                val intent = Intent("com.samsung.android.game.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("packageName", packageName)
                    putExtra("game_name", title)
                    putExtra("game_title", title)
                    putExtra("is_game", true)
                    putExtra("game_type", 1)
                    putExtra("category", "game")
                }
                safeSendExplicitBroadcast(context, pkg, intent)
            }

            val toolsIntent = Intent("com.samsung.android.game.gametools.ACTION_START_GAME").apply {
                putExtra("package_name", packageName)
                putExtra("game_title", title)
                putExtra("is_game", true)
            }
            safeSendExplicitBroadcast(context, "com.samsung.android.game.gametools", toolsIntent)

            val gosIntent = Intent("com.samsung.android.game.action.GAME_FOREGROUND").apply {
                putExtra("package_name", packageName)
                putExtra("game_title", title)
            }
            safeSendExplicitBroadcast(context, "com.samsung.android.game.gos", gosIntent)
            Log.info("[UniversalGameModeManager] Samsung Game Booster start dispatched")
        }

        // --- Xiaomi / POCO / Redmi (Game Turbo & Joyose) ---
        if (currentVendor == Vendor.XIAOMI || isPackageInstalled(context, "com.xiaomi.joyose") || isPackageInstalled(context, "com.miui.securitycenter")) {
            val xiaomiPackages = listOf("com.xiaomi.joyose", "com.miui.securitycenter")
            for (pkg in xiaomiPackages) {
                val turboIntent = Intent("miui.intent.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("packageName", packageName)
                    putExtra("game_name", title)
                    putExtra("is_game", true)
                }
                safeSendExplicitBroadcast(context, pkg, turboIntent)

                val joyoseIntent = Intent("com.xiaomi.joyose.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("game_name", title)
                    putExtra("game_mode", 1)
                }
                safeSendExplicitBroadcast(context, pkg, joyoseIntent)
            }
            Log.info("[UniversalGameModeManager] Xiaomi Game Turbo start dispatched")
        }

        // --- OnePlus / OPPO / Realme (HyperBoost & ColorOS Game Space) ---
        if (currentVendor == Vendor.OPPO_ONEPLUS_REALME || isPackageInstalled(context, "com.oplus.games") || isPackageInstalled(context, "com.coloros.gamespace")) {
            val oplusPackages = listOf("com.oplus.games", "com.coloros.gamespace", "com.heytap.gamecenter")
            for (pkg in oplusPackages) {
                val oplusIntent = Intent("com.oplus.games.action.GAME_START").apply {
                    putExtra("pkgName", packageName)
                    putExtra("packageName", packageName)
                    putExtra("gameName", title)
                    putExtra("state", 1)
                }
                safeSendExplicitBroadcast(context, pkg, oplusIntent)

                val hyperIntent = Intent("com.oplus.hyperboost.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("game_name", title)
                }
                safeSendExplicitBroadcast(context, pkg, hyperIntent)
            }
            Log.info("[UniversalGameModeManager] OPlus HyperBoost start dispatched")
        }

        // --- Vivo / iQOO (Ultra Game Mode & Multi-Turbo) ---
        if (currentVendor == Vendor.VIVO || isPackageInstalled(context, "com.vivo.game") || isPackageInstalled(context, "com.vivo.gamewatch")) {
            val vivoPackages = listOf("com.vivo.game", "com.vivo.gamewatch")
            for (pkg in vivoPackages) {
                val vivoIntent = Intent("com.vivo.game.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("game_title", title)
                    putExtra("status", 1)
                }
                safeSendExplicitBroadcast(context, pkg, vivoIntent)
            }
            Log.info("[UniversalGameModeManager] Vivo Ultra Game Mode start dispatched")
        }

        // --- ASUS ROG / Zenfone (Armoury Crate & ROG GameCenter) ---
        if (currentVendor == Vendor.ASUS || isPackageInstalled(context, "com.asus.armourygamecenter") || isPackageInstalled(context, "com.asus.gamecenter")) {
            val asusPackages = listOf("com.asus.armourygamecenter", "com.asus.gamecenter")
            for (pkg in asusPackages) {
                val asusIntent = Intent("com.asus.gamecenter.action.GAME_START").apply {
                    putExtra("packageName", packageName)
                    putExtra("package_name", packageName)
                    putExtra("isGame", true)
                }
                safeSendExplicitBroadcast(context, pkg, asusIntent)
            }
            Log.info("[UniversalGameModeManager] ASUS Armoury Crate start dispatched")
        }

        // --- Huawei / Honor (Game Suite & Game Assistant) ---
        if (currentVendor == Vendor.HUAWEI_HONOR || isPackageInstalled(context, "com.huawei.gamebox") || isPackageInstalled(context, "com.huawei.gameassistant")) {
            val huaweiPackages = listOf("com.huawei.gamebox", "com.huawei.gameassistant")
            for (pkg in huaweiPackages) {
                val huaweiIntent = Intent("com.huawei.game.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("game_name", title)
                }
                safeSendExplicitBroadcast(context, pkg, huaweiIntent)
            }
            Log.info("[UniversalGameModeManager] Huawei Game Suite start dispatched")
        }
    }

    private fun dispatchVendorGameResume(context: Context, packageName: String, title: String) {
        if (currentVendor == Vendor.SAMSUNG || isPackageInstalled(context, "com.samsung.android.game.gos")) {
            val samsungPackages = listOf("com.samsung.android.game.gos", "com.samsung.android.game.gametools")
            for (pkg in samsungPackages) {
                val intent = Intent("com.samsung.android.game.action.GAME_RESUME").apply {
                    putExtra("package_name", packageName)
                    putExtra("game_name", title)
                }
                safeSendExplicitBroadcast(context, pkg, intent)
            }
        }
        if (currentVendor == Vendor.XIAOMI || isPackageInstalled(context, "com.xiaomi.joyose")) {
            val intent = Intent("miui.intent.action.GAME_RESUME").apply {
                putExtra("package_name", packageName)
            }
            safeSendExplicitBroadcast(context, "com.xiaomi.joyose", intent)
        }
    }

    private fun dispatchVendorGamePause(context: Context, packageName: String) {
        if (currentVendor == Vendor.SAMSUNG || isPackageInstalled(context, "com.samsung.android.game.gos")) {
            val samsungPackages = listOf("com.samsung.android.game.gos", "com.samsung.android.game.gametools")
            for (pkg in samsungPackages) {
                val intent = Intent("com.samsung.android.game.action.GAME_PAUSE").apply {
                    putExtra("package_name", packageName)
                }
                safeSendExplicitBroadcast(context, pkg, intent)
            }
        }
        if (currentVendor == Vendor.XIAOMI || isPackageInstalled(context, "com.xiaomi.joyose")) {
            val intent = Intent("miui.intent.action.GAME_PAUSE").apply {
                putExtra("package_name", packageName)
            }
            safeSendExplicitBroadcast(context, "com.xiaomi.joyose", intent)
        }
    }

    private fun dispatchVendorGameStop(context: Context, packageName: String) {
        if (currentVendor == Vendor.SAMSUNG || isPackageInstalled(context, "com.samsung.android.game.gos")) {
            val samsungPackages = listOf("com.samsung.android.game.gos", "com.samsung.android.game.gametools")
            for (pkg in samsungPackages) {
                val intent = Intent("com.samsung.android.game.action.GAME_STOP").apply {
                    putExtra("package_name", packageName)
                }
                safeSendExplicitBroadcast(context, pkg, intent)
            }
        }
        if (currentVendor == Vendor.XIAOMI || isPackageInstalled(context, "com.xiaomi.joyose")) {
            val intent = Intent("miui.intent.action.GAME_STOP").apply {
                putExtra("package_name", packageName)
            }
            safeSendExplicitBroadcast(context, "com.xiaomi.joyose", intent)
            safeSendExplicitBroadcast(context, "com.miui.securitycenter", intent)
        }
        if (currentVendor == Vendor.OPPO_ONEPLUS_REALME || isPackageInstalled(context, "com.oplus.games")) {
            val intent = Intent("com.oplus.games.action.GAME_STOP").apply {
                putExtra("pkgName", packageName)
                putExtra("state", 0)
            }
            safeSendExplicitBroadcast(context, "com.oplus.games", intent)
        }
        if (currentVendor == Vendor.VIVO || isPackageInstalled(context, "com.vivo.game")) {
            val intent = Intent("com.vivo.game.action.GAME_STOP").apply {
                putExtra("package_name", packageName)
                putExtra("status", 0)
            }
            safeSendExplicitBroadcast(context, "com.vivo.game", intent)
        }
        if (currentVendor == Vendor.ASUS || isPackageInstalled(context, "com.asus.gamecenter")) {
            val intent = Intent("com.asus.gamecenter.action.GAME_STOP").apply {
                putExtra("packageName", packageName)
            }
            safeSendExplicitBroadcast(context, "com.asus.gamecenter", intent)
        }
        if (currentVendor == Vendor.HUAWEI_HONOR || isPackageInstalled(context, "com.huawei.gamebox")) {
            val intent = Intent("com.huawei.game.action.GAME_STOP").apply {
                putExtra("package_name", packageName)
            }
            safeSendExplicitBroadcast(context, "com.huawei.gamebox", intent)
        }
    }
}
