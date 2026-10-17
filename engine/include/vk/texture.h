#pragma once
#include <vulkan/vulkan.h>
#include <string>

struct Texture
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    uint32_t width, height, miplevels = 1;

    VkSampler sampler = VK_NULL_HANDLE;
};

Texture CreateTextureFromFile(const std::string &filepath);
void CreateTextureSampler(VkSampler &sampler);
void DestroyTexture(Texture &texture);