#pragma once
#include "api.h"
#include "SDL3/SDL.h"
#include "Mathf/Mathf.h"

#include "vk/vkcontext.h"
#include "vk/pipeline.h"

extern bool engine_running;
extern bool screen_resized;
extern SDL_Window *window;

extern Pipeline defaultGraphicsPipeline;
extern int currentFrame;

extern "C"
{
    ENGINE_API bool engine_init(
        const char *title,
        Vec2 screen_size,
        bool fullscreen,
        bool resizeable);
    ENGINE_API bool engine_poll();
    ENGINE_API void engine_recreate_swapchain();
    ENGINE_API void engine_clean();
    ENGINE_API void engine_draw_frame();
}

void CreateVertexBuffer();
void CreateIndexBuffer();

void CreateSyncObjects();

void CreateCommandBuffers();
void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
void CreateCommandPool();

void CreateRenderPass();
void CreateFramebuffers();
void DestroyFramebuffers();