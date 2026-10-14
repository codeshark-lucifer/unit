#pragma once
#include <vulkan/vulkan.h>

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
inline constexpr bool validation_enable = true;
#else
inline constexpr bool validation_enable = false;
#endif

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator);

void SetupDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT &debugMessenger);