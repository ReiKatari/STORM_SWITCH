// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
namespace {
enum class Mode : u64 {
    Default,
    CovMask,
    Covered,
    Offset,
    CentroidOffset,
    MyIndex,
};
} // Anonymous namespace

void TranslatorVisitor::PIXLD(u64 insn) {
    union {
        u64 raw;
        BitField<31, 3, Mode> mode;
        BitField<0, 8, IR::Reg> dest_reg;
        BitField<8, 8, IR::Reg> addr_reg;
        BitField<20, 8, s64> addr_offset;
        BitField<45, 3, IR::Pred> dest_pred;
    } const pixld{insn};

    if (pixld.dest_pred != IR::Pred::PT) {
        LOG_WARNING(Shader, "PIXLD destination predicate {} ignored", static_cast<u64>(pixld.dest_pred.Value()));
    }
    switch (pixld.mode) {
    case Mode::CovMask:
    case Mode::Covered:
        X(pixld.dest_reg, ir.Imm32(1U));
        break;
    case Mode::Offset:
    case Mode::CentroidOffset:
        X(pixld.dest_reg, ir.Imm32(0U));
        break;
    case Mode::MyIndex:
        X(pixld.dest_reg, ir.SampleId());
        break;
    default:
        LOG_WARNING(Shader, "PIXLD unimplemented mode {}", static_cast<u64>(pixld.mode.Value()));
        X(pixld.dest_reg, ir.Imm32(0U));
        break;
    }
}

} // namespace Shader::Maxwell
