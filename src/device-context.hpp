#ifndef DEVICE_CONTEXT_HPP
#define DEVICE_CONTEXT_HPP

// For better autocompletion (recommendation by Khronos)
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

#include "vma-usage.hpp"

#include <optional>

namespace vkdemo {

    /// @brief Class that represents the Vulkan context for a given GPU. Handles initializing the Vulkan library, Vulkan extensions and choosing a (single) GPU.
    /// This context object is passed onto most objects that interact with vulkan as a dependency-injection style pattern.
    class DeviceContext {
    public:
        DeviceContext() = default;
        virtual ~DeviceContext();

        void Initialize();

        [[nodiscard]] std::optional<uint32_t> GetMemoryType(uint32_t memory_type_bits, vk::MemoryPropertyFlags memory_property_flags) const;

        vk::Instance       Instance;
        vk::PhysicalDevice PhysicalDevice;
        vk::Device         Device;

        std::array<uint32_t, 4>  QueueFamilies{};
        std::array<vk::Queue, 4> Queues{};

        VmaAllocator VmaAllocator{nullptr};

    private:
        static constexpr uint32_t kVulkanAPIVersion = vk::ApiVersion14;

        const std::vector<const char *> kRequiredLayers             = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> kRequiredInstanceExtensions = {vk::EXTDebugUtilsExtensionName};
        const std::vector<const char *> kRequiredDeviceExtensions   = {
                vk::KHRSwapchainExtensionName,
                vk::EXTMemoryBudgetExtensionName,
                vk::EXTHostImageCopyExtensionName,        // Required for image copies on the host
                vk::EXTScalarBlockLayoutExtensionName,    // Required for scalar block layouts (no padding)
                vk::KHRShaderDrawParametersExtensionName, // Required for using special semantic values in Slang shaders
                vk::KHRMaintenance5ExtensionName,         // Slang current doesn't allow writing PointSize, so this must be enabled to compensate (still can't write to it!)
                vk::KHRShaderRelaxedExtendedInstructionExtensionName, // Required for added debug info using forward references to pointers
        };

        void               create_instance();
        [[nodiscard]] bool check_layer_support();
        void               select_physical_device();
        [[nodiscard]] bool check_device_extension_support(const vk::PhysicalDevice &physical_device);
        void               create_logical_device();
        void               select_queue_families();
        void               read_physical_device_properties();
        void               initialize_vma();

        std::vector<vk::LayerProperties>     m_layer_properties;
        std::vector<vk::ExtensionProperties> m_device_extensions;
        vk::DebugUtilsMessengerEXT           m_debug_utils_messenger;

        vk::PhysicalDeviceProperties2               m_physical_device_properties{};
        vk::PhysicalDeviceMemoryProperties2         m_physical_device_memory_properties{};
        vk::PhysicalDeviceMemoryBudgetPropertiesEXT m_physical_device_memory_budget_properties{};
    };
} // namespace vkdemo

#endif // DEVICE_CONTEXT_HPP
