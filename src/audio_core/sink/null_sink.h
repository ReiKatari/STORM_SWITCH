// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "audio_core/common/common.h"
#include "audio_core/sink/sink.h"
#include "audio_core/sink/sink_stream.h"
#include "core/core.h"
#include "core/core_timing.h"

namespace Core {
class System;
} // namespace Core

namespace AudioCore::Sink {
class NullSinkStreamImpl final : public SinkStream {
public:
    explicit NullSinkStreamImpl(Core::System& system_, StreamType type_)
        : SinkStream{system_, type_}, start_time{system_.CoreTiming().GetGlobalTimeNs()} {}
    ~NullSinkStreamImpl() override = default;

    void AppendBuffer(SinkBuffer& buffer, std::span<s16>) override {
        buffer.consumed = true;
        played_sample_count += buffer.frames;
    }

    std::vector<s16> ReleaseBuffer(u64 num_samples) override {
        return std::vector<s16>(num_samples, 0);
    }

    u64 GetExpectedPlayedSampleCount() override {
        const auto cur_time{system.CoreTiming().GetGlobalTimeNs()};
        const auto time_delta{cur_time - start_time};
        const u64 time_based_samples{
            static_cast<u64>((TargetSampleRate * time_delta) / std::chrono::seconds{1})};
        return std::max<u64>(time_based_samples, played_sample_count) + TargetSampleCount * 5;
    }

private:
    std::chrono::nanoseconds start_time{};
    u64 played_sample_count{0};
};

/**
 * A no-op sink for when no audio out is wanted.
 */
class NullSink final : public Sink {
public:
    explicit NullSink(std::string_view) {}
    ~NullSink() override = default;

    SinkStream* AcquireSinkStream(Core::System& system, u32, const std::string&,
                                  StreamType type) override {
        if (null_sink == nullptr) {
            null_sink = std::make_unique<NullSinkStreamImpl>(system, type);
        }
        return null_sink.get();
    }

    void CloseStream(SinkStream*) override {}
    void CloseStreams() override {}
    void ClearStreams() override {}
    f32 GetDeviceVolume() const override {
        return 1.0f;
    }
    void SetDeviceVolume(f32 volume) override {}
    void SetSystemVolume(f32 volume) override {}

private:
    SinkStreamPtr null_sink{};
};

} // namespace AudioCore::Sink
