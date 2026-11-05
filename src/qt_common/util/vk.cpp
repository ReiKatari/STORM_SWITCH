// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include <vector>

#include "qt_common/qt_common.h"

#include "common/dynamic_library.h"
#include "common/logging.h"
#include "qt_common/util/vk.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "video_core/vulkan_common/vulkan_surface.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"
#include "vulkan/vulkan_core.h"

class QWindow;

namespace VkDeviceInfo {
Record::Record(std::string_view name_, const std::vector<VkPresentModeKHR>& vsync_modes_,
               bool has_broken_compute_, std::string_view driver_family_,
               std::string_view driver_version_, std::string_view vulkan_version_,
               const std::vector<std::string>& key_extensions_)
    : name{name_}, vsync_support{vsync_modes_}, has_broken_compute{has_broken_compute_},
      driver_family{driver_family_}, driver_version{driver_version_},
      vulkan_version{vulkan_version_}, key_extensions{key_extensions_} {}

Record::~Record() = default;

void PopulateRecords(std::vector<Record>& records, QWindow* window) try {
    using namespace Vulkan;

    // Create a test window with a Vulkan surface type for checking present modes.
    QWindow test_window(window);
    test_window.setSurfaceType(QWindow::VulkanSurface);
    test_window.create();
    auto wsi = QtCommon::GetWindowSystemInfo(&test_window);

    vk::InstanceDispatch dld;
    const auto library = OpenLibrary();
    const vk::Instance instance = CreateInstance(*library, dld, VK_API_VERSION_1_1, wsi.type);
    const std::vector<VkPhysicalDevice> physical_devices = instance.EnumeratePhysicalDevices();
    vk::SurfaceKHR surface = CreateSurface(instance, wsi);

    records.clear();
    records.reserve(physical_devices.size());
    for (const VkPhysicalDevice device : physical_devices) {
        const auto physical_device = vk::PhysicalDevice(device, dld);
        const auto props = physical_device.GetProperties();
        std::string name = props.deviceName;

#ifdef _WIN32
        if (name.find("Graphics Device") != std::string::npos || name.empty()) {
            const std::string marketing_name = Vulkan::GetGpuMarketingName(props.vendorID, props.deviceID);
            if (!marketing_name.empty()) {
                name = marketing_name;
            }
        }
#endif

        const std::vector<VkPresentModeKHR> present_modes =
            physical_device.GetSurfacePresentModesKHR(*surface);

        VkPhysicalDeviceDriverProperties driver_properties{};
        driver_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        driver_properties.pNext = nullptr;
        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        properties.pNext = &driver_properties;
        dld.vkGetPhysicalDeviceProperties2(physical_device, &properties);

        const auto driverID = driver_properties.driverID;

        bool has_broken_compute{
            Vulkan::Device::CheckBrokenCompute(driverID, properties.properties.driverVersion)};

        std::string driver_string = Vulkan::vk::GetDriverName(driver_properties);

        if (driver_string.empty())
            driver_string = "Unknown";

        name = fmt::format("{} ({})", name, driver_string);

        std::string driver_family = "Unknown";
        switch (driverID) {
        case VK_DRIVER_ID_MESA_PANVK:
            driver_family = "Mesa PanVK (Arm Mali)";
            break;
        case VK_DRIVER_ID_MESA_TURNIP:
            driver_family = "Mesa Turnip (Qualcomm Adreno)";
            break;
        case VK_DRIVER_ID_MESA_RADV:
            driver_family = "Mesa RADV (AMD RDNA)";
            break;
        case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
            driver_family = "Mesa ANV (Intel)";
            break;
        case VK_DRIVER_ID_MESA_NVK:
            driver_family = "Mesa NVK (NVIDIA)";
            break;
        case VK_DRIVER_ID_MESA_LLVMPIPE:
            driver_family = "Mesa Lavapipe / llvmpipe (Software)";
            break;
        case VK_DRIVER_ID_ARM_PROPRIETARY:
            driver_family = "Arm Mali Proprietary";
            break;
        case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
            driver_family = "Qualcomm Proprietary";
            break;
        case VK_DRIVER_ID_SAMSUNG_PROPRIETARY:
            driver_family = "Samsung Xclipse (AMD RDNA)";
            break;
        case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
            driver_family = "NVIDIA Proprietary";
            break;
        case VK_DRIVER_ID_AMD_PROPRIETARY:
            driver_family = "AMD Proprietary";
            break;
        case VK_DRIVER_ID_AMD_OPEN_SOURCE:
            driver_family = "AMDVLK (AMD Open Source)";
            break;
        case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
            driver_family = "Intel Proprietary";
            break;
        case VK_DRIVER_ID_MOLTENVK:
            driver_family = "MoltenVK (Apple Metal)";
            break;
        default:
            if (driver_string.find("PanVK") != std::string::npos || driver_string.find("panvk") != std::string::npos) {
                driver_family = "Mesa PanVK (Arm Mali)";
            } else if (driver_string.find("Turnip") != std::string::npos) {
                driver_family = "Mesa Turnip (Qualcomm Adreno)";
            } else if (driver_string.find("RADV") != std::string::npos) {
                driver_family = "Mesa RADV (AMD RDNA)";
            } else if (driver_string.find("ANV") != std::string::npos) {
                driver_family = "Mesa ANV (Intel)";
            } else if (driver_string.find("NVK") != std::string::npos) {
                driver_family = "Mesa NVK (NVIDIA)";
            } else if (driver_string.find("llvmpipe") != std::string::npos || driver_string.find("lavapipe") != std::string::npos) {
                driver_family = "Mesa Lavapipe / llvmpipe (Software)";
            } else {
                driver_family = driver_string;
            }
            break;
        }

        std::string driver_ver_str = driver_properties.driverInfo;
        if (driver_ver_str.empty()) {
            driver_ver_str = fmt::format("{}.{}.{}", VK_VERSION_MAJOR(properties.properties.driverVersion),
                                         VK_VERSION_MINOR(properties.properties.driverVersion),
                                         VK_VERSION_PATCH(properties.properties.driverVersion));
        }

        std::string vk_api_ver = fmt::format("{}.{}.{}", VK_VERSION_MAJOR(properties.properties.apiVersion),
                                             VK_VERSION_MINOR(properties.properties.apiVersion),
                                             VK_VERSION_PATCH(properties.properties.apiVersion));

        std::vector<std::string> key_exts;
        u32 ext_count = 0;
        dld.vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
        if (ext_count > 0) {
            std::vector<VkExtensionProperties> exts(ext_count);
            dld.vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, exts.data());
            const auto has_ext = [&](const char* ext_name) {
                for (const auto& e : exts) {
                    if (std::strcmp(e.extensionName, ext_name) == 0) return true;
                }
                return false;
            };
            if (has_ext(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) key_exts.push_back("Dynamic Rendering");
            if (has_ext(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)) key_exts.push_back("Descriptor Indexing");
            if (has_ext(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) key_exts.push_back("Timeline Semaphore");
            if (has_ext(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) key_exts.push_back("Synchronization2");
            if (has_ext(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)) key_exts.push_back("Extended Dynamic State");
            if (has_ext(VK_KHR_MAINTENANCE_4_EXTENSION_NAME)) key_exts.push_back("Maintenance4");
#ifndef VK_KHR_MAINTENANCE_5_EXTENSION_NAME
#define VK_KHR_MAINTENANCE_5_EXTENSION_NAME "VK_KHR_maintenance5"
#endif
            if (has_ext(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) key_exts.push_back("Maintenance5");
            if (has_ext(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME)) key_exts.push_back("Custom Border Color");
            if (has_ext(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME)) key_exts.push_back("Robustness2");
        }

        records.push_back(VkDeviceInfo::Record(name, present_modes, has_broken_compute, driver_family, driver_ver_str, vk_api_ver, key_exts));
    }
} catch (const Vulkan::vk::Exception& exception) {
    LOG_ERROR(Frontend, "Failed to enumerate devices with error: {}", exception.what());
}

} // namespace VkDeviceInfo
