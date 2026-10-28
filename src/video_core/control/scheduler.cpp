// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <memory>
#include <vector>

#include "common/assert.h"
#include "video_core/control/channel_state.h"
#include "video_core/control/scheduler.h"
#include "video_core/gpu.h"

namespace Tegra::Control {

void Scheduler::Push(GPU& gpu, s32 channel, CommandList&& entries) {
    std::shared_ptr<ChannelState> channel_state;
    {
        std::unique_lock lk(scheduling_guard);
        auto it = channels.find(channel);
        ASSERT(it != channels.end());
        channel_state = it->second;
        gpu.BindChannel(channel_state->bind_id);
    }
    // Process commands outside the lock to reduce contention.
    // Multiple channels can prepare their commands in parallel.
    if (!channel_state || !channel_state->payload) {
        return;
    }
    channel_state->payload->dma_pusher.Push(std::move(entries));
    channel_state->payload->dma_pusher.DispatchCalls();
    if (channel_state->payload->dma_pusher.HasPendingNativeAcquire()) {
        std::unique_lock lk(scheduling_guard);
        if (std::find(pending_native_channels.begin(), pending_native_channels.end(), channel_state) == pending_native_channels.end()) {
            pending_native_channels.push_back(channel_state);
        }
    }
    ResumePending(gpu);
}

void Scheduler::ResumePending(GPU& gpu) {
    std::vector<std::shared_ptr<ChannelState>> pending_copy;
    {
        std::unique_lock lk(scheduling_guard);
        if (pending_native_channels.empty()) {
            return;
        }
        pending_copy = pending_native_channels;
    }
    ResumeReadyNativeChannels(pending_copy,
        [](const auto& state) {
            return state && state->payload &&
                   state->payload->dma_pusher.HasPendingNativeAcquire() &&
                   state->payload->dma_pusher.CanResumeNativeAcquire();
        },
        [&](const auto& state) {
            if (!state || !state->payload) return;
            {
                std::scoped_lock lock{scheduling_guard};
                gpu.BindChannel(state->bind_id);
            }
            state->payload->dma_pusher.DispatchCalls();
        });
    {
        std::unique_lock lk(scheduling_guard);
        std::erase_if(pending_native_channels,
            [](const auto& state) {
                return !state || !state->payload || !state->payload->dma_pusher.HasPendingNativeAcquire();
            });
    }
}

void Scheduler::DeclareChannel(std::shared_ptr<ChannelState> new_channel) {
    s32 channel = new_channel->bind_id;
    std::unique_lock lk(scheduling_guard);
    channels.emplace(channel, new_channel);
}

} // namespace Tegra::Control
