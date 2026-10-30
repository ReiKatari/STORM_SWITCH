// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.model

sealed class HomeSettingListItem {
    data class CategoryHeader(
        val categoryId: Int,
        val titleResId: Int,
        val iconResId: Int,
        val subtitleResId: Int = 0
    ) : HomeSettingListItem()

    data class OptionItem(
        val setting: HomeSetting
    ) : HomeSettingListItem()
}
