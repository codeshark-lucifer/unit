#pragma once
#include "vulkan/vulkan.h"
#include "vk/device.h"
#include "vk/swapchain.h"
#include "vk/framebuffer.h"

#include <array>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
struct FrameData
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
};

struct UniformBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void *mapped = nullptr;
};

struct VkContext
{
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frames;

    Device device;
    SwapChain swapchain;
    std::vector<Framebuffer> framebuffers;

    std::vector<UniformBuffer>
        uniforms;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets{};

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

extern VkContext vk_context;