#ifndef VULKAN_PARTICLES_GLOBALS_HPP
#define VULKAN_PARTICLES_GLOBALS_HPP

#include <string_view>

namespace vkdemo::globals {
    static constexpr std::string_view kApplicationName = "Vulkan Demo";
    static constexpr bool             kPrintDeviceCapabilities{true};
    static constexpr bool             kVSyncEnabled{false};
} // namespace vkdemo::globals

#endif // VULKAN_PARTICLES_GLOBALS_HPP
