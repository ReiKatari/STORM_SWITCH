// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/container_hash.h"

#if __has_include(<boost/unordered/unordered_flat_set.hpp>)
#include <boost/unordered/unordered_flat_set.hpp>
namespace Common {
template <class Key, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<Key>>
using unordered_set = boost::unordered::unordered_flat_set<Key, Hash, Pred, Allocator>;
}
#elif __has_include(<ankerl/unordered_dense.h>)
#include <ankerl/unordered_dense.h>
namespace Common {
template <class Key, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<Key>>
using unordered_set = ankerl::unordered_dense::set<Key, Hash, Pred, Allocator>;
}
#else
#include <unordered_set>
namespace Common {
template <class Key, class Hash = FastHash<Key>, class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<Key>>
using unordered_set = std::unordered_set<Key, Hash, Pred, Allocator>;
}
#endif
