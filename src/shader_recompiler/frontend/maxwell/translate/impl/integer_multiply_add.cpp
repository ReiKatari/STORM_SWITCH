// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/bit_field.h"
#include "common/common_types.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
namespace {

void IMAD(TranslatorVisitor& v, u64 insn, const IR::U32& src_b, const IR::U32& src_c) {
    union {
        u64 raw;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_a;
        BitField<47, 1, u64> cc;
    } const imad{insn};

    const IR::U32 op_a{v.X(imad.src_a)};
    const IR::U32 product{v.ir.IMul(op_a, src_b)};
    const IR::U32 result{v.ir.IAdd(product, src_c)};

    v.X(imad.dest_reg, result);

    if (imad.cc != 0) {
        v.SetZFlag(v.ir.GetZeroFromOp(result));
        v.SetSFlag(v.ir.GetSignFromOp(result));
        v.SetCFlag(v.ir.GetCarryFromOp(result));
        v.SetOFlag(v.ir.GetOverflowFromOp(result));
    }
}

void IMUL(TranslatorVisitor& v, u64 insn, const IR::U32& src_b) {
    union {
        u64 raw;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_a;
        BitField<47, 1, u64> cc;
    } const imul{insn};

    const IR::U32 op_a{v.X(imul.src_a)};
    const IR::U32 product{v.ir.IMul(op_a, src_b)};

    v.X(imul.dest_reg, product);

    if (imul.cc != 0) {
        v.SetZFlag(v.ir.GetZeroFromOp(product));
        v.SetSFlag(v.ir.GetSignFromOp(product));
        v.SetCFlag(v.ir.GetCarryFromOp(product));
        v.SetOFlag(v.ir.GetOverflowFromOp(product));
    }
}

} // Anonymous namespace

void TranslatorVisitor::IMAD_reg(u64 insn) {
    IMAD(*this, insn, GetReg20(insn), GetReg39(insn));
}

void TranslatorVisitor::IMAD_rc(u64 insn) {
    IMAD(*this, insn, GetReg39(insn), GetCbuf(insn));
}

void TranslatorVisitor::IMAD_cr(u64 insn) {
    IMAD(*this, insn, GetCbuf(insn), GetReg39(insn));
}

void TranslatorVisitor::IMAD_imm(u64 insn) {
    IMAD(*this, insn, GetImm20(insn), GetReg39(insn));
}

void TranslatorVisitor::IMAD32I(u64 insn) {
    union {
        u64 raw;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_a;
        BitField<52, 1, u64> cc;
    } const imad32i{insn};

    const IR::U32 op_a{X(imad32i.src_a)};
    const IR::U32 op_b{GetImm32(insn)};
    const IR::U32 op_c{X(imad32i.dest_reg)};

    const IR::U32 product{ir.IMul(op_a, op_b)};
    const IR::U32 result{ir.IAdd(product, op_c)};

    X(imad32i.dest_reg, result);

    if (imad32i.cc != 0) {
        SetZFlag(ir.GetZeroFromOp(result));
        SetSFlag(ir.GetSignFromOp(result));
        SetCFlag(ir.GetCarryFromOp(result));
        SetOFlag(ir.GetOverflowFromOp(result));
    }
}

void TranslatorVisitor::IMADSP_reg(u64 insn) {
    IMAD(*this, insn, GetReg20(insn), GetReg39(insn));
}

void TranslatorVisitor::IMADSP_rc(u64 insn) {
    IMAD(*this, insn, GetReg39(insn), GetCbuf(insn));
}

void TranslatorVisitor::IMADSP_cr(u64 insn) {
    IMAD(*this, insn, GetCbuf(insn), GetReg39(insn));
}

void TranslatorVisitor::IMADSP_imm(u64 insn) {
    IMAD(*this, insn, GetImm20(insn), GetReg39(insn));
}

void TranslatorVisitor::IMUL_reg(u64 insn) {
    IMUL(*this, insn, GetReg20(insn));
}

void TranslatorVisitor::IMUL_cbuf(u64 insn) {
    IMUL(*this, insn, GetCbuf(insn));
}

void TranslatorVisitor::IMUL_imm(u64 insn) {
    IMUL(*this, insn, GetImm20(insn));
}

void TranslatorVisitor::IMUL32I(u64 insn) {
    union {
        u64 raw;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> src_a;
        BitField<52, 1, u64> cc;
    } const imul32i{insn};

    const IR::U32 op_a{X(imul32i.src_a)};
    const IR::U32 op_b{GetImm32(insn)};
    const IR::U32 product{ir.IMul(op_a, op_b)};

    X(imul32i.dest_reg, product);

    if (imul32i.cc != 0) {
        SetZFlag(ir.GetZeroFromOp(product));
        SetSFlag(ir.GetSignFromOp(product));
        SetCFlag(ir.GetCarryFromOp(product));
        SetOFlag(ir.GetOverflowFromOp(product));
    }
}

} // namespace Shader::Maxwell
