#pragma once
#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Indices of the queue families we care about.
// compute/transfer prefer dedicated families, but fall back to the graphics family.
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;

    bool IsComplete() const { return graphics.has_value() && present.has_value(); }
};

// A retrieved queue plus the info needed to use it later
// (command pool creation, swapchain sharing mode, queue family ownership transfers...).
struct QueueInfo
{
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = UINT32_MAX;
    uint32_t index = 0; // index inside the family
};

class Device
{
public:
    Device() = default;
    ~Device() { Shutdown(); }

    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    // Picks a physical device that can present to `surface`, then creates the logical device.
    bool Init(VkInstance instance, VkSurfaceKHR surface);
    void Shutdown();

    VkPhysicalDevice GetPhysicalDevice() const { return m_physical; }
    VkDevice GetDevice() const { return m_device; }

    const QueueFamilyIndices &GetQueueFamilies() const { return m_families; }
    const QueueInfo &GetGraphicsQueue() const { return m_graphics; }
    const QueueInfo &GetPresentQueue() const { return m_present; }
    const QueueInfo &GetComputeQueue() const { return m_compute; }
    const QueueInfo &GetTransferQueue() const { return m_transfer; }

    const VkPhysicalDeviceProperties &GetProperties() const { return m_properties; }
    const VkPhysicalDeviceMemoryProperties &GetMemoryProperties() const { return m_memoryProperties; }

    // Handy for buffer/image allocation later. Returns UINT32_MAX if nothing matches.
    uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags required) const;

    bool IsValid() const { return m_device != VK_NULL_HANDLE; }

private:
    bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    bool CreateLogicalDevice();

    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    static bool CheckExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &required);
    static bool HasExtension(VkPhysicalDevice device, const char *name);
    static bool CheckSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    static bool CheckFeatureSupport(VkPhysicalDevice device);
    static int RateDevice(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties m_properties{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};

    QueueFamilyIndices m_families;
    QueueInfo m_graphics;
    QueueInfo m_present;
    QueueInfo m_compute;
    QueueInfo m_transfer;

    std::vector<const char *> m_deviceExtensions;
};