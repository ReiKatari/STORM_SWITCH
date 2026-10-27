// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <span>
#include <dynarmic/interface/A32/config.h>
#include <dynarmic/interface/A64/config.h>
#include <dynarmic/interface/optimization_flags.h>
#include "common/common_types.h"
#include "common/literals.h"
#include "common/settings.h"

namespace Core {

/**
 * StormTier2Compiler:
 * Tier-2 AArch64/AArch32 Instruction Stream Pre-Optimizer & Superblock JIT Tuner for STORM SWITCH.
 *
 * Works alongside Dynarmic JIT and NCE to:
 * 1. Pre-optimize 4 KiB guest code pages before Dynarmic IR lifting:
 *    - Canonicalizes ARMv8.3-A PAC (PACIASP/AUTIASP/PACIBSP/AUTIBSP) and ARMv8.5-A BTI
 *      hint instructions into canonical NOP (0xD503201F) so basic blocks are not split.
 *    - Rewrites guest virtual counter reads (MRS Xn, CNTVCT_EL0 -> MRS Xn, CNTPCT_EL0)
 *      to use Dynarmic's inline fast-path counter read without undefined instruction traps.
 * 2. Tunes Dynarmic Tier-2 IR optimization flags and code cache geometry for large AAA titles
 *    (Xenoblade Chronicles 1/2/3, Hogwarts Legacy, EA Sports FC 24-27, Disco Elysium).
 */
class StormTier2Compiler {
public:
    static constexpr u32 A64_NOP   = 0xD503201FU;
    static constexpr u32 A64_YIELD = 0xD503203FU;
    static constexpr u32 A64_WFE   = 0xD503205FU;
    static constexpr u32 A64_WFI   = 0xD503207FU;
    static constexpr u32 A64_SEV   = 0xD503209FU;
    static constexpr u32 A64_SEVL  = 0xD50320BFU;

    // ARMv8.7 WFET / WFIT (Hint encodings)
    static constexpr u32 A64_WFET_MASK = 0xFFFFFFE0U;
    static constexpr u32 A64_WFET_VAL  = 0xD5031000U;

    // Redundant MOV Xd, Xm (encoded as ORR Xd, XZR, Xm: 0xAA0003E0 | (Rm << 16) | Rd)
    static constexpr u32 A64_MOV_REG_MASK = 0xFFE0FFE0U;
    static constexpr u32 A64_MOV_REG_VAL  = 0xAA0003E0U;

    static inline std::atomic<u64> s_rewritten_inst_count{0};
    static inline std::atomic<u64> s_superblock_opt_count{0};

    [[nodiscard]] static inline u64 GetRewrittenCount() noexcept {
        return s_rewritten_inst_count.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static inline u64 GetSuperblockOptCount() noexcept {
        return s_superblock_opt_count.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static inline bool IsRedundantSelfMov64(u32 inst) noexcept {
        if ((inst & A64_MOV_REG_MASK) != A64_MOV_REG_VAL) [[likely]] {
            return false;
        }
        const u32 rd = inst & 0x1FU;
        const u32 rm = (inst >> 16) & 0x1FU;
        return rd == rm && rd != 31U;
    }

    [[nodiscard]] static inline u32 OptimizeInstruction64(u32 inst, bool enabled) noexcept {
        if (!enabled) {
            return inst;
        }

        // Fold redundant self-MOV (MOV Xd, Xd -> NOP)
        if (IsRedundantSelfMov64(inst)) [[unlikely]] {
            s_superblock_opt_count.fetch_add(1, std::memory_order_relaxed);
            return A64_NOP;
        }

        // Fast-path: 99.9% of remaining A64 instructions are not in the 0xD5000000 system space
        if ((inst & 0xFFC00000U) != 0xD5000000U) [[likely]] {
            return inst;
        }

        // 1. Rewrite MRS Xn, CNTVCT_EL0 (0xD53BE040 | Rt) -> MRS Xn, CNTPCT_EL0 (0xD53BE020 | Rt)
        if ((inst & 0xFFFFFFE0U) == 0xD53BE040U) [[unlikely]] {
            s_rewritten_inst_count.fetch_add(1, std::memory_order_relaxed);
            return 0xD53BE020U | (inst & 0x1FU);
        }

        // 2. Eliminate basic-block exits on SEV (0xD503209F) & SEVL (0xD50320BF):
        // Dynarmic terminates the basic block and exits to C++ ExceptionRaised (which is a no-op)
        // on every SEV/SEVL. Rewriting them to NOP keeps spinlocks inside the JIT superblock.
        if (inst == A64_SEV || inst == A64_SEVL) [[unlikely]] {
            s_rewritten_inst_count.fetch_add(1, std::memory_order_relaxed);
            return A64_NOP;
        }

        // 3. Fast WFE/WFET -> YIELD rewrite under Low-End Turbo / Unsafe CPU to avoid micro-stutters
        if ((inst == A64_WFE || (inst & A64_WFET_MASK) == A64_WFET_VAL) &&
            (Settings::values.storm_lowend_turbo.GetValue() ||
             Settings::values.cpu_accuracy.GetValue() == Settings::CpuAccuracy::Unsafe)) [[unlikely]] {
            s_rewritten_inst_count.fetch_add(1, std::memory_order_relaxed);
            return A64_YIELD;
        }

        // 4. Canonicalize ARMv8.3 PAC / ARMv8.5 BTI hint instructions in System Hint space
        // Preserve only YIELD (0xD503203F), WFE (0xD503205F), WFI (0xD503207F).
        if ((inst & 0xFFFFF01FU) == 0xD503201FU) [[unlikely]] {
            if (inst != A64_NOP &&
                inst != A64_YIELD &&
                inst != A64_WFE &&
                inst != A64_WFI) {
                s_rewritten_inst_count.fetch_add(1, std::memory_order_relaxed);
                return A64_NOP;
            }
        }
        return inst;
    }

    [[nodiscard]] static inline u32 OptimizeInstruction64(u32 inst) noexcept {
        if ((inst & 0xFFC00000U) != 0xD5000000U && !IsRedundantSelfMov64(inst)) [[likely]] {
            return inst;
        }
        return OptimizeInstruction64(inst, Settings::values.use_storm_tier2_compiler.GetValue());
    }

    static inline void OptimizePage64(std::span<u32> page) noexcept {
        if (!Settings::values.use_storm_tier2_compiler.GetValue() || page.empty()) {
            return;
        }
        u64 rewritten = 0;
        u64 superblock_opts = 0;
        const bool wfe_to_yield =
            Settings::values.storm_lowend_turbo.GetValue() ||
            Settings::values.cpu_accuracy.GetValue() == Settings::CpuAccuracy::Unsafe;

        for (size_t i = 0; i < page.size(); ++i) {
            u32 inst = page[i];
            if ((inst & 0xFFC00000U) == 0xD5000000U) [[unlikely]] {
                if ((inst & 0xFFFFFFE0U) == 0xD53BE040U) {
                    page[i] = 0xD53BE020U | (inst & 0x1FU);
                    ++rewritten;
                } else if (inst == A64_SEV || inst == A64_SEVL) {
                    page[i] = A64_NOP;
                    ++rewritten;
                } else if (wfe_to_yield &&
                           (inst == A64_WFE || (inst & A64_WFET_MASK) == A64_WFET_VAL)) {
                    page[i] = A64_YIELD;
                    ++rewritten;
                } else if ((inst & 0xFFFFF01FU) == 0xD503201FU &&
                           inst != A64_NOP && inst != A64_YIELD &&
                           inst != A64_WFE && inst != A64_WFI) {
                    page[i] = A64_NOP;
                    ++rewritten;
                }
                inst = page[i];
            } else if (IsRedundantSelfMov64(inst)) [[unlikely]] {
                page[i] = A64_NOP;
                inst = A64_NOP;
                ++superblock_opts;
            }
            // Superblock 2-instruction peephole: collapse back-to-back YIELD hints into YIELD + NOP
            if (i > 0 && inst == A64_YIELD && page[i - 1] == A64_YIELD) [[unlikely]] {
                page[i] = A64_NOP;
                ++superblock_opts;
            }
        }
        if (rewritten > 0) {
            s_rewritten_inst_count.fetch_add(rewritten, std::memory_order_relaxed);
        }
        if (superblock_opts > 0) {
            s_superblock_opt_count.fetch_add(superblock_opts, std::memory_order_relaxed);
        }
    }

    static void ApplyTier2Config64(Dynarmic::A64::UserConfig& config, bool is_null_jit) noexcept {
        using namespace Common::Literals;
        if (!Settings::values.use_storm_tier2_compiler.GetValue()) {
            return;
        }
        if (!is_null_jit) {
#if defined(ARCHITECTURE_arm64)
            config.code_cache_size = (std::max)(config.code_cache_size, static_cast<std::uint32_t>(128_MiB));
#else
            config.code_cache_size = (std::max)(config.code_cache_size, static_cast<std::uint32_t>(512_MiB));
#endif
        }

        const auto accuracy = Settings::values.cpu_accuracy.GetValue();
        if (accuracy == Settings::CpuAccuracy::Auto || accuracy == Settings::CpuAccuracy::Unsafe) {
            config.optimizations |= Dynarmic::OptimizationFlag::BlockLinking;
            config.optimizations |= Dynarmic::OptimizationFlag::ReturnStackBuffer;
            config.optimizations |= Dynarmic::OptimizationFlag::FastDispatch;
            config.optimizations |= Dynarmic::OptimizationFlag::GetSetElimination;
            config.optimizations |= Dynarmic::OptimizationFlag::ConstProp;
            config.optimizations |= Dynarmic::OptimizationFlag::MiscIROpt;
            if (Settings::values.cpuopt_unsafe_ignore_standard_fpcr.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreStandardFPCRValue;
            }
            if (Settings::values.cpuopt_unsafe_unfuse_fma.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
            }
            if (Settings::values.cpuopt_unsafe_reduce_fp_error.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_ReducedErrorFP;
            }
        }
    }

    static void ApplyTier2Config32(Dynarmic::A32::UserConfig& config, bool is_null_jit) noexcept {
        using namespace Common::Literals;
        if (!Settings::values.use_storm_tier2_compiler.GetValue()) {
            return;
        }
        if (!is_null_jit) {
#if defined(ARCHITECTURE_arm64)
            config.code_cache_size = (std::max)(config.code_cache_size, static_cast<std::uint32_t>(128_MiB));
#else
            config.code_cache_size = (std::max)(config.code_cache_size, static_cast<std::uint32_t>(256_MiB));
#endif
        }

        const auto accuracy = Settings::values.cpu_accuracy.GetValue();
        if (accuracy == Settings::CpuAccuracy::Auto || accuracy == Settings::CpuAccuracy::Unsafe) {
            config.optimizations |= Dynarmic::OptimizationFlag::BlockLinking;
            config.optimizations |= Dynarmic::OptimizationFlag::ReturnStackBuffer;
            config.optimizations |= Dynarmic::OptimizationFlag::FastDispatch;
            config.optimizations |= Dynarmic::OptimizationFlag::GetSetElimination;
            config.optimizations |= Dynarmic::OptimizationFlag::ConstProp;
            config.optimizations |= Dynarmic::OptimizationFlag::MiscIROpt;
            if (Settings::values.cpuopt_unsafe_ignore_standard_fpcr.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_IgnoreStandardFPCRValue;
            }
            if (Settings::values.cpuopt_unsafe_unfuse_fma.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_UnfuseFMA;
            }
            if (Settings::values.cpuopt_unsafe_reduce_fp_error.GetValue()) {
                config.optimizations |= Dynarmic::OptimizationFlag::Unsafe_ReducedErrorFP;
            }
        }
    }
};

namespace ARM {
using StormTier2Compiler = ::Core::StormTier2Compiler;
} // namespace ARM

} // namespace Core
