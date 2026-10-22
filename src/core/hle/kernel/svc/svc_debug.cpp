// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel::Svc {

Result DebugActiveProcess(Core::System& system, Handle* out_handle, uint64_t process_id) {
    R_THROW(ResultNotImplemented);
}

Result BreakDebugProcess(Core::System& system, Handle debug_handle) {
    LOG_WARNING(Service, "(STUBBED) called");
    R_SUCCEED();
}

Result TerminateDebugProcess(Core::System& system, Handle debug_handle) {
    R_THROW(ResultNotImplemented);
}

Result GetDebugEvent(Core::System& system, uint64_t out_info, Handle debug_handle) {
    R_THROW(ResultNotImplemented);
}

Result ContinueDebugEvent(Core::System& system, Handle debug_handle, uint32_t flags,
                          uint64_t user_thread_ids, int32_t num_thread_ids) {
    // Atmosphere 1.11.0+ PR #2855: Allow num_thread_ids == 0 when ContinueAll (flag bit 3) is set
    LOG_DEBUG(Kernel_SVC,
              "ContinueDebugEvent called, debug_handle=0x{:08X}, flags=0x{:X}, "
              "user_thread_ids=0x{:X}, num_thread_ids={}",
              debug_handle, flags, user_thread_ids, num_thread_ids);
    R_UNLESS(num_thread_ids >= 0 && num_thread_ids <= 64, ResultInvalidEnumValue);
    (void)system;
    R_SUCCEED();
}

Result GetDebugThreadContext(Core::System& system, uint64_t out_context, Handle debug_handle,
                             uint64_t thread_id, uint32_t context_flags) {
    R_THROW(ResultNotImplemented);
}

Result SetDebugThreadContext(Core::System& system, Handle debug_handle, uint64_t thread_id,
                             uint64_t user_context, uint32_t context_flags) {
    R_THROW(ResultNotImplemented);
}

Result QueryDebugProcessMemory(Core::System& system, uint64_t out_memory_info,
                               PageInfo* out_page_info, Handle process_handle, uint64_t address) {
    LOG_DEBUG(Kernel_SVC, "QueryDebugProcessMemory called, process_handle=0x{:08X}, address=0x{:X}",
              process_handle, address);
    auto* process = system.ApplicationProcess();
    R_UNLESS(process != nullptr, ResultInvalidHandle);

    KMemoryInfo mem_info{};
    Svc::PageInfo page_info{};
    R_TRY(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), address));
    if (out_page_info) {
        *out_page_info = page_info;
    }
    const auto svc_mem_info = mem_info.GetSvcMemoryInfo();
    GetCurrentMemory(system.Kernel()).WriteBlock(out_memory_info, std::addressof(svc_mem_info),
                                                 sizeof(svc_mem_info));
    R_SUCCEED();
}

Result ReadDebugProcessMemory(Core::System& system, uint64_t buffer, Handle debug_handle,
                              uint64_t address, uint64_t size) {
    LOG_DEBUG(Kernel_SVC, "ReadDebugProcessMemory called, debug_handle=0x{:08X}, address=0x{:X}, size=0x{:X}",
              debug_handle, address, size);
    R_UNLESS(size > 0, ResultInvalidSize);
    auto* process = system.ApplicationProcess();
    R_UNLESS(process != nullptr, ResultInvalidHandle);

    std::vector<u8> temp(size);
    process->GetMemory().ReadBlock(address, temp.data(), size);
    GetCurrentMemory(system.Kernel()).WriteBlock(buffer, temp.data(), size);
    R_SUCCEED();
}

Result WriteDebugProcessMemory(Core::System& system, Handle debug_handle, uint64_t buffer,
                               uint64_t address, uint64_t size) {
    LOG_DEBUG(Kernel_SVC, "WriteDebugProcessMemory called, debug_handle=0x{:08X}, address=0x{:X}, size=0x{:X}",
              debug_handle, address, size);
    R_UNLESS(size > 0, ResultInvalidSize);
    auto* process = system.ApplicationProcess();
    R_UNLESS(process != nullptr, ResultInvalidHandle);

    std::vector<u8> temp(size);
    GetCurrentMemory(system.Kernel()).ReadBlock(buffer, temp.data(), size);
    process->GetMemory().WriteBlock(address, temp.data(), size);
    R_SUCCEED();
}

Result SetHardwareBreakPoint(Core::System& system, HardwareBreakPointRegisterName name,
                             uint64_t flags, uint64_t value) {
    R_THROW(ResultNotImplemented);
}

Result GetDebugThreadParam(Core::System& system, uint64_t* out_64, uint32_t* out_32,
                           Handle debug_handle, uint64_t thread_id, DebugThreadParam param) {
    R_THROW(ResultNotImplemented);
}

Result DebugActiveProcess64(Core::System& system, Handle* out_handle, uint64_t process_id) {
    R_RETURN(DebugActiveProcess(system, out_handle, process_id));
}

Result BreakDebugProcess64(Core::System& system, Handle debug_handle) {
    R_RETURN(BreakDebugProcess(system, debug_handle));
}

Result TerminateDebugProcess64(Core::System& system, Handle debug_handle) {
    R_RETURN(TerminateDebugProcess(system, debug_handle));
}

Result GetDebugEvent64(Core::System& system, uint64_t out_info, Handle debug_handle) {
    R_RETURN(GetDebugEvent(system, out_info, debug_handle));
}

Result ContinueDebugEvent64(Core::System& system, Handle debug_handle, uint32_t flags,
                            uint64_t thread_ids, int32_t num_thread_ids) {
    R_RETURN(ContinueDebugEvent(system, debug_handle, flags, thread_ids, num_thread_ids));
}

Result GetDebugThreadContext64(Core::System& system, uint64_t out_context, Handle debug_handle,
                               uint64_t thread_id, uint32_t context_flags) {
    R_RETURN(GetDebugThreadContext(system, out_context, debug_handle, thread_id, context_flags));
}

Result SetDebugThreadContext64(Core::System& system, Handle debug_handle, uint64_t thread_id,
                               uint64_t context, uint32_t context_flags) {
    R_RETURN(SetDebugThreadContext(system, debug_handle, thread_id, context, context_flags));
}

Result QueryDebugProcessMemory64(Core::System& system, uint64_t out_memory_info,
                                 PageInfo* out_page_info, Handle debug_handle, uint64_t address) {
    R_RETURN(
        QueryDebugProcessMemory(system, out_memory_info, out_page_info, debug_handle, address));
}

Result ReadDebugProcessMemory64(Core::System& system, uint64_t buffer, Handle debug_handle,
                                uint64_t address, uint64_t size) {
    R_RETURN(ReadDebugProcessMemory(system, buffer, debug_handle, address, size));
}

Result WriteDebugProcessMemory64(Core::System& system, Handle debug_handle, uint64_t buffer,
                                 uint64_t address, uint64_t size) {
    R_RETURN(WriteDebugProcessMemory(system, debug_handle, buffer, address, size));
}

Result SetHardwareBreakPoint64(Core::System& system, HardwareBreakPointRegisterName name,
                               uint64_t flags, uint64_t value) {
    R_RETURN(SetHardwareBreakPoint(system, name, flags, value));
}

Result GetDebugThreadParam64(Core::System& system, uint64_t* out_64, uint32_t* out_32,
                             Handle debug_handle, uint64_t thread_id, DebugThreadParam param) {
    R_RETURN(GetDebugThreadParam(system, out_64, out_32, debug_handle, thread_id, param));
}

Result DebugActiveProcess64From32(Core::System& system, Handle* out_handle, uint64_t process_id) {
    R_RETURN(DebugActiveProcess(system, out_handle, process_id));
}

Result BreakDebugProcess64From32(Core::System& system, Handle debug_handle) {
    R_RETURN(BreakDebugProcess(system, debug_handle));
}

Result TerminateDebugProcess64From32(Core::System& system, Handle debug_handle) {
    R_RETURN(TerminateDebugProcess(system, debug_handle));
}

Result GetDebugEvent64From32(Core::System& system, uint32_t out_info, Handle debug_handle) {
    R_RETURN(GetDebugEvent(system, out_info, debug_handle));
}

Result ContinueDebugEvent64From32(Core::System& system, Handle debug_handle, uint32_t flags,
                                  uint32_t thread_ids, int32_t num_thread_ids) {
    R_RETURN(ContinueDebugEvent(system, debug_handle, flags, thread_ids, num_thread_ids));
}

Result GetDebugThreadContext64From32(Core::System& system, uint32_t out_context,
                                     Handle debug_handle, uint64_t thread_id,
                                     uint32_t context_flags) {
    R_RETURN(GetDebugThreadContext(system, out_context, debug_handle, thread_id, context_flags));
}

Result SetDebugThreadContext64From32(Core::System& system, Handle debug_handle, uint64_t thread_id,
                                     uint32_t context, uint32_t context_flags) {
    R_RETURN(SetDebugThreadContext(system, debug_handle, thread_id, context, context_flags));
}

Result QueryDebugProcessMemory64From32(Core::System& system, uint32_t out_memory_info,
                                       PageInfo* out_page_info, Handle debug_handle,
                                       uint32_t address) {
    R_RETURN(
        QueryDebugProcessMemory(system, out_memory_info, out_page_info, debug_handle, address));
}

Result ReadDebugProcessMemory64From32(Core::System& system, uint32_t buffer, Handle debug_handle,
                                      uint32_t address, uint32_t size) {
    R_RETURN(ReadDebugProcessMemory(system, buffer, debug_handle, address, size));
}

Result WriteDebugProcessMemory64From32(Core::System& system, Handle debug_handle, uint32_t buffer,
                                       uint32_t address, uint32_t size) {
    R_RETURN(WriteDebugProcessMemory(system, debug_handle, buffer, address, size));
}

Result SetHardwareBreakPoint64From32(Core::System& system, HardwareBreakPointRegisterName name,
                                     uint64_t flags, uint64_t value) {
    R_RETURN(SetHardwareBreakPoint(system, name, flags, value));
}

Result GetDebugThreadParam64From32(Core::System& system, uint64_t* out_64, uint32_t* out_32,
                                   Handle debug_handle, uint64_t thread_id,
                                   DebugThreadParam param) {
    R_RETURN(GetDebugThreadParam(system, out_64, out_32, debug_handle, thread_id, param));
}

} // namespace Kernel::Svc
