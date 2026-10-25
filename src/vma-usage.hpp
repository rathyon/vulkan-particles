/// As per the VMA documentation:
/// This library includes header <vulkan/vulkan.h>, which in turn includes <windows.h> on Windows.
/// If you need some specific macros defined before including these headers (like WIN32_LEAN_AND_MEAN or WINVER for Windows, VK_USE_PLATFORM_WIN32_KHR for Vulkan),
/// you must define them before every #include of this library.
/// It may be a good idea to create a dedicate header file for this purpose, e.g. "VmaUsage.h", that will be included in other source files instead of VMA header directly.

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
