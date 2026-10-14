#pragma once
#include <vulkan/vulkan.h>

// Creates a buffer and allocates + binds memory for it.
void CreateBuffer(VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer &buffer,
                  VkDeviceMemory &memory);

// Allocates a one-time command buffer from vk_context.commandPool and starts recording.
VkCommandBuffer BeginSingleTimeCommands();

// Ends recording, submits to the graphics queue, waits for completion, frees the buffer.
void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

// Copies 'size' bytes from src to dst on the GPU (both buffers need the right TRANSFER usage bits).
void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

// Uploads 'data' into a new DEVICE_LOCAL buffer using a temporary staging buffer.
// 'usage' is the final usage (e.g. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
// TRANSFER_DST is added automatically.
void CreateDeviceLocalBuffer(const void *data,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkBuffer &buffer,
                             VkDeviceMemory &memory);