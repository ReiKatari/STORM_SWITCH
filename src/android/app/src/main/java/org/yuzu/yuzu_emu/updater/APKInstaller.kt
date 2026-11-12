// SPDX-FileCopyrightText: 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.core.content.FileProvider
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.utils.Log
import java.io.File
import java.io.IOException
import java.lang.ref.WeakReference

/**
 * Play-Protect-clean native APK installer.
 * Delegates installation cleanly to Android native system Package Installer UI
 * via ACTION_VIEW and FileProvider, eliminating PHA:Dropper heuristics.
 */
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
        private const val APK_MIME = "application/vnd.android.package-archive"

        @Volatile
        var pendingApkFile: File? = null
            private set

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
                openSystemPackageInstaller(context, apkFile)
                onComplete()
            } catch (e: Exception) {
                Log.error("[$TAG] Native PackageInstaller handoff failed: ${e.message}")
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
                        openSystemPackageInstaller(activity, apk)
                    } catch (e: Exception) {
                        Log.error("[$TAG] resumePendingInstall failed: ${e.message}")
                    }
                }
            } else {
                pendingApkFile = null
                try {
                    openSystemPackageInstaller(activity, apk)
                } catch (e: Exception) {
                    Log.error("[$TAG] resumePendingInstall failed: ${e.message}")
                }
            }
        }

        private fun openSystemPackageInstaller(context: Context, apkFile: File) {
            val appContext = context.applicationContext
            val fileUri = runCatching {
                FileProvider.getUriForFile(appContext, "${appContext.packageName}.provider", apkFile)
            }.getOrNull() ?: Uri.fromFile(apkFile)

            val installIntent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(fileUri, APK_MIME)
                flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_GRANT_READ_URI_PERMISSION
            }

            val activity = (context as? Activity) ?: currentActivityRef?.get()
            if (activity != null && !activity.isFinishing && !activity.isDestroyed) {
                activity.startActivity(installIntent)
            } else {
                appContext.startActivity(installIntent)
            }
            Log.info("[$TAG] System Package Installer launched cleanly via FileProvider URI")
        }
    }
}
