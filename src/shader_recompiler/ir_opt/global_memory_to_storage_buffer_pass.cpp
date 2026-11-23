// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <optional>
#include <unordered_map>

#include <boost/container/flat_set.hpp>
#include <boost/container/small_vector.hpp>

#include "common/alignment.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/breadth_first_search.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {
namespace {
/// Address in constant buffers to the storage buffer descriptor
struct StorageBufferAddr {
    auto operator<=>(const StorageBufferAddr&) const noexcept = default;

    u32 index;
    u32 offset;
};

/// Block iterator to a global memory instruction and the storage buffer it uses
struct StorageInst {
    StorageBufferAddr storage_buffer;
    IR::Inst* inst;
    IR::Block* block;
};

/// Bias towards a certain range of constant buffers when looking for storage buffers
struct Bias {
    u32 index;
    u32 offset_begin;
    u32 offset_end;
    u32 alignment;
};

using boost::container::flat_set;
using boost::container::small_vector;
using StorageBufferSet =
    flat_set<StorageBufferAddr, std::less<StorageBufferAddr>, small_vector<StorageBufferAddr, 16>>;
using StorageInstVector = small_vector<StorageInst, 24>;
using StorageWritesSet =
    flat_set<StorageBufferAddr, std::less<StorageBufferAddr>, small_vector<StorageBufferAddr, 16>>;

struct StorageInfo {
    StorageBufferSet set;
    StorageInstVector to_replace;
    StorageWritesSet writes;
    /// Track results by the instruction the search starts from, with and without the NVN bias:
    /// accesses through the same address share them, and a search can walk thousands of
    /// instructions (the IR does not change while storage buffers are collected).
    std::unordered_map<const IR::Inst*, std::optional<StorageBufferAddr>> biased_tracks;
    std::unordered_map<const IR::Inst*, std::optional<StorageBufferAddr>> unbiased_tracks;
};

/// Returns true when the instruction is a global memory instruction
bool IsGlobalMemory(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadGlobalS8:
    case IR::Opcode::LoadGlobalU8:
    case IR::Opcode::LoadGlobalS16:
    case IR::Opcode::LoadGlobalU16:
    case IR::Opcode::LoadGlobal32:
    case IR::Opcode::LoadGlobal64:
    case IR::Opcode::LoadGlobal128:
    case IR::Opcode::WriteGlobalS8:
    case IR::Opcode::WriteGlobalU8:
    case IR::Opcode::WriteGlobalS16:
    case IR::Opcode::WriteGlobalU16:
    case IR::Opcode::WriteGlobal32:
    case IR::Opcode::WriteGlobal64:
    case IR::Opcode::WriteGlobal128:
    case IR::Opcode::GlobalAtomicIAdd32:
    case IR::Opcode::GlobalAtomicSMin32:
    case IR::Opcode::GlobalAtomicUMin32:
    case IR::Opcode::GlobalAtomicSMax32:
    case IR::Opcode::GlobalAtomicUMax32:
    case IR::Opcode::GlobalAtomicInc32:
    case IR::Opcode::GlobalAtomicDec32:
    case IR::Opcode::GlobalAtomicAnd32:
    case IR::Opcode::GlobalAtomicOr32:
    case IR::Opcode::GlobalAtomicXor32:
    case IR::Opcode::GlobalAtomicExchange32:
    case IR::Opcode::GlobalAtomicIAdd64:
    case IR::Opcode::GlobalAtomicSMin64:
    case IR::Opcode::GlobalAtomicUMin64:
    case IR::Opcode::GlobalAtomicSMax64:
    case IR::Opcode::GlobalAtomicUMax64:
    case IR::Opcode::GlobalAtomicAnd64:
    case IR::Opcode::GlobalAtomicOr64:
    case IR::Opcode::GlobalAtomicXor64:
    case IR::Opcode::GlobalAtomicExchange64:
    case IR::Opcode::GlobalAtomicIAdd32x2:
    case IR::Opcode::GlobalAtomicSMin32x2:
    case IR::Opcode::GlobalAtomicUMin32x2:
    case IR::Opcode::GlobalAtomicSMax32x2:
    case IR::Opcode::GlobalAtomicUMax32x2:
    case IR::Opcode::GlobalAtomicAnd32x2:
    case IR::Opcode::GlobalAtomicOr32x2:
    case IR::Opcode::GlobalAtomicXor32x2:
    case IR::Opcode::GlobalAtomicExchange32x2:
    case IR::Opcode::GlobalAtomicAddF32:
    case IR::Opcode::GlobalAtomicAddF16x2:
    case IR::Opcode::GlobalAtomicAddF32x2:
    case IR::Opcode::GlobalAtomicMinF16x2:
    case IR::Opcode::GlobalAtomicMinF32x2:
    case IR::Opcode::GlobalAtomicMaxF16x2:
    case IR::Opcode::GlobalAtomicMaxF32x2:
        return true;
    default:
        return false;
    }
}

/// Returns true when the instruction is a global memory instruction
bool IsGlobalMemoryWrite(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::WriteGlobalS8:
    case IR::Opcode::WriteGlobalU8:
    case IR::Opcode::WriteGlobalS16:
    case IR::Opcode::WriteGlobalU16:
    case IR::Opcode::WriteGlobal32:
    case IR::Opcode::WriteGlobal64:
    case IR::Opcode::WriteGlobal128:
    case IR::Opcode::GlobalAtomicIAdd32:
    case IR::Opcode::GlobalAtomicSMin32:
    case IR::Opcode::GlobalAtomicUMin32:
    case IR::Opcode::GlobalAtomicSMax32:
    case IR::Opcode::GlobalAtomicUMax32:
    case IR::Opcode::GlobalAtomicInc32:
    case IR::Opcode::GlobalAtomicDec32:
    case IR::Opcode::GlobalAtomicAnd32:
    case IR::Opcode::GlobalAtomicOr32:
    case IR::Opcode::GlobalAtomicXor32:
    case IR::Opcode::GlobalAtomicExchange32:
    case IR::Opcode::GlobalAtomicIAdd64:
    case IR::Opcode::GlobalAtomicSMin64:
    case IR::Opcode::GlobalAtomicUMin64:
    case IR::Opcode::GlobalAtomicSMax64:
    case IR::Opcode::GlobalAtomicUMax64:
    case IR::Opcode::GlobalAtomicAnd64:
    case IR::Opcode::GlobalAtomicOr64:
    case IR::Opcode::GlobalAtomicXor64:
    case IR::Opcode::GlobalAtomicExchange64:
    case IR::Opcode::GlobalAtomicIAdd32x2:
    case IR::Opcode::GlobalAtomicSMin32x2:
    case IR::Opcode::GlobalAtomicUMin32x2:
    case IR::Opcode::GlobalAtomicSMax32x2:
    case IR::Opcode::GlobalAtomicUMax32x2:
    case IR::Opcode::GlobalAtomicAnd32x2:
    case IR::Opcode::GlobalAtomicOr32x2:
    case IR::Opcode::GlobalAtomicXor32x2:
    case IR::Opcode::GlobalAtomicExchange32x2:
    case IR::Opcode::GlobalAtomicAddF32:
    case IR::Opcode::GlobalAtomicAddF16x2:
    case IR::Opcode::GlobalAtomicAddF32x2:
    case IR::Opcode::GlobalAtomicMinF16x2:
    case IR::Opcode::GlobalAtomicMinF32x2:
    case IR::Opcode::GlobalAtomicMaxF16x2:
    case IR::Opcode::GlobalAtomicMaxF32x2:
        return true;
    default:
        return false;
    }
}

/// Converts a global memory opcode to its storage buffer equivalent
IR::Opcode GlobalToStorage(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::LoadGlobalS8:
        return IR::Opcode::LoadStorageS8;
    case IR::Opcode::LoadGlobalU8:
        return IR::Opcode::LoadStorageU8;
    case IR::Opcode::LoadGlobalS16:
        return IR::Opcode::LoadStorageS16;
    case IR::Opcode::LoadGlobalU16:
        return IR::Opcode::LoadStorageU16;
    case IR::Opcode::LoadGlobal32:
        return IR::Opcode::LoadStorage32;
    case IR::Opcode::LoadGlobal64:
        return IR::Opcode::LoadStorage64;
    case IR::Opcode::LoadGlobal128:
        return IR::Opcode::LoadStorage128;
    case IR::Opcode::WriteGlobalS8:
        return IR::Opcode::WriteStorageS8;
    case IR::Opcode::WriteGlobalU8:
        return IR::Opcode::WriteStorageU8;
    case IR::Opcode::WriteGlobalS16:
        return IR::Opcode::WriteStorageS16;
    case IR::Opcode::WriteGlobalU16:
        return IR::Opcode::WriteStorageU16;
    case IR::Opcode::WriteGlobal32:
        return IR::Opcode::WriteStorage32;
    case IR::Opcode::WriteGlobal64:
        return IR::Opcode::WriteStorage64;
    case IR::Opcode::WriteGlobal128:
        return IR::Opcode::WriteStorage128;
    case IR::Opcode::GlobalAtomicIAdd32:
        return IR::Opcode::StorageAtomicIAdd32;
    case IR::Opcode::GlobalAtomicSMin32:
        return IR::Opcode::StorageAtomicSMin32;
    case IR::Opcode::GlobalAtomicUMin32:
        return IR::Opcode::StorageAtomicUMin32;
    case IR::Opcode::GlobalAtomicSMax32:
        return IR::Opcode::StorageAtomicSMax32;
    case IR::Opcode::GlobalAtomicUMax32:
        return IR::Opcode::StorageAtomicUMax32;
    case IR::Opcode::GlobalAtomicInc32:
        return IR::Opcode::StorageAtomicInc32;
    case IR::Opcode::GlobalAtomicDec32:
        return IR::Opcode::StorageAtomicDec32;
    case IR::Opcode::GlobalAtomicAnd32:
        return IR::Opcode::StorageAtomicAnd32;
    case IR::Opcode::GlobalAtomicOr32:
        return IR::Opcode::StorageAtomicOr32;
    case IR::Opcode::GlobalAtomicXor32:
        return IR::Opcode::StorageAtomicXor32;
    case IR::Opcode::GlobalAtomicExchange32:
        return IR::Opcode::StorageAtomicExchange32;
    case IR::Opcode::GlobalAtomicIAdd64:
        return IR::Opcode::StorageAtomicIAdd64;
    case IR::Opcode::GlobalAtomicSMin64:
        return IR::Opcode::StorageAtomicSMin64;
    case IR::Opcode::GlobalAtomicUMin64:
        return IR::Opcode::StorageAtomicUMin64;
    case IR::Opcode::GlobalAtomicSMax64:
        return IR::Opcode::StorageAtomicSMax64;
    case IR::Opcode::GlobalAtomicUMax64:
        return IR::Opcode::StorageAtomicUMax64;
    case IR::Opcode::GlobalAtomicAnd64:
        return IR::Opcode::StorageAtomicAnd64;
    case IR::Opcode::GlobalAtomicOr64:
        return IR::Opcode::StorageAtomicOr64;
    case IR::Opcode::GlobalAtomicXor64:
        return IR::Opcode::StorageAtomicXor64;
    case IR::Opcode::GlobalAtomicExchange64:
        return IR::Opcode::StorageAtomicExchange64;
    case IR::Opcode::GlobalAtomicIAdd32x2:
        return IR::Opcode::StorageAtomicIAdd32x2;
    case IR::Opcode::GlobalAtomicSMin32x2:
        return IR::Opcode::StorageAtomicSMin32x2;
    case IR::Opcode::GlobalAtomicUMin32x2:
        return IR::Opcode::StorageAtomicUMin32x2;
    case IR::Opcode::GlobalAtomicSMax32x2:
        return IR::Opcode::StorageAtomicSMax32x2;
    case IR::Opcode::GlobalAtomicUMax32x2:
        return IR::Opcode::StorageAtomicUMax32x2;
    case IR::Opcode::GlobalAtomicAnd32x2:
        return IR::Opcode::StorageAtomicAnd32x2;
    case IR::Opcode::GlobalAtomicOr32x2:
        return IR::Opcode::StorageAtomicOr32x2;
    case IR::Opcode::GlobalAtomicXor32x2:
        return IR::Opcode::StorageAtomicXor32x2;
    case IR::Opcode::GlobalAtomicExchange32x2:
        return IR::Opcode::StorageAtomicExchange32x2;
    case IR::Opcode::GlobalAtomicAddF32:
        return IR::Opcode::StorageAtomicAddF32;
    case IR::Opcode::GlobalAtomicAddF16x2:
        return IR::Opcode::StorageAtomicAddF16x2;
    case IR::Opcode::GlobalAtomicMinF16x2:
        return IR::Opcode::StorageAtomicMinF16x2;
    case IR::Opcode::GlobalAtomicMaxF16x2:
        return IR::Opcode::StorageAtomicMaxF16x2;
    case IR::Opcode::GlobalAtomicAddF32x2:
        return IR::Opcode::StorageAtomicAddF32x2;
    case IR::Opcode::GlobalAtomicMinF32x2:
        return IR::Opcode::StorageAtomicMinF32x2;
    case IR::Opcode::GlobalAtomicMaxF32x2:
        return IR::Opcode::StorageAtomicMaxF32x2;
    default:
        throw InvalidArgument("Invalid global memory opcode {}", opcode);
    }
}

/// Returns true when a storage buffer address satisfies a bias
bool MeetsBias(const StorageBufferAddr& storage_buffer, const Bias& bias) noexcept {
    return storage_buffer.index == bias.index && storage_buffer.offset >= bias.offset_begin &&
           storage_buffer.offset < bias.offset_end;
}

struct LowAddrInfo {
    IR::U32 value;
    s32 imm_offset;
    bool exact_low_addr;
};

constexpr size_t kMaxAddrTrackSteps = 256;

std::optional<LowAddrInfo> ExtractExactLowAddr(const IR::Value& addr, s32 imm_offset, u32 depth = 0) {
    if (depth >= 24 || addr.IsImmediate()) {
        return std::nullopt;
    }
    const IR::Inst* const inst{addr.InstRecursive()};
    switch (inst->GetOpcode()) {
    case IR::Opcode::IAdd64: {
        const IR::Value arg0{inst->Arg(0)};
        const IR::Value arg1{inst->Arg(1)};
        if (arg1.IsImmediate()) {
            const s32 delta = static_cast<s32>(static_cast<s64>(arg1.U64()));
            const u32 sum = static_cast<u32>(imm_offset) + static_cast<u32>(delta);
            return ExtractExactLowAddr(arg0, static_cast<s32>(sum), depth + 1);
        }
        if (arg0.IsImmediate()) {
            const s32 delta = static_cast<s32>(static_cast<s64>(arg0.U64()));
            const u32 sum = static_cast<u32>(imm_offset) + static_cast<u32>(delta);
            return ExtractExactLowAddr(arg1, static_cast<s32>(sum), depth + 1);
        }
        return std::nullopt;
    }
    case IR::Opcode::ISub64: {
        const IR::Value arg0{inst->Arg(0)};
        const IR::Value arg1{inst->Arg(1)};
        if (arg1.IsImmediate()) {
            const s32 delta = static_cast<s32>(static_cast<s64>(arg1.U64()));
            const u32 diff = static_cast<u32>(imm_offset) - static_cast<u32>(delta);
            return ExtractExactLowAddr(arg0, static_cast<s32>(diff), depth + 1);
        }
        return std::nullopt;
    }
    case IR::Opcode::ConvertU64U32:
        return LowAddrInfo{
            .value{IR::U32{inst->Arg(0)}},
            .imm_offset = imm_offset,
            .exact_low_addr = true,
        };
    case IR::Opcode::PackUint2x32: {
        const IR::Value vector{inst->Arg(0)};
        if (vector.IsImmediate()) {
            return std::nullopt;
        }
        const IR::Inst* const vec_inst{vector.InstRecursive()};
        if (vec_inst->GetOpcode() == IR::Opcode::CompositeConstructU32x2) {
            return LowAddrInfo{
                .value{IR::U32{vec_inst->Arg(0)}},
                .imm_offset = imm_offset,
                .exact_low_addr = true,
            };
        }
        return std::nullopt;
    }
    case IR::Opcode::CompositeConstructU32x2:
        return LowAddrInfo{
            .value{IR::U32{inst->Arg(0)}},
            .imm_offset = imm_offset,
            .exact_low_addr = true,
        };
    default:
        return std::nullopt;
    }
}

/// Tries to track the first 32-bits of a global memory instruction (exact low-32 fast path)
std::optional<LowAddrInfo> TrackLowAddress(IR::Inst* inst) {
    return ExtractExactLowAddr(inst->Arg(0), 0);
}

std::optional<u32> ExtractCbufBaseOffset(const IR::Value& value, u32 depth = 0) {
    if (value.IsImmediate()) {
        return value.U32();
    }
    if (depth >= 24) {
        return std::nullopt;
    }
    const IR::Inst* const inst{value.InstRecursive()};
    if (!inst) {
        return std::nullopt;
    }
    switch (inst->GetOpcode()) {
    case IR::Opcode::Identity:
        return ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
    case IR::Opcode::IAdd32: {
        const auto lhs = ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
        const auto rhs = ExtractCbufBaseOffset(inst->Arg(1), depth + 1);
        if (lhs && rhs) {
            return *lhs + *rhs;
        }
        return std::nullopt;
    }
    case IR::Opcode::ISub32: {
        const auto lhs = ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
        const auto rhs = ExtractCbufBaseOffset(inst->Arg(1), depth + 1);
        if (lhs && rhs) {
            return *lhs - *rhs;
        }
        return std::nullopt;
    }
    case IR::Opcode::ShiftLeftLogical32: {
        const auto lhs = ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
        const auto rhs = ExtractCbufBaseOffset(inst->Arg(1), depth + 1);
        if (lhs && rhs && *rhs < 32U) {
            return *lhs << *rhs;
        }
        return std::nullopt;
    }
    case IR::Opcode::BitwiseAnd32: {
        const auto lhs = ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
        const auto rhs = ExtractCbufBaseOffset(inst->Arg(1), depth + 1);
        if (lhs && rhs) {
            return *lhs & *rhs;
        }
        return std::nullopt;
    }
    case IR::Opcode::BitwiseOr32: {
        const auto lhs = ExtractCbufBaseOffset(inst->Arg(0), depth + 1);
        const auto rhs = ExtractCbufBaseOffset(inst->Arg(1), depth + 1);
        if (lhs && rhs) {
            return *lhs | *rhs;
        }
        return std::nullopt;
    }
    case IR::Opcode::Phi: {
        if (inst->NumArgs() == 0) {
            return std::nullopt;
        }
        std::optional<u32> agreed;
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            const auto branch = ExtractCbufBaseOffset(inst->Arg(i), depth + 1);
            if (!branch) {
                return std::nullopt;
            }
            if (!agreed) {
                agreed = *branch;
            } else if (*agreed != *branch) {
                return std::nullopt;
            }
        }
        return agreed;
    }
    default:
        return std::nullopt;
    }
}

/// Tries to track the storage buffer address used by a global memory instruction
std::optional<StorageBufferAddr> Track(const IR::Value& value, const Bias* bias) {
    const auto pred{[bias](const IR::Inst* inst) -> std::optional<StorageBufferAddr> {
        if (inst->GetOpcode() != IR::Opcode::GetCbufU32 &&
            inst->GetOpcode() != IR::Opcode::GetCbufF32 &&
            inst->GetOpcode() != IR::Opcode::GetCbufU32x2) {
            return std::nullopt;
        }
        const IR::Value index{inst->Arg(0)};
        const IR::Value offset{inst->Arg(1)};
        if (!index.IsImmediate()) {
            // Definitely not a storage buffer if it's read from a
            // non-immediate index
            return std::nullopt;
        }
        const std::optional<u32> extracted_offset{ExtractCbufBaseOffset(offset)};
        if (!extracted_offset) {
            return std::nullopt;
        }
        const u32 offset_val{*extracted_offset};
        const StorageBufferAddr storage_buffer{
            .index = index.U32(),
            .offset = offset_val,
        };
        const u32 alignment{bias ? bias->alignment : 8U};
        if (!Common::IsAligned(storage_buffer.offset, alignment)) {
            // The SSBO pointer has to be aligned
            return std::nullopt;
        }
        if (bias && !MeetsBias(storage_buffer, *bias)) {
            // We have to blacklist some addresses in case we wrongly
            // point to them
            return std::nullopt;
        }
        return storage_buffer;
    }};
    return BreadthFirstSearch(value, pred);
}

/// Collects the storage buffer used by a global memory instruction and the instruction itself
void CollectStorageBuffers(IR::Block& block, IR::Inst& inst, StorageInfo& info) {
    // NVN puts storage buffers in a specific range, we have to bias towards these addresses to
    // avoid getting false positives
    static constexpr Bias nvn_bias{
        .index = 0,
        .offset_begin = 0x100,
        .offset_end = 0x700,
        .alignment = 16,
    };
    const IR::Value addr{inst.Arg(0)};
    if (addr.IsImmediate()) {
        return;
    }
    const auto cached_track = [&](const IR::Value& low_addr, const Bias* bias, auto& cache) {
        if (low_addr.IsImmediate()) {
            return Track(low_addr, bias);
        }
        const IR::Inst* const start{low_addr.InstRecursive()};
        if (const auto it = cache.find(start); it != cache.end()) {
            return it->second;
        }
        const std::optional<StorageBufferAddr> result{Track(low_addr, bias)};
        cache.emplace(start, result);
        return result;
    };
    enum class ResolveStatus : u8 { Failed, Cycle, Resolved };
    struct ResolveResult {
        ResolveStatus status{ResolveStatus::Failed};
        StorageBufferAddr addr{};
    };
    auto resolve_expr = [&](auto& self, const IR::Value& value, const Bias* bias, auto& cache,
                            boost::container::small_vector<const IR::Inst*, 64>& on_stack,
                            size_t& steps, bool allow_u32_ptr) -> ResolveResult {
        if (value.IsImmediate() || steps >= kMaxAddrTrackSteps) {
            return {};
        }
        const IR::Inst* const node{value.InstRecursive()};
        if (std::ranges::find(on_stack, node) != on_stack.end()) {
            return {.status = ResolveStatus::Cycle};
        }
        ++steps;
        on_stack.push_back(node);
        struct PopGuard {
            boost::container::small_vector<const IR::Inst*, 64>& s;
            ~PopGuard() { s.pop_back(); }
        } pop{on_stack};

        switch (node->GetOpcode()) {
        case IR::Opcode::IAdd64: {
            const IR::Value arg0{node->Arg(0)};
            const IR::Value arg1{node->Arg(1)};
            if (arg1.IsImmediate()) {
                return self(self, arg0, bias, cache, on_stack, steps, allow_u32_ptr);
            }
            if (arg0.IsImmediate()) {
                return self(self, arg1, bias, cache, on_stack, steps, allow_u32_ptr);
            }
            const ResolveResult lhs = self(self, arg0, bias, cache, on_stack, steps, false);
            const ResolveResult rhs = self(self, arg1, bias, cache, on_stack, steps, false);
            if (lhs.status == ResolveStatus::Resolved && rhs.status == ResolveStatus::Resolved) {
                return lhs.addr == rhs.addr ? lhs : ResolveResult{};
            }
            if (lhs.status == ResolveStatus::Resolved) {
                return lhs;
            }
            if (rhs.status == ResolveStatus::Resolved) {
                return rhs;
            }
            if (lhs.status == ResolveStatus::Cycle || rhs.status == ResolveStatus::Cycle) {
                return {.status = ResolveStatus::Cycle};
            }
            return {};
        }
        case IR::Opcode::ISub64: {
            const IR::Value arg0{node->Arg(0)};
            const IR::Value arg1{node->Arg(1)};
            if (arg1.IsImmediate()) {
                return self(self, arg0, bias, cache, on_stack, steps, allow_u32_ptr);
            }
            const ResolveResult lhs = self(self, arg0, bias, cache, on_stack, steps, false);
            if (lhs.status == ResolveStatus::Resolved || lhs.status == ResolveStatus::Cycle) {
                return lhs;
            }
            return {};
        }
        case IR::Opcode::SelectU64:
        case IR::Opcode::Phi: {
            const size_t start_idx = node->GetOpcode() == IR::Opcode::SelectU64 ? 1 : 0;
            const size_t count = node->NumArgs();
            std::optional<StorageBufferAddr> agreed;
            for (size_t i = start_idx; i < count; ++i) {
                const ResolveResult br =
                    self(self, node->Arg(i), bias, cache, on_stack, steps, allow_u32_ptr);
                if (br.status == ResolveStatus::Failed) {
                    return {};
                }
                if (br.status == ResolveStatus::Resolved) {
                    if (!agreed) {
                        agreed = br.addr;
                    } else if (*agreed != br.addr) {
                        return {};
                    }
                }
            }
            return agreed ? ResolveResult{ResolveStatus::Resolved, *agreed}
                          : ResolveResult{ResolveStatus::Cycle};
        }
        case IR::Opcode::ConvertU64U32: {
            if (!allow_u32_ptr) {
                return {};
            }
            const auto found = cached_track(IR::U32{node->Arg(0)}, bias, cache);
            return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
        }
        case IR::Opcode::BitCastU64F64: {
            return self(self, node->Arg(0), bias, cache, on_stack, steps, allow_u32_ptr);
        }
        case IR::Opcode::ShiftLeftLogical64: {
            if (node->Arg(1).IsImmediate() && node->Arg(1).U64() == 0ULL) {
                return self(self, node->Arg(0), bias, cache, on_stack, steps, allow_u32_ptr);
            }
            return {};
        }
        case IR::Opcode::IMul32: {
            if (!allow_u32_ptr) {
                return {};
            }
            if (node->Arg(0).IsImmediate()) {
                const auto found = cached_track(IR::U32{node->Arg(1)}, bias, cache);
                return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
            }
            if (node->Arg(1).IsImmediate()) {
                const auto found = cached_track(IR::U32{node->Arg(0)}, bias, cache);
                return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
            }
            return {};
        }
        case IR::Opcode::PackUint2x32: {
            auto resolve_u32x2 = [&](auto& u_self, const IR::Value& vec_val) -> ResolveResult {
                if (vec_val.IsImmediate() || steps >= kMaxAddrTrackSteps) {
                    return {};
                }
                const IR::Inst* const vec_inst{vec_val.InstRecursive()};
                if (!vec_inst) {
                    return {};
                }
                if (std::ranges::find(on_stack, vec_inst) != on_stack.end()) {
                    return {.status = ResolveStatus::Cycle};
                }
                ++steps;
                on_stack.push_back(vec_inst);
                PopGuard vec_pop{on_stack};
                if (vec_inst->GetOpcode() == IR::Opcode::GetCbufU32x2) {
                    const auto found = cached_track(vec_val, bias, cache);
                    return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
                }
                if (vec_inst->GetOpcode() == IR::Opcode::CompositeConstructU32x2) {
                    const IR::Value hi_val{vec_inst->Arg(1)};
                    if (!allow_u32_ptr && hi_val.IsImmediate() && hi_val.U32() == 0U) {
                        return {};
                    }
                    const auto found = cached_track(IR::U32{vec_inst->Arg(0)}, bias, cache);
                    return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
                }
                if (vec_inst->GetOpcode() == IR::Opcode::UnpackUint2x32) {
                    return self(self, vec_inst->Arg(0), bias, cache, on_stack, steps,
                                allow_u32_ptr);
                }
                if (vec_inst->GetOpcode() == IR::Opcode::CompositeInsertU32x2) {
                    if (!vec_inst->Arg(2).IsImmediate()) {
                        return {};
                    }
                    const u32 elem{vec_inst->Arg(2).U32()};
                    if (elem == 0) {
                        const auto found = cached_track(IR::U32{vec_inst->Arg(1)}, bias, cache);
                        return found ? ResolveResult{ResolveStatus::Resolved, *found}
                                     : ResolveResult{};
                    }
                    if (elem == 1) {
                        return u_self(u_self, vec_inst->Arg(0));
                    }
                    return {};
                }
                if (vec_inst->GetOpcode() == IR::Opcode::Phi) {
                    std::optional<StorageBufferAddr> agreed;
                    for (size_t i = 0; i < vec_inst->NumArgs(); ++i) {
                        const ResolveResult br = u_self(u_self, vec_inst->Arg(i));
                        if (br.status == ResolveStatus::Failed) {
                            return {};
                        }
                        if (br.status == ResolveStatus::Resolved) {
                            if (!agreed) {
                                agreed = br.addr;
                            } else if (*agreed != br.addr) {
                                return {};
                            }
                        }
                    }
                    return agreed ? ResolveResult{ResolveStatus::Resolved, *agreed}
                                  : ResolveResult{ResolveStatus::Cycle};
                }
                return {};
            };
            return resolve_u32x2(resolve_u32x2, node->Arg(0));
        }
        case IR::Opcode::GetCbufU32x2: {
            const auto found = cached_track(value, bias, cache);
            return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
        }
        case IR::Opcode::CompositeInsertU32x2: {
            if (!node->Arg(2).IsImmediate()) {
                return {};
            }
            const u32 elem{node->Arg(2).U32()};
            if (elem == 0) {
                const auto found = cached_track(IR::U32{node->Arg(1)}, bias, cache);
                return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
            }
            if (elem == 1) {
                return self(self, node->Arg(0), bias, cache, on_stack, steps, allow_u32_ptr);
            }
            return {};
        }
        case IR::Opcode::UnpackUint2x32: {
            return self(self, node->Arg(0), bias, cache, on_stack, steps, allow_u32_ptr);
        }
        case IR::Opcode::CompositeConstructU32x2: {
            const auto found = cached_track(IR::U32{node->Arg(0)}, bias, cache);
            return found ? ResolveResult{ResolveStatus::Resolved, *found} : ResolveResult{};
        }
        default:
            return {};
        }
    };
    auto resolve_with_bias = [&](const Bias* bias, auto& cache) -> std::optional<StorageBufferAddr> {
        boost::container::small_vector<const IR::Inst*, 64> on_stack;
        size_t steps = 0;
        const ResolveResult res =
            resolve_expr(resolve_expr, addr, bias, cache, on_stack, steps, true);
        if (res.status == ResolveStatus::Resolved) {
            return res.addr;
        }
        return std::nullopt;
    };
    std::optional<StorageBufferAddr> storage_buffer{resolve_with_bias(&nvn_bias, info.biased_tracks)};
    if (!storage_buffer) {
        // If it fails, track without a bias
        storage_buffer = resolve_with_bias(nullptr, info.unbiased_tracks);
        if (!storage_buffer) {
            // If that also fails, use NVN fallbacks
            LOG_WARNING(Shader, "Storage buffer failed to track, using global memory fallbacks");
            return;
        }
        LOG_DEBUG(Shader, "Storage buffer tracked without bias, index {} offset {}",
                    storage_buffer->index, storage_buffer->offset);
    }
    // Collect storage buffer and the instruction
    if (IsGlobalMemoryWrite(inst)) {
        info.writes.insert(*storage_buffer);
    }
    info.set.insert(*storage_buffer);
    info.to_replace.push_back(StorageInst{
        .storage_buffer{*storage_buffer},
        .inst = &inst,
        .block = &block,
    });
}

/// Returns the offset in indices (not bytes) for an equivalent storage instruction
IR::U32 StorageOffset(IR::Block& block, IR::Inst& inst, StorageBufferAddr buffer, u32 alignment) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    IR::U32 offset;
    if (const std::optional<LowAddrInfo> low_addr{TrackLowAddress(&inst)};
        low_addr && low_addr->exact_low_addr) {
        offset = low_addr->value;
        if (low_addr->imm_offset != 0) {
            offset = ir.IAdd(offset, ir.Imm32(low_addr->imm_offset));
        }
    } else {
        offset = ir.UConvert(32, IR::U64{inst.Arg(0)});
    }
    // Subtract the least significant 32 bits from the guest offset. The result is the storage
    // buffer offset in bytes.
    IR::U32 low_cbuf{ir.GetCbuf(ir.Imm32(buffer.index), ir.Imm32(buffer.offset))};

    // Align the offset base to match the host alignment requirements
    low_cbuf = ir.BitwiseAnd(low_cbuf, ir.Imm32(~(alignment - 1U)));
    return ir.ISub(offset, low_cbuf);
}

/// Replace a global memory load instruction with its storage buffer equivalent
void ReplaceLoad(IR::Block& block, IR::Inst& inst, const IR::U32& storage_index,
                 const IR::U32& offset) {
    const IR::Opcode new_opcode{GlobalToStorage(inst.GetOpcode())};
    const auto it{IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::Value value{&*block.PrependNewInst(it, new_opcode, {storage_index, offset})};
    inst.ReplaceUsesWith(value);
}

/// Replace a global memory write instruction with its storage buffer equivalent
void ReplaceWrite(IR::Block& block, IR::Inst& inst, const IR::U32& storage_index,
                  const IR::U32& offset) {
    const IR::Opcode new_opcode{GlobalToStorage(inst.GetOpcode())};
    const auto it{IR::Block::InstructionList::s_iterator_to(inst)};
    block.PrependNewInst(it, new_opcode, {storage_index, offset, inst.Arg(1)});
    inst.Invalidate();
}

/// Replace an atomic operation on global memory instruction with its storage buffer equivalent
void ReplaceAtomic(IR::Block& block, IR::Inst& inst, const IR::U32& storage_index,
                   const IR::U32& offset) {
    const IR::Opcode new_opcode{GlobalToStorage(inst.GetOpcode())};
    const auto it{IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::Value value{
        &*block.PrependNewInst(it, new_opcode, {storage_index, offset, inst.Arg(1)})};
    inst.ReplaceUsesWith(value);
}

/// Replace a global memory instruction with its storage buffer equivalent
void Replace(IR::Block& block, IR::Inst& inst, const IR::U32& storage_index,
             const IR::U32& offset) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadGlobalS8:
    case IR::Opcode::LoadGlobalU8:
    case IR::Opcode::LoadGlobalS16:
    case IR::Opcode::LoadGlobalU16:
    case IR::Opcode::LoadGlobal32:
    case IR::Opcode::LoadGlobal64:
    case IR::Opcode::LoadGlobal128:
        return ReplaceLoad(block, inst, storage_index, offset);
    case IR::Opcode::WriteGlobalS8:
    case IR::Opcode::WriteGlobalU8:
    case IR::Opcode::WriteGlobalS16:
    case IR::Opcode::WriteGlobalU16:
    case IR::Opcode::WriteGlobal32:
    case IR::Opcode::WriteGlobal64:
    case IR::Opcode::WriteGlobal128:
        return ReplaceWrite(block, inst, storage_index, offset);
    case IR::Opcode::GlobalAtomicIAdd32:
    case IR::Opcode::GlobalAtomicSMin32:
    case IR::Opcode::GlobalAtomicUMin32:
    case IR::Opcode::GlobalAtomicSMax32:
    case IR::Opcode::GlobalAtomicUMax32:
    case IR::Opcode::GlobalAtomicInc32:
    case IR::Opcode::GlobalAtomicDec32:
    case IR::Opcode::GlobalAtomicAnd32:
    case IR::Opcode::GlobalAtomicOr32:
    case IR::Opcode::GlobalAtomicXor32:
    case IR::Opcode::GlobalAtomicExchange32:
    case IR::Opcode::GlobalAtomicIAdd64:
    case IR::Opcode::GlobalAtomicSMin64:
    case IR::Opcode::GlobalAtomicUMin64:
    case IR::Opcode::GlobalAtomicSMax64:
    case IR::Opcode::GlobalAtomicUMax64:
    case IR::Opcode::GlobalAtomicAnd64:
    case IR::Opcode::GlobalAtomicOr64:
    case IR::Opcode::GlobalAtomicXor64:
    case IR::Opcode::GlobalAtomicExchange64:
    case IR::Opcode::GlobalAtomicIAdd32x2:
    case IR::Opcode::GlobalAtomicSMin32x2:
    case IR::Opcode::GlobalAtomicUMin32x2:
    case IR::Opcode::GlobalAtomicSMax32x2:
    case IR::Opcode::GlobalAtomicUMax32x2:
    case IR::Opcode::GlobalAtomicAnd32x2:
    case IR::Opcode::GlobalAtomicOr32x2:
    case IR::Opcode::GlobalAtomicXor32x2:
    case IR::Opcode::GlobalAtomicExchange32x2:
    case IR::Opcode::GlobalAtomicAddF32:
    case IR::Opcode::GlobalAtomicAddF16x2:
    case IR::Opcode::GlobalAtomicAddF32x2:
    case IR::Opcode::GlobalAtomicMinF16x2:
    case IR::Opcode::GlobalAtomicMinF32x2:
    case IR::Opcode::GlobalAtomicMaxF16x2:
    case IR::Opcode::GlobalAtomicMaxF32x2:
        return ReplaceAtomic(block, inst, storage_index, offset);
    default:
        throw InvalidArgument("Invalid global memory opcode {}", inst.GetOpcode());
    }
}
} // Anonymous namespace

void GlobalMemoryToStorageBufferPass(IR::Program& program, const HostTranslateInfo& host_info) {
    StorageInfo info;
    for (IR::Block* const block : program.post_order_blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!IsGlobalMemory(inst)) {
                continue;
            }
            CollectStorageBuffers(*block, inst, info);
        }
    }
    for (const StorageBufferAddr& storage_buffer : info.set) {
        program.info.storage_buffers_descriptors.push_back({
            .cbuf_index = storage_buffer.index,
            .cbuf_offset = storage_buffer.offset,
            .count = 1,
            .is_written = info.writes.contains(storage_buffer),
        });
    }
    for (const StorageInst& storage_inst : info.to_replace) {
        const StorageBufferAddr storage_buffer{storage_inst.storage_buffer};
        const auto it{info.set.find(storage_inst.storage_buffer)};
        const IR::U32 index{IR::Value{static_cast<u32>(info.set.index_of(it))}};
        IR::Block* const block{storage_inst.block};
        IR::Inst* const inst{storage_inst.inst};
        const IR::U32 offset{
            StorageOffset(*block, *inst, storage_buffer, u32(host_info.min_ssbo_alignment))};
        Replace(*block, *inst, index, offset);
    }
}

void JoinStorageInfo(Info& base, Info& source) {
    auto& descriptors = base.storage_buffers_descriptors;
    for (auto& desc : source.storage_buffers_descriptors) {
        auto it{std::ranges::find_if(descriptors, [&desc](const auto& existing) {
            return desc.cbuf_index == existing.cbuf_index &&
                   desc.cbuf_offset == existing.cbuf_offset && desc.count == existing.count;
        })};
        if (it != descriptors.end()) {
            it->is_written |= desc.is_written;
            continue;
        }
        descriptors.push_back(desc);
    }
}

} // namespace Shader::Optimization
