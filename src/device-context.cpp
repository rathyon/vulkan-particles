#include "device-context.hpp"

#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

namespace vkpart {
    DeviceContext::DeviceContext() {
        create_instance();
        select_physical_device();
        create_logical_device();
        initialize_vma();
        create_window_and_swapchain();
    }

    DeviceContext::~DeviceContext() {
        m_device.destroy();
        m_instance.destroy();
    }

    void DeviceContext::create_instance() {
        // Volk is used for linking with the Vulkan loader
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan device context: Vulkan loader is missing.");
        }

        // Create the application
        vk::ApplicationInfo app_info{kApplicationName.data(), vk::makeVersion(1, 0, 0), "No engine", vk::makeVersion(1, 0, 0), kVulkanAPIVersion, nullptr};

        vk::InstanceCreateInfo instance_create_info{{}, &app_info, 0, nullptr, 0, nullptr};

        m_instance = vk::createInstance(instance_create_info);
        volkLoadInstance(m_instance);
    }

    void DeviceContext::select_physical_device() {
        const std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();

        // List available GPUs
        std::cout << "Available devices:" << '\n';
        for (const vk::PhysicalDevice &physical_device: physical_devices) {
            const auto &properties = physical_device.getProperties2();
            std::cout << properties.properties.deviceName << '\n';
        }

        // Select the first one: my PC only has 1 GPU
        m_physical_device            = physical_devices[0];
        m_physical_device_properties = m_physical_device.getProperties2();

        std::cout << "Selected device: " << m_physical_device_properties.properties.deviceName << "\n";

        // Find required queue families
        std::vector<vk::QueueFamilyProperties2> queue_families_properties = m_physical_device.getQueueFamilyProperties2();

        // List queue families
        std::cout << "Available queue families:" << '\n';
        for (const auto &family_properties: queue_families_properties) {
            std::cout << "Queue Count: " << family_properties.queueFamilyProperties.queueCount << " Flags: " << vk::to_string(family_properties.queueFamilyProperties.queueFlags) << "\n";
        }

        // todo select and store compute and transfer queues!

        // Select queue families
        uint32_t queue_family_index = 0;
        for (const auto &family_properties: queue_families_properties) {
            // Just grab the first compatible queue family for Graphics
            if (family_properties.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                m_graphics_queue_family_index = queue_family_index;
                break;
            }

            // Optimization point: select ideal queue family for exclusive use (e.g. Compute or Transfer)
            ++queue_family_index;
        }
    }

    void DeviceContext::create_logical_device() {
        const std::vector         queue_priorities{1.0f};
        vk::DeviceQueueCreateInfo graphics_queue_create_info{{}, m_graphics_queue_family_index, queue_priorities};

        vk::PhysicalDeviceVulkan13Features enabled_vk13_features{};
        enabled_vk13_features.synchronization2 = true;
        enabled_vk13_features.dynamicRendering = true;

        vk::PhysicalDeviceVulkan12Features enabled_vk12_features{};

        // Descriptor indexing features
        enabled_vk12_features.descriptorIndexing                        = true;
        enabled_vk12_features.shaderSampledImageArrayNonUniformIndexing = true;
        enabled_vk12_features.runtimeDescriptorArray                    = true;

        // todo review why this is required
        enabled_vk12_features.descriptorBindingVariableDescriptorCount = true;

        // Lets us access buffers via pointers instead of going through descriptors
        enabled_vk12_features.bufferDeviceAddress = true;

        enabled_vk12_features.pNext = &enabled_vk13_features;

        vk::PhysicalDeviceFeatures2 enabled_features{};
        enabled_features.features.samplerAnisotropy = true;

        vk::DeviceCreateInfo device_create_info{};
        device_create_info.pNext = enabled_vk12_features;
        device_create_info.setQueueCreateInfos({graphics_queue_create_info});
        device_create_info.setPEnabledExtensionNames(kRequiredDeviceExtensions);

        m_device         = m_physical_device.createDevice(device_create_info);
        m_graphics_queue = m_device.getQueue2(vk::DeviceQueueInfo2{{}, m_graphics_queue_family_index});

        // This program uses a single device, so entrypoints can be loaded directly from the driver
        volkLoadDevice(m_device);
    }

    void DeviceContext::initialize_vma() {
        // todo investigate how to initialize VMA with volk
    }

    void DeviceContext::create_window_and_swapchain() {

        // m_sdl_window = SDL_CreateWindow(kApplicationName, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        // if (m_sdl_window == nullptr) {
        //     throw std::runtime_error("Failed to create SDL window!");
        // }
        //
        // if (SDL_Vulkan_CreateSurface(m_sdl_window, m_instance, nullptr, reinterpret_cast<VkSurfaceKHR *>(&m_surface)) == false) {
        //     throw std::runtime_error("Failed to create surface from SDL window!");
        // }
        // if (SDL_GetWindowSize(m_sdl_window, &m_window_size.x, &m_window_size.y) == false) {
        //     throw std::runtime_error("Failed to get window size from SDL window!");
        // }
        //
        // vk::SurfaceCapabilities2KHR surface_capabilities = m_physical_device.getSurfaceCapabilities2KHR({m_surface});
        // vk::SwapchainCreateInfoKHR  swapchain_create_info{};
        // swapchain_create_info.surface          = m_surface;
        // swapchain_create_info.minImageCount    = surface_capabilities.surfaceCapabilities.minImageCount;
        // swapchain_create_info.imageFormat      = kSwapchainFormat;
        // swapchain_create_info.imageColorSpace  = kSwapchainColorSpace;
        // swapchain_create_info.imageExtent      = vk::Extent2D{surface_capabilities.surfaceCapabilities.currentExtent.width, surface_capabilities.surfaceCapabilities.currentExtent.height};
        // swapchain_create_info.imageArrayLayers = 1;
        // swapchain_create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
        // swapchain_create_info.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        // swapchain_create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        //
        // swapchain_create_info.presentMode = vk::PresentModeKHR::eImmediate; // Uncapped framerate, screen tearing
        // // swapchain_create_info.presentMode = vk::PresentModeKHR::eFifo; // V-SYNC, no screen tearing
        //
        // m_swapchain        = m_device.createSwapchainKHR(swapchain_create_info);
        // m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);
        //
        // vk::ImageViewCreateInfo image_view_create_info{};
        // image_view_create_info.viewType         = vk::ImageViewType::e2D;
        // image_view_create_info.format           = kSwapchainFormat;
        // image_view_create_info.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        //
        // for (const auto &swapchain_image: m_swapchain_images) {
        //     image_view_create_info.image = swapchain_image;
        //     m_swapchain_image_views.emplace_back(m_device.createImageView(image_view_create_info));
        // }
    }
} // namespace vkpart
