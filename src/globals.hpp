#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "glm-usage.hpp"

#include <string_view>
#include <vulkan/vulkan.hpp>

namespace vkdemo::globals {
    static constexpr std::string_view kApplicationName = "Vulkan Demo";
    static constexpr vk::Extent2D     kRenderExtent2D{1920, 1080};
    static constexpr vk::Extent3D     kRenderExtent3D{1920, 1080, 1};
    static constexpr glm::ivec2       kWindowSize{1280, 720};
    static constexpr bool             kEnableVulkanLayers = true;
    static constexpr bool             kPrintDeviceCapabilities{true};
    static constexpr bool             kPrintDeviceMemoryProperties{true};
    static constexpr bool             kVSyncEnabled{false};
    static constexpr uint32_t         kFramesInFlight{2};
    static constexpr vk::Format       kColorAttachmentFormat        = vk::Format::eB8G8R8A8Unorm;
    static constexpr vk::Format       kDepthStencilAttachmentFormat = vk::Format::eD32SfloatS8Uint;
    static constexpr vk::Format       kColorImageFormat             = vk::Format::eR8G8B8A8Unorm;
    static constexpr std::string_view kVertexEntryFunctionName      = "vs_main";
    static constexpr std::string_view kFragmentEntryFunctionName    = "fs_main";
    static constexpr std::string_view kComputeEntryFunctionName     = "main";
    static constexpr glm::vec3        kUpAxis{0, 1, 0};
    static constexpr float            kAspectRatio = 1920.0f / 1080.0f;

    static constexpr uint32_t kParticleCount = 1 << 10;

} // namespace vkdemo::globals

#endif // GLOBALS_HPP
