// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
    static constexpr u32 A64_NOP = 0xD503201FU;

    [[nodiscard]] static inline u32 OptimizeInstruction64(u32 inst, bool enabled) noexcept {
        // Fast-path: 99.9% of A64 instructions are not in the 0xD5000000 system instruction space
        if ((inst & 0xFFC00000U) != 0xD5000000U) [[likely]] {
            return inst;
        }
        if (!enabled) {
            return inst;
        }

        // 1. Rewrite MRS Xn, CNTVCT_EL0 (0xD53BE040 | Rt) -> MRS Xn, CNTPCT_EL0 (0xD53BE020 | Rt)
        // SDK 18.x - 23.0.1 titles occasionally emit CNTVCT_EL0 directly; redirecting to CNTPCT_EL0
        // enables Dynarmic's inline wall-clock / cycle counter path without trapping.
        if ((inst & 0xFFFFFFE0U) == 0xD53BE040U) [[unlikely]] {
            return 0xD53BE020U | (inst & 0x1FU);
        }

        // 2. Canonicalize ARMv8.3 PAC / ARMv8.5 BTI hint instructions in System Hint space
        // System hints match 0xD503201F with CRm [11:8] and op2 [7:5]: Mask 0xFFFFF01F == 0xD503201F.
        // Preserve YIELD (0xD503203F), WFE (0xD503205F), WFI (0xD503207F), SEV (0xD503209F), SEVL (0xD50320BF).
        if ((inst & 0xFFFFF01FU) == 0xD503201FU) [[unlikely]] {
            if (inst != A64_NOP &&
                inst != 0xD503203FU && // YIELD
                inst != 0xD503205FU && // WFE
                inst != 0xD503207FU && // WFI
                inst != 0xD503209FU && // SEV
                inst != 0xD50320BFU) { // SEVL
                return A64_NOP;
            }
        }
        return inst;
    }

    [[nodiscard]] static inline u32 OptimizeInstruction64(u32 inst) noexcept {
        if ((inst & 0xFFC00000U) != 0xD5000000U) [[likely]] {
            return inst;
        }
        return OptimizeInstruction64(inst, Settings::values.use_storm_tier2_compiler.GetValue());
    }

    static inline void OptimizePage64(std::span<u32> page) noexcept {
        if (!Settings::values.use_storm_tier2_compiler.GetValue()) {
            return;
        }
        for (u32& inst : page) {
            if ((inst & 0xFFC00000U) == 0xD5000000U) [[unlikely]] {
                inst = OptimizeInstruction64(inst, true);
            }
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
        }
    }
};

} // namespace Core
