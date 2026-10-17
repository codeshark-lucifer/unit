#include "graphics/renderer.h"
#include "ecs/built-in.h"
#include "vk/vkcontext.h"
#include "vk/framebuffer.h"
#include "graphics/type.h"
#include "engine/engine.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <mutex>

Registry registry;

ENGINE_API uint32_t GetOrRegisterComponentTypeId(const char *name)
{
    static std::mutex mutex;
    static std::unordered_map<std::string, uint32_t> componentIds;

    std::lock_guard<std::mutex> lock(mutex);
    auto it = componentIds.find(name);
    if (it != componentIds.end())
    {
        return it->second;
    }
    uint32_t newId = static_cast<uint32_t>(componentIds.size());
    componentIds[name] = newId;
    return newId;
}

void Renderer_Init()
{
    auto entities = registry.View<MeshRenderer, Transform>();
    for (Entity entity : entities)
    {
        Transform *transform = registry.Get<Transform>(entity);
        MeshRenderer *renderer = registry.Get<MeshRenderer>(entity);
        MeshFilter *filter = registry.Get<MeshFilter>(entity);
        if (!filter || !filter->shared_mesh)
        {
            std::cout << "Mesh Filter Required." << std::endl;
            return;
        }
        renderer->count = static_cast<uint32_t>(filter->shared_mesh->indices.size());

        // create vertex, index buffer
        CreateVertexBuffer(renderer->vertex, filter->shared_mesh->vertices);
        CreateIndexBuffer(renderer->index, filter->shared_mesh->indices);
    }
}

void UpdateUniformBuffer(Mat4 model, Mat4 view, Mat4 proj)
{
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    UniformBufferObject ubo{
        .model = model,
        .view = view,
        .proj = proj
    };

    memcpy(vk_context.uniforms[currentFrame].mapped, &ubo, sizeof(ubo));
}

void Renderer_Draw(VkCommandBuffer commandBuffer, const Framebuffer &framebuffer)
{
    VkClearValue clearColor{};
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vk_context.renderPass;
    renderPassInfo.framebuffer = framebuffer.handle();
    renderPassInfo.renderArea = framebuffer.scissor();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultGraphicsPipeline.usage);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            defaultGraphicsPipeline.layout, 0, 1,
                            &vk_context.descriptorSets[currentFrame], 0, nullptr);

    // Viewport and scissor are dynamic state in the pipeline, so they must be set every frame.
    VkViewport viewport = framebuffer.viewport();
    VkRect2D scissor = framebuffer.scissor();
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    auto cameraEntities = registry.View<Transform, Camera>();
    if (cameraEntities.empty())
    {
        vkCmdEndRenderPass(commandBuffer);
        return;
    }

    auto *cam_cmp = registry.Get<Camera>(cameraEntities[0]);
    auto *cam_trans = registry.Get<Transform>(cameraEntities[0]);
    if (!cam_cmp || !cam_trans)
    {
        vkCmdEndRenderPass(commandBuffer);
        return;
    }

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (h > 0)
    {
        cam_cmp->aspect = static_cast<float>(w) / static_cast<float>(h);
    }

    auto entities = registry.View<MeshRenderer, Transform>();
    for (Entity entity : entities)
    {
        Transform *transform = registry.Get<Transform>(entity);
        MeshRenderer *renderer = registry.Get<MeshRenderer>(entity);
        MeshFilter *filter = registry.Get<MeshFilter>(entity);
        if (!filter || !filter->shared_mesh)
        {
            continue;
        }

        // Lazy initialize vertex and index buffers if not yet created
        if (renderer->vertex.usage == VK_NULL_HANDLE)
        {
            renderer->count = static_cast<uint32_t>(filter->shared_mesh->indices.size());
            CreateVertexBuffer(renderer->vertex, filter->shared_mesh->vertices);
            CreateIndexBuffer(renderer->index, filter->shared_mesh->indices);
        }

        if (renderer->count == 0)
        {
            continue;
        }

        UpdateUniformBuffer(
            transform->GetMatrix(),
            cam_cmp->GetViewMatrix(
                cam_trans->position,
                cam_trans->rotation),
            cam_cmp->GetProjectionMatrix());

        VkBuffer vertexBuffers[] = {renderer->vertex.usage};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, renderer->index.usage, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(renderer->count), 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffer);
}

void Renderer_Clean()
{
    VkDevice device = vk_context.device.GetDevice();

    auto entities = registry.View<MeshRenderer>();
    for (Entity entity : entities)
    {
        MeshRenderer *renderer = registry.Get<MeshRenderer>(entity);
        if (!renderer)
            continue;

        // clean vertex, index buffer
        if (renderer->vertex.usage != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, renderer->vertex.usage, nullptr);
            renderer->vertex.usage = VK_NULL_HANDLE;
        }
        if (renderer->vertex.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, renderer->vertex.memory, nullptr);
            renderer->vertex.memory = VK_NULL_HANDLE;
        }

        if (renderer->index.usage != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, renderer->index.usage, nullptr);
            renderer->index.usage = VK_NULL_HANDLE;
        }
        if (renderer->index.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, renderer->index.memory, nullptr);
            renderer->index.memory = VK_NULL_HANDLE;
        }
    }
}
