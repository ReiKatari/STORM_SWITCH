// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging.h"
#include "core/file_sys/errors.h"
#include "core/hle/service/filesystem/fsp/fsp_ldr.h"
#include "core/hle/service/ipc_helpers.h"

namespace Service::FileSystem {

FSP_LDR::FSP_LDR(Core::System& system_) : ServiceFramework{system_, "fsp:ldr"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &FSP_LDR::OpenCodeFileSystem, "OpenCodeFileSystem"},
        {1, &FSP_LDR::IsArchivedProgram, "IsArchivedProgram"},
        {2, &FSP_LDR::SetCurrentProcess, "SetCurrentProcess"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

FSP_LDR::~FSP_LDR() = default;

void FSP_LDR::OpenCodeFileSystem(HLERequestContext& ctx) {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(FileSys::ResultTargetNotFound);
}

void FSP_LDR::IsArchivedProgram(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u64 pid = rp.Pop<u64>();
    LOG_DEBUG(Service_FS, "called, pid={:#x}", pid);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u8>(0);
}

void FSP_LDR::SetCurrentProcess(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    u64 pid = ctx.GetPID();
    if (pid == 0) {
        pid = rp.Pop<u64>();
    }
    LOG_DEBUG(Service_FS, "called, pid={:#x}", pid);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::FileSystem
