// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.SharedPreferences
import android.net.Uri
import android.provider.DocumentsContract
import androidx.preference.PreferenceManager
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.File
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.model.Game
import org.yuzu.yuzu_emu.model.GameDir
import org.yuzu.yuzu_emu.model.MinimalDocumentFile
import androidx.core.content.edit
import androidx.core.net.toUri
import androidx.documentfile.provider.DocumentFile
import java.util.Locale
import org.yuzu.yuzu_emu.features.settings.model.BooleanSetting

object GameHelper {
    private const val KEY_OLD_GAME_PATH = "game_path"
    const val KEY_GAMES = "Games"

    var cachedGameList = mutableListOf<Game>()

    private lateinit var preferences: SharedPreferences

    fun getGames(): List<Game> {
        val games = mutableListOf<Game>()
        val context = YuzuApplication.appContext
        preferences = PreferenceManager.getDefaultSharedPreferences(context)

        val gameDirs = mutableListOf<GameDir>()
        val oldGamesDir = preferences.getString(KEY_OLD_GAME_PATH, "") ?: ""
        if (oldGamesDir.isNotEmpty()) {
            gameDirs.add(GameDir(oldGamesDir, true))
            preferences.edit() { remove(KEY_OLD_GAME_PATH) }
        }
        NativeConfig.getGameDirs().forEach { dir ->
            if (dir.uriString.isNotBlank() && gameDirs.none { it.uriString == dir.uriString }) {
                gameDirs.add(dir)
            }
        }

        // Restore from backup if NativeConfig has no directories loaded yet
        val backupDirs = preferences.getStringSet("game_directories_backup", emptySet()) ?: emptySet()
        if (gameDirs.isEmpty() && backupDirs.isNotEmpty()) {
            backupDirs.forEach { uriStr ->
                if (uriStr.isNotBlank() && gameDirs.none { it.uriString == uriStr }) {
                    gameDirs.add(GameDir(uriStr, true))
                }
            }
            if (gameDirs.isNotEmpty()) {
                NativeConfig.setGameDirs(gameDirs.toTypedArray())
                NativeConfig.saveGlobalConfig()
            }
        }

        if (gameDirs.isEmpty()) {
            // NEVER clear KEY_GAMES or cachedGameList when directories are temporarily empty
            if (cachedGameList.isEmpty()) {
                val stored = preferences.getStringSet(KEY_GAMES, emptySet()) ?: emptySet()
                for (item in stored) {
                    try {
                        val game = Json.decodeFromString<Game>(item)
                        if (!isUpdateOrDlcPath(game.path) && !isUpdateOrDlcPath(game.title)) {
                            cachedGameList.add(upgradeGameVersionIfNeeded(game))
                        }
                    } catch (_: Exception) {}
                }
            }
            return cachedGameList.toList()
        } else {
            val dirSet = gameDirs.map { it.uriString }.toSet()
            preferences.edit().putStringSet("game_directories_backup", dirSet).apply()
        }

        if (cachedGameList.isEmpty()) {
            val stored = preferences.getStringSet(KEY_GAMES, emptySet()) ?: emptySet()
            for (item in stored) {
                try {
                    val game = Json.decodeFromString<Game>(item)
                    if (!isUpdateOrDlcPath(game.path) && !isUpdateOrDlcPath(game.title)) {
                        cachedGameList.add(upgradeGameVersionIfNeeded(game))
                    }
                } catch (_: Exception) {}
            }
        }

        // Ensure keys are loaded so that ROM metadata can be decrypted.
        NativeLibrary.reloadKeys()

        // Reset metadata so we don't use stale information
        GameMetadata.resetMetadata()

        // Remove previous filesystem provider information so we can get up to date version info
        NativeLibrary.clearFilesystemProvider()

        val mountedContainerUris = mutableSetOf<String>()
        mountExternalContentDirectories(mountedContainerUris)

        // Stage 1: Pre-mount ALL containers across ALL game directories and subdirectories
        gameDirs.forEach { gameDir ->
            val gameDirUri = gameDir.uriString.toUri()
            if (FileUtil.isTreeUriValid(gameDirUri)) {
                val scanDepth = if (gameDir.deepScan) 5 else 3
                scanContentContainersRecursive(FileUtil.listFiles(gameDirUri), scanDepth) {
                    val filePath = it.uri.toString()
                    if (mountedContainerUris.add(filePath)) {
                        NativeLibrary.addGameFolderFileToFilesystemProvider(filePath)
                    }
                }
            }
        }

        // Stage 2: Load games with all content/updates/DLCs already registered in ContentProvider
        gameDirs.forEach { gameDir ->
            val gameDirUri = gameDir.uriString.toUri()
            val isValid = FileUtil.isTreeUriValid(gameDirUri)
            if (isValid) {
                val scanDepth = if (gameDir.deepScan) 5 else 3

                addGamesRecursive(
                    games,
                    FileUtil.listFiles(gameDirUri),
                    scanDepth
                )
            }
        }

        val finalGames = deduplicateGames(games)

        // Upgrade and sanitize versions for all loaded games
        finalGames.forEach { game ->
            upgradeGameVersionIfNeeded(game)
        }

        // Preserve higher addon counts and non-base versions from cached metadata
        if (cachedGameList.isNotEmpty()) {
            val cachedMap = cachedGameList.associateBy { it.programIdHex.uppercase(Locale.ROOT) }
            finalGames.forEach { game ->
                val cached = cachedMap[game.programIdHex.uppercase(Locale.ROOT)]
                if (cached != null) {
                    if (cached.addonCount > game.addonCount) {
                        game.addonCount = cached.addonCount
                    }
                    val currentVer = game.version.removePrefix("v").removePrefix("V").trim()
                    val cachedVer = cached.version.removePrefix("v").removePrefix("V").trim()
                    if (isBaseVersion(currentVer) && !isBaseVersion(cachedVer)) {
                        game.version = cached.version
                    }
                    val currentIv = game.internalVersion.toLongOrNull() ?: 0L
                    val cachedIv = cached.internalVersion.toLongOrNull() ?: 0L
                    if (currentIv < 65536L && cachedIv >= 65536L) {
                        game.internalVersion = cached.internalVersion
                    }
                }
            }
        }

        if (finalGames.isNotEmpty()) {
            // Cache list of games found on disk
            val serializedGames = mutableSetOf<String>()
            finalGames.forEach {
                serializedGames.add(Json.encodeToString(it))
            }
            preferences.edit() {
                remove(KEY_GAMES)
                    .putStringSet(KEY_GAMES, serializedGames)
            }
            cachedGameList = finalGames.toMutableList()
            return finalGames
        } else {
            // Verify whether any previously cached games still actually exist on disk
            val verified = cachedGameList.filter { game ->
                try {
                    val uri = Uri.parse(game.path)
                    if (uri.scheme == "content") {
                        DocumentFile.fromSingleUri(YuzuApplication.appContext, uri)?.exists() == true
                    } else {
                        File(game.path).exists()
                    }
                } catch (_: Exception) {
                    false
                }
            }

            if (verified.isEmpty()) {
                // All games were deleted from the folder! Purge cache and clear the list
                preferences.edit() {
                    remove(KEY_GAMES)
                }
                cachedGameList.clear()
                return emptyList()
            }

            cachedGameList = verified.toMutableList()
            return verified
        }
    }

    fun getGameDeduplicationKey(game: Game): String {
        val pid = game.programIdHex.trim()
        if (pid != "0" && pid.isNotEmpty()) {
            val normVer = game.version.removePrefix("v").removePrefix("V").trim()
            val intVer = game.internalVersion.trim()
            return "PID_${pid.uppercase(Locale.ROOT)}_V_${normVer}_IV_${intVer}"
        }
        val ext = game.extension.uppercase(Locale.ROOT)
        val extSuffix = if (ext.isNotEmpty()) "_$ext" else ""
        val cleanTitle = cleanGameTitle(game.title).lowercase(Locale.ROOT).trim()
        if (cleanTitle.isNotEmpty() && cleanTitle != "homebrew") {
            return "TITLE_${cleanTitle}${extSuffix}"
        }
        return "${game.path}${extSuffix}"
    }

    fun selectBetterGame(existing: Game, candidate: Game): Game {
        // 1. Prefer non-update/patch container over an update dump
        val existingIsUpdate = existing.path.contains("update", ignoreCase = true) ||
            existing.path.contains("[upd", ignoreCase = true) ||
            existing.path.contains("0100.*800", ignoreCase = true)
        val candidateIsUpdate = candidate.path.contains("update", ignoreCase = true) ||
            candidate.path.contains("[upd", ignoreCase = true) ||
            candidate.path.contains("0100.*800", ignoreCase = true)
        if (existingIsUpdate && !candidateIsUpdate) {
            return candidate
        }
        if (!existingIsUpdate && candidateIsUpdate) {
            return existing
        }

        // 2. Compare internal versions
        val existingIntVer = existing.internalVersion.toLongOrNull() ?: 0L
        val candidateIntVer = candidate.internalVersion.toLongOrNull() ?: 0L
        if (candidateIntVer > existingIntVer) {
            return candidate
        } else if (candidateIntVer < existingIntVer) {
            return existing
        }

        // 3. Compare display version if internal version is equal
        val existingVer = existing.version.removePrefix("v").removePrefix("V").trim()
        val candidateVer = candidate.version.removePrefix("v").removePrefix("V").trim()
        if (!isBaseVersion(candidateVer) && isBaseVersion(existingVer)) {
            return candidate
        } else if (isBaseVersion(candidateVer) && !isBaseVersion(existingVer)) {
            return existing
        }

        // 4. Compare DLC / Addon count
        if (candidate.addonCount > existing.addonCount) {
            return candidate
        } else if (candidate.addonCount < existing.addonCount) {
            return existing
        }

        // 5. Prefer .xci / .xcz cartridges over loose .nsp
        val candidateIsXci = candidate.path.endsWith(".xci", ignoreCase = true) ||
            candidate.path.endsWith(".xcz", ignoreCase = true)
        val existingIsXci = existing.path.endsWith(".xci", ignoreCase = true) ||
            existing.path.endsWith(".xcz", ignoreCase = true)
        if (candidateIsXci && !existingIsXci) {
            return candidate
        }
        if (!candidateIsXci && existingIsXci) {
            return existing
        }

        return existing
    }

    fun deduplicateGames(games: List<Game>): List<Game> {
        val uniqueGamesMap = linkedMapOf<String, Game>()
        games.forEach { game ->
            val key = getGameDeduplicationKey(game)
            val existing = uniqueGamesMap[key]
            if (existing == null) {
                uniqueGamesMap[key] = game
            } else {
                uniqueGamesMap[key] = selectBetterGame(existing, game)
            }
        }
        return uniqueGamesMap.values.toList()
    }

    fun restoreContentForGame(game: Game) {
        NativeLibrary.reloadKeys()

        val mountedContainerUris = mutableSetOf<String>()
        mountExternalContentDirectories(mountedContainerUris)
        mountGameFolderContent(Uri.parse(game.path), mountedContainerUris, force = true)
        NativeLibrary.addFileToFilesystemProvider(game.path)
    }

    // File extensions considered as external content, buuut should
    // be done better imo.
    private val externalContentExtensions = setOf("nsp", "xci", "nsz", "xcz")

    private fun scanContentContainersRecursive(
        files: Array<MinimalDocumentFile>,
        depth: Int,
        onContainerFound: (MinimalDocumentFile) -> Unit
    ) {
        if (depth <= 0) {
            return
        }

        files.forEach {
            if (it.isDirectory) {
                scanContentContainersRecursive(
                    FileUtil.listFiles(it.uri),
                    depth - 1,
                    onContainerFound
                )
            } else {
                val extension = if (it.filename.isNotEmpty() && it.filename.contains('.')) {
                    it.filename.substringAfterLast('.').lowercase()
                } else {
                    FileUtil.getExtension(it.uri).lowercase()
                }
                if (externalContentExtensions.contains(extension)) {
                    onContainerFound(it)
                }
            }
        }
    }

    private fun addGamesRecursive(
        games: MutableList<Game>,
        files: Array<MinimalDocumentFile>,
        depth: Int
    ) {
        if (depth <= 0) {
            return
        }

        files.forEach {
            if (it.isDirectory) {
                addGamesRecursive(
                    games,
                    FileUtil.listFiles(it.uri),
                    depth - 1
                )
            } else {
                val extension = if (it.filename.isNotEmpty() && it.filename.contains('.')) {
                    it.filename.substringAfterLast('.').lowercase()
                } else {
                    FileUtil.getExtension(it.uri).lowercase()
                }
                if (Game.extensions.contains(extension)) {
                    val game = getGame(it.uri, true, false)
                    if (game != null) {
                        games.add(game)
                    }
                }
            }
        }
    }

    private fun mountExternalContentDirectories(mountedContainerUris: MutableSet<String>) {
        val uniqueExternalContentDirs = linkedSetOf<String>()
        NativeConfig.getExternalContentDirs().forEach { externalDir ->
            if (externalDir.isNotEmpty()) {
                uniqueExternalContentDirs.add(externalDir)
            }
        }

        for (externalDir in uniqueExternalContentDirs) {
            val externalDirUri = externalDir.toUri()
            if (FileUtil.isTreeUriValid(externalDirUri)) {
                scanContentContainersRecursive(FileUtil.listFiles(externalDirUri), 3) {
                    val containerUri = it.uri.toString()
                    if (mountedContainerUris.add(containerUri)) {
                        NativeLibrary.addFileToFilesystemProvider(containerUri)
                    }
                }
            }
        }
    }

    private fun mountGameFolderContent(gameUri: Uri, mountedContainerUris: MutableSet<String>, force: Boolean = false) {
        if (!force && !BooleanSetting.EXT_CONTENT_FROM_GAME_DIRS.getBoolean()) {
            return
        }
        if (gameUri.scheme == "content") {
            val parentUri = getParentDocumentUri(gameUri) ?: return
            scanContentContainersRecursive(FileUtil.listFiles(parentUri), 1) {
                val containerUri = it.uri.toString()
                if (mountedContainerUris.add(containerUri)) {
                    NativeLibrary.addGameFolderFileToFilesystemProvider(containerUri)
                }
            }
            return
        }

        val gameFile = File(gameUri.path ?: gameUri.toString())
        val parentDir = gameFile.parentFile ?: return
        parentDir.listFiles()?.forEach { sibling ->
            if (!sibling.isFile) {
                return@forEach
            }

            val extension = sibling.extension.lowercase()
            if (externalContentExtensions.contains(extension)) {
                val containerUri = Uri.fromFile(sibling).toString()
                if (mountedContainerUris.add(containerUri)) {
                    NativeLibrary.addGameFolderFileToFilesystemProvider(containerUri)
                }
            }
        }
    }

    private fun getParentDocumentUri(uri: Uri): Uri? {
        return try {
            val documentId = DocumentsContract.getDocumentId(uri)
            val separatorIndex = documentId.lastIndexOf('/')
            if (separatorIndex == -1) {
                null
            } else {
                val parentDocumentId = documentId.substring(0, separatorIndex)
                DocumentsContract.buildDocumentUriUsingTree(uri, parentDocumentId)
            }
        } catch (_: Exception) {
            null
        }
    }

    fun cleanGameTitle(rawTitle: String): String {
        var clean = rawTitle.trim()
        clean = clean.replace(Regex("\\.(nsp|nsz|xci|xcz|zip|7z)$", RegexOption.IGNORE_CASE), "").trim()
        val firstBracket = clean.indexOfAny(charArrayOf('[', '('))
        if (firstBracket > 0) {
            clean = clean.substring(0, firstBracket).trim()
        } else if (firstBracket == 0) {
            clean = clean.replace(Regex("\\[.*?\\]"), " ").replace(Regex("\\(.*?\\)"), " ").trim()
        }
        clean = clean.replace(Regex("[-_:]+$"), "").trim()
        return clean.ifEmpty { rawTitle }
    }

    fun isUpdateOrDlcPath(str: String): Boolean {
        val lower = str.lowercase(Locale.ROOT)
        if (lower.contains("[upd") || lower.contains("(upd") ||
            lower.contains("[update") || lower.contains("(update") ||
            lower.contains("[dlc") || lower.contains("(dlc") ||
            lower.contains("[patch") || lower.contains("(patch")) {
            return true
        }
        val match = Regex("\\[0100[0-9a-f]{8}[0-9a-f]{3}800\\]", RegexOption.IGNORE_CASE).find(lower)
        if (match != null) {
            return true
        }
        return false
    }

    fun getGame(
        uri: Uri,
        addedToLibrary: Boolean = false,
        registerFilesystemProvider: Boolean = true
    ): Game? {
        val filePath = uri.toString()
        val filename = FileUtil.getFilename(uri)

        if (addedToLibrary && isUpdateOrDlcPath(filename)) {
            // If the filename clearly designates an update/DLC, double-check if it has base content
            if (!GameMetadata.isBaseGame(filePath)) {
                return null
            }
        }
        if (!GameMetadata.getIsValid(filePath)) {
            return null
        }
        if (!GameMetadata.isBaseGame(filePath)) {
            return null
        }

        if (registerFilesystemProvider) {
            // Needed to update installed content information
            NativeLibrary.addFileToFilesystemProvider(filePath)
        }

        val nacpTitle = GameMetadata.getTitle(filePath).trim()
        val useFilename = BooleanSetting.SHOW_FILENAME_AS_TITLE.getBoolean()
        val name = if (useFilename) {
            cleanGameTitle(filename)
        } else {
            if (nacpTitle.isNotEmpty()) nacpTitle else cleanGameTitle(filename)
        }

        var programId = GameMetadata.getProgramId(filePath)

        val pIdLong = programId.toULongOrNull(16)
        if (pIdLong != null && (pIdLong and 0xFFFuL) != 0uL) {
            return null // Exclude standalone Updates and DLCs from game list
        }

        // If the game's ID field is empty, use the filename without extension.
        if (programId.isEmpty()) {
            programId = if (filename.contains(".")) filename.substring(0, filename.lastIndexOf(".")) else filename
        }

        val rawVersion = GameMetadata.getVersion(filePath, false)
        val rawInternalVersion = GameMetadata.getInternalVersion(filePath)

        val decodedFilename = runCatching { Uri.decode(filename) }.getOrDefault(filename)
        val decodedFilePath = runCatching { Uri.decode(filePath) }.getOrDefault(filePath)
        val candidateNames = listOf(filename, decodedFilename, decodedFilePath, filePath)

        val (cleanVersion, cleanInternalVersion) = resolveVersionFromNames(
            candidateNames,
            rawVersion,
            rawInternalVersion
        )

        val addonCount = GameMetadata.getAddonCount(filePath)
        val finalAddonCount = if (addonCount > 0) {
            addonCount
        } else {
            cachedGameList.firstOrNull { it.path == filePath || it.programId == programId }?.addonCount ?: 0
        }

        var resolvedVersion = cleanVersion
        var resolvedInternalVersion = cleanInternalVersion
        var resolvedAddonCount = finalAddonCount

        if (isBaseVersion(resolvedVersion) || resolvedAddonCount == 0) {
            try {
                val catalog = org.yuzu.yuzu_emu.fragments.StormGamesWorldDialogFragment.getCachedCatalog(org.yuzu.yuzu_emu.YuzuApplication.appContext)
                val cleanHex = programId.toULongOrNull(16)?.let { String.format(java.util.Locale.ROOT, "%016X", it.toLong()) } ?: programId
                val matchedGame = catalog.firstOrNull { cat ->
                    (cat.serialId.isNotEmpty() && cat.serialId.equals(cleanHex, ignoreCase = true)) ||
                    (cat.finalTitle.isNotEmpty() && filename.contains(cat.finalTitle.replace(Regex("[\\\\/:*?\"<>|]"), "_").trim(), ignoreCase = true))
                }
                if (matchedGame != null) {
                    if (isBaseVersion(resolvedVersion) && matchedGame.version.isNotEmpty() && !isBaseVersion(matchedGame.version)) {
                        resolvedVersion = matchedGame.version
                        if (matchedGame.internalVersion.isNotEmpty()) {
                            resolvedInternalVersion = matchedGame.internalVersion
                        }
                    }
                    if (resolvedAddonCount == 0 && matchedGame.dlcCount > 0) {
                        resolvedAddonCount = matchedGame.dlcCount
                    }
                }
            } catch (_: Throwable) {}
        }

        val newGame = Game(
            name,
            filePath,
            programId,
            GameMetadata.getDeveloper(filePath),
            resolvedVersion,
            resolvedInternalVersion,
            GameMetadata.getIsHomebrew(filePath),
            resolvedAddonCount
        )


        if (addedToLibrary) {
            val addedTime = preferences.getLong(newGame.keyAddedToLibraryTime, 0L)
            if (addedTime == 0L) {
                preferences.edit()
                    .putLong(newGame.keyAddedToLibraryTime, System.currentTimeMillis())
                    .apply()
            }
        }

        return newGame
    }

    fun isBaseVersion(ver: String): Boolean {
        val v = ver.trim().removePrefix("v").removePrefix("V").trim()
        return v.isEmpty() || v == "0" || v == "1.0" || v == "1.0.0" || v == "1.0.0.0" || v.startsWith("0.")
    }

    fun resolveVersionFromNames(
        names: List<String>,
        currentVersion: String,
        currentInternalVersion: String
    ): Pair<String, String> {
        var cleanVersion = currentVersion.trim().removePrefix("v").removePrefix("V").trim()
        var cleanInternalVersion = currentInternalVersion.trim().removePrefix("v").removePrefix("V").trim()
        val parsedIntVer = cleanInternalVersion.toLongOrNull() ?: 0L

        // If version is base OR internal version is missing / corrupted (< 65536 for non-base), search filenames
        if (isBaseVersion(cleanVersion) || parsedIntVer < 65536L) {
            for (rawName in names) {
                if (rawName.isEmpty()) continue
                val name = rawName.substringAfterLast('/').substringAfterLast('\\')
                // 1. Paired version: e.g. "(1.0.9 - 458752 - 0100EC9010258000)" or "(4.2.1-b5628 - 0 - 01000B900D270000)" or "[1.0.9 - 458752]"
                val pairMatches = Regex("""[\[\(]([0-9]+(?:\.[0-9]+)+(?:[-_][a-zA-Z0-9_\.]+)*)\s*-\s*([0-9]+)(?:\s*-\s*[0-9A-Fa-f]+)?[\)\]]""", RegexOption.IGNORE_CASE).findAll(name)
                var foundPair = false
                for (pairMatch in pairMatches) {
                    val pVer = pairMatch.groupValues[1].trim()
                    val pIntVer = pairMatch.groupValues[2].trim()
                    if (pVer.isNotEmpty() && !isBaseVersion(pVer)) {
                        cleanVersion = pVer
                    }
                    if (pIntVer.isNotEmpty() && (cleanInternalVersion == "0" || cleanInternalVersion.isEmpty() || (cleanInternalVersion.toLongOrNull() ?: 0L) < 65536L)) {
                        val num = pIntVer.toLongOrNull() ?: 0L
                        if (num in 1..4294967295L) {
                            cleanInternalVersion = pIntVer
                        }
                    }
                    if (!isBaseVersion(cleanVersion) && (cleanInternalVersion.toLongOrNull() ?: 0L) >= 65536L) {
                        foundPair = true
                        break
                    }
                }
                if (foundPair) break

                // 2. Bracketed version: e.g. "[1.0.9]" or "(v4.2.1-b5628)" or "[Update 1.29.0]" or "[UPD 1.29.0]"
                if (isBaseVersion(cleanVersion)) {
                    val bracketMatches = Regex("""[\[\(](?:v|ver|upd|update)?\s*([0-9]+(?:\.[0-9]+)+(?:[-_][a-zA-Z0-9_\.]+)*)[\]\)]""", RegexOption.IGNORE_CASE).findAll(name)
                    for (bm in bracketMatches) {
                        val parsedVer = bm.groupValues[1].trim()
                        if (!isBaseVersion(parsedVer)) {
                            cleanVersion = parsedVer
                            break
                        }
                    }
                }

                // 3. Fallback standalone version pattern in filename
                if (isBaseVersion(cleanVersion)) {
                    val standaloneMatches = Regex("""(?:^|[\s_-\[\(])(?:v|ver|upd|update)?([0-9]+(?:\.[0-9]+)+(?:[-_][a-zA-Z0-9_\.]+)*)(?:[\s_-\]\)]|$)""", RegexOption.IGNORE_CASE).findAll(name)
                    for (sm in standaloneMatches) {
                        val parsedVer = sm.groupValues[1].trim()
                        if (!isBaseVersion(parsedVer)) {
                            cleanVersion = parsedVer
                            break
                        }
                    }
                }
            }
        }

        val currInt = cleanInternalVersion.toLongOrNull() ?: 0L
        if (currInt < 65536L) {
            for (rawName in names) {
                if (rawName.isEmpty()) continue
                val name = rawName.substringAfterLast('/').substringAfterLast('\\')
                // Require 5-9 digits to prevent matching 16-hex Title IDs!
                val intMatches = Regex("""[\[\(](?:v|ver|upd|update)?\s*(\d{5,9})[\]\)]""", RegexOption.IGNORE_CASE).findAll(name)
                for (match in intMatches) {
                    val num = match.groupValues[1].toLongOrNull() ?: 0L
                    if (num in 1..4294967295L) {
                        cleanInternalVersion = match.groupValues[1]
                        break
                    }
                }
                if ((cleanInternalVersion.toLongOrNull() ?: 0L) >= 65536L) break
            }
        }

        if (isBaseVersion(cleanVersion) && cleanInternalVersion.isNotEmpty() && cleanInternalVersion != "0") {
            val numVer = cleanInternalVersion.toLongOrNull() ?: 0L
            if (numVer > 0L) {
                val upd = numVer / 65536L
                if (upd > 0L) {
                    cleanVersion = "1.0.$upd"
                }
            }
        }

        val currentNum = cleanInternalVersion.toLongOrNull() ?: 0L
        if ((currentNum == 0L || currentNum < 65536L) && !isBaseVersion(cleanVersion)) {
            val parts = cleanVersion.split('.').mapNotNull { it.filter { c -> c.isDigit() }.toIntOrNull() }
            if (parts.size >= 4 && parts[0] == 1 && parts[1] == 0 && parts[2] == 0 && parts[3] > 0) {
                cleanInternalVersion = (parts[3] * 65536).toString()
            } else if (parts.size >= 3 && parts[0] == 1 && parts[1] == 0 && parts[2] > 0) {
                cleanInternalVersion = (parts[2] * 65536).toString()
            } else if (parts.size >= 2 && parts[0] == 1 && parts[1] > 0) {
                cleanInternalVersion = (parts[1] * 65536).toString()
            } else if (parts.isNotEmpty()) {
                val lastVal = parts.last()
                if (lastVal > 0) {
                    cleanInternalVersion = (lastVal * 65536).toString()
                }
            }
        }

        if (cleanVersion.isEmpty()) {
            cleanVersion = "1.0.0"
        }
        if (cleanInternalVersion.isEmpty()) {
            cleanInternalVersion = "0"
        }

        return Pair(cleanVersion, cleanInternalVersion)
    }

    fun upgradeGameVersionIfNeeded(game: Game): Game {
        val intVer = game.internalVersion.toLongOrNull() ?: 0L
        // Only skip if game has a real non-base internal version (>= 65536) and a valid non-base display version
        if (!isBaseVersion(game.version) && intVer >= 65536L) {
            return game
        }
        val filename = try {
            FileUtil.getFilename(game.path.toUri())
        } catch (_: Exception) {
            ""
        }
        val decodedFilename = runCatching { Uri.decode(filename) }.getOrDefault(filename)
        val decodedFilePath = runCatching { Uri.decode(game.path) }.getOrDefault(game.path)
        val candidateNames = listOf(filename, decodedFilename, decodedFilePath, game.path)

        // Try getting fresh internal version from native metadata if currently 0 or corrupted (< 65536)
        val rawNativeInternal = if (intVer < 65536L) {
            runCatching { GameMetadata.getInternalVersion(game.path) }.getOrDefault("")
        } else {
            game.internalVersion
        }

        val (upgradedVersion, upgradedInternalVersion) = resolveVersionFromNames(
            candidateNames,
            game.version,
            rawNativeInternal
        )

        game.version = upgradedVersion
        game.internalVersion = upgradedInternalVersion
        return game
    }
}
