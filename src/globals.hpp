#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string_view>

namespace vkdemo::globals {
    static constexpr std::string_view kApplicationName    = "Vulkan Demo";
    static constexpr bool             kEnableVulkanLayers = true;
    static constexpr bool             kPrintDeviceCapabilities{true};
    static constexpr bool             kPrintDeviceMemoryProperties{true};
    static constexpr bool             kVSyncEnabled{false};
} // namespace vkdemo::globals

#endif // GLOBALS_HPP
