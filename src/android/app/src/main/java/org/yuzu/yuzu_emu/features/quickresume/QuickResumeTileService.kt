// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.features.quickresume

import android.app.PendingIntent
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.os.Build
import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import androidx.annotation.RequiresApi
import org.yuzu.yuzu_emu.ui.main.MainActivity

/**
 * Quick Settings Tile Service for STORM SWITCH (Ported from STORM DS).
 *
 * Allows Android users to pull down the notification shade and tap the
 * "STORM SWITCH — Quick Resume" tile to jump straight back into their
 * last played Nintendo Switch game in 1 tap!
 */
@RequiresApi(Build.VERSION_CODES.N)
class QuickResumeTileService : TileService() {

    companion object {
        const val TILE_LABEL = "STORM SWITCH — Quick Resume"

        fun requestTileUpdate(context: Context) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                runCatching {
                    requestListeningState(
                        context,
                        ComponentName(context, QuickResumeTileService::class.java)
                    )
                }
            }
        }
    }

    override fun onStartListening() {
        super.onStartListening()
        updateTileState()
    }

    override fun onClick() {
        super.onClick()
        val launchRunnable = Runnable {
            val lastGame = QuickResumeStore.getLastGame(this)
            val intent = if (lastGame != null) {
                QuickResumeStore.createLaunchIntent(this, lastGame)
            } else {
                Intent(this, MainActivity::class.java).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
                }
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                val pendingIntent = PendingIntent.getActivity(
                    this,
                    0,
                    intent,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                )
                startActivityAndCollapse(pendingIntent)
            } else {
                @Suppress("DEPRECATION")
                startActivityAndCollapse(intent)
            }
        }

        if (isLocked) {
            unlockAndRun(launchRunnable)
        } else {
            launchRunnable.run()
        }
    }

    private fun updateTileState() {
        val tile = qsTile ?: return
        val lastGame = QuickResumeStore.getLastGame(this)

        if (lastGame != null && lastGame.title.isNotEmpty()) {
            tile.state = Tile.STATE_ACTIVE
            tile.label = TILE_LABEL
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                tile.subtitle = lastGame.title
            }
        } else {
            tile.state = Tile.STATE_INACTIVE
            tile.label = TILE_LABEL
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                tile.subtitle = "Запустить последнюю игру"
            }
        }
        tile.updateTile()
    }
}
