// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/assert.h"
#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

class KernelCore;
class GlobalSchedulerContext;

template <typename SchedulerType>
class KAbstractSchedulerLock {
public:
    explicit KAbstractSchedulerLock(KernelCore& kernel) : m_kernel{kernel} {}

    static uintptr_t CurrentHostThreadToken() noexcept {
        static std::atomic<uintptr_t> s_next_host_token{1};
        thread_local const uintptr_t t_host_token =
            (s_next_host_token.fetch_add(1, std::memory_order_relaxed) << 1) | uintptr_t{1};
        return t_host_token;
    }

    uintptr_t CurrentOwnerToken() const noexcept {
        if (KThread* cur = GetCurrentThreadPointer(m_kernel); cur != nullptr) {
            return reinterpret_cast<uintptr_t>(cur);
        }
        return CurrentHostThreadToken();
    }

    bool IsLockedByCurrentThread() const {
        return m_owner_token.load(std::memory_order_relaxed) == CurrentOwnerToken();
    }

    void Lock() {
        const uintptr_t token = CurrentOwnerToken();
        if (m_owner_token.load(std::memory_order_relaxed) == token) {
            // If we already own the lock, the lock count should be > 0.
            // For debug, ensure this is true.
            ASSERT(m_lock_count > 0);
        } else {
            // Otherwise, we want to disable scheduling and acquire the spinlock.
            SchedulerType::DisableScheduling(m_kernel);
            m_spin_lock.Lock();

            ASSERT(m_lock_count == 0);
            ASSERT(m_owner_token.load(std::memory_order_relaxed) == 0);

            // Take ownership of the lock (non-zero token even for non-KThread host threads).
            m_owner_token.store(token, std::memory_order_relaxed);
        }

        // Increment the lock count.
        m_lock_count++;
    }

    void Unlock() {
        ASSERT(this->IsLockedByCurrentThread());
        ASSERT(m_lock_count > 0);

        // Release an instance of the lock.
        if ((--m_lock_count) == 0) {
            // Perform a memory barrier here.
            std::atomic_thread_fence(std::memory_order_seq_cst);

            // We're no longer going to hold the lock. Take note of what cores need scheduling.
            const u64 cores_needing_scheduling =
                SchedulerType::UpdateHighestPriorityThreads(m_kernel);

            // Note that we no longer hold the lock, and unlock the spinlock.
            m_owner_token.store(0, std::memory_order_relaxed);
            m_spin_lock.Unlock();

            // Enable scheduling, and perform a rescheduling operation.
            SchedulerType::EnableScheduling(m_kernel, cores_needing_scheduling);
        }
    }

private:
    friend class GlobalSchedulerContext;

    KernelCore& m_kernel;
    KAlignedSpinLock m_spin_lock{};
    s32 m_lock_count{};
    std::atomic<uintptr_t> m_owner_token{0};
};

} // namespace Kernel
