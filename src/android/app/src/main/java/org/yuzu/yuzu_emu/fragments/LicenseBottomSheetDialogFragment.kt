// SPDX-FileCopyrightText: 2026 STORM TEAM
// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.fragments

import android.app.Dialog
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.Window
import androidx.core.view.WindowCompat
import androidx.fragment.app.DialogFragment
import org.yuzu.yuzu_emu.databinding.DialogLicenseBinding
import org.yuzu.yuzu_emu.model.License
import org.yuzu.yuzu_emu.utils.SerializableHelper.parcelable
import org.yuzu.yuzu_emu.utils.ThemeHelper

class LicenseBottomSheetDialogFragment : DialogFragment() {
    private var _binding: DialogLicenseBinding? = null
    private val binding get() = _binding!!

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setStyle(STYLE_NORMAL, ThemeHelper.getSelectedStaticThemeColor())
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        dialog.window?.let { window ->
            window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            WindowCompat.setDecorFitsSystemWindows(window, false)
        }
        dialog.setOnKeyListener { _, keyCode, event ->
            if (keyCode == KeyEvent.KEYCODE_ESCAPE && event.action == KeyEvent.ACTION_UP) {
                dismissAllowingStateLoss()
                true
            } else {
                false
            }
        }
        return dialog
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = DialogLicenseBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.btnClose?.setOnClickListener { dismiss() }

        val license = requireArguments().parcelable<License>(LICENSE)!!

        binding.apply {
            textTitle.setText(license.titleId)
            textLink.setText(license.linkId)
            textCopyright.setText(license.copyrightId)
            textLicense.setText(license.licenseId)
        }
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val displayMetrics = resources.displayMetrics
            val width = (displayMetrics.widthPixels * 0.90).toInt()
            val height = (displayMetrics.heightPixels * 0.85).toInt()
            window.setLayout(width, height)
            ThemeHelper.applySystemBarsTheme(window, requireContext())
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    companion object {
        const val TAG = "LicenseBottomSheetDialogFragment"

        const val LICENSE = "License"

        fun newInstance(
            license: License
        ): LicenseBottomSheetDialogFragment {
            val dialog = LicenseBottomSheetDialogFragment()
            val bundle = Bundle()
            bundle.putParcelable(LICENSE, license)
            dialog.arguments = bundle
            return dialog
        }
    }
}
