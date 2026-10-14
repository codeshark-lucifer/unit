#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

class SwapChain
{
private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = { 0, 0 };

    void Create(VkExtent2D extent, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

public:
    static const std::vector<const char*> device_extensions;

    SwapChain() = default;
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    void Init(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkSurfaceKHR surface,
        VkExtent2D extent
    );

    void Recreate(VkExtent2D extent);

    void Destroy();

    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }

    const std::vector<VkImage>& GetImages() const { return m_images; }

    const std::vector<VkImageView>& GetImageViews() const
    {
        return m_imageViews;
    }

    VkFormat GetImageFormat() const { return m_imageFormat; }

    VkExtent2D GetExtent() const { return m_extent; }
};