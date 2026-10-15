// Precompiled headers file
#ifndef PCH_HPP
#define PCH_HPP

// STL libs
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <string_view>

// Vulkan headers
// #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

// todo consider defining VULKAN_HPP_NO_EXCEPTIONS to remove exception throwing by Vulkan-HPP

// // VMA
// #include <vma/vk_mem_alloc.h>
//
// // SDL
// #include <SDL3/SDL.h>
// #include <SDL3/SDL_vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#endif // PCH_HPP