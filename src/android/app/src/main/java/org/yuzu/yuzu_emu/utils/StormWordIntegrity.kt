// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.app.Dialog
import android.os.Build
import android.text.Layout
import android.view.View
import android.view.ViewGroup
import android.widget.TextView

/**
 * Global Zero Mid-Word Break Enforcer.
 * Recursively enforces HYPHENATION_FREQUENCY_NONE and BREAK_STRATEGY_SIMPLE
 * on all TextView and Button components across the entire view hierarchy.
 */
object StormWordIntegrity {

    @JvmStatic
    fun enforce(root: View?) {
        if (root == null) return

        if (root is TextView) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                root.hyphenationFrequency = Layout.HYPHENATION_FREQUENCY_NONE
                root.breakStrategy = Layout.BREAK_STRATEGY_SIMPLE
            }
        }

        if (root is ViewGroup) {
            for (i in 0 until root.childCount) {
                enforce(root.getChildAt(i))
            }
        }
    }

    @JvmStatic
    fun enforce(dialog: Dialog?) {
        if (dialog == null) return
        dialog.window?.decorView?.let { enforce(it) }
        dialog.setOnShowListener {
            dialog.window?.decorView?.let { enforce(it) }
        }
    }
}
