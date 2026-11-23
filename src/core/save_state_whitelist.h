// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include "common/common_types.h"
#include "common/settings_enums.h"

namespace Core {

// Whitelist of Title IDs verified to work safely with guest RAM/CPU-only save states.
// Titles not in this list are blocked in "VerifiedOnly" mode to prevent game freezes or crashes.
inline constexpr std::array<u64, 10> VERIFIED_SAVE_STATE_TIDS = {
    // Super Mario Odyssey
    0x0100000000010000ULL,
    // Sonic Mania
    0x01009AA000FAA000ULL,
    // Xeno Crisis
    0x010013F009B88000ULL,
    // Super Mario Bros. Wonder
    0x010015100B514000ULL,
    // Celeste
    0x01002B30028F6000ULL,
    // Hollow Knight
    0x0100633007D48000ULL,
    // Hades
    0x0100535012974000ULL,
    // Metroid Dread
    0x010093801237C000ULL,
    // Dead Cells
    0x0100646009FBE000ULL,
    // Animal Well
    0x010020D01AD24000ULL,
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
