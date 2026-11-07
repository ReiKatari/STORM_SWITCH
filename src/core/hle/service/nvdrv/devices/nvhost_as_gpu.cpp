#include "common/settings.h"
// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <utility>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/heap_mapper.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/devices/ioctl_serialization.h"
#include "core/hle/service/nvdrv/devices/gpu_address_space_layout.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"
#include <limits>
#include "core/hle/service/nvdrv/devices/native_gpu_kind.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/hle/service/nvdrv/devices/nx2_stub_device.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "video_core/control/channel_state.h"
#include "video_core/gpu.h"
#include "video_core/host1x/host1x.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace Service::Nvidia::Devices {

nvhost_as_gpu::nvhost_as_gpu(Core::System& system_, Module& module_, NvCore::Container& core)
    : nvdevice{system_, Type::GpuAddressSpace}, module{module_}, container{core}, nvmap{core.GetNvMapFile()}, vm{},
      gmmu{} {}

nvhost_as_gpu::~nvhost_as_gpu() {
    Nx2GpuMemoryRegistry::Get().RemoveListener(this);
    if (m_owner_process) {
        m_owner_process->Close(system.Kernel());
    }
}

NvResult nvhost_as_gpu::Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                               std::span<u8> output) {
    switch (command.group) {
    case 'A':
        switch (command.cmd) {
        case 0x1:
            return WrapFixed(this, &nvhost_as_gpu::BindChannel, input, output);
        case 0x2:
            return WrapFixed(this, &nvhost_as_gpu::AllocateSpace, input, output);
        case 0x3:
            return WrapFixed(this, &nvhost_as_gpu::FreeSpace, input, output);
        case 0x5:
            return WrapFixed(this, &nvhost_as_gpu::UnmapBuffer, input, output);
        case 0x6:
            return WrapFixed(this, &nvhost_as_gpu::MapBufferEx, input, output);
        case 0x8:
            return WrapFixed(this, &nvhost_as_gpu::GetVARegions1, input, output);
        case 0x9: {
            const auto result = WrapFixed(this, &nvhost_as_gpu::AllocAsEx, input, output);
            if (result == NvResult::Success && g_nx2_driver && !m_shared_parent) {
                Nx2GpuMemoryRegistry::Get().AddListener(
                    this,
                    [this](u64 gpu_address, u64 cpu_address, u64 size) {
                        Nx2Map(gpu_address, cpu_address, size);
                    },
                    [this](u64 gpu_address, u64 size) { Nx2Unmap(gpu_address, size); },
                    [this](u64 cpu_address, u64 size) { Nx2Release(cpu_address, size); },
                    [this](u64 gpu_address, u64 size, bool create) {
                        Nx2Sparse(gpu_address, size, create);
                    },
                    [this](u64 gpu_address, u64 cpu_address, u64 size) {
                        Nx2BackSparse(gpu_address, cpu_address, size);
                    });
            }
            return result;
        }
        case 0xb:
            if (g_nx2_driver) {
                return WrapFixed(this, &nvhost_as_gpu::Nx2GetSyncpointShimInfo, input, output);
            }
            return WrapFixed(this, &nvhost_as_gpu::GetSyncpointShimInfo, input, output);
        case 0xc:
            return WrapFixed(this, &nvhost_as_gpu::ModifyKind, input, output);
        case 0x14:
            return WrapVariable(this, &nvhost_as_gpu::Remap, input, output);
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (g_nx2_driver || Settings::IsCurrentProgramSwitch2()) {
        return Nx2UnknownIoctl("nvhost-as-gpu", command, input, output);
    }
    LOG_WARNING(Service_NVDRV, "Unimplemented nvhost-as-gpu ioctl={:08X}", command.raw);
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_as_gpu::Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                               std::span<const u8> inline_input, std::span<u8> output) {
    if (g_nx2_driver || Settings::IsCurrentProgramSwitch2()) {
        return Nx2UnknownIoctl("nvhost-as-gpu", command, input, output);
    }
    LOG_WARNING(Service_NVDRV, "Unimplemented nvhost-as-gpu ioctl={:08X}", command.raw);
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_as_gpu::Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input,
                               std::span<u8> output, std::span<u8> inline_output) {
    switch (command.group) {
    case 'A':
        switch (command.cmd) {
        case 0x8:
            return WrapFixedInlOut(this, &nvhost_as_gpu::GetVARegions3, input, output,
                                   inline_output);
        default:
            break;
        }
        break;
    default:
        break;
    }
    if (g_nx2_driver || Settings::IsCurrentProgramSwitch2()) {
        std::fill(inline_output.begin(), inline_output.end(), u8{0});
        return Nx2UnknownIoctl("nvhost-as-gpu", command, input, output);
    }
    LOG_WARNING(Service_NVDRV, "Unimplemented nvhost-as-gpu ioctl3={:08X}", command.raw);
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_as_gpu::OnOpen(NvCore::SessionId session_id, DeviceFD fd) {
    m_fd = fd;
    nx2_session = session_id;
    auto* session = container.GetSession(session_id);
    m_owner_process = session ? session->process : nullptr;
    if (m_owner_process) {
        m_owner_process->Open(system.Kernel());
        m_extended_abi = m_owner_process->UsesPAuthReturns();
    }
}

void nvhost_as_gpu::Nx2Map(u64 gpu_address, u64 cpu_address, u64 size) {
    std::scoped_lock lock(mutex);
    auto* session = container.GetSession(nx2_session);
    if (!gmmu || session == nullptr) {
        LOG_ERROR(Service_NVDRV, "Switch 2 GPU mapping {:#x}+{:#x}: no address space or session",
                  gpu_address, size);
        return;
    }
    // The same CPU range is often mapped twice (at its own address and in a sparse region), so
    // reuse its SMMU mapping.
    DAddr device_address{};
    bool heap = false;
    if (const auto it = nx2_device_ranges.find({cpu_address, size}); it != nx2_device_ranges.end()) {
        device_address = it->second.address;
        heap = it->second.heap;
    } else if (session->has_preallocated_area && session->mapper->IsInBounds(cpu_address, size)) {
        device_address = session->mapper->Map(cpu_address, size);
        heap = true;
    } else {
        auto& smmu = system.Host1x().MemoryManager();
        device_address = smmu.Allocate(Common::AlignUp(size, 0x20000));
        if (device_address == 0) {
            LOG_CRITICAL(Service_NVDRV, "Switch 2 GPU memory {:#x}+{:#x}: out of SMMU space",
                         cpu_address, size);
            return;
        }
        smmu.Map(device_address, cpu_address, size, session->asid, true);
    }
    nx2_device_ranges[{cpu_address, size}] = {device_address, heap};
    Nx2GpuMemoryRegistry::Get().RecordDeviceRange(cpu_address, size, device_address);
    gmmu->Map(gpu_address, device_address, size, Tegra::PTEKind::PITCH, false);
    LOG_DEBUG(Service_NVDRV, "Switch 2 GPU mapping {:#x}+{:#x} -> CPU {:#x}", gpu_address, size,
              cpu_address);
}

void nvhost_as_gpu::Nx2Unmap(u64 gpu_address, u64 size) {
    std::scoped_lock lock(mutex);
    if (!gmmu) {
        return;
    }
    // The GPU thread may still be running command lists that use the range.
    system.GPU().RunInOrder([gmmu = gmmu.get(), gpu_address, size] { gmmu->Unmap(gpu_address, size); });
    LOG_DEBUG(Service_NVDRV, "Switch 2 GPU unmapping {:#x}+{:#x}", gpu_address, size);
}

void nvhost_as_gpu::Nx2Release(u64 cpu_address, u64 size) {
    std::scoped_lock lock(mutex);
    const auto it = nx2_device_ranges.find({cpu_address, size});
    if (it == nx2_device_ranges.end()) {
        return;
    }
    const Nx2DeviceRange range = it->second;
    nx2_device_ranges.erase(it);
    auto* session = container.GetSession(nx2_session);
    auto* const mapper = range.heap && session != nullptr ? session->mapper.get() : nullptr;
    auto& smmu = system.Host1x().MemoryManager();
    // Queued after the GPU unmaps of the range, so the GPU thread is done with it.
    system.GPU().RunInOrder([&smmu, mapper, range, cpu_address, size] {
        if (range.heap) {
            if (mapper != nullptr) {
                mapper->Unmap(cpu_address, size);
            }
            return;
        }
        smmu.Unmap(range.address, size);
        smmu.Free(range.address, Common::AlignUp(size, 0x20000));
    });
    LOG_DEBUG(Service_NVDRV, "Switch 2 released backing {:#x}+{:#x}", cpu_address, size);
}

void nvhost_as_gpu::Nx2Sparse(u64 gpu_address, u64 size, bool create) {
    std::scoped_lock lock(mutex);
    if (!gmmu) {
        return;
    }
    auto& smmu = system.Host1x().MemoryManager();
    const u64 device_size = Common::AlignUp(size, 0x20000);
    if (!create) {
        const auto it = nx2_sparse_ranges.find(gpu_address);
        if (it == nx2_sparse_ranges.end()) {
            return;
        }
        const DAddr device_address = it->second.second;
        nx2_sparse_ranges.erase(it);
        // After the command lists that may still use it.
        system.GPU().RunInOrder([gmmu = gmmu.get(), &smmu, gpu_address, size, device_address,
                                 device_size] {
            gmmu->Unmap(gpu_address, size);
            smmu.Unmap(device_address, size);
            smmu.Free(device_address, device_size);
        });
        return;
    }
    const DAddr device_address = smmu.Allocate(device_size);
    if (device_address == 0) {
        LOG_CRITICAL(Service_NVDRV, "Switch 2 sparse region {:#x}+{:#x}: out of SMMU space",
                     gpu_address, size);
        return;
    }
    nx2_sparse_ranges[gpu_address] = {size, device_address};
    gmmu->Map(gpu_address, device_address, size, Tegra::PTEKind::PITCH, false);
    LOG_DEBUG(Service_NVDRV, "Switch 2 sparse region {:#x}+{:#x} -> device {:#x}", gpu_address,
              size, device_address);
}

void nvhost_as_gpu::Nx2BackSparse(u64 gpu_address, u64 cpu_address, u64 size) {
    std::scoped_lock lock(mutex);
    auto it = nx2_sparse_ranges.upper_bound(gpu_address);
    auto* session = container.GetSession(nx2_session);
    if (it == nx2_sparse_ranges.begin() || session == nullptr) {
        return;
    }
    --it;
    const auto& [start, range] = *it;
    const auto& [region_size, device_address] = range;
    if (gpu_address + size > start + region_size) {
        return;
    }
    if (cpu_address == 0) {
        system.Host1x().MemoryManager().Unmap(device_address + (gpu_address - start), size);
        return;
    }
    system.Host1x().MemoryManager().Map(device_address + (gpu_address - start), cpu_address, size,
                                        session->asid, true);
}

void nvhost_as_gpu::OnClose(DeviceFD fd) {}

NvResult nvhost_as_gpu::MapNative4K(u32 handle_id, u64 address, u64 buffer_offset, u64 size, u8 kind) {
    if (!m_extended_abi || !size || !Common::IsAligned(address, VM::YUZU_PAGESIZE) ||
        !Common::IsAligned(size, VM::YUZU_PAGESIZE) ||
        !Common::IsAligned(buffer_offset, VM::YUZU_PAGESIZE)) {
        return NvResult::BadParameter;
    }
    const auto handle = nvmap.GetHandle(handle_id);
    if (!handle || buffer_offset > handle->orig_size || size > handle->orig_size - buffer_offset) {
        return NvResult::BadParameter;
    }
    {
        const std::scoped_lock lock{mutex};
        if (!vm.initialised || address < vm.va_range_start || address >= vm.va_range_split ||
            size > vm.va_range_split - address) {
            return NvResult::BadParameter;
        }
        const auto next = allocation_map.lower_bound(address);
        if ((next != allocation_map.end() && next->first < address + size) ||
            (next != allocation_map.begin() &&
             std::prev(next)->first + std::prev(next)->second.size > address)) {
            return NvResult::BadParameter;
        }
    }
    IoctlAllocSpace reservation{};
    reservation.pages = static_cast<u32>(size >> VM::PAGE_SIZE_BITS);
    reservation.page_size = VM::YUZU_PAGESIZE;
    reservation.flags = MappingFlags::Fixed;
    reservation.offset = address;
    const auto reserved = AllocateSpace(reservation);
    if (reserved != NvResult::Success) { return reserved; }
    IoctlMapBufferEx mapping{};
    mapping.flags = MappingFlags::Fixed;
    mapping.kind = kind;
    mapping.handle = handle_id;
    mapping.page_size = VM::YUZU_PAGESIZE;
    mapping.buffer_offset = static_cast<s64>(buffer_offset);
    mapping.mapping_size = size;
    mapping.offset = static_cast<s64>(address);
    const auto result = MapBufferEx(mapping);
    if (result != NvResult::Success) {
        nvmap.UnpinHandle(handle_id);
        IoctlFreeSpace release{};
        release.offset = address;
        release.pages = reservation.pages;
        release.page_size = VM::YUZU_PAGESIZE;
        FreeSpace(release);
    }
    return result;
}

NvResult nvhost_as_gpu::UnmapNative4K(u64 address, u64 size) {
    if (!m_extended_abi || !size || !Common::IsAligned(address, VM::YUZU_PAGESIZE) ||
        !Common::IsAligned(size, VM::YUZU_PAGESIZE)) {
        return NvResult::BadParameter;
    }
    IoctlFreeSpace release{};
    release.offset = address;
    release.pages = static_cast<u32>(size >> VM::PAGE_SIZE_BITS);
    release.page_size = VM::YUZU_PAGESIZE;
    return FreeSpace(release);
}

NvResult nvhost_as_gpu::GetNativeSyncpointShim(u64& address, u32& stride, u32& count) {
    if (!m_extended_abi || !vm.initialised || !gmmu) { return NvResult::NotSupported; }
    address = vm.va_range_split - VM::YUZU_PAGESIZE;
    if (!m_syncpoint_shim_reserved) {
        IoctlAllocSpace allocation{};
        allocation.pages = 1;
        allocation.page_size = VM::YUZU_PAGESIZE;
        allocation.flags = MappingFlags::Fixed;
        allocation.offset = address;
        const auto result = AllocateSpace(allocation);
        if (result != NvResult::Success) { return result; }
        m_syncpoint_shim_reserved = true;
        gmmu->SetNativeSyncpointShim(address);
    }
    stride = sizeof(u32);
    count = 192;
    return NvResult::Success;
}

NvResult nvhost_as_gpu::GetSyncpointShimInfo(IoctlSyncpointShimInfo& params) {
    u64 address{};
    u32 stride{}, count{};
    const auto result = GetNativeSyncpointShim(address, stride, count);
    params.address = address;
    params.stride = stride;
    params.count = count;
    LOG_DEBUG(Service_NVDRV, "native syncpoint shim address={:#x}, stride={}, count={}, result={}",
              address, stride, count, static_cast<u32>(result));
    return result;
}

NvResult nvhost_as_gpu::ModifyNativeKind(u64 address, u64 size, u8 requested, u8 fallback) {
    IoctlModifyKind params{};
    params.compressed_kind = requested;
    params.uncompressed_kind = fallback;
    params.mapping_base = address;
    params.size = size;
    return ModifyKind(params);
}

NvResult nvhost_as_gpu::ModifyKind(IoctlModifyKind& params) {
    const u32 requested = params.compressed_kind;
    const u32 fallback = params.uncompressed_kind;
    const u64 base = params.mapping_base;
    const u64 offset = params.buffer_offset;
    const u64 size = params.size;
    LOG_DEBUG(Service_NVDRV, "native modify kind base={:#x}, offset={:#x}, size={:#x}, kinds={:#x}/{:#x}",
              base, offset, size, requested, fallback);
    if (!m_extended_abi || params.reserved != 0 || fallback > 255 ||
        (requested > 255 && requested != 0xffff) || !size ||
        offset > std::numeric_limits<u64>::max() - base ||
        !Common::IsAligned(base + offset, VM::YUZU_PAGESIZE) ||
        !Common::IsAligned(size, VM::YUZU_PAGESIZE)) {
        return NvResult::BadParameter;
    }
    const auto kind = SelectNativeMemoryKind(static_cast<u8>(requested == 0xffff ? fallback : requested),
                                             static_cast<u8>(fallback));
    if (!kind) { return NvResult::NotSupported; }
    const std::scoped_lock lock{mutex};
    const u64 address = base + offset;
    if (!vm.initialised || !gmmu) { return NvResult::BadParameter; }
    auto it = mapping_map.upper_bound(address);
    if (it != mapping_map.begin()) {
        --it;
        const auto& mapping = it->second;
        const u64 delta = address - it->first;
        if (delta < mapping.size && size <= mapping.size - delta) {
            gmmu->Map(address, mapping.ptr + delta, size, static_cast<Tegra::PTEKind>(kind->backend),
                      mapping.big_page);
            return NvResult::Success;
        }
    }
    if (const auto daddr = gmmu->GpuToCpuAddress(address)) {
        gmmu->Map(address, *daddr, size, static_cast<Tegra::PTEKind>(kind->backend), false);
        return NvResult::Success;
    }
    return NvResult::BadParameter;
}

// Switch 2 NvRm (with characteristics flag 26) asks for the read-only syncpoint shim: a GPU page
// whose u32 at id * stride reads as that syncpoint's value; NVN waits on syncpoints through it and
// asserts without one. Same scheme as changes.patch, placed on the last page of the VA range.
NvResult nvhost_as_gpu::Nx2GetSyncpointShimInfo(IoctlNx2SyncpointShimInfo& params) {
    std::scoped_lock lock(mutex);
    if (!vm.initialised || !gmmu) {
        return NvResult::NotSupported;
    }
    const u64 address = vm.va_range_end - VM::YUZU_PAGESIZE;
    if (!nx2_syncpoint_shim) {
        nx2_syncpoint_shim = true;
        gmmu->SetNativeSyncpointShim(address);
    }
    params.address = address;
    params.stride = sizeof(u32);
    params.count = 192;
    LOG_INFO(Service_NVDRV, "Switch 2 syncpoint shim at {:#x} ({} syncpoints)", address,
             params.count);
    return NvResult::Success;
}

NvResult nvhost_as_gpu::AllocAsEx(IoctlAllocAsEx& params) {
    LOG_DEBUG(Service_NVDRV,
              "called, flags={:#x} as_fd={} big_page_size={:#x} va_range_start={:#x} va_range_end={:#x} va_range_split={:#x}",
              params.flags, params.as_fd, params.big_page_size, params.va_range_start,
              params.va_range_end, params.va_range_split);

    std::scoped_lock lock(mutex);

    if (vm.initialised) {
        ASSERT_MSG(false, "Cannot initialise an address space twice!");
        return NvResult::InvalidState;
    }

    const bool extended = m_extended_abi;
    if (extended) {
        LOG_DEBUG(Service_NVDRV,
                  "Switch 2 AllocAsEx: fd={} flags={:#x} as_fd={} big_page_size={:#x} VA=[{:#x}, {:#x}] split={:#x}",
                 m_fd, params.flags, params.as_fd, params.big_page_size,
                 params.va_range_start, params.va_range_end, params.va_range_split);
    }
    std::shared_ptr<nvhost_as_gpu> shared_parent;
    if (extended && params.as_fd > 0 && params.as_fd != m_fd) {
        const auto device = module.GetDevice<nvdevice>(params.as_fd);
        if (device && device->GetType() == Type::GpuAddressSpace) {
            auto candidate = std::static_pointer_cast<nvhost_as_gpu>(device);
            if (candidate->vm.initialised) {
                shared_parent = std::move(candidate);
            }
        }
    }

    const u32 big_page_size = params.big_page_size;
    if (big_page_size) {
        if (!std::has_single_bit(big_page_size)) {
            LOG_ERROR(Service_NVDRV, "Non power-of-2 big page size: {:#x}!", big_page_size);
            return NvResult::BadValue;
        }

        if ((big_page_size & VM::SUPPORTED_BIG_PAGE_SIZES) == 0) {
            LOG_ERROR(Service_NVDRV, "Unsupported big page size: {:#x}!", big_page_size);
            return NvResult::BadValue;
        }

        vm.big_page_size = big_page_size;
        vm.big_page_size_bits = static_cast<u32>(std::countr_zero(big_page_size));

        vm.va_range_start = static_cast<u64>(big_page_size) << VM::VA_START_SHIFT;
    }

    if (extended && !params.va_range_start) {
        const auto layout = GetGpuAddressSpaceLayout(true);
        vm.va_range_start = layout.start;
        vm.va_range_split = layout.split;
        vm.va_range_end = layout.end;
        vm.absolute_big_page_limit = true;
    }

    // If this is unspecified then default values should be used
    if (params.va_range_start) {
        vm.va_range_start = params.va_range_start;
        vm.va_range_split = params.va_range_split;
        vm.va_range_end = params.va_range_end;
        if (shared_parent) {
            vm.va_range_split = params.va_range_end;
            vm.va_range_end = (std::max<u64>)(params.va_range_end + (u64{4} << 30), shared_parent->vm.va_range_end);
            vm.absolute_big_page_limit = true;
        } else if (vm.va_range_split == 0) {
            vm.va_range_split = vm.va_range_end;
            vm.va_range_end += 4ULL << 30;
            vm.absolute_big_page_limit = true;
            LOG_WARNING(Service_NVDRV, "No VA split given; small pages up to {:#x}", vm.va_range_split);
        } else if (extended) {
            vm.absolute_big_page_limit = true;
        }
    }

    const u64 max_big_page_bits = Common::Log2Ceil(vm.va_range_end);

    const auto start_pages{static_cast<u32>(vm.va_range_start >> VM::PAGE_SIZE_BITS)};
    const auto end_pages{static_cast<u32>(vm.va_range_split >> VM::PAGE_SIZE_BITS)};
    vm.small_page_allocator.emplace(start_pages, end_pages);

    const auto start_big_pages{static_cast<u32>(vm.va_range_split >> vm.big_page_size_bits)};
    const auto end_big_pages{static_cast<u32>(
        (vm.absolute_big_page_limit ? vm.va_range_end : vm.va_range_end - vm.va_range_split) >>
        vm.big_page_size_bits)};
    vm.big_page_allocator.emplace(start_big_pages, end_big_pages);

    if (shared_parent) {
        m_shared_parent = std::move(shared_parent);
        gmmu = m_shared_parent->gmmu;
        LOG_DEBUG(Service_NVDRV, "native AS view fd={} shares page table with fd={}", m_fd, params.as_fd);
    } else {
        gmmu = std::make_shared<Tegra::MemoryManager>(system, max_big_page_bits, vm.va_range_split,
                                                      vm.big_page_size_bits, VM::PAGE_SIZE_BITS);
        if (extended) {
            gmmu->EnableNativeTiling();
        }
        system.GPU().InitAddressSpace(*gmmu);
    }
    vm.initialised = true;

    return NvResult::Success;
}

NvResult nvhost_as_gpu::AllocateSpace(IoctlAllocSpace& params) {
    LOG_DEBUG(Service_NVDRV, "called, pages={:X}, page_size={:X}, flags={:X}", params.pages,
              params.page_size, params.flags);

    std::scoped_lock lock(mutex);

    if (!vm.initialised) {
        return NvResult::BadValue;
    }

    if (params.page_size != VM::YUZU_PAGESIZE && params.page_size != vm.big_page_size) {
        return NvResult::BadValue;
    }

    if (!m_extended_abi &&
        params.page_size != vm.big_page_size &&
        ((params.flags & MappingFlags::Sparse) != MappingFlags::None)) {
        LOG_WARNING(Service_NVDRV, "Sparse small pages are not implemented!");
        return NvResult::NotImplemented;
    }

    const u32 page_size_bits{params.page_size == VM::YUZU_PAGESIZE ? VM::PAGE_SIZE_BITS
                                                                   : vm.big_page_size_bits};

    auto& allocator{params.page_size == VM::YUZU_PAGESIZE ? *vm.small_page_allocator
                                                          : *vm.big_page_allocator};

    if ((params.flags & MappingFlags::Fixed) != MappingFlags::None) {
        allocator.AllocateFixed(static_cast<u32>(params.offset >> page_size_bits), params.pages);
    } else {
        params.offset = static_cast<u64>(allocator.Allocate(params.pages)) << page_size_bits;
        if (!params.offset) {
            ASSERT_MSG(false, "Failed to allocate free space in the GPU AS!");
            return NvResult::InsufficientMemory;
        }
    }

    u64 size{static_cast<u64>(params.pages) * params.page_size};

    if ((params.flags & MappingFlags::Sparse) != MappingFlags::None) {
        gmmu->MapSparse(params.offset, size);
    }

    allocation_map[params.offset] = {
        .mappings{},
        .size = size,
        .page_size = params.page_size,
        .sparse = (params.flags & MappingFlags::Sparse) != MappingFlags::None,
        .big_pages = params.page_size != VM::YUZU_PAGESIZE,
    };

    return NvResult::Success;
}

bool nvhost_as_gpu::FreeMappingLocked(u64 offset) noexcept {
    if (auto const it = mapping_map.find(offset); it != mapping_map.end()) {
        auto const mapping = it->second;
        if (!mapping.fixed) {
            auto& allocator{mapping.big_page ? *vm.big_page_allocator : *vm.small_page_allocator};
            u32 page_size_bits{mapping.big_page ? vm.big_page_size_bits : VM::PAGE_SIZE_BITS};
            u32 page_size{mapping.big_page ? vm.big_page_size : VM::YUZU_PAGESIZE};
            u64 aligned_size{Common::AlignUp(mapping.size, page_size)};
            allocator.Free(u32(mapping.offset >> page_size_bits), u32(aligned_size >> page_size_bits));
        }
        nvmap.UnpinHandle(mapping.handle);
        // Sparse mappings shouldn't be fully unmapped, just returned to their sparse state
        // Only FreeSpace can unmap them fully
        if (mapping.sparse_alloc) {
            gmmu->MapSparse(offset, mapping.size, mapping.big_page);
        } else {
            gmmu->Unmap(offset, mapping.size);
        }
        mapping_map.erase(offset);
        return true;
    }
    return false;
}

NvResult nvhost_as_gpu::FreeSpace(IoctlFreeSpace& params) {
    LOG_DEBUG(Service_NVDRV, "called, offset={:X}, pages={:X}, page_size={:X}", params.offset, params.pages, params.page_size);
    std::scoped_lock lock(mutex);
    if (!vm.initialised) {
        return NvResult::BadValue;
    }
    if (auto const it = allocation_map.find(params.offset); it != allocation_map.end()) {
        auto const allocation = it->second;
        if (allocation.page_size != params.page_size || allocation.size != (u64(params.pages) * params.page_size))
            return NvResult::BadValue;

        for (const auto mapping_offset : allocation.mappings)
            if (!FreeMappingLocked(mapping_offset))
                return NvResult::BadValue;

        // Unset sparse flag if required
        if (allocation.sparse)
            gmmu->Unmap(params.offset, allocation.size);

        auto& allocator{params.page_size == VM::YUZU_PAGESIZE ? *vm.small_page_allocator : *vm.big_page_allocator};
        u32 page_size_bits{params.page_size == VM::YUZU_PAGESIZE ? VM::PAGE_SIZE_BITS : vm.big_page_size_bits};

        allocator.Free(u32(params.offset >> page_size_bits), u32(allocation.size >> page_size_bits));
        allocation_map.erase(params.offset);
        return NvResult::Success;
    }
    return NvResult::BadValue;
}

NvResult nvhost_as_gpu::Remap(std::span<IoctlRemapEntry> entries) {
    LOG_DEBUG(Service_NVDRV, "called, num_entries={:#x}", entries.size());

    if (!vm.initialised) {
        return NvResult::BadValue;
    }

    for (const auto& entry : entries) {
        GPUVAddr virtual_address{static_cast<u64>(entry.as_offset_big_pages)
                                 << vm.big_page_size_bits};
        u64 size{static_cast<u64>(entry.big_pages) << vm.big_page_size_bits};

        auto alloc{allocation_map.upper_bound(virtual_address)};

        if (alloc-- == allocation_map.begin() ||
            (virtual_address - alloc->first) + size > alloc->second.size) {
            LOG_WARNING(Service_NVDRV, "Cannot remap into an unallocated region!");
            return NvResult::BadValue;
        }

        if (!alloc->second.sparse) {
            LOG_WARNING(Service_NVDRV, "Cannot remap a non-sparse mapping!");
            return NvResult::BadValue;
        }

        const bool use_big_pages = alloc->second.big_pages;
        if (!entry.handle) {
            gmmu->MapSparse(virtual_address, size, use_big_pages);
        } else {
            auto handle{nvmap.GetHandle(entry.handle)};
            if (!handle) {
                return NvResult::BadValue;
            }

            DAddr base = nvmap.PinHandle(entry.handle, false);
            DAddr device_address{static_cast<DAddr>(
                base + (static_cast<u64>(entry.handle_offset_big_pages) << vm.big_page_size_bits))};

            gmmu->Map(virtual_address, device_address, size,
                      static_cast<Tegra::PTEKind>(entry.kind), use_big_pages);
        }
    }

    return NvResult::Success;
}

NvResult nvhost_as_gpu::MapBufferEx(IoctlMapBufferEx& params) {
    LOG_DEBUG(Service_NVDRV,
              "called, flags={:X}, nvmap_handle={:X}, buffer_offset={}, mapping_size={}"
              ", offset={:#x}",
              params.flags, params.handle, params.buffer_offset, params.mapping_size,
              params.offset);

    std::scoped_lock lock(mutex);

    if (!vm.initialised) {
        return NvResult::BadValue;
    }

    // Remaps a subregion of an existing mapping to a different PA
    if ((params.flags & MappingFlags::Remap) != MappingFlags::None) {
        if (auto const it = mapping_map.find(params.offset); it != mapping_map.end()) {
            auto const mapping = it->second;
            if (mapping.size < params.mapping_size) {
                LOG_WARNING(Service_NVDRV, "Cannot remap a partially mapped GPU address space region: {:#x}", params.offset);
                return NvResult::BadValue;
            }
            u64 gpu_address = u64(params.offset + params.buffer_offset);
            VAddr device_address{mapping.ptr + params.buffer_offset};
            gmmu->Map(gpu_address, device_address, params.mapping_size, Tegra::PTEKind(params.kind), mapping.big_page);
            return NvResult::Success;
        } else {
            LOG_WARNING(Service_NVDRV, "Cannot remap an unmapped GPU address space region: {:#x}", params.offset);
            return NvResult::BadValue;
        }
    }

    auto handle{nvmap.GetHandle(params.handle)};
    if (!handle) {
        return NvResult::BadValue;
    }

    DAddr device_address = DAddr(nvmap.PinHandle(params.handle, false) + params.buffer_offset);
    u64 size{params.mapping_size ? params.mapping_size : handle->orig_size};

    bool big_page{[&]() {
        if (Common::IsAligned(handle->align, vm.big_page_size)) {
            return true;
        } else if (Common::IsAligned(handle->align, VM::YUZU_PAGESIZE)) {
            return false;
        } else {
            LOG_WARNING(Service_NVDRV, "Handle alignment {:#x} is not a multiple of big_page_size or YUZU_PAGESIZE", handle->align);
            return false;
        }
    }()};

    if ((params.flags & MappingFlags::Fixed) != MappingFlags::None) {
        auto alloc{allocation_map.upper_bound(params.offset)};

        if (alloc-- == allocation_map.begin() ||
            (params.offset - alloc->first) + size > alloc->second.size) {
            ASSERT_MSG(false, "Cannot perform a fixed mapping into an unallocated region!");
            return NvResult::BadValue;
        }

        const bool use_big_pages = alloc->second.big_pages && big_page;
        gmmu->Map(params.offset, device_address, size, static_cast<Tegra::PTEKind>(params.kind), use_big_pages);

        alloc->second.mappings.push_back(params.offset);
        mapping_map.insert_or_assign(params.offset, Mapping(params.handle, device_address, params.offset, size, true, use_big_pages, alloc->second.sparse));
    } else {
        auto& allocator{big_page ? *vm.big_page_allocator : *vm.small_page_allocator};
        u32 page_size{big_page ? vm.big_page_size : VM::YUZU_PAGESIZE};
        u32 page_size_bits{big_page ? vm.big_page_size_bits : VM::PAGE_SIZE_BITS};

        params.offset = u64(allocator.Allocate(u32(Common::AlignUp(size, page_size) >> page_size_bits))) << page_size_bits;
        if (!params.offset) {
            ASSERT_MSG(false, "Failed to allocate free space in the GPU AS!");
            return NvResult::InsufficientMemory;
        }
        gmmu->Map(params.offset, device_address, Common::AlignUp(size, page_size), Tegra::PTEKind(params.kind), big_page);
        mapping_map.insert_or_assign(params.offset, Mapping(params.handle, device_address, params.offset, size, false, big_page, false));
    }

    map_buffer_offsets.insert(params.offset);

    return NvResult::Success;
}

NvResult nvhost_as_gpu::UnmapBuffer(IoctlUnmapBuffer& params) {
    std::scoped_lock lock(mutex);
    if (auto const offset_it = map_buffer_offsets.find(params.offset); offset_it != map_buffer_offsets.end()) {
        LOG_DEBUG(Service_NVDRV, "called, offset={:#x}", params.offset);
        if (!vm.initialised) {
            return NvResult::BadValue;
        }

        auto const it = mapping_map.find(params.offset);
        auto const mapping = it->second;
        if (!mapping.fixed) {
            auto& allocator{mapping.big_page ? *vm.big_page_allocator : *vm.small_page_allocator};
            u32 page_size_bits{mapping.big_page ? vm.big_page_size_bits : VM::PAGE_SIZE_BITS};
            allocator.Free(u32(mapping.offset >> page_size_bits), u32(mapping.size >> page_size_bits));
        }

        // Sparse mappings shouldn't be fully unmapped, just returned to their sparse state
        // Only FreeSpace can unmap them fully
        if (mapping.sparse_alloc) {
            gmmu->MapSparse(params.offset, mapping.size, mapping.big_page);
        } else {
            gmmu->Unmap(params.offset, mapping.size);
        }

        nvmap.UnpinHandle(mapping.handle);
        mapping_map.erase(params.offset);
        map_buffer_offsets.erase(params.offset);
    }
    return NvResult::Success;
}

NvResult nvhost_as_gpu::BindChannel(IoctlBindChannel& params) {
    LOG_DEBUG(Service_NVDRV, "called, fd={:X}", params.fd);

    auto gpu_channel_device = module.GetDevice<nvhost_gpu>(params.fd);
    if (!gpu_channel_device || !vm.initialised || !gmmu) {
        return NvResult::InvalidState;
    }
    gpu_channel_device->channel_state->memory_manager_owner = gmmu;
    gpu_channel_device->channel_state->memory_manager = gmmu.get();
    return NvResult::Success;
}

void nvhost_as_gpu::GetVARegionsImpl(IoctlGetVaRegions& params) {
    params.buf_size = 2 * sizeof(VaRegion);

    params.regions = std::array<VaRegion, 2>{
        VaRegion{
            .offset = GpuPageAddress(vm.small_page_allocator->GetVAStart(), VM::PAGE_SIZE_BITS),
            .page_size = VM::YUZU_PAGESIZE,
            ._pad0_{},
            .pages = vm.small_page_allocator->GetVALimit() - vm.small_page_allocator->GetVAStart(),
        },
        VaRegion{
            .offset = GpuPageAddress(vm.big_page_allocator->GetVAStart(), vm.big_page_size_bits),
            .page_size = vm.big_page_size,
            ._pad0_{},
            .pages = vm.big_page_allocator->GetVALimit() - vm.big_page_allocator->GetVAStart(),
        },
    };
}

NvResult nvhost_as_gpu::GetVARegions1(IoctlGetVaRegions& params) {
    LOG_DEBUG(Service_NVDRV, "called, buf_addr={:X}, buf_size={:X}", params.buf_addr, params.buf_size);

    std::scoped_lock lock(mutex);

    if (!vm.initialised) {
        return NvResult::BadValue;
    }

    GetVARegionsImpl(params);

    return NvResult::Success;
}

NvResult nvhost_as_gpu::GetVARegions3(IoctlGetVaRegions& params, std::span<VaRegion> regions) {
    LOG_DEBUG(Service_NVDRV, "called, buf_addr={:X}, buf_size={:X}", params.buf_addr,
              params.buf_size);

    std::scoped_lock lock(mutex);

    if (!vm.initialised) {
        return NvResult::BadValue;
    }

    GetVARegionsImpl(params);

    const size_t num_regions = (std::min)(params.regions.size(), regions.size());
    for (size_t i = 0; i < num_regions; i++) {
        regions[i] = params.regions[i];
    }

    return NvResult::Success;
}

Kernel::KEvent* nvhost_as_gpu::QueryEvent(u32 event_id) {
    LOG_CRITICAL(Service_NVDRV, "Unknown AS GPU Event {}", event_id);
    return nullptr;
}

} // namespace Service::Nvidia::Devices
