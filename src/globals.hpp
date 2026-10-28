#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "glm-usage.hpp"

#include <string_view>
#include <vulkan/vulkan.hpp>

/// @brief Contains constant values used throughout the application. Useful as a sort of "application" settings agglomeration.
/// @note The renderer uses a left handed coordinate system with +X = Right, +Y = Up and +Z = Forward
namespace vkdemo::globals {
    static constexpr std::string_view kApplicationName = "Vulkan Demo";
    static constexpr vk::Extent2D     kRenderExtent2D{1920, 1080};
    static constexpr vk::Extent3D     kRenderExtent3D{1920, 1080, 1};
    static constexpr glm::ivec2       kWindowSize{1280, 720};
    static constexpr bool             kEnableVulkanLayers = true;
    static constexpr bool             kPrintDeviceCapabilities{true};
    static constexpr bool             kPrintDeviceMemoryProperties{true};
    static constexpr bool             kVSyncEnabled{true};
    static constexpr uint32_t         kFramesInFlight{2};
    static constexpr vk::Format       kColorAttachmentFormat          = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format       kDepthStencilAttachmentFormat   = vk::Format::eD32SfloatS8Uint;
    static constexpr vk::Format       kColorImageFormat               = vk::Format::eR8G8B8A8Srgb;
    static constexpr std::string_view kSlangVertexEntryFunctionName   = "vs_main";
    static constexpr std::string_view kSlangFragmentEntryFunctionName = "fs_main";
    static constexpr std::string_view kSlangComputeEntryFunctionName  = "main";
    static constexpr std::string_view kGlslEntryFunctionName          = "main";
    static constexpr glm::vec3        kUpAxis{0, 1, 0};
    static constexpr float            kAspectRatio = 1920.0f / 1080.0f;

    static constexpr uint32_t kParticleCount = 1 << 12;
    static constexpr uint32_t kRNGSeed       = 7; // Lucky seven :)

    static constexpr float kMinOrbitRadius  = 2.4f;
    static constexpr float kMaxOrbitRadius  = 2.5f;
    static constexpr float kMinAngularSpeed = 0.2f;
    static constexpr float kMaxAngularSpeed = 6.0f;

    static constexpr uint8_t kDualKawaseBlurLevelCount = 3;

} // namespace vkdemo::globals

#endif // GLOBALS_HPP
