// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/settings.h"
#include "core/hle/service/cmif_types.h"
#include "core/hle/service/nx2_stub_interface.h"
#include "core/hle/service/service.h"

namespace Service::AM {

// Switch 2 interface returned by IApplicationProxy command 60. nn::oe's
// GetCurrentGraphicsScale reads command 10 and accepts 10, 20 or 30 (scale enum 0, 1, 2).
// Tier 0 (10) = Handheld, Tier 1 (20) = Console / Docked, Tier 2 (30) = Ultra/Enhanced.
// Commands without a handler are auto-stubbed because the name carries the nx2: prefix.
class IGraphicsScaleController final : public ServiceFramework<IGraphicsScaleController> {
public:
    explicit IGraphicsScaleController(Core::System& system_)
        : ServiceFramework{system_, "nx2:IGraphicsScaleController"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {10, D<&IGraphicsScaleController::GetCurrentGraphicsScale>, "GetCurrentGraphicsScale"},
            {20, D<&IGraphicsScaleController::SetGraphicsScaleChangedNotificationEnabled>, "SetGraphicsScaleChangedNotificationEnabled"},
            {1050, D<&IGraphicsScaleController::Unknown1050>, "Unknown1050"},
        };
        // clang-format on
        RegisterHandlers(functions);
    }

private:
    Result GetCurrentGraphicsScale(Out<u32> out_scale) {
        const bool is_docked = Settings::IsDockedMode();
        *out_scale = is_docked ? 20 : 10;
        LOG_DEBUG(Service_AM, "called, scale={} ({})", *out_scale, is_docked ? "Console" : "Handheld");
        R_SUCCEED();
    }

    Result SetGraphicsScaleChangedNotificationEnabled(bool enabled) {
        LOG_DEBUG(Service_AM, "called, enabled={}", enabled);
        R_SUCCEED();
    }

    Result Unknown1050() {
        LOG_DEBUG(Service_AM, "Unknown1050 called");
        R_SUCCEED();
    }
};

} // namespace Service::AM
