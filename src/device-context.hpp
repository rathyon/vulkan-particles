#ifndef VULKAN_PARTICLES_DEVICE_CONTEXT_HPP
#define VULKAN_PARTICLES_DEVICE_CONTEXT_HPP

namespace vkpart {

class DeviceContext {
public:
    DeviceContext();
    virtual ~DeviceContext();

private:
    static constexpr auto kVulkanAPIVersion = vk::ApiVersion14;
    static constexpr std::array kRequiredDeviceExtensions = {vk::KHRSwapchainExtensionName};
    static constexpr std::string_view kApplicationName = "Vulkan Particles";
    static constexpr vk::Format kSwapchainFormat = vk::Format::eB8G8R8A8Srgb;
    static constexpr vk::ColorSpaceKHR kSwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    void create_instance();
    void select_physical_device();
    void create_logical_device();
    void initialize_vma();
    void create_window_and_swapchain();

    vk::Instance m_instance;
    vk::PhysicalDevice m_physical_device;
    vk::PhysicalDeviceProperties2 m_physical_device_properties;
    uint32_t m_graphics_queue_family_index;
    vk::Device m_device;
    vk::Queue m_graphics_queue;
    VmaAllocator m_vma_allocator;
    SDL_Window* m_sdl_window;
    vk::SurfaceKHR m_surface;
    glm::ivec2 m_window_size;
    vk::SwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::ImageView> m_swapchain_image_views;
};

}

#endif //VULKAN_PARTICLES_DEVICE_CONTEXT_HPP
