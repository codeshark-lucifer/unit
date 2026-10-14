#pragma once
#include "./vkcontext.h"
#include "SDL3/SDL.h"

bool CreateInstance(VkContext &ctx, SDL_Window *window);
bool CreateSurface(VkContext &ctx, SDL_Window *window);