// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.adapters

import android.content.Context
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import org.yuzu.yuzu_emu.utils.FreedrenoPreset
import org.yuzu.yuzu_emu.databinding.ListItemFreedrenoPresetBinding

/**
 * Adapter for displaying Freedreno preset configurations in a horizontal list.
 */
class FreedrenoPresetAdapter(
    private val onPresetClicked: (FreedrenoPreset) -> Unit
) : ListAdapter<FreedrenoPreset, FreedrenoPresetAdapter.PresetViewHolder>(DiffCallback) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): PresetViewHolder {
        val binding = ListItemFreedrenoPresetBinding.inflate(
            LayoutInflater.from(parent.context),
            parent,
            false
        )
        return PresetViewHolder(binding)
    }

    override fun onBindViewHolder(holder: PresetViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class PresetViewHolder(private val binding: ListItemFreedrenoPresetBinding) :
        RecyclerView.ViewHolder(binding.root) {

        fun bind(preset: FreedrenoPreset) {
            val ctx = binding.root.context
            val currentLang = ctx.resources.configuration.locales[0].language
            val isRu = currentLang.equals("ru", ignoreCase = true) || currentLang.equals("uk", ignoreCase = true) || currentLang.equals("be", ignoreCase = true)
            val locName = preset.getLocalizedName(isRu)
            val locDesc = preset.getLocalizedDescription(isRu)

            binding.presetButton.apply {
                text = locName
                setOnClickListener {
                    onPresetClicked(preset)
                }
                contentDescription = "$locName: $locDesc"
            }
        }
    }

    companion object {
        private val DiffCallback = object : DiffUtil.ItemCallback<FreedrenoPreset>() {
            override fun areItemsTheSame(oldItem: FreedrenoPreset, newItem: FreedrenoPreset): Boolean =
                oldItem.name == newItem.name

            override fun areContentsTheSame(oldItem: FreedrenoPreset, newItem: FreedrenoPreset): Boolean =
                oldItem == newItem
        }
    }
}
