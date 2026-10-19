// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cctype>
#include "core/core.h"
#include "core/debugger/debugger.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_types.h"
#include "core/memory.h"
#include "core/reporter.h"

namespace Kernel::Svc {

/// Break program execution
void Break(Core::System& system, BreakReason reason, u64 info1, u64 info2) {
    BreakReason break_reason =
        reason & static_cast<BreakReason>(~BreakReason::NotificationOnlyFlag);
    bool notification_only = True(reason & BreakReason::NotificationOnlyFlag);

    bool has_dumped_buffer{};
    std::vector<u8> debug_buffer;
    bool is_benign_zero_err = false;
    if (info2 == sizeof(u32) && info1 != 0) {
        auto& memory = GetCurrentMemory(system.Kernel());
        if (memory.IsValidVirtualAddressRange(info1, sizeof(u32)) && memory.Read32(info1) == 0) {
            is_benign_zero_err = !system.DebuggerEnabled();
        }
    }

    const auto handle_debug_buffer = [&](u64 addr, u64 sz) {
        if (sz == 0 || addr == 0 || has_dumped_buffer) {
            return;
        }

        auto& memory = GetCurrentMemory(system.Kernel());

        // This typically is an error code so we're going to assume this is the case
        if (sz == sizeof(u32)) {
            const u32 err_code = memory.IsValidVirtualAddressRange(addr, sizeof(u32)) ? memory.Read32(addr) : 0;
            if (err_code == 0 && !system.DebuggerEnabled()) {
                LOG_WARNING(Debug_Emulated, "debug_buffer_err_code={:X} (ignored benign break)", err_code);
            } else {
                LOG_CRITICAL(Debug_Emulated, "debug_buffer_err_code={:X}", err_code);
            }
        } else if (sz <= 4096 && memory.IsValidVirtualAddressRange(addr, sz)) {
            // We don't know what's in here so we'll hexdump it (clamped to 4KB to prevent bad_alloc when info2 is an address/line)
            debug_buffer.resize(sz);
            memory.ReadBlock(addr, debug_buffer.data(), sz);
            std::string hexdump;
            for (std::size_t i = 0; i < debug_buffer.size(); i++) {
                hexdump += fmt::format("{:02X} ", debug_buffer[i]);
                if ((i + 1) % 32 == 0) {
                    hexdump += '\n';
                }
            }
            LOG_CRITICAL(Debug_Emulated, "debug_buffer=\n{}", hexdump);
        }
        has_dumped_buffer = true;
    };
    switch (break_reason) {
    case BreakReason::Panic:
        if (is_benign_zero_err) {
            LOG_WARNING(Debug_Emulated, "Userspace PANIC (err_code=0, continuing): info1={:#016x}, info2={:#016x}",
                        info1, info2);
        } else {
            LOG_CRITICAL(Debug_Emulated, "Userspace PANIC! info1={:#016x}, info2={:#016x}", info1,
                         info2);
        }
        handle_debug_buffer(info1, info2);
        break;
    case BreakReason::Assert:
        if (is_benign_zero_err) {
            LOG_WARNING(Debug_Emulated, "Userspace Assertion (err_code=0, continuing): info1={:#016x}, info2={:#016x}",
                        info1, info2);
        } else {
            LOG_CRITICAL(Debug_Emulated, "Userspace Assertion failed! info1={:#016x}, info2={:#016x}",
                         info1, info2);
        }
        handle_debug_buffer(info1, info2);
        break;
    case BreakReason::User:
        LOG_WARNING(Debug_Emulated, "Userspace Break! {:#016x} with size {:#016x}", info1, info2);
        handle_debug_buffer(info1, info2);
        break;
    case BreakReason::PreLoadDll:
        LOG_INFO(Debug_Emulated,
                 "Userspace Attempting to load an NRO at {:#016x} with size {:#016x}", info1,
                 info2);
        break;
    case BreakReason::PostLoadDll:
        LOG_INFO(Debug_Emulated, "Userspace Loaded an NRO at {:#016x} with size {:#016x}", info1,
                 info2);
        break;
    case BreakReason::PreUnloadDll:
        LOG_INFO(Debug_Emulated,
                 "Userspace Attempting to unload an NRO at {:#016x} with size {:#016x}", info1,
                 info2);
        break;
    case BreakReason::PostUnloadDll:
        LOG_INFO(Debug_Emulated, "Userspace Unloaded an NRO at {:#016x} with size {:#016x}",
                 info1, info2);
        break;
    case BreakReason::CppException:
        LOG_CRITICAL(Debug_Emulated, "Signalling debugger. Uncaught C++ exception encountered.");
        break;
    default:
        LOG_WARNING(
            Debug_Emulated,
            "Signalling debugger, Unknown break reason {:#x}, info1={:#016x}, info2={:#016x}",
            reason, info1, info2);
        handle_debug_buffer(info1, info2);
        break;
    }

    if (!is_benign_zero_err) {
        system.GetReporter().SaveSvcBreakReport(
            static_cast<u32>(reason), notification_only, info1, info2,
            has_dumped_buffer ? std::make_optional(debug_buffer) : std::nullopt);
    }

    if (!notification_only) {
        if (auto* process = GetCurrentProcessPointer(system.Kernel()); process != nullptr) {
            const u64 plr_addr = GetInteger(process->GetProcessLocalRegionAddress());
            auto& memory = GetCurrentMemory(system.Kernel());
            if (plr_addr != 0 && memory.IsValidVirtualAddressRange(plr_addr + 0x1c0, 16)) {
                const u64 dying_addr = memory.Read64(plr_addr + 0x1c0);
                const u64 dying_size = memory.Read64(plr_addr + 0x1c8);
                if (dying_addr != 0 && dying_size > 0 && dying_size <= 0x10000 &&
                    memory.IsValidVirtualAddressRange(dying_addr, dying_size)) {
                    std::string dying_msg(dying_size, '\0');
                    memory.ReadBlock(dying_addr, dying_msg.data(), dying_size);
                    if (const auto nul_pos = dying_msg.find('\0'); nul_pos != std::string::npos) {
                        dying_msg.resize(nul_pos);
                    }
                    if (!dying_msg.empty()) {
                        LOG_CRITICAL(Debug_Emulated, "Process DyingMessage ({:#x}, size={:#x}):\n{}",
                                     dying_addr, dying_size, dying_msg);
                    }
                }
            }
        }
        if (is_benign_zero_err) {
            LOG_WARNING(
                Debug_Emulated,
                "Emulated program broke execution with err_code=0 (continuing)! reason={:#016x}, info1={:#016x}, info2={:#016x}",
                reason, info1, info2);
            system.CurrentPhysicalCore().LogBacktrace(system.Kernel());
        } else {
            LOG_CRITICAL(
                Debug_Emulated,
                "Emulated program broke execution! reason={:#016x}, info1={:#016x}, info2={:#016x}",
                reason, info1, info2);
            handle_debug_buffer(info1, info2);
            system.CurrentPhysicalCore().LogBacktrace(system.Kernel());
        }
    }

    const bool should_break = !notification_only;
    if (system.DebuggerEnabled() && should_break) {
        auto* thread = system.Kernel().GetCurrentEmuThread();
        system.GetDebugger().NotifyThreadStopped(thread);
        thread->RequestSuspend(system.Kernel(), Kernel::SuspendType::Debug);
    }
}

void ReturnFromException(Core::System& system, Result result) {
    LOG_WARNING(Kernel_SVC, "ReturnFromException called with result={:#x}", result.raw);
}

void Break64(Core::System& system, BreakReason break_reason, uint64_t arg, uint64_t size) {
    Break(system, break_reason, arg, size);
}

void Break64From32(Core::System& system, BreakReason break_reason, uint32_t arg, uint32_t size) {
    Break(system, break_reason, arg, size);
}

void ReturnFromException64(Core::System& system, Result result) {
    ReturnFromException(system, result);
}

void ReturnFromException64From32(Core::System& system, Result result) {
    ReturnFromException(system, result);
}

} // namespace Kernel::Svc
