// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project

// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project

// SPDX-License-Identifier: GPL-2.0-or-later



#include "common/logging.h"

#include "core/arm/arm_interface.h"

#include "core/arm/debug.h"

#include "core/core.h"

#include "core/hle/kernel/k_process.h"



namespace Core {



void ArmInterface::LogBacktrace(Kernel::KProcess* process) const {

    Kernel::Svc::ThreadContext ctx;

    this->GetContext(ctx);



    std::array<u64, 32> xreg{

        ctx.r[0], ctx.r[1], ctx.r[2], ctx.r[3],

        ctx.r[4], ctx.r[5], ctx.r[6], ctx.r[7],

        ctx.r[8], ctx.r[9], ctx.r[10], ctx.r[11],

        ctx.r[12], ctx.r[13], ctx.r[14], ctx.r[15],

        ctx.r[16], ctx.r[17], ctx.r[18], ctx.r[19],

        ctx.r[20], ctx.r[21], ctx.r[22], ctx.r[23],

        ctx.r[24], ctx.r[25], ctx.r[26], ctx.r[27],

        ctx.r[28], ctx.fp, ctx.lr, ctx.sp,

    };



    std::string msg = fmt::format("Backtrace @ PC={:016X}\n", ctx.pc);

    for (size_t i = 0; i < 32; i += 4)

        msg += fmt::format("R{:02}={:016X} R{:02}={:016X} R{:02}={:016X} R{:02}={:016X}\n",

            i + 0, xreg[i + 0], i + 1, xreg[i + 1],

            i + 2, xreg[i + 2], i + 3, xreg[i + 3]);

    for (size_t i = 0; i < 32; i += 2)

        msg += fmt::format("V{:02}={:016X}_{:016X} V{:02}={:016X}_{:016X}\n",

            i + 0, ctx.v[i + 0][0], ctx.v[i + 0][1],

            i + 1, ctx.v[i + 1][0], ctx.v[i + 1][1]);

    msg += fmt::format("PSTATE={:08X} FPCR={:08X} FPSR={:08X} TPIDR={:016X}\n", ctx.pstate, ctx.fpcr, ctx.fpsr, ctx.tpidr);

    msg += fmt::format("{:20}{:20}{:20}{:20}{}\n", "Module", "Address", "Original Address", "Offset", "Symbol");

    auto const backtrace = GetBacktraceFromContext(process, ctx);

    for (auto const& entry : backtrace)

        msg += fmt::format("{:20}{:016X}    {:016X}    {:016X}    {}\n", entry.module, entry.address, entry.original_address, entry.offset, entry.name);

    // Diagnostic string printer for aborts
    std::string diag_str;
    if (process) {
        auto& mem = process->GetMemory();
        for (const auto& th : process->GetThreadList()) {
            const u64 th_sp = th.GetContext().sp;
            if (std::abs(static_cast<s64>(th_sp - ctx.sp)) < 0x20000) {
                diag_str += fmt::format("  Thread: ID={} PC={:#x} SP={:#x}\n",
                                        th.GetThreadId(), th.GetContext().pc, th_sp);
            }
        }
        const std::pair<const char*, u64> regs[] = {
            {"R00", ctx.r[0]}, {"R01", ctx.r[1]}, {"R02", ctx.r[2]}, {"R06", ctx.r[6]},
            {"R07", ctx.r[7]}, {"R08", ctx.r[8]}, {"R09", ctx.r[9]}, {"SP", ctx.sp},
        };
        for (const auto& [reg_name, reg_val] : regs) {
            if (reg_val >= 0x1000 && mem.IsValidVirtualAddress(reg_val)) {
                std::string s;
                for (u64 p = reg_val; p < reg_val + 128; ++p) {
                    if (!mem.IsValidVirtualAddress(p)) break;
                    char c = static_cast<char>(mem.Read8(p));
                    if (c == 0) break;
                    if (c >= 32 && c <= 126) s += c; else s += '.';
                }
                std::string hex;
                for (u64 p = reg_val; p < reg_val + 32; ++p) {
                    if (!mem.IsValidVirtualAddress(p)) break;
                    hex += fmt::format("{:02x} ", mem.Read8(p));
                }
                diag_str += fmt::format("  {} ({:#x}): str='{}' hex=[{}]\n", reg_name, reg_val, s, hex);
            }
        }
        // Scan stack for string pointers
        for (u64 cur = ctx.sp; cur < ctx.sp + 0x400; cur += 8) {
            u64 val = 0;
            if (mem.ReadBlock(cur, &val, 8)) {
                if (val >= 0x1000 && mem.IsValidVirtualAddress(val)) {
                    std::string s;
                    for (u64 p = val; p < val + 128; ++p) {
                        if (!mem.IsValidVirtualAddress(p)) break;
                        char c = static_cast<char>(mem.Read8(p));
                        if (c == 0) break;
                        if (c >= 32 && c <= 126) s += c; else { s.clear(); break; }
                    }
                    if (s.size() >= 3) {
                        diag_str += fmt::format("  Stack [sp+{:#x} -> {:#x}]: \"{}\"\n", cur - ctx.sp, val, s);
                    }
                }
            }
        }
        for (auto const& entry : backtrace) {
            std::string code_hex;
            const u64 addr = (entry.original_address != 0) ? entry.original_address : entry.address;
            const u64 code_start = (addr >= 16) ? (addr - 16) : addr;
            for (u64 p = code_start; p < addr + 16; p += 4) {
                if (mem.IsValidVirtualAddress(p)) {
                    code_hex += fmt::format("{:08x} ", mem.Read32(p));
                }
            }
            diag_str += fmt::format("  Code around {:#x} (orig {:#x}): {}\n", entry.address, entry.original_address, code_hex);
        }
    }
    if (!diag_str.empty()) {
        msg += fmt::format("Diagnostic Strings and Memory:\n{}", diag_str);
    }
    LOG_ERROR(Core_ARM, "{}", msg);

}



const Kernel::DebugWatchpoint* ArmInterface::MatchingWatchpoint(

    u64 addr, u64 size, Kernel::DebugWatchpointType access_type) const {

    if (!m_watchpoints) {

        return nullptr;

    }



    const u64 start_address{addr};

    const u64 end_address{addr + size};



    for (size_t i = 0; i < Core::Hardware::NUM_WATCHPOINTS; i++) {

        const auto& watch{(*m_watchpoints)[i]};



        if (end_address <= GetInteger(watch.start_address)) {

            continue;

        }

        if (start_address >= GetInteger(watch.end_address)) {

            continue;

        }

        if ((access_type & watch.type) == Kernel::DebugWatchpointType::None) {

            continue;

        }



        return &watch;

    }



    return nullptr;

}



} // namespace Core

