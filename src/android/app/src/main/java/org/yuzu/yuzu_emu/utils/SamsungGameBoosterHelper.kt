// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.GameManager
import android.app.GameState
import android.content.Context
import android.content.Intent
import android.os.Build

/**
 * Handles full system integration with Samsung Game Booster, Game Tools,
 * Game Optimizing Service (GOS), Game Home, and Android 12+ GameManager.
 */
object SamsungGameBoosterHelper {

    fun isSamsungDevice(): Boolean {
        return Build.MANUFACTURER.contains("samsung", ignoreCase = true) ||
               Build.BRAND.contains("samsung", ignoreCase = true)
    }

    private fun safeSendBroadcast(context: Context, intent: Intent) {
        try {
            context.sendBroadcast(intent)
        } catch (_: Throwable) {}
    }

    /**
     * Notify Samsung Game Booster and Android OS that active gameplay has started.
     */
    fun onGameStart(context: Context, gameTitle: String?) {
        val packageName = context.packageName
        val title = gameTitle ?: "Nintendo Switch Game"

        // 1. Android OS Game Manager (API 33+ GameState, API 31+ GameManager)
        // Samsung Game Booster (GOS) natively monitors GameManager across all modern One UI devices
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    gameManager?.setGameState(GameState(false, GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
                }
            } catch (e: Throwable) {
                Log.warning("[SamsungGameBoosterHelper] GameManager setGameState error: ${e.message}")
            }
        }

        // 2. Samsung-specific broadcasts (safely dispatched only on Samsung devices)
        if (isSamsungDevice()) {
            try {
                val startIntent = Intent("com.samsung.android.game.action.GAME_START").apply {
                    putExtra("package_name", packageName)
                    putExtra("packageName", packageName)
                    putExtra("game_name", title)
                    putExtra("game_title", title)
                    putExtra("is_game", true)
                    putExtra("game_type", 1)
                }
                safeSendBroadcast(context, startIntent)

                val toolsIntent = Intent("com.samsung.android.game.gametools.ACTION_START_GAME").apply {
                    setPackage("com.samsung.android.game.gametools")
                    putExtra("package_name", packageName)
                    putExtra("game_title", title)
                }
                safeSendBroadcast(context, toolsIntent)

                val gosIntent = Intent("com.samsung.android.game.action.GAME_FOREGROUND").apply {
                    setPackage("com.samsung.android.game.gos")
                    putExtra("package_name", packageName)
                    putExtra("game_title", title)
                }
                safeSendBroadcast(context, gosIntent)

                Log.info("[SamsungGameBoosterHelper] Game start signaled for $title ($packageName)")
            } catch (e: Throwable) {
                Log.warning("[SamsungGameBoosterHelper] Error dispatching game start: ${e.message}")
            }
        }
    }

    /**
     * Notify Samsung Game Booster and Android OS that emulation has resumed.
     */
    fun onGameResume(context: Context, gameTitle: String?) {
        val packageName = context.packageName
        val title = gameTitle ?: "Nintendo Switch Game"

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
            } catch (_: Throwable) {}
        }

        if (isSamsungDevice()) {
            val resumeIntent = Intent("com.samsung.android.game.action.GAME_RESUME").apply {
                putExtra("package_name", packageName)
                putExtra("game_name", title)
            }
            safeSendBroadcast(context, resumeIntent)

            val toolsIntent = Intent("com.samsung.android.game.gametools.ACTION_GAME_RESUME").apply {
                setPackage("com.samsung.android.game.gametools")
                putExtra("package_name", packageName)
            }
            safeSendBroadcast(context, toolsIntent)
        }
    }

    /**
     * Notify Samsung Game Booster and Android OS that emulation has paused.
     */
    fun onGamePause(context: Context) {
        val packageName = context.packageName

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_NONE))
            } catch (_: Throwable) {}
        }

        if (isSamsungDevice()) {
            val pauseIntent = Intent("com.samsung.android.game.action.GAME_PAUSE").apply {
                putExtra("package_name", packageName)
            }
            safeSendBroadcast(context, pauseIntent)

            val toolsIntent = Intent("com.samsung.android.game.gametools.ACTION_GAME_PAUSE").apply {
                setPackage("com.samsung.android.game.gametools")
                putExtra("package_name", packageName)
            }
            safeSendBroadcast(context, toolsIntent)
        }
    }

    /**
     * Notify Samsung Game Booster and Android OS that game session has ended.
     */
    fun onGameStop(context: Context) {
        val packageName = context.packageName

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            try {
                val gameManager = context.getSystemService(GameManager::class.java)
                gameManager?.setGameState(GameState(false, GameState.MODE_NONE))
            } catch (_: Throwable) {}
        }

        if (isSamsungDevice()) {
            val stopIntent = Intent("com.samsung.android.game.action.GAME_STOP").apply {
                putExtra("package_name", packageName)
            }
            safeSendBroadcast(context, stopIntent)

            val toolsIntent = Intent("com.samsung.android.game.gametools.ACTION_GAME_STOP").apply {
                setPackage("com.samsung.android.game.gametools")
                putExtra("package_name", packageName)
            }
            safeSendBroadcast(context, toolsIntent)
        }
    }
}
