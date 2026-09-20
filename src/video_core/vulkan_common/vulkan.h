// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define VK_NO_PROTOTYPES
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__HAIKU__)
#define VK_USE_PLATFORM_XCB_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <vulkan/vulkan.h>

// Define maintenance 7-8 extension names (not yet in official Vulkan headers)
#ifndef VK_KHR_MAINTENANCE_7_EXTENSION_NAME
#define VK_KHR_MAINTENANCE_7_EXTENSION_NAME "VK_KHR_maintenance7"
#endif
#ifndef VK_KHR_MAINTENANCE_8_EXTENSION_NAME
#define VK_KHR_MAINTENANCE_8_EXTENSION_NAME "VK_KHR_maintenance8"
#endif

// Define VK_EXT_swapchain_maintenance1
#ifndef VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME
#define VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME "VK_EXT_swapchain_maintenance1"
#endif
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT (static_cast<VkStructureType>(1000275000))
#endif
#ifndef VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT
#define VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT (static_cast<VkStructureType>(1000275001))
#endif
#ifndef VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT
#define VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT (static_cast<VkStructureType>(1000275002))
#endif
#ifndef VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT
#define VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT (static_cast<VkStructureType>(1000275003))
#endif
#ifndef VK_STRUCTURE_TYPE_RELEASE_SWAPPED_IMAGE_RESERVATIONS_INFO_EXT
#define VK_STRUCTURE_TYPE_RELEASE_SWAPPED_IMAGE_RESERVATIONS_INFO_EXT (static_cast<VkStructureType>(1000275004))
#endif

#ifndef VK_EXT_swapchain_maintenance1
#define VK_EXT_swapchain_maintenance1 1

typedef struct VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT {
    VkStructureType sType;
    void* pNext;
    VkBool32 swapchainMaintenance1;
} VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT;

typedef struct VkSwapchainPresentModesCreateInfoEXT {
    VkStructureType sType;
    const void* pNext;
    uint32_t presentModeCount;
    const VkPresentModeKHR* pPresentModes;
} VkSwapchainPresentModesCreateInfoEXT;

typedef struct VkSwapchainPresentModeInfoEXT {
    VkStructureType sType;
    const void* pNext;
    uint32_t swapchainCount;
    const VkPresentModeKHR* pPresentModes;
} VkSwapchainPresentModeInfoEXT;
#endif

// Sanitize macros
#undef CreateEvent
#undef CreateSemaphore
#undef Always
#undef False
#undef None
#undef True

// "Catch-all" handle for both Android and.. the rest of platforms
struct VkSurfaceKHR_T;
