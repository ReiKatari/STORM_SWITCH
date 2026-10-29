// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>

#include "audio_core/adsp/adsp.h"
#include "audio_core/audio_core.h"
#include "audio_core/renderer/system_manager.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"

namespace AudioCore::Renderer {

SystemManager::SystemManager(Core::System& core_)
    : core{core_}, audio_renderer{core.AudioCore().ADSP().AudioRenderer()} {}

SystemManager::~SystemManager() {
    Stop();
}

void SystemManager::InitializeUnsafe() {
    if (!active) {
        active = true;
        audio_renderer.Start();
        thread = std::jthread([this](std::stop_token stop_token) {
            Common::SetCurrentThreadName("AudioRenderSystemManager");
            Common::SetCurrentThreadPriority(Common::ThreadPriority::High);
            Common::SetCurrentThreadToEfficiencyCores();
            while (active && !stop_token.stop_requested()) {
                {
                    std::unique_lock lk{hold_mutex};
                    hold_cv.wait(lk, [this, &stop_token] {
                        return !held || !active || stop_token.stop_requested();
                    });
                }
                if (!active || stop_token.stop_requested()) {
                    break;
                }
                {
                    std::scoped_lock l{mutex1};
                    for (auto system : systems)
                        system->SendCommandToDsp();
                }
                audio_renderer.Signal();
                audio_renderer.Wait();
            }
        });
    }
}

void SystemManager::Stop() {
    if (active) {
        active = false;
        {
            std::scoped_lock lk{hold_mutex};
            held = false;
        }
        hold_cv.notify_all();
        thread.request_stop();
        thread.join();
        audio_renderer.Stop();
    }
}

void SystemManager::Hold() {
    std::scoped_lock lk{hold_mutex};
    held = true;
}

void SystemManager::Release() {
    {
        std::scoped_lock lk{hold_mutex};
        held = false;
    }
    hold_cv.notify_all();
}

void SystemManager::Resynchronize() {
    std::scoped_lock l{mutex1};
    for (auto* sys : systems) {
        if (sys) {
            sys->Resynchronize();
        }
    }
}

bool SystemManager::Add(System& system_) {
    std::scoped_lock l2{mutex2};
    if (systems.size() + 1 > MaxRendererSessions) {
        LOG_ERROR(Service_Audio, "Maximum AudioRenderer Systems active, cannot add more!");
        return false;
    }
    {
        std::scoped_lock l{mutex1};
        if (systems.empty())
            InitializeUnsafe();
    }
    systems.push_back(&system_);
    return true;
}

bool SystemManager::Remove(System& system_) {
    std::scoped_lock l2{mutex2};
    {
        std::scoped_lock l{mutex1};
        if (systems.remove(&system_) == 0) {
            LOG_ERROR(Service_Audio, "Failed to remove a render system, it was not found in the list!");
            return false;
        }
    }

    if (systems.empty())
        Stop();
    return true;
}

} // namespace AudioCore::Renderer
