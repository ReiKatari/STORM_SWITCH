// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <memory>

#include "common/settings.h"
#include "common/signal_chain.h"
#include "core/arm/nce/arm_nce.h"
#include "core/arm/nce/interpreter_visitor.h"
#include "core/arm/nce/patcher.h"
#include "core/constants.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/memory.h"

#include "core/hle/kernel/k_process.h"

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace Core {

namespace {

struct sigaction g_orig_bus_action;
struct sigaction g_orig_segv_action;
struct sigaction g_orig_ill_action;

// Verify assembly offsets.
using NativeExecutionParameters = Kernel::KThread::NativeExecutionParameters;
static_assert(offsetof(NativeExecutionParameters, native_context) == TpidrEl0NativeContext);
static_assert(offsetof(NativeExecutionParameters, lock) == TpidrEl0Lock);
static_assert(offsetof(NativeExecutionParameters, magic) == TpidrEl0TlsMagic);

fpsimd_context* GetFloatingPointState(mcontext_t& host_ctx) {
    _aarch64_ctx* header = reinterpret_cast<_aarch64_ctx*>(&host_ctx.__reserved);
    while (header->magic != FPSIMD_MAGIC) {
        header = reinterpret_cast<_aarch64_ctx*>(reinterpret_cast<char*>(header) + header->size);
    }
    return reinterpret_cast<fpsimd_context*>(header);
}

using namespace Common::Literals;
constexpr u32 StackSize = 128_KiB;

} // namespace

void* ArmNce::RestoreGuestContext(void* raw_context) {
    // Retrieve the host context.
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;

    // Thread-local parameters will be located in x9.
    auto* tpidr = reinterpret_cast<NativeExecutionParameters*>(host_ctx.regs[9]);
    auto* guest_ctx = static_cast<GuestContext*>(tpidr->native_context);

    // Retrieve the host floating point state.
    auto* fpctx = GetFloatingPointState(host_ctx);

    // Save host callee-saved registers.
    std::memcpy(guest_ctx->host_ctx.host_saved_vregs.data(), &fpctx->vregs[8],
                sizeof(guest_ctx->host_ctx.host_saved_vregs));
    std::memcpy(guest_ctx->host_ctx.host_saved_regs.data(), &host_ctx.regs[19],
                sizeof(guest_ctx->host_ctx.host_saved_regs));

    // Save stack pointer.
    guest_ctx->host_ctx.host_sp = host_ctx.sp;

    // Restore all guest state except tpidr_el0.
    host_ctx.sp = guest_ctx->sp;
    host_ctx.pc = guest_ctx->pc;
    host_ctx.pstate = guest_ctx->pstate;
    fpctx->fpcr = guest_ctx->fpcr;
    fpctx->fpsr = guest_ctx->fpsr;
    std::memcpy(host_ctx.regs, guest_ctx->cpu_registers.data(), sizeof(host_ctx.regs));
    std::memcpy(fpctx->vregs, guest_ctx->vector_registers.data(), sizeof(fpctx->vregs));

    // Return the new thread-local storage pointer.
    return tpidr;
}

void ArmNce::SaveGuestContext(GuestContext* guest_ctx, void* raw_context) {
    // Retrieve the host context.
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;

    // Retrieve the host floating point state.
    auto* fpctx = GetFloatingPointState(host_ctx);

    // Save all guest registers except tpidr_el0.
    std::memcpy(guest_ctx->cpu_registers.data(), host_ctx.regs, sizeof(host_ctx.regs));
    std::memcpy(guest_ctx->vector_registers.data(), fpctx->vregs, sizeof(fpctx->vregs));
    guest_ctx->fpsr = fpctx->fpsr;
    guest_ctx->fpcr = fpctx->fpcr;
    guest_ctx->pstate = static_cast<u32>(host_ctx.pstate);
    guest_ctx->pc = host_ctx.pc;
    guest_ctx->sp = host_ctx.sp;

    // Restore stack pointer.
    host_ctx.sp = guest_ctx->host_ctx.host_sp;

    // Restore host callee-saved registers.
    std::memcpy(&host_ctx.regs[19], guest_ctx->host_ctx.host_saved_regs.data(),
                sizeof(guest_ctx->host_ctx.host_saved_regs));
    std::memcpy(&fpctx->vregs[8], guest_ctx->host_ctx.host_saved_vregs.data(),
                sizeof(guest_ctx->host_ctx.host_saved_vregs));

    // Return from the call on exit by setting pc to x30.
    host_ctx.pc = guest_ctx->host_ctx.host_saved_regs[11];

    // Clear esr_el1 and return it.
    host_ctx.regs[0] = guest_ctx->esr_el1.exchange(0);
}

bool ArmNce::HandleFailedGuestFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;
    auto* info = static_cast<siginfo_t*>(raw_info);
    auto& memory = guest_ctx->parent->m_running_thread->GetOwnerProcess()->GetMemory();
    const u64 fastmem_base =
        reinterpret_cast<u64>(guest_ctx->system->DeviceMemory().buffer.VirtualBasePointer());

    const auto ResolveProcessAddr = [&](u64 addr, size_t sz = sizeof(u32)) -> std::optional<u64> {
        if (memory.IsValidVirtualAddressRange(addr, sz)) {
            return addr;
        }
        if (addr >= fastmem_base && memory.IsValidVirtualAddressRange(addr - fastmem_base, sz)) {
            return addr - fastmem_base;
        }
        if (addr < fastmem_base && memory.IsValidVirtualAddressRange(addr + fastmem_base, sz)) {
            return addr + fastmem_base;
        }
        return std::nullopt;
    };

    const auto resolved_pc = ResolveProcessAddr(host_ctx.pc);

    // We can't handle the access, so determine why we crashed.
    const bool is_prefetch_abort =
        (host_ctx.pc == reinterpret_cast<u64>(info->si_addr)) || !resolved_pc.has_value();

    // For data aborts or when ignoring memory aborts, handle gracefully.
    // For data aborts, skip the faulting instruction (pc += 4).
    // For prefetch aborts (e.g. calling null function pointer in Streets of Rage 4),
    // safely return to LR (link register) instead of crashing into unmapped pc + 4.
    if (!is_prefetch_abort) {
        // Some guest object wrappers guard a pointer with CBZ immediately before loading its
        // type field. A packed handle can incorrectly reach this path as a non-null pointer. If
        // this exact sequence faults, follow the wrapper's own null-object return path.
        if (*resolved_pc >= 4 && ResolveProcessAddr(host_ctx.pc - 4, sizeof(u32) * 2).has_value()) {
            u32 fault_instruction{};
            u32 null_check_instruction{};
            std::memcpy(&fault_instruction, reinterpret_cast<const void*>(host_ctx.pc),
                        sizeof(fault_instruction));
            std::memcpy(&null_check_instruction, reinterpret_cast<const void*>(host_ctx.pc - 4),
                        sizeof(null_check_instruction));
            constexpr u32 LoadTypeFromX0 = 0xB9400808; // ldr w8, [x0, #8]
            constexpr u32 CbzX0Mask = 0xFF00001F;
            constexpr u32 CbzX0 = 0xB4000000;
            const s64 fault_delta = static_cast<s64>(reinterpret_cast<u64>(info->si_addr)) -
                                    static_cast<s64>(host_ctx.regs[0]);
            if (fault_instruction == LoadTypeFromX0 &&
                (null_check_instruction & CbzX0Mask) == CbzX0 && fault_delta == 8) {
                const s64 branch_words = static_cast<s32>(null_check_instruction << 8) >> 13;
                const u64 null_return_pc = host_ctx.pc - 4 + branch_words * 4;
                if (null_return_pc > host_ctx.pc && null_return_pc - host_ctx.pc <= 0x100) {
                    LOG_WARNING(Core_ARM,
                                "Recovered invalid NCE object through null guard: pc={:#x}, "
                                "x0={:#x}, null_return_pc={:#x}",
                                host_ctx.pc, host_ctx.regs[0], null_return_pc);
                    host_ctx.regs[0] = 0;
                    host_ctx.pc = null_return_pc;
                    return true;
                }
            }
        }

        // For data aborts, safely advance PC and recover, preventing game crashes on NCE (7.3.2 behavior)
        host_ctx.pc += 4;
        return true;
    } else if (Settings::values.cpuopt_ignore_memory_aborts.GetValue() ||
               info->si_signo == SIGILL) {
        const u64 lr = host_ctx.regs[30];
        if (lr != 0 && (lr & 3) == 0 && lr != host_ctx.pc && ResolveProcessAddr(lr).has_value()) {
            const u64 host_lr = (lr >= fastmem_base) ? lr : (fastmem_base + lr);
            host_ctx.regs[0] = 0;
            host_ctx.regs[1] = 0;
            host_ctx.pc = host_lr;
            return true;
        }
    }

    // Set appropriate halt reason.
    if (is_prefetch_abort) {
        guest_ctx->esr_el1.fetch_or(static_cast<u64>(HaltReason::PrefetchAbort));
    } else {
        guest_ctx->esr_el1.fetch_or(static_cast<u64>(HaltReason::DataAbort));
    }

    // Forcibly mark the context as locked. We are still running.
    // We may race with SignalInterrupt here:
    // - If we lose the race, then SignalInterrupt will send us a signal we are masking,
    //   and it will do nothing when it is unmasked, as we have already left guest code.
    // - If we win the race, then SignalInterrupt will wait for us to unlock first.
    auto& thread_params = guest_ctx->parent->m_running_thread->GetNativeExecutionParameters();
    thread_params.lock.store(SpinLockLocked);

    // Return to host.
    SaveGuestContext(guest_ctx, raw_context);
    return false;
}

bool ArmNce::HandleGuestAlignmentFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;
    auto* fpctx = GetFloatingPointState(host_ctx);
    auto& memory = guest_ctx->parent->m_running_thread->GetOwnerProcess()->GetMemory();

    // Match and execute an instruction.
    auto next_pc = MatchAndExecuteOneInstruction(memory, &host_ctx, fpctx);
    if (next_pc) {
        host_ctx.pc = *next_pc;
        return true;
    }

    // We couldn't handle the access.
    return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
}

bool ArmNce::HandleGuestAccessFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto* info = static_cast<siginfo_t*>(raw_info);

    const u64 fault_addr = reinterpret_cast<u64>(info->si_addr);
    const Common::ProcessAddress addr = (fault_addr & ~Memory::YUZU_PAGEMASK);
    auto& memory = guest_ctx->parent->m_running_thread->GetOwnerProcess()->GetMemory();

    bool handled = memory.InvalidateNCE(addr, Memory::YUZU_PAGESIZE);
    // Handle accesses which split a page boundary (e.g. unaligned 128-bit vector, STP Qn, or 64-byte DC ZVA)
    if ((fault_addr + 64) > (addr + Memory::YUZU_PAGESIZE)) {
        handled |= memory.InvalidateNCE(addr + Memory::YUZU_PAGESIZE, Memory::YUZU_PAGESIZE);
    }

    if (handled) {
        // We handled the access successfully and are returning to guest code.
        return true;
    }

    // We couldn't handle the access.
    return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
}

void ArmNce::HandleHostAlignmentFault(int sig, void* raw_info, void* raw_context) {
    if (g_orig_bus_action.sa_sigaction) {
        g_orig_bus_action.sa_sigaction(sig, static_cast<siginfo_t*>(raw_info), raw_context);
    }
}

void ArmNce::HandleHostAccessFault(int sig, void* raw_info, void* raw_context) {
    if (g_orig_segv_action.sa_sigaction) {
        g_orig_segv_action.sa_sigaction(sig, static_cast<siginfo_t*>(raw_info), raw_context);
    }
}

void ArmNce::HandleHostIllegalInstruction(int sig, void* raw_info, void* raw_context) {
    if (g_orig_ill_action.sa_sigaction) {
        g_orig_ill_action.sa_sigaction(sig, static_cast<siginfo_t*>(raw_info), raw_context);
    }
}

bool ArmNce::HandleGuestIllegalInstruction(GuestContext* guest_ctx, void* raw_info,
                                           void* raw_context) {
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;
    auto* fpctx = GetFloatingPointState(host_ctx);
    auto* thread = guest_ctx->parent->m_running_thread;
    auto* process = thread->GetOwnerProcess();
    auto& memory = process->GetMemory();
    auto& thread_params = thread->GetNativeExecutionParameters();

    const u64 fastmem_base =
        reinterpret_cast<u64>(guest_ctx->system->DeviceMemory().buffer.VirtualBasePointer());

    const auto ResolveProcessAddr = [&](u64 addr, size_t sz = sizeof(u32)) -> std::optional<u64> {
        if (memory.IsValidVirtualAddressRange(addr, sz)) {
            return addr;
        }
        if (addr >= fastmem_base && memory.IsValidVirtualAddressRange(addr - fastmem_base, sz)) {
            return addr - fastmem_base;
        }
        if (addr < fastmem_base && memory.IsValidVirtualAddressRange(addr + fastmem_base, sz)) {
            return addr + fastmem_base;
        }
        return std::nullopt;
    };

    const auto resolved_pc = ResolveProcessAddr(host_ctx.pc);
    if (!resolved_pc.has_value()) {
        const u64 lr = host_ctx.regs[30];
        if (lr != 0 && (lr & 3) == 0 && lr != host_ctx.pc && ResolveProcessAddr(lr).has_value()) {
            const u64 host_lr = (lr >= fastmem_base) ? lr : (fastmem_base + lr);
            LOG_WARNING(Core_ARM, "NCE SIGILL at unmapped PC {:#x}, returning to LR {:#x}",
                        host_ctx.pc, host_lr);
            host_ctx.regs[0] = 0;
            host_ctx.regs[1] = 0;
            host_ctx.pc = host_lr;
            return true;
        }
        return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
    }
    const u64 guest_pc = (host_ctx.pc >= fastmem_base) ? (host_ctx.pc - fastmem_base) : host_ctx.pc;

    // Try the NCE instruction interpreter first.
    if (auto next_pc = MatchAndExecuteOneInstruction(memory, &host_ctx, fpctx)) {
        host_ctx.pc = *next_pc;
        return true;
    }

    u32 inst = 0;
    std::memcpy(&inst, reinterpret_cast<const void*>(host_ctx.pc), sizeof(u32));

    // 1. Emulate unpatched MRS Xt, <sysreg> (encoding: 0xD5300000 | (sysreg << 5) | rt)
    if ((inst & 0xFFF00000U) == 0xD5300000U) {
        const u32 rt = inst & 0x1FU;
        const u32 sysreg = (inst >> 5) & 0x7FFFU;
        u64 val = 0;
        switch (sysreg) {
        case 0x5E82: // TPIDR_EL0
            val = thread_params.tpidr_el0;
            break;
        case 0x5E83: // TPIDRRO_EL0
            val = thread_params.tpidrro_el0;
            break;
        case 0x5F00: // CNTFRQ_EL0
            val = Hardware::CNTFREQ;
            break;
        case 0x5F01: // CNTPCT_EL0
        case 0x5F02: // CNTVCT_EL0
            val = guest_ctx->system->CoreTiming().GetClockTicks();
            break;
        case 0x5801: // CTR_EL0 (Cortex-A57 cache type register)
            val = 0x8444C004ULL;
            break;
        case 0x5807: // DCZID_EL0 (64-byte cache line zeroing)
            val = 0x00000004ULL;
            break;
        case 0x4000: // MIDR_EL1 (Nintendo Switch Tegra X1 Cortex-A57 r1p3)
            val = 0x411FD073ULL;
            break;
        case 0x5920: // RNDR
        case 0x5921: // RNDRRS
            val = guest_ctx->system->CoreTiming().GetClockTicks() ^ 0x9E3779B97F4A7C15ULL;
            break;
        default:
            val = 0;
            break;
        }
        if (rt < 31) {
            host_ctx.regs[rt] = val;
        }
        LOG_DEBUG(Core_ARM, "NCE emulated trapped MRS sysreg={:#x} -> X{}={:#x} at pc={:#x}",
                  sysreg, rt, val, host_ctx.pc);
        host_ctx.pc += 4;
        return true;
    }

    // 2. Emulate unpatched MSR <sysreg>, Xt / SYS / HINT / PAC / BTI
    if ((inst & 0xFFF00000U) == 0xD5100000U || (inst & 0xFFF80000U) == 0xD5080000U ||
        (inst & 0xFFFFF01FU) == 0xD503201FU) {
        if ((inst & 0xFFF00000U) == 0xD5100000U) {
            const u32 rt = inst & 0x1FU;
            const u32 sysreg = (inst >> 5) & 0x7FFFU;
            if (sysreg == 0x5E82) { // TPIDR_EL0
                thread_params.tpidr_el0 = (rt < 31) ? host_ctx.regs[rt] : 0;
            }
        }
        host_ctx.pc += 4;
        return true;
    }

    // 3. Handle UDF (#0x0000xxxx), BRK (#0xD420xxxx), or trapped instructions (e.g. Frostbite Career Mode OOM/assert traps)
    u32 next_inst = 0;
    if (ResolveProcessAddr(host_ctx.pc + 4).has_value()) {
        std::memcpy(&next_inst, reinterpret_cast<const void*>(host_ctx.pc + 4), sizeof(u32));
    }

    // Check if next_inst is the start of a new function (prologue) or zeroed padding, meaning
    // the current instruction is a noreturn trap (__builtin_trap / abort) at the end of a function.
    const bool next_is_prologue_or_zero =
        (next_inst == 0x00000000U) ||
        ((next_inst & 0xFFC07FFFU) == 0xA9807BFDU) || // STP X29, X30, [SP, #-imm]!
        ((next_inst & 0xFFC003FFU) == 0xA98003E0U) || // STP Xn, Xm, [SP, #-imm]!
        ((next_inst & 0xFF0003FFU) == 0xD10003FFU) || // SUB SP, SP, #imm
        (next_inst == 0xD503233FU) ||                 // PACIASP
        ((next_inst & 0xFFFFFF3FU) == 0xD503241FU);   // BTI

    if (inst == 0x00000000U && next_inst == 0x00000000U) {
        // PC jumped into zero-filled memory; return to LR if valid
        const u64 lr = host_ctx.regs[30];
        if (lr != 0 && (lr & 3) == 0 && lr != host_ctx.pc && ResolveProcessAddr(lr).has_value()) {
            const u64 host_lr = (lr >= fastmem_base) ? lr : (fastmem_base + lr);
            LOG_WARNING(Core_ARM, "NCE SIGILL in zeroed memory at {:#x}, returning to LR {:#x}",
                        host_ctx.pc, host_lr);
            host_ctx.regs[0] = 0;
            host_ctx.regs[1] = 0;
            host_ctx.pc = host_lr;
            return true;
        }
    }

    if (next_is_prologue_or_zero) {
        // Scan backwards up to 128 instructions to locate the function's own epilogue (RET = 0xD65F03C0)
        for (u32 k = 1; k <= 128; ++k) {
            if (guest_pc < k * 4) {
                break;
            }
            const u64 cand_pc = host_ctx.pc - static_cast<u64>(k) * 4;
            if (!ResolveProcessAddr(cand_pc).has_value()) {
                break;
            }
            u32 cand_inst = 0;
            std::memcpy(&cand_inst, reinterpret_cast<const void*>(cand_pc), sizeof(u32));
            if (cand_inst == 0xD65F03C0U) { // RET
                // Walk backwards across contiguous epilogue restore instructions (LDP/LDR from SP, ADD SP, SP)
                u64 epilogue_pc = cand_pc;
                for (u32 j = 1; j <= 16 && (guest_pc >= (k + j) * 4); ++j) {
                    const u64 prev_pc = cand_pc - static_cast<u64>(j) * 4;
                    if (!ResolveProcessAddr(prev_pc).has_value()) {
                        break;
                    }
                    u32 prev_inst = 0;
                    std::memcpy(&prev_inst, reinterpret_cast<const void*>(prev_pc), sizeof(u32));
                    const bool is_ldp_sp = ((prev_inst & 0x7FC003E0U) == 0x294003E0U) || // LDP Xt1, Xt2, [SP, #imm]
                                           ((prev_inst & 0x7FC003E0U) == 0x28C003E0U);   // LDP Xt1, Xt2, [SP], #imm
                    const bool is_ldr_sp = ((prev_inst & 0xBFC003E0U) == 0xB94003E0U) || // LDR Xt, [SP, #imm]
                                           ((prev_inst & 0xBFE00FE0U) == 0xB84007E0U);   // LDR Xt, [SP], #imm
                    const bool is_add_sp = ((prev_inst & 0xFF0003FFU) == 0x910003FFU);   // ADD SP, SP, #imm
                    if (is_ldp_sp || is_ldr_sp || is_add_sp) {
                        epilogue_pc = prev_pc;
                    } else {
                        break;
                    }
                }
                LOG_WARNING(Core_ARM,
                            "NCE recovered guest trap inst={:#010x} at pc={:#x} via function epilogue at {:#x}",
                            inst, host_ctx.pc, epilogue_pc);
                host_ctx.regs[0] = 0;
                host_ctx.regs[1] = 0;
                host_ctx.pc = epilogue_pc;
                return true;
            }
        }
    }

    // 4. If trapped inside AbortImpl / nn::diag::detail::Abort (UDF / BRK), walk AArch64 Frame Pointer (X29) chain
    if ((inst & 0xFFFF0000U) == 0U || (inst & 0xFFE0001FU) == 0xD4200000U) {
        u64 fp = host_ctx.regs[29];
        for (int depth = 0; depth < 6; ++depth) {
            if (fp == 0 || (fp & 7) != 0) {
                break;
            }
            const auto resolved_fp = ResolveProcessAddr(fp, 16);
            if (!resolved_fp.has_value()) {
                break;
            }
            const u64 next_fp = memory.Read64(*resolved_fp);
            const u64 saved_lr = memory.Read64(*resolved_fp + 8);
            const u64 rel_lr = (saved_lr >= fastmem_base) ? (saved_lr - fastmem_base) : saved_lr;
            const auto resolved_lr = ResolveProcessAddr(saved_lr);
            if (rel_lr != 0 && (rel_lr & 3) == 0 &&
                (rel_lr < (guest_pc > 0x40 ? guest_pc - 0x40 : 0) || rel_lr > guest_pc + 0x40) &&
                resolved_lr.has_value()) {
                const u32 lr_inst = memory.Read32(*resolved_lr);
                if ((lr_inst & 0xFFFF0000U) != 0U && (lr_inst & 0xFFE0001FU) != 0xD4200000U) {
                    const u64 host_lr = (saved_lr >= fastmem_base) ? saved_lr : (fastmem_base + rel_lr);
                    LOG_WARNING(Core_ARM,
                                "NCE recovered guest abort trap {:#010x} at pc={:#x} via FP unwind (depth={}) -> lr={:#x}",
                                inst, host_ctx.pc, depth + 1, host_lr);
                    host_ctx.regs[29] = next_fp;
                    host_ctx.regs[30] = host_lr;
                    const u64 host_fp = (fp >= fastmem_base) ? fp : (fastmem_base + fp);
                    if (host_fp >= host_ctx.sp && (host_fp - host_ctx.sp) <= 0x10000) {
                        host_ctx.sp = host_fp + 16;
                    }
                    host_ctx.regs[0] = 0;
                    host_ctx.regs[1] = 0;
                    host_ctx.pc = host_lr;
                    return true;
                }
            }
            if (next_fp <= fp || (next_fp - fp) > 0x100000) {
                break;
            }
            fp = next_fp;
        }
    }

    LOG_WARNING(Core_ARM, "NCE skipping trapped/illegal guest instruction {:#010x} at pc={:#x}",
                inst, host_ctx.pc);
    host_ctx.regs[0] = 0;
    host_ctx.pc += 4;
    return true;
}

void ArmNce::LockThread(Kernel::KThread* thread) {
    auto* thread_params = &thread->GetNativeExecutionParameters();
    LockThreadParameters(thread_params);
}

void ArmNce::UnlockThread(Kernel::KThread* thread) {
    auto* thread_params = &thread->GetNativeExecutionParameters();
    m_guest_ctx.tpidr_el0 = thread_params->tpidr_el0;
    m_guest_ctx.tpidrro_el0 = thread_params->tpidrro_el0;
    thread_params->native_context = nullptr;
    UnlockThreadParameters(thread_params);
}

HaltReason ArmNce::RunThread(Kernel::KThread* thread) {
    auto* thread_params = &thread->GetNativeExecutionParameters();
    // Lemon-Project v1.3.0 fix: ensure tpidr_el0 & tpidrro_el0 are synced before early return
    thread_params->tpidr_el0 = m_guest_ctx.tpidr_el0;
    thread_params->tpidrro_el0 = m_guest_ctx.tpidrro_el0;

    // Check if we're already interrupted.
    // If we are, we can just return immediately.
    HaltReason hr = static_cast<HaltReason>(m_guest_ctx.esr_el1.exchange(0));
    if (True(hr)) {
        return hr;
    }

    // Pre-fetch thread context data to improve cache locality
    auto* process = thread->GetOwnerProcess();

    // Move non-critical operations outside the locked section
    const u64 tpidr_el0_cache = m_guest_ctx.tpidr_el0;
    const u64 tpidrro_el0_cache = m_guest_ctx.tpidrro_el0;

    // Critical section begins - minimize operations here
    m_running_thread = thread;
    m_guest_ctx.parent = this;
    thread_params->native_context = &m_guest_ctx;
    thread_params->tpidr_el0 = tpidr_el0_cache;
    thread_params->tpidrro_el0 = tpidrro_el0_cache;

    // Memory barrier to ensure visibility of changes
    std::atomic_thread_fence(std::memory_order_release);
    thread_params->is_running = true;

    // TODO: finding and creating the post handler needs to be locked
    // to deal with dynamic loading of NROs.
    const auto& post_handlers = process->GetPostHandlers();
    if (auto it = post_handlers.find(m_guest_ctx.pc); it != post_handlers.end()) {
        hr = ReturnToRunCodeByTrampoline(thread_params, &m_guest_ctx, it->second);
    } else {
        hr = ReturnToRunCodeByExceptionLevelChange(m_thread_id, thread_params);  // Android: Use "process handle SIGUSR2 -n true -p true -s false" (and SIGURG) in LLDB when debugging
    }

    // Critical section for thread cleanup
    std::atomic_thread_fence(std::memory_order_acquire);

    // Cache values before releasing thread
    const u64 final_tpidr_el0 = thread_params->tpidr_el0;

    // Minimize critical section
    thread_params->is_running = false;
    thread_params->native_context = nullptr;
    m_running_thread = nullptr;

    // Non-critical updates can happen after releasing the thread
    m_guest_ctx.tpidr_el0 = final_tpidr_el0;

    // Return the halt reason.
    return hr;
}

HaltReason ArmNce::StepThread(Kernel::KThread* thread) {
    return HaltReason::StepThread;
}

u32 ArmNce::GetSvcNumber() const {
    return m_guest_ctx.svc;
}

void ArmNce::GetSvcArguments(std::span<uint64_t, 8> args) const {
    for (size_t i = 0; i < 8; i++) {
        args[i] = m_guest_ctx.cpu_registers[i];
    }
}

void ArmNce::SetSvcArguments(std::span<const uint64_t, 8> args) {
    for (size_t i = 0; i < 8; i++) {
        m_guest_ctx.cpu_registers[i] = args[i];
    }
}

ArmNce::ArmNce(System& system, bool uses_wall_clock, std::size_t core_index)
    : ArmInterface{uses_wall_clock}, m_system{system}, m_core_index{core_index} {
    m_guest_ctx.system = &m_system;
}

ArmNce::~ArmNce() = default;

void ArmNce::Initialize() {
    if (m_thread_id == -1) {
        m_thread_id = gettid();
    }

    // Configure signal stack.
    if (!m_stack) {
        m_stack = std::make_unique<u8[]>(StackSize);

        stack_t ss{};
        ss.ss_sp = m_stack.get();
        ss.ss_size = StackSize;
        sigaltstack(&ss, nullptr);
    }

    // Set up signals.
    static std::once_flag flag;
    std::call_once(flag, [] {
        using HandlerType = decltype(sigaction::sa_sigaction);

        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, ReturnToRunCodeByExceptionLevelChangeSignal);
        sigaddset(&signal_mask, BreakFromRunCodeSignal);
        sigaddset(&signal_mask, GuestAlignmentFaultSignal);
        sigaddset(&signal_mask, GuestAccessFaultSignal);
        sigaddset(&signal_mask, GuestIllegalInstructionSignal);

        struct sigaction return_to_run_code_action {};
        return_to_run_code_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        return_to_run_code_action.sa_sigaction = reinterpret_cast<HandlerType>(
            &ArmNce::ReturnToRunCodeByExceptionLevelChangeSignalHandler);
        return_to_run_code_action.sa_mask = signal_mask;
        Common::SigAction(ReturnToRunCodeByExceptionLevelChangeSignal, &return_to_run_code_action,
                          nullptr);

        struct sigaction break_from_run_code_action {};
        break_from_run_code_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        break_from_run_code_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::BreakFromRunCodeSignalHandler);
        break_from_run_code_action.sa_mask = signal_mask;
        Common::SigAction(BreakFromRunCodeSignal, &break_from_run_code_action, nullptr);

        struct sigaction alignment_fault_action {};
        alignment_fault_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        alignment_fault_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::GuestAlignmentFaultSignalHandler);
        alignment_fault_action.sa_mask = signal_mask;
        Common::SigAction(GuestAlignmentFaultSignal, &alignment_fault_action, &g_orig_bus_action);

        struct sigaction access_fault_action {};
        access_fault_action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
        access_fault_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::GuestAccessFaultSignalHandler);
        access_fault_action.sa_mask = signal_mask;
        Common::SigAction(GuestAccessFaultSignal, &access_fault_action, &g_orig_segv_action);

        struct sigaction illegal_instruction_action {};
        illegal_instruction_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        illegal_instruction_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::GuestIllegalInstructionSignalHandler);
        illegal_instruction_action.sa_mask = signal_mask;
        Common::SigAction(GuestIllegalInstructionSignal, &illegal_instruction_action,
                          &g_orig_ill_action);
    });
}

void ArmNce::SetTpidrroEl0(u64 value) {
    m_guest_ctx.tpidrro_el0 = value;
}

void ArmNce::GetContext(Kernel::Svc::ThreadContext& ctx) const {
    for (size_t i = 0; i < 29; i++) {
        ctx.r[i] = m_guest_ctx.cpu_registers[i];
    }
    ctx.fp = m_guest_ctx.cpu_registers[29];
    ctx.lr = m_guest_ctx.cpu_registers[30];
    ctx.sp = m_guest_ctx.sp;
    ctx.pc = m_guest_ctx.pc;
    ctx.pstate = m_guest_ctx.pstate;
    ctx.v = m_guest_ctx.vector_registers;
    ctx.fpcr = m_guest_ctx.fpcr;
    ctx.fpsr = m_guest_ctx.fpsr;
    ctx.tpidr = m_guest_ctx.tpidr_el0;
}

void ArmNce::SetContext(const Kernel::Svc::ThreadContext& ctx) {
    for (size_t i = 0; i < 29; i++) {
        m_guest_ctx.cpu_registers[i] = ctx.r[i];
    }
    m_guest_ctx.cpu_registers[29] = ctx.fp;
    m_guest_ctx.cpu_registers[30] = ctx.lr;
    m_guest_ctx.sp = ctx.sp;
    m_guest_ctx.pc = ctx.pc;
    m_guest_ctx.pstate = ctx.pstate;
    m_guest_ctx.vector_registers = ctx.v;
    m_guest_ctx.fpcr = ctx.fpcr;
    m_guest_ctx.fpsr = ctx.fpsr;
    m_guest_ctx.tpidr_el0 = ctx.tpidr;
}

void ArmNce::SignalInterrupt(Kernel::KThread* thread) {
    // Add break loop condition.
    m_guest_ctx.esr_el1.fetch_or(static_cast<u64>(HaltReason::BreakLoop));

    auto* params = &thread->GetNativeExecutionParameters();
    LockThreadParameters(params);

    // Ensure visibility of is_running after lock acquire
    std::atomic_thread_fence(std::memory_order_acquire);

    if (params->is_running) {
        // We should signal to the running thread.
        // The running thread will unlock the thread context.
        syscall(SYS_tkill, m_thread_id, BreakFromRunCodeSignal);
    } else {
        // If the thread is no longer running, we have nothing to do.
        UnlockThreadParameters(params);
    }
}

[[maybe_unused]] const std::size_t CACHE_PAGE_SIZE = 4096;

void ArmNce::ClearInstructionCache() {
#ifdef __aarch64__
    // Ensure all previous memory operations complete
    asm volatile("dsb ish\n"
                 "dsb ish\n"
                 "isb" ::: "memory");
#endif
}

void ArmNce::InvalidateCacheRange(u64 addr, std::size_t size) {
#ifdef __aarch64__
    auto* base_ptr = reinterpret_cast<char*>(m_system.DeviceMemory().buffer.VirtualBasePointer());
    if (base_ptr != nullptr && size > 0) {
        char* start = (addr >= reinterpret_cast<u64>(base_ptr)) ? reinterpret_cast<char*>(addr)
                                                                : (base_ptr + addr);
        __builtin___clear_cache(start, start + size);
        return;
    }
#endif
    this->ClearInstructionCache();
}

} // namespace Core
