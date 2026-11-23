// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <ankerl/unordered_dense.h>

#include "video_core/dma_pusher.h"

namespace Tegra {

class GPU;

namespace Control {

struct ChannelState;

class Scheduler {
public:
    void Push(GPU& gpu, s32 channel, CommandList&& entries);

    bool ResumePending(GPU& gpu);
    [[nodiscard]] bool HasPendingNativeChannels() const {
        return has_pending_native.load(std::memory_order_acquire);
    }

    void DeclareChannel(std::shared_ptr<ChannelState> new_channel);

private:
    ankerl::unordered_dense::map<s32, std::shared_ptr<ChannelState>> channels;
    mutable std::mutex scheduling_guard;
    std::atomic<bool> has_pending_native{false};
    std::vector<std::shared_ptr<ChannelState>> pending_native_channels;
};

} // namespace Control

} // namespace Tegra
