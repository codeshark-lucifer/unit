#include "vk/framebuffer.h"

#include <utility>

Framebuffer::~Framebuffer()
{
    destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
{
    *this = std::move(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        destroy();

        m_device           = std::exchange(other.m_device, VK_NULL_HANDLE);
        m_physicalDevice   = std::exchange(other.m_physicalDevice, VK_NULL_HANDLE);
        m_renderPass       = std::exchange(other.m_renderPass, VK_NULL_HANDLE);
        m_framebuffer      = std::exchange(other.m_framebuffer, VK_NULL_HANDLE);
        m_width            = std::exchange(other.m_width, 0u);
        m_height           = std::exchange(other.m_height, 0u);
        m_layers           = std::exchange(other.m_layers, 1u);
        m_ownsAttachments  = std::exchange(other.m_ownsAttachments, false);
        m_descs            = std::move(other.m_descs);
        m_attachments      = std::move(other.m_attachments);
        m_views            = std::move(other.m_views);

        other.m_descs.clear();
        other.m_attachments.clear();
        other.m_views.clear();
    }
    return *this;
}

bool Framebuffer::create(VkDevice device,
                         VkPhysicalDevice physicalDevice,
                         VkRenderPass renderPass,
                         uint32_t width,
                         uint32_t height,
                         const std::vector<FramebufferAttachmentDesc>& attachments,
                         uint32_t layers)
{
    destroy();

    m_device          = device;
    m_physicalDevice  = physicalDevice;
    m_renderPass      = renderPass;
    m_width           = width;
    m_height          = height;
    m_layers          = layers;
    m_descs           = attachments;
    m_ownsAttachments = true;

    if (!buildAttachments() || !buildFramebuffer())
    {
        destroy();
        return false;
    }
    return true;
}

bool Framebuffer::createFromViews(VkDevice device,
                                  VkRenderPass renderPass,
                                  uint32_t width,
                                  uint32_t height,
                                  const std::vector<VkImageView>& views,
                                  uint32_t layers)
{
    destroy();

    m_device          = device;
    m_renderPass      = renderPass;
    m_width           = width;
    m_height          = height;
    m_layers          = layers;
    m_ownsAttachments = false;
    m_views           = views;

    if (!buildFramebuffer())
    {
        destroy();
        return false;
    }
    return true;
}

bool Framebuffer::resize(uint32_t width, uint32_t height)
{
    if (!m_ownsAttachments || m_device == VK_NULL_HANDLE)
        return false;

    if (width == m_width && height == m_height && valid())
        return true;

    if (m_framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }
    destroyAttachments();

    m_width  = width;
    m_height = height;

    if (!buildAttachments() || !buildFramebuffer())
    {
        destroy();
        return false;
    }
    return true;
}

void Framebuffer::destroy()
{
    if (m_device != VK_NULL_HANDLE)
    {
        if (m_framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);

        destroyAttachments();
    }

    m_framebuffer = VK_NULL_HANDLE;
    m_views.clear();
    m_attachments.clear();
    m_descs.clear();
    m_ownsAttachments = false;
    m_width = m_height = 0;
    m_layers = 1;
}

VkViewport Framebuffer::viewport() const
{
    VkViewport vp{};
    vp.x        = 0.0f;
    vp.y        = 0.0f;
    vp.width    = static_cast<float>(m_width);
    vp.height   = static_cast<float>(m_height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    return vp;
}

VkRect2D Framebuffer::scissor() const
{
    return { { 0, 0 }, { m_width, m_height } };
}

bool Framebuffer::buildAttachments()
{
    m_attachments.clear();
    m_views.clear();
    m_attachments.reserve(m_descs.size());
    m_views.reserve(m_descs.size());

    for (const FramebufferAttachmentDesc& desc : m_descs)
    {
        FramebufferAttachment att{};
        att.format  = desc.format;
        att.usage   = desc.usage;
        att.samples = desc.samples;
        att.aspect  = aspectForFormat(desc.format);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.format        = desc.format;
        imageInfo.extent        = { m_width, m_height, 1 };
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = m_layers;
        imageInfo.samples       = desc.samples;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage         = desc.usage;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_device, &imageInfo, nullptr, &att.image) != VK_SUCCESS)
            return false;

        // Push now so destroyAttachments() cleans up partial state on failure.
        m_attachments.push_back(att);
        FramebufferAttachment& stored = m_attachments.back();

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(m_device, stored.image, &memReq);

        uint32_t typeIndex = findMemoryType(m_physicalDevice,
                                            memReq.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (typeIndex == UINT32_MAX)
            return false;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReq.size;
        allocInfo.memoryTypeIndex = typeIndex;

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stored.memory) != VK_SUCCESS)
            return false;

        if (vkBindImageMemory(m_device, stored.image, stored.memory, 0) != VK_SUCCESS)
            return false;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = stored.image;
        viewInfo.viewType                        = (m_layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                                                  : VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = desc.format;
        viewInfo.subresourceRange.aspectMask     = stored.aspect;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = m_layers;

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &stored.view) != VK_SUCCESS)
            return false;

        m_views.push_back(stored.view);
    }

    return true;
}

bool Framebuffer::buildFramebuffer()
{
    if (m_renderPass == VK_NULL_HANDLE || m_views.empty())
        return false;

    VkFramebufferCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass      = m_renderPass;
    info.attachmentCount = static_cast<uint32_t>(m_views.size());
    info.pAttachments    = m_views.data();
    info.width           = m_width;
    info.height          = m_height;
    info.layers          = m_layers;

    return vkCreateFramebuffer(m_device, &info, nullptr, &m_framebuffer) == VK_SUCCESS;
}

void Framebuffer::destroyAttachments()
{
    if (m_device == VK_NULL_HANDLE)
        return;

    for (FramebufferAttachment& att : m_attachments)
    {
        if (att.view != VK_NULL_HANDLE)   vkDestroyImageView(m_device, att.view, nullptr);
        if (att.image != VK_NULL_HANDLE)  vkDestroyImage(m_device, att.image, nullptr);
        if (att.memory != VK_NULL_HANDLE) vkFreeMemory(m_device, att.memory, nullptr);
        att = {};
    }

    if (m_ownsAttachments)
    {
        m_attachments.clear();
        m_views.clear();
    }
}

uint32_t Framebuffer::findMemoryType(VkPhysicalDevice physicalDevice,
                                     uint32_t typeBits,
                                     VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return UINT32_MAX;
}

VkImageAspectFlags Framebuffer::aspectForFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}