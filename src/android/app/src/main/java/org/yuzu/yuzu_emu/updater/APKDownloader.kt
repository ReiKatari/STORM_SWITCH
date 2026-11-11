// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import okhttp3.OkHttpClient
import okhttp3.Request
import org.yuzu.yuzu_emu.utils.SmartDns
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

class APKDownloader(
    private val urls: List<String>,
    private val outputFile: File
) {
    constructor(url: String, outputFile: File) : this(listOf(url), outputFile)

    @Volatile
    private var isCancelled = false

    fun cancel() {
        isCancelled = true
    }

    fun download(
        onProgress: (progress: Int, currentBytes: Long, totalBytes: Long) -> Unit,
        onComplete: (Boolean) -> Unit
    ) {
        Thread {
            val client = OkHttpClient.Builder()
                .dns(SmartDns)
                .connectTimeout(20, TimeUnit.SECONDS)
                .readTimeout(60, TimeUnit.SECONDS)
                .writeTimeout(30, TimeUnit.SECONDS)
                .followRedirects(true)
                .followSslRedirects(true)
                .build()

            for (candidateUrl in urls.distinct()) {
                if (isCancelled) break
                if (candidateUrl.isBlank()) continue
                try {
                    if (outputFile.exists()) {
                        outputFile.delete()
                    }
                    val request = Request.Builder()
                        .url(candidateUrl)
                        .header("User-Agent", "STORM-SWITCH-Updater")
                        .build()

                    var succeeded = false
                    client.newCall(request).execute().use { response ->
                        if (response.isSuccessful && !isCancelled) {
                            val body = response.body
                            if (body != null) {
                                val contentLength = body.contentLength()
                                body.byteStream().use { inputStream ->
                                    FileOutputStream(outputFile).use { outputStream ->
                                        val buffer = ByteArray(32768)
                                        var bytesRead = 0
                                        var totalBytesRead: Long = 0L
                                        while (!isCancelled && inputStream.read(buffer).also { bytesRead = it } != -1) {
                                            outputStream.write(buffer, 0, bytesRead)
                                            totalBytesRead += bytesRead
                                            val progress = if (contentLength > 0) {
                                                ((totalBytesRead * 100L) / contentLength).toInt().coerceIn(0, 100)
                                            } else {
                                                ((totalBytesRead / (350L * 1024L)).toInt()).coerceIn(0, 99)
                                            }
                                            onProgress(progress, totalBytesRead, contentLength)
                                        }
                                        outputStream.flush()
                                    }
                                }
                                if (!isCancelled && isValidApkFile(outputFile)) {
                                    succeeded = true
                                }
                            }
                        }
                    }
                    if (isCancelled) {
                        try { if (outputFile.exists()) outputFile.delete() } catch (_: Throwable) {}
                        onComplete(false)
                        return@Thread
                    }
                    if (succeeded) {
                        val finalLen = outputFile.length()
                        onProgress(100, finalLen, finalLen)
                        onComplete(true)
                        return@Thread
                    }
                } catch (_: Throwable) {
                    try {
                        if (outputFile.exists()) outputFile.delete()
                    } catch (_: Throwable) {}
                }
            }
            onComplete(false)
        }.start()
    }

    fun download(onProgress: (Int) -> Unit, onComplete: (Boolean) -> Unit) {
        download(
            onProgress = { percent, _, _ -> onProgress(percent) },
            onComplete = onComplete
        )
    }

    companion object {
        fun isValidApkFile(file: File): Boolean {
            return try {
                if (!file.exists() || file.length() <= 1024L * 1024L) {
                    false
                } else {
                    val magic = ByteArray(4)
                    val readBytes = file.inputStream().use { it.read(magic) }
                    readBytes == 4 &&
                        magic[0] == 0x50.toByte() &&
                        magic[1] == 0x4B.toByte() &&
                        magic[2] == 0x03.toByte() &&
                        magic[3] == 0x04.toByte()
                }
            } catch (_: Throwable) {
                false
            }
        }
    }
}
