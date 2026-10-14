#include "engine/engine.h"
#include "vk/vkcontext.h"

#include "vk/instance.h"
#include "vk/debug.h"
#include "vk/vertex.h"
#include "vk/buffer.h"

#include <iostream>
#include <vector>
#include <stdexcept>

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 0: top-left
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // 1: top-right
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   // 2: bottom-right
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // 3: bottom-left
};

const std::vector<uint16_t> indices = {
    0, 1, 2, // first triangle
    0, 2, 3  // second triangle
};

bool engine_running = true;
bool screen_resized = false;
SDL_Window *window = nullptr;
VkContext vk_context;
Pipeline defaultGraphicsPipeline;
int currentFrame = 0;

// One "render finished" semaphore PER SWAPCHAIN IMAGE (not per frame in flight).
// The presentation engine may still be waiting on a semaphore after the frame's
// fence has signaled, so these are indexed by the acquired image index.
static std::vector<VkSemaphore> renderFinished;

static void CreateRenderFinishedSemaphores()
{
    VkDevice device = vk_context.device.GetDevice();
    const size_t imageCount = vk_context.swapchain.GetImageViews().size();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    renderFinished.assign(imageCount, VK_NULL_HANDLE);
    for (size_t i = 0; i < imageCount; ++i)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinished[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render finished semaphore!");
        }
    }
}

static void DestroyRenderFinishedSemaphores()
{
    VkDevice device = vk_context.device.GetDevice();
    for (VkSemaphore semaphore : renderFinished)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
    renderFinished.clear();
}

bool engine_init(
    const char *title,
    Vec2 screen_size,
    bool fullscreen,
    bool resizeable)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        return false;
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    if (fullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    if (resizeable)
    {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    window = SDL_CreateWindow(
        title,
        static_cast<int>(screen_size.x),
        static_cast<int>(screen_size.y),
        flags);

    if (window == nullptr)
    {
        SDL_Quit();
        return false;
    }

    if (!CreateInstance(vk_context, window))
    {
        std::cout << "Failed to create instance.\n";
        return false;
    }

    SetupDebugMessenger(vk_context.instance, vk_context.debugMessenger);

    if (!CreateSurface(vk_context, window))
    {
        std::cout << "Failed to create surface.\n";
        return false;
    }

    if (!vk_context.device.Init(vk_context.instance, vk_context.surface))
    {
        std::cout << "Failed to create device.\n";
        return false;
    }

    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &width, &height) || width <= 0 || height <= 0)
    {
        width = static_cast<int>(screen_size.x);
        height = static_cast<int>(screen_size.y);
    }

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    vk_context.swapchain.Init(
        vk_context.device.GetPhysicalDevice(),
        vk_context.device.GetDevice(),
        vk_context.surface,
        extent);

    CreateRenderPass();
    CreateGraphicsPipeline(defaultGraphicsPipeline);
    CreateFramebuffers();

    CreateCommandPool();
    CreateCommandBuffers();

    CreateSyncObjects();
    CreateRenderFinishedSemaphores();

    CreateVertexBuffer();
    CreateIndexBuffer();

    return true;
}

void engine_recreate_swapchain()
{
    if (window == nullptr || !vk_context.device.IsValid())
    {
        return;
    }

    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &width, &height))
    {
        return;
    }

    // If the window is minimized or has zero area, pause recreation until restored.
    if (width <= 0 || height <= 0)
    {
        return;
    }

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    try
    {
        vkDeviceWaitIdle(vk_context.device.GetDevice());
        DestroyFramebuffers();
        DestroyRenderFinishedSemaphores();
        vk_context.swapchain.Recreate(extent);
        CreateFramebuffers();
        CreateRenderFinishedSemaphores();
        screen_resized = false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to recreate swapchain: " << e.what() << std::endl;
    }
}

bool engine_poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            engine_running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            screen_resized = true;
            break;

        default:
            break;
        }
    }

    if (screen_resized)
    {
        engine_recreate_swapchain();
    }

    return engine_running;
}

void engine_clean()
{
    if (vk_context.device.IsValid())
    {
        vkDeviceWaitIdle(vk_context.device.GetDevice());

        VkDevice device = vk_context.device.GetDevice();

        DestroyRenderFinishedSemaphores();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, vk_context.frames[i].imageAvailable, nullptr);
            vkDestroyFence(device, vk_context.frames[i].inFlight, nullptr);
        }

        vkDestroyCommandPool(device, vk_context.commandPool, nullptr);

        DestroyFramebuffers();
        vkDestroyPipeline(device, defaultGraphicsPipeline.usage, nullptr);
        vkDestroyPipelineLayout(device, defaultGraphicsPipeline.layout, nullptr);
        vkDestroyRenderPass(device, vk_context.renderPass, nullptr);

        vk_context.swapchain.Destroy();

        vkDestroyBuffer(device, vk_context.indexBuffer, nullptr);
        vkFreeMemory(device, vk_context.indexBufferMemory, nullptr);
        vkDestroyBuffer(device, vk_context.vertexBuffer, nullptr);
        vkFreeMemory(device, vk_context.vertexBufferMemory, nullptr);

        vk_context.device.Shutdown();
    }

    if (vk_context.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(
            vk_context.instance,
            vk_context.surface,
            nullptr);
        vk_context.surface = VK_NULL_HANDLE;
    }

    if (validation_enable && vk_context.debugMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(
            vk_context.instance,
            vk_context.debugMessenger,
            nullptr);
        vk_context.debugMessenger = VK_NULL_HANDLE;
    }

    if (vk_context.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(
            vk_context.instance,
            nullptr);
        vk_context.instance = VK_NULL_HANDLE;
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

void engine_draw_frame()
{
    VkDevice device = vk_context.device.GetDevice();
    auto &frame = vk_context.frames[currentFrame];
    VkQueue queue = vk_context.device.GetGraphicsQueue().queue;

    // Wait until the GPU is done with the work this frame slot submitted last time.
    vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device,
        vk_context.swapchain.GetSwapchain(),
        UINT64_MAX,
        frame.imageAvailable,
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Nothing was submitted, so the fence is still signaled. Safe to return.
        engine_recreate_swapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swapchain image!");
    }

    // Only reset the fence once we are sure we will submit work that signals it.
    vkResetFences(device, 1, &frame.inFlight);

    vkResetCommandBuffer(frame.cmd, 0);
    RecordCommandBuffer(frame.cmd, imageIndex);

    VkSemaphore waitSemaphores[] = {frame.imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinished[imageIndex]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(queue, 1, &submitInfo, frame.inFlight) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapchains[] = {vk_context.swapchain.GetSwapchain()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    // Note: this assumes the graphics queue can also present (true on virtually all GPUs).
    // If your Device exposes a separate present queue, use that here instead.
    result = vkQueuePresentKHR(queue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || screen_resized)
    {
        engine_recreate_swapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swapchain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CreateVertexBuffer()
{
    VkDevice device = vk_context.device.GetDevice();
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

    CreateBuffer(size,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vk_context.vertexBuffer,
                 vk_context.vertexBufferMemory);

    void *data;
    vkMapMemory(device, vk_context.vertexBufferMemory, 0, size, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device, vk_context.vertexBufferMemory);
}

void CreateIndexBuffer()
{
    VkDevice device = vk_context.device.GetDevice();
    VkDeviceSize size = sizeof(indices[0]) * indices.size();

    CreateBuffer(size,
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vk_context.indexBuffer,
                 vk_context.indexBufferMemory);

    void *data;
    vkMapMemory(device, vk_context.indexBufferMemory, 0, size, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(size));
    vkUnmapMemory(device, vk_context.indexBufferMemory);
}

void CreateSyncObjects()
{
    VkDevice device = vk_context.device.GetDevice();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Start signaled so the very first vkWaitForFences does not block forever.
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &vk_context.frames[i].imageAvailable) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &vk_context.frames[i].inFlight) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create sync objects!");
        }
    }
}

void CreateCommandBuffers()
{
    VkDevice device = vk_context.device.GetDevice();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vk_context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkAllocateCommandBuffers(device, &allocInfo, &vk_context.frames[i].cmd) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
}

void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    const Framebuffer &framebuffer = vk_context.framebuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

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

    // Viewport and scissor are dynamic state in the pipeline, so they must be set every frame.
    VkViewport viewport = framebuffer.viewport();
    VkRect2D scissor = framebuffer.scissor();
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vk_context.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vk_context.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = vk_context.device.GetQueueFamilies();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics.value();

    if (vkCreateCommandPool(vk_context.device.GetDevice(), &poolInfo, nullptr, &vk_context.commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vk_context.swapchain.GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Makes the layout transition wait until the swapchain image has actually been acquired.
    // (Subpass dependencies belong to the render pass, not to vkCmdBeginRenderPass.)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vk_context.device.GetDevice(), &renderPassInfo, nullptr, &vk_context.renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void CreateFramebuffers()
{
    const auto &imageViews = vk_context.swapchain.GetImageViews();
    VkExtent2D extent = vk_context.swapchain.GetExtent();
    VkDevice device = vk_context.device.GetDevice();

    vk_context.framebuffers.clear();
    vk_context.framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        std::vector<VkImageView> attachments = {imageViews[i]};
        if (!vk_context.framebuffers[i].createFromViews(
                device,
                vk_context.renderPass,
                extent.width,
                extent.height,
                attachments))
        {
            throw std::runtime_error("failed to create framebuffer for swapchain image!");
        }
    }
}

void DestroyFramebuffers()
{
    for (auto &fb : vk_context.framebuffers)
    {
        fb.destroy();
    }
    vk_context.framebuffers.clear();
}