// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.updater

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.core.content.ContextCompat
import androidx.core.content.FileProvider
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.io.File

class APKInstaller(private val context: Context) {

    fun install(apkFile: File, onComplete: () -> Unit = {}, onFailure: (Exception) -> Unit = {}) {
        try {
            if (!APKDownloader.isValidApkFile(apkFile)) {
                onFailure(IllegalStateException("Invalid APK file"))
                return
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
                !context.packageManager.canRequestPackageInstalls()
            ) {
                pendingApkFile = apkFile
                val permIntent = Intent(
                    Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                    Uri.parse("package:${context.packageName}")
                ).apply {
                    if (context !is Activity) {
                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    }
                }
                context.startActivity(permIntent)
                return
            }

            pendingApkFile = null
            launchPackageInstaller(context, apkFile, onComplete, onFailure)
        } catch (e: Exception) {
            onFailure(e)
        }
    }

    companion object {
        @Volatile
        private var pendingApkFile: File? = null

        fun resumePendingInstall(context: Context) {
            val file = pendingApkFile ?: return
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
                !context.packageManager.canRequestPackageInstalls()
            ) {
                return
            }
            pendingApkFile = null
            if (APKDownloader.isValidApkFile(file)) {
                try {
                    launchPackageInstaller(context, file, {}, {})
                } catch (_: Throwable) {}
            }
        }

        private fun launchPackageInstaller(
            context: Context,
            apkFile: File,
            onComplete: () -> Unit,
            onFailure: (Exception) -> Unit
        ) {
            val apkUri: Uri = FileProvider.getUriForFile(
                context,
                context.applicationContext.packageName + ".provider",
                apkFile
            )
            val intent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(apkUri, "application/vnd.android.package-archive")
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
                putExtra(Intent.EXTRA_NOT_UNKNOWN_SOURCE, true)
            }
            context.startActivity(intent)

            GlobalScope.launch {
                try {
                    val receiver = AppInstallReceiver(onComplete, onFailure)
                    val filter = IntentFilter().apply {
                        addAction(Intent.ACTION_PACKAGE_ADDED)
                        addAction(Intent.ACTION_PACKAGE_REPLACED)
                        addDataScheme("package")
                    }
                    ContextCompat.registerReceiver(
                        context.applicationContext,
                        receiver,
                        filter,
                        ContextCompat.RECEIVER_EXPORTED
                    )
                } catch (_: Throwable) {}
            }
        }
    }
}
