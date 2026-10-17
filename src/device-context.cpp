#include "device-context.hpp"
#include <iostream>

#include "globals.hpp"
#include "utils.hpp"
#include "window.hpp"

#include <SDL3/SDL_vulkan.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vkdemo {
    DeviceContext::~DeviceContext() {
        for (const auto& view: m_swapchain_image_views) {
            m_device.destroyImageView(view);
        }
        m_swapchain_image_views.clear();

        m_device.destroySwapchainKHR(m_swapchain);
        vmaDestroyAllocator(m_vma_allocator);
        m_device.destroy();
        m_instance.destroy();
    }

    void DeviceContext::Initialize(const Window &window) {
        create_instance();
        select_physical_device();
        create_logical_device();
        initialize_vma();
        create_swapchain(window);
    }

    void DeviceContext::create_instance() {
        // Initialize the dynamic dispatcher first before any vulkan call
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        vk::ApplicationInfo app_info{globals::kApplicationName.data(), vk::makeVersion(1, 0, 0), "No engine", vk::makeVersion(1, 0, 0), kVulkanAPIVersion, nullptr};

        // include required SDL instance extensions
        uint32_t    extension_count     = 0;
        auto        sdl_extension_names = SDL_Vulkan_GetInstanceExtensions(&extension_count);
        std::vector extension_names{kRequiredInstanceExtensions};
        for (int i = 0; i < extension_count; ++i) {
            extension_names.push_back(sdl_extension_names[i]);
        }

        vk::InstanceCreateInfo instance_create_info{{}, &app_info, 0, nullptr, static_cast<uint32_t>(extension_names.size()), extension_names.data()};
        m_instance = vk::createInstance(instance_create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    }

    void DeviceContext::select_physical_device() {
        const std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();

        // List available GPUs
        if constexpr (globals::kPrintDeviceCapabilities) {
            std::cout << "Available devices:" << '\n';
            for (const vk::PhysicalDevice &physical_device: physical_devices) {
                const auto &properties = physical_device.getProperties2();
                std::cout << properties.properties.deviceName << '\n';
            }
        }

        // Select the first one: my PC only has 1 GPU
        m_physical_device            = physical_devices[0];
        m_physical_device_properties = m_physical_device.getProperties2();

        std::cout << "Selected device: " << m_physical_device_properties.properties.deviceName << "\n";

        // Find required queue families
        std::vector<vk::QueueFamilyProperties2> queue_families_properties = m_physical_device.getQueueFamilyProperties2();

        // List queue families
        if constexpr (globals::kPrintDeviceCapabilities) {
            std::cout << "Available queue families:" << '\n';
            for (const auto &family_properties: queue_families_properties) {
                std::cout << "Queue Count: " << family_properties.queueFamilyProperties.queueCount << " Flags: " << vk::to_string(family_properties.queueFamilyProperties.queueFlags) << "\n";
            }
        }

        // Select queue families
        // Just grab the first compatible queue family for Graphics
        uint32_t queue_family_index = 0;
        for (const auto &family_properties: queue_families_properties) {
            if (family_properties.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                m_graphics_queue_family_index = queue_family_index;
                break;
            }

            ++queue_family_index;
        }
    }

    void DeviceContext::create_logical_device() {
        const std::vector         queue_priorities{1.0f};
        vk::DeviceQueueCreateInfo graphics_queue_create_info{{}, m_graphics_queue_family_index, queue_priorities};

        // Enable vulkan features
        vk::PhysicalDeviceVulkan13Features enabled_vk13_features{};
        enabled_vk13_features.synchronization2 = true;
        enabled_vk13_features.dynamicRendering = true;

        vk::PhysicalDeviceVulkan12Features enabled_vk12_features{};

        // Descriptor indexing
        enabled_vk12_features.descriptorIndexing                        = true;
        enabled_vk12_features.shaderSampledImageArrayNonUniformIndexing = true;
        enabled_vk12_features.runtimeDescriptorArray                    = true;
        enabled_vk12_features.descriptorBindingVariableDescriptorCount  = true;

        // Lets us access buffers via pointers instead of going through descriptors
        enabled_vk12_features.bufferDeviceAddress = true;
        enabled_vk12_features.pNext               = &enabled_vk13_features;

        vk::PhysicalDeviceFeatures2 enabled_features{};
        enabled_features.features.samplerAnisotropy = true;

        vk::DeviceCreateInfo device_create_info{};
        device_create_info.pNext = enabled_vk12_features;
        device_create_info.setQueueCreateInfos({graphics_queue_create_info});
        device_create_info.setPEnabledExtensionNames(kRequiredDeviceExtensions);

        m_device         = m_physical_device.createDevice(device_create_info);
        m_graphics_queue = m_device.getQueue2(vk::DeviceQueueInfo2{{}, m_graphics_queue_family_index});

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
    }

    void DeviceContext::create_swapchain(const Window &window) {
        check(window.CreateSurface(m_instance, m_surface), "Failed to create VkSurfaceKHR from window.");

        vk::SurfaceCapabilities2KHR surface_capabilities = m_physical_device.getSurfaceCapabilities2KHR({m_surface});
        vk::SwapchainCreateInfoKHR  swapchain_create_info{};
        swapchain_create_info.surface          = m_surface;
        swapchain_create_info.minImageCount    = surface_capabilities.surfaceCapabilities.minImageCount;
        swapchain_create_info.imageFormat      = kSwapchainFormat;
        swapchain_create_info.imageColorSpace  = kSwapchainColorSpace;
        swapchain_create_info.imageExtent      = vk::Extent2D{surface_capabilities.surfaceCapabilities.currentExtent.width, surface_capabilities.surfaceCapabilities.currentExtent.height};
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
        swapchain_create_info.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        swapchain_create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;

        if constexpr (globals::kVSyncEnabled) {
            swapchain_create_info.presentMode = vk::PresentModeKHR::eMailbox; // V-SYNC, no screen tearing
        } else {
            swapchain_create_info.presentMode = vk::PresentModeKHR::eImmediate; // Uncapped framerate, screen tearing
        }

        m_swapchain        = m_device.createSwapchainKHR(swapchain_create_info);
        m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);

        vk::ImageViewCreateInfo image_view_create_info{};
        image_view_create_info.viewType         = vk::ImageViewType::e2D;
        image_view_create_info.format           = kSwapchainFormat;
        image_view_create_info.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        for (const auto &swapchain_image: m_swapchain_images) {
            image_view_create_info.image = swapchain_image;
            m_swapchain_image_views.emplace_back(m_device.createImageView(image_view_create_info));
        }
    }

    void DeviceContext::initialize_vma() {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_create_info{};
        allocator_create_info.instance = m_instance;
        allocator_create_info.physicalDevice = m_physical_device;
        allocator_create_info.device = m_device;
        allocator_create_info.vulkanApiVersion = kVulkanAPIVersion;

        vmaCreateAllocator(&allocator_create_info, &m_vma_allocator);
    }
} // namespace vkdemo
