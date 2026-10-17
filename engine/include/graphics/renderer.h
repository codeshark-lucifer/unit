#pragma once
#include <vulkan/vulkan.h>
#include "ecs/registry.h"
#include "api.h"

class Framebuffer;

extern ENGINE_API Registry registry;

void Renderer_Init();
void Renderer_Draw(VkCommandBuffer commandBuffer,const Framebuffer& framebuffer);
void Renderer_Clean();