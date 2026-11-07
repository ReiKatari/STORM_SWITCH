// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>
#include "common/swap.h"
#include "common/alignment.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/core.h"
#include "core/hle/kernel/k_client_session.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/k_system_resource.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_group.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/service/am/service/graphics_session.h"
#include "core/hle/service/am/service/gpu_session.h"
#include "core/hle/service/am/service/gpu_memory_registration.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"
#include "core/hle/service/nvdrv/devices/native_gpu_kind.h"
#include "core/hle/service/nvdrv/devices/nx2_stub_device.h"
#include "video_core/nx2.h"

namespace Service::AM {
struct GpuDeviceRegistry {
    GpuDeviceRegistry(Core::System& system, std::shared_ptr<Nvidia::Module> module_, Kernel::KProcess* process_)
        : context{system, "GpuDevices"}, module{std::move(module_)}, process{process_} {
        process->Open(context.kernel);
    }
    ~GpuDeviceRegistry() {
        for (const auto& fd : address_spaces) { if (fd) { module->Close(*fd); } }
        if (nv_session) { module->GetContainer().CloseSession(Nvidia::NvCore::SessionId{*nv_session}); }
        process->Close(context.kernel);
        for (auto* event : events) { context.CloseEvent(event); }
    }
    KernelHelpers::ServiceContext context;
    std::array<Kernel::KEvent*, 64> events{};
    struct Device { u64 name; u64 debug_label; };
    std::mutex mutex;
    std::shared_ptr<Nvidia::Module> module;
    Kernel::KProcess* process;
    std::array<std::optional<s32>, 8> address_spaces{};
    std::array<bool, 8> address_space_registered{};
    std::optional<size_t> nv_session;
    std::array<std::optional<Device>, 256> devices{};
};
namespace {
class IGpuWorker final : public ServiceFramework<IGpuWorker> {
public:
    IGpuWorker(Core::System& system, std::shared_ptr<Nvidia::Module> module,
               Kernel::KProcess* process, std::shared_ptr<GpuDeviceRegistry> devices)
        : ServiceFramework{system, "IGpuWorker"}, m_devices{std::move(devices)},
          m_module{std::move(module)}, m_process{process} {
        m_process->Open(kernel);
        static const FunctionInfo functions[] = {
            {101, &IGpuWorker::CreateGpuAccessibleMemory, "CreateGpuAccessibleMemory"},
            {102, &IGpuWorker::DestroyGpuAccessibleMemory, "DestroyGpuAccessibleMemory"},
            {211, &IGpuWorker::NormalMapGpuMemory4K, "NormalMapGpuMemory4K"},
            {212, &IGpuWorker::NormalModifyGpuMemory4K, "NormalModifyGpuMemory4K"},
            {213, &IGpuWorker::NormalUnmapGpuMemory4K, "NormalUnmapGpuMemory4K"},
            {214, &IGpuWorker::CreateGpuOnlyRegion, "CreateGpuOnlyRegion"},
            {215, &IGpuWorker::DestroyGpuOnlyRegion, "DestroyGpuOnlyRegion"},
            {216, &IGpuWorker::ExecuteSparseMapRequests, "ExecuteSparseMapRequests"},
        };
        RegisterHandlersTipc(functions);
    }
    ~IGpuWorker() override {
        for (const auto& mapping : m_mappings) {
            if (auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(mapping.as_id)) {
                as->UnmapNative4K(mapping.address, mapping.size);
            }
        }
        auto& nvmap_cleanup = m_module->GetContainer().GetNvMapFile();
        for (auto& reg : m_gpu_only_regions) {
            if (reg.sparse) {
                Nvidia::Devices::Nx2GpuMemoryRegistry::Get().DestroySparse(reg.gpu_address, reg.size);
            } else {
                if (reg.handle_id == 0 && reg.cpu_address != 0) {
                    Nvidia::Devices::Nx2GpuMemoryRegistry::Get().Unmap(reg.gpu_address, reg.size);
                    Nvidia::Devices::Nx2GpuMemoryRegistry::Get().ReleaseBacking(reg.cpu_address, reg.size);
                }
                if (reg.handle_id != 0) {
                    nvmap_cleanup.FreeHandle(reg.handle_id, false);
                }
                if (reg.pages) {
                    m_process->GetPageTable().UnmapPageGroup(
                        Common::ProcessAddress{reg.cpu_address}, *reg.pages,
                        Kernel::KMemoryState::IoMemory);
                    reg.pages->Close(kernel);
                }
            }
        }
        m_gpu_only_regions.clear();
        auto& nvmap = m_module->GetContainer().GetNvMapFile();
        for (const auto id : m_memory) {
            if (const auto info = nvmap.FreeHandle(id, false); info && info->can_unlock) {
                const auto result = m_process->GetPageTable().UnlockForDeviceAddressSpace(
                    info->address, info->size);
                if (result.IsError()) {
                    LOG_ERROR(Service_AM, "GPU memory cleanup failed: {:#x}", result.raw);
                }
            }
        }
        if (m_nv_session) {
            m_module->GetContainer().CloseSession(Nvidia::NvCore::SessionId{*m_nv_session});
        }
        m_process->Close(kernel);
    }
    Result HandleSyncRequest(Kernel::KServerSession& session, HLERequestContext& ctx) override {
        if (!ctx.IsTipc() || handlers_tipc.contains(ctx.GetCommand())) {
            return ServiceFrameworkBase::HandleSyncRequest(session, ctx);
        }
        const auto guard = LockService();
        LOG_WARNING(Service_AM, "IGpuWorker unsupported TIPC command={}, w0={:#x}, w1={:#x}, w2={:#x}, w3={:#x}", ctx.GetCommand(), ctx.CommandBuffer()[0], ctx.CommandBuffer()[1], ctx.CommandBuffer()[2], ctx.CommandBuffer()[3]);
        InvokeRequestTipc(ctx);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(Kernel::ResultNotImplemented.raw);
        return system.IsPoweredOn() ? ctx.WriteToOutgoingCommandBuffer() : ResultSuccess;
    }
private:
    struct Mapping { u32 as_id; u64 address; u64 size; u32 memory_id; };
    std::vector<Mapping> m_mappings;
    struct GpuOnlyRegion {
        u32 as_id{};
        u64 gpu_address{};
        u64 size{};
        u64 cpu_address{};
        u32 handle_id{};
        bool sparse{};
        std::unique_ptr<Kernel::KPageGroup> pages;
    };
    std::vector<GpuOnlyRegion> m_gpu_only_regions;
    u64 m_alias_hint{};
    static u64 ReadPair(const u32* packet, size_t index) {
        return u64{packet[index]} | (u64{packet[index + 1]} << 32);
    }
    bool OwnsAddressSpace(u32 id) const {
        for (size_t i = 0; i < m_devices->address_spaces.size(); ++i) {
            if (m_devices->address_spaces[i] &&
                static_cast<u32>(*m_devices->address_spaces[i]) == id &&
                m_devices->address_space_registered[i]) { return true; }
        }
        return false;
    }
    Result MapMemory(const u32* packet, u8& selected_kind) {
        R_UNLESS(OwnsAddressSpace(packet[2]), Kernel::ResultNotFound);
        R_UNLESS(std::find(m_memory.begin(), m_memory.end(), packet[7]) != m_memory.end(),
                 Kernel::ResultNotFound);
        R_UNLESS(packet[15] < m_devices->devices.size() && m_devices->devices[packet[15]],
                 Kernel::ResultNotFound);
        if (!(packet[10] == 0 && packet[11] <= 1 && packet[14] == 0 && packet[16] <= 1)) { LOG_WARNING(Service_AM, "MapMemory SDK20 flags p10={:#x} p11={:#x} p14={:#x} p16={:#x}", packet[10], packet[11], packet[14], packet[16]); }
        const auto kind = Nvidia::Devices::SelectNativeMemoryKind(
            static_cast<u8>(packet[12]), static_cast<u8>(packet[13]));
        R_UNLESS(kind, Kernel::ResultNotImplemented);
        const u64 begin = ReadPair(packet, 3), end = ReadPair(packet, 5);
        R_UNLESS(end >= begin && end != ~u64{0}, Kernel::ResultInvalidSize);
        R_UNLESS(m_mappings.size() < 65536, Kernel::ResultOutOfResource);
        const auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(packet[2]);
        R_UNLESS(as, Kernel::ResultNotFound);
        const auto result = as->MapNative4K(packet[7], begin, ReadPair(packet, 8),
                                           end - begin + 1, kind->backend);
        R_UNLESS(result == Nvidia::NvResult::Success, Kernel::ResultInvalidCurrentMemory);
        m_mappings.push_back({packet[2], begin, end - begin + 1, packet[7]});
        selected_kind = kind->selected;
        R_SUCCEED();
    }
    void NormalMapGpuMemory4K(HLERequestContext& ctx) {
        std::array<u32, 18> words;
        std::copy_n(ctx.CommandBuffer(), words.size(), words.begin());
        // These arguments are single-byte TIPC fields; SDK padding is not zeroed.
        for (size_t i = 10; i <= 16; ++i) { words[i] &= 0xff; }
        const auto* packet = words.data();
        const std::scoped_lock device_lock{m_devices->mutex};
        Result result = Kernel::ResultInvalidCombination;
        u8 selected_kind{};
        if (packet[0] == 0xe3 && packet[1] == 16 && packet[17] < m_devices->events.size() &&
            m_devices->events[packet[17]]) {
            result = MapMemory(packet, selected_kind);
            m_devices->events[packet[17]]->Signal(kernel);
            LOG_DEBUG(Service_AM, "GPU map4K as={}, memory={}, VA={:#x}..{:#x}, permission={}, cache={}, kinds={:#x}/{:#x}, mode={}, sync={}, notification={}, selected={:#x}, result={:#x}",
                      packet[2], packet[7], ReadPair(packet, 3), ReadPair(packet, 5), packet[10],
                      packet[11], packet[12], packet[13], packet[14], packet[16], packet[17], selected_kind, result.raw);
        }
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(result.raw);
        rb.Push<u32>(selected_kind);
    }
    void NormalModifyGpuMemory4K(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const std::scoped_lock device_lock{m_devices->mutex};
        Result result = Kernel::ResultInvalidCombination;
        u8 selected{};
        const u64 begin = ReadPair(packet, 3), end = ReadPair(packet, 5);
        const auto requested = static_cast<u8>(packet[7]);
        const auto fallback = static_cast<u8>(packet[8]);
        if (packet[0] == 0xe4 && packet[1] == 9 && (packet[9] & 0xff) <= 1 &&
            packet[10] < m_devices->events.size() && m_devices->events[packet[10]]) {
            result = Kernel::ResultNotFound;
            const auto kind = Nvidia::Devices::SelectNativeMemoryKind(requested, fallback);
            const auto owner = std::find_if(m_mappings.begin(), m_mappings.end(), [&](const auto& mapping) {
                return mapping.as_id == packet[2] && begin >= mapping.address &&
                       begin - mapping.address < mapping.size && end >= begin &&
                       end - mapping.address < mapping.size;
            });
            if (kind && OwnsAddressSpace(packet[2]) && owner != m_mappings.end()) {
                const auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(packet[2]);
                if (as && as->ModifyNativeKind(begin, end - begin + 1, requested, fallback) ==
                              Nvidia::NvResult::Success) {
                    selected = kind->selected;
                    result = ResultSuccess;
                }
            }
            m_devices->events[packet[10]]->Signal(kernel);
        }
        LOG_DEBUG(Service_AM, "GPU modify4K as={}, VA={:#x}..{:#x}, kinds={:#x}/{:#x}, sync={}, notification={}, selected={}, result={:#x}",
                  packet[2], begin, end, requested, fallback, packet[9], packet[10], selected, result.raw);
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(result.raw);
        rb.Push<u32>(selected);
    }
    void NormalUnmapGpuMemory4K(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const std::scoped_lock device_lock{m_devices->mutex};
        Result result = Kernel::ResultInvalidCombination;
        u64 unmapped{};
        if (packet[0] == 0xe5 && packet[1] == 7 && (packet[7] & 0xff) <= 1 &&
            packet[8] < m_devices->events.size() && m_devices->events[packet[8]]) {
            const u64 begin = ReadPair(packet, 3), end = ReadPair(packet, 5);
            const auto it = std::find_if(m_mappings.begin(), m_mappings.end(), [&](const auto& mapping) {
                return mapping.as_id == packet[2] && mapping.address == begin &&
                       end >= begin && end - begin + 1 == mapping.size;
            });
            result = Kernel::ResultNotFound;
            if (it != m_mappings.end()) {
                const auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(it->as_id);
                result = Kernel::ResultInvalidState;
                if (as && as->UnmapNative4K(it->address, it->size) == Nvidia::NvResult::Success) {
                    unmapped = it->size;
                    m_mappings.erase(it);
                    result = ResultSuccess;
                }
            }
            m_devices->events[packet[8]]->Signal(kernel);
            LOG_DEBUG(Service_AM, "GPU unmap4K as={}, VA={:#x}..{:#x}, bytes={:#x}, result={:#x}",
                      packet[2], begin, end, unmapped, result.raw);
        }
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(result.raw);
        rb.Push(unmapped);
    }
    void CreateGpuOnlyRegion(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const std::scoped_lock device_lock{m_devices->mutex};
        const u32 as_id = packet[2];
        const u64 begin = ReadPair(packet, 3);
        const u64 end = ReadPair(packet, 5);
        const u32 slot = packet[1] >= 10 ? packet[11] : packet[10];
        const bool is_sparse_flag = (packet[7] & 0x200U) != 0;
        auto kind = Nvidia::Devices::SelectNativeMemoryKind(
            static_cast<u8>(packet[8]), static_cast<u8>(packet[9]));
        if (!kind) {
            kind = Nvidia::Devices::SelectNativeMemoryKind(
                static_cast<u8>(packet[7]), static_cast<u8>(packet[8]));
        }
        const u8 selected_kind = kind ? kind->selected : u8{6};
        const u8 backend_kind = kind ? kind->backend : static_cast<u8>(Tegra::PTEKind::GENERIC_16BX2);
        Result result = Kernel::ResultInvalidCombination;
        if (end >= begin && end != ~u64{0} && OwnsAddressSpace(as_id)) {
            const u64 size = end - begin + 1;
            const auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(as_id);
            if (as && Common::IsAligned(begin, Kernel::PageSize) && Common::IsAligned(size, Kernel::PageSize)) {
                result = Kernel::ResultOutOfResource;
                bool allocated_full = false;
                if (!is_sparse_flag && size <= (2048ULL << 20)) {
                    auto pg = std::make_unique<Kernel::KPageGroup>(
                        kernel, std::addressof(kernel.GetSystemSystemResource().GetBlockInfoManager()));
                    if (R_SUCCEEDED(kernel.MemoryManager().AllocateAndOpen(
                            pg.get(), size / Kernel::PageSize,
                            Kernel::KMemoryManager::EncodeOption(
                                Kernel::KMemoryManager::Pool::SystemNonSecure,
                                Kernel::KMemoryManager::Direction::FromBack)))) {
                        auto& page_table = m_process->GetPageTable();
                        const u64 region_start = GetInteger(page_table.GetAliasCodeRegionStart());
                        const u64 region_size = page_table.GetAliasCodeRegionSize();
                        constexpr u64 Alignment = 0x200000ULL;
                        const u64 slots = region_size / Alignment;
                        u64 mapped_cpu = 0;
                        for (u64 i = 0; i < slots; ++i) {
                            const u64 offset = ((m_alias_hint + i) % slots) * Alignment;
                            if (offset + size > region_size) {
                                continue;
                            }
                            const Common::ProcessAddress cand{region_start + offset};
                            if (R_SUCCEEDED(page_table.MapPageGroup(
                                    cand, *pg, Kernel::KMemoryState::IoMemory,
                                    Kernel::KMemoryPermission::UserReadWrite))) {
                                m_alias_hint = (offset + Common::AlignUp(size, Alignment)) / Alignment;
                                mapped_cpu = GetInteger(cand);
                                break;
                            }
                        }
                        if (mapped_cpu != 0) {
                            if (!m_nv_session) {
                                m_nv_session = m_module->GetContainer().OpenSession(m_process).id;
                            }
                            auto& nvmap = m_module->GetContainer().GetNvMapFile();
                            std::shared_ptr<Nvidia::NvCore::NvMap::Handle> handle;
                            if (nvmap.CreateHandle(size, handle) == Nvidia::NvResult::Success &&
                                handle->Alloc({}, Kernel::PageSize, 0, mapped_cpu,
                                              Nvidia::NvCore::SessionId{*m_nv_session}) ==
                                    Nvidia::NvResult::Success) {
                                if (as->MapNative4K(handle->id, begin, 0, size, backend_kind) ==
                                    Nvidia::NvResult::Success) {
                                    m_mappings.push_back({as_id, begin, size, handle->id});
                                    m_gpu_only_regions.push_back(
                                        {as_id, begin, size, mapped_cpu, handle->id, false, std::move(pg)});
                                    result = ResultSuccess;
                                    allocated_full = true;
                                } else {
                                    Nvidia::Devices::Nx2GpuMemoryRegistry::Get().MapAt(begin, mapped_cpu, size);
                                    nvmap.FreeHandle(handle->id, false);
                                    m_gpu_only_regions.push_back(
                                        {as_id, begin, size, mapped_cpu, 0, false, std::move(pg)});
                                    result = ResultSuccess;
                                    allocated_full = true;
                                }
                            } else {
                                if (handle) {
                                    nvmap.FreeHandle(handle->id, false);
                                }
                                page_table.UnmapPageGroup(Common::ProcessAddress{mapped_cpu}, *pg,
                                                          Kernel::KMemoryState::IoMemory);
                                pg->Close(kernel);
                            }
                        } else {
                            pg->Close(kernel);
                        }
                    }
                }
                if (!allocated_full) {
                    Nvidia::Devices::Nx2GpuMemoryRegistry::Get().CreateSparse(begin, size);
                    m_gpu_only_regions.push_back({as_id, begin, size, 0, 0, true, nullptr});
                    result = ResultSuccess;
                }
            }
        }
        if (slot < m_devices->events.size() && m_devices->events[slot]) {
            m_devices->events[slot]->Signal(kernel);
        }
        LOG_INFO(Service_AM, "IGpuWorker::CreateGpuOnlyRegion as={}, VA={:#x}..{:#x}, w7={:#x}, w8={:#x}, w9={:#x}->selected={:#x}, slot={}, result={:#x}",
                 as_id, begin, end, packet[7], packet[8], packet[9], selected_kind, slot, result.raw);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result.raw);
    }
    void ExecuteSparseMapRequests(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const std::scoped_lock device_lock{m_devices->mutex};
        const u32 as_id = packet[2];
        const u32 slot = packet[1] >= 8 ? packet[8] : (packet[1] >= 7 ? packet[7] : 0);

        const auto in_buffer = ctx.ReadBuffer();
        if (!in_buffer.empty()) {
            constexpr size_t RequestSize = 40;
            const size_t num_requests = in_buffer.size() / RequestSize;
            for (size_t i = 0; i < num_requests; ++i) {
                const u8* req_ptr = in_buffer.data() + i * RequestSize;
                u64 virtual_address = 0;
                u64 physical_address = 0;
                u64 size = 0;
                u32 flags = 0;
                std::memcpy(&virtual_address, req_ptr, sizeof(u64));
                std::memcpy(&physical_address, req_ptr + 8, sizeof(u64));
                std::memcpy(&size, req_ptr + 16, sizeof(u64));
                std::memcpy(&flags, req_ptr + 24, sizeof(u32));

                if (virtual_address != 0 && size != 0) {
                    if (physical_address != 0) {
                        Nvidia::Devices::Nx2GpuMemoryRegistry::Get().MapAt(virtual_address, physical_address, size);
                        Tegra::Nx2MarkGpuWritten(virtual_address, size);
                    } else {
                        Nvidia::Devices::Nx2GpuMemoryRegistry::Get().Unmap(virtual_address, size);
                    }
                }
            }
        }

        if (slot < m_devices->events.size() && m_devices->events[slot]) {
            m_devices->events[slot]->Signal(kernel);
        }
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void DestroyGpuOnlyRegion(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const std::scoped_lock device_lock{m_devices->mutex};
        const u32 as_id = packet[2];
        const u64 begin = ReadPair(packet, 3);
        const u64 end = ReadPair(packet, 5);
        const u32 slot = packet[1] >= 7 ? packet[8] : packet[7];
        Result result = Kernel::ResultNotFound;
        if (end >= begin && end != ~u64{0}) {
            const u64 size = end - begin + 1;
            const auto it = std::find_if(m_gpu_only_regions.begin(), m_gpu_only_regions.end(),
                                         [&](const GpuOnlyRegion& r) {
                                             return r.as_id == as_id && r.gpu_address == begin && r.size == size;
                                         });
            if (it != m_gpu_only_regions.end()) {
                const auto as = m_module->GetDevice<Nvidia::Devices::nvhost_as_gpu>(as_id);
                if (it->sparse) {
                    Nvidia::Devices::Nx2GpuMemoryRegistry::Get().DestroySparse(begin, size);
                } else {
                    if (it->handle_id != 0) {
                        if (as) {
                            as->UnmapNative4K(begin, size);
                        }
                        std::erase_if(m_mappings, [&](const Mapping& m) {
                            return m.as_id == as_id && m.address == begin && m.size == size;
                        });
                        m_module->GetContainer().GetNvMapFile().FreeHandle(it->handle_id, false);
                    } else {
                        Nvidia::Devices::Nx2GpuMemoryRegistry::Get().Unmap(begin, size);
                        Nvidia::Devices::Nx2GpuMemoryRegistry::Get().ReleaseBacking(it->cpu_address, size);
                    }
                    if (it->pages) {
                        m_process->GetPageTable().UnmapPageGroup(
                            Common::ProcessAddress{it->cpu_address}, *it->pages,
                            Kernel::KMemoryState::IoMemory);
                        it->pages->Close(kernel);
                    }
                }
                m_gpu_only_regions.erase(it);
                result = ResultSuccess;
            }
        }
        if (slot < m_devices->events.size() && m_devices->events[slot]) {
            m_devices->events[slot]->Signal(kernel);
        }
        LOG_INFO(Service_AM, "IGpuWorker::DestroyGpuOnlyRegion as={}, VA={:#x}..{:#x}, slot={}, result={:#x}",
                 as_id, begin, end, slot, result.raw);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result.raw);
    }
    Result RegisterMemory(u64 begin, u64 end, u32 flags, u32 device, u32& id) {
        if (flags != 0) { LOG_WARNING(Service_AM, "RegisterMemory SDK20 non-zero flags={:#x}", flags); }
        R_UNLESS(device < m_devices->devices.size(), Kernel::ResultNotFound);
        const std::scoped_lock device_lock{m_devices->mutex};
        R_UNLESS(m_devices->devices[device], Kernel::ResultNotFound);
        R_UNLESS(end >= begin && end != ~u64{0}, Kernel::ResultInvalidSize);
        const u64 size = end - begin + 1;
        R_UNLESS(Common::IsAligned(begin, Kernel::PageSize), Kernel::ResultInvalidAddress);
        R_UNLESS(Common::IsAligned(size, Kernel::PageSize), Kernel::ResultInvalidSize);
        R_UNLESS(m_memory.size() < 65536, Kernel::ResultOutOfResource);
        auto& page_table = m_process->GetPageTable();
        R_UNLESS(page_table.Contains(begin, size), Kernel::ResultInvalidCurrentMemory);
        for (u64 address = begin; address <= end;) {
            Kernel::KMemoryInfo info{};
            Kernel::Svc::PageInfo page{};
            R_TRY(page_table.QueryInfo(&info, &page, address));
            R_UNLESS(True(info.m_attribute & Kernel::KMemoryAttribute::GpuAccessible),
                     Kernel::ResultInvalidCurrentMemory);
            const u64 next = info.m_address + info.m_size;
            R_UNLESS(next > address, Kernel::ResultInvalidCurrentMemory);
            address = next;
        }
        if (!m_nv_session) {
            m_nv_session = m_module->GetContainer().OpenSession(m_process).id;
        }
        auto& nvmap = m_module->GetContainer().GetNvMapFile();
        std::shared_ptr<Nvidia::NvCore::NvMap::Handle> handle;
        R_UNLESS(nvmap.CreateHandle(size, handle) == Nvidia::NvResult::Success,
                 Kernel::ResultOutOfResource);
        bool is_io{};
        const auto lock_result = page_table.LockForMapDeviceAddressSpace(
            &is_io, begin, size, Kernel::KMemoryPermission::None, true, true);
        if (lock_result.IsError()) {
            nvmap.FreeHandle(handle->id, false);
            R_RETURN(lock_result);
        }
        const auto alloc_result = handle->Alloc({}, Kernel::PageSize, 0, begin,
                                               Nvidia::NvCore::SessionId{*m_nv_session});
        if (alloc_result != Nvidia::NvResult::Success) {
            page_table.UnlockForDeviceAddressSpace(begin, size);
            nvmap.FreeHandle(handle->id, false);
            R_RETURN(Kernel::ResultOutOfResource);
        }
        id = handle->id;
        m_memory.push_back(id);
        R_SUCCEED();
    }
    Result ReleaseMemory(u32 id, u32 notification) {
        const std::scoped_lock device_lock{m_devices->mutex};
        R_UNLESS(notification < m_devices->events.size() && m_devices->events[notification],
                 Kernel::ResultInvalidCombination);
        const auto it = std::find(m_memory.begin(), m_memory.end(), id);
        R_UNLESS(it != m_memory.end(), Kernel::ResultNotFound);
        auto& nvmap = m_module->GetContainer().GetNvMapFile();
        auto handle = nvmap.GetHandle(id);
        R_UNLESS(handle, Kernel::ResultNotFound);
        {
            const std::scoped_lock handle_lock{handle->mutex};
            // No asynchronous GPU work is implemented yet. Never report release
            // completion for memory still retained or pinned by another user.
            R_UNLESS(handle->pins == 0 && handle->internal_dupes == 0 && handle->dupes == 1,
                     Kernel::ResultBusy);
        }
        // FreeHandle uses the final shared reference to decide when CPU memory
        // may be unshared. Drop this temporary inspection reference first.
        handle.reset();
        const auto info = nvmap.FreeHandle(id, false);
        R_UNLESS(info && info->can_unlock, Kernel::ResultInvalidState);
        m_memory.erase(it);
        R_TRY(m_process->GetPageTable().UnlockForDeviceAddressSpace(info->address, info->size));
        m_devices->events[notification]->Signal(kernel);
        R_SUCCEED();
    }
    void DestroyGpuAccessibleMemory(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        const auto result = packet[0] == 0x76 && packet[1] == 2
            ? ReleaseMemory(packet[2], packet[3]) : Kernel::ResultInvalidCombination;
        LOG_DEBUG(Service_AM, "GPU memory release id={}, notification={}, result={:#x}",
                  packet[2], packet[3], result.raw);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result.raw);
    }
    void CreateGpuAccessibleMemory(HLERequestContext& ctx) {
        const auto* packet = ctx.CommandBuffer();
        u32 id{};
        Result result = Kernel::ResultInvalidCombination;
        if (const auto request = DecodeGpuMemoryRegistration(std::span<const u32>{packet, 8})) {
            result = RegisterMemory(request->begin, request->end, request->flags, request->device, id);
            LOG_DEBUG(Service_AM, "GPU memory {:#x}..{:#x}, device={}, raw_device={:#x}, flags={}, id={}, result={:#x}",
                      request->begin, request->end, request->device, packet[7], request->flags, id, result.raw);
        }
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(result.raw);
        rb.Push(id);
    }
    std::shared_ptr<GpuDeviceRegistry> m_devices;
    std::optional<size_t> m_nv_session;
    std::vector<u32> m_memory;
    std::shared_ptr<Nvidia::Module> m_module;
    Kernel::KProcess* const m_process;
};
} // namespace

IGpuSession::IGpuSession(Core::System& system, std::shared_ptr<Nvidia::Module> module,
                         Kernel::KProcess* process)
    : ServiceFramework{system, "IGpuSession"},
      m_process{process}, m_module{std::move(module)} {
    m_process->Open(kernel);
    m_devices = std::make_shared<GpuDeviceRegistry>(system, m_module, m_process);
    static const FunctionInfo functions[] = {
        {11, &IGpuSession::GetNotificationRange, "GetNotificationRange"},
        {12, &IGpuSession::GetNotificationEvent, "GetNotificationEvent"},
        {21, &IGpuSession::RequestWorker, "RequestWorker"},
        {22, &IGpuSession::AcquireWorker, "AcquireWorker"},
        {33, &IGpuSession::CreateAddressSpace, "CreateAddressSpace"},
        {41, &IGpuSession::CreateGraphicsDevice, "CreateGraphicsDevice"},
        {42, &IGpuSession::DestroyGraphicsDevice, "DestroyGraphicsDevice"},
        {201, &IGpuSession::GetAddressSpacePartition, "GetUserPartition201"},
        {202, &IGpuSession::GetAddressSpacePartition, "GetGsvPartition"},
        {203, &IGpuSession::GetAddressSpacePartition, "GetUserPartition"},
    };
    RegisterHandlersTipc(functions);
}

IGpuSession::~IGpuSession() {
    if (m_worker) { m_worker->Close(kernel); }
    m_process->Close(kernel);
}

Result IGpuSession::HandleSyncRequest(Kernel::KServerSession& session, HLERequestContext& ctx) {
    if (!ctx.IsTipc() || handlers_tipc.contains(ctx.GetCommand())) {
        return ServiceFrameworkBase::HandleSyncRequest(session, ctx);
    }
    const auto guard = LockService();
    LOG_WARNING(Service_AM, "IGpuSession unsupported TIPC command={}, w0={:#x}, w1={:#x}, w2={:#x}, w3={:#x}", ctx.GetCommand(), ctx.CommandBuffer()[0], ctx.CommandBuffer()[1], ctx.CommandBuffer()[2], ctx.CommandBuffer()[3]);
    InvokeRequestTipc(ctx);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(Kernel::ResultNotImplemented.raw);
    return system.IsPoweredOn() ? ctx.WriteToOutgoingCommandBuffer() : ResultSuccess;
}

Result IGpuSession::RegisterAddressSpace(u32 id) {
    const auto guard = LockService();
    const std::scoped_lock device_lock{m_devices->mutex};
    for (size_t i = 0; i < m_devices->address_spaces.size(); ++i) {
        if (m_devices->address_spaces[i] && static_cast<u32>(*m_devices->address_spaces[i]) == id) {
            m_devices->address_space_registered[i] = true;
            R_SUCCEED();
        }
    }
    R_THROW(Kernel::ResultNotFound);
}

void IGpuSession::GetAddressSpacePartition(HLERequestContext& ctx) {
    const std::scoped_lock device_lock{m_devices->mutex};
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    std::array<u64, 9> values{};
    const bool user = ctx.GetCommand() == 203 || ctx.GetCommand() == 201;
    if (!user) {
        values[0] = ~u64{0}; // No supervisor-owned partition is reserved by this adapter.
    }
    if (packet[0] == 16 + ctx.GetCommand() && packet[1] == 1) {
        const u32 id = packet[2];
        const bool exists = std::any_of(m_devices->address_spaces.begin(), m_devices->address_spaces.end(),
                                       [id](const auto& fd) { return fd && static_cast<u32>(*fd) == id; });
        result = exists ? ResultSuccess : Kernel::ResultNotFound;
        if (exists && user) {
            std::array<u8, 64> input{};
            std::array<u8, 64> output{};
            const u32_le buffer_size{48};
            std::memcpy(input.data() + 8, &buffer_size, sizeof(buffer_size));
            Nvidia::Ioctl command{};
            command.raw = 0xc0404108;
            if (m_module->Ioctl1(static_cast<s32>(id), command, input, output) ==
                Nvidia::NvResult::Success) {
                std::array<Nvidia::Devices::nvhost_as_gpu::VaRegion, 2> regions;
                std::memcpy(regions.data(), output.data() + 16, sizeof(regions));
                values[0] = 1;
                values[1] = regions[0].offset;
                values[2] = regions[1].offset + regions[1].pages * regions[1].page_size - 1;
            } else {
                result = Kernel::ResultInvalidState;
            }
        }
        LOG_DEBUG(Service_AM, "GPU partition command={}, id={}, begin={:#x}, end={:#x}, result={:#x}",
                  ctx.GetCommand(), id, user ? values[1] : values[0],
                  user ? values[2] : values[1], result.raw);
    }
    IPC::ResponseBuilder rb{ctx, user ? 20U : 6U};
    rb.Push(result.raw);
    for (size_t i = 0; i < (user ? values.size() : size_t{2}); ++i) {
        rb.Push(values[i]);
    }
}

void IGpuSession::CreateAddressSpace(HLERequestContext& ctx) {
    const std::scoped_lock device_lock{m_devices->mutex};
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    u32 id{};
    if (packet[0] == 0x31 && packet[1] == 1 && packet[2] == 0) {
        result = Kernel::ResultOutOfResource;
        for (auto& slot : m_devices->address_spaces) {
            if (slot) {
                continue;
            }
            if (!m_devices->nv_session) {
                m_devices->nv_session = m_module->GetContainer().OpenSession(m_process).id;
            }
            const auto fd = m_module->Open("/dev/nvhost-as-gpu",
                                          Nvidia::NvCore::SessionId{*m_devices->nv_session});
            if (fd == Nvidia::INVALID_NVDRV_FD) {
                break;
            }
            std::array<u8, 40> input{};
            std::array<u8, 40> output{};
            const u32_le big_page_size{0x20000};
            std::memcpy(input.data() + 8, &big_page_size, sizeof(big_page_size));
            Nvidia::Ioctl command{};
            command.raw = 0x40284109;
            const auto nv_result = m_module->Ioctl1(fd, command, input, output);
            if (nv_result == Nvidia::NvResult::Success) {
                slot = fd;
                id = fd;
                result = ResultSuccess;
            } else {
                m_module->Close(fd);
                result = Kernel::ResultInvalidState;
            }
            break;
        }
    }
    LOG_DEBUG(Service_AM, "GPU address space id={}, result={:#x}", id, result.raw);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result.raw);
    rb.Push(id);
}

void IGpuSession::CreateGraphicsDevice(HLERequestContext& ctx) {
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    u32 id{};
    if (packet[0] == 0x39 && packet[1] == 6) {
        const u64 name = u64{packet[2]} | (u64{packet[3]} << 32);
        const u64 flags = u64{packet[4]} | (u64{packet[5]} << 32);
        const u64 debug_label = u64{packet[6]} | (u64{packet[7]} << 32);
        if (flags != 0) { LOG_WARNING(Service_AM, "CreateGraphicsDevice SDK20 non-zero flags={:#x}", flags); }
        result = Kernel::ResultOutOfResource;
        {
            const std::scoped_lock device_lock{m_devices->mutex};
            for (u32 candidate = 1; candidate < m_devices->devices.size(); ++candidate) {
                if (!m_devices->devices[candidate]) {
                    id = candidate;
                    m_devices->devices[id] = GpuDeviceRegistry::Device{name, debug_label};
                    result = ResultSuccess;
                    break;
                }
            }
        }
        LOG_DEBUG(Service_AM, "graphics device name={:#x}, flags={:#x}, id={}, result={:#x}",
                  name, flags, id, result.raw);
    }
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result.raw);
    rb.Push(id);
}

void IGpuSession::DestroyGraphicsDevice(HLERequestContext& ctx) {
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    if (packet[0] == 0x3a && packet[1] == 1) {
        const u8 id = static_cast<u8>(packet[2]);
        const std::scoped_lock device_lock{m_devices->mutex};
        result = m_devices->devices[id] ? ResultSuccess : Kernel::ResultNotFound;
        m_devices->devices[id].reset();
    }
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result.raw);
}

void IGpuSession::GetNotificationRange(HLERequestContext& ctx) {
    const auto* packet = ctx.CommandBuffer();
    const Result result = packet[0] == 0x1b && packet[1] == 0
        ? ResultSuccess : Kernel::ResultInvalidCombination;
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result.raw);
    rb.Push<u32>(0);
    rb.Push<u32>(static_cast<u32>(m_devices->events.size() - 1));
    LOG_DEBUG(Service_AM, "GPU notification range 0..{}", m_devices->events.size() - 1);
}

void IGpuSession::GetNotificationEvent(HLERequestContext& ctx) {
    const std::scoped_lock device_lock{m_devices->mutex};
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    Kernel::KReadableEvent* readable{};
    if (packet[0] == 0x1c && packet[1] == 1 && packet[2] < m_devices->events.size()) {
        const u32 index = packet[2];
        if (!m_devices->events[index]) { m_devices->events[index] = m_devices->context.CreateEvent("GpuNotification"); }
        if (m_devices->events[index]) {
            readable = &m_devices->events[index]->GetReadableEvent();
            readable->Open(kernel);
            result = ResultSuccess;
        } else { result = Kernel::ResultOutOfResource; }
        LOG_DEBUG(Service_AM, "GPU notification event index={}, result={:#x}", index, result.raw);
    }
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(result.raw);
    ctx.AddMoveObject(readable);
}

void IGpuSession::RequestWorker(HLERequestContext& ctx) {
    const std::scoped_lock device_lock{m_devices->mutex};
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    if (packet[0] == 0x25 && packet[1] == 1 && packet[2] < m_devices->events.size()) {
        const u32 index = packet[2];
        if (!m_devices->events[index]) {
            result = Kernel::ResultInvalidState;
        } else {
            result = ResultSuccess;
            if (!m_worker) {
                result = CreateGraphicsSession(system, ctx,
                    std::make_shared<IGpuWorker>(system, m_module, m_process, m_devices), &m_worker);
            }
            if (result.IsSuccess()) { m_devices->events[index]->Signal(kernel); }
        }
        LOG_DEBUG(Service_AM, "GPU worker request index={}, result={:#x}", index, result.raw);
    }
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result.raw);
}

void IGpuSession::AcquireWorker(HLERequestContext& ctx) {
    const auto* packet = ctx.CommandBuffer();
    Result result = Kernel::ResultInvalidCombination;
    Kernel::KClientSession* client{};
    if (packet[0] == 0x26 && packet[1] == 0) {
        if (m_worker) {
            client = m_worker;
            client->Open(kernel);
            result = ResultSuccess;
        } else { result = Kernel::ResultInvalidState; }
    }
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(result.raw);
    ctx.AddMoveObject(client);
    LOG_DEBUG(Service_AM, "GPU worker acquisition result={:#x}", result.raw);
}
} // namespace Service::AM
