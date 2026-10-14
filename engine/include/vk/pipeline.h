#pragma once
#include <vulkan/vulkan.h>

struct Pipeline
{
    VkPipelineLayout layout;
    VkPipeline usage;
};

void CreateGraphicsPipeline(Pipeline &pipeline);