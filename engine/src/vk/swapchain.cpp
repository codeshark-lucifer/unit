#include "vk/swapchain.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

const std::vector<const char*> SwapChain::device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

SwapChain::~SwapChain()
{
    Destroy();
}

void SwapChain::Init(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    VkExtent2D extent
)
{
    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "SwapChain::Init: Invalid Vulkan handles."
        );
    }

    m_physicalDevice = physicalDevice;
    m_device = device;
    m_surface = surface;

    Create(extent, VK_NULL_HANDLE);
}

void SwapChain::Recreate(VkExtent2D extent)
{
    if (m_device == VK_NULL_HANDLE || m_surface == VK_NULL_HANDLE)
    {
        return;
    }

    // Wait for the device to finish all queued operations before touching swapchain resources.
    vkDeviceWaitIdle(m_device);

    // Destroy existing image views before creating the new swapchain.
    for (VkImageView imageView : m_imageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(
                m_device,
                imageView,
                nullptr
            );
        }
    }
    m_imageViews.clear();
    m_images.clear();

    // Preserve previous swapchain handle for recycling.
    VkSwapchainKHR oldSwapchain = m_swapchain;
    m_swapchain = VK_NULL_HANDLE;

    Create(extent, oldSwapchain);

    // Destroy the retired old swapchain.
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(
            m_device,
            oldSwapchain,
            nullptr
        );
    }
}

void SwapChain::Create(VkExtent2D extent, VkSwapchainKHR oldSwapchain)
{
    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR capabilities{};

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_physicalDevice,
        m_surface,
        &capabilities
    ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to query surface capabilities."
        );
    }

    // Query available surface formats.
    uint32_t formatCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,
        m_surface,
        &formatCount,
        nullptr
    );

    if (formatCount == 0)
    {
        throw std::runtime_error(
            "No surface formats available."
        );
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,
        m_surface,
        &formatCount,
        formats.data()
    ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to retrieve surface formats."
        );
    }

    // Prefer BGRA8 with nonlinear sRGB color space.
    VkSurfaceFormatKHR surfaceFormat = formats.front();

    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = format;
            break;
        }
    }

    m_imageFormat = surfaceFormat.format;

    // Query presentation modes.
    uint32_t presentModeCount = 0;

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,
        m_surface,
        &presentModeCount,
        nullptr
    );

    if (presentModeCount == 0)
    {
        throw std::runtime_error(
            "No presentation modes available."
        );
    }

    std::vector<VkPresentModeKHR> presentModes(
        presentModeCount
    );

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,
        m_surface,
        &presentModeCount,
        presentModes.data()
    ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to retrieve presentation modes."
        );
    }

    // Prefer VSync.
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Choose the swapchain image extent.
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
    {
        m_extent = capabilities.currentExtent;
    }
    else
    {
        m_extent.width = std::clamp(
            extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );

        m_extent.height = std::clamp(
            extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );
    }

    // If window is minimized, extent may be 0x0; avoid invalid swapchain creation.
    if (m_extent.width == 0 || m_extent.height == 0)
    {
        return;
    }

    // Select the number of swapchain images.
    uint32_t imageCount = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // Configure swapchain creation.
    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType =
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_imageFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;

    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Graphics and presentation use the same queue family.
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.preTransform = capabilities.currentTransform;

    createInfo.compositeAlpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    if (vkCreateSwapchainKHR(
        m_device,
        &createInfo,
        nullptr,
        &m_swapchain
    ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create Vulkan swapchain."
        );
    }

    // Retrieve swapchain images.
    uint32_t actualImageCount = 0;

    if (vkGetSwapchainImagesKHR(
        m_device,
        m_swapchain,
        &actualImageCount,
        nullptr
    ) != VK_SUCCESS)
    {
        Destroy();

        throw std::runtime_error(
            "Failed to query swapchain images."
        );
    }

    m_images.resize(actualImageCount);

    if (vkGetSwapchainImagesKHR(
        m_device,
        m_swapchain,
        &actualImageCount,
        m_images.data()
    ) != VK_SUCCESS)
    {
        Destroy();

        throw std::runtime_error(
            "Failed to retrieve swapchain images."
        );
    }

    // Create one image view per swapchain image.
    m_imageViews.resize(m_images.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < m_images.size(); ++i)
    {
        VkImageViewCreateInfo viewInfo{};

        viewInfo.sType =
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        viewInfo.image = m_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_imageFormat;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(
            m_device,
            &viewInfo,
            nullptr,
            &m_imageViews[i]
        ) != VK_SUCCESS)
        {
            Destroy();

            throw std::runtime_error(
                "Failed to create swapchain image view."
            );
        }
    }
}

void SwapChain::Destroy()
{
    if (m_device != VK_NULL_HANDLE)
    {
        // Image views must be destroyed before the swapchain.
        for (VkImageView imageView : m_imageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(
                    m_device,
                    imageView,
                    nullptr
                );
            }
        }

        m_imageViews.clear();
        m_images.clear();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(
                m_device,
                m_swapchain,
                nullptr
            );

            m_swapchain = VK_NULL_HANDLE;
        }
    }

    m_imageFormat = VK_FORMAT_UNDEFINED;
    m_extent = { 0, 0 };

    m_physicalDevice = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
}