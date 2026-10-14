#include "vk/buffer.h"

#include "vk/vkcontext.h"

#include <cstring>
#include <stdexcept>

void CreateBuffer(VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer &buffer,
                  VkDeviceMemory &memory)
{
    VkDevice device = vk_context.device.GetDevice();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    uint32_t typeIndex = vk_context.device.FindMemoryType(memRequirements.memoryTypeBits, properties);
    if (typeIndex == UINT32_MAX)
    {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, memory, 0);
}

VkCommandBuffer BeginSingleTimeCommands()
{
    VkDevice device = vk_context.device.GetDevice();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vk_context.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate single-time command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device, vk_context.commandPool, 1, &commandBuffer);
        throw std::runtime_error("failed to begin single-time command buffer!");
    }

    return commandBuffer;
}

void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    VkDevice device = vk_context.device.GetDevice();
    VkQueue queue = vk_context.device.GetGraphicsQueue().queue;

    VkResult result = vkEndCommandBuffer(commandBuffer);

    if (result == VK_SUCCESS)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result == VK_SUCCESS)
        {
            // Simple and fine for startup uploads. For many uploads per frame,
            // use a fence instead so transfers can overlap.
            vkQueueWaitIdle(queue);
        }
    }

    vkFreeCommandBuffers(device, vk_context.commandPool, 1, &commandBuffer);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit single-time command buffer!");
    }
}

void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

void CreateDeviceLocalBuffer(const void *data,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkBuffer &buffer,
                             VkDeviceMemory &memory)
{
    VkDevice device = vk_context.device.GetDevice();

    // 1) Staging buffer: CPU-visible, only used as the copy source.
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    CreateBuffer(size,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingMemory);

    void *mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    std::memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMemory);

    try
    {
        // 2) Final buffer: lives in VRAM, can be a copy destination.
        CreateBuffer(size,
                     usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     buffer,
                     memory);

        // 3) GPU-side copy, blocks until finished.
        CopyBuffer(stagingBuffer, buffer, size);
    }
    catch (...)
    {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        throw;
    }

    // 4) The staging buffer has done its job.
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}