// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "core/arm/dynarmic/arm_dynarmic.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/arm/dynarmic/dynarmic_exclusive_monitor.h"
#include "core/arm/dynarmic/storm_tier2_compiler.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_process.h"

namespace Core {

using namespace Common::Literals;

DynarmicCallbacks64::DynarmicCallbacks64(ArmDynarmic64& parent, Kernel::KProcess* process)
    : m_parent{parent}, m_memory(process->GetMemory())
    , m_process(process), m_debugger_enabled{parent.m_system.DebuggerEnabled()}
    , m_check_memory_access{m_debugger_enabled || !Settings::values.cpuopt_ignore_memory_aborts.GetValue()}
{}

u8 DynarmicCallbacks64::MemoryRead8(u64 vaddr) {
    CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Read);
    return m_memory.Read8(vaddr);
}
u16 DynarmicCallbacks64::MemoryRead16(u64 vaddr) {
    CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Read);
    return m_memory.Read16(vaddr);
}
u32 DynarmicCallbacks64::MemoryRead32(u64 vaddr) {
    CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Read);
    return m_memory.Read32(vaddr);
}
u64 DynarmicCallbacks64::MemoryRead64(u64 vaddr) {
    CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Read);
    return m_memory.Read64(vaddr);
}
Dynarmic::A64::Vector DynarmicCallbacks64::MemoryRead128(u64 vaddr) {
    CheckMemoryAccess(vaddr, 16, Kernel::DebugWatchpointType::Read);
    return {m_memory.Read64(vaddr), m_memory.Read64(vaddr + 8)};
}

std::optional<u32> DynarmicCallbacks64::MemoryReadCode(u64 vaddr) {
    if (!m_memory.IsValidVirtualAddressRange(vaddr, sizeof(u32)))
        return std::nullopt;
    auto const aligned_vaddr = vaddr & ~Core::Memory::YUZU_PAGEMASK;
    if (last_code_addr != aligned_vaddr) {
        m_memory.ReadBlock(aligned_vaddr, &cached_code_page, sizeof(cached_code_page));
        StormTier2Compiler::OptimizePage64(cached_code_page.inst);
        last_code_addr = aligned_vaddr;
    }
    return cached_code_page.inst[(vaddr & Core::Memory::YUZU_PAGEMASK) / sizeof(u32)];
}

void DynarmicCallbacks64::MemoryWrite8(u64 vaddr, u8 value) {
    if (CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Write)) {
        m_memory.Write8(vaddr, value);
    }
}
void DynarmicCallbacks64::MemoryWrite16(u64 vaddr, u16 value) {
    if (CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Write)) {
        m_memory.Write16(vaddr, value);
    }
}
void DynarmicCallbacks64::MemoryWrite32(u64 vaddr, u32 value) {
    if (CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Write)) {
        m_memory.Write32(vaddr, value);
    }
}
void DynarmicCallbacks64::MemoryWrite64(u64 vaddr, u64 value) {
    if (CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Write)) {
        m_memory.Write64(vaddr, value);
    }
}
void DynarmicCallbacks64::MemoryWrite128(u64 vaddr, Dynarmic::A64::Vector value) {
    if (CheckMemoryAccess(vaddr, 16, Kernel::DebugWatchpointType::Write)) {
        m_memory.Write64(vaddr, value[0]);
        m_memory.Write64(vaddr + 8, value[1]);
    }
}

bool DynarmicCallbacks64::MemoryWriteExclusive8(u64 vaddr, std::uint8_t value, std::uint8_t expected) {
    return CheckMemoryAccess(vaddr, 1, Kernel::DebugWatchpointType::Write) &&
            m_memory.WriteExclusive8(vaddr, value, expected);
}
bool DynarmicCallbacks64::MemoryWriteExclusive16(u64 vaddr, std::uint16_t value, std::uint16_t expected) {
    return CheckMemoryAccess(vaddr, 2, Kernel::DebugWatchpointType::Write) &&
            m_memory.WriteExclusive16(vaddr, value, expected);
}
bool DynarmicCallbacks64::MemoryWriteExclusive32(u64 vaddr, std::uint32_t value, std::uint32_t expected) {
    return CheckMemoryAccess(vaddr, 4, Kernel::DebugWatchpointType::Write) &&
            m_memory.WriteExclusive32(vaddr, value, expected);
}
bool DynarmicCallbacks64::MemoryWriteExclusive64(u64 vaddr, std::uint64_t value, std::uint64_t expected) {
    return CheckMemoryAccess(vaddr, 8, Kernel::DebugWatchpointType::Write) &&
            m_memory.WriteExclusive64(vaddr, value, expected);
}
bool DynarmicCallbacks64::MemoryWriteExclusive128(u64 vaddr, Dynarmic::A64::Vector value, Dynarmic::A64::Vector expected) {
    return CheckMemoryAccess(vaddr, 16, Kernel::DebugWatchpointType::Write) &&
            m_memory.WriteExclusive128(vaddr, value, expected);
}

void DynarmicCallbacks64::InstructionCacheOperationRaised(Dynarmic::A64::InstructionCacheOperation op, u64 value) {
    last_code_addr = u64(-1); //invalidate cached page
    switch (op) {
    case Dynarmic::A64::InstructionCacheOperation::InvalidateByVAToPoU: {
        static constexpr u64 ICACHE_LINE_SIZE = 64;
        const u64 cache_line_start = value & ~(ICACHE_LINE_SIZE - 1);
        m_parent.InvalidateCacheRange(cache_line_start, ICACHE_LINE_SIZE);
        break;
    }
    case Dynarmic::A64::InstructionCacheOperation::InvalidateAllToPoU:
    case Dynarmic::A64::InstructionCacheOperation::InvalidateAllToPoUInnerSharable:
        m_parent.ClearInstructionCache();
        break;
    default:
        LOG_DEBUG(Core_ARM, "Unprocesseed instruction cache operation: {}", op);
        break;
    }
    m_parent.m_jit->HaltExecution(Dynarmic::HaltReason::CacheInvalidation);
}

void DynarmicCallbacks64::ExceptionRaised(u64 pc, Dynarmic::A64::Exception exception) {
    const auto is_valid_return_target = [&](u64 target) -> bool {
        if (target == 0 || (target & 3) != 0 || target == pc) {
            return false;
        }
        if (target >= (pc > 0x40 ? pc - 0x40 : 0) && target <= pc + 0x40) {
            return false;
        }
        if (!m_memory.IsValidVirtualAddressRange(target, sizeof(u32))) {
            return false;
        }
        const u32 target_inst = m_memory.Read32(target);
        if ((target_inst & 0xFFFF0000U) == 0U || (target_inst & 0xFFE0001FU) == 0xD4200000U) {
            return false;
        }
        return true;
    };

    const auto try_unwind_to_caller = [&](u64 current_lr, u64 avoid_lr) -> bool {
        // 1. Walk the AArch64 Frame Pointer (X29) chain
        u64 fp = m_parent.m_jit->GetRegister(29);
        const u64 sp = m_parent.m_jit->GetSP();
        for (int depth = 0; depth < 12; ++depth) {
            if (fp == 0 || (fp & 7) != 0 || !m_memory.IsValidVirtualAddressRange(fp, 16)) {
                break;
            }
            const u64 next_fp = m_memory.Read64(fp);
            const u64 saved_lr = m_memory.Read64(fp + 8);
            if (saved_lr != current_lr && saved_lr != avoid_lr && is_valid_return_target(saved_lr)) {
                LOG_WARNING(Core_ARM, "Unwinding guest fault at PC={:#010x} via FP chain (depth={}) -> LR={:#010x}",
                            pc, depth + 1, saved_lr);
                m_parent.m_jit->SetRegister(29, next_fp);
                m_parent.m_jit->SetRegister(30, saved_lr);
                if (fp >= sp && (fp - sp) <= 0x20000) {
                    m_parent.m_jit->SetSP(fp + 16);
                }
                m_parent.m_jit->SetRegister(0, 0);
                m_parent.m_jit->SetRegister(1, 0);
                m_parent.m_jit->SetPC(saved_lr);
                m_parent.m_jit->HaltExecution(BreakLoop);
                return true;
            }
            if (next_fp <= fp || (next_fp - fp) > 0x100000) {
                break;
            }
            fp = next_fp;
        }

        // 2. Fallback for -fomit-frame-pointer: scan active stack [sp + 8, sp + 0x800] for a saved
        // return address immediately preceded by a BL (0x94000000) or BLR (0xD63F0000) call instruction
        if (sp >= 0x1000 && (sp & 7) == 0) {
            for (u64 s_off = 8; s_off <= 0x800; s_off += 8) {
                const u64 slot_addr = sp + s_off;
                if (!m_memory.IsValidVirtualAddressRange(slot_addr, 8)) {
                    break;
                }
                const u64 cand_lr = m_memory.Read64(slot_addr);
                if (cand_lr == current_lr || cand_lr == avoid_lr || cand_lr < 0x1004 || !is_valid_return_target(cand_lr)) {
                    continue;
                }
                if (!m_memory.IsValidVirtualAddressRange(cand_lr - 4, sizeof(u32))) {
                    continue;
                }
                const u32 call_inst = m_memory.Read32(cand_lr - 4);
                const bool is_bl = (call_inst & 0xFC000000U) == 0x94000000U;
                const bool is_blr = (call_inst & 0xFFFFFC1FU) == 0xD63F0000U;
                if (is_bl || is_blr) {
                    LOG_WARNING(Core_ARM, "Unwinding guest fault at PC={:#010x} via stack call-site scan (+{:#x}) -> LR={:#010x}",
                                pc, s_off, cand_lr);
                    m_parent.m_jit->SetSP((slot_addr + 8) & ~0xFULL);
                    m_parent.m_jit->SetRegister(30, cand_lr);
                    m_parent.m_jit->SetRegister(0, 0);
                    m_parent.m_jit->SetRegister(1, 0);
                    m_parent.m_jit->SetPC(cand_lr);
                    m_parent.m_jit->HaltExecution(BreakLoop);
                    return true;
                }
            }
        }
        return false;
    };

    switch (exception) {
    case Dynarmic::A64::Exception::WaitForInterrupt:
    case Dynarmic::A64::Exception::WaitForEvent:
        m_parent.m_jit->HaltExecution(BreakLoop);
        return;
    case Dynarmic::A64::Exception::SendEvent:
    case Dynarmic::A64::Exception::SendEventLocal:
        return;
    case Dynarmic::A64::Exception::Yield:
#if defined(_MSC_VER) && defined(ARCHITECTURE_x86_64)
        _mm_pause();
#endif
        return;
    case Dynarmic::A64::Exception::NoExecuteFault:
        if (!m_debugger_enabled) {
            thread_local u64 tls_last_noexec_lr = 0;
            thread_local u32 tls_noexec_repeat_count = 0;
            const u64 lr = m_parent.m_jit->GetRegister(30);
            if (is_valid_return_target(lr) && (lr != tls_last_noexec_lr || tls_noexec_repeat_count == 0)) {
                if (lr == tls_last_noexec_lr) {
                    ++tls_noexec_repeat_count;
                } else {
                    tls_last_noexec_lr = lr;
                    tls_noexec_repeat_count = 1;
                }
                LOG_WARNING(Core_ARM, "Recovering from execution fault at {:#016x}, returning to LR {:#016x}", pc, lr);
                m_parent.m_jit->SetRegister(0, 0);
                m_parent.m_jit->SetRegister(1, 0);
                m_parent.m_jit->SetPC(lr);
                m_parent.m_jit->HaltExecution(BreakLoop);
                return;
            }
            if (try_unwind_to_caller(lr, tls_last_noexec_lr)) {
                tls_last_noexec_lr = m_parent.m_jit->GetPC();
                tls_noexec_repeat_count = 1;
                return;
            }
        }
        LOG_CRITICAL(Core_ARM, "Cannot execute instruction at unmapped address {:#016x}", pc);
        m_parent.LogBacktrace(m_process);
        ReturnException(pc, PrefetchAbort);
        return;
    default:
        if (m_debugger_enabled) {
            ReturnException(pc, InstructionBreakpoint);
        } else {
            // 1. Try direct LR return if LR points outside the abort stub
            const u64 lr = m_parent.m_jit->GetRegister(30);
            if (is_valid_return_target(lr)) {
                LOG_WARNING(Core_ARM, "Recovering from guest trap at PC={:#010x} via LR={:#010x}", pc, lr);
                m_parent.m_jit->SetRegister(0, 0);
                m_parent.m_jit->SetRegister(1, 0);
                m_parent.m_jit->SetPC(lr);
                m_parent.m_jit->HaltExecution(BreakLoop);
                return;
            }

            // 2. Walk the AArch64 Frame Pointer (X29) / stack call-site chain out of AbortImpl
            if (try_unwind_to_caller(lr, 0)) {
                return;
            }

            // 3. Fallback: advance PC past the offending UDF/BRK/trap instruction so execution does not hang
            if (m_memory.IsValidVirtualAddressRange(pc + 4, sizeof(u32))) {
                LOG_WARNING(Core_ARM, "Skipping guest trap instruction at PC={:#010x} -> {:#010x}", pc, pc + 4);
                m_parent.m_jit->SetRegister(0, 0);
                m_parent.m_jit->SetRegister(1, 0);
                m_parent.m_jit->SetPC(pc + 4);
                m_parent.m_jit->HaltExecution(BreakLoop);
                return;
            }
            static std::atomic<u32> s_exception_log_count{0};
            if (s_exception_log_count.fetch_add(1, std::memory_order_relaxed) < 8) {
                m_parent.LogBacktrace(m_process);
                LOG_CRITICAL(Core_ARM, "ExceptionRaised(exception = {}, pc = {:08X}, code = {:08X})", static_cast<std::size_t>(exception), pc, m_memory.Read32(pc));
            }
        }
    }
}

void DynarmicCallbacks64::CallSVC(u32 svc) {
    m_parent.m_svc = svc;
    m_parent.m_jit->HaltExecution(SupervisorCall);
}

void DynarmicCallbacks64::AddTicks(u64 ticks) {
    ASSERT(!m_parent.m_uses_wall_clock && "Dynarmic ticking disabled");
    // Divide the number of ticks by the amount of CPU cores. TODO(Subv): This yields only a
    // rough approximation of the amount of executed ticks in the system, it may be thrown off
    // if not all cores are doing a similar amount of work. Instead of doing this, we should
    // device a way so that timing is consistent across all cores without increasing the ticks 4
    // times.
    u64 amortized_ticks = ticks / Core::Hardware::NUM_CPU_CORES;
    // Always execute at least one tick.
    amortized_ticks = std::max<u64>(amortized_ticks, 1);
    m_parent.m_system.CoreTiming().AddTicks(amortized_ticks);
}

u64 DynarmicCallbacks64::GetTicksRemaining() {
    ASSERT(!m_parent.m_uses_wall_clock && "Dynarmic ticking disabled");
    return std::max<s64>(m_parent.m_system.CoreTiming().downcount, 0);
}

u64 DynarmicCallbacks64::GetCNTPCT() {
    return m_parent.m_system.CoreTiming().GetClockTicks();
}

bool DynarmicCallbacks64::CheckMemoryAccess(u64 addr, u64 size, Kernel::DebugWatchpointType type) {
    if (!m_check_memory_access) {
        return true;
    }

    if (!m_memory.IsValidVirtualAddressRange(addr, size)) {
        if (m_debugger_enabled) {
            LOG_CRITICAL(Core_ARM, "Stopping execution due to unmapped memory access at {:#x}",
                            addr);
            m_parent.m_jit->HaltExecution(PrefetchAbort);
        }
        return false;
    }

    if (!m_debugger_enabled) {
        return true;
    }

    const auto match{m_parent.MatchingWatchpoint(addr, size, type)};
    if (match) {
        m_parent.m_halted_watchpoint = match;
        m_parent.m_jit->HaltExecution(DataAbort);
        return false;
    }

    return true;
}

void DynarmicCallbacks64::ReturnException(u64 pc, Dynarmic::HaltReason hr) {
    m_parent.GetContext(m_parent.m_breakpoint_context);
    m_parent.m_breakpoint_context.pc = pc;
    m_parent.m_jit->HaltExecution(hr);
}

void ArmDynarmic64::MakeJit(Common::PageTable* page_table, std::size_t address_space_bits) {
    Dynarmic::A64::UserConfig config;

    // Callbacks
    config.callbacks = std::addressof(*m_cb);
    config.enable_pauth_returns = m_cb && m_cb->m_process && m_cb->m_process->UsesPAuthReturns();

    // Memory
    if (page_table) {
        constexpr size_t PageLog2Stride = 5;
        static_assert(1 << PageLog2Stride == sizeof(Common::PageTable::PageEntryData));

        config.page_table = reinterpret_cast<void**>(page_table->entries.data());
        config.page_table_address_space_bits = std::uint32_t(address_space_bits);
        config.page_table_pointer_mask_bits = Common::PageTable::ATTRIBUTE_BITS;
        config.page_table_log2_stride = PageLog2Stride;
        config.silently_mirror_page_table = false;
        config.absolute_offset_page_table = true;
        config.detect_misaligned_access_via_page_table = 16 | 32 | 64 | 128;
        config.only_detect_misalignment_via_page_table_on_page_boundary = true;

        config.fastmem_pointer = page_table->fastmem_arena ?
            std::optional<uintptr_t>{reinterpret_cast<uintptr_t>(page_table->fastmem_arena)} :
            std::nullopt;
        config.fastmem_address_space_bits = std::uint32_t(address_space_bits);
        config.silently_mirror_fastmem = false;

        config.fastmem_exclusive_access = config.fastmem_pointer != std::nullopt;
        config.recompile_on_exclusive_fastmem_failure = true;
    }

    // Multi-process state
    config.processor_id = std::uint8_t(m_core_index);
    config.global_monitor = &m_exclusive_monitor.monitor;

    // System registers
    config.tpidrro_el0 = &m_cb->m_tpidrro_el0;
    config.tpidr_el0 = &m_cb->m_tpidr_el0;
    config.dczid_el0 = 4;
    config.ctr_el0 = 0x8444c004;
    config.cntfrq_el0 = Hardware::CNTFREQ;

    // Unpredictable instructions
    config.define_unpredictable_behaviour = true;

    // Timing
    config.wall_clock_cntpct = m_uses_wall_clock;
    config.enable_cycle_counting = !m_uses_wall_clock;

    // Code cache size
#if defined(ARCHITECTURE_arm64) || defined(__sun__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
    config.code_cache_size = std::uint32_t(128_MiB);
#else
    config.code_cache_size = std::uint32_t(512_MiB);
#endif

    // Allow memory fault handling to work
    if (m_system.DebuggerEnabled()) {
        config.check_halt_on_memory_access = true;
    }

    // null_jit
    if (!page_table) {
        // Don't waste too much memory on null_jit
        config.code_cache_size = std::uint32_t(8_MiB);
    }

    switch (Settings::values.cpu_accuracy.GetValue()) {
    // Debug mode
    case Settings::CpuAccuracy::Debugging:
        if (!Settings::values.cpuopt_page_tables) {
            config.page_table = nullptr;
        }
        if (!Settings::values.cpuopt_block_linking) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::BlockLinking;
        }
        if (!Settings::values.cpuopt_return_stack_buffer) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::ReturnStackBuffer;
        }
        if (!Settings::values.cpuopt_fast_dispatcher) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::FastDispatch;
        }
        if (!Settings::values.cpuopt_context_elimination) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::GetSetElimination;
        }
        if (!Settings::values.cpuopt_const_prop) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::ConstProp;
        }
        if (!Settings::values.cpuopt_misc_ir) {
            config.optimizations &= ~Dynarmic::OptimizationFlag::MiscIROpt;
        }
        if (!Settings::values.cpuopt_reduce_misalign_checks) {
            config.only_detect_misalignment_via_page_table_on_page_boundary = false;
        }
        if (!Settings::values.cpuopt_fastmem) {
            config.fastmem_pointer = std::nullopt;
            config.fastmem_exclusive_access = false;
        }
        if (!Settings::values.cpuopt_fastmem_exclusives) {
            config.fastmem_exclusive_access = false;
        }
        if (!Settings::values.cpuopt_recompile_exclusives) {
            config.recompile_on_exclusive_fastmem_failure = false;
        }
        if (!Settings::values.cpuopt_ignore_memory_aborts) {
            config.check_halt_on_memory_access = true;
        }
        break;
    // Unsafe optimizations
    case Settings::CpuAccuracy::Unsafe:
        config.unsafe_optimizations = true;
        if (!Settings::values.cpuopt_unsafe_host_mmu) {
            config.fastmem_pointer = std::nullopt;
            config.fastmem_exclusive_access = false;
        }
        if (Settings::values.cpuopt_unsafe_unfuse_fma) {
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
        }
        if (Settings::values.cpuopt_unsafe_reduce_fp_error) {
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_ReducedErrorFP;
        }
        if (Settings::values.cpuopt_unsafe_inaccurate_nan) {
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_InaccurateNaN;
        }
        if (Settings::values.cpuopt_unsafe_fastmem_check) {
            config.fastmem_address_space_bits = 64;
        }
        if (Settings::values.cpuopt_unsafe_ignore_global_monitor) {
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreGlobalMonitor;
        }
        break;
    // Safe optimisations
    case Settings::CpuAccuracy::Auto:
        config.unsafe_optimizations = true;
        config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
        if (Settings::values.cpuopt_unsafe_ignore_global_monitor) {
            config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreGlobalMonitor;
        }
        break;
    // Paranoia mode for debugging optimizations
    case Settings::CpuAccuracy::Paranoid:
        config.unsafe_optimizations = false;
        config.optimizations = Dynarmic::no_optimizations;
        break;
    case Settings::CpuAccuracy::Accurate:
    default:
        break;
    }
    if (!Settings::IsFastmemEnabled()) {
        config.fastmem_pointer = std::nullopt;
        config.fastmem_exclusive_access = false;
    }
    StormTier2Compiler::ApplyTier2Config64(config, page_table == nullptr);
    m_jit.emplace(config);
}

HaltReason ArmDynarmic64::RunThread(Kernel::KThread* thread) {
    m_jit->ClearExclusiveState();
    return TranslateHaltReason(m_jit->Run());
}

HaltReason ArmDynarmic64::StepThread(Kernel::KThread* thread) {
    m_jit->ClearExclusiveState();
    return TranslateHaltReason(m_jit->Step());
}

u32 ArmDynarmic64::GetSvcNumber() const {
    return m_svc;
}

void ArmDynarmic64::GetSvcArguments(std::span<uint64_t, 8> args) const {
    Dynarmic::A64::Jit const& j = *m_jit;
    for (size_t i = 0; i < 8; i++)
        args[i] = j.GetRegister(i);
}

void ArmDynarmic64::SetSvcArguments(std::span<const uint64_t, 8> args) {
    Dynarmic::A64::Jit& j = *m_jit;
    for (size_t i = 0; i < 8; i++)
        j.SetRegister(i, args[i]);
}

const Kernel::DebugWatchpoint* ArmDynarmic64::HaltedWatchpoint() const {
    return m_halted_watchpoint;
}

void ArmDynarmic64::RewindBreakpointInstruction() {
    this->SetContext(m_breakpoint_context);
}

ArmDynarmic64::ArmDynarmic64(System& system, bool uses_wall_clock, Kernel::KProcess* process, DynarmicExclusiveMonitor& exclusive_monitor, std::size_t core_index)
    : ArmInterface{uses_wall_clock}, m_system{system}, m_exclusive_monitor{exclusive_monitor}
    , m_cb(std::make_optional<DynarmicCallbacks64>(*this, process))
    , m_core_index{core_index}
{
    auto& page_table = process->GetPageTable().GetBasePageTable();
    auto& page_table_impl = page_table.GetImpl();
    MakeJit(&page_table_impl, page_table.GetAddressSpaceWidth());
}

ArmDynarmic64::~ArmDynarmic64() = default;

void ArmDynarmic64::SetTpidrroEl0(u64 value) {
    m_cb->m_tpidrro_el0 = value;
}

void ArmDynarmic64::GetContext(Kernel::Svc::ThreadContext& ctx) const {
    Dynarmic::A64::Jit const& j = *m_jit;
    auto gpr = j.GetRegisters();
    auto fpr = j.GetVectors();
    // TODO: this is inconvenient
    for (size_t i = 0; i < 29; i++)
        ctx.r[i] = gpr[i];
    ctx.fp = gpr[29];
    ctx.lr = gpr[30];
    ctx.sp = j.GetSP();
    ctx.pc = j.GetPC();
    ctx.pstate = j.GetPstate();
    ctx.v = fpr;
    ctx.fpcr = j.GetFpcr();
    ctx.fpsr = j.GetFpsr();
    ctx.tpidr = m_cb->m_tpidr_el0;
}

void ArmDynarmic64::SetContext(const Kernel::Svc::ThreadContext& ctx) {
    Dynarmic::A64::Jit& j = *m_jit;
    // TODO: this is inconvenient
    std::array<u64, 31> gpr;
    for (size_t i = 0; i < 29; i++)
        gpr[i] = ctx.r[i];
    gpr[29] = ctx.fp;
    gpr[30] = ctx.lr;
    j.SetRegisters(gpr);
    j.SetSP(ctx.sp);
    j.SetPC(ctx.pc);
    j.SetPstate(ctx.pstate);
    j.SetVectors(ctx.v);
    j.SetFpcr(ctx.fpcr);
    j.SetFpsr(ctx.fpsr);
    m_cb->m_tpidr_el0 = ctx.tpidr;
}

void ArmDynarmic64::SignalInterrupt(Kernel::KThread* thread) {
    m_jit->HaltExecution(BreakLoop);
}

void ArmDynarmic64::ClearInstructionCache() {
    m_cb->last_code_addr = u64(-1);
    m_jit->ClearCache();
}

void ArmDynarmic64::InvalidateCacheRange(u64 addr, std::size_t size) {
    // Refetch the source page too, including instructions restored by the debugger.
    m_cb->last_code_addr = u64(-1);
    m_jit->InvalidateCacheRange(addr, size);
}

} // namespace Core
