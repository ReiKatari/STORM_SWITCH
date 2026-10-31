// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <numeric>
#include "core/arm/nce/interpreter_visitor.h"

namespace Core {

namespace {
// Prefetch tuning parameters
[[maybe_unused]] constexpr size_t CACHE_LINE_SIZE = 64;
[[maybe_unused]] constexpr size_t PREFETCH_STRIDE = 128; // 2 cache lines ahead
[[maybe_unused]] constexpr size_t SIMD_PREFETCH_THRESHOLD = 32; // Bytes
} // namespace

template <u32 BitSize>
u64 SignExtendToLong(u64 value) {
    u64 mask = 1ULL << (BitSize - 1);
    value &= (1ULL << BitSize) - 1;
    return (value ^ mask) - mask;
}

static u64 SignExtendToLong(u64 value, u64 bitsize) {
    switch (bitsize) {
    case 8:
        return SignExtendToLong<8>(value);
    case 16:
        return SignExtendToLong<16>(value);
    case 32:
        return SignExtendToLong<32>(value);
    default:
        return value;
    }
}

template <u64 BitSize>
u32 SignExtendToWord(u32 value) {
    u32 mask = 1ULL << (BitSize - 1);
    value &= (1ULL << BitSize) - 1;
    return (value ^ mask) - mask;
}

static u32 SignExtendToWord(u32 value, u64 bitsize) {
    switch (bitsize) {
    case 8:
        return SignExtendToWord<8>(value);
    case 16:
        return SignExtendToWord<16>(value);
    default:
        return value;
    }
}

static u64 SignExtend(u64 value, u64 bitsize, u64 regsize) {
    if (regsize == 64) {
        return SignExtendToLong(value, bitsize);
    } else {
        return SignExtendToWord(static_cast<u32>(value), bitsize);
    }
}

static u128 VectorGetElement(u128 value, u64 bitsize) {
    switch (bitsize) {
    case 8:
        return {value[0] & ((1ULL << 8) - 1), 0};
    case 16:
        return {value[0] & ((1ULL << 16) - 1), 0};
    case 32:
        return {value[0] & ((1ULL << 32) - 1), 0};
    case 64:
        return {value[0], 0};
    default:
        return value;
    }
}

u64 InterpreterVisitor::ExtendReg(size_t bitsize, Reg reg, Imm<3> option, u8 shift) {
    ASSERT(shift <= 4);
    ASSERT(bitsize == 32 || bitsize == 64);
    u64 val = this->GetReg(reg);
    size_t len;
    u64 extended;
    bool signed_extend;

    switch (option.ZeroExtend()) {
    case 0b000: { // UXTB
        val &= ((1ULL << 8) - 1);
        len = 8;
        signed_extend = false;
        break;
    }
    case 0b001: { // UXTH
        val &= ((1ULL << 16) - 1);
        len = 16;
        signed_extend = false;
        break;
    }
    case 0b010: { // UXTW
        val &= ((1ULL << 32) - 1);
        len = 32;
        signed_extend = false;
        break;
    }
    case 0b011: { // UXTX
        len = 64;
        signed_extend = false;
        break;
    }
    case 0b100: { // SXTB
        val &= ((1ULL << 8) - 1);
        len = 8;
        signed_extend = true;
        break;
    }
    case 0b101: { // SXTH
        val &= ((1ULL << 16) - 1);
        len = 16;
        signed_extend = true;
        break;
    }
    case 0b110: { // SXTW
        val &= ((1ULL << 32) - 1);
        len = 32;
        signed_extend = true;
        break;
    }
    case 0b111: { // SXTX
        len = 64;
        signed_extend = true;
        break;
    }
    default:
        len = bitsize;
        signed_extend = false;
        break;
    }

    if (len < bitsize && signed_extend) {
        extended = SignExtend(val, len, bitsize);
    } else {
        extended = val;
    }

    return extended << shift;
}

u128 InterpreterVisitor::GetVec(Vec v) {
    return m_fpsimd_regs[static_cast<u32>(v)];
}

u64 InterpreterVisitor::GetReg(Reg r) {
    return m_regs[static_cast<u32>(r)];
}

u64 InterpreterVisitor::GetSp() {
    return m_sp;
}

u64 InterpreterVisitor::GetPc() {
    return m_pc;
}

void InterpreterVisitor::SetVec(Vec v, u128 value) {
    m_fpsimd_regs[static_cast<u32>(v)] = value;
}

void InterpreterVisitor::SetReg(Reg r, u64 value) {
    m_regs[static_cast<u32>(r)] = value;
}

void InterpreterVisitor::SetSp(u64 value) {
    m_sp = value;
}

bool InterpreterVisitor::Ordered(size_t size, bool L, bool o0, Reg Rn, Reg Rt) {
    const auto memop = L ? MemOp::Load : MemOp::Store;
    const size_t elsize = 8 << size;
    const size_t datasize = elsize;
    const size_t dbytes = datasize / 8;

    u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    switch (memop) {
    case MemOp::Store: {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        u64 value = this->GetReg(Rt);
        m_memory.WriteBlock(address, &value, dbytes);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        break;
    }
    case MemOp::Load: {
        u64 value = 0;
        m_memory.ReadBlock(address, &value, dbytes);
        this->SetReg(Rt, value);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        break;
    }
    default:
        return false;
    }
    return true;
}

bool InterpreterVisitor::STLLR(Imm<2> sz, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const bool L = 0;
    const bool o0 = 0;
    return this->Ordered(size, L, o0, Rn, Rt);
}

bool InterpreterVisitor::STLR(Imm<2> sz, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const bool L = 0;
    const bool o0 = 1;
    return this->Ordered(size, L, o0, Rn, Rt);
}

bool InterpreterVisitor::LDLAR(Imm<2> sz, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const bool L = 1;
    const bool o0 = 0;
    return this->Ordered(size, L, o0, Rn, Rt);
}

bool InterpreterVisitor::LDAR(Imm<2> sz, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const bool L = 1;
    const bool o0 = 1;
    return this->Ordered(size, L, o0, Rn, Rt);
}

bool InterpreterVisitor::LDR_lit_gen(bool opc_0, Imm<19> imm19, Reg Rt) {
    const size_t size = opc_0 == 0 ? 4 : 8;
    const s64 offset = Dynarmic::concatenate(imm19, Imm<2>{0}).SignExtend<s64>();
    const u64 address = this->GetPc() + offset;

    u64 data = 0;
    m_memory.ReadBlock(address, &data, size);

    this->SetReg(Rt, data);
    return true;
}

bool InterpreterVisitor::LDR_lit_fpsimd(Imm<2> opc, Imm<19> imm19, Vec Vt) {
    if (opc == 0b11) {
        // Unallocated encoding
        return false;
    }

    // Size in bytes
    const u64 size = 4 << opc.ZeroExtend();
    const u64 offset = imm19.SignExtend<u64>() << 2;
    const u64 address = this->GetPc() + offset;

    u128 data{};
    m_memory.ReadBlock(address, &data, size);
    this->SetVec(Vt, data);
    return true;
}

bool InterpreterVisitor::STP_LDP_gen(Imm<2> opc, bool not_postindex, bool wback, Imm<1> L,
                                     Imm<7> imm7, Reg Rt2, Reg Rn, Reg Rt) {
    if ((L == 0 && opc.Bit<0>() == 1) || opc == 0b11) {
        // Unallocated encoding
        return false;
    }

    const auto memop = L == 1 ? MemOp::Load : MemOp::Store;
    if (memop == MemOp::Load && wback && (Rt == Rn || Rt2 == Rn) && Rn != Reg::R31) {
        // Unpredictable instruction
        return false;
    }
    if (memop == MemOp::Store && wback && (Rt == Rn || Rt2 == Rn) && Rn != Reg::R31) {
        // Unpredictable instruction
        return false;
    }
    if (memop == MemOp::Load && Rt == Rt2) {
        // Unpredictable instruction
        return false;
    }

    u64 address;
    if (Rn == Reg::SP) {
        address = this->GetSp();
    } else {
        address = this->GetReg(Rn);
    }

    const bool postindex = !not_postindex;
    const bool signed_ = opc.Bit<0>() != 0;
    const size_t scale = 2 + opc.Bit<1>();
    const size_t datasize = 8 << scale;
    const u64 offset = imm7.SignExtend<u64>() << scale;

    if (!postindex) {
        address += offset;
    }

    const size_t dbytes = datasize / 8;
    switch (memop) {
    case MemOp::Store: {
        u64 data1 = this->GetReg(Rt);
        u64 data2 = this->GetReg(Rt2);
        m_memory.WriteBlock(address, &data1, dbytes);
        m_memory.WriteBlock(address + dbytes, &data2, dbytes);
        break;
    }
    case MemOp::Load: {
        u64 data1 = 0, data2 = 0;
        m_memory.ReadBlock(address, &data1, dbytes);
        m_memory.ReadBlock(address + dbytes, &data2, dbytes);
        if (signed_) {
            this->SetReg(Rt, SignExtend(data1, datasize, 64));
            this->SetReg(Rt2, SignExtend(data2, datasize, 64));
        } else {
            this->SetReg(Rt, data1);
            this->SetReg(Rt2, data2);
        }
        break;
    }
    default:
        return false;
    }

    if (wback) {
        if (postindex) {
            address += offset;
        }

        if (Rn == Reg::SP) {
            this->SetSp(address);
        } else {
            this->SetReg(Rn, address);
        }
    }

    return true;
}

bool InterpreterVisitor::STP_LDP_fpsimd(Imm<2> opc, bool not_postindex, bool wback, Imm<1> L,
                                        Imm<7> imm7, Vec Vt2, Reg Rn, Vec Vt) {
    if (opc == 0b11) {
        // Unallocated encoding
        return false;
    }

    const auto memop = L == 1 ? MemOp::Load : MemOp::Store;
    if (memop == MemOp::Load && Vt == Vt2) {
        // Unpredictable instruction
        return false;
    }

    u64 address;
    if (Rn == Reg::SP) {
        address = this->GetSp();
    } else {
        address = this->GetReg(Rn);
    }

    const bool postindex = !not_postindex;
    const size_t scale = 2 + opc.ZeroExtend<size_t>();
    const size_t datasize = 8 << scale;
    const u64 offset = imm7.SignExtend<u64>() << scale;
    const size_t dbytes = datasize / 8;

    if (!postindex) {
        address += offset;
    }

    switch (memop) {
    case MemOp::Store: {
        u128 data1 = VectorGetElement(this->GetVec(Vt), datasize);
        u128 data2 = VectorGetElement(this->GetVec(Vt2), datasize);
        m_memory.WriteBlock(address, &data1, dbytes);
        m_memory.WriteBlock(address + dbytes, &data2, dbytes);
        break;
    }
    case MemOp::Load: {
        u128 data1{}, data2{};
        m_memory.ReadBlock(address, &data1, dbytes);
        m_memory.ReadBlock(address + dbytes, &data2, dbytes);
        this->SetVec(Vt, data1);
        this->SetVec(Vt2, data2);
        break;
    }
    default:
        return false;
    }

    if (wback) {
        if (postindex) {
            address += offset;
        }

        if (Rn == Reg::SP) {
            this->SetSp(address);
        } else {
            this->SetReg(Rn, address);
        }
    }

    return true;
}

bool InterpreterVisitor::RegisterImmediate(bool wback, bool postindex, size_t scale, u64 offset,
                                           Imm<2> size, Imm<2> opc, Reg Rn, Reg Rt) {
    MemOp memop;
    bool signed_ = false;
    size_t regsize = 0;
    const size_t datasize = 8 << scale;

    if (opc.Bit<1>() == 0) {
        memop = opc.Bit<0>() ? MemOp::Load : MemOp::Store;
        regsize = size == 0b11 ? 64 : 32;
    } else if (size == 0b11) {
        memop = MemOp::Prefetch;
        ASSERT(!opc.Bit<0>());
    } else {
        memop = MemOp::Load;
        ASSERT(!(size == 0b10 && opc.Bit<0>() == 1));
        regsize = opc.Bit<0>() ? 32 : 64;
        signed_ = true;
    }

    u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    if (!postindex)
        address += offset;

    switch (memop) {
    case MemOp::Store: {
        u64 data = this->GetReg(Rt);
        m_memory.WriteBlock(address, &data, datasize / 8);
        break;
    }
    case MemOp::Load: {
        u64 data = 0;
        m_memory.ReadBlock(address, &data, datasize / 8);
        if (signed_) {
            this->SetReg(Rt, SignExtend(data, datasize, regsize));
        } else {
            this->SetReg(Rt, data);
        }
        break;
    }
    case MemOp::Prefetch:
        break;
    }

    if (wback) {
        if (postindex)
            address += offset;
        if (Rn == Reg::SP)
            this->SetSp(address);
        else
            this->SetReg(Rn, address);
    }
    return true;
}

bool InterpreterVisitor::STRx_LDRx_imm_1(Imm<2> size, Imm<2> opc, Imm<9> imm9, bool not_postindex,
                                         Reg Rn, Reg Rt) {
    const bool wback = true;
    const bool postindex = !not_postindex;
    const size_t scale = size.ZeroExtend<size_t>();
    const u64 offset = imm9.SignExtend<u64>();

    return this->RegisterImmediate(wback, postindex, scale, offset, size, opc, Rn, Rt);
}

bool InterpreterVisitor::STRx_LDRx_imm_2(Imm<2> size, Imm<2> opc, Imm<12> imm12, Reg Rn, Reg Rt) {
    const bool wback = false;
    const bool postindex = false;
    const size_t scale = size.ZeroExtend<size_t>();
    const u64 offset = imm12.ZeroExtend<u64>() << scale;

    return this->RegisterImmediate(wback, postindex, scale, offset, size, opc, Rn, Rt);
}

bool InterpreterVisitor::STURx_LDURx(Imm<2> size, Imm<2> opc, Imm<9> imm9, Reg Rn, Reg Rt) {
    const bool wback = false;
    const bool postindex = false;
    const size_t scale = size.ZeroExtend<size_t>();
    const u64 offset = imm9.SignExtend<u64>();

    return this->RegisterImmediate(wback, postindex, scale, offset, size, opc, Rn, Rt);
}

bool InterpreterVisitor::SIMDImmediate(bool wback, bool postindex, size_t scale, u64 offset,
                                       MemOp memop, Reg Rn, Vec Vt) {
    const size_t datasize = 8 << scale;
    u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    if (!postindex)
        address += offset;

    switch (memop) {
    case MemOp::Store: {
        u128 data = VectorGetElement(this->GetVec(Vt), datasize);
        m_memory.WriteBlock(address, &data, datasize / 8);
        break;
    }
    case MemOp::Load: {
        u128 data{};
        m_memory.ReadBlock(address, &data, datasize / 8);
        this->SetVec(Vt, data);
        break;
    }
    default:
        return false;
    }

    if (wback) {
        if (postindex)
            address += offset;
        if (Rn == Reg::SP)
            this->SetSp(address);
        else
            this->SetReg(Rn, address);
    }
    return true;
}

bool InterpreterVisitor::STR_imm_fpsimd_1(Imm<2> size, Imm<1> opc_1, Imm<9> imm9,
                                          bool not_postindex, Reg Rn, Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = true;
    const bool postindex = !not_postindex;
    const u64 offset = imm9.SignExtend<u64>();

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Store, Rn, Vt);
}

bool InterpreterVisitor::STR_imm_fpsimd_2(Imm<2> size, Imm<1> opc_1, Imm<12> imm12, Reg Rn,
                                          Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = false;
    const bool postindex = false;
    const u64 offset = imm12.ZeroExtend<u64>() << scale;

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Store, Rn, Vt);
}

bool InterpreterVisitor::LDR_imm_fpsimd_1(Imm<2> size, Imm<1> opc_1, Imm<9> imm9,
                                          bool not_postindex, Reg Rn, Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = true;
    const bool postindex = !not_postindex;
    const u64 offset = imm9.SignExtend<u64>();

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Load, Rn, Vt);
}

bool InterpreterVisitor::LDR_imm_fpsimd_2(Imm<2> size, Imm<1> opc_1, Imm<12> imm12, Reg Rn,
                                          Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = false;
    const bool postindex = false;
    const u64 offset = imm12.ZeroExtend<u64>() << scale;

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Load, Rn, Vt);
}

bool InterpreterVisitor::STUR_fpsimd(Imm<2> size, Imm<1> opc_1, Imm<9> imm9, Reg Rn, Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = false;
    const bool postindex = false;
    const u64 offset = imm9.SignExtend<u64>();

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Store, Rn, Vt);
}

bool InterpreterVisitor::LDUR_fpsimd(Imm<2> size, Imm<1> opc_1, Imm<9> imm9, Reg Rn, Vec Vt) {
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }

    const bool wback = false;
    const bool postindex = false;
    const u64 offset = imm9.SignExtend<u64>();

    return this->SIMDImmediate(wback, postindex, scale, offset, MemOp::Load, Rn, Vt);
}

bool InterpreterVisitor::RegisterOffset(size_t scale, u8 shift, Imm<2> size, Imm<1> opc_1,
                                        Imm<1> opc_0, Reg Rm, Imm<3> option, Reg Rn, Reg Rt) {
    MemOp memop;
    size_t regsize = 64;
    bool signed_ = false;

    if (opc_1 == 0) {
        memop = opc_0 == 1 ? MemOp::Load : MemOp::Store;
        regsize = size == 0b11 ? 64 : 32;
        signed_ = false;
    } else if (size == 0b11) {
        memop = MemOp::Prefetch;
        if (opc_0 == 1) {
            // Unallocated encoding
            return false;
        }
    } else {
        memop = MemOp::Load;
        if (size == 0b10 && opc_0 == 1) {
            // Unallocated encoding
            return false;
        }
        regsize = opc_0 == 1 ? 32 : 64;
        signed_ = true;
    }

    const size_t datasize = 8 << scale;

    // Operation
    const u64 offset = this->ExtendReg(64, Rm, option, shift);

    u64 address;
    if (Rn == Reg::SP) {
        address = this->GetSp();
    } else {
        address = this->GetReg(Rn);
    }
    address += offset;

    switch (memop) {
    case MemOp::Store: {
        u64 data = this->GetReg(Rt);
        m_memory.WriteBlock(address, &data, datasize / 8);
        break;
    }
    case MemOp::Load: {
        u64 data = 0;
        m_memory.ReadBlock(address, &data, datasize / 8);
        if (signed_) {
            this->SetReg(Rt, SignExtend(data, datasize, regsize));
        } else {
            this->SetReg(Rt, data);
        }
        break;
    }
    case MemOp::Prefetch:
        break;
    }

    return true;
}

bool InterpreterVisitor::STRx_reg(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn,
                                  Reg Rt) {
    const Imm<1> opc_0{0};
    const size_t scale = size.ZeroExtend<size_t>();
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        // Unallocated encoding
        return false;
    }
    return this->RegisterOffset(scale, shift, size, opc_1, opc_0, Rm, option, Rn, Rt);
}

bool InterpreterVisitor::LDRx_reg(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S, Reg Rn,
                                  Reg Rt) {
    const Imm<1> opc_0{1};
    const size_t scale = size.ZeroExtend<size_t>();
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        // Unallocated encoding
        return false;
    }
    return this->RegisterOffset(scale, shift, size, opc_1, opc_0, Rm, option, Rn, Rt);
}

bool InterpreterVisitor::SIMDOffset(size_t scale, u8 shift, Imm<1> opc_0, Reg Rm, Imm<3> option,
                                    Reg Rn, Vec Vt) {
    const auto memop = opc_0 == 1 ? MemOp::Load : MemOp::Store;
    const size_t datasize = 8 << scale;

    // Operation
    const u64 offset = this->ExtendReg(64, Rm, option, shift);

    u64 address;
    if (Rn == Reg::SP) {
        address = this->GetSp();
    } else {
        address = this->GetReg(Rn);
    }
    address += offset;

    switch (memop) {
    case MemOp::Store: {
        u128 data = VectorGetElement(this->GetVec(Vt), datasize);
        m_memory.WriteBlock(address, &data, datasize / 8);
        break;
    }
    case MemOp::Load: {
        u128 data{};
        m_memory.ReadBlock(address, &data, datasize / 8);
        this->SetVec(Vt, data);
        break;
    }
    default:
        return false;
    }

    return true;
}

bool InterpreterVisitor::STR_reg_fpsimd(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S,
                                        Reg Rn, Vec Vt) {
    const Imm<1> opc_0{0};
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        // Unallocated encoding
        return false;
    }
    return this->SIMDOffset(scale, shift, opc_0, Rm, option, Rn, Vt);
}

bool InterpreterVisitor::LDR_reg_fpsimd(Imm<2> size, Imm<1> opc_1, Reg Rm, Imm<3> option, bool S,
                                        Reg Rn, Vec Vt) {
    const Imm<1> opc_0{1};
    const size_t scale = Dynarmic::concatenate(opc_1, size).ZeroExtend<size_t>();
    if (scale > 4) {
        // Unallocated encoding
        return false;
    }
    const u8 shift = S ? static_cast<u8>(scale) : 0;
    if (!option.Bit<1>()) {
        // Unallocated encoding
        return false;
    }
    return this->SIMDOffset(scale, shift, opc_0, Rm, option, Rn, Vt);
}

bool InterpreterVisitor::STXR(Imm<2> sz, Reg Rs, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const size_t dbytes = 1ULL << size;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u64 val = this->GetReg(Rt);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    m_memory.WriteBlock(address, &val, dbytes);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    this->SetReg(Rs, 0);
    return true;
}

bool InterpreterVisitor::STXP(Imm<1> sz, Reg Rs, Reg Rt2, Reg Rn, Reg Rt) {
    const size_t dbytes = sz.ZeroExtend<size_t>() == 1 ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u64 val1 = this->GetReg(Rt);
    const u64 val2 = this->GetReg(Rt2);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    m_memory.WriteBlock(address, &val1, dbytes);
    m_memory.WriteBlock(address + dbytes, &val2, dbytes);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    this->SetReg(Rs, 0);
    return true;
}

bool InterpreterVisitor::STLXP(Imm<1> sz, Reg Rs, Reg Rt2, Reg Rn, Reg Rt) {
    return this->STXP(sz, Rs, Rt2, Rn, Rt);
}

bool InterpreterVisitor::LDXR(Imm<2> sz, Reg Rn, Reg Rt) {
    const size_t size = sz.ZeroExtend<size_t>();
    const size_t dbytes = 1ULL << size;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 val = 0;
    m_memory.ReadBlock(address, &val, dbytes);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    this->SetReg(Rt, val);
    return true;
}

bool InterpreterVisitor::LDXP(Imm<1> sz, Reg Rt2, Reg Rn, Reg Rt) {
    const size_t dbytes = sz.ZeroExtend<size_t>() == 1 ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 val1 = 0;
    u64 val2 = 0;
    m_memory.ReadBlock(address, &val1, dbytes);
    m_memory.ReadBlock(address + dbytes, &val2, dbytes);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    this->SetReg(Rt, val1);
    this->SetReg(Rt2, val2);
    return true;
}

bool InterpreterVisitor::LDAXP(Imm<1> sz, Reg Rt2, Reg Rn, Reg Rt) {
    return this->LDXP(sz, Rt2, Rn, Rt);
}

bool InterpreterVisitor::CAS(Imm<2> sz, bool L, Reg Rs, bool o0, Reg Rn, Reg Rt) {
    const size_t dbytes = sz.Bit<0>() ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 expected = this->GetReg(Rs);
    u64 desired = this->GetReg(Rt);
    if (dbytes == 4) {
        expected = static_cast<u32>(expected);
        desired = static_cast<u32>(desired);
    }
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
        if (dbytes == 4) {
            u32 exp32 = static_cast<u32>(expected);
            const u32 des32 = static_cast<u32>(desired);
#if defined(_MSC_VER)
            const u32 old = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(ptr), des32, exp32);
            this->SetReg(Rs, old);
#else
            __atomic_compare_exchange_n(reinterpret_cast<u32*>(ptr), &exp32, des32, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
            this->SetReg(Rs, exp32);
#endif
        } else {
#if defined(_MSC_VER)
            const u64 old = _InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(ptr), desired, expected);
            this->SetReg(Rs, old);
#else
            __atomic_compare_exchange_n(reinterpret_cast<u64*>(ptr), &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
            this->SetReg(Rs, expected);
#endif
        }
    } else {
        u64 cur = 0;
        m_memory.ReadBlock(address, &cur, dbytes);
        if (cur == expected) {
            m_memory.WriteBlock(address, &desired, dbytes);
        }
        this->SetReg(Rs, cur);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::CASB(bool L, Reg Rs, bool o0, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u8 expected = static_cast<u8>(this->GetReg(Rs));
    const u8 desired = static_cast<u8>(this->GetReg(Rt));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr) {
#if !defined(_MSC_VER)
        __atomic_compare_exchange_n(reinterpret_cast<u8*>(ptr), &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        this->SetReg(Rs, expected);
#else
        const u8 cur = *ptr;
        if (cur == expected) {
            *ptr = desired;
        }
        this->SetReg(Rs, cur);
#endif
    } else {
        const u8 cur = m_memory.Read8(address);
        if (cur == expected) {
            m_memory.Write8(address, desired);
        }
        this->SetReg(Rs, cur);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::CASH(bool L, Reg Rs, bool o0, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u16 expected = static_cast<u16>(this->GetReg(Rs));
    const u16 desired = static_cast<u16>(this->GetReg(Rt));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % 2 == 0)) {
#if !defined(_MSC_VER)
        __atomic_compare_exchange_n(reinterpret_cast<u16*>(ptr), &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        this->SetReg(Rs, expected);
#else
        const u16 old = _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(ptr), desired, expected);
        this->SetReg(Rs, old);
#endif
    } else {
        const u16 cur = m_memory.Read16(address);
        if (cur == expected) {
            m_memory.Write16(address, desired);
        }
        this->SetReg(Rs, cur);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::CASP(bool sz, bool L, Reg Rs, bool o0, Reg Rn, Reg Rt) {
    const size_t dbytes = sz ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const Reg Rs2 = static_cast<Reg>(static_cast<size_t>(Rs) + 1);
    const Reg Rt2 = static_cast<Reg>(static_cast<size_t>(Rt) + 1);
    u64 exp1 = this->GetReg(Rs);
    u64 exp2 = this->GetReg(Rs2);
    u64 des1 = this->GetReg(Rt);
    u64 des2 = this->GetReg(Rt2);
    if (dbytes == 4) {
        exp1 = static_cast<u32>(exp1);
        exp2 = static_cast<u32>(exp2);
        des1 = static_cast<u32>(des1);
        des2 = static_cast<u32>(des2);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    u64 cur1 = 0;
    u64 cur2 = 0;
    m_memory.ReadBlock(address, &cur1, dbytes);
    m_memory.ReadBlock(address + dbytes, &cur2, dbytes);
    if (cur1 == exp1 && cur2 == exp2) {
        m_memory.WriteBlock(address, &des1, dbytes);
        m_memory.WriteBlock(address + dbytes, &des2, dbytes);
    }
    this->SetReg(Rs, cur1);
    this->SetReg(Rs2, cur2);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::SWP(bool sz, bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const size_t dbytes = sz ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 val = this->GetReg(Rs);
    if (dbytes == 4) {
        val = static_cast<u32>(val);
    }
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
        if (dbytes == 4) {
#if defined(_MSC_VER)
            const u32 old = _InterlockedExchange(reinterpret_cast<volatile long*>(ptr), static_cast<u32>(val));
            this->SetReg(Rt, old);
#else
            const u32 old = __atomic_exchange_n(reinterpret_cast<u32*>(ptr), static_cast<u32>(val), __ATOMIC_SEQ_CST);
            this->SetReg(Rt, old);
#endif
        } else {
#if defined(_MSC_VER)
            const u64 old = _InterlockedExchange64(reinterpret_cast<volatile long long*>(ptr), val);
            this->SetReg(Rt, old);
#else
            const u64 old = __atomic_exchange_n(reinterpret_cast<u64*>(ptr), val, __ATOMIC_SEQ_CST);
            this->SetReg(Rt, old);
#endif
        }
    } else {
        u64 old = 0;
        m_memory.ReadBlock(address, &old, dbytes);
        m_memory.WriteBlock(address, &val, dbytes);
        this->SetReg(Rt, old);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::SWPB(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u8 val = static_cast<u8>(this->GetReg(Rs));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr) {
#if !defined(_MSC_VER)
        const u8 old = __atomic_exchange_n(reinterpret_cast<u8*>(ptr), val, __ATOMIC_SEQ_CST);
        this->SetReg(Rt, old);
#else
        const u8 old = *ptr;
        *ptr = val;
        this->SetReg(Rt, old);
#endif
    } else {
        const u8 old = m_memory.Read8(address);
        m_memory.Write8(address, val);
        this->SetReg(Rt, old);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::SWPH(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u16 val = static_cast<u16>(this->GetReg(Rs));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % 2 == 0)) {
#if !defined(_MSC_VER)
        const u16 old = __atomic_exchange_n(reinterpret_cast<u16*>(ptr), val, __ATOMIC_SEQ_CST);
        this->SetReg(Rt, old);
#else
        const u16 old = _InterlockedExchange16(reinterpret_cast<volatile short*>(ptr), val);
        this->SetReg(Rt, old);
#endif
    } else {
        const u16 old = m_memory.Read16(address);
        m_memory.Write16(address, val);
        this->SetReg(Rt, old);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDADD(bool sz, bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const size_t dbytes = sz ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 add_val = this->GetReg(Rs);
    if (dbytes == 4) {
        add_val = static_cast<u32>(add_val);
    }
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
        if (dbytes == 4) {
#if defined(_MSC_VER)
            const u32 old = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(ptr), static_cast<u32>(add_val));
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u32 old = __atomic_fetch_add(reinterpret_cast<u32*>(ptr), static_cast<u32>(add_val), __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        } else {
#if defined(_MSC_VER)
            const u64 old = _InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(ptr), add_val);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u64 old = __atomic_fetch_add(reinterpret_cast<u64*>(ptr), add_val, __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        }
    } else {
        u64 old = 0;
        m_memory.ReadBlock(address, &old, dbytes);
        const u64 res = old + add_val;
        m_memory.WriteBlock(address, &res, dbytes);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDADDB(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u8 add_val = static_cast<u8>(this->GetReg(Rs));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr) {
#if !defined(_MSC_VER)
        const u8 old = __atomic_fetch_add(reinterpret_cast<u8*>(ptr), add_val, __ATOMIC_SEQ_CST);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#else
        const u8 old = *ptr;
        *ptr = old + add_val;
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#endif
    } else {
        const u8 old = m_memory.Read8(address);
        m_memory.Write8(address, static_cast<u8>(old + add_val));
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDADDH(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u16 add_val = static_cast<u16>(this->GetReg(Rs));
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % 2 == 0)) {
#if !defined(_MSC_VER)
        const u16 old = __atomic_fetch_add(reinterpret_cast<u16*>(ptr), add_val, __ATOMIC_SEQ_CST);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#else
        const u16 old = _InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(ptr), add_val);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#endif
    } else {
        const u16 old = m_memory.Read16(address);
        m_memory.Write16(address, static_cast<u16>(old + add_val));
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDCLR(bool sz, bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const size_t dbytes = sz ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 val = this->GetReg(Rs);
    if (dbytes == 4) {
        val = static_cast<u32>(val);
    }
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
        if (dbytes == 4) {
#if !defined(_MSC_VER)
            const u32 old = __atomic_fetch_and(reinterpret_cast<u32*>(ptr), ~static_cast<u32>(val), __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u32 old = _InterlockedAnd(reinterpret_cast<volatile long*>(ptr), ~static_cast<u32>(val));
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        } else {
#if !defined(_MSC_VER)
            const u64 old = __atomic_fetch_and(reinterpret_cast<u64*>(ptr), ~val, __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u64 old = _InterlockedAnd64(reinterpret_cast<volatile long long*>(ptr), ~val);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        }
    } else {
        u64 old = 0;
        m_memory.ReadBlock(address, &old, dbytes);
        const u64 res = old & ~val;
        m_memory.WriteBlock(address, &res, dbytes);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDCLRB(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u8 val = static_cast<u8>(this->GetReg(Rs));
    const u8 old = m_memory.Read8(address);
    m_memory.Write8(address, static_cast<u8>(old & ~val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

bool InterpreterVisitor::LDCLRH(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u16 val = static_cast<u16>(this->GetReg(Rs));
    const u16 old = m_memory.Read16(address);
    m_memory.Write16(address, static_cast<u16>(old & ~val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

bool InterpreterVisitor::LDSET(bool sz, bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const size_t dbytes = sz ? 8 : 4;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    u64 val = this->GetReg(Rs);
    if (dbytes == 4) {
        val = static_cast<u32>(val);
    }
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
        if (dbytes == 4) {
#if !defined(_MSC_VER)
            const u32 old = __atomic_fetch_or(reinterpret_cast<u32*>(ptr), static_cast<u32>(val), __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u32 old = _InterlockedOr(reinterpret_cast<volatile long*>(ptr), static_cast<u32>(val));
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        } else {
#if !defined(_MSC_VER)
            const u64 old = __atomic_fetch_or(reinterpret_cast<u64*>(ptr), val, __ATOMIC_SEQ_CST);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#else
            const u64 old = _InterlockedOr64(reinterpret_cast<volatile long long*>(ptr), val);
            if (Rt != Reg::ZR) {
                this->SetReg(Rt, old);
            }
#endif
        }
    } else {
        u64 old = 0;
        m_memory.ReadBlock(address, &old, dbytes);
        const u64 res = old | val;
        m_memory.WriteBlock(address, &res, dbytes);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDSETB(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u8 val = static_cast<u8>(this->GetReg(Rs));
    const u8 old = m_memory.Read8(address);
    m_memory.Write8(address, static_cast<u8>(old | val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

bool InterpreterVisitor::LDSETH(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u16 val = static_cast<u16>(this->GetReg(Rs));
    const u16 old = m_memory.Read16(address);
    m_memory.Write16(address, static_cast<u16>(old | val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

bool InterpreterVisitor::LDEOR(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const size_t dbytes = 8;
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u64 val = this->GetReg(Rs);
    u8* ptr = m_memory.GetPointer(address);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (ptr && (reinterpret_cast<uintptr_t>(ptr) % dbytes == 0)) {
#if !defined(_MSC_VER)
        const u64 old = __atomic_fetch_xor(reinterpret_cast<u64*>(ptr), val, __ATOMIC_SEQ_CST);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#else
        const u64 old = _InterlockedXor64(reinterpret_cast<volatile long long*>(ptr), val);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
#endif
    } else {
        u64 old = 0;
        m_memory.ReadBlock(address, &old, dbytes);
        const u64 res = old ^ val;
        m_memory.WriteBlock(address, &res, dbytes);
        if (Rt != Reg::ZR) {
            this->SetReg(Rt, old);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

bool InterpreterVisitor::LDEORB(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u8 val = static_cast<u8>(this->GetReg(Rs));
    const u8 old = m_memory.Read8(address);
    m_memory.Write8(address, static_cast<u8>(old ^ val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

bool InterpreterVisitor::LDEORH(bool A, bool R, Reg Rs, Reg Rn, Reg Rt) {
    const u64 address = (Rn == Reg::SP) ? this->GetSp() : this->GetReg(Rn);
    const u16 val = static_cast<u16>(this->GetReg(Rs));
    const u16 old = m_memory.Read16(address);
    m_memory.Write16(address, static_cast<u16>(old ^ val));
    if (Rt != Reg::ZR) {
        this->SetReg(Rt, old);
    }
    return true;
}

std::optional<u64> MatchAndExecuteOneInstruction(Core::Memory::Memory& memory, mcontext_t* context, fpsimd_context* fpsimd_context) {
    std::span<u64, 31> regs(reinterpret_cast<u64*>(context->regs), 31);
    std::span<u128, 32> vregs(reinterpret_cast<u128*>(fpsimd_context->vregs), 32);
    u64& sp = *reinterpret_cast<u64*>(&context->sp);
    const u64& pc = *reinterpret_cast<u64*>(&context->pc);

    InterpreterVisitor visitor(memory, regs, vregs, sp, pc);
    u32 instruction = memory.Read32(pc);
    bool was_executed = false;

    auto decoder = Dynarmic::A64::Decode<VisitorBase, bool>(visitor, instruction);
    if (decoder) {
        was_executed = *decoder;
    } else {
        was_executed = false;
    }
    return was_executed ? std::optional<u64>(pc + 4) : std::nullopt;
}

} // namespace Core
