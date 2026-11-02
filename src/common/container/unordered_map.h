// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/container_hash.h"

#if __has_include(<boost/unordered/unordered_flat_map.hpp>)
#include <boost/unordered/unordered_flat_map.hpp>
namespace Common {
template <class Key, class T, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using unordered_map = boost::unordered::unordered_flat_map<Key, T, Hash, Pred, Allocator>;
}
#elif __has_include(<ankerl/unordered_dense.h>)
#include <ankerl/unordered_dense.h>
namespace Common {
template <class Key, class T, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using unordered_map = ankerl::unordered_dense::map<Key, T, Hash, Pred, Allocator>;
}
#else
#include <unordered_map>
namespace Common {
template <class Key, class T, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using unordered_map = std::unordered_map<Key, T, Hash, Pred, Allocator>;
}
#endif
