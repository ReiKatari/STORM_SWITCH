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

        val gameDirs = NativeConfig.getGameDirs()
        val firstDir = gameDirs.firstOrNull()
        var isSaf = false
        var targetFolder: File? = null
        var safTree: DocumentFile? = null
        var targetDocPart: DocumentFile? = null
        var targetNormalPart: File? = null

        if (firstDir != null) {
            val dirUri = Uri.parse(firstDir.uriString)
            if (dirUri.scheme == "content") {
                isSaf = true
                safTree = DocumentFile.fromTreeUri(appContext, dirUri)
            } else {
                val p = dirUri.path ?: firstDir.uriString
                targetFolder = File(p)
                if (!targetFolder.exists()) targetFolder.mkdirs()
            }
        }
        if (!isSaf && targetFolder == null) {
            targetFolder = File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "STORM_SWITCH_GAMES"
            )
            if (!targetFolder.exists()) targetFolder.mkdirs()
        }

        var retryCount = 0
        val maxRetries = 10
        var completed = false

        while (retryCount <= maxRetries && !isCancelled && !isPaused && !completed) {
            var outputStream: OutputStream? = null
            var inputStream: InputStream? = null

            try {
                var existingBytes = 0L

                if (isSaf && safTree != null) {
                    targetDocPart = safTree.findFile(partFilename)
                    if (targetDocPart != null && targetDocPart.exists()) {
                        existingBytes = targetDocPart.length()
                        outputStream = appContext.contentResolver.openOutputStream(targetDocPart.uri, "wa")
                    } else {
                        targetDocPart = safTree.createFile("application/octet-stream", partFilename)
                        if (targetDocPart != null) {
                            outputStream = appContext.contentResolver.openOutputStream(targetDocPart.uri, "w")
                        }
                    }
                } else if (targetFolder != null) {
                    targetNormalPart = File(targetFolder, partFilename)
                    if (targetNormalPart.exists()) {
                        existingBytes = targetNormalPart.length()
                    }
                    outputStream = FileOutputStream(targetNormalPart, true)
                }

                if (outputStream == null) {
                    throw RuntimeException("Не удалось открыть целевой файл для записи")
                }

                val downloadUrl = "https://stormgamesworld.ru/api/games/${game.id}/download"
                val reqBuilder = Request.Builder()
                    .url(downloadUrl)
                    .header("User-Agent", "STORM_SWITCH/8.7.4 (Android)")

                if (existingBytes > 0L) {
                    reqBuilder.header("Range", "bytes=$existingBytes-")
                }

                val req = reqBuilder.build()
                val call = httpClient.newCall(req)
                activeCall = call

                val resumeMsg = if (existingBytes > 0L) {
                    val mb = existingBytes / (1024 * 1024)
                    "Возобновление загрузки с ${mb} МБ..."
                } else {
                    "Подключение к серверу загрузки..."
                }
                _state.value = _state.value?.copy(
                    status = StormDownloadStatus.DOWNLOADING,
                    downloadedBytes = existingBytes,
                    statsText = resumeMsg
                )

                val resp = call.execute()

                if (resp.code == 416) {
                    outputStream.close()
                    completed = true
                    break
                }

                if (!resp.isSuccessful) {
                    throw RuntimeException("HTTP ${resp.code}: ${resp.message}")
                }

                val body = resp.body ?: throw RuntimeException("Пустой ответ сервера")
                val isPartial = (resp.code == 206)

                if (existingBytes > 0L && !isPartial) {
                    outputStream.close()
                    if (isSaf && targetDocPart != null) {
                        outputStream = appContext.contentResolver.openOutputStream(targetDocPart.uri, "w")
                    } else if (targetNormalPart != null) {
                        outputStream = FileOutputStream(targetNormalPart, false)
                    }
                    existingBytes = 0L
                }

                val streamLen = body.contentLength()
                val totalBytes = if (isPartial) {
                    existingBytes + (if (streamLen > 0) streamLen else 0L)
                } else {
                    if (streamLen > 0) streamLen else game.fileSizeBytes
                }

                val bufferSize = 1024 * 1024 // 1 MB buffer for maximum download throughput
                val bufferedOut = BufferedOutputStream(outputStream, bufferSize)
                outputStream = bufferedOut
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

                outputStream?.flush()

                if (!isPaused && !isCancelled) {
                    completed = true
                    break
                }

            } catch (e: Exception) {
                if (isPaused || isCancelled) break
                Log.error("[StormDownloadManager] Interrupted: ${e.message}")
                retryCount++
                if (retryCount <= maxRetries) {
                    _state.value = _state.value?.copy(
                        statsText = "Обрыв соединения (${e.localizedMessage ?: "Сбой"}). Повтор $retryCount из $maxRetries..."
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
                try { outputStream?.flush() } catch (_: Exception) {}
                try { outputStream?.close() } catch (_: Exception) {}
            }
        }

        if (completed) {
            try {
                if (isSaf && safTree != null && targetDocPart != null) {
                    val existingFinal = safTree.findFile(baseFilename)
                    if (existingFinal != null && existingFinal.exists()) {
                        existingFinal.delete()
                    }
                    targetDocPart.renameTo(baseFilename)
                } else if (targetNormalPart != null) {
                    val finalFile = File(targetNormalPart.parentFile, baseFilename)
                    if (finalFile.exists()) finalFile.delete()
                    targetNormalPart.renameTo(finalFile)
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
