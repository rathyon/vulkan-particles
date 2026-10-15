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
    vk::Device m_device;
};

}

#endif //VULKAN_PARTICLES_DEVICE_CONTEXT_HPP
