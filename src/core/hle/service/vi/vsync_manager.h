// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <set>

namespace Kernel {
class KernelCore;
}

namespace Service {
class Event;
}

namespace Service::VI {

class DisplayList;

class VsyncManager {
public:
    explicit VsyncManager();
    ~VsyncManager();

    VsyncManager(VsyncManager&&) noexcept = default;
    VsyncManager& operator=(VsyncManager&&) noexcept = default;
    VsyncManager(const VsyncManager&) = delete;
    VsyncManager& operator=(const VsyncManager&) = delete;

    void SignalVsync(Kernel::KernelCore& kernel);
    void LinkVsyncEvent(Event* event) {
        std::scoped_lock lk{GetVsyncMutex()};
        m_vsync_events.insert(event);
    }
    void UnlinkVsyncEvent(Event* event) {
        std::scoped_lock lk{GetVsyncMutex()};
        m_vsync_events.erase(event);
    }

private:
    static std::mutex& GetVsyncMutex() {
        static std::mutex s_mutex;
        return s_mutex;
    }
    std::set<Event*> m_vsync_events;
};

} // namespace Service::VI
