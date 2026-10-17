#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string_view>

#include <glm/vec2.hpp>

namespace vkdemo::globals {
    static constexpr std::string_view kApplicationName = "Vulkan Demo";
    static constexpr glm::ivec2       kRenderExtent{1920, 1080};
    static constexpr glm::ivec2       kWindowSize{1280, 720};
    static constexpr bool             kEnableVulkanLayers = true;
    static constexpr bool             kPrintDeviceCapabilities{true};
    static constexpr bool             kPrintDeviceMemoryProperties{true};
    static constexpr bool             kVSyncEnabled{false};
} // namespace vkdemo::globals

#endif // GLOBALS_HPP
