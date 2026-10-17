#pragma once
#include <vulkan/vulkan.h>

struct Pipeline
{
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline usage = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor = VK_NULL_HANDLE;
};

void CreateGraphicsPipeline(Pipeline &pipeline);
void CreateDescriptorSetLayout(Pipeline& pipeline);

void CreateDescriptorPool();