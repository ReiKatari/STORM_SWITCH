// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.SmartDns
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

/**
 * Resilient multi-mirror APK downloader.
 * Supports primary fast mirror (stormgamesworld.ru) and GitHub Release asset mirror,
 * manually following cross-domain 301/302/303/307/308 Location headers up to 6 hops
 * with SmartDns and strict APK integrity validation.
 */
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
            val redirectClient = OkHttpClient.Builder()
                .dns(SmartDns)
                .connectTimeout(20, TimeUnit.SECONDS)
                .readTimeout(60, TimeUnit.SECONDS)
                .writeTimeout(30, TimeUnit.SECONDS)
                .followRedirects(false)
                .followSslRedirects(false)
                .build()

            for (candidateUrl in urls.distinct()) {
                if (isCancelled) break
                if (candidateUrl.isBlank()) continue
                try {
                    if (outputFile.exists()) {
                        outputFile.delete()
                    }

                    var currentUrl = candidateUrl
                    var redirects = 0
                    var activeResponse: Response? = null

                    while (redirects < 6 && !isCancelled) {
                        val request = Request.Builder()
                            .url(currentUrl)
                            .header("User-Agent", "STORM-SWITCH-Updater")
                            .header("Accept", "application/octet-stream, */*")
                            .build()

                        val response = redirectClient.newCall(request).execute()
                        val code = response.code
                        if (code in 300..399) {
                            val location = response.header("Location")
                            response.close()
                            if (location.isNullOrBlank()) {
                                break
                            }
                            val resolved = response.request.url.resolve(location)?.toString() ?: location
                            currentUrl = resolved
                            redirects++
                        } else if (response.isSuccessful) {
                            activeResponse = response
                            break
                        } else {
                            response.close()
                            break
                        }
                    }

                    val response = activeResponse
                    var succeeded = false

                    if (response != null && response.isSuccessful && !isCancelled) {
                        response.use { resp ->
                            val body = resp.body
                            if (body != null) {
                                val contentLength = body.contentLength()
                                body.byteStream().use { inputStream ->
                                    FileOutputStream(outputFile).use { outputStream ->
                                        val buffer = ByteArray(64 * 1024)
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
                        Log.info("[APKDownloader] Download succeeded from $candidateUrl ($finalLen bytes)")
                        onComplete(true)
                        return@Thread
                    } else {
                        Log.warning("[APKDownloader] Download incomplete or invalid from $candidateUrl, trying next mirror...")
                        try { if (outputFile.exists()) outputFile.delete() } catch (_: Throwable) {}
                    }
                } catch (e: Throwable) {
                    Log.warning("[APKDownloader] Error downloading from $candidateUrl: ${e.message}")
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
        const val MIN_VALID_APK_BYTES = 15_000_000L

        fun isValidApkFile(file: File): Boolean {
            return try {
                if (!file.exists() || file.length() < MIN_VALID_APK_BYTES) {
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
