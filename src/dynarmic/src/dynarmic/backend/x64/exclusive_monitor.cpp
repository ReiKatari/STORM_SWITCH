// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/interface/exclusive_monitor.h"

#include <algorithm>

#include "common/assert.h"

namespace Dynarmic {

ExclusiveMonitor::ExclusiveMonitor(std::size_t processor_count)
        : reservations(processor_count) {}

size_t ExclusiveMonitor::GetProcessorCount() const {
    return reservations.size();
}

void ExclusiveMonitor::Lock() {
    lock.Lock();
}

void ExclusiveMonitor::Unlock() {
    lock.Unlock();
}

bool ExclusiveMonitor::CheckAndClear(std::size_t processor_id, VAddr address) {
    const VAddr masked_address = address & RESERVATION_GRANULE_MASK;
    Lock();
    if (reservations[processor_id].address != masked_address) {
        Unlock();
        return false;
    }
    for (Reservation& other : reservations)
        if (other.address == masked_address)
            other.address = INVALID_EXCLUSIVE_ADDRESS;
    return true;
}

void ExclusiveMonitor::Clear() {
    Lock();
    for (Reservation& other : reservations) {
        other.address = INVALID_EXCLUSIVE_ADDRESS;
    }
    Unlock();
}

void ExclusiveMonitor::ClearProcessor(size_t processor_id) {
    Lock();
    reservations[processor_id].address = INVALID_EXCLUSIVE_ADDRESS;
    Unlock();
}

}  // namespace Dynarmic
