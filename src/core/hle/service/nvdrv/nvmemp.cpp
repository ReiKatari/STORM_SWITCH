// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging.h"
#include "core/core.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "core/hle/service/nvdrv/nvmemp.h"

namespace Service::Nvidia {

NVMEMP::NVMEMP(Core::System& system_) : ServiceFramework{system_, "nvmemp"} {
    static const FunctionInfo functions[] = {
        {0, &NVMEMP::Open, "Open"},
        {1, &NVMEMP::GetAruid, "GetAruid"},
    };
    RegisterHandlers(functions);
}

NVMEMP::~NVMEMP() = default;

void NVMEMP::Open(HLERequestContext& ctx) {
    LOG_WARNING(Service_NVDRV, "(STUBBED) NVMEMP::Open called");
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<u32>(0);
    rb.PushEnum(NvResult::NotSupported);
}

void NVMEMP::GetAruid(HLERequestContext& ctx) {
    LOG_WARNING(Service_NVDRV, "(STUBBED) NVMEMP::GetAruid called");
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<u64>(system.GetApplicationProcessProgramID());
}

} // namespace Service::Nvidia
