#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

// Description of one attachment that the Framebuffer should allocate itself.
struct FramebufferAttachmentDesc
{
    VkFormat              format  = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags     usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

// A single image + memory + view owned by the framebuffer.
struct FramebufferAttachment
{
    VkImage               image   = VK_NULL_HANDLE;
    VkDeviceMemory        memory  = VK_NULL_HANDLE;
    VkImageView           view    = VK_NULL_HANDLE;
    VkFormat              format  = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags     usage   = 0;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageAspectFlags    aspect  = 0;
};

// Wraps a VkFramebuffer. Two ways to use it:
//
//  1) Self-owned attachments (offscreen rendering, G-buffers, shadow maps):
//       fb.create(device, physDev, renderPass, w, h, { {colorFmt}, {depthFmt, depthUsage} });
//
//  2) Wrap externally owned views (e.g. swapchain images):
//       fb.createFromViews(device, renderPass, w, h, { swapchainView, depthView });
//
// The order of attachments must match the order in the VkRenderPass.
class Framebuffer
{
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    // Allocates images/memory/views and the VkFramebuffer. Returns false on failure.
    bool create(VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkRenderPass renderPass,
                uint32_t width,
                uint32_t height,
                const std::vector<FramebufferAttachmentDesc>& attachments,
                uint32_t layers = 1);

    // Wraps existing image views. The views are NOT owned or destroyed.
    bool createFromViews(VkDevice device,
                         VkRenderPass renderPass,
                         uint32_t width,
                         uint32_t height,
                         const std::vector<VkImageView>& views,
                         uint32_t layers = 1);

    // Recreates self-owned attachments at a new size (e.g. on window resize).
    // Only valid for framebuffers made with create(). Caller must make sure the
    // GPU is no longer using the old resources (vkDeviceWaitIdle or fences).
    bool resize(uint32_t width, uint32_t height);

    void destroy();

    VkFramebuffer handle()  const { return m_framebuffer; }
    VkRenderPass  renderPass() const { return m_renderPass; }
    VkExtent2D    extent()  const { return { m_width, m_height }; }
    uint32_t      width()   const { return m_width; }
    uint32_t      height()  const { return m_height; }
    uint32_t      layers()  const { return m_layers; }
    bool          valid()   const { return m_framebuffer != VK_NULL_HANDLE; }

    size_t attachmentCount() const { return m_views.size(); }
    VkImageView view(size_t index) const { return m_views[index]; }
    const FramebufferAttachment& attachment(size_t index) const { return m_attachments[index]; }

    // Convenience: viewport/scissor covering the whole framebuffer.
    VkViewport viewport() const;
    VkRect2D   scissor()  const;

private:
    bool buildAttachments();
    bool buildFramebuffer();
    void destroyAttachments();

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                   uint32_t typeBits,
                                   VkMemoryPropertyFlags properties);
    static VkImageAspectFlags aspectForFormat(VkFormat format);

    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkRenderPass     m_renderPass     = VK_NULL_HANDLE;
    VkFramebuffer    m_framebuffer    = VK_NULL_HANDLE;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    uint32_t m_layers = 1;

    bool m_ownsAttachments = false;

    std::vector<FramebufferAttachmentDesc> m_descs;       // only for owned attachments
    std::vector<FramebufferAttachment>     m_attachments; // only for owned attachments
    std::vector<VkImageView>               m_views;       // all views, in render pass order
};