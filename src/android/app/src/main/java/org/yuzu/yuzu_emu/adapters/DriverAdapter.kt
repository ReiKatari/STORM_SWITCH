// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.CardDriverOptionBinding
import org.yuzu.yuzu_emu.features.settings.model.StringSetting
import org.yuzu.yuzu_emu.model.Driver
import org.yuzu.yuzu_emu.model.DriverViewModel
import org.yuzu.yuzu_emu.utils.ViewUtils.setVisible
import org.yuzu.yuzu_emu.viewholder.AbstractViewHolder

class DriverAdapter(
    private val driverViewModel: DriverViewModel,
    private val onDriverClicked: ((driver: Driver, position: Int) -> Unit)? = null
) :
    AbstractSingleSelectionList<Driver, DriverAdapter.DriverViewHolder>(
        driverViewModel.driverList.value
    ) {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): DriverViewHolder {
        CardDriverOptionBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            .also { return DriverViewHolder(it) }
    }

    inner class DriverViewHolder(val binding: CardDriverOptionBinding) :
        AbstractViewHolder<Driver>(binding) {
        override fun bind(model: Driver) {
            binding.apply {
                radioButton.isChecked = model.selected

                // Highlight selected card border
                if (model.selected) {
                    val primaryColor = com.google.android.material.color.MaterialColors.getColor(
                        root,
                        com.google.android.material.R.attr.colorPrimary
                    )
                    cardDriver.strokeColor = primaryColor
                    cardDriver.strokeWidth = (root.context.resources.displayMetrics.density * 2).toInt()
                } else {
                    val outlineColor = com.google.android.material.color.MaterialColors.getColor(
                        root,
                        com.google.android.material.R.attr.colorOutline
                    )
                    cardDriver.strokeColor = outlineColor
                    cardDriver.strokeWidth = (root.context.resources.displayMetrics.density * 1).toInt()
                }

                root.setOnClickListener {
                    if (onDriverClicked != null) {
                        onDriverClicked.invoke(model, bindingAdapterPosition)
                    } else {
                        selectItem(bindingAdapterPosition) {
                            driverViewModel.onDriverSelected(it)
                            driverViewModel.showClearButton(!StringSetting.DRIVER_PATH.global)
                        }
                    }
                }

                val isSystemDriver = bindingAdapterPosition == 0 ||
                    model.title == root.context.getString(R.string.system_gpu_driver)

                if (driverViewModel.isPerGame) {
                    val isCustomOverride = !StringSetting.DRIVER_PATH.global
                    val useGlobalText = root.context.getString(R.string.menu_driver_use_global)
                    buttonDelete.setVisible(model.selected && isCustomOverride)
                    buttonDelete.contentDescription = useGlobalText
                    buttonDelete.tooltipText = useGlobalText
                    buttonDelete.setOnClickListener {
                        driverViewModel.onDriverRemoved(bindingAdapterPosition, 0)
                        replaceList(driverViewModel.driverList.value)
                    }
                } else {
                    val deleteText = root.context.getString(R.string.delete)
                    buttonDelete.contentDescription = deleteText
                    buttonDelete.tooltipText = deleteText
                    buttonDelete.setVisible(!isSystemDriver)
                    buttonDelete.setOnClickListener {
                        val currentPos = bindingAdapterPosition
                        if (currentPos > 0 && currentPos - 1 in driverViewModel.driverData.indices) {
                            val driverEntry = driverViewModel.driverData[currentPos - 1]
                            val driverTitle = model.title
                            com.google.android.material.dialog.MaterialAlertDialogBuilder(root.context)
                                .setTitle(R.string.delete)
                                .setMessage(root.context.getString(R.string.driver_delete_confirm, driverTitle))
                                .setPositiveButton(R.string.delete) { _, _ ->
                                    driverViewModel.deleteDriver(driverEntry.first)
                                    replaceList(driverViewModel.driverList.value)
                                }
                                .setNegativeButton(R.string.cancel, null)
                                .show()
                        }
                    }
                }

                title.text = model.title
                if (model.version.isNotBlank() && model.version != model.title) {
                    version.setVisible(true)
                    version.text = model.version
                } else {
                    version.setVisible(false)
                }

                if (model.description.isNotBlank()) {
                    description.setVisible(true)
                    description.text = model.description
                } else {
                    description.setVisible(false)
                }
            }
        }
    }
}
