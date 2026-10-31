// SPDX-FileCopyrightText: 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.view.KeyEvent

import android.app.Dialog
import android.content.DialogInterface
import android.os.Bundle
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R

class CoreErrorDialogFragment : DialogFragment() {
    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog =
        MaterialAlertDialogBuilder(requireActivity())
            .setTitle(requireArguments().getString(TITLE))
            .setMessage(requireArguments().getString(MESSAGE))
            .setPositiveButton(R.string.continue_button, null)
            .setNegativeButton(R.string.abort_button) { _: DialogInterface?, _: Int ->
                NativeLibrary.coreErrorAlertResult = false
                synchronized(NativeLibrary.coreErrorAlertLock) {
                    NativeLibrary.coreErrorAlertLock.notify()
                }
            }
            .create()

    override fun onDismiss(dialog: DialogInterface) {
        super.onDismiss(dialog)
        NativeLibrary.coreErrorAlertResult = true
        synchronized(NativeLibrary.coreErrorAlertLock) { NativeLibrary.coreErrorAlertLock.notify() }
    }

    override fun onStart() {
        super.onStart()
        dialog?.setOnKeyListener { _, keyCode, event ->
            if (keyCode == KeyEvent.KEYCODE_ESCAPE && event.action == KeyEvent.ACTION_UP) {
                dismissAllowingStateLoss()
                true
            } else {
                false
            }
        }
    }

    companion object {
        const val TITLE = "Title"
        const val MESSAGE = "Message"

        fun newInstance(title: String, message: String): CoreErrorDialogFragment {
            val frag = CoreErrorDialogFragment()
            val args = Bundle()
            args.putString(TITLE, title)
            args.putString(MESSAGE, message)
            frag.arguments = args
            return frag
        }
    }
}
