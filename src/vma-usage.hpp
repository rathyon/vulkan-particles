#ifndef VMA_USAGE_HPP
#define VMA_USAGE_HPP

// suppress clang warnings for VMA (clang-cl)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

#define VK_USE_PLATFORM_WIN32_KHR
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vma/vk_mem_alloc.h>

#pragma clang diagnostic pop

#endif // VMA_USAGE_HPP
