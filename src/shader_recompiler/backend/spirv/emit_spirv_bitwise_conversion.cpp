// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

void EmitBitCastU16F16(EmitContext&) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitBitCastU32F32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[1], value);
}

void EmitBitCastU64F64(EmitContext&) {
    throw NotImplementedException("SPIR-V Instruction");
}

void EmitBitCastF16U16(EmitContext&) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitBitCastF32U32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F32[1], value);
}

void EmitBitCastF64U64(EmitContext&) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitPackUint2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U64, value);
}

Id EmitUnpackUint2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[2], value);
}

Id EmitPackFloat2x16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[1], value);
}

Id EmitUnpackFloat2x16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F16[2], value);
}

static Id SanitizeHalf2x16Vec2(EmitContext& ctx, Id value) {
    if (!Settings::values.fix_bloom_effects.GetValue()) {
        return value;
    }
    return ctx.OpCompositeConstruct(
        ctx.F32[2],
        SanitizeFiniteF32(ctx, ctx.OpCompositeExtract(ctx.F32[1], value, 0u), 65504.0f),
        SanitizeFiniteF32(ctx, ctx.OpCompositeExtract(ctx.F32[1], value, 1u), 65504.0f));
}

Id EmitPackHalf2x16(EmitContext& ctx, Id value) {
    return ctx.OpPackHalf2x16(ctx.U32[1], SanitizeHalf2x16Vec2(ctx, value));
}

Id EmitUnpackHalf2x16(EmitContext& ctx, Id value) {
    return SanitizeHalf2x16Vec2(ctx, ctx.OpUnpackHalf2x16(ctx.F32[2], value));
}

Id EmitPackDouble2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F64[1], value);
}

Id EmitUnpackDouble2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[2], value);
}

} // namespace Shader::Backend::SPIRV
