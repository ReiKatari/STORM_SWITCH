// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.model

import android.net.Uri
import android.widget.Toast
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.preference.PreferenceManager
import java.util.Locale
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.settings.model.Settings
import org.yuzu.yuzu_emu.utils.GameHelper
import org.yuzu.yuzu_emu.utils.NativeConfig
import java.util.concurrent.atomic.AtomicBoolean

class GamesViewModel : ViewModel() {
    val games: StateFlow<List<Game>> get() = _games
    private val _games = MutableStateFlow(emptyList<Game>())

    val isReloading: StateFlow<Boolean> get() = _isReloading
    private val _isReloading = MutableStateFlow(false)

    private val reloading = AtomicBoolean(false)

    val shouldSwapData: StateFlow<Boolean> get() = _shouldSwapData
    private val _shouldSwapData = MutableStateFlow(false)

    val shouldScrollToTop: StateFlow<Boolean> get() = _shouldScrollToTop
    private val _shouldScrollToTop = MutableStateFlow(false)

    val searchFocused: StateFlow<Boolean> get() = _searchFocused
    private val _searchFocused = MutableStateFlow(false)

    val shouldScrollAfterReload: StateFlow<Boolean> get() = _shouldScrollAfterReload
    private val _shouldScrollAfterReload = MutableStateFlow(false)

    private val _folders = MutableStateFlow(mutableListOf<GameDir>())
    val folders = _folders.asStateFlow()

    private val _filteredGames = MutableStateFlow<List<Game>>(emptyList())

    var lastScrollPosition: Int = 0

    init {
        // Ensure keys are loaded so that ROM metadata can be decrypted.
        NativeLibrary.reloadKeys()

        getGameDirsAndExternalContent()
        reloadGames(directoriesChanged = false, firstStartup = true)
    }

    fun setGames(games: List<Game>) {
        val deduplicated = GameHelper.deduplicateGames(games)
        val sortedList = deduplicated.sortedWith(
            compareBy(
                { it.title.lowercase(Locale.getDefault()) },
                { it.path }
            )
        )

        _games.value = sortedList
    }

    fun setShouldSwapData(shouldSwap: Boolean) {
        _shouldSwapData.value = shouldSwap
    }

    fun setShouldScrollToTop(shouldScroll: Boolean) {
        _shouldScrollToTop.value = shouldScroll
    }

    fun setShouldScrollAfterReload(shouldScroll: Boolean) {
        _shouldScrollAfterReload.value = shouldScroll
    }

    fun setSearchFocused(searchFocused: Boolean) {
        _searchFocused.value = searchFocused
    }

    fun setFilteredGames(games: List<Game>) {
        _filteredGames.value = games
    }

    fun reloadGames(directoriesChanged: Boolean, firstStartup: Boolean = false) {
        if (reloading.get()) {
            return
        }
        reloading.set(true)
        _isReloading.value = true

        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                try {
                    if (firstStartup) {
                        // Retrieve list of cached games
                        val storedGames =
                            PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)
                                .getStringSet(GameHelper.KEY_GAMES, emptySet())
                        if (storedGames!!.isNotEmpty()) {
                            val deserializedGames = mutableSetOf<Game>()
                            storedGames.forEach {
                                val game: Game
                                try {
                                    game = Json.decodeFromString(it)
                                } catch (e: Exception) {
                                    // We don't care about any errors related to parsing the game cache
                                    return@forEach
                                }

                                val gameExists =
                                    DocumentFile.fromSingleUri(
                                        YuzuApplication.appContext,
                                        Uri.parse(game.path)
                                    )?.exists()
                                if (gameExists == true) {
                                    deserializedGames.add(game)
                                }
                            }
                            setGames(deserializedGames.toList())
                        }
                    }

                    setGames(GameHelper.getGames())
                    _shouldScrollAfterReload.value = true
                    setShouldSwapData(true)
                } catch (e: Throwable) {
                    org.yuzu.yuzu_emu.utils.Log.error("[GamesViewModel] Error reloading games: ${e.message}")
                } finally {
                    reloading.set(false)
                    _isReloading.value = false
                }
            }
        }
    }

    fun addFolder(gameDir: GameDir, savedFromGameFragment: Boolean) =
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                when (gameDir.type) {
                    DirectoryType.GAME -> {
                        NativeConfig.addGameDir(gameDir)
                        val prefs = PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)
                        val currentBackup = prefs.getStringSet("game_directories_backup", emptySet())?.toMutableSet() ?: mutableSetOf()
                        currentBackup.add(gameDir.uriString)
                        prefs.edit().putStringSet("game_directories_backup", currentBackup).apply()
                        NativeConfig.saveGlobalConfig()
                        val isFirstTimeSetup = prefs.getBoolean(Settings.PREF_FIRST_APP_LAUNCH, true)
                        getGameDirsAndExternalContent(!isFirstTimeSetup)
                    }
                    DirectoryType.EXTERNAL_CONTENT -> {
                        addExternalContentDir(gameDir.uriString)
                        NativeConfig.saveGlobalConfig()
                        getGameDirsAndExternalContent(true)
                    }
                }
            }

            if (savedFromGameFragment) {
                NativeConfig.saveGlobalConfig()
                Toast.makeText(
                    YuzuApplication.appContext,
                    R.string.add_directory_success,
                    Toast.LENGTH_SHORT
                ).show()
            }
        }

    fun removeFolder(gameDir: GameDir) =
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val currentFolders = _folders.value.toMutableList()
                val remainingFolders = currentFolders.filterNot { it.uriString == gameDir.uriString }.toMutableList()
                val prefs = PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)

                when (gameDir.type) {
                    DirectoryType.GAME -> {
                        val remainingGameDirs = remainingFolders.filter { it.type == DirectoryType.GAME }
                        NativeConfig.setGameDirs(remainingGameDirs.toTypedArray())
                        if (remainingGameDirs.isEmpty()) {
                            prefs.edit().remove("game_directories_backup").apply()
                            prefs.edit().remove(GameHelper.KEY_GAMES).apply()
                            GameHelper.cachedGameList.clear()
                            withContext(Dispatchers.Main) {
                                setGames(emptyList())
                            }
                        } else {
                            val dirSet = remainingGameDirs.map { it.uriString }.toSet()
                            prefs.edit().putStringSet("game_directories_backup", dirSet).apply()
                        }
                    }
                    DirectoryType.EXTERNAL_CONTENT -> {
                        removeExternalContentDir(gameDir.uriString)
                    }
                }
                NativeConfig.saveGlobalConfig()
                getGameDirsAndExternalContent(reloadList = true)
            }
        }

    fun updateGameDirs() =
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                val gameDirs = _folders.value.filter { it.type == DirectoryType.GAME }
                NativeConfig.setGameDirs(gameDirs.toTypedArray())
                val prefs = PreferenceManager.getDefaultSharedPreferences(YuzuApplication.appContext)
                val dirSet = gameDirs.map { it.uriString }.toSet()
                prefs.edit().putStringSet("game_directories_backup", dirSet).apply()
                NativeConfig.saveGlobalConfig()
                getGameDirsAndExternalContent(reloadList = true)
            }
        }

    fun onOpenGameFoldersFragment() =
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                getGameDirsAndExternalContent()
            }
        }

    fun onCloseGameFoldersFragment() {
        NativeConfig.saveGlobalConfig()
        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                getGameDirsAndExternalContent(true)
            }
        }
    }

    private fun getGameDirsAndExternalContent(reloadList: Boolean = false) {
        val gameDirs = NativeConfig.getGameDirs().distinctBy { it.uriString }.toMutableList()
        val externalContentDirs = NativeConfig.getExternalContentDirs().distinct().map {
            GameDir(it, false, DirectoryType.EXTERNAL_CONTENT)
        }
        externalContentDirs.forEach { ext ->
            if (gameDirs.none { it.uriString == ext.uriString }) {
                gameDirs.add(ext)
            }
        }
        _folders.value = gameDirs
        if (reloadList) {
            reloadGames(true)
        }
    }

    private fun addExternalContentDir(path: String) {
        val currentDirs = NativeConfig.getExternalContentDirs().toMutableList()
        if (!currentDirs.contains(path)) {
            currentDirs.add(path)
            NativeConfig.setExternalContentDirs(currentDirs.toTypedArray())
            NativeConfig.saveGlobalConfig()
        }
    }

    private fun removeExternalContentDir(path: String) {
        val currentDirs = NativeConfig.getExternalContentDirs().toMutableList()
        currentDirs.remove(path)
        NativeConfig.setExternalContentDirs(currentDirs.toTypedArray())
        NativeConfig.saveGlobalConfig()
    }
}
