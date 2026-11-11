// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.app.Activity
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageInstaller
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.core.content.ContextCompat
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.utils.Log
import java.io.File
import java.io.IOException
import java.lang.ref.WeakReference

class APKInstaller(private val context: Context) {

    fun install(
        apkFile: File,
        onComplete: () -> Unit = {},
        onFailure: (Exception) -> Unit = {}
    ) {
        installApk(context, apkFile, onComplete, onFailure)
    }

    companion object {
        private const val TAG = "StormAPKInstaller"
        private const val ACTION_INSTALL_STATUS = ".UPDATE_INSTALL_STATUS"

        @Volatile
        var pendingApkFile: File? = null
            private set

        @Volatile
        private var isReceiverRegistered = false

        private var currentActivityRef: WeakReference<Activity>? = null

        fun setCurrentActivity(activity: Activity?) {
            currentActivityRef = if (activity != null) WeakReference(activity) else null
        }

        fun installApk(
            context: Context,
            apkFile: File,
            onComplete: () -> Unit = {},
            onFailure: (Exception) -> Unit = {}
        ) {
            val appContext = context.applicationContext
            if (!apkFile.exists() || apkFile.length() <= 1024L * 1024L) {
                val err = IOException("APK file does not exist or is corrupted: ${apkFile.absolutePath}")
                onFailure(err)
                return
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                if (!appContext.packageManager.canRequestPackageInstalls()) {
                    pendingApkFile = apkFile
                    val activity = (context as? Activity) ?: currentActivityRef?.get()
                    if (activity != null && !activity.isFinishing && !activity.isDestroyed) {
                        MaterialAlertDialogBuilder(activity)
                            .setTitle(R.string.allow_install_unknown_apps_title)
                            .setMessage(R.string.allow_install_unknown_apps_desc)
                            .setPositiveButton(R.string.open_settings) { _, _ ->
                                try {
                                    val intent = Intent(
                                        Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                                        Uri.parse("package:${appContext.packageName}")
                                    ).apply {
                                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                    }
                                    activity.startActivity(intent)
                                } catch (e: Exception) {
                                    Log.error("[$TAG] Failed to open unknown app sources settings: ${e.message}")
                                }
                            }
                            .setNegativeButton(R.string.cancel) { dialog, _ ->
                                pendingApkFile = null
                                dialog.dismiss()
                            }
                            .setCancelable(false)
                            .show()
                    } else {
                        try {
                            val intent = Intent(
                                Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                                Uri.parse("package:${appContext.packageName}")
                            ).apply {
                                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                            }
                            appContext.startActivity(intent)
                        } catch (e: Exception) {
                            Log.error("[$TAG] Failed to launch unknown app sources: ${e.message}")
                        }
                    }
                    return
                }
            }

            pendingApkFile = null
            try {
                commitInstallSession(appContext, apkFile)
                onComplete()
            } catch (e: Exception) {
                Log.error("[$TAG] PackageInstaller session commit failed: ${e.message}")
                onFailure(e)
            }
        }

        fun resumePendingInstall(activity: Activity) {
            setCurrentActivity(activity)
            val apk = pendingApkFile ?: return
            if (!apk.exists() || apk.length() <= 1024L * 1024L) {
                pendingApkFile = null
                return
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                if (activity.packageManager.canRequestPackageInstalls()) {
                    pendingApkFile = null
                    try {
                        commitInstallSession(activity.applicationContext, apk)
                    } catch (e: Exception) {
                        Log.error("[$TAG] resumePendingInstall failed: ${e.message}")
                    }
                }
            } else {
                pendingApkFile = null
                try {
                    commitInstallSession(activity.applicationContext, apk)
                } catch (e: Exception) {
                    Log.error("[$TAG] resumePendingInstall failed: ${e.message}")
                }
            }
        }

        private fun commitInstallSession(context: Context, apkFile: File) {
            val appContext = context.applicationContext
            val action = appContext.packageName + ACTION_INSTALL_STATUS

            registerStatusReceiver(appContext, action)

            val packageInstaller = appContext.packageManager.packageInstaller
            val params = PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL).apply {
                setAppPackageName(appContext.packageName)
                setSize(apkFile.length())
            }

            val sessionId = packageInstaller.createSession(params)
            try {
                packageInstaller.openSession(sessionId).use { session ->
                    apkFile.inputStream().use { inStream ->
                        session.openWrite("base.apk", 0, apkFile.length()).use { outStream ->
                            inStream.copyTo(outStream)
                            session.fsync(outStream)
                        }
                    }

                    val intent = Intent(action).setPackage(appContext.packageName)
                    val flags = PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
                    val pendingIntent = PendingIntent.getBroadcast(appContext, sessionId, intent, flags)

                    session.commit(pendingIntent.intentSender)
                }
                Log.info("[$TAG] PackageInstaller session $sessionId committed successfully")
            } catch (e: Exception) {
                try {
                    packageInstaller.abandonSession(sessionId)
                } catch (_: Throwable) {}
                throw e
            }
        }

        @Synchronized
        private fun registerStatusReceiver(appContext: Context, action: String) {
            if (isReceiverRegistered) return
            isReceiverRegistered = true

            val receiver = object : BroadcastReceiver() {
                override fun onReceive(context: Context, intent: Intent) {
                    val status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE)
                    val message = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE)

                    when (status) {
                        PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                            val confirmIntent = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                                intent.getParcelableExtra(Intent.EXTRA_INTENT, Intent::class.java)
                            } else {
                                @Suppress("DEPRECATION")
                                intent.getParcelableExtra(Intent.EXTRA_INTENT)
                            }
                            if (confirmIntent != null) {
                                confirmIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                val activity = currentActivityRef?.get()
                                if (activity != null && !activity.isFinishing && !activity.isDestroyed) {
                                    activity.startActivity(confirmIntent)
                                } else {
                                    context.startActivity(confirmIntent)
                                }
                            }
                        }
                        PackageInstaller.STATUS_SUCCESS -> {
                            Log.info("[$TAG] Update installation finished successfully")
                        }
                        else -> {
                            Log.warning("[$TAG] Installation status: $status, message: $message")
                        }
                    }
                }
            }

            ContextCompat.registerReceiver(
                appContext,
                receiver,
                IntentFilter(action),
                ContextCompat.RECEIVER_NOT_EXPORTED
            )
        }
    }
}
