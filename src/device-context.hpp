#ifndef DEVICE_CONTEXT_HPP
#define DEVICE_CONTEXT_HPP

// For better autocompletion (recommendation by Khronos)
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

#include "vma-usage.hpp"

namespace vkdemo {
    class Window;
    class DeviceContext {
    public:
        DeviceContext() = default;
        virtual ~DeviceContext();

        void Initialize(const Window &window);

    private:
        static constexpr uint32_t          kVulkanAPIVersion    = vk::ApiVersion14;
        static constexpr vk::Format        kSwapchainFormat     = vk::Format::eB8G8R8A8Srgb;
        static constexpr vk::ColorSpaceKHR kSwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

        const std::vector<const char *> kRequiredInstanceExtensions = {vk::EXTDebugUtilsExtensionName};
        const std::vector<const char *> kRequiredDeviceExtensions   = {vk::KHRSwapchainExtensionName, vk::EXTMemoryBudgetExtensionName, vk::EXTHostImageCopyExtensionName,
                                                                       vk::EXTScalarBlockLayoutExtensionName};

        void create_instance();
        void select_physical_device();
        void create_logical_device();
        void read_physical_device_properties();
        void create_swapchain(const Window &window);
        void initialize_vma();

        vk::Instance               m_instance;
        vk::PhysicalDevice         m_physical_device;
        vk::Device                 m_device;
        uint32_t                   m_graphics_queue_family_index{};
        vk::Queue                  m_graphics_queue;
        vk::SurfaceKHR             m_surface;
        vk::SwapchainKHR           m_swapchain;
        std::vector<vk::Image>     m_swapchain_images{};
        std::vector<vk::ImageView> m_swapchain_image_views{};
        VmaAllocator               m_vma_allocator{nullptr};

        vk::DebugUtilsMessengerEXT m_debug_utils_messenger;

        vk::PhysicalDeviceProperties2               m_physical_device_properties{};
        vk::PhysicalDeviceMemoryProperties2         m_physical_device_memory_properties{};
        vk::PhysicalDeviceMemoryBudgetPropertiesEXT m_physical_device_memory_budget_properties{};
    };
} // namespace vkdemo

#endif // DEVICE_CONTEXT_HPP
