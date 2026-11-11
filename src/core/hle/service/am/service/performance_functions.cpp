// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/am/service/performance_functions.h"
#include "core/hle/service/ipc_helpers.h"

namespace Service::AM {

IPerformanceFunctions::IPerformanceFunctions(Core::System& system)
    : ServiceFramework{system, "IPerformanceFunctions"} {
    static const FunctionInfo functions[] = {
        {0, &IPerformanceFunctions::IsCpuOverclockEnabled, "IsCpuOverclockEnabled"},
        {1, &IPerformanceFunctions::SetCpuOverclockEnabled, "SetCpuOverclockEnabled"},
    };
    RegisterHandlers(functions);
}

IPerformanceFunctions::~IPerformanceFunctions() = default;

void IPerformanceFunctions::IsCpuOverclockEnabled(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(false);
}

void IPerformanceFunctions::SetCpuOverclockEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto cpu_overclock_enabled = rp.Pop<bool>();
    LOG_WARNING(Service_AM, "(STUBBED) called, cpu_overclock_enabled={}", cpu_overclock_enabled);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::AM
