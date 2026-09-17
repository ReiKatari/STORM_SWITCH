// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.adapters

import android.content.res.ColorStateList
import android.text.Html
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.res.ResourcesCompat
import com.google.android.material.button.MaterialButton
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.PageSetupBinding
import org.yuzu.yuzu_emu.model.ButtonState
import org.yuzu.yuzu_emu.model.PageButton
import org.yuzu.yuzu_emu.model.PageState
import org.yuzu.yuzu_emu.model.SetupCallback
import org.yuzu.yuzu_emu.model.SetupPage
import org.yuzu.yuzu_emu.utils.ThemeHelper
import org.yuzu.yuzu_emu.utils.ViewUtils
import org.yuzu.yuzu_emu.viewholder.AbstractViewHolder
import java.util.Collections
import java.util.WeakHashMap

class SetupAdapter(val activity: AppCompatActivity, pages: List<SetupPage>) :
    AbstractListAdapter<SetupPage, SetupAdapter.SetupPageViewHolder>(pages) {

    private val activeHolders = Collections.newSetFromMap(WeakHashMap<SetupPageViewHolder, Boolean>())

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): SetupPageViewHolder {
        val binding = PageSetupBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        val holder = SetupPageViewHolder(binding)
        activeHolders.add(holder)
        return holder
    }

    fun refreshAllButtonStates() {
        activeHolders.forEach { it.refreshButtons() }
    }

    inner class SetupPageViewHolder(val binding: PageSetupBinding) :
        AbstractViewHolder<SetupPage>(binding), SetupCallback {

        private var currentPage: SetupPage? = null

        override fun bind(model: SetupPage) {
            currentPage = model
            binding.pageButtonContainer.removeAllViews()

            val isPageComplete = model.pageSteps.invoke() == PageState.COMPLETE
            if (isPageComplete) {
                ViewUtils.hideView(binding.pageButtonContainer, 0)
                ViewUtils.showView(binding.textConfirmation, 0)
            } else {
                ViewUtils.showView(binding.pageButtonContainer, 0)
                ViewUtils.hideView(binding.textConfirmation, 0)
            }

            if (model.pageButtons != null && !isPageComplete) {
                for (pageButton in model.pageButtons) {
                    val pageButtonView = LayoutInflater.from(activity)
                        .inflate(
                            R.layout.page_button,
                            binding.pageButtonContainer,
                            false
                        ) as MaterialButton

                    pageButtonView.apply {
                        id = pageButton.titleId
                        text = activity.resources.getString(pageButton.titleId)
                    }

                    pageButtonView.setOnClickListener {
                        pageButton.buttonAction.invoke(this@SetupPageViewHolder)
                    }

                    binding.pageButtonContainer.addView(pageButtonView)
                    applyButtonState(pageButtonView, pageButton)
                }
            }

            binding.icon.setImageDrawable(
                ResourcesCompat.getDrawable(
                    activity.resources,
                    model.iconId,
                    activity.theme
                )
            )
            binding.textTitle.text = activity.resources.getString(model.titleId)
            binding.textDescription.text =
                Html.fromHtml(activity.resources.getString(model.descriptionId), 0)
        }

        fun refreshButtons() {
            val model = currentPage ?: return
            val isPageComplete = model.pageSteps.invoke() == PageState.COMPLETE
            if (isPageComplete) {
                ViewUtils.hideView(binding.pageButtonContainer, 200)
                ViewUtils.showView(binding.textConfirmation, 200)
                return
            } else {
                ViewUtils.showView(binding.pageButtonContainer, 200)
                ViewUtils.hideView(binding.textConfirmation, 200)
            }

            model.pageButtons?.forEach { pageButton ->
                val buttonView = binding.pageButtonContainer.findViewById<MaterialButton>(pageButton.titleId)
                if (buttonView != null) {
                    applyButtonState(buttonView, pageButton)
                }
            }
        }

        private fun applyButtonState(button: MaterialButton, pageButton: PageButton) {
            val isComplete = pageButton.buttonState.invoke() == ButtonState.BUTTON_ACTION_COMPLETE
            val primaryColor = ThemeHelper.getColor(activity, com.google.android.material.R.attr.colorPrimary)
            val outlineColor = ThemeHelper.getColor(activity, com.google.android.material.R.attr.colorOutline)
            val onSurfaceColor = ThemeHelper.getColor(activity, com.google.android.material.R.attr.colorOnSurface)

            if (isComplete) {
                val greenColor = 0xFF10B981.toInt()
                val greenList = ColorStateList.valueOf(greenColor)
                button.strokeColor = greenList
                button.iconTint = greenList
                button.setTextColor(greenList)
                button.icon = ResourcesCompat.getDrawable(
                    activity.resources,
                    R.drawable.ic_check,
                    activity.theme
                )
                button.alpha = 0.95f
            } else {
                button.strokeColor = ColorStateList.valueOf(outlineColor)
                button.iconTint = ColorStateList.valueOf(primaryColor)
                button.setTextColor(onSurfaceColor)
                button.icon = ResourcesCompat.getDrawable(
                    activity.resources,
                    pageButton.iconId,
                    activity.theme
                )
                button.alpha = 1.0f
            }
        }

        override fun onStepCompleted(pageButtonId: Int, pageFullyCompleted: Boolean) {
            refreshButtons()
        }
    }
}
