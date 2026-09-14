// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.overlay.model

data class OverlayControlData(
    val id: String,
    var enabled: Boolean,
    var landscapePosition: Pair<Double, Double>,
    var portraitPosition: Pair<Double, Double>,
    var foldablePosition: Pair<Double, Double>,
    var individualScale: Float,
    var individualOpacity: Float = 1.0f
) {
    constructor(
        id: String,
        enabled: Boolean,
        landscapePosition: Pair<Double, Double>,
        portraitPosition: Pair<Double, Double>,
        foldablePosition: Pair<Double, Double>,
        individualScale: Float
    ) : this(id, enabled, landscapePosition, portraitPosition, foldablePosition, individualScale, 1.0f)
    fun positionFromLayout(layout: OverlayLayout): Pair<Double, Double> =
        when (layout) {
            OverlayLayout.Landscape -> landscapePosition
            OverlayLayout.Portrait -> portraitPosition
            OverlayLayout.Foldable -> foldablePosition
        }
}
