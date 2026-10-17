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

        const std::vector<const char *> kRequiredInstanceExtensions = {};
        const std::vector<const char *> kRequiredDeviceExtensions   = {vk::KHRSwapchainExtensionName};

        void create_instance();
        void select_physical_device();
        void create_logical_device();
        void create_swapchain(const Window &window);
        void initialize_vma();

        vk::Instance                  m_instance;
        vk::PhysicalDevice            m_physical_device;
        vk::PhysicalDeviceProperties2 m_physical_device_properties;
        vk::Device                    m_device;
        uint32_t                      m_graphics_queue_family_index{};
        vk::Queue                     m_graphics_queue;
        vk::SurfaceKHR                m_surface;
        vk::SwapchainKHR              m_swapchain;
        std::vector<vk::Image>        m_swapchain_images{};
        std::vector<vk::ImageView>    m_swapchain_image_views{};
        VmaAllocator                  m_vma_allocator;
    };
} // namespace vkdemo

#endif // DEVICE_CONTEXT_HPP
