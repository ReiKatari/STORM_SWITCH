// SPDX-FileCopyrightText: 2026 STORM Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "common/settings.h"
#include "core/hle/service/game_fix_database.h"
#include "shader_recompiler/frontend/maxwell/translate_program.h"
#include "video_core/buffer_cache/buffer_cache_base.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/storm_mvcc_radix.h"
#include "video_core/texture_cache/util.h"
#include "video_core/textures/decoders.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"
#include "video_core/renderer_vulkan/storm_hft_staging_ring.h"
#include "video_core/renderer_vulkan/storm_direct_uma_arena.h"

namespace VideoCommon {

TEST_CASE("StormMvccRadixDirectory: 40-bit 1-to-N Aliasing and Compaction", "[video_core]") {
    constexpr u64 kBaseAddr = 0x180000000ULL; // > 4 GiB (40-bit address)

    SECTION("1-to-N page aliasing registration and count") {
        StormMvccRadixDirectory dir;
        dir.RegisterPage(kBaseAddr, 100);
        dir.RegisterPage(kBaseAddr, 101);
        dir.RegisterPage(kBaseAddr, 102);

        REQUIRE(dir.CountOnPage(kBaseAddr) == 3);

        std::vector<u32> collected;
        dir.ForEachOnPage(kBaseAddr, [&](u32 id) {
            collected.push_back(id);
        });

        REQUIRE(collected.size() == 3);
        REQUIRE(std::find(collected.begin(), collected.end(), 100U) != collected.end());
        REQUIRE(std::find(collected.begin(), collected.end(), 101U) != collected.end());
        REQUIRE(std::find(collected.begin(), collected.end(), 102U) != collected.end());
    }

    SECTION("Swap-with-tail compaction leaving zero tombstones") {
        StormMvccRadixDirectory dir;
        dir.RegisterPage(kBaseAddr, 201);
        dir.RegisterPage(kBaseAddr, 202);
        dir.RegisterPage(kBaseAddr, 203);

        // Unregister middle element
        dir.UnregisterPage(kBaseAddr, 202);
        REQUIRE(dir.CountOnPage(kBaseAddr) == 2);

        std::vector<u32> after_unreg;
        dir.ForEachOnPage(kBaseAddr, [&](u32 id) {
            after_unreg.push_back(id);
        });
        REQUIRE(after_unreg.size() == 2);
        REQUIRE(std::find(after_unreg.begin(), after_unreg.end(), 202U) == after_unreg.end());

        // Unregister remaining elements
        dir.UnregisterPage(kBaseAddr, 201);
        dir.UnregisterPage(kBaseAddr, 203);
        REQUIRE(dir.CountOnPage(kBaseAddr) == 0);
    }
}

TEST_CASE("SelectCausalFramebuffer: Ping-Pong and Static Splash Resolution", "[video_core]") {
    SECTION("Ping-pong dynamic resolution with 3-way aliasing") {
        std::array<StormMvccCandidate, 3> cands{{
            {10, 0x20000000ULL, 1920, 1080, 1, 0, 100, true, false},
            {20, 0x20000000ULL, 1920, 1080, 1, 0, 105, true, false}, // Newest active RT
            {30, 0x20000800ULL, 1920, 1080, 1, 0, 110, true, false}, // Offset mismatch
        }};

        const u32 chosen = SelectCausalFramebuffer(
            cands.data(), cands.size(), 0x20000000ULL, 1920, 1080, 1);
        REQUIRE(chosen == 20);
    }

    SECTION("Absolum static splash causality (gpu_tick = 0, cpu_modified = true)") {
        StormMvccCandidate splash{
            .image_id = 77,
            .cpu_addr = 0x30000000ULL,
            .width = 1280,
            .height = 720,
            .format_class = 1,
            .cpu_tick = 50,
            .gpu_tick = 0,
            .gpu_modified = false,
            .cpu_modified = true,
        };

        const u32 chosen = SelectCausalFramebuffer(
            &splash, 1, 0x30000000ULL, 1280, 720, 1);
        REQUIRE(chosen == 77);
    }
}

} // namespace VideoCommon

namespace Vulkan {

TEST_CASE("MaxwellDepthClampClip & DynamicState::Refresh3 polarity and guards", "[video_core]") {
    Maxwell regs{};
    regs.viewport_clip_control.geometry_clip.Assign(
        Maxwell::ViewportClipControl::GeometryClip::FrustumXYZ);

    // 0 = clamp active, 1 = clip/no-clamp (Maxwell Z-min/max polarity)
    regs.viewport_clip_control.pixel_min_z.Assign(1);
    regs.viewport_clip_control.pixel_max_z.Assign(1);
    const auto all_clip = MaxwellDepthClampClip(regs, true);
    REQUIRE_FALSE(all_clip.clamp);
    REQUIRE(all_clip.clip);

    regs.viewport_clip_control.pixel_min_z.Assign(0);
    regs.viewport_clip_control.pixel_max_z.Assign(1);
    const auto min_clamp = MaxwellDepthClampClip(regs, true);
    REQUIRE(min_clamp.clamp);
    REQUIRE(min_clamp.clip);

    // DynamicState::Refresh3 must NOT overwrite static depth_clamp_disabled when dynamic EDS3 is active
    FixedPipelineState::DynamicState dyn{};
    dyn.raw1 = 0;
    DynamicFeatures dyn_features{};
    dyn_features.has_dynamic_state3_depth_clamp_enable = true;
    dyn_features.has_depth_clip_enable = true;
    dyn.Refresh3(regs, dyn_features);
    REQUIRE(dyn.depth_clamp_disabled.Value() == 0);
    REQUIRE(dyn.depth_clip_disabled.Value() == 0);

    // Without EDS3 depth_clamp_enable, static bit tracks !clamp
    regs.viewport_clip_control.pixel_min_z.Assign(1);
    regs.viewport_clip_control.pixel_max_z.Assign(1);
    FixedPipelineState::DynamicState static_dyn{};
    static_dyn.raw1 = 0;
    DynamicFeatures static_features{};
    static_features.has_dynamic_state3_depth_clamp_enable = false;
    static_features.has_depth_clip_enable = false;
    static_dyn.Refresh3(regs, static_features);
    REQUIRE(static_dyn.depth_clamp_disabled.Value() == 1);
    REQUIRE(static_dyn.depth_clip_disabled.Value() == 0);
}

TEST_CASE("StormMpscTlabStagingRing: Multi-thread Concurrency and Reclamation", "[video_core]") {
    StormMpscTlabStagingRing ring(16ULL * 1024ULL * 1024ULL); // 16 MiB ring

    constexpr int kNumProducers = 4;
    constexpr size_t kAllocsPerThread = 500;
    std::atomic<bool> start_signal{false};
    std::atomic<size_t> successful_allocs{0};

    std::vector<std::thread> producers;
    for (int t = 0; t < kNumProducers; ++t) {
        producers.emplace_back([&, t]() {
            while (!start_signal.load(std::memory_order_acquire)) {}
            u8 data[128];
            std::memset(data, 0x10 + t, sizeof(data));
            for (size_t i = 0; i < kAllocsPerThread; ++i) {
                auto alloc = ring.AllocateAndCommit(data, sizeof(data), 1);
                if (alloc.ptr != nullptr && alloc.ptr[0] == static_cast<u8>(0x10 + t)) {
                    successful_allocs.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    start_signal.store(true, std::memory_order_release);
    for (auto& p : producers) {
        p.join();
    }

    REQUIRE(successful_allocs.load() == kNumProducers * kAllocsPerThread);

    // Reclaim epoch
    ring.ReclaimCompletedEpoch(1);
}

TEST_CASE("StormHierarchicalDirtyBitset: tzcnt Accuracy and Clearing", "[video_core]") {
    constexpr size_t kPages = 4096;
    StormHierarchicalDirtyBitset bitset(kPages);

    bitset.MarkDirty(15, 0x1ULL);
    bitset.MarkDirty(128, 0x2ULL);
    bitset.MarkDirty(2047, 0x4ULL);

    std::vector<size_t> scanned_pages;
    bitset.ForEachDirtyAndClear([&](size_t page_idx, u64 submask) {
        scanned_pages.push_back(page_idx);
    });

    REQUIRE(scanned_pages.size() == 3);
    REQUIRE(scanned_pages[0] == 15);
    REQUIRE(scanned_pages[1] == 128);
    REQUIRE(scanned_pages[2] == 2047);

    // Verify bitset is now completely clean
    size_t count_after = 0;
    bitset.ForEachDirtyAndClear([&](size_t, u64) {
        count_after++;
    });
    REQUIRE(count_after == 0);
}

TEST_CASE("StormDirectUmaArena: Strict Mutation Gate Acceptance and Rejection", "[video_core]") {
    constexpr size_t kArenaSize = 65536 * 2;
#ifdef _WIN32
    u8* mock_arena = static_cast<u8*>(_aligned_malloc(kArenaSize, 65536));
#else
    void* ptr = nullptr;
    posix_memalign(&ptr, 65536, kArenaSize);
    u8* mock_arena = static_cast<u8*>(ptr);
#endif
    std::memset(mock_arena, 0x42, kArenaSize);

    SECTION("Valid dynamic uniform buffer accepted") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 2,
            .size = 2048,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == true);

        auto binding = arena.BindStreamingOrFallback(0, 2048, info);
        REQUIRE(binding.ptr != nullptr);
        REQUIRE(binding.zero_copy_used == true);
    }

    SECTION("Static mesh rejected (0 cpu writes)") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 0,
            .size = 4096,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

    SECTION("Oversized buffer rejected (> 256 KiB)") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 4,
            .size = 512 * 1024,
            .has_gpu_writes = false,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

    SECTION("GPU-written SSBO rejected") {
        StormDirectUmaArena arena(mock_arena, kArenaSize, 65536, true);
        StormBufferMutationInfo info{
            .cpu_writes_per_frame = 1,
            .size = 1024,
            .has_gpu_writes = true,
        };
        REQUIRE(arena.ShouldBypassCache(info) == false);
    }

#ifdef _WIN32
    _aligned_free(mock_arena);
#else
    free(mock_arena);
#endif
}

} // namespace Vulkan

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/program.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/ir_opt/passes.h"
#include "shader_recompiler/object_pool.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

namespace {

bool HasOpcode(const IR::Program& program, IR::Opcode opcode) {
    for (const IR::Block* const block : program.blocks) {
        for (const IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == opcode) {
                return true;
            }
        }
    }
    return false;
}

IR::U64 MakeCbufPtr(IR::IREmitter& ir, u32 binding, u32 offset) {
    const IR::U32 lo{ir.GetCbuf(ir.Imm32(binding), ir.Imm32(offset))};
    const IR::U32 hi{ir.GetCbuf(ir.Imm32(binding), ir.Imm32(offset + 4))};
    return ir.PackUint2x32(ir.CompositeConstruct(lo, hi));
}

} // namespace

TEST_CASE("EmitSPIRV: SanitizeFragColor uses integer exponent bounds with fix_bloom_effects",
          "[shader_recompiler]") {
    ObjectPool<IR::Inst> inst_pool;
    ObjectPool<IR::Block> block_pool;

    IR::Block* block = block_pool.Create(inst_pool);
    IR::IREmitter ir{*block};
    ir.Prologue();
    const IR::U32 raw = ir.GetCbuf(ir.Imm32(0U), ir.Imm32(0U));
    ir.SetFragColor(0, 0, ir.BitCast<IR::F32>(raw));
    ir.Epilogue();

    IR::AbstractSyntaxNode block_node{};
    block_node.type = IR::AbstractSyntaxNode::Type::Block;
    block_node.data.block = block;
    IR::AbstractSyntaxNode ret_node{};
    ret_node.type = IR::AbstractSyntaxNode::Type::Return;

    IR::Program program{};
    program.stage = Shader::Stage::Fragment;
    program.blocks = {block};
    program.post_order_blocks = {block};
    program.syntax_list = {block_node, ret_node};
    program.info.stores_frag_color[0] = true;
    program.info.constant_buffer_mask |= 1U;
    program.info.constant_buffer_used_sizes[0] = 0x100;
    program.info.constant_buffer_descriptors.push_back({.index = 0, .count = 1});
    program.info.used_constant_buffer_types = IR::Type::U32;

    Shader::Profile profile{};
    profile.supported_spirv = 0x00010500;
    Shader::RuntimeInfo runtime_info{};
    Shader::Backend::Bindings bindings{};

    const bool prev_bloom = Settings::values.fix_bloom_effects.GetValue();
    Settings::values.fix_bloom_effects.SetValue(true);
    const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
    Settings::values.fix_bloom_effects.SetValue(prev_bloom);

    REQUIRE(spirv.size() > 5);
    REQUIRE(spirv[0] == 0x07230203u);

    constexpr u16 kOpBitcast = 124u;
    constexpr u16 kOpFOrdLessThan = 182u;
    bool has_op_bitcast = false;
    bool has_ford_less_than = false;
    bool has_abs_mask = false;
    bool has_inf_bits = false;
    for (size_t i = 5; i < spirv.size();) {
        const u32 word_count = spirv[i] >> 16;
        const u16 opcode = static_cast<u16>(spirv[i] & 0xFFFFu);
        REQUIRE(word_count >= 1);
        REQUIRE(i + word_count <= spirv.size());
        if (opcode == kOpBitcast) {
            has_op_bitcast = true;
        }
        if (opcode == kOpFOrdLessThan) {
            has_ford_less_than = true;
        }
        for (size_t k = 1; k < word_count; ++k) {
            if (spirv[i + k] == 0x7FFFFFFFu) {
                has_abs_mask = true;
            }
            if (spirv[i + k] == 0x7F800000u) {
                has_inf_bits = true;
            }
        }
        i += word_count;
    }

    REQUIRE(has_op_bitcast);
    REQUIRE(has_abs_mask);
    REQUIRE(has_inf_bits);
    REQUIRE_FALSE(has_ford_less_than);
}

TEST_CASE("GlobalMemoryToStorageBufferPass: Complex SSBO Pointer Tracking", "[shader_recompiler]") {
    ObjectPool<IR::Inst> inst_pool;
    ObjectPool<IR::Block> block_pool;

    HostTranslateInfo host_info{};
    host_info.min_ssbo_alignment = 16;

    SECTION("IAdd64(imm, base) reversed operand order tracks SSBO") {
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        const IR::U64 base = MakeCbufPtr(ir, 2, 0x40);
        const IR::U64 ptr{ir.IAdd(ir.Imm64(u64{0x20}), base)};
        const IR::U32 loaded = ir.LoadGlobal32(ptr);
        ir.Reference(loaded);

        IR::Program program{};
        program.blocks = {block};
        program.post_order_blocks = {block};

        GlobalMemoryToStorageBufferPass(program, host_info);

        REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
        REQUIRE(program.info.storage_buffers_descriptors[0].is_written == false);
        REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
        REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
    }

    SECTION("LDC.64 GetCbufU32x2, CompositeInsertU32x2, and UnpackUint2x32 track SSBO") {
        // 1) Direct GetCbufU32x2 -> PackUint2x32 -> LoadGlobal32
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::Value vec{ir.GetCbuf(ir.Imm32(2U), ir.Imm32(0x40U), 64, false)};
            const IR::U64 ptr{ir.PackUint2x32(vec)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
        // 2) CompositeInsertU32x2 (elem 1 high-word update) + UnpackUint2x32 round-trip
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::Value base_vec{ir.GetCbuf(ir.Imm32(2U), ir.Imm32(0x40U), 64, false)};
            const IR::Value high_upd{ir.CompositeInsert(base_vec, ir.Imm32(0U), 1)};
            const IR::U64 packed{ir.PackUint2x32(high_upd)};
            const IR::Value unpacked{ir.UnpackUint2x32(packed)};
            const IR::U64 ptr{ir.PackUint2x32(unpacked)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
        // 3) CompositeInsertU32x2 elem 0 replaces low word with cbuf descriptor
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::Value dummy_vec{ir.CompositeConstruct(ir.Imm32(0U), ir.Imm32(0U))};
            const IR::U32 lo{ir.GetCbuf(ir.Imm32(2U), ir.Imm32(0x60U))};
            const IR::Value ins{ir.CompositeInsert(dummy_vec, lo, 0)};
            const IR::U64 ptr{ir.PackUint2x32(ins)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x60);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
        // 4) Maxwell LDC.64 pattern: GetCbufU32x2 -> CompositeExtract(0/1) -> IAdd(lo, 0x10) ->
        //    CompositeConstruct -> PackUint2x32 -> LoadGlobal32
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::Value vec{ir.GetCbuf(ir.Imm32(2U), ir.Imm32(0x80U), 64, false)};
            const IR::U32 lo{IR::U32{ir.CompositeExtract(vec, 0)}};
            const IR::U32 hi{IR::U32{ir.CompositeExtract(vec, 1)}};
            const IR::U32 lo_off{ir.IAdd(lo, ir.Imm32(0x10U))};
            const IR::U64 ptr{ir.PackUint2x32(ir.CompositeConstruct(lo_off, hi))};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x80);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
    }

    SECTION("SPIR-V compacted NVN SSBO, SelectF16 lowering, and PackHalf2x16 sanitize") {
        auto emit_case = [&](bool with_ssbo, bool sanitize_bloom) {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            ir.Prologue();
            const IR::U32 cb0 = ir.GetCbuf(ir.Imm32(0U), ir.Imm32(0U));
            const IR::F32 dyn_x = ir.BitCast<IR::F32>(cb0);
            const IR::U1 cond = ir.IEqual(cb0, ir.Imm32(1U));
            const IR::Value f16_a = ir.FPConvert(16, dyn_x);
            const IR::Value f16_b = ir.FPConvert(16, ir.Imm32(2.0f));
            const IR::Value sel16 = ir.Select(cond, f16_a, f16_b);
            const IR::F32 sel32 = ir.FPConvert(32, IR::F16{sel16});
            const IR::Value half_vec = ir.CompositeConstruct(dyn_x, sel32);
            const IR::U32 packed_u32 = ir.PackHalf2x16(half_vec);
            const IR::U64 gptr =
                ir.PackUint2x32(ir.CompositeConstruct(ir.Imm32(0x1000U), ir.Imm32(0U)));
            const IR::U32 gload = ir.LoadGlobal32(gptr);
            ir.SetFragColor(0, 0, ir.BitCast<IR::F32>(packed_u32));
            ir.SetFragColor(0, 1, ir.BitCast<IR::F32>(gload));
            ir.Epilogue();

            IR::AbstractSyntaxNode block_node{};
            block_node.type = IR::AbstractSyntaxNode::Type::Block;
            block_node.data.block = block;
            IR::AbstractSyntaxNode ret_node{};
            ret_node.type = IR::AbstractSyntaxNode::Type::Return;

            IR::Program program{};
            program.stage = Shader::Stage::Fragment;
            program.blocks = {block};
            program.post_order_blocks = {block};
            program.syntax_list = {block_node, ret_node};
            program.info.uses_global_memory = true;
            program.info.uses_int64 = true;
            program.info.stores_frag_color[0] = true;
            program.info.constant_buffer_mask |= 1U;
            program.info.constant_buffer_used_sizes[0] = 0x400;
            program.info.constant_buffer_descriptors.push_back({.index = 0, .count = 1});
            program.info.used_constant_buffer_types =
                IR::Type::U32x4 | IR::Type::U32x2 | IR::Type::U32;
            program.info.used_storage_buffer_types = IR::Type::U32;
            program.info.nvn_buffer_base = 0x310u;
            program.info.nvn_buffer_used.reset();
            if (with_ssbo) {
                program.info.nvn_buffer_used.set(2);
                program.info.storage_buffers_descriptors.push_back({
                    .cbuf_index = 0,
                    .cbuf_offset = 0x330,
                    .count = 1,
                    .is_written = false,
                });
            }

            LowerFp16ToFp32(program);
            IdentityRemovalPass(program);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::SelectF16));
            REQUIRE(HasOpcode(program, IR::Opcode::SelectF32));

            Shader::Profile profile{};
            profile.supported_spirv = 0x00010500;
            profile.support_int64 = true;
            profile.min_ssbo_alignment = 16;
            Shader::RuntimeInfo runtime_info{};
            Shader::Backend::Bindings bindings{};
            Settings::values.fix_bloom_effects.SetValue(sanitize_bloom);
            return Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
        };

        const bool prev_bloom = Settings::values.fix_bloom_effects.GetValue();
        const auto spirv_no_ssbo = emit_case(false, false);
        const auto spirv_with_ssbo = emit_case(true, false);
        const auto spirv_bloom_sanitized = emit_case(true, true);
        Settings::values.fix_bloom_effects.SetValue(prev_bloom);

        REQUIRE(spirv_no_ssbo.size() > 5);
        REQUIRE(spirv_no_ssbo[0] == 0x07230203u);
        REQUIRE(spirv_with_ssbo.size() > spirv_no_ssbo.size() + 20);
        REQUIRE(spirv_bloom_sanitized.size() > spirv_with_ssbo.size() + 10);
    }

    SECTION("IAdd64(base, dynamic UConvert64U32 offset) tracks SSBO") {
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        const IR::U64 base = MakeCbufPtr(ir, 3, 0x80);
        const IR::U32 dyn_idx = ir.GetAttributeU32(IR::Attribute::PositionX);
        const IR::U64 dyn_off{ir.UConvert(64, dyn_idx)};
        const IR::U64 ptr{ir.IAdd(base, ir.IAdd(dyn_off, ir.Imm64(u64{0x10})))};
        const IR::U32 loaded = ir.LoadGlobal32(ptr);
        ir.Reference(loaded);

        IR::Program program{};
        program.blocks = {block};
        program.post_order_blocks = {block};

        GlobalMemoryToStorageBufferPass(program, host_info);

        REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 3);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x80);
        REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
        REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
    }

    SECTION("Converging SelectU64 tracks while diverging SelectU64 falls back cleanly") {
        // Converging Select
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::U64 base = MakeCbufPtr(ir, 2, 0x40);
            const IR::U1 cond = ir.IEqual(ir.GetReg(IR::Reg::R0), ir.Imm32(1U));
            const IR::U64 a{ir.IAdd(base, ir.Imm64(u64{0x10}))};
            const IR::U64 b{ir.IAdd(ir.Imm64(u64{0x20}), base)};
            const IR::U64 ptr{ir.Select(cond, a, b)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
        // Diverging Select
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::U64 base_a = MakeCbufPtr(ir, 2, 0x40);
            const IR::U64 base_b = MakeCbufPtr(ir, 2, 0x60);
            const IR::U1 cond = ir.IEqual(ir.GetReg(IR::Reg::R0), ir.Imm32(1U));
            const IR::U64 ptr{ir.Select(cond, base_a, base_b)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.empty());
            REQUIRE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
        // Mixed SSBO + untracked dynamic pointer Select must also fall back safely
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::U64 base_a = MakeCbufPtr(ir, 2, 0x40);
            const IR::U64 dyn_ptr = ir.PackUint2x32(
                ir.CompositeConstruct(ir.GetReg(IR::Reg::R2), ir.GetReg(IR::Reg::R3)));
            const IR::U1 cond = ir.IEqual(ir.GetReg(IR::Reg::R0), ir.Imm32(1U));
            const IR::U64 ptr{ir.Select(cond, base_a, dyn_ptr)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.empty());
            REQUIRE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
    }

    SECTION("Exact low-address tracking skips ConvertU32U64 while Select emits safe UConvert(32) fallback") {
        // Exact low-address tracking (PackUint2x32(CompositeConstruct(low, high)) + imm)
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::U64 base = MakeCbufPtr(ir, 2, 0x40);
            const IR::U64 ptr{ir.IAdd(base, ir.Imm64(u64{0x10}))};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::ConvertU32U64));
        }
        // Converging Select requires UConvert(32, ptr) fallback without crashing
        {
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            const IR::U64 base = MakeCbufPtr(ir, 2, 0x40);
            const IR::U1 cond = ir.IEqual(ir.GetReg(IR::Reg::R0), ir.Imm32(1U));
            const IR::U64 a{ir.IAdd(base, ir.Imm64(u64{0x10}))};
            const IR::U64 b{ir.IAdd(base, ir.Imm64(u64{0x20}))};
            const IR::U64 ptr{ir.Select(cond, a, b)};
            ir.Reference(ir.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {block};
            program.post_order_blocks = {block};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
            REQUIRE(HasOpcode(program, IR::Opcode::ConvertU32U64));
        }
    }

    SECTION("Converging Phi tracks WriteGlobal32 while diverging Phi falls back cleanly") {
        // Converging Phi
        {
            IR::Block* entry = block_pool.Create(inst_pool);
            IR::Block* left = block_pool.Create(inst_pool);
            IR::Block* right = block_pool.Create(inst_pool);
            IR::Block* merge = block_pool.Create(inst_pool);
            entry->AddBranch(left);
            entry->AddBranch(right);
            left->AddBranch(merge);
            right->AddBranch(merge);

            IR::IREmitter ir_entry{*entry};
            const IR::U64 base = MakeCbufPtr(ir_entry, 4, 0x20);
            const IR::U32 dyn_off32 = ir_entry.GetReg(IR::Reg::R1);

            IR::IREmitter ir_left{*left};
            const IR::U64 ptr_left{ir_left.IAdd(base, ir_left.Imm64(u64{0x10}))};

            IR::IREmitter ir_right{*right};
            const IR::U64 ptr_right{ir_right.IAdd(base, ir_right.UConvert(64, dyn_off32))};

            auto phi_it = merge->PrependNewInst(merge->end(), IR::Opcode::Phi);
            phi_it->SetFlags(IR::Type::U64);
            phi_it->AddPhiOperand(left, ptr_left);
            phi_it->AddPhiOperand(right, ptr_right);
            IR::IREmitter ir_merge{*merge};
            ir_merge.WriteGlobal32(IR::U64{IR::Value{&*phi_it}}, ir_merge.Imm32(0x1234U));

            IR::Program program{};
            program.blocks = {entry, left, right, merge};
            program.post_order_blocks = {merge, right, left, entry};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 4);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x20);
            REQUIRE(program.info.storage_buffers_descriptors[0].is_written == true);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::WriteGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::WriteStorage32));
        }
        // Diverging Phi
        {
            IR::Block* entry = block_pool.Create(inst_pool);
            IR::Block* left = block_pool.Create(inst_pool);
            IR::Block* right = block_pool.Create(inst_pool);
            IR::Block* merge = block_pool.Create(inst_pool);
            entry->AddBranch(left);
            entry->AddBranch(right);
            left->AddBranch(merge);
            right->AddBranch(merge);

            IR::IREmitter ir_left{*left};
            const IR::U64 ptr_left = MakeCbufPtr(ir_left, 4, 0x20);

            IR::IREmitter ir_right{*right};
            const IR::U64 ptr_right = MakeCbufPtr(ir_right, 5, 0x20);

            auto phi_it = merge->PrependNewInst(merge->end(), IR::Opcode::Phi);
            phi_it->SetFlags(IR::Type::U64);
            phi_it->AddPhiOperand(left, ptr_left);
            phi_it->AddPhiOperand(right, ptr_right);
            IR::IREmitter ir_merge{*merge};
            ir_merge.Reference(ir_merge.LoadGlobal32(IR::U64{IR::Value{&*phi_it}}));

            IR::Program program{};
            program.blocks = {entry, left, right, merge};
            program.post_order_blocks = {merge, right, left, entry};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.empty());
            REQUIRE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
    }

    SECTION("Loop-induction U64 Phi and converging U32x2 Phi -> PackUint2x32 track SSBO") {
        // Loop U64 Phi: ptr = Phi(entry: base, loop: IAdd64(ptr, UConvert64U32(stride)))
        {
            IR::Block* entry = block_pool.Create(inst_pool);
            IR::Block* loop = block_pool.Create(inst_pool);
            IR::Block* exit = block_pool.Create(inst_pool);
            entry->AddBranch(loop);
            loop->AddBranch(loop);
            loop->AddBranch(exit);

            IR::IREmitter ir_entry{*entry};
            const IR::U64 base = MakeCbufPtr(ir_entry, 2, 0x40);

            auto phi_it = loop->PrependNewInst(loop->end(), IR::Opcode::Phi);
            phi_it->SetFlags(IR::Type::U64);
            const IR::U64 phi_ptr{&*phi_it};

            IR::IREmitter ir_loop{*loop};
            ir_loop.Reference(ir_loop.LoadGlobal32(phi_ptr));
            const IR::U64 next_ptr{ir_loop.IAdd(phi_ptr, ir_loop.UConvert(64, ir_loop.Imm32(4U)))};
            phi_it->AddPhiOperand(entry, base);
            phi_it->AddPhiOperand(loop, next_ptr);

            IR::Program program{};
            program.blocks = {entry, loop, exit};
            program.post_order_blocks = {exit, loop, entry};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }

        // Converging U32x2 Phi -> PackUint2x32 -> LoadGlobal32
        {
            IR::Block* entry = block_pool.Create(inst_pool);
            IR::Block* left = block_pool.Create(inst_pool);
            IR::Block* right = block_pool.Create(inst_pool);
            IR::Block* merge = block_pool.Create(inst_pool);
            entry->AddBranch(left);
            entry->AddBranch(right);
            left->AddBranch(merge);
            right->AddBranch(merge);

            IR::IREmitter ir_entry{*entry};
            const IR::U32 lo{ir_entry.GetCbuf(ir_entry.Imm32(2U), ir_entry.Imm32(0x40U))};
            const IR::U32 hi{ir_entry.GetCbuf(ir_entry.Imm32(2U), ir_entry.Imm32(0x44U))};

            IR::IREmitter ir_left{*left};
            const IR::Value pair_left{ir_left.CompositeConstruct(lo, hi)};

            IR::IREmitter ir_right{*right};
            const IR::Value pair_right{
                ir_right.CompositeConstruct(ir_right.IAdd(lo, ir_right.Imm32(0x10U)), hi)};

            auto phi_it = merge->PrependNewInst(merge->end(), IR::Opcode::Phi);
            phi_it->SetFlags(IR::Type::U32x2);
            phi_it->AddPhiOperand(left, pair_left);
            phi_it->AddPhiOperand(right, pair_right);

            IR::IREmitter ir_merge{*merge};
            const IR::U64 ptr{ir_merge.PackUint2x32(IR::Value{&*phi_it})};
            ir_merge.Reference(ir_merge.LoadGlobal32(ptr));

            IR::Program program{};
            program.blocks = {entry, left, right, merge};
            program.post_order_blocks = {merge, right, left, entry};

            GlobalMemoryToStorageBufferPass(program, host_info);

            REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
            REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
            REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
            REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
        }
    }

    SECTION("BitwiseAnd32 + BitwiseOr32 low-pointer alignment/tagging tracks SSBO") {
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        const IR::U32 base_low = ir.GetCbuf(ir.Imm32(2), ir.Imm32(0x40));
        const IR::U32 masked_low = ir.BitwiseAnd(base_low, ir.Imm32(~0xFU));
        const IR::U32 low32 = ir.BitwiseOr(masked_low, ir.Imm32(0x4U));
        const IR::U32 high32 = ir.GetCbuf(ir.Imm32(2), ir.Imm32(0x44));
        const IR::U64 ptr{ir.PackUint2x32(ir.CompositeConstruct(low32, high32))};
        ir.Reference(ir.LoadGlobal32(ptr));

        IR::Program program{};
        program.blocks = {block};
        program.post_order_blocks = {block};

        GlobalMemoryToStorageBufferPass(program, host_info);

        REQUIRE(program.info.storage_buffers_descriptors.size() == 1);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_index == 2);
        REQUIRE(program.info.storage_buffers_descriptors[0].cbuf_offset == 0x40);
        REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadGlobal32));
        REQUIRE(HasOpcode(program, IR::Opcode::LoadStorage32));
    }

    SECTION("AddNVNStorageBuffers binds all 16 NVN slots even when nvn_buffer_used has partial bits") {
        IR::Program program{};
        program.stage = Shader::Stage::Fragment;
        program.info.uses_global_memory = true;
        program.info.stores_global_memory = false;
        program.info.nvn_buffer_used.reset();
        program.info.nvn_buffer_used.set(0);

        Shader::Maxwell::AddNVNStorageBuffers(program);

        REQUIRE(program.info.storage_buffers_descriptors.size() == 16);
        for (u32 i = 0; i < 16; ++i) {
            REQUIRE(program.info.storage_buffers_descriptors[i].cbuf_index == 0);
            REQUIRE(program.info.storage_buffers_descriptors[i].cbuf_offset == 0x510u + i * 0x10u);
        }
    }

    SECTION("Dynamic cbuf SSBO index array preserves LoadGlobal32 instead of mis-tracking slot 0") {
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        const IR::U32 slot_idx = ir.GetReg(IR::Reg::R0);
        const IR::U32 cbuf_off =
            ir.IAdd(ir.Imm32(0x310U), ir.ShiftLeftLogical(slot_idx, ir.Imm32(4U)));
        const IR::U32 low32 = ir.GetCbuf(ir.Imm32(0U), cbuf_off);
        const IR::U32 high32 = ir.GetCbuf(ir.Imm32(0U), ir.IAdd(cbuf_off, ir.Imm32(4U)));
        const IR::U64 ptr = ir.PackUint2x32(ir.CompositeConstruct(low32, high32));
        ir.Reference(ir.LoadGlobal32(ptr));

        IR::Program program{};
        program.blocks = {block};
        program.post_order_blocks = {block};

        GlobalMemoryToStorageBufferPass(program, host_info);

        REQUIRE(program.info.storage_buffers_descriptors.empty());
        REQUIRE(HasOpcode(program, IR::Opcode::LoadGlobal32));
        REQUIRE_FALSE(HasOpcode(program, IR::Opcode::LoadStorage32));
    }
}

TEST_CASE("Tegra::Texture::UnswizzleSubrect preserves non-zero origin_z slice", "[video_core]") {
    constexpr u32 kWidth = 64;
    constexpr u32 kHeight = 16;
    constexpr u32 kDepth = 2;
    constexpr u32 kBpp = 4;
    constexpr u32 kBlockHeight = 0;
    constexpr u32 kBlockDepth = 1;
    constexpr u32 kPitch = kWidth * kBpp;

    const std::size_t tiled_size =
        Tegra::Texture::CalculateSize(true, kBpp, kWidth, kHeight, kDepth, kBlockHeight, kBlockDepth);
    std::vector<u8> tiled(tiled_size, 0);

    std::vector<u8> slice0(kPitch * kHeight, 0x11);
    std::vector<u8> slice1(kPitch * kHeight, 0xA7);
    Tegra::Texture::SwizzleSubrect(tiled, slice0, kBpp, kWidth, kHeight, kDepth, 0, 0, kWidth, kHeight,
                                   kBlockHeight, kBlockDepth, kPitch, false, 0);
    Tegra::Texture::SwizzleSubrect(tiled, slice1, kBpp, kWidth, kHeight, kDepth, 0, 0, kWidth, kHeight,
                                   kBlockHeight, kBlockDepth, kPitch, false, 1);

    constexpr u32 kSubW = 16;
    constexpr u32 kSubH = 4;
    constexpr u32 kOutPitch = kSubW * kBpp;
    std::vector<u8> out_slice1(kOutPitch * kSubH, 0);
    Tegra::Texture::UnswizzleSubrect(out_slice1, tiled, kBpp, kWidth, kHeight, kDepth, 8, 2, kSubW,
                                     kSubH, kBlockHeight, kBlockDepth, kOutPitch, false, 1);

    REQUIRE(std::all_of(out_slice1.begin(), out_slice1.end(), [](u8 v) { return v == 0xA7; }));
}

} // namespace Shader::Optimization

namespace VideoCommon {

TEST_CASE("VideoCommon::IsBlockLinearSizeCompatibleBPPRelaxed", "[video_core]") {
    auto make_image = [](VideoCore::Surface::PixelFormat fmt, u32 width, u32 height, u32 levels = 1) {
        ImageInfo info{};
        info.type = ImageType::e2D;
        info.format = fmt;
        info.size = Extent3D{width, height, 1};
        info.resources.levels = static_cast<s32>(levels);
        info.resources.layers = 1;
        return info;
    };

    const auto bc4_256 = make_image(VideoCore::Surface::PixelFormat::BC4_UNORM, 256, 256, 2);
    const auto dma_bc4_mip0 = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 64 * 8, 64);
    const auto dma_bc4_mip1 = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 32 * 8, 32);
    const auto dma_bc4_bad_h = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 64 * 8, 128);
    const auto dma_bc4_bad_w = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 32 * 8, 64);

    REQUIRE(IsBlockLinearSizeCompatibleBPPRelaxed(bc4_256, dma_bc4_mip0, 0, 0));
    REQUIRE(IsBlockLinearSizeCompatibleBPPRelaxed(bc4_256, dma_bc4_mip1, 1, 0));
    REQUIRE_FALSE(IsBlockLinearSizeCompatibleBPPRelaxed(bc4_256, dma_bc4_bad_h, 0, 0));
    REQUIRE_FALSE(IsBlockLinearSizeCompatibleBPPRelaxed(bc4_256, dma_bc4_bad_w, 0, 0));

    const auto bc1_128 = make_image(VideoCore::Surface::PixelFormat::BC1_RGBA_UNORM, 128, 128);
    const auto dma_bc1 = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 32 * 8, 32);
    REQUIRE(IsBlockLinearSizeCompatibleBPPRelaxed(bc1_128, dma_bc1, 0, 0));

    const auto bc7_128x64 = make_image(VideoCore::Surface::PixelFormat::BC7_UNORM, 128, 64);
    const auto dma_bc7 = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 32 * 16, 16);
    const auto dma_bc7_bad = make_image(VideoCore::Surface::PixelFormat::R8_UNORM, 32 * 16, 32);
    REQUIRE(IsBlockLinearSizeCompatibleBPPRelaxed(bc7_128x64, dma_bc7, 0, 0));
    REQUIRE_FALSE(IsBlockLinearSizeCompatibleBPPRelaxed(bc7_128x64, dma_bc7_bad, 0, 0));
}

} // namespace VideoCommon

namespace Shader::Optimization {

namespace {

struct SpirvAuditStats {
    bool has_bitcast{};
    bool has_bitwise_and{};
    bool has_uless_than{};
    bool has_iequal{};
    bool has_is_nan{};
    bool has_ford_less_than{};
    bool has_fconvert{};
    bool has_fmin_ext{};
    bool has_fmax_ext{};
    bool has_fclamp_ext{};
    bool has_abs_mask{};
    bool has_inf_bits{};
    bool has_neg_inf_bits{};
    bool has_pos_65000{};
    bool has_neg_65000{};
    bool has_pos_65504{};
    bool has_neg_65504{};
    bool has_zero_f32_lower_clamp{};
};

SpirvAuditStats EmitAndAuditFragOp(
    bool fix_bloom_effects, bool broken_spirv_clamp,
    const std::function<IR::F32(IR::IREmitter&, const IR::U32&)>& build_value) {
    ObjectPool<IR::Inst> inst_pool;
    ObjectPool<IR::Block> block_pool;

    IR::Block* block = block_pool.Create(inst_pool);
    IR::IREmitter ir{*block};
    ir.Prologue();
    const IR::U32 raw = ir.GetCbuf(ir.Imm32(0U), ir.Imm32(0U));
    ir.SetFragColor(0, 0, build_value(ir, raw));
    ir.Epilogue();

    IR::AbstractSyntaxNode block_node{};
    block_node.type = IR::AbstractSyntaxNode::Type::Block;
    block_node.data.block = block;
    IR::AbstractSyntaxNode ret_node{};
    ret_node.type = IR::AbstractSyntaxNode::Type::Return;

    IR::Program program{};
    program.stage = Shader::Stage::Fragment;
    program.blocks = {block};
    program.post_order_blocks = {block};
    program.syntax_list = {block_node, ret_node};
    program.info.stores_frag_color[0] = true;
    program.info.constant_buffer_mask |= 1U;
    program.info.constant_buffer_used_sizes[0] = 0x100;
    program.info.constant_buffer_descriptors.push_back({.index = 0, .count = 1});
    program.info.used_constant_buffer_types = IR::Type::U32;

    Shader::Profile profile{};
    profile.supported_spirv = 0x00010500;
    profile.has_broken_spirv_clamp = broken_spirv_clamp;
    Shader::RuntimeInfo runtime_info{};
    Shader::Backend::Bindings bindings{};

    const bool prev_bloom = Settings::values.fix_bloom_effects.GetValue();
    Settings::values.fix_bloom_effects.SetValue(fix_bloom_effects);
    const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
    Settings::values.fix_bloom_effects.SetValue(prev_bloom);

    REQUIRE(spirv.size() > 5);
    REQUIRE(spirv[0] == 0x07230203u);

    constexpr u16 kOpExtInst = 12u;
    constexpr u16 kOpConstant = 43u;
    constexpr u16 kOpFConvert = 115u;
    constexpr u16 kOpBitcast = 124u;
    constexpr u16 kOpIsNan = 156u;
    constexpr u16 kOpSelect = 169u;
    constexpr u16 kOpIEqual = 170u;
    constexpr u16 kOpULessThan = 176u;
    constexpr u16 kOpFOrdLessThan = 182u;
    constexpr u16 kOpBitwiseAnd = 199u;
    constexpr u32 kGlslFMin = 37u;
    constexpr u32 kGlslFMax = 40u;
    constexpr u32 kGlslFClamp = 43u;

    const u32 pos_65000_bits = std::bit_cast<u32>(65000.0f);
    const u32 neg_65000_bits = std::bit_cast<u32>(-65000.0f);
    const u32 pos_65504_bits = std::bit_cast<u32>(65504.0f);
    const u32 neg_65504_bits = std::bit_cast<u32>(-65504.0f);

    SpirvAuditStats stats{};
    for (size_t i = 5; i < spirv.size();) {
        const u32 word_count = spirv[i] >> 16;
        const u16 opcode = static_cast<u16>(spirv[i] & 0xFFFFu);
        REQUIRE(word_count >= 1);
        REQUIRE(i + word_count <= spirv.size());
        if (opcode == kOpBitcast) stats.has_bitcast = true;
        if (opcode == kOpBitwiseAnd) stats.has_bitwise_and = true;
        if (opcode == kOpULessThan) stats.has_uless_than = true;
        if (opcode == kOpIEqual) stats.has_iequal = true;
        if (opcode == kOpIsNan) stats.has_is_nan = true;
        if (opcode == kOpFOrdLessThan) stats.has_ford_less_than = true;
        if (opcode == kOpFConvert) stats.has_fconvert = true;
        if (opcode == kOpExtInst && word_count >= 5) {
            if (spirv[i + 4] == kGlslFMin) stats.has_fmin_ext = true;
            if (spirv[i + 4] == kGlslFMax) stats.has_fmax_ext = true;
            if (spirv[i + 4] == kGlslFClamp) stats.has_fclamp_ext = true;
        }
        if (opcode == kOpConstant && word_count >= 4) {
            const u32 lit = spirv[i + 3];
            if (lit == 0x7FFFFFFFu) stats.has_abs_mask = true;
            if (lit == 0x7F800000u) stats.has_inf_bits = true;
            if (lit == 0xFF800000u) stats.has_neg_inf_bits = true;
            if (lit == pos_65000_bits) stats.has_pos_65000 = true;
            if (lit == neg_65000_bits) stats.has_neg_65000 = true;
            if (lit == pos_65504_bits) stats.has_pos_65504 = true;
            if (lit == neg_65504_bits) stats.has_neg_65504 = true;
        }
        (void)kOpSelect;
        i += word_count;
    }
    stats.has_zero_f32_lower_clamp = !(stats.has_neg_65000 || stats.has_neg_65504);
    return stats;
}

} // namespace

TEST_CASE("EmitSPIRV: Snowdrop blowout shield sanitizes F16/Half2x16/Exp2/Log2/Rcp/Rsq cleanly",
          "[shader_recompiler]") {
    SECTION("EmitConvertF16F32 clamps signed F32 before F16 conversion without F16 OpIsNan") {
        const auto s = EmitAndAuditFragOp(true, false, [](IR::IREmitter& ir, const IR::U32& raw) {
            const IR::F32 f = ir.BitCast<IR::F32>(raw);
            const IR::F16 h = ir.FPConvert(16, f);
            return static_cast<IR::F32>(ir.FPConvert(32, h));
        });
        REQUIRE(s.has_fconvert);
        REQUIRE_FALSE(s.has_is_nan);
        REQUIRE(s.has_pos_65504);
        REQUIRE(s.has_neg_65504);
        REQUIRE_FALSE(s.has_zero_f32_lower_clamp);
    }

    SECTION("EmitUnpackHalf2x16 and EmitPackHalf2x16 preserve signed negative bounds cleanly") {
        const auto su = EmitAndAuditFragOp(true, true, [](IR::IREmitter& ir, const IR::U32& raw) {
            const IR::Value vec = ir.UnpackHalf2x16(raw);
            return IR::F32{ir.CompositeExtract(vec, 0)};
        });
        REQUIRE(su.has_bitcast);
        REQUIRE(su.has_bitwise_and);
        REQUIRE(su.has_uless_than);
        REQUIRE(su.has_pos_65504);
        REQUIRE(su.has_neg_65504);
        REQUIRE(su.has_fmin_ext);
        REQUIRE(su.has_fmax_ext);
        REQUIRE_FALSE(su.has_fclamp_ext);

        const auto sp = EmitAndAuditFragOp(true, false, [](IR::IREmitter& ir, const IR::U32& raw) {
            const IR::F32 f = ir.BitCast<IR::F32>(raw);
            const IR::U32 packed = ir.PackHalf2x16(ir.CompositeConstruct(f, f));
            return ir.BitCast<IR::F32>(packed);
        });
        REQUIRE(sp.has_pos_65504);
        REQUIRE(sp.has_neg_65504);
        REQUIRE_FALSE(sp.has_ford_less_than);
    }

    SECTION("EmitFPLog2 maps -Inf to negative bound instead of 0.0f so pow(0, p) stays 0.0f") {
        const auto s = EmitAndAuditFragOp(true, false, [](IR::IREmitter& ir, const IR::U32& raw) {
            return ir.FPLog2(ir.BitCast<IR::F32>(raw));
        });
        REQUIRE(s.has_iequal);
        REQUIRE(s.has_neg_inf_bits);
        REQUIRE(s.has_pos_65000);
        REQUIRE(s.has_neg_65000);
    }

    SECTION("EmitFPExp2, EmitFPRecip32, and EmitFPRecipSqrt32 guard Inf/NaN when fix_bloom_effects=true") {
        const auto build_ops = std::array<std::function<IR::F32(IR::IREmitter&, const IR::U32&)>, 3>{
            [](IR::IREmitter& ir, const IR::U32& raw) { return ir.FPExp2(ir.BitCast<IR::F32>(raw)); },
            [](IR::IREmitter& ir, const IR::U32& raw) {
                return IR::F32{ir.FPRecip(ir.BitCast<IR::F32>(raw))};
            },
            [](IR::IREmitter& ir, const IR::U32& raw) {
                return IR::F32{ir.FPRecipSqrt(ir.BitCast<IR::F32>(raw))};
            },
        };
        for (const auto& fn : build_ops) {
            const auto on = EmitAndAuditFragOp(true, false, fn);
            REQUIRE(on.has_bitcast);
            REQUIRE(on.has_bitwise_and);
            REQUIRE(on.has_uless_than);
            REQUIRE(on.has_abs_mask);
            REQUIRE(on.has_inf_bits);
            REQUIRE(on.has_pos_65000);
            REQUIRE(on.has_neg_65000);
            REQUIRE(on.has_fclamp_ext);

            const auto off = EmitAndAuditFragOp(false, false, fn);
            REQUIRE_FALSE(off.has_abs_mask);
            REQUIRE_FALSE(off.has_pos_65000);
            REQUIRE_FALSE(off.has_neg_65000);
        }
    }
}

TEST_CASE("GameFixDatabase: Build #12 D2R, M+R Overrides and Absolum Lock Invariant", "[core]") {
    for (const u64 tid : {0x0100726014352000ULL, 0x01002A801458A000ULL, 0x0100916014D8C000ULL}) {
        const auto* profile = Core::GameFixDatabase::GetProfile(tid);
        REQUIRE(profile != nullptr);
        REQUIRE(profile->ini_settings.at("Core\\memory_layout_mode") == "1");
        REQUIRE(profile->ini_settings.at("Renderer\\gpu_accuracy") == "1");
        REQUIRE(profile->ini_settings.at("Renderer\\dyna_state") == "1");
        REQUIRE(profile->ini_settings.at("Renderer\\dma_accuracy") == "3");
        REQUIRE(profile->ini_settings.at("Renderer\\use_fast_gpu_time") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\enable_gpu_buffer_readback") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\barrier_feedback_loops") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\use_reactive_flushing") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\use_vulkan_driver_pipeline_cache") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\use_asynchronous_shaders") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\async_presentation") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\sync_memory_operations") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\fix_bloom_effects") == "false");
    }

    for (const u64 tid : {0x010067300059A000ULL, 0x010067300B762000ULL}) {
        const auto* profile = Core::GameFixDatabase::GetProfile(tid);
        REQUIRE(profile != nullptr);
        REQUIRE(profile->ini_settings.at("Core\\memory_layout_mode") == "0");
        REQUIRE(profile->ini_settings.at("Renderer\\gpu_accuracy") == "2");
        REQUIRE(profile->ini_settings.at("Renderer\\dma_accuracy") == "3");
        REQUIRE(profile->ini_settings.at("Renderer\\fix_bloom_effects") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\enable_gpu_buffer_readback") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\sync_memory_operations") == "true");
        REQUIRE(profile->ini_settings.at("Renderer\\use_asynchronous_shaders") == "false");
        REQUIRE(profile->ini_settings.at("Renderer\\async_presentation") == "false");
    }

    const auto* absolum = Core::GameFixDatabase::GetProfile(0x0100A83022A22000ULL);
    REQUIRE(absolum != nullptr);
    REQUIRE(absolum->ini_settings.at("Core\\memory_layout_mode") == "0");
    REQUIRE(absolum->ini_settings.at("Renderer\\gpu_accuracy") == "2");
    REQUIRE(absolum->ini_settings.at("Renderer\\nvdec_emulation") == "2");
    REQUIRE(absolum->ini_settings.at("Renderer\\dma_accuracy") == "3");
    REQUIRE(absolum->ini_settings.at("Renderer\\use_asynchronous_shaders") == "true");
    REQUIRE(absolum->ini_settings.at("Renderer\\async_presentation") == "false");
    REQUIRE(absolum->ini_settings.at("Renderer\\use_fast_gpu_time") == "false");
    REQUIRE(absolum->ini_settings.at("Renderer\\sync_memory_operations") == "true");
    REQUIRE(absolum->ini_settings.at("Renderer\\enable_gpu_buffer_readback") == "true");
    REQUIRE(absolum->ini_settings.at("Renderer\\fix_bloom_effects") == "true");
}

TEST_CASE("EmitSPIRV: Warp BroadcastLane and ShuffleAbsolute scale across max_subgroup_size",
          "[shader_recompiler]") {
    auto emit_warp_size = [](u32 max_subgroup_size, bool broken_shuffle, bool use_dynamic_idx) {
        ObjectPool<IR::Inst> inst_pool;
        ObjectPool<IR::Block> block_pool;
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        ir.Prologue();
        const IR::U32 val = ir.GetCbuf(ir.Imm32(0U), ir.Imm32(0U));
        const IR::U32 idx = use_dynamic_idx ? ir.GetCbuf(ir.Imm32(0U), ir.Imm32(4U)) : ir.Imm32(0U);
        const IR::U32 shuffled = ir.ShuffleIndex(val, idx, ir.Imm32(31U), ir.Imm32(0U));
        ir.SetFragColor(0, 0, ir.BitCast<IR::F32>(shuffled));
        ir.Epilogue();

        IR::AbstractSyntaxNode block_node{};
        block_node.type = IR::AbstractSyntaxNode::Type::Block;
        block_node.data.block = block;
        IR::AbstractSyntaxNode ret_node{};
        ret_node.type = IR::AbstractSyntaxNode::Type::Return;

        IR::Program program{};
        program.stage = Shader::Stage::Fragment;
        program.blocks = {block};
        program.post_order_blocks = {block};
        program.syntax_list = {block_node, ret_node};
        program.info.stores_frag_color[0] = true;
        program.info.constant_buffer_mask |= 1U;
        program.info.constant_buffer_used_sizes[0] = 0x100;
        program.info.constant_buffer_descriptors.push_back({.index = 0, .count = 1});
        program.info.used_constant_buffer_types = IR::Type::U32;
        program.info.uses_subgroup_shuffles = true;
        program.info.uses_subgroup_invocation_id = true;

        Shader::Profile profile{};
        profile.supported_spirv = 0x00010500;
        profile.support_vote = true;
        profile.supported_subgroup_stages = 0x7Fu;
        profile.warp_size_potentially_larger_than_guest = true;
        profile.has_broken_spirv_subgroup_shuffle = broken_shuffle;
        profile.max_subgroup_size = max_subgroup_size;

        Shader::RuntimeInfo runtime_info{};
        Shader::Backend::Bindings bindings{};
        const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
        REQUIRE(spirv.size() > 5);
        REQUIRE(spirv[0] == 0x07230203u);

        constexpr u16 kOpSelect = 169u;
        size_t select_count = 0;
        for (size_t i = 5; i < spirv.size();) {
            const u32 word_count = spirv[i] >> 16;
            const u16 opcode = static_cast<u16>(spirv[i] & 0xFFFFu);
            REQUIRE(word_count >= 1);
            REQUIRE(i + word_count <= spirv.size());
            if (opcode == kOpSelect) {
                ++select_count;
            }
            i += word_count;
        }
        return std::pair<size_t, size_t>{spirv.size(), select_count};
    };

    SECTION("BroadcastLane expands partition selects when max_subgroup_size > 32") {
        const auto [words_0, sel_0] = emit_warp_size(0u, false, false);
        const auto [words_64, sel_64] = emit_warp_size(64u, false, false);
        const auto [words_128, sel_128] = emit_warp_size(128u, false, false);
        REQUIRE(words_64 > words_0);
        REQUIRE(words_128 > words_64);
        REQUIRE(sel_64 > sel_0);
        REQUIRE(sel_128 > sel_64);
    }

    SECTION("ShuffleAbsolute fallback scales with max_subgroup_size") {
        const auto [words_0, sel_0] = emit_warp_size(0u, true, true);
        const auto [words_64, sel_64] = emit_warp_size(64u, true, true);
        const auto [words_128, sel_128] = emit_warp_size(128u, true, true);
        REQUIRE(words_64 > words_0);
        REQUIRE(words_128 > words_64);
        REQUIRE(sel_64 > sel_0);
        REQUIRE(sel_128 > sel_64);
    }
}

TEST_CASE("EmitSPIRV: ImageFetch skips SanitizeBloomSample and DefineGlobalMemoryFunctions resolves non-NVN SSBO 8 MiB fallback",
          "[shader_recompiler]") {
    SECTION("NUM_STORAGE_BUFFERS capacity matches Shader::Info::MAX_SSBOS") {
        REQUIRE(VideoCommon::NUM_STORAGE_BUFFERS == 32u);
        REQUIRE(VideoCommon::NUM_STORAGE_BUFFERS >= Shader::Info::MAX_SSBOS);
    }

    SECTION("EmitImageFetch preserves raw texel values when fix_bloom_effects=true") {
        ObjectPool<IR::Inst> inst_pool;
        ObjectPool<IR::Block> block_pool;
        IR::Block* block = block_pool.Create(inst_pool);
        IR::IREmitter ir{*block};
        ir.Prologue();
        IR::TextureInstInfo tex_info{};
        tex_info.descriptor_index.Assign(0u);
        tex_info.type.Assign(Shader::TextureType::Color2D);
        const IR::Value coords = ir.CompositeConstruct(ir.Imm32(0U), ir.Imm32(0U));
        const IR::Value fetched = ir.ImageFetch(ir.Imm32(0U), coords, {}, ir.Imm32(0U), {}, tex_info);
        fetched.InstRecursive()->ReplaceOpcode(IR::Opcode::ImageFetch);
        ir.SetFragColor(0, 0, IR::F32{ir.CompositeExtract(fetched, 0)});
        ir.Epilogue();

        IR::AbstractSyntaxNode block_node{};
        block_node.type = IR::AbstractSyntaxNode::Type::Block;
        block_node.data.block = block;
        IR::AbstractSyntaxNode ret_node{};
        ret_node.type = IR::AbstractSyntaxNode::Type::Return;

        IR::Program program{};
        program.stage = Shader::Stage::Fragment;
        program.blocks = {block};
        program.post_order_blocks = {block};
        program.syntax_list = {block_node, ret_node};
        program.info.stores_frag_color[0] = true;
        program.info.texture_descriptors.push_back({
            .type = Shader::TextureType::Color2D,
            .cbuf_index = 0,
            .cbuf_offset = 0x20,
            .count = 1,
        });

        Shader::Profile profile{};
        profile.supported_spirv = 0x00010500;
        Shader::RuntimeInfo runtime_info{};
        runtime_info.color_output_types[0] = Shader::AttributeType::UnsignedInt;
        Shader::Backend::Bindings bindings{};

        const bool prev_bloom = Settings::values.fix_bloom_effects.GetValue();
        Settings::values.fix_bloom_effects.SetValue(true);
        const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
        Settings::values.fix_bloom_effects.SetValue(prev_bloom);

        REQUIRE(spirv.size() > 5);
        REQUIRE(spirv[0] == 0x07230203u);

        constexpr u16 kOpConstant = 43u;
        const u32 pos_65000_bits = std::bit_cast<u32>(65000.0f);
        const u32 neg_65000_bits = std::bit_cast<u32>(-65000.0f);
        bool has_pos_65000 = false;
        bool has_neg_65000 = false;
        for (size_t i = 5; i < spirv.size();) {
            const u32 word_count = spirv[i] >> 16;
            const u16 opcode = static_cast<u16>(spirv[i] & 0xFFFFu);
            REQUIRE(word_count >= 1);
            REQUIRE(i + word_count <= spirv.size());
            if (opcode == kOpConstant && word_count >= 4) {
                if (spirv[i + 3] == pos_65000_bits) has_pos_65000 = true;
                if (spirv[i + 3] == neg_65000_bits) has_neg_65000 = true;
            }
            i += word_count;
        }
        REQUIRE_FALSE(has_pos_65000);
        REQUIRE_FALSE(has_neg_65000);
    }

    SECTION("EmitImageFetch on integer TextureBuffer emits U32 sampled Dim::Buffer image") {
        auto emit_texbuf = [](bool is_int_texbuf) {
            ObjectPool<IR::Inst> inst_pool;
            ObjectPool<IR::Block> block_pool;
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            ir.Prologue();
            IR::TextureInstInfo tex_info{};
            tex_info.descriptor_index.Assign(0u);
            tex_info.type.Assign(Shader::TextureType::Buffer);
            tex_info.is_integer.Assign(is_int_texbuf ? 1u : 0u);
            const IR::Value fetched =
                ir.ImageFetch(ir.Imm32(0U), ir.Imm32(0U), {}, ir.Imm32(0U), {}, tex_info);
            fetched.InstRecursive()->ReplaceOpcode(IR::Opcode::ImageFetch);
            ir.SetFragColor(0, 0, IR::F32{ir.CompositeExtract(fetched, 0)});
            ir.Epilogue();

            IR::AbstractSyntaxNode block_node{};
            block_node.type = IR::AbstractSyntaxNode::Type::Block;
            block_node.data.block = block;
            IR::AbstractSyntaxNode ret_node{};
            ret_node.type = IR::AbstractSyntaxNode::Type::Return;

            IR::Program program{};
            program.stage = Shader::Stage::Fragment;
            program.blocks = {block};
            program.post_order_blocks = {block};
            program.syntax_list = {block_node, ret_node};
            program.info.stores_frag_color[0] = true;
            program.info.texture_buffer_descriptors.push_back({
                .has_secondary = false,
                .is_integer = is_int_texbuf,
                .cbuf_index = 0,
                .cbuf_offset = 0x20,
                .count = 1,
            });

            Shader::Profile profile{};
            profile.supported_spirv = 0x00010500;
            Shader::RuntimeInfo runtime_info{};
            runtime_info.color_output_types[0] = Shader::AttributeType::UnsignedInt;
            Shader::Backend::Bindings bindings{};
            const auto spirv =
                Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
            REQUIRE(spirv.size() > 5);

            constexpr u16 kOpTypeInt = 21u;
            constexpr u16 kOpTypeFloat = 22u;
            constexpr u16 kOpTypeImage = 25u;
            constexpr u32 kSpvDimBuffer = 5u;
            u32 u32_type_id = 0;
            u32 f32_type_id = 0;
            u32 buffer_sampled_type_id = 0;
            for (size_t i = 5; i < spirv.size();) {
                const u32 wc = spirv[i] >> 16;
                const u16 op = static_cast<u16>(spirv[i] & 0xFFFFu);
                if (op == kOpTypeInt && wc >= 4 && spirv[i + 2] == 32u && spirv[i + 3] == 0u) {
                    u32_type_id = spirv[i + 1];
                } else if (op == kOpTypeFloat && wc >= 3 && spirv[i + 2] == 32u) {
                    f32_type_id = spirv[i + 1];
                } else if (op == kOpTypeImage && wc >= 9 && spirv[i + 3] == kSpvDimBuffer) {
                    buffer_sampled_type_id = spirv[i + 2];
                }
                i += wc;
            }
            REQUIRE(u32_type_id != 0);
            REQUIRE(f32_type_id != 0);
            REQUIRE(buffer_sampled_type_id != 0);
            return std::tuple{buffer_sampled_type_id, u32_type_id, f32_type_id};
        };

        const auto [int_sampled, int_u32, int_f32] = emit_texbuf(true);
        REQUIRE(int_sampled == int_u32);
        REQUIRE(int_sampled != int_f32);

        const auto [flt_sampled, flt_u32, flt_f32] = emit_texbuf(false);
        REQUIRE(flt_sampled == flt_f32);
        REQUIRE(flt_sampled != flt_u32);
    }

    SECTION("DefineGlobalMemoryFunctions resolves non-NVN descriptor with 8 MiB fallback") {
        for (const bool aliasing : {false, true}) {
            ObjectPool<IR::Inst> inst_pool;
            ObjectPool<IR::Block> block_pool;
            IR::Block* block = block_pool.Create(inst_pool);
            IR::IREmitter ir{*block};
            ir.Prologue();
            const IR::U32 raw = ir.GetCbuf(ir.Imm32(0U), ir.Imm32(0U));
            const IR::U64 addr = ir.PackUint2x32(ir.CompositeConstruct(raw, raw));
            const IR::U32 val = ir.LoadGlobal32(addr);
            ir.SetFragColor(0, 0, ir.BitCast<IR::F32>(val));
            ir.Epilogue();

            IR::AbstractSyntaxNode block_node{};
            block_node.type = IR::AbstractSyntaxNode::Type::Block;
            block_node.data.block = block;
            IR::AbstractSyntaxNode ret_node{};
            ret_node.type = IR::AbstractSyntaxNode::Type::Return;

            IR::Program program{};
            program.stage = Shader::Stage::Fragment;
            program.blocks = {block};
            program.post_order_blocks = {block};
            program.syntax_list = {block_node, ret_node};
            program.info.stores_frag_color[0] = true;
            program.info.uses_int64 = true;
            program.info.uses_global_memory = true;
            program.info.used_storage_buffer_types = IR::Type::U32;
            program.info.used_constant_buffer_types = IR::Type::U32;
            program.info.constant_buffer_mask = (1U << 0) | (1U << 2);
            program.info.constant_buffer_used_sizes[0] = 0x100;
            program.info.constant_buffer_used_sizes[2] = 0x10;
            program.info.constant_buffer_descriptors.push_back({.index = 0, .count = 1});
            program.info.constant_buffer_descriptors.push_back({.index = 2, .count = 1});
            program.info.storage_buffers_descriptors.push_back({
                .cbuf_index = 2,
                .cbuf_offset = 0,
                .count = 1,
                .is_written = false,
            });

            Shader::Profile profile{};
            profile.supported_spirv = 0x00010500;
            profile.support_int64 = true;
            profile.support_descriptor_aliasing = aliasing;
            profile.min_ssbo_alignment = 16;
            Shader::RuntimeInfo runtime_info{};
            Shader::Backend::Bindings bindings{};

            const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
            REQUIRE(spirv.size() > 5);
            REQUIRE(spirv[0] == 0x07230203u);

            constexpr u16 kOpConstant = 43u;
            bool has_8mib_fallback = false;
            for (size_t i = 5; i < spirv.size();) {
                const u32 word_count = spirv[i] >> 16;
                const u16 opcode = static_cast<u16>(spirv[i] & 0xFFFFu);
                REQUIRE(word_count >= 1);
                REQUIRE(i + word_count <= spirv.size());
                if (opcode == kOpConstant && word_count >= 4 && spirv[i + 3] == 0x800000u) {
                    has_8mib_fallback = true;
                }
                i += word_count;
            }
            REQUIRE(has_8mib_fallback);
        }
    }
}

} // namespace Shader::Optimization
