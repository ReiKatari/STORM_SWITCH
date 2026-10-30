// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace Shader::Ampere {

void ClearLoweringCache() {
    for (auto& shard : s_ampere_lower_shards) {
        shard.Clear();
    }
}

size_t GetLoweringCacheSize() {
    size_t total = 0;
    for (const auto& shard : s_ampere_lower_shards) {
        total += shard.Size();
    }
    return total;
}

} // namespace Shader::Ampere
