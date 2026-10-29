// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/renderer/system_manager.h"
#include "audio_core/sink/sink_details.h"
#include <algorithm>
#include "common/settings.h"
#include "core/core.h"

namespace AudioCore {

AudioCore::AudioCore(Core::System& system) {
    audio_manager.emplace();
    CreateSinks();
    // Must be created after the sinks
    adsp.emplace(system, *output_sink);
}

AudioCore ::~AudioCore() {
    Shutdown();
}

void AudioCore::CreateSinks() {
    const auto& sink_id{Settings::values.sink_id};
    const auto& audio_output_device_id{Settings::values.audio_output_device_id};
    const auto& audio_input_device_id{Settings::values.audio_input_device_id};

    output_sink = Sink::CreateSinkFromID(sink_id.GetValue(), audio_output_device_id.GetValue());
    input_sink = Sink::CreateSinkFromID(sink_id.GetValue(), audio_input_device_id.GetValue());
}

void AudioCore::Shutdown() {
    audio_manager->Shutdown();
}

void AudioCore::ResynchronizeSaveState() {
    if (output_sink) {
        output_sink->ClearStreams();
    }
    if (input_sink) {
        input_sink->ClearStreams();
    }
    if (adsp) {
        adsp->AudioRenderer().ClearAllRemainCommandCounts();
    }
}

void AudioCore::RegisterSystemManager(Renderer::SystemManager* manager) {
    std::scoped_lock lk{system_manager_mutex};
    system_managers.push_back(manager);
}

void AudioCore::UnregisterSystemManager(Renderer::SystemManager* manager) {
    std::scoped_lock lk{system_manager_mutex};
    std::erase(system_managers, manager);
}

void AudioCore::HoldAudioRenderer() {
    std::scoped_lock lk{system_manager_mutex};
    for (auto* sm : system_managers) {
        if (sm) {
            sm->Hold();
        }
    }
}

void AudioCore::ReleaseAudioRenderer() {
    std::scoped_lock lk{system_manager_mutex};
    for (auto* sm : system_managers) {
        if (sm) {
            sm->Release();
        }
    }
}

AudioManager& AudioCore::GetAudioManager() {
    return *audio_manager;
}

Sink::Sink& AudioCore::GetOutputSink() {
    return *output_sink;
}

Sink::Sink& AudioCore::GetInputSink() {
    return *input_sink;
}

ADSP::ADSP& AudioCore::ADSP() {
    return *adsp;
}

} // namespace AudioCore
