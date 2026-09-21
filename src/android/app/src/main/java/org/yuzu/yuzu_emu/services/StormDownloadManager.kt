// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.services

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Environment
import androidx.core.content.ContextCompat
import androidx.documentfile.provider.DocumentFile
import androidx.preference.PreferenceManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import okhttp3.Call
import okhttp3.ConnectionPool
import okhttp3.Dispatcher
import okhttp3.OkHttpClient
import okhttp3.Protocol
import okhttp3.Request
import org.yuzu.yuzu_emu.fragments.StormWorldGameItem
import org.yuzu.yuzu_emu.utils.FileUtil
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.NativeConfig
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream
import java.text.DecimalFormat
import java.text.DecimalFormatSymbols
import java.util.Locale
import java.util.concurrent.TimeUnit

enum class StormDownloadStatus {
    IDLE,
    CONNECTING,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    ERROR,
    CANCELLED
}

data class StormDownloadProgress(
    val game: StormWorldGameItem,
    val status: StormDownloadStatus,
    val progressPercent: Int = 0,
    val downloadedBytes: Long = 0L,
    val totalBytes: Long = 0L,
    val speedMbps: Double = 0.0,
    val etaSeconds: Int = 0,
    val statsText: String = "",
    val errorMessage: String = ""
)

object StormDownloadManager {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private var downloadJob: Job? = null
    private var activeCall: Call? = null

    private val _state = MutableStateFlow<StormDownloadProgress?>(null)
    val state: StateFlow<StormDownloadProgress?> = _state.asStateFlow()

    @Volatile
    var isPaused: Boolean = false
        private set

    @Volatile
    var isCancelled: Boolean = false
        private set

    val httpClient: OkHttpClient by lazy {
        val dispatcher = Dispatcher().apply {
            maxRequests = 64
            maxRequestsPerHost = 16
        }
        val pool = ConnectionPool(32, 5, TimeUnit.MINUTES)
        OkHttpClient.Builder()
            .dispatcher(dispatcher)
            .connectionPool(pool)
            .protocols(listOf(Protocol.HTTP_2, Protocol.HTTP_1_1))
            .connectTimeout(30, TimeUnit.SECONDS)
            .readTimeout(60, TimeUnit.SECONDS)
            .writeTimeout(60, TimeUnit.SECONDS)
            .retryOnConnectionFailure(true)
            .build()
    }

    private val decimalFormat = DecimalFormat("#,##0.0", DecimalFormatSymbols(Locale("ru", "RU")).apply {
        groupingSeparator = ' '
        decimalSeparator = ','
    })

    fun isDownloading(): Boolean {
        val current = _state.value ?: return false
        return current.status == StormDownloadStatus.CONNECTING ||
               current.status == StormDownloadStatus.DOWNLOADING
    }

    fun getCurrentGame(): StormWorldGameItem? = _state.value?.game

    fun startDownload(context: Context, game: StormWorldGameItem) {
        if (isDownloading()) return

        isCancelled = false
        isPaused = false

        val initialProgress = StormDownloadProgress(
            game = game,
            status = StormDownloadStatus.CONNECTING,
            statsText = "Подключение к серверу загрузки..."
        )
        _state.value = initialProgress

        // Start Foreground Service so Android won't kill download in background
        val serviceIntent = Intent(context, StormDownloadService::class.java).apply {
            action = StormDownloadService.ACTION_START
            putExtra(StormDownloadService.EXTRA_GAME_TITLE, game.finalTitle.ifEmpty { game.title })
        }
        try {
            ContextCompat.startForegroundService(context, serviceIntent)
        } catch (e: Exception) {
            Log.error("[StormDownloadManager] Failed to start foreground service: ${e.message}")
        }

        downloadJob?.cancel()
        downloadJob = scope.launch {
            runDownloadLoop(context.applicationContext, game)
        }
    }

    fun pauseDownload(context: Context) {
        if (!isDownloading()) return
        isPaused = true
        activeCall?.cancel()
        activeCall = null

        val curr = _state.value
        if (curr != null) {
            _state.value = curr.copy(
                status = StormDownloadStatus.PAUSED,
                statsText = "⏸ Загрузка приостановлена"
            )
        }

        val serviceIntent = Intent(context, StormDownloadService::class.java).apply {
            action = StormDownloadService.ACTION_UPDATE
            putExtra(StormDownloadService.EXTRA_IS_PAUSED, true)
        }
        try {
            context.startService(serviceIntent)
        } catch (_: Exception) {}
    }

    fun resumeDownload(context: Context) {
        val curr = _state.value ?: return
        if (curr.status != StormDownloadStatus.PAUSED) return
        startDownload(context, curr.game)
    }

    fun togglePause(context: Context) {
        if (isPaused) {
            resumeDownload(context)
        } else {
            pauseDownload(context)
        }
    }

    fun cancelDownload(context: Context? = null) {
        isCancelled = true
        isPaused = false
        activeCall?.cancel()
        activeCall = null
        downloadJob?.cancel()

        val curr = _state.value
        if (curr != null) {
            _state.value = curr.copy(
                status = StormDownloadStatus.CANCELLED,
                statsText = "Загрузка отменена"
            )
        }

        context?.let { ctx ->
            val serviceIntent = Intent(ctx, StormDownloadService::class.java).apply {
                action = StormDownloadService.ACTION_STOP
            }
            try {
                ctx.startService(serviceIntent)
            } catch (_: Exception) {}
        }
    }

    const val PREF_CUSTOM_DOWNLOAD_DIR = "storm_world_custom_download_dir"

    sealed class TargetStorage {
        data class Saf(val docTree: DocumentFile, val uri: Uri) : TargetStorage()
        data class RegularFile(val folder: File) : TargetStorage()
    }

    data class StreamTarget(
        var outputStream: OutputStream,
        var existingBytes: Long,
        val safTree: DocumentFile?,
        val safDocFile: DocumentFile?,
        val normalFile: File?,
        val isSaf: Boolean,
        val targetDescription: String
    )

    private fun tryOpenStream(
        appContext: Context,
        target: TargetStorage,
        partFilename: String
    ): StreamTarget? {
        return try {
            when (target) {
                is TargetStorage.Saf -> {
                    val safTree = target.docTree
                    if (!safTree.exists() || !safTree.canWrite()) {
                        Log.warning("[StormDownloadManager] SAF directory not writable or missing: ${target.uri}")
                        return null
                    }
                    var targetDocPart = safTree.findFile(partFilename)
                    var existingBytes = 0L
                    var outputStream: OutputStream? = null

                    if (targetDocPart != null && targetDocPart.exists()) {
                        existingBytes = targetDocPart.length()
                        outputStream = appContext.contentResolver.openOutputStream(targetDocPart.uri, "wa")
                    }
                    if (outputStream == null) {
                        if (targetDocPart == null || !targetDocPart.exists()) {
                            targetDocPart = safTree.createFile("application/octet-stream", partFilename)
                        }
                        if (targetDocPart != null && targetDocPart.exists()) {
                            outputStream = appContext.contentResolver.openOutputStream(targetDocPart.uri, "w")
                            existingBytes = 0L
                        }
                    }

                    if (outputStream != null && targetDocPart != null) {
                        StreamTarget(
                            outputStream = outputStream,
                            existingBytes = existingBytes,
                            safTree = safTree,
                            safDocFile = targetDocPart,
                            normalFile = null,
                            isSaf = true,
                            targetDescription = safTree.name ?: "SAF"
                        )
                    } else {
                        Log.warning("[StormDownloadManager] Could not open SAF stream in ${safTree.name}")
                        null
                    }
                }
                is TargetStorage.RegularFile -> {
                    val folder = target.folder
                    if (!folder.exists()) {
                        folder.mkdirs()
                    }
                    if (!folder.exists() || !folder.canWrite()) {
                        Log.warning("[StormDownloadManager] Directory not writable: ${folder.absolutePath}")
                        return null
                    }
                    val normalFile = File(folder, partFilename)
                    val existingBytes = if (normalFile.exists()) normalFile.length() else 0L
                    val outputStream = FileOutputStream(normalFile, true)
                    StreamTarget(
                        outputStream = outputStream,
                        existingBytes = existingBytes,
                        safTree = null,
                        safDocFile = null,
                        normalFile = normalFile,
                        isSaf = false,
                        targetDescription = folder.name
                    )
                }
            }
        } catch (e: Exception) {
            Log.error("[StormDownloadManager] tryOpenStream failed: ${e.message}")
            null
        }
    }

    fun getCandidateTargets(context: Context): List<TargetStorage> {
        val candidates = mutableListOf<TargetStorage>()
        val prefs = PreferenceManager.getDefaultSharedPreferences(context)

        // 1. User-selected custom download directory
        val customUriStr = prefs.getString(PREF_CUSTOM_DOWNLOAD_DIR, null)
        if (!customUriStr.isNullOrBlank()) {
            try {
                val uri = Uri.parse(customUriStr)
                if (uri.scheme == "content") {
                    val tree = DocumentFile.fromTreeUri(context, uri)
                    if (tree != null && tree.exists() && tree.canWrite()) {
                        candidates.add(TargetStorage.Saf(tree, uri))
                    }
                } else {
                    val path = uri.path ?: customUriStr
                    val f = File(path)
                    candidates.add(TargetStorage.RegularFile(f))
                }
            } catch (e: Exception) {
                Log.error("[StormDownloadManager] Error resolving custom download dir: ${e.message}")
            }
        }

        // 2. Configured game directories (prefer actual games directories over mod/cheat/patch paths)
        val gameDirs = NativeConfig.getGameDirs().filter { it.uriString.isNotBlank() }
        val sortedDirs = gameDirs.sortedBy { dir ->
            val s = dir.uriString.lowercase(Locale.ROOT)
            if (s.contains("mod") || s.contains("cheat") || s.contains("60fps") || s.contains("patch")) 1 else 0
        }

        for (dir in sortedDirs) {
            try {
                val uri = Uri.parse(dir.uriString)
                if (uri.scheme == "content") {
                    val tree = DocumentFile.fromTreeUri(context, uri)
                    if (tree != null && tree.exists() && tree.canWrite()) {
                        candidates.add(TargetStorage.Saf(tree, uri))
                    }
                } else {
                    val path = uri.path ?: dir.uriString
                    val f = File(path)
                    candidates.add(TargetStorage.RegularFile(f))
                }
            } catch (_: Exception) {}
        }

        // 3. Standard public Downloads/STORM_SWITCH_GAMES
        try {
            val pubDir = File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "STORM_SWITCH_GAMES"
            )
            candidates.add(TargetStorage.RegularFile(pubDir))
        } catch (_: Exception) {}

        // 4. Guaranteed app-specific external storage (always 100% accessible on all Android versions)
        val appFiles = context.getExternalFilesDir("games") ?: File(context.filesDir, "games")
        candidates.add(TargetStorage.RegularFile(appFiles))

        return candidates
    }

    private suspend fun runDownloadLoop(appContext: Context, game: StormWorldGameItem) {
        val cleanTitle = (if (game.finalTitle.isNotEmpty()) game.finalTitle else game.title)
            .replace(Regex("[\\\\/:*?\"<>|]"), "_")
            .trim()
        val baseFilename = if (cleanTitle.endsWith(".nsp", ignoreCase = true) ||
            cleanTitle.endsWith(".xci", ignoreCase = true) ||
            cleanTitle.endsWith(".nsz", ignoreCase = true)) {
            cleanTitle
        } else {
            "$cleanTitle${game.realExtension}"
        }
        val partFilename = "$baseFilename.part"

        var activeTarget: StreamTarget? = null
        for (candidate in getCandidateTargets(appContext)) {
            val st = tryOpenStream(appContext, candidate, partFilename)
            if (st != null) {
                activeTarget = st
                break
            }
        }

        if (activeTarget == null) {
            throw RuntimeException("Не удалось открыть каталог для сохранения. Выберите доступную папку внизу карточки игры.")
        }

        var retryCount = 0
        val maxRetries = 10
        var completed = false

        while (retryCount <= maxRetries && !isCancelled && !isPaused && !completed) {
            var inputStream: InputStream? = null
            var bufferedOut: BufferedOutputStream? = null

            try {
                val currentTarget = activeTarget ?: run {
                    for (candidate in getCandidateTargets(appContext)) {
                        val st = tryOpenStream(appContext, candidate, partFilename)
                        if (st != null) {
                            activeTarget = st
                            break
                        }
                    }
                    activeTarget ?: throw RuntimeException("Не удалось открыть целевой файл для записи")
                }

                var existingBytes = currentTarget.existingBytes

                val downloadUrl = "https://stormgamesworld.ru/api/games/${game.id}/download"
                val reqBuilder = Request.Builder()
                    .url(downloadUrl)
                    .header("User-Agent", "STORM_SWITCH/9.1.0 (Android)")

                if (existingBytes > 0L) {
                    reqBuilder.header("Range", "bytes=$existingBytes-")
                }

                val req = reqBuilder.build()
                val call = httpClient.newCall(req)
                activeCall = call

                val resumeMsg = if (existingBytes > 0L) {
                    val mb = existingBytes / (1024 * 1024)
                    "Возобновление загрузки с ${mb} МБ (${currentTarget.targetDescription})..."
                } else {
                    "Подключение к серверу загрузки (${currentTarget.targetDescription})..."
                }
                _state.value = _state.value?.copy(
                    status = StormDownloadStatus.DOWNLOADING,
                    downloadedBytes = existingBytes,
                    statsText = resumeMsg
                )

                val resp = call.execute()

                if (resp.code == 416) {
                    try { currentTarget.outputStream.close() } catch (_: Exception) {}
                    completed = true
                    break
                }

                if (!resp.isSuccessful) {
                    throw RuntimeException("HTTP ${resp.code}: ${resp.message}")
                }

                val body = resp.body ?: throw RuntimeException("Пустой ответ сервера")
                val isPartial = (resp.code == 206)

                if (existingBytes > 0L && !isPartial) {
                    try { currentTarget.outputStream.close() } catch (_: Exception) {}
                    val reopened = if (currentTarget.isSaf && currentTarget.safDocFile != null) {
                        appContext.contentResolver.openOutputStream(currentTarget.safDocFile.uri, "w")
                    } else if (currentTarget.normalFile != null) {
                        FileOutputStream(currentTarget.normalFile, false)
                    } else null

                    if (reopened != null) {
                        currentTarget.outputStream = reopened
                    }
                    existingBytes = 0L
                    currentTarget.existingBytes = 0L
                }

                val streamLen = body.contentLength()
                val totalBytes = if (isPartial) {
                    existingBytes + (if (streamLen > 0) streamLen else 0L)
                } else {
                    if (streamLen > 0) streamLen else game.fileSizeBytes
                }

                val bufferSize = 1024 * 1024 // 1 MB buffer for high throughput
                bufferedOut = BufferedOutputStream(currentTarget.outputStream, bufferSize)
                val bufferedIn = BufferedInputStream(body.byteStream(), bufferSize)
                inputStream = bufferedIn
                val buffer = ByteArray(bufferSize)

                var totalRead = existingBytes
                var lastUpdateTime = System.currentTimeMillis()
                var bytesSinceLastUpdate = 0L
                var currentSpeedMbps = 0.0

                while (true) {
                    if (isPaused || isCancelled) break
                    val bytesRead = bufferedIn.read(buffer)
                    if (bytesRead == -1) break

                    bufferedOut.write(buffer, 0, bytesRead)
                    totalRead += bytesRead
                    bytesSinceLastUpdate += bytesRead

                    val now = System.currentTimeMillis()
                    val delta = now - lastUpdateTime
                    if (delta >= 500) {
                        currentSpeedMbps = (bytesSinceLastUpdate.toDouble() / (1024.0 * 1024.0)) / (delta.toDouble() / 1000.0)
                        bytesSinceLastUpdate = 0L
                        lastUpdateTime = now

                        val pct = if (totalBytes > 0) ((totalRead * 100) / totalBytes).toInt() else 0
                        val recMb = totalRead.toDouble() / (1024.0 * 1024.0)
                        val totMb = if (totalBytes > 0) totalBytes.toDouble() / (1024.0 * 1024.0) else 0.0
                        val remMb = (totMb - recMb).coerceAtLeast(0.0)
                        val etaSec = if (currentSpeedMbps > 0.05) (remMb / currentSpeedMbps).toInt() else 0

                        val statsText = if (totMb > 0) {
                            val line1 = "Скачано: ${decimalFormat.format(recMb)} МБ из ${decimalFormat.format(totMb)} МБ ($pct%)"
                            val line2 = "Скорость: ${decimalFormat.format(currentSpeedMbps)} МБ/с • Осталось: ${etaSec / 60} мин ${etaSec % 60} сек"
                            "$line1\n$line2"
                        } else {
                            val line1 = "Скачано: ${decimalFormat.format(recMb)} МБ"
                            val line2 = "Скорость: ${decimalFormat.format(currentSpeedMbps)} МБ/с"
                            "$line1\n$line2"
                        }

                        _state.value = StormDownloadProgress(
                            game = game,
                            status = StormDownloadStatus.DOWNLOADING,
                            progressPercent = pct,
                            downloadedBytes = totalRead,
                            totalBytes = totalBytes,
                            speedMbps = currentSpeedMbps,
                            etaSeconds = etaSec,
                            statsText = statsText
                        )
                    }
                }

                bufferedOut.flush()

                if (!isPaused && !isCancelled) {
                    completed = true
                    break
                }

            } catch (e: Exception) {
                if (isPaused || isCancelled) break
                Log.error("[StormDownloadManager] Interrupted: ${e.message}")
                retryCount++

                try { bufferedOut?.flush() } catch (_: Exception) {}
                try { bufferedOut?.close() } catch (_: Exception) {}
                activeTarget = null

                if (retryCount <= maxRetries) {
                    _state.value = _state.value?.copy(
                        statsText = "Повторное подключение (${e.localizedMessage ?: "Сбой"}). Попытка $retryCount из $maxRetries..."
                    )
                    delay(retryCount * 1200L)
                } else {
                    _state.value = _state.value?.copy(
                        status = StormDownloadStatus.ERROR,
                        errorMessage = e.message ?: "Сбой соединения",
                        statsText = "Ошибка загрузки: ${e.localizedMessage}"
                    )
                    break
                }
            } finally {
                try { inputStream?.close() } catch (_: Exception) {}
                try { bufferedOut?.flush() } catch (_: Exception) {}
                try { bufferedOut?.close() } catch (_: Exception) {}
            }
        }

        val target = activeTarget
        if (completed && target != null) {
            try {
                val safDoc = target.safDocFile
                val normalF = target.normalFile
                if (target.isSaf && safDoc != null) {
                    val existingFinal = target.safTree?.findFile(baseFilename)
                    if (existingFinal != null && existingFinal.exists()) {
                        existingFinal.delete()
                    }
                    safDoc.renameTo(baseFilename)
                } else if (normalF != null) {
                    val finalFile = File(normalF.parentFile, baseFilename)
                    if (finalFile.exists()) finalFile.delete()
                    normalF.renameTo(finalFile)
                }
            } catch (e: Exception) {
                Log.error("[StormDownloadManager] Rename error: ${e.message}")
            }

            game.isDownloaded = true
            _state.value = StormDownloadProgress(
                game = game,
                status = StormDownloadStatus.COMPLETED,
                progressPercent = 100,
                statsText = "✅ Игра успешно скачана!"
            )

            val serviceIntent = Intent(appContext, StormDownloadService::class.java).apply {
                action = StormDownloadService.ACTION_COMPLETE
                putExtra(StormDownloadService.EXTRA_GAME_TITLE, game.finalTitle.ifEmpty { game.title })
            }
            try {
                appContext.startService(serviceIntent)
            } catch (_: Exception) {}

        } else if (isPaused) {
            val curr = _state.value
            if (curr != null) {
                _state.value = curr.copy(
                    status = StormDownloadStatus.PAUSED,
                    statsText = "⏸ Загрузка приостановлена"
                )
            }
        } else if (!isCancelled && _state.value?.status != StormDownloadStatus.ERROR) {
            _state.value = _state.value?.copy(
                status = StormDownloadStatus.ERROR,
                statsText = "Загрузка прервана. Нажмите «Возобновить»."
            )
        }
    }
}
