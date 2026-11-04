// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include "common/common_types.h"
#include "common/settings_enums.h"

namespace Core {

// Whitelist of Title IDs verified to work safely with guest RAM/CPU-only save states.
// Titles not in this list are blocked in "VerifiedOnly" mode to prevent game freezes or crashes.
inline constexpr std::array<u64, 2> VERIFIED_SAVE_STATE_TIDS = {
    // Super Mario Odyssey (tested memory-safe state serialization)
    0x0100000000010000ULL,
    // Sonic Mania (deterministic 2D title with no host-side async state drift)
    0x01009AA000FAA000ULL,
};

[[nodiscard]] constexpr bool IsTitleSaveStateVerified(u64 title_id) {
    if (title_id == 0) {
        return false;
    }
    for (const auto tid : VERIFIED_SAVE_STATE_TIDS) {
        if (tid == title_id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline bool CanUseSaveStates(u64 title_id, Settings::SaveStatesMode mode) {
    if (mode == Settings::SaveStatesMode::Off) {
        return false;
    }
    if (mode == Settings::SaveStatesMode::AllGames) {
        return true;
    }
    return IsTitleSaveStateVerified(title_id);
}

} // namespace Core
