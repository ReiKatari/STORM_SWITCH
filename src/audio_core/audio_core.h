#include <mutex>
#include <vector>
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "audio_core/adsp/adsp.h"
#include "audio_core/audio_manager.h"
#include "audio_core/sink/sink.h"

namespace Core {
class System;
}

namespace AudioCore {

namespace Renderer {
class SystemManager;
}

/// @brief Main audio class, stored inside the core, and holding the audio manager, all sinks, and the ADSP.
class AudioCore {
public:
    explicit AudioCore(Core::System& system);
    ~AudioCore();

    /**
     * Shutdown the audio core.
     */
    void Shutdown();

    /**
     * Resynchronize internal audio sessions and queues after savestate restore.
     */
    void ResynchronizeSaveState();

    /**
     * Register a SystemManager with the AudioCore.
     */
    void RegisterSystemManager(Renderer::SystemManager* manager);

    /**
     * Unregister a SystemManager from the AudioCore.
     */
    void UnregisterSystemManager(Renderer::SystemManager* manager);

    /**
     * Hold audio renderer execution during savestate capture or restore.
     */
    void HoldAudioRenderer();

    /**
     * Release audio renderer execution after savestate capture or restore.
     */
    void ReleaseAudioRenderer();

    /**
     * Get a reference to the audio manager.
     *
     * @return Ref to the audio manager.
     */
    AudioManager& GetAudioManager();

    /**
     * Get the audio output sink currently in use.
     *
     * @return Ref to the sink.
     */
    Sink::Sink& GetOutputSink();

    /**
     * Get the audio input sink currently in use.
     *
     * @return Ref to the sink.
     */
    Sink::Sink& GetInputSink();

    /// @brief Get the ADSP.
    /// @return Ref to the ADSP.
    ADSP::ADSP& ADSP();

private:
    /// @brief Create the sinks on startup.
    void CreateSinks();

    /// Main audio manager for audio in/out
    std::optional<AudioManager> audio_manager;
    /// Sink used for audio renderer and audio out
    std::unique_ptr<Sink::Sink> output_sink;
    /// Sink used for audio input
    std::unique_ptr<Sink::Sink> input_sink;
    /// The ADSP in the sysmodule
    std::optional<ADSP::ADSP> adsp;
    /// Registered system managers
    std::mutex system_manager_mutex{};
    std::vector<Renderer::SystemManager*> system_managers{};
};

} // namespace AudioCore
