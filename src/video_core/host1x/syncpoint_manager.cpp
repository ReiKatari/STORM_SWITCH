// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <vector>
#include "common/logging.h"
#include "common/settings.h"
#include "video_core/host1x/syncpoint_manager.h"

namespace Tegra {

namespace Host1x {

SyncpointManager::ActionHandle SyncpointManager::RegisterAction(
    std::atomic<u32>& syncpoint, std::list<RegisteredAction>& action_storage, u32 expected_value,
    std::function<void()>&& action) {
    if (syncpoint.load(std::memory_order_acquire) >= expected_value) {
        action();
        return {};
    }

    std::unique_lock lk(guard);
    if (syncpoint.load(std::memory_order_relaxed) >= expected_value) {
        lk.unlock();
        action();
        return {};
    }
    auto it = action_storage.begin();
    while (it != action_storage.end()) {
        if (it->expected_value >= expected_value) {
            break;
        }
        ++it;
    }
    return action_storage.emplace(it, expected_value, std::move(action));
}

void SyncpointManager::DeregisterAction(std::list<RegisteredAction>& action_storage,
                                        const ActionHandle& handle) {
    std::scoped_lock lk(guard);

    for (auto it = action_storage.begin(); it != action_storage.end(); it++) {
        if (it == handle) {
            action_storage.erase(it);
            return;
        }
    }
}

void SyncpointManager::DeregisterGuestAction(u32 syncpoint_id, const ActionHandle& handle) {
    DeregisterAction(guest_action_storage[syncpoint_id], handle);
}

void SyncpointManager::DeregisterHostAction(u32 syncpoint_id, const ActionHandle& handle) {
    DeregisterAction(host_action_storage[syncpoint_id], handle);
}

void SyncpointManager::IncrementGuest(u32 syncpoint_id) {
    Increment(syncpoints_guest[syncpoint_id], wait_guest_cv, guest_action_storage[syncpoint_id]);
}

void SyncpointManager::IncrementHost(u32 syncpoint_id) {
    Increment(syncpoints_host[syncpoint_id], wait_host_cv, host_action_storage[syncpoint_id]);
}

void SyncpointManager::WaitGuest(u32 syncpoint_id, u32 expected_value) {
    Wait(syncpoints_guest[syncpoint_id], wait_guest_cv, expected_value);
}

void SyncpointManager::WaitHost(u32 syncpoint_id, u32 expected_value) {
    Wait(syncpoints_host[syncpoint_id], wait_host_cv, expected_value);
}

void SyncpointManager::FastForwardSyncpoint(u32 syncpoint_id, u32 target_value) {
    if (syncpoint_id >= NUM_MAX_SYNCPOINTS || target_value == 0) {
        return;
    }
    auto advance = [&](std::atomic<u32>& sp, std::condition_variable& wait_cv,
                       std::list<RegisteredAction>& action_storage) {
        u32 cur = sp.load(std::memory_order_acquire);
        while (cur < target_value &&
               !sp.compare_exchange_weak(cur, target_value, std::memory_order_acq_rel)) {
        }
        const u32 new_val = sp.load(std::memory_order_acquire);
        std::vector<std::function<void()>> ready_actions;
        {
            std::scoped_lock lk(guard);
            auto it = action_storage.begin();
            while (it != action_storage.end()) {
                if (it->expected_value > new_val) {
                    break;
                }
                ready_actions.push_back(std::move(it->action));
                it = action_storage.erase(it);
            }
            wait_cv.notify_all();
        }
        for (auto& act : ready_actions) {
            if (act) {
                act();
            }
        }
    };
    advance(syncpoints_guest[syncpoint_id], wait_guest_cv, guest_action_storage[syncpoint_id]);
    advance(syncpoints_host[syncpoint_id], wait_host_cv, host_action_storage[syncpoint_id]);
}

void SyncpointManager::Increment(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv,
                                 std::list<RegisteredAction>& action_storage) {
    auto new_value{syncpoint.fetch_add(1, std::memory_order_acq_rel) + 1};

    std::vector<std::function<void()>> ready_actions;
    {
        std::scoped_lock lk(guard);
        auto it = action_storage.begin();
        while (it != action_storage.end()) {
            if (it->expected_value > new_value) {
                break;
            }
            ready_actions.push_back(std::move(it->action));
            it = action_storage.erase(it);
        }
        wait_cv.notify_all();
    }

    for (auto& act : ready_actions) {
        if (act) {
            act();
        }
    }
}

void SyncpointManager::Wait(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv,
                            u32 expected_value) {
    const auto pred = [&]() { return syncpoint.load(std::memory_order_acquire) >= expected_value; };
    if (pred()) {
        return;
    }

    const auto behavior = Settings::values.gpu_fence_behavior.GetValue();
    if (behavior == Settings::GpuFenceBehavior::Immediate) {
        return;
    }

    std::chrono::milliseconds wait_interval{5000};
    u32 max_timeouts = 6;
    switch (behavior) {
    case Settings::GpuFenceBehavior::Default:
        wait_interval = std::chrono::milliseconds{1000};
        max_timeouts = 3;
        break;
    case Settings::GpuFenceBehavior::Balanced:
        wait_interval = std::chrono::milliseconds{2000};
        max_timeouts = 5;
        break;
    case Settings::GpuFenceBehavior::Accurate:
        wait_interval = std::chrono::milliseconds{5000};
        max_timeouts = 6;
        break;
    case Settings::GpuFenceBehavior::Strict:
        wait_interval = std::chrono::milliseconds{10000};
        max_timeouts = 12;
        break;
    default:
        break;
    }

    std::unique_lock lk(guard);
    u32 timeout_count = 0;
    while (!pred()) {
        if (!wait_cv.wait_for(lk, wait_interval, pred)) {
            LOG_WARNING(HW_GPU, "Syncpoint wait timed out (attempt {})! expected={}, current={}",
                        ++timeout_count, expected_value, syncpoint.load(std::memory_order_relaxed));
            if (timeout_count >= max_timeouts) {
                LOG_CRITICAL(HW_GPU, "Syncpoint wait deadlocked! expected={}, current={}",
                             expected_value, syncpoint.load(std::memory_order_relaxed));
                break;
            }
        }
    }
}

} // namespace Host1x

} // namespace Tegra
