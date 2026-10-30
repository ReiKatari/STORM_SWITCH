// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.res.ResourcesCompat
import androidx.lifecycle.LifecycleOwner
import androidx.recyclerview.widget.RecyclerView
import org.yuzu.yuzu_emu.databinding.CardHomeOptionBinding
import org.yuzu.yuzu_emu.databinding.HeaderHomeCategoryBinding
import org.yuzu.yuzu_emu.fragments.MessageDialogFragment
import org.yuzu.yuzu_emu.model.HomeSetting
import org.yuzu.yuzu_emu.model.HomeSettingListItem
import org.yuzu.yuzu_emu.utils.ViewUtils.marquee
import org.yuzu.yuzu_emu.utils.ViewUtils.setVisible
import org.yuzu.yuzu_emu.utils.collect
import org.yuzu.yuzu_emu.viewholder.AbstractViewHolder

class HomeSettingAdapter(
    private val activity: AppCompatActivity,
    private val viewLifecycle: LifecycleOwner,
    items: List<HomeSettingListItem>
) : AbstractListAdapter<HomeSettingListItem, AbstractViewHolder<HomeSettingListItem>>(items) {

    constructor(
        activity: AppCompatActivity,
        viewLifecycle: LifecycleOwner,
        options: List<HomeSetting>,
        @Suppress("UNUSED_PARAMETER") legacyOptions: Boolean = true
    ) : this(
        activity,
        viewLifecycle,
        options.map { HomeSettingListItem.OptionItem(it) }
    )

    companion object {
        const val VIEW_TYPE_HEADER = 0
        const val VIEW_TYPE_ITEM = 1
    }

    override fun getItemViewType(position: Int): Int = when (currentList[position]) {
        is HomeSettingListItem.CategoryHeader -> VIEW_TYPE_HEADER
        is HomeSettingListItem.OptionItem -> VIEW_TYPE_ITEM
    }

    @Suppress("UNCHECKED_CAST")
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): AbstractViewHolder<HomeSettingListItem> {
        val inflater = LayoutInflater.from(parent.context)
        return when (viewType) {
            VIEW_TYPE_HEADER -> CategoryHeaderViewHolder(
                HeaderHomeCategoryBinding.inflate(inflater, parent, false)
            ) as AbstractViewHolder<HomeSettingListItem>
            else -> HomeOptionViewHolder(
                CardHomeOptionBinding.inflate(inflater, parent, false)
            ) as AbstractViewHolder<HomeSettingListItem>
        }
    }

    inner class CategoryHeaderViewHolder(val binding: HeaderHomeCategoryBinding) :
        AbstractViewHolder<HomeSettingListItem>(binding) {
        override fun bind(model: HomeSettingListItem) {
            val header = model as HomeSettingListItem.CategoryHeader
            binding.categoryTitle.setText(header.titleResId)
            binding.categoryIcon.setImageDrawable(
                ResourcesCompat.getDrawable(activity.resources, header.iconResId, activity.theme)
            )
        }
    }

    inner class HomeOptionViewHolder(val binding: CardHomeOptionBinding) :
        AbstractViewHolder<HomeSettingListItem>(binding) {
        override fun bind(model: HomeSettingListItem) {
            val item = (model as HomeSettingListItem.OptionItem).setting
            binding.optionTitle.text = activity.resources.getString(item.titleId)
            binding.optionDescription.text = activity.resources.getString(item.descriptionId)
            binding.optionIcon.setImageDrawable(
                ResourcesCompat.getDrawable(
                    activity.resources,
                    item.iconId,
                    activity.theme
                )
            )

            if (!item.isEnabled.invoke()) {
                binding.optionTitle.alpha = 0.5f
                binding.optionDescription.alpha = 0.5f
                binding.optionIcon.alpha = 0.5f
            } else {
                binding.optionTitle.alpha = 1.0f
                binding.optionDescription.alpha = 1.0f
                binding.optionIcon.alpha = 1.0f
            }

            item.details.collect(viewLifecycle) { updateOptionDetails(it) }
            binding.optionDetail.marquee()

            binding.root.setOnClickListener { onClick(item) }
        }

        private fun onClick(model: HomeSetting) {
            if (model.isEnabled.invoke()) {
                model.onClick.invoke()
            } else {
                MessageDialogFragment.newInstance(
                    activity,
                    titleId = model.disabledTitleId,
                    descriptionId = model.disabledMessageId
                ).show(activity.supportFragmentManager, MessageDialogFragment.TAG)
            }
        }

        private fun updateOptionDetails(detailString: String) {
            if (detailString.isNotEmpty()) {
                binding.optionDetail.text = detailString
                binding.optionDetail.setVisible(true)
            } else {
                binding.optionDetail.setVisible(false)
            }
        }
    }
}
