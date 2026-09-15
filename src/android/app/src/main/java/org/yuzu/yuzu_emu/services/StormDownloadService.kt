// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.services

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.ui.main.MainActivity
import org.yuzu.yuzu_emu.utils.Log
import java.util.Locale

class StormDownloadService : Service() {

    private val serviceScope = CoroutineScope(Dispatchers.Main + Job())
    private var collectorJob: Job? = null
    private var lastNotificationUpdateTime = 0L

    companion object {
        const val DOWNLOAD_NOTIFICATION_ID = 0x544F // "ST"
        const val CHANNEL_ID = "storm_downloads_channel"

        const val ACTION_START = "org.yuzu.yuzu_emu.services.ACTION_START"
        const val ACTION_PAUSE = "org.yuzu.yuzu_emu.services.ACTION_PAUSE"
        const val ACTION_RESUME = "org.yuzu.yuzu_emu.services.ACTION_RESUME"
        const val ACTION_CANCEL = "org.yuzu.yuzu_emu.services.ACTION_CANCEL"
        const val ACTION_STOP = "org.yuzu.yuzu_emu.services.ACTION_STOP"
        const val ACTION_UPDATE = "org.yuzu.yuzu_emu.services.ACTION_UPDATE"
        const val ACTION_COMPLETE = "org.yuzu.yuzu_emu.services.ACTION_COMPLETE"

        const val EXTRA_GAME_TITLE = "extra_game_title"
        const val EXTRA_IS_PAUSED = "extra_is_paused"
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        ensureNotificationChannel()
        startObservingProgress()
    }

    private fun ensureNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as? NotificationManager
            val channel = NotificationChannel(
                CHANNEL_ID,
                "STORM GAMES WORLD Загрузки",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Уведомления о скачивании игр и обновлений"
                setSound(null, null)
                enableVibration(false)
            }
            notificationManager?.createNotificationChannel(channel)
        }
    }

    private fun startObservingProgress() {
        collectorJob?.cancel()
        collectorJob = serviceScope.launch {
            StormDownloadManager.state.collectLatest { progress ->
                if (progress == null) return@collectLatest
                val now = System.currentTimeMillis()
                // Throttle notification updates to once per 800ms unless status changed
                if (now - lastNotificationUpdateTime >= 800 ||
                    progress.status == StormDownloadStatus.COMPLETED ||
                    progress.status == StormDownloadStatus.ERROR ||
                    progress.status == StormDownloadStatus.PAUSED) {
                    lastNotificationUpdateTime = now
                    updateNotification(progress)
                }
            }
        }
    }

    private fun updateNotification(progress: StormDownloadProgress) {
        val title = progress.game.finalTitle.ifEmpty { progress.game.title }
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
            },
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val pauseResumeIntent = Intent(this, StormDownloadService::class.java).apply {
            action = if (progress.status == StormDownloadStatus.PAUSED) ACTION_RESUME else ACTION_PAUSE
        }
        val pauseResumePending = PendingIntent.getService(
            this,
            1,
            pauseResumeIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val cancelIntent = Intent(this, StormDownloadService::class.java).apply {
            action = ACTION_CANCEL
        }
        val cancelPending = PendingIntent.getService(
            this,
            2,
            cancelIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_install)
            .setContentTitle("STORM GAMES WORLD: $title")
            .setContentIntent(openIntent)
            .setOnlyAlertOnce(true)
            .setVibrate(null)
            .setSound(null)

        when (progress.status) {
            StormDownloadStatus.CONNECTING -> {
                builder.setContentText("Подключение к серверу загрузки...")
                    .setProgress(100, 0, true)
                    .setOngoing(true)
                    .addAction(R.drawable.ic_clear, "Отмена", cancelPending)
                startForeground(DOWNLOAD_NOTIFICATION_ID, builder.build())
            }

            StormDownloadStatus.DOWNLOADING -> {
                val subText = if (progress.speedMbps > 0.05) {
                    "${String.format(Locale.ROOT, "%.1f", progress.speedMbps)} МБ/с • Ост: ${progress.etaSeconds / 60} мин"
                } else {
                    "${progress.progressPercent}%"
                }
                builder.setContentText(progress.statsText.ifEmpty { "Загрузка: ${progress.progressPercent}%" })
                    .setSubText(subText)
                    .setProgress(100, progress.progressPercent, false)
                    .setOngoing(true)
                    .addAction(R.drawable.ic_pause, "Пауза", pauseResumePending)
                    .addAction(R.drawable.ic_clear, "Отмена", cancelPending)
                startForeground(DOWNLOAD_NOTIFICATION_ID, builder.build())
            }

            StormDownloadStatus.PAUSED -> {
                builder.setContentText("⏸ Загрузка приостановлена (${progress.progressPercent}%)")
                    .setProgress(100, progress.progressPercent, false)
                    .setOngoing(true)
                    .addAction(R.drawable.ic_play, "Продолжить", pauseResumePending)
                    .addAction(R.drawable.ic_clear, "Отмена", cancelPending)
                startForeground(DOWNLOAD_NOTIFICATION_ID, builder.build())
            }

            StormDownloadStatus.COMPLETED -> {
                builder.setContentText("✅ Игра успешно скачана!")
                    .setProgress(0, 0, false)
                    .setOngoing(false)
                    .setAutoCancel(true)
                try {
                    NotificationManagerCompat.from(this).notify(DOWNLOAD_NOTIFICATION_ID + 1, builder.build())
                    stopForeground(STOP_FOREGROUND_REMOVE)
                } catch (_: Exception) {}
                stopSelf()
            }

            StormDownloadStatus.ERROR -> {
                builder.setContentText("⚠️ ${progress.errorMessage.ifEmpty { "Ошибка загрузки" }}")
                    .setProgress(0, 0, false)
                    .setOngoing(false)
                    .setAutoCancel(true)
                try {
                    NotificationManagerCompat.from(this).notify(DOWNLOAD_NOTIFICATION_ID + 2, builder.build())
                    stopForeground(STOP_FOREGROUND_REMOVE)
                } catch (_: Exception) {}
                stopSelf()
            }

            StormDownloadStatus.CANCELLED, StormDownloadStatus.IDLE -> {
                try {
                    stopForeground(STOP_FOREGROUND_REMOVE)
                    NotificationManagerCompat.from(this).cancel(DOWNLOAD_NOTIFICATION_ID)
                } catch (_: Exception) {}
                stopSelf()
            }
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val action = intent?.action
        when (action) {
            ACTION_START -> {
                val gameTitle = intent.getStringExtra(EXTRA_GAME_TITLE) ?: "Игра"
                val builder = NotificationCompat.Builder(this, CHANNEL_ID)
                    .setSmallIcon(R.drawable.ic_install)
                    .setContentTitle("STORM GAMES WORLD: $gameTitle")
                    .setContentText("Подключение к серверу загрузки...")
                    .setProgress(100, 0, true)
                    .setOngoing(true)
                startForeground(DOWNLOAD_NOTIFICATION_ID, builder.build())
            }

            ACTION_PAUSE -> {
                StormDownloadManager.pauseDownload(this)
            }

            ACTION_RESUME -> {
                StormDownloadManager.resumeDownload(this)
            }

            ACTION_CANCEL, ACTION_STOP -> {
                StormDownloadManager.cancelDownload(this)
                try {
                    stopForeground(STOP_FOREGROUND_REMOVE)
                    NotificationManagerCompat.from(this).cancel(DOWNLOAD_NOTIFICATION_ID)
                } catch (_: Exception) {}
                stopSelfResult(startId)
                return START_NOT_STICKY
            }

            ACTION_COMPLETE -> {
                try {
                    stopForeground(STOP_FOREGROUND_REMOVE)
                } catch (_: Exception) {}
                stopSelfResult(startId)
                return START_NOT_STICKY
            }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        collectorJob?.cancel()
        try {
            NotificationManagerCompat.from(this).cancel(DOWNLOAD_NOTIFICATION_ID)
        } catch (_: Exception) {}
        super.onDestroy()
    }
}
