#include "vk/device.h"

#include <cstdio>
#include <cstring>
#include <set>

static const std::vector<const char *> kRequiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices result;

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    auto supportsPresent = [&](uint32_t i)
    {
        VkBool32 supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
        return supported == VK_TRUE;
    };

    // Graphics: first family with graphics bit, preferring one that can also present.
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;

        if (!result.graphics.has_value())
            result.graphics = i;

        if (supportsPresent(i))
        {
            result.graphics = i;
            result.present = i;
            break;
        }
    }

    // Present: if graphics family can't present, find any family that can.
    if (!result.present.has_value())
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            if (supportsPresent(i))
            {
                result.present = i;
                break;
            }
        }
    }

    // Compute: prefer dedicated (compute without graphics).
    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFlags f = families[i].queueFlags;
        if ((f & VK_QUEUE_COMPUTE_BIT) && !(f & VK_QUEUE_GRAPHICS_BIT))
        {
            result.compute = i;
            break;
        }
    }
    if (!result.compute.has_value())
        result.compute = result.graphics;

    // Transfer: prefer dedicated (transfer without graphics/compute).
    for (uint32_t i = 0; i < count; ++i)
    {
        const VkQueueFlags f = families[i].queueFlags;
        if ((f & VK_QUEUE_TRANSFER_BIT) && !(f & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
        {
            result.transfer = i;
            break;
        }
    }
    if (!result.transfer.has_value())
        result.transfer = result.compute.has_value() ? result.compute : result.graphics;

    return result;
}

bool Device::HasExtension(VkPhysicalDevice device, const char *name)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

    for (const VkExtensionProperties &ext : available)
    {
        if (std::strcmp(ext.extensionName, name) == 0)
            return true;
    }
    return false;
}

bool Device::CheckExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &required)
{
    for (const char *name : required)
    {
        if (!HasExtension(device, name))
            return false;
    }
    return true;
}

bool Device::CheckSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);

    return formatCount > 0 && modeCount > 0;
}

bool Device::CheckFeatureSupport(VkPhysicalDevice device)
{
    VkPhysicalDeviceVulkan13Features f13{};
    f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f2.pNext = &f13;

    vkGetPhysicalDeviceFeatures2(device, &f2);

    return f13.dynamicRendering == VK_TRUE && f13.synchronization2 == VK_TRUE;
}

// Returns <= 0 if the device is unsuitable, otherwise a score (higher is better).
int Device::RateDevice(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);

    if (props.apiVersion < VK_API_VERSION_1_3)
        return 0;

    if (!FindQueueFamilies(device, surface).IsComplete())
        return 0;

    if (!CheckExtensionSupport(device, kRequiredExtensions))
        return 0;

    if (!CheckSwapchainSupport(device, surface))
        return 0;

    if (!CheckFeatureSupport(device))
        return 0;

    int score = 1;

    switch (props.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   score += 1000; break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 500;  break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    score += 100;  break;
    default: break;
    }

    score += static_cast<int>(props.limits.maxImageDimension2D / 1024);
    return score;
}

// ---------------------------------------------------------------------------
// Physical device
// ---------------------------------------------------------------------------

bool Device::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0)
    {
        std::fprintf(stderr, "No Vulkan capable GPU found\n");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    int bestScore = 0;
    for (VkPhysicalDevice candidate : devices)
    {
        const int score = RateDevice(candidate, surface);
        if (score > bestScore)
        {
            bestScore = score;
            m_physical = candidate;
        }
    }

    if (m_physical == VK_NULL_HANDLE)
    {
        std::fprintf(stderr, "No suitable GPU found (need Vulkan 1.3, swapchain, dynamicRendering, synchronization2)\n");
        return false;
    }

    vkGetPhysicalDeviceProperties(m_physical, &m_properties);
    vkGetPhysicalDeviceMemoryProperties(m_physical, &m_memoryProperties);
    m_families = FindQueueFamilies(m_physical, surface);

    std::printf("Selected GPU: %s\n", m_properties.deviceName);
    return true;
}

// ---------------------------------------------------------------------------
// Logical device
// ---------------------------------------------------------------------------

bool Device::CreateLogicalDevice()
{
    // One VkDeviceQueueCreateInfo per unique family.
    const std::set<uint32_t> uniqueFamilies = {
        m_families.graphics.value(),
        m_families.present.value(),
        m_families.compute.value(),
        m_families.transfer.value(),
    };

    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());

    for (uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = family;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        queueInfos.push_back(q);
    }

    // Extensions
    m_deviceExtensions = kRequiredExtensions;

    // MoltenVK etc.: if the device exposes the portability subset, it MUST be enabled.
    if (HasExtension(m_physical, "VK_KHR_portability_subset"))
        m_deviceExtensions.push_back("VK_KHR_portability_subset");

    // Features (Vulkan 1.3 core)
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features13;
    // features2.features.samplerAnisotropy = VK_TRUE; // enable core 1.0 features here when needed

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2; // features chain; pEnabledFeatures must stay null
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
    // Device layers are deprecated; instance layers apply to the device too.

    const VkResult result = vkCreateDevice(m_physical, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkCreateDevice failed: %d\n", static_cast<int>(result));
        m_device = VK_NULL_HANDLE;
        return false;
    }

    // Retrieve queues (index 0 of each family).
    auto fetch = [&](QueueInfo &info, uint32_t family)
    {
        info.family = family;
        info.index = 0;
        vkGetDeviceQueue(m_device, family, 0, &info.queue);
    };

    fetch(m_graphics, m_families.graphics.value());
    fetch(m_present, m_families.present.value());
    fetch(m_compute, m_families.compute.value());
    fetch(m_transfer, m_families.transfer.value());

    return true;
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

bool Device::Init(VkInstance instance, VkSurfaceKHR surface)
{
    if (!PickPhysicalDevice(instance, surface))
        return false;

    if (!CreateLogicalDevice())
        return false;

    return true;
}

void Device::Shutdown()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    m_physical = VK_NULL_HANDLE;
    m_families = {};
    m_graphics = {};
    m_present = {};
    m_compute = {};
    m_transfer = {};
    m_deviceExtensions.clear();
}

uint32_t Device::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags required) const
{
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) &&
            (m_memoryProperties.memoryTypes[i].propertyFlags & required) == required)
        {
            return i;
        }
    }
    return UINT32_MAX;
}