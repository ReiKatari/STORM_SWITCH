// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include "common/logging.h"
#include "core/core.h"
#include "video_core/macro.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/hle/service/nvdrv/devices/nvhost_tsg_gpu.h"
#include "core/hle/service/nvdrv/devices/nx2_stub_device.h"

namespace Service::Nvidia::Devices {

nvhost_tsg_gpu::nvhost_tsg_gpu(Core::System& system_, Module& module_, NvCore::Container& core_)
    : nvdevice{system_, Type::GpuTsg},
      native_macros{std::make_shared<Tegra::NativeMacroContext>()},
      module{module_},
      core{core_} {
    native_macros->engine.native_turing = true;
    native_macros->compute_engine.native_turing = true;
}

nvhost_tsg_gpu::~nvhost_tsg_gpu() {
    OnClose(0);
}

void nvhost_tsg_gpu::OnOpen(NvCore::SessionId id, DeviceFD fd) {
    group_fd = fd;
    if (const auto* session = core.GetSession(id)) {
        process = session->process;
        if (process) {
            process->Open(system.Kernel());
        }
    }
}

void nvhost_tsg_gpu::OnClose(DeviceFD fd) {
    for (const auto& [channel_fd, channel] : channels) {
        channel->UnbindFromTsg(group_fd);
    }
    channels.clear();
    if (process) {
        process->Close(system.Kernel());
        process = nullptr;
    }
}

NvResult nvhost_tsg_gpu::Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                                std::span<u8> output) {
    std::array<u32, 8> words{};
    if (!input.empty()) {
        std::memcpy(words.data(), input.data(), (std::min)(input.size(), sizeof(words)));
    }
    LOG_DEBUG(Service_NVDRV,
              "native TSG ioctl={:#x} (group={:#x}, cmd={:#x}), input={}, output={}, words={:#x},{:#x},{:#x},{:#x}",
              command.raw, command.group.Value(), command.cmd.Value(), input.size(), output.size(),
              words[0], words[1], words[2], words[3]);

    if (command.group == 'T') {
        // 0x01 = NVGPU_TSG_IOCTL_BIND_CHANNEL (4 bytes: {s32 channel_fd})
        // 0x0b = NVGPU_TSG_IOCTL_BIND_CHANNEL_EX (24 bytes: {s32 channel_fd, u32 subcontext_id, ...})
        if (command.cmd == 0x01 || command.cmd == 0x0b) {
            if (!process || input.size() < sizeof(s32)) {
                return NvResult::BadParameter;
            }
            const DeviceFD channel_fd = static_cast<s32>(words[0]);
            if (!channels.contains(channel_fd)) {
                if (channels.size() >= 64) {
                    return NvResult::InsufficientMemory;
                }
                const auto device = module.GetDevice<nvdevice>(channel_fd);
                if (!device || device->GetType() != Type::GpuChannel) {
                    return NvResult::BadParameter;
                }
                const auto channel = std::static_pointer_cast<nvhost_gpu>(device);
                const auto result = channel->BindToTsg(fd, process, native_macros);
                if (result != NvResult::Success && result != NvResult::Busy) {
                    return result;
                }
                channels.emplace(channel_fd, channel);
            }
            if (!output.empty()) {
                std::fill(output.begin(), output.end(), u8{0});
                std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
            }
            return NvResult::Success;
        }
        // 0x02 = NVGPU_TSG_IOCTL_UNBIND_CHANNEL
        if (command.cmd == 0x02) {
            if (input.size() < sizeof(s32)) {
                return NvResult::BadParameter;
            }
            const DeviceFD channel_fd = static_cast<s32>(words[0]);
            if (const auto it = channels.find(channel_fd); it != channels.end()) {
                it->second->UnbindFromTsg(fd);
                channels.erase(it);
            }
            if (!output.empty()) {
                std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
            }
            return NvResult::Success;
        }
        // 0x0f = NVGPU_TSG_IOCTL_CREATE_SUBCONTEXT ({u32 type, s32 as_fd, u32 veid_out, u32 reserved})
        if (command.cmd == 0x0f) {
            std::fill(output.begin(), output.end(), u8{0});
            std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
            const u32 veid = static_cast<u32>((channels.size() + 1) & 0x3F);
            if (output.size() >= 12) {
                std::memcpy(output.data() + 8, &veid, sizeof(veid));
            }
            return NvResult::Success;
        }
        // 0x08 = NVGPU_TSG_IOCTL_SET_RUNLIST_INTERLEAVE (0xC0045408)
        // 0x04 = NVGPU_TSG_IOCTL_SET_TIMESLICE
        // 0x05 = NVGPU_TSG_IOCTL_SET_LONG_TIMESLICE
        // 0x07 = NVGPU_TSG_IOCTL_SET_INTERLEAVE
        // 0x09 = NVGPU_TSG_IOCTL_GET_RUNLIST_INTERLEAVE
        // 0x11 = NVGPU_TSG_IOCTL_SET_RUNLIST_POLICY
        if (command.cmd == 0x08 || command.cmd == 0x04 || command.cmd == 0x05 ||
            command.cmd == 0x07 || command.cmd == 0x09 || command.cmd == 0x11) {
            if (!output.empty()) {
                std::fill(output.begin(), output.end(), u8{0});
                if (!input.empty()) {
                    std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
                }
            }
            return NvResult::Success;
        }
    }

    return Nx2UnknownIoctl("nvhost-tsg-gpu", command, input, output);
}

NvResult nvhost_tsg_gpu::Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                                std::span<const u8> inline_input, std::span<u8> output) {
    if (command.group == 'T' && (command.cmd == 0x08 || command.cmd == 0x04 || command.cmd == 0x05 ||
                                 command.cmd == 0x07 || command.cmd == 0x09 || command.cmd == 0x11)) {
        if (!output.empty()) {
            std::fill(output.begin(), output.end(), u8{0});
            if (!input.empty()) {
                std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
            }
        }
        return NvResult::Success;
    }
    return Nx2UnknownIoctl("nvhost-tsg-gpu", command, input, output);
}

NvResult nvhost_tsg_gpu::Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input,
                                std::span<u8> output, std::span<u8> inline_output) {
    std::fill(inline_output.begin(), inline_output.end(), u8{0});
    if (command.group == 'T' && (command.cmd == 0x08 || command.cmd == 0x04 || command.cmd == 0x05 ||
                                 command.cmd == 0x07 || command.cmd == 0x09 || command.cmd == 0x11)) {
        if (!output.empty()) {
            std::fill(output.begin(), output.end(), u8{0});
            if (!input.empty()) {
                std::copy_n(input.begin(), (std::min)(input.size(), output.size()), output.begin());
            }
        }
        return NvResult::Success;
    }
    return Nx2UnknownIoctl("nvhost-tsg-gpu", command, input, output);
}

} // namespace Service::Nvidia::Devices
