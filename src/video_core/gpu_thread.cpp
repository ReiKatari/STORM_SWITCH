// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <thread>

#include "common/assert.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/frontend/graphics_context.h"
#include "video_core/control/scheduler.h"
#include "video_core/dma_pusher.h"
#include "video_core/gpu.h"
#include "video_core/gpu_thread.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/host1x/host1x.h"
#include "video_core/renderer_base.h"

namespace VideoCommon::GPUThread {

ThreadManager::ThreadManager(Core::System& system_)
    : system{system_}
{}

ThreadManager::~ThreadManager() = default;

void ThreadManager::StartThread(VideoCore::RendererBase& renderer, Core::Frontend::GraphicsContext& context, Tegra::Control::Scheduler& scheduler) {
    shutting_down.store(false, std::memory_order_release);
    rasterizer.store(renderer.ReadRasterizer(), std::memory_order_release);
    thread = std::jthread([&](std::stop_token stop_token) {
        Common::SetCurrentThreadName("GPU");
#ifdef __ANDROID__
        Common::SetCurrentThreadPriority(Common::ThreadPriority::High);
#else
        Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
#endif
        system.RegisterHostThread();

        auto current_context = context.Acquire();
        CommandDataContainer next;
        u32 pending_idle_polls = 0;
        while (!stop_token.stop_requested()) {
            if (scheduler.HasPendingNativeChannels()) {
                const bool resumed = scheduler.ResumePending(system.GPU());
                if (resumed) {
                    pending_idle_polls = 0;
                }
                if (scheduler.HasPendingNativeChannels()) {
                    const auto wait_us = std::chrono::microseconds{
                        resumed ? 250U : (std::min)(16000U, 500U << (std::min)(pending_idle_polls, 5U))};
                    if (!state.queue.TryPopWait(next, wait_us, stop_token)) {
                        ++pending_idle_polls;
                        continue;
                    }
                    pending_idle_polls = 0;
                } else {
                    pending_idle_polls = 0;
                    if (!state.queue.TryPopWait(next, std::chrono::milliseconds{16}, stop_token)) {
                        continue;
                    }
                }
            } else {
                pending_idle_polls = 0;
                if (!state.queue.TryPopWait(next, std::chrono::milliseconds{16}, stop_token)) {
                    continue;
                }
            }
            if (stop_token.stop_requested()) {
                break;
            }
            if (auto* submit_list = std::get_if<SubmitListCommand>(&next.data)) {
                scheduler.Push(system.GPU(), submit_list->channel, std::move(submit_list->entries));
            } else if (std::holds_alternative<GPUTickCommand>(next.data)) {
                system.GPU().TickWork();
            } else if (const auto* flush = std::get_if<FlushRegionCommand>(&next.data)) {
                renderer.ReadRasterizer()->FlushRegion(flush->addr, flush->size);
            } else if (const auto* invalidate = std::get_if<InvalidateRegionCommand>(&next.data)) {
                renderer.ReadRasterizer()->OnCacheInvalidation(invalidate->addr, invalidate->size);
            } else {
                LOG_WARNING(HW_GPU, "Unknown GPU thread command variant index={}", next.data.index());
            }
            state.signaled_fence.store(next.fence);
            if (next.block) {
                // We have to lock the write_lock to ensure that the condition_variable wait not get a
                // race between the check and the lock itself.
                std::scoped_lock lk{state.write_lock};
                state.cv.notify_all();
            }
        }
    });
}

void ThreadManager::SubmitList(s32 channel, Tegra::CommandList&& entries, bool is_async) {
    PushCommand(SubmitListCommand(channel, std::move(entries)), false, is_async);
}

void ThreadManager::FlushRegion(DAddr addr, u64 size, bool is_async) {
    if (!is_async) {
        // Always flush with synchronous GPU mode
        PushCommand(FlushRegionCommand(addr, size), false, is_async);
    }
}

void ThreadManager::TickGPU(bool is_async) {
    PushCommand(GPUTickCommand(), false, is_async);
}

void ThreadManager::InvalidateRegion(DAddr addr, u64 size) {
    if (shutting_down.load(std::memory_order_acquire)) {
        return;
    }
    if (auto* const rast = rasterizer.load(std::memory_order_acquire)) {
        rast->OnCacheInvalidation(addr, size);
    }
}


void ThreadManager::FlushAndInvalidateRegion(DAddr addr, u64 size, bool is_async) {
    if (shutting_down.load(std::memory_order_acquire)) {
        return;
    }
    auto* const rast = rasterizer.load(std::memory_order_acquire);
    if (!rast) {
        return;
    }
    if ((Settings::IsGPULevelNormal() || Settings::IsGPULevelHigh() || Settings::values.use_reactive_flushing.GetValue()) &&
        rast->MustFlushRegion(addr, size)) {
        PushCommand(FlushRegionCommand(addr, size), true, is_async);
    }
    if (!shutting_down.load(std::memory_order_acquire)) {
        rast->OnCacheInvalidation(addr, size);
    }
}

u64 ThreadManager::PushCommand(CommandData&& command_data, bool block, bool is_async) {
    if (!is_async) {
        // In synchronous GPU mode, block the caller until the command has executed
        block = true;
    }

    std::unique_lock lk(state.write_lock);
    const u64 fence{++state.last_fence};
    state.queue.EmplaceWait(std::move(command_data), fence, block);

    if (block) {
        state.cv.wait(lk, thread.get_stop_token(), [this, fence] {
            return fence <= state.signaled_fence.load(std::memory_order_relaxed);
        });
    }

    return fence;
}

void ThreadManager::NotifyShutdown() {
    shutting_down.store(true, std::memory_order_release);
    rasterizer.store(nullptr, std::memory_order_release);
    if (thread.joinable()) {
        thread.request_stop();
        thread.join();
    }
}

} // namespace VideoCommon::GPUThread
