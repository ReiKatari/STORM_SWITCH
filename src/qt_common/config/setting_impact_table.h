// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>
#include <unordered_map>
#include <QString>
#include <QLocale>
#include "qt_common/config/uisettings.h"

namespace ConfigurationShared {

enum class ImpactLevel {
    None,
    Low,
    Med,
    High
};

struct SettingImpact {
    ImpactLevel performance = ImpactLevel::None;
    ImpactLevel heat = ImpactLevel::None;
    ImpactLevel compatibility = ImpactLevel::None;
    ImpactLevel quality = ImpactLevel::None;
    const char* effect_en = "";
    const char* effect_ru = "";
};

inline const std::unordered_map<std::string_view, SettingImpact>& GetSettingImpactMap() {
    static const std::unordered_map<std::string_view, SettingImpact> kMap = {
    {"cabinet_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"controller_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"data_erase_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"error_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"net_connect_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"player_select_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"swkbd_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"mii_edit_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"web_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"shop_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"photo_viewer_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"offline_web_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"login_share_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"wifi_web_auth_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"my_page_applet_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Configures whether applet runs in HLE or LLE mode. Currently NOT WIRED in library_applet_creator.", "Режим работы апплета: высокоуровневая эмуляция (HLE) или LLE."}},
    {"enable_overlay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"output_engine", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"output_device", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"input_device", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"sound_index", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "System sound mode (Mono/Stereo/Surround). Ignored by audio_core.", "System sound mode (Mono/Stereo/Surround). Ignored by audio_core."}},
    {"volume", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"audio_muted", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dump_audio_commands", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_multi_core", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"memory_layout_mode", {ImpactLevel::Med, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Low, "Configures guest physical DRAM pool size (4GB, 6GB, 8GB).", "Объем оперативной памяти гостевой системы (4 ГБ стандарт, 6/8 ГБ для модов)."}},
    {"use_speed_limit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Ограничение скорости эмуляции (100% = нормальная скорость игры)."}},
    {"speed_limit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Ограничение скорости эмуляции (100% = нормальная скорость игры)."}},
    {"slow_speed_limit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Ограничение скорости эмуляции (100% = нормальная скорость игры)."}},
    {"turbo_speed_limit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Ограничение скорости эмуляции (100% = нормальная скорость игры)."}},
    {"current_speed_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"sync_core_speed", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"cpu_backend", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"cpu_accuracy", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, "Selects Dynarmic JIT optimization tier (Auto, Accurate, Unsafe, Paranoid, Debugging).", "Selects Dynarmic JIT optimization tier (Auto, Accurate, Unsafe, Paranoid, Debugging)."}},
    {"cpu_clock", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_custom_cpu_ticks", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"cpu_ticks", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"cpuopt_page_tables", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_page_tables). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_page_tables). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_block_linking", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_block_linking). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_block_linking). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_return_stack_buffer", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_return_stack_buffer). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_return_stack_buffer). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_fast_dispatcher", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_fast_dispatcher). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_fast_dispatcher). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_context_elimination", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_context_elimination). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_context_elimination). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_const_prop", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_const_prop). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_const_prop). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_misc_ir", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Dynarmic IR pass flag (cpuopt_misc_ir). Ignored outside CpuAccuracy::Debugging.", "Dynarmic IR pass flag (cpuopt_misc_ir). Ignored outside CpuAccuracy::Debugging."}},
    {"cpuopt_reduce_misalign_checks", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_reduce_misalign_checks.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_reduce_misalign_checks."}},
    {"cpuopt_fastmem", {ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups.", "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups."}},
    {"cpuopt_fastmem_exclusives", {ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups.", "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups."}},
    {"cpuopt_recompile_exclusives", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_recompile_exclusives.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_recompile_exclusives."}},
    {"cpuopt_ignore_memory_aborts", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_ignore_memory_aborts.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_ignore_memory_aborts."}},
    {"cpuopt_unsafe_host_mmu", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_host_mmu.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_host_mmu."}},
    {"cpuopt_unsafe_unfuse_fma", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_unfuse_fma.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_unfuse_fma."}},
    {"cpuopt_unsafe_reduce_fp_error", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_reduce_fp_error.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_reduce_fp_error."}},
    {"cpuopt_unsafe_ignore_standard_fpcr", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_ignore_standard_fpcr.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_ignore_standard_fpcr."}},
    {"cpuopt_unsafe_inaccurate_nan", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_inaccurate_nan.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_inaccurate_nan."}},
    {"cpuopt_unsafe_fastmem_check", {ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups.", "Maps guest virtual address space directly to host virtual memory, bypassing software page table lookups."}},
    {"cpuopt_unsafe_ignore_global_monitor", {ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Med, ImpactLevel::Low, "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_ignore_global_monitor.", "Fine-grained Dynarmic JIT compiler optimization flag: cpuopt_unsafe_ignore_global_monitor."}},
    {"backend", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vulkan_device", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"resolution_setup", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Масштаб разрешения рендеринга (четкость и нагрузка на видеокарту)."}},
    {"use_vsync", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Вертикальная синхронизация кадров для устранения разрывов экрана."}},
    {"scaling_filter", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"fsr_sharpening_slider", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"fsr2_quality_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"aspect_ratio", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"anti_aliasing", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"frame_gen", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_multiplier", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_target_rate", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_flow_scale_auto", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_flow_scale", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_queue_target", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_fp16", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"frame_gen_dump_flow", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"use_asynchronous_gpu_emulation", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Асинхронные вычисления для сглаживания фреймтайма."}},
    {"fullscreen_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"bg_red", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"bg_green", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"bg_blue", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_accuracy", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, "Controls GPU emulation precision and Maxwell texture/buffer memory synchronization frequency.", "Controls GPU emulation precision and Maxwell texture/buffer memory synchronization frequency."}},
    {"dma_accuracy", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_fence_behavior", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vram_usage_mode", {ImpactLevel::Med, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Low, "Controls Vulkan VRAM allocation aggressiveness and garbage collection thresholds (Conservative vs Aggressive).", "Controls Vulkan VRAM allocation aggressiveness and garbage collection thresholds (Conservative vs Aggressive)."}},
    {"nvdec_emulation", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"max_anisotropy", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"accelerate_astc", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Декодирование текстур ASTC на процессоре или видеокарте."}},
    {"frame_pacing_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"astc_recompression", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Декодирование текстур ASTC на процессоре или видеокарте."}},
    {"drs_resolution_lock", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Масштаб разрешения рендеринга (четкость и нагрузка на видеокарту)."}},
    {"sync_memory_operations", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"force_max_clock", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"early_release_fences", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"optimize_spirv_output", {ImpactLevel::Med, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Med, "Runs SPIR-V optimization passes (DCE and identity removal) to eliminate redundant instructions and boost shader execution.", "Оптимизация SPIR-V шейдеров (DCE и удаление тождественных инструкций) для повышения производительности GPU."}},
    {"use_fast_gpu_time", {ImpactLevel::Med, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Med, "Approximates GPU timestamp ticks based on elapsed host time rather than serialized command stream execution.", "Approximates GPU timestamp ticks based on elapsed host time rather than serialized command stream execution."}},
    {"enable_frame_skipping", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_disk_shader_cache", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Сохранение скомпилированных шейдеров на диск для мгновенного запуска."}},
    {"use_vulkan_driver_pipeline_cache", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_compute_pipelines", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_video_framerate", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_reactive_flushing", {ImpactLevel::Med, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Low, "Monitors guest CPU reads to texture cache and triggers selective CPU-GPU buffer synchronization.", "Monitors guest CPU reads to texture cache and triggers selective CPU-GPU buffer synchronization."}},
    {"barrier_feedback_loops", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_buffer_history", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_optimized_vertex_buffers", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"eco_thermal_mode", {ImpactLevel::Med, ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, "STORM Eco Thermal Governor: clamps pipeline compilation threads, drops resolution floor, and caps CPU boost.", "STORM Eco Thermal Governor: clamps pipeline compilation threads, drops resolution floor, and caps CPU boost."}},
    {"eco_frame_pacing", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"smart_shader_throttle", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"cpu_affinity_pinning", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vram_garbage_collection", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_hdr10", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"auto_aspect_ratio_patch", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"auto_60fps_unlock", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vram_budget_governor", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"storm_lowend_turbo", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"storm_thermal_governor", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_clock", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"skip_cpu_inner_invalidation", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"async_presentation", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Асинхронные вычисления для сглаживания фреймтайма."}},
    {"fix_bloom_effects", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"emulate_bgr565", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"rescale_hack", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_gpu_buffer_readback", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_asynchronous_shaders", {ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Med, ImpactLevel::Med, "Compiles newly encountered Vulkan shader pipelines asynchronously in background worker threads.", "Асинхронная компиляция шейдеров в фоновых потоках для устранения микрофризов."}},
    {"gpu_unswizzle_texture_size", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_unswizzle_stream_size", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_unswizzle_chunk_size", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_unswizzle_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dyna_state", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"sample_shading_fraction", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vertex_input_dynamic_state", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"debug", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"shader_feedback", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"nsight_aftermath", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_shader_loop_safety_checks", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"renderdoc_hotkey", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"patch_old_qcom_drivers", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_buffer_reorder", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"language_index", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"region_index", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"time_zone_index", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"serial_battery", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"serial_unit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"custom_rtc_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"custom_rtc", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"custom_rtc_offset", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"rng_seed_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"rng_seed", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"device_name", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"current_user", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_docked_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_wgi_xinput", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_raw_input", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"controller_navigation", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_joycon_driver", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_procon_driver", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"vibration_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_accurate_vibrations", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"motion_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"udp_input_servers", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_udp_controller", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"pause_tas_on_load", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"tas_enable", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"tas_loop", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"tas_show_recording_dialog", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_sensitivity", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Dead legacy mouse panning sensitivity, replaced by mouse_panning_x_sensitivity and mouse_panning_y_sensitivity.", "Dead legacy mouse panning sensitivity, replaced by mouse_panning_x_sensitivity and mouse_panning_y_sensitivity."}},
    {"mouse_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_x_sensitivity", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_y_sensitivity", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_deadzone_counterweight", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_decay_strength", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"mouse_panning_min_decay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"emulate_analog_keyboard", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"keyboard_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"debug_pad_enabled", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"touch_device", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"touch_from_button_map", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_ring_controller", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_ir_sensor", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"ir_sensor_device", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"random_amiibo_id", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_virtual_sd", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Legacy setting for virtual SD card mounting, hardcoded enabled in file_sys.", "Legacy setting for virtual SD card mounting, hardcoded enabled in file_sys."}},
    {"gamecard_inserted", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gamecard_current_game", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gamecard_path", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"ext_content_from_game_dirs", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_gdbstub", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gdbstub_port", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"program_args", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dump_exefs", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dump_nso", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dump_guest_shaders", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dump_macros", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_fs_access_log", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"reporting_services", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"quest_flag", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_dev_keys", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_macro_jit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_macro_hle", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"extended_logging", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_debug_asserts", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"use_auto_stub", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"enable_all_controllers", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"perform_vulkan_check", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"disable_web_applet", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_level", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_vulkan_calls", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_shader_dumps", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_memory_tracking", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_driver_debug", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"gpu_log_ring_buffer_size", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"debug_knobs", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"log_filter", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"flush_line", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"censor_username", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"first_launch", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"network_interface", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"airplane_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"web_api_url", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"eden_username", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"eden_token", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"dynamic_performance_scaler", {ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "STORM Dynamic Performance Scaler: dynamically steps resolution scaling up/down to maintain target framerate.", "STORM Dynamic Performance Scaler: dynamically steps resolution scaling up/down to maintain target framerate."}},
    {"use_storm_tier2_compiler", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"post_processing_shader", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::High, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"pipeline_worker_threads", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, "Configures number of parallel background worker threads for Vulkan pipeline compilation.", "Configures number of parallel background worker threads for Vulkan pipeline compilation."}},
    {"enable_frame_interpolation", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Med, ImpactLevel::Low, "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames.", "Lossless Scaling temporal frame generation using optical flow interpolation between consecutive frames."}},
    {"save_states_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"current_program_id", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "General configuration", "Базовый параметр конфигурации эмулятора."}},
    {"picture_in_picture", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"screen_layout", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"vertical_alignment", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"driver_path", {ImpactLevel::High, ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Low, "Custom Vulkan GPU driver package path (Mesa Turnip, Qualcomm Adreno proprietary).", "Custom Vulkan GPU driver package path (Mesa Turnip, Qualcomm Adreno proprietary)."}},
    {"theme", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: theme.", "Android HUD / Overlay rendering flag: theme."}},
    {"theme_mode", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: theme_mode.", "Android HUD / Overlay rendering flag: theme_mode."}},
    {"static_theme_color", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: static_theme_color.", "Android HUD / Overlay rendering flag: static_theme_color."}},
    {"black_backgrounds", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"app_language", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"enable_update_checks", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"enable_folder_button", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"enable_qlaunch_button", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"show_filename_as_title", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_filename_as_title.", "Android HUD / Overlay rendering flag: show_filename_as_title."}},
    {"invert_confirm_back_controller_buttons", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"thermal_emergency_protection", {ImpactLevel::Med, ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, "Monitors device battery/SoC temperature and pauses emulation if limit is exceeded.", "Monitors device battery/SoC temperature and pauses emulation if limit is exceeded."}},
    {"thermal_emergency_limit", {ImpactLevel::Low, ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, "Target temperature in Celsius for emergency thermal shutdown (default 48°C).", "Target temperature in Celsius for emergency thermal shutdown (default 48°C)."}},
    {"control_scale", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: overlay_scale.", "Android HUD / Overlay rendering flag: overlay_scale."}},
    {"control_opacity", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: overlay_opacity.", "Android HUD / Overlay rendering flag: overlay_opacity."}},
    {"overlay_skin_theme", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: overlay_skin_theme.", "Android HUD / Overlay rendering flag: overlay_skin_theme."}},
    {"joystick_rel_center", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"dpad_slide", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"haptic_feedback", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"show_performance_overlay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_performance_overlay.", "Android HUD / Overlay rendering flag: show_performance_overlay."}},
    {"enable_input_overlay_auto_hide", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: enable_input_overlay_auto_hide.", "Android HUD / Overlay rendering flag: enable_input_overlay_auto_hide."}},
    {"input_overlay_auto_hide", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: input_overlay_auto_hide.", "Android HUD / Overlay rendering flag: input_overlay_auto_hide."}},
    {"hide_overlay_on_controller_input", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: hide_overlay_on_controller_input.", "Android HUD / Overlay rendering flag: hide_overlay_on_controller_input."}},
    {"perf_overlay_background", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: perf_overlay_background.", "Android HUD / Overlay rendering flag: perf_overlay_background."}},
    {"perf_overlay_position", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: perf_overlay_position.", "Android HUD / Overlay rendering flag: perf_overlay_position."}},
    {"show_fps", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_fps.", "Android HUD / Overlay rendering flag: show_fps."}},
    {"show_frame_time", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_frame_time.", "Android HUD / Overlay rendering flag: show_frame_time."}},
    {"show_app_ram_usage", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_app_ram_usage.", "Android HUD / Overlay rendering flag: show_app_ram_usage."}},
    {"show_system_ram_usage", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_system_ram_usage.", "Android HUD / Overlay rendering flag: show_system_ram_usage."}},
    {"show_bat_temperature", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_bat_temperature.", "Android HUD / Overlay rendering flag: show_bat_temperature."}},
    {"bat_temperature_unit", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"show_power_info", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_power_info.", "Android HUD / Overlay rendering flag: show_power_info."}},
    {"show_shaders_building", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "Android HUD / Overlay rendering flag: show_shaders_building.", "Android HUD / Overlay rendering flag: show_shaders_building."}},
    {"pipeline_worker_count", {ImpactLevel::High, ImpactLevel::High, ImpactLevel::Low, ImpactLevel::Low, "Android background shader compilation worker thread count.", "Android background shader compilation worker thread count."}},
    {"show_input_overlay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_input_overlay.", "Android HUD / Overlay rendering flag: show_input_overlay."}},
    {"overlay_snap_to_grid", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: overlay_snap_to_grid.", "Android HUD / Overlay rendering flag: overlay_snap_to_grid."}},
    {"overlay_grid_size", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: overlay_grid_size.", "Android HUD / Overlay rendering flag: overlay_grid_size."}},
    {"touchscreen", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"lock_drawer", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}},
    {"show_soc_overlay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_soc_overlay.", "Android HUD / Overlay rendering flag: show_soc_overlay."}},
    {"show_build_id", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_build_id.", "Android HUD / Overlay rendering flag: show_build_id."}},
    {"show_driver_version", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_driver_version.", "Android HUD / Overlay rendering flag: show_driver_version."}},
    {"show_device_model", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_device_model.", "Android HUD / Overlay rendering flag: show_device_model."}},
    {"show_gpu_model", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_gpu_model.", "Android HUD / Overlay rendering flag: show_gpu_model."}},
    {"show_soc_model", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_soc_model.", "Android HUD / Overlay rendering flag: show_soc_model."}},
    {"show_firmware_version", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_fw_version.", "Android HUD / Overlay rendering flag: show_fw_version."}},
    {"soc_overlay_background", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: soc_overlay_background.", "Android HUD / Overlay rendering flag: soc_overlay_background."}},
    {"soc_overlay_position", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: soc_overlay_position.", "Android HUD / Overlay rendering flag: soc_overlay_position."}},
    {"show_device_load_overlay", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android HUD / Overlay rendering flag: show_device_load_overlay.", "Android HUD / Overlay rendering flag: show_device_load_overlay."}},
    {"dont_show_driver_shader_warning", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Med, "Android HUD / Overlay rendering flag: dont_show_driver_shader_warning.", "Android HUD / Overlay rendering flag: dont_show_driver_shader_warning."}},
    {"enable_quick_settings", {ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, ImpactLevel::Low, "Android specific configuration / overlay", "Android specific configuration / overlay"}}
    };
    return kMap;
}

inline const SettingImpact* GetSettingImpact(std::string_view key) {
    const auto& m = GetSettingImpactMap();
    auto it = m.find(key);
    return it != m.end() ? &it->second : nullptr;
}

inline QString FormatLevelRu(ImpactLevel lvl) {
    switch (lvl) {
    case ImpactLevel::High: return QStringLiteral("Высокий");
    case ImpactLevel::Med:  return QStringLiteral("Средний");
    case ImpactLevel::Low:  return QStringLiteral("Низкий");
    default: return QStringLiteral("—");
    }
}

inline QString FormatLevelEn(ImpactLevel lvl) {
    switch (lvl) {
    case ImpactLevel::High: return QStringLiteral("High");
    case ImpactLevel::Med:  return QStringLiteral("Medium");
    case ImpactLevel::Low:  return QStringLiteral("Low");
    default: return QStringLiteral("—");
    }
}

inline QString FormatImpactBadge(const SettingImpact& impact, bool is_russian = false) {
    return QString();
}

inline QString FormatImpactTooltip(const SettingImpact& impact, bool is_russian = false) {
    const QString effect = is_russian ? QString::fromUtf8(impact.effect_ru) : QString::fromUtf8(impact.effect_en);
    if (is_russian) {
        return QStringLiteral("<b>%1</b><br><br>"
                              "• <b>Производительность:</b> %2<br>"
                              "• <b>Нагрев:</b> %3<br>"
                              "• <b>Совместимость:</b> %4<br>"
                              "• <b>Качество:</b> %5")
            .arg(effect)
            .arg(FormatLevelRu(impact.performance))
            .arg(FormatLevelRu(impact.heat))
            .arg(FormatLevelRu(impact.compatibility))
            .arg(FormatLevelRu(impact.quality));
    } else {
        return QStringLiteral("<b>%1</b><br><br>"
                              "• <b>Performance:</b> %2<br>"
                              "• <b>Heat:</b> %3<br>"
                              "• <b>Compatibility:</b> %4<br>"
                              "• <b>Quality:</b> %5")
            .arg(effect)
            .arg(FormatLevelEn(impact.performance))
            .arg(FormatLevelEn(impact.heat))
            .arg(FormatLevelEn(impact.compatibility))
            .arg(FormatLevelEn(impact.quality));
    }
}

} // namespace ConfigurationShared
