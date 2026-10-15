#include "device-context.hpp"

//todo use Volk instead
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace vkpart {
    DeviceContext::DeviceContext() {
        // VULKAN_HPP_DEFAULT_DISPATCHER.init();
        create_instance();
        select_physical_device();

        create_logical_device();
        initialize_vma();
        create_window_and_swapchain();

        std::cout << "Created DeviceContext.\n";
    }

    DeviceContext::~DeviceContext() {
        m_device.destroy();
        m_instance.destroy();
        std::cout << "Destroyed DeviceContext.\n";
    }

    void DeviceContext::create_instance() {
        vk::ApplicationInfo app_info{
            kApplicationName.data(),
            vk::makeVersion(1, 0, 0),
            "No engine",
            vk::makeVersion(1, 0, 0),
            kVulkanAPIVersion,
            nullptr
        };

        // Create the application
        vk::InstanceCreateInfo instance_create_info{
            {},
            &app_info,
            0, nullptr,
            0,  nullptr
        };

        m_instance = vk::createInstance(instance_create_info);

        // VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    }

    void DeviceContext::select_physical_device() {
    }

    void DeviceContext::create_logical_device() {
    }

    void DeviceContext::initialize_vma() {
    }

    void DeviceContext::create_window_and_swapchain() {
    }
}
