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
// For better autocompletion
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

// VMA - Vulkan Memory Allocator
#include <vma/vk_mem_alloc.h>

// SDL for Windowing
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#endif // PCH_HPP