#include "vk/instance.h"
#include "vk/debug.h"
#include "SDL3/SDL_vulkan.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static const char *kValidationLayer = "VK_LAYER_KHRONOS_validation";

static bool IsLayerSupported(const char *layerName)
{
#ifdef _WIN32
    // If VK_LAYER_PATH is not set, set it from VULKAN_SDK so the Vulkan loader can find Khronos validation layers
    if (!std::getenv("VK_LAYER_PATH"))
    {
        const char *vulkanSdk = std::getenv("VULKAN_SDK");
        if (vulkanSdk)
        {
            std::string layerPath = std::string(vulkanSdk) + "\\Bin";
            SetEnvironmentVariableA("VK_LAYER_PATH", layerPath.c_str());
        }
    }
#endif

    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    for (const VkLayerProperties &layer : layers)
    {
        if (std::strcmp(layer.layerName, layerName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool CreateInstance(VkContext &ctx, SDL_Window *window)
{
    (void)window; // not needed for instance creation

    // Extensions required by SDL for surface creation (SDL3 uses Uint32 count)
    Uint32 sdl_extensions_count = 0;
    const char *const *sdl_extensions =
        SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

    if (sdl_extensions == nullptr)
    {
        std::fprintf(stderr, "SDL_Vulkan_GetInstanceExtensions failed\n");
        return false;
    }

    std::vector<const char *> extensions(
        sdl_extensions,
        sdl_extensions + sdl_extensions_count);

    // Layers & Debug Messenger for instance creation
    std::vector<const char *> layers;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (validation_enable)
    {
        if (IsLayerSupported(kValidationLayer))
        {
            layers.push_back(kValidationLayer);
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            PopulateDebugMessengerCreateInfo(debugCreateInfo);
        }
        else
        {
            std::fprintf(stderr, "Validation layer requested but not available\n");
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Unit";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "UnitEngine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = (validation_enable && !layers.empty()) ? &debugCreateInfo : nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &ctx.instance);
    if (result != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkCreateInstance failed: %d\n", static_cast<int>(result));
        ctx.instance = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

bool CreateSurface(VkContext &ctx, SDL_Window *window)
{
    // SDL3 signature: (window, instance, allocator, &surface)
    if (!SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &ctx.surface))
    {
        std::fprintf(stderr, "SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}