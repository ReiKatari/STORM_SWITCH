// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.features.quickresume

import android.content.Context
import android.content.Intent
import android.net.Uri
import org.yuzu.yuzu_emu.activities.EmulationActivity
import org.yuzu.yuzu_emu.model.Game

/**
 * Persistent Quick Resume Store for STORM SWITCH (Ported from STORM DS).
 *
 * Saves the last played Nintendo Switch game and provides instant 1-tap launch intent
 * for Android Quick Settings Tile, desktop shortcuts, and home screen widgets.
 */
object QuickResumeStore {
    private const val PREFS_NAME = "storm_switch_quick_resume"
    private const val KEY_TITLE = "last_game_title"
    private const val KEY_PATH = "last_game_path"
    private const val KEY_PROGRAM_ID = "last_game_program_id"
    private const val KEY_DEVELOPER = "last_game_developer"
    private const val KEY_VERSION = "last_game_version"
    private const val KEY_TIMESTAMP = "last_played_time"

    @Volatile
    private var cachedLastGame: Game? = null

    fun rememberLastGame(context: Context, game: Game) {
        cachedLastGame = game
        val prefs = context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit()
            .putString(KEY_TITLE, game.title)
            .putString(KEY_PATH, game.path)
            .putString(KEY_PROGRAM_ID, game.programId)
            .putString(KEY_DEVELOPER, game.developer)
            .putString(KEY_VERSION, game.version)
            .putLong(KEY_TIMESTAMP, System.currentTimeMillis())
            .apply()

        QuickResumeTileService.requestTileUpdate(context.applicationContext)
    }

    fun getLastGame(context: Context): Game? {
        cachedLastGame?.let { return it }

        val prefs = context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val path = prefs.getString(KEY_PATH, null) ?: return null
        val title = prefs.getString(KEY_TITLE, "") ?: ""
        val programId = prefs.getString(KEY_PROGRAM_ID, "") ?: ""
        val developer = prefs.getString(KEY_DEVELOPER, "") ?: ""
        val version = prefs.getString(KEY_VERSION, "") ?: ""

        val reconstructed = Game(
            title = title,
            path = path,
            programId = programId,
            developer = developer,
            version = version
        )
        cachedLastGame = reconstructed
        return reconstructed
    }

    fun createLaunchIntent(context: Context, game: Game): Intent {
        return Intent(context, EmulationActivity::class.java).apply {
            action = Intent.ACTION_VIEW
            data = Uri.parse(game.path)
            putExtra(EmulationActivity.EXTRA_SELECTED_GAME, game)
            putExtra("game", game)
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
        }
    }
}
