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
                    } catch (e: Exception) {
                        Log.error("[GameHelper] Error decoding cached game: ${e.message}")
                    }
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
                } catch (e: Exception) {
                    Log.error("[GameHelper] Error decoding stored game: ${e.message}")
                }
            }
        }

        // Ensure keys are loaded so that ROM metadata can be decrypted.
        NativeLibrary.reloadKeys()

        // Reset metadata so we don't use stale information only when cache is empty
        if (cachedGameList.isEmpty()) {
            GameMetadata.resetMetadata()
            NativeLibrary.clearFilesystemProvider()
        }

        val mountedContainerUris = mutableSetOf<String>()
        mountExternalContentDirectories(mountedContainerUris)

        val discoveredUpdateVersionsByPid = mutableMapOf<String, Pair<String, String>>()
        val discoveredUpdateVersionsByTitle = mutableMapOf<String, Pair<String, String>>()

        fun recordDiscoveredVersion(rawName: String, rawPath: String) {
            val (ver, intVer) = resolveVersionFromNames(listOf(rawName, rawPath), "1.0.0", "0")
            if (isBaseVersion(ver) && (intVer.toLongOrNull() ?: 0L) <= 0L) return
            val idMatch = Regex("0100[0-9a-fA-F]{12}").find(rawName) ?: Regex("0100[0-9a-fA-F]{12}").find(rawPath)
            if (idMatch != null) {
                val parsed = idMatch.value.toULongOrNull(16)
                if (parsed != null && parsed != 0uL) {
                    val baseHex = String.format(Locale.ROOT, "%016X", (parsed and 0xFFFFFFFFFFFFE000uL).toLong())
                    val prev = discoveredUpdateVersionsByPid[baseHex]
                    if (prev == null || compareVersionStrings(ver, intVer, prev.first, prev.second) > 0) {
                        discoveredUpdateVersionsByPid[baseHex] = Pair(ver, intVer)
                    }
                }
            }
            val cleanT = cleanGameTitle(rawName).lowercase(Locale.ROOT).trim()
            if (cleanT.isNotEmpty()) {
                val prevT = discoveredUpdateVersionsByTitle[cleanT]
                if (prevT == null || compareVersionStrings(ver, intVer, prevT.first, prevT.second) > 0) {
                    discoveredUpdateVersionsByTitle[cleanT] = Pair(ver, intVer)
                }
            }
        }

        // Also harvest any versions from previously cached entries before filtering out standalone updates/DLCs
        cachedGameList.forEach { cg ->
            recordDiscoveredVersion(cg.title, cg.path)
            val pid = getBaseProgramIdHex(cg.programIdHex)
            if (pid != "0") {
                val prev = discoveredUpdateVersionsByPid[pid]
                if (prev == null || compareVersionStrings(cg.version, cg.internalVersion, prev.first, prev.second) > 0) {
                    discoveredUpdateVersionsByPid[pid] = Pair(cg.version, cg.internalVersion)
                }
            }
        }

        // Pass 1: Mount all external content containers (.nsp, .nsz, .xci, .xcz) across all gameDirs FIRST
        // so ContentProvider knows about 100% of Updates and DLCs before Pass 2 queries GameMetadata!
        val dirFilesCache = mutableListOf<Pair<Array<MinimalDocumentFile>, Int>>()
        gameDirs.forEach { gameDir ->
            val gameDirUri = gameDir.uriString.toUri()
            if (FileUtil.isTreeUriValid(gameDirUri)) {
                val scanDepth = if (gameDir.deepScan) 7 else 5
                val topFiles = FileUtil.listFiles(gameDirUri)
                dirFilesCache.add(Pair(topFiles, scanDepth))
                scanContentContainersRecursive(topFiles, scanDepth) { containerFile ->
                    val containerUri = containerFile.uri.toString()
                    recordDiscoveredVersion(containerFile.filename, containerUri)
                    if (mountedContainerUris.add(containerUri)) {
                        NativeLibrary.addGameFolderFileToFilesystemProvider(containerUri)
                    }
                }
            }
        }

        val cachedMapByPath = cachedGameList.associateBy { it.path }
        val cachedMapByName = cachedGameList.groupBy {
            try { FileUtil.getFilename(it.path.toUri()) } catch (e: Exception) {
                Log.error("[GameHelper] Error extracting filename for ${it.path}: ${e.message}")
                ""
            }
        }.filter { it.key.isNotEmpty() && it.value.size == 1 }
         .mapValues { it.value.first() }

        // Pass 2: Discover bootable base games (excluding standalone updates and DLCs)
        dirFilesCache.forEach { (topFiles, scanDepth) ->
            scanAndAddGamesRecursive(
                games,
                topFiles,
                scanDepth,
                mountedContainerUris,
                cachedMapByPath,
                cachedMapByName
            )
        }

        val finalGames = deduplicateGames(games)

        // Upgrade and sanitize versions and addon counts for all deduplicated games
        finalGames.forEach { game ->
            upgradeGameVersionIfNeeded(game)
            val freshAddons = runCatching { GameMetadata.getAddonCount(game.path) }.getOrDefault(0)
            if (freshAddons > game.addonCount) {
                game.addonCount = freshAddons
            }
            val freshVer = runCatching { GameMetadata.getVersion(game.path, true) }.getOrDefault("")
            val freshIntVer = runCatching { GameMetadata.getInternalVersion(game.path) }.getOrDefault("")
            if (compareVersionStrings(freshVer, freshIntVer, game.version, game.internalVersion) > 0) {
                if (!isBaseVersion(freshVer)) game.version = freshVer
                if ((freshIntVer.toLongOrNull() ?: 0L) > (game.internalVersion.toLongOrNull() ?: 0L)) {
                    game.internalVersion = freshIntVer
                }
            }
            val basePid = getBaseProgramIdHex(game.programIdHex)
            val discPid = discoveredUpdateVersionsByPid[basePid]
            if (discPid != null && compareVersionStrings(discPid.first, discPid.second, game.version, game.internalVersion) > 0) {
                if (!isBaseVersion(discPid.first)) game.version = discPid.first
                if ((discPid.second.toLongOrNull() ?: 0L) > (game.internalVersion.toLongOrNull() ?: 0L)) {
                    game.internalVersion = discPid.second
                }
            }
            val cleanT = cleanGameTitle(game.title).lowercase(Locale.ROOT).trim()
            val discTitle = discoveredUpdateVersionsByTitle[cleanT]
            if (discTitle != null && compareVersionStrings(discTitle.first, discTitle.second, game.version, game.internalVersion) > 0) {
                if (!isBaseVersion(discTitle.first)) game.version = discTitle.first
                if ((discTitle.second.toLongOrNull() ?: 0L) > (game.internalVersion.toLongOrNull() ?: 0L)) {
                    game.internalVersion = discTitle.second
                }
            }
        }

        // Preserve higher addon counts and versions from cached metadata across any same-ProgramID entry
        if (cachedGameList.isNotEmpty()) {
            finalGames.forEach { game ->
                val basePid = getBaseProgramIdHex(game.programIdHex)
                cachedGameList.forEach { cached ->
                    if (getBaseProgramIdHex(cached.programIdHex) == basePid && basePid != "0") {
                        if (cached.addonCount > game.addonCount) {
                            game.addonCount = cached.addonCount
                        }
                        if (compareVersionStrings(cached.version, cached.internalVersion, game.version, game.internalVersion) > 0) {
                            if (!isBaseVersion(cached.version)) {
                                game.version = cached.version
                            }
                            val cachedIv = cached.internalVersion.toLongOrNull() ?: 0L
                            val currentIv = game.internalVersion.toLongOrNull() ?: 0L
                            if (cachedIv > currentIv) {
                                game.internalVersion = cached.internalVersion
                            }
                        }
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
                        val localPath = if (uri.scheme == "file") uri.path ?: "" else game.path
                        File(localPath).exists() || File(Uri.decode(localPath)).exists()
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

    fun getBaseProgramIdHex(rawHex: String): String {
        val clean = rawHex.trim().uppercase(Locale.ROOT)
        if (clean.isEmpty() || clean == "0" || clean == "0000000000000000") return "0"
        val parsed = clean.toULongOrNull(16)
        return if (parsed != null && parsed != 0uL) {
            String.format(Locale.ROOT, "%016X", (parsed and 0xFFFFFFFFFFFFE000uL).toLong())
        } else {
            clean
        }
    }

    fun compareVersionStrings(verA: String, intVerA: String, verB: String, intVerB: String): Int {
        val ivA = intVerA.trim().toLongOrNull() ?: 0L
        val ivB = intVerB.trim().toLongOrNull() ?: 0L
        if (ivA != ivB && (ivA >= 65536L || ivB >= 65536L)) {
            return ivA.compareTo(ivB)
        }
        val cleanA = verA.trim().removePrefix("v").removePrefix("V").trim()
        val cleanB = verB.trim().removePrefix("v").removePrefix("V").trim()
        val baseA = isBaseVersion(cleanA)
        val baseB = isBaseVersion(cleanB)
        if (!baseA && baseB) return 1
        if (baseA && !baseB) return -1
        val partsA = Regex("\\d+").findAll(cleanA).map { it.value.toIntOrNull() ?: 0 }.toList()
        val partsB = Regex("\\d+").findAll(cleanB).map { it.value.toIntOrNull() ?: 0 }.toList()
        val maxLen = maxOf(partsA.size, partsB.size)
        for (i in 0 until maxLen) {
            val a = if (i < partsA.size) partsA[i] else 0
            val b = if (i < partsB.size) partsB[i] else 0
            if (a != b) return a.compareTo(b)
        }
        return ivA.compareTo(ivB)
    }

    fun getGameDeduplicationKey(game: Game): String {
        val ext = game.extension.uppercase(Locale.ROOT).ifEmpty { "NSP" }
        val basePid = getBaseProgramIdHex(game.programIdHex)
        if (basePid != "0" && basePid.isNotEmpty()) {
            return "PID_${basePid}_FMT_${ext}"
        }
        val cleanTitle = cleanGameTitle(game.title).lowercase(Locale.ROOT).trim()
        if (cleanTitle.isNotEmpty() && cleanTitle != "homebrew") {
            return "TITLE_${cleanTitle}_FMT_${ext}"
        }
        return "${game.path}_FMT_${ext}"
    }

    fun selectBetterGame(existing: Game, candidate: Game): Game {
        // Compute the highest version, internalVersion, and addonCount across both entries
        val bestVerPair = if (compareVersionStrings(candidate.version, candidate.internalVersion, existing.version, existing.internalVersion) > 0) {
            Pair(
                if (!isBaseVersion(candidate.version)) candidate.version else existing.version,
                if ((candidate.internalVersion.toLongOrNull() ?: 0L) >= (existing.internalVersion.toLongOrNull() ?: 0L)) candidate.internalVersion else existing.internalVersion
            )
        } else {
            Pair(
                if (!isBaseVersion(existing.version)) existing.version else candidate.version,
                if ((existing.internalVersion.toLongOrNull() ?: 0L) >= (candidate.internalVersion.toLongOrNull() ?: 0L)) existing.internalVersion else candidate.internalVersion
            )
        }
        val bestAddonCount = maxOf(existing.addonCount, candidate.addonCount)

        // Choose which entry provides the bootable base game path
        val existingIsUpdate = isUpdateOrDlcPath(existing.path) || isUpdateOrDlcPath(existing.title)
        val candidateIsUpdate = isUpdateOrDlcPath(candidate.path) || isUpdateOrDlcPath(candidate.title)

        val winner = when {
            existingIsUpdate && !candidateIsUpdate -> candidate
            !existingIsUpdate && candidateIsUpdate -> existing
            else -> {
                val cmp = compareVersionStrings(candidate.version, candidate.internalVersion, existing.version, existing.internalVersion)
                if (cmp > 0) candidate else existing
            }
        }

        winner.version = bestVerPair.first
        winner.internalVersion = bestVerPair.second
        winner.addonCount = bestAddonCount
        return winner
    }

    fun deduplicateGames(games: List<Game>): List<Game> {
        val uniqueByPidMap = linkedMapOf<String, Game>()
        games.forEach { game ->
            val key = getGameDeduplicationKey(game)
            val existing = uniqueByPidMap[key]
            if (existing == null) {
                uniqueByPidMap[key] = game
            } else {
                uniqueByPidMap[key] = selectBetterGame(existing, game)
            }
        }
        val uniqueByTitleAndFormat = linkedMapOf<String, Game>()
        uniqueByPidMap.values.forEach { game ->
            val ext = game.extension.uppercase(Locale.ROOT).ifEmpty { "NSP" }
            val cleanTitle = cleanGameTitle(game.title).lowercase(Locale.ROOT).trim()
            val titleKey = if (cleanTitle.isNotEmpty() && cleanTitle != "homebrew") {
                "TITLE_${cleanTitle}_FMT_${ext}"
            } else {
                getGameDeduplicationKey(game)
            }
            val existing = uniqueByTitleAndFormat[titleKey]
            if (existing == null) {
                uniqueByTitleAndFormat[titleKey] = game
            } else {
                uniqueByTitleAndFormat[titleKey] = selectBetterGame(existing, game)
            }
        }
        return uniqueByTitleAndFormat.values.filterNot { g ->
            val ext = g.extension.lowercase(Locale.ROOT)
            val isCart = ext == "xci" || ext == "xcz"
            !isCart && (isUpdateOrDlcPath(g.path) || isUpdateOrDlcPath(g.title)) &&
                !runCatching { GameMetadata.isBaseGame(g.path) }.getOrDefault(false)
        }
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
                    try {
                        onContainerFound(it)
                    } catch (e: Throwable) {
                        Log.error("[GameHelper] Failed to mount container ${it.filename}: ${e.message}")
                    }
                }
            }
        }
    }

    private fun scanAndAddGamesRecursive(
        games: MutableList<Game>,
        files: Array<MinimalDocumentFile>,
        depth: Int,
        mountedContainerUris: MutableSet<String>,
        cachedMapByPath: Map<String, Game>,
        cachedMapByName: Map<String, Game>
    ) {
        if (depth <= 0) {
            return
        }

        for (it in files) {
            if (it.isDirectory) {
                scanAndAddGamesRecursive(
                    games,
                    FileUtil.listFiles(it.uri),
                    depth - 1,
                    mountedContainerUris,
                    cachedMapByPath,
                    cachedMapByName
                )
            } else {
                val extension = if (it.filename.isNotEmpty() && it.filename.contains('.')) {
                    it.filename.substringAfterLast('.').lowercase()
                } else {
                    FileUtil.getExtension(it.uri).lowercase()
                }
                val filePath = it.uri.toString()

                if (externalContentExtensions.contains(extension)) {
                    if (mountedContainerUris.add(filePath)) {
                        try {
                            NativeLibrary.addGameFolderFileToFilesystemProvider(filePath)
                        } catch (e: Throwable) {
                            Log.error("[GameHelper] Failed to mount container ${it.filename}: ${e.message}")
                        }
                    }
                }

                if (Game.extensions.contains(extension)) {
                    try {
                        val cached = cachedMapByPath[filePath] ?: cachedMapByName[it.filename]?.let { old ->
                            if (old.path != filePath) {
                                Game(
                                    title = old.title,
                                    path = filePath,
                                    programId = old.programId,
                                    developer = old.developer,
                                    version = old.version,
                                    internalVersion = old.internalVersion,
                                    isHomebrew = old.isHomebrew,
                                    addonCount = old.addonCount
                                )
                            } else old
                        }
                        val isCartridge = extension == "xci" || extension == "xcz"
                        if (cached != null) {
                            if (isCartridge || (!isUpdateOrDlcPath(filePath) && !isUpdateOrDlcPath(it.filename) && GameMetadata.isBaseGame(filePath))) {
                                games.add(cached)
                            }
                        } else {
                            val game = getGame(it.uri, true, false)
                            if (game != null) {
                                games.add(game)
                            }
                        }
                    } catch (e: Throwable) {
                        Log.error("[GameHelper] Failed to parse game ${it.filename}: ${e.message}")
                    }
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
                    try {
                        val game = getGame(it.uri, true, false)
                        if (game != null) {
                            games.add(game)
                        }
                    } catch (e: Throwable) {
                        Log.error("[GameHelper] Failed to parse game ${it.filename}: ${e.message}")
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
        // Cartridge dumps (.xci, .xcz) are always full games, never standalone updates/DLCs
        if (lower.endsWith(".xci") || lower.endsWith(".xcz")) {
            return false
        }
        val leaf = lower.substringAfterLast('/').substringAfterLast('\\')
        if (leaf.contains("[upd") || leaf.contains("(upd") ||
            leaf.contains("[update") || leaf.contains("(update") ||
            leaf.contains("[dlc") || leaf.contains("(dlc") ||
            leaf.contains("[patch") || leaf.contains("(patch")) {
            return true
        }
        // Any 16-hex Switch Title ID in the leaf filename whose last 3 hex digits are NOT 000
        // is an Update (800) or DLC (001..7FF), unless the filename explicitly marks a merged bundle (1G+)
        if (!leaf.contains("1g+")) {
            val tidMatches = Regex("0100[0-9a-f]{12}").findAll(leaf).toList()
            if (tidMatches.isNotEmpty() && tidMatches.none { it.value.endsWith("000") }) {
                return true
            }
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
        val isCartridge = filename.endsWith(".xci", ignoreCase = true) || filename.endsWith(".xcz", ignoreCase = true)

        if (addedToLibrary && isUpdateOrDlcPath(filename)) {
            // If the filename clearly designates an update/DLC, double-check if it has base content
            if (!isCartridge && !GameMetadata.isBaseGame(filePath)) {
                return null
            }
        }
        if (!GameMetadata.getIsValid(filePath)) {
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
        if (!isCartridge && pIdLong != null && pIdLong != 0uL && (pIdLong and 0xFFFuL) != 0uL) {
            return null // Exclude standalone Updates and DLCs from game list
        }

        // If the game's ID field is empty or zero, extract from filename or use filename without extension
        if (programId.isEmpty() || programId == "0000000000000000" || programId == "0") {
            val idMatch = Regex("""0100[0-9a-fA-F]{12}""").find(filename)
            programId = if (idMatch != null) {
                idMatch.value.uppercase(Locale.ROOT)
            } else if (filename.contains(".")) {
                filename.substring(0, filename.lastIndexOf("."))
            } else {
                filename
            }
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
                    val standaloneMatches = Regex("""(?:^|[-\s_\[\(])(?:v|ver|upd|update)?([0-9]+(?:\.[0-9]+)+(?:[-_][a-zA-Z0-9_\.]+)*)(?:[-\s_\]\)]|$)""", RegexOption.IGNORE_CASE).findAll(name)
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
