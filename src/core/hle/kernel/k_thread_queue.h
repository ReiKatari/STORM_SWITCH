// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"

namespace Kernel {

class KHardwareTimer;

class KThreadQueue {
public:
    explicit KThreadQueue(KernelCore& kernel) : m_kernel{&kernel}, m_hardware_timer{} {}
    virtual ~KThreadQueue();

    void SetHardwareTimer(KHardwareTimer* timer) {
        m_hardware_timer = timer;
    }

    void SetBoundThread(KThread* thread) noexcept {
        m_bound_thread = thread;
    }

    void ClearBoundThread(KThread* thread) noexcept {
        if (m_bound_thread == thread) {
            m_bound_thread = nullptr;
        }
    }

    virtual void NotifyAvailable(KernelCore& kernel, KThread* waiting_thread, KSynchronizationObject* signaled_object, Result wait_result);
    virtual void EndWait(KernelCore& kernel, KThread* waiting_thread, Result wait_result);
    virtual void CancelWait(KernelCore& kernel, KThread* waiting_thread, Result wait_result, bool cancel_timer_task);

private:
    KernelCore* m_kernel{};
    KThread* m_bound_thread{};
    KHardwareTimer* m_hardware_timer{};
};

class KThreadQueueWithoutEndWait : public KThreadQueue {
public:
    explicit KThreadQueueWithoutEndWait(KernelCore& kernel) : KThreadQueue(kernel) {}
    void EndWait(KernelCore& kernel, KThread* waiting_thread, Result wait_result) override final;
};

} // namespace Kernel
