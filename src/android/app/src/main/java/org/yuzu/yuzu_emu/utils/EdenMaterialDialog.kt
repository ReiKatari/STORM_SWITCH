// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Dialog
import android.content.Context
import androidx.appcompat.app.AlertDialog
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.R

object EdenMaterialDialog {

    @JvmStatic
    fun builder(context: Context): MaterialAlertDialogBuilder {
        val themedContext = androidx.appcompat.view.ContextThemeWrapper(
            context,
            ThemeHelper.getSelectedStaticThemeColor()
        )
        return object : MaterialAlertDialogBuilder(themedContext, R.style.EdenMaterialDialog) {
            override fun create(): AlertDialog {
                val dialog = super.create()
                dialog.setOnShowListener {
                    StormWordIntegrity.enforce(dialog.window?.decorView)
                }
                return dialog
            }
        }
    }

    @JvmStatic
    fun enforce(dialog: Dialog?) {
        StormWordIntegrity.enforce(dialog)
    }

    @JvmStatic
    fun enforce(root: android.view.View?) {
        StormWordIntegrity.enforce(root)
    }
}
