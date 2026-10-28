// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace Shader::Ampere {

void ClearLoweringCache() {
    std::scoped_lock lk(s_ampere_lower_mutex);
    s_ampere_lower_cache.clear();
}

size_t GetLoweringCacheSize() {
    std::scoped_lock lk(s_ampere_lower_mutex);
    return s_ampere_lower_cache.size();
}

} // namespace Shader::Ampere
