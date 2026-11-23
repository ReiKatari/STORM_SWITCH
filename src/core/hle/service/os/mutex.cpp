// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/service/os/mutex.h"

namespace Service {

Mutex::Mutex(Core::System& system) : m_system(system) {
    m_event = Kernel::KEvent::Create(system.Kernel());
    m_event->Initialize(system.Kernel(), nullptr);

    // Register the event.
    Kernel::KEvent::Register(system.Kernel(), m_event);

    ASSERT(R_SUCCEEDED(m_event->Signal(system.Kernel())));
}

Mutex::~Mutex() {
    m_event->GetReadableEvent().Close(m_system.Kernel());
    m_event->Close(m_system.Kernel());
}

void Mutex::lock() {
    // Infinitely retry until we successfully clear the event (unless shutting down).
    while (R_FAILED(m_event->GetReadableEvent().Reset(m_system.Kernel()))) {
        if (m_system.Kernel().IsShuttingDown()) {
            return;
        }
        s32 index;
        Kernel::KSynchronizationObject* obj = &m_event->GetReadableEvent();

        // The event was already cleared!
        // Wait for it to become signaled again.
        const Result res =
            Kernel::KSynchronizationObject::Wait(m_system.Kernel(), &index, &obj, 1, -1);
        if (res == Kernel::ResultTerminationRequested || m_system.Kernel().IsShuttingDown()) {
            return;
        }
        ASSERT(R_SUCCEEDED(res));
    }

    // We successfully cleared the event, and now have exclusive ownership.
}

void Mutex::unlock() {
    // Unlock.
    ASSERT(R_SUCCEEDED(m_event->Signal(m_system.Kernel())));
}

} // namespace Service
