// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <QString>
#include "common/common_types.h"

namespace ConfigurationShared {

struct SettingConflict {
    std::string rule_id;
    QString title;
    QString message;
    std::function<void()> apply_fix;

    QString LocalizedTitle(bool is_ru = false) const {
        return title;
    }
    QString LocalizedMessage(bool is_ru = false) const {
        return message;
    }
};

class SettingConflicts {
public:
    static std::vector<SettingConflict> DetectConflicts(bool is_per_game = false, u64 title_id = 0);
    static bool HasConflicts(bool is_per_game = false, u64 title_id = 0);
};

} // namespace ConfigurationShared
