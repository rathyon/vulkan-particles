#include "device-context.hpp"
#include <format>
#include <iostream>
#include <unordered_set>

#include "globals.hpp"
#include "util.hpp"
#include "window.hpp"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_to_string.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

// Messenger callback definition
VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_utils_messenger_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity, vk::DebugUtilsMessageTypeFlagsEXT message_type,
                                                                const vk::DebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data) {
    std::string message = std::format("Debug callback {}\n{}: {}", vk::to_string(message_type), callback_data->pMessageIdName, callback_data->pMessage);
    switch (message_severity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            std::cout << message << std::endl;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            std::cerr << message << std::endl;
            break;
        default:
            std::cout << std::format("Unknown message severity {}!\n{}", vk::to_string(message_severity), message);
            break;
    }
    return VK_FALSE;
}

namespace vkdemo {
    DeviceContext::~DeviceContext() {
        vmaDestroyAllocator(VmaAllocator);
        Instance.destroyDebugUtilsMessengerEXT(m_debug_utils_messenger, nullptr);
        Device.destroy();
        Instance.destroy();
    }

    void DeviceContext::Initialize() {
        create_instance();
        select_physical_device();
        create_logical_device();
        initialize_vma();
    }

    void DeviceContext::create_instance() {
        // Initialize the dynamic dispatcher first before any vulkan call
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        if constexpr (globals::kEnableVulkanLayers) {
            check(check_layer_support(), "Required Vulkan layers not present!");
        }

        vk::ApplicationInfo app_info{globals::kApplicationName.data(), vk::makeVersion(1, 0, 0), "No engine", vk::makeVersion(1, 0, 0), kVulkanAPIVersion, nullptr};

        // include required SDL instance extensions
        uint32_t    extension_count     = 0;
        auto        sdl_extension_names = SDL_Vulkan_GetInstanceExtensions(&extension_count);
        std::vector extension_names{kRequiredInstanceExtensions};
        for (int i = 0; i < extension_count; ++i) {
            extension_names.push_back(sdl_extension_names[i]);
        }

        vk::DebugUtilsMessengerCreateInfoEXT messenger_create_info{};
        messenger_create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
        messenger_create_info.messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
        messenger_create_info.pfnUserCallback = debug_utils_messenger_callback;


        vk::InstanceCreateInfo instance_create_info{{}, &app_info, 0, nullptr, static_cast<uint32_t>(extension_names.size()), extension_names.data()};
        if constexpr (globals::kEnableVulkanLayers) {
            instance_create_info.enabledLayerCount   = static_cast<uint32_t>(kRequiredLayers.size());
            instance_create_info.ppEnabledLayerNames = kRequiredLayers.data();
        }

        instance_create_info.pNext = &messenger_create_info;

        Instance = vk::createInstance(instance_create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(Instance);

        m_debug_utils_messenger = Instance.createDebugUtilsMessengerEXT(messenger_create_info);
    }

    bool DeviceContext::check_layer_support() {
        m_layer_properties = vk::enumerateInstanceLayerProperties();
        std::unordered_set<std::string> required_layers{kRequiredLayers.cbegin(), kRequiredLayers.cend()};

        for (const auto &available_layer: m_layer_properties) {
            required_layers.erase(available_layer.layerName);
        }

        return required_layers.empty();
    }

    void DeviceContext::select_physical_device() {
        const std::vector<vk::PhysicalDevice> physical_devices = Instance.enumeratePhysicalDevices();

        // List available GPUs
        if constexpr (globals::kPrintDeviceCapabilities) {
            std::cout << "Available devices:" << '\n';
            for (const vk::PhysicalDevice &physical_device: physical_devices) {
                const auto &properties = physical_device.getProperties2();
                std::cout << properties.properties.deviceName << '\n';
            }
        }

        // Select the first one: my PC only has 1 GPU
        check(check_device_extension_support(physical_devices[0]), "Required device extensions not present!");
        PhysicalDevice = physical_devices[0];
        read_physical_device_properties();

        std::cout << "Selected device: " << m_physical_device_properties.properties.deviceName << "\n";

        // Find required queue families
        std::vector<vk::QueueFamilyProperties2> queue_families_properties = PhysicalDevice.getQueueFamilyProperties2();

        // List queue families
        if constexpr (globals::kPrintDeviceCapabilities) {
            std::cout << "Available queue families:" << '\n';
            for (const auto &family_properties: queue_families_properties) {
                std::cout << "Queue Count: " << family_properties.queueFamilyProperties.queueCount << " Flags: " << vk::to_string(family_properties.queueFamilyProperties.queueFlags) << "\n";
            }
        }
    }

    bool DeviceContext::check_device_extension_support(const vk::PhysicalDevice &physical_device) {
        m_device_extensions = physical_device.enumerateDeviceExtensionProperties();
        std::unordered_set<std::string> required_extensions{kRequiredDeviceExtensions.cbegin(), kRequiredDeviceExtensions.cend()};

        // Erase required extensions from the set as they appear. If the set is empty at the end, all required extensions have been found.
        for (const auto &available_extension: m_device_extensions) {
            required_extensions.erase(available_extension.extensionName);
        }

        return required_extensions.empty();
    }

    void DeviceContext::select_queue_families() {
        const std::vector<vk::QueueFamilyProperties> properties_list = PhysicalDevice.getQueueFamilyProperties();

        // Initialize family indices with max uint value to represent null/none
        static constexpr uint32_t kFamilyNone = std::numeric_limits<uint32_t>::max();
        GraphicsQueueFamily = ComputeQueueFamily = PresentQueueFamily = TransferQueueFamily = kFamilyNone;

        // Lambda function to help select queue families
        auto find_family = [&properties_list](const vk::QueueFlags flags, const vk::QueueFlags exclude_flags = {}) {
            for (auto property_it = properties_list.cbegin(); property_it != properties_list.cend(); ++property_it) {
                const vk::QueueFamilyProperties properties = *property_it;
                if (properties.queueCount <= 0)
                    continue;

                if (properties.queueFlags & (flags) && !(properties.queueFlags & exclude_flags)) {
                    return static_cast<uint32_t>(std::distance(properties_list.cbegin(), property_it));
                }
            }
            return kFamilyNone;
        };

        // Select graphics queue family (select the first one that appears) it should also include Compute
        GraphicsQueueFamily = find_family(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
        check(GraphicsQueueFamily != kFamilyNone, "Failed to find Graphics queue family!");

        // Select compute queue family. A family that supports Compute but not Graphics is preferred (dedicated Compute)
        ComputeQueueFamily = find_family(vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics);
        if (ComputeQueueFamily == kFamilyNone) {
            std::cout << "Dedicated compute queue family not found. Using graphics family." << std::endl;
            ComputeQueueFamily = GraphicsQueueFamily;
        }
        check(ComputeQueueFamily != kFamilyNone, "Failed to find Compute queue family!");

        // Select optimal transfer queue if it exists.
        // Find a Transfer queue. We try from best case to worst and by the one with the least amount of queues.
        // Best case: If it supports Transfer but not Graphics and Compute
        TransferQueueFamily = find_family(vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
        // Second-best case: If it supports Transfer but not Graphics
        TransferQueueFamily = TransferQueueFamily != kFamilyNone ? TransferQueueFamily : find_family(vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics);
        // Fallback: use the graphics family. It is guaranteed to support transfer by the spec.
        TransferQueueFamily = TransferQueueFamily != kFamilyNone ? TransferQueueFamily : GraphicsQueueFamily;
        check(TransferQueueFamily != kFamilyNone, "Failed to find Present queue family.");

        // Select present queue family
        for (auto property_it = properties_list.cbegin(); property_it != properties_list.cend(); ++property_it) {
            const vk::QueueFamilyProperties properties = *property_it;
            if (properties.queueCount <= 0)
                continue;

            const uint32_t   family_index    = static_cast<uint32_t>(std::distance(properties_list.cbegin(), property_it));
            const vk::Bool32 present_support = SDL_Vulkan_GetPresentationSupport(Instance, PhysicalDevice, family_index);
            if (present_support) {
                PresentQueueFamily = family_index;
                break;
            }
        }
        check(PresentQueueFamily != kFamilyNone, "Failed to find Present queue family.");

        std::cout << "Selected queue families:\n"
                  << "Graphics: " << GraphicsQueueFamily << "\nCompute: " << ComputeQueueFamily << "\nPresent: " << PresentQueueFamily
                  << "\nTransfer: " << TransferQueueFamily << std::endl;
    }

    void DeviceContext::create_logical_device() {
        select_queue_families();

        // Create a set of unique family indices (duplicates are discarded)
        const std::unordered_set unique_queue_families{GraphicsQueueFamily, PresentQueueFamily, ComputeQueueFamily, TransferQueueFamily};
        const std::vector         all_queue_priority{1.0f};

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        for (const auto& cur_family : unique_queue_families) {
            // Create with no flags, with 1 queue and priority = 1.0f
            vk::DeviceQueueCreateInfo create_info{{}, cur_family, all_queue_priority};
            queue_create_infos.emplace_back(create_info);
        }

        // Enable desired vulkan features
        vk::PhysicalDeviceHostImageCopyFeatures host_image_copy_features{};
        host_image_copy_features.hostImageCopy = true;

        vk::PhysicalDeviceVulkan13Features enabled_vk13_features{};
        enabled_vk13_features.synchronization2 = true;
        enabled_vk13_features.dynamicRendering = true;
        enabled_vk13_features.pNext            = &host_image_copy_features;

        vk::PhysicalDeviceVulkan12Features enabled_vk12_features{};
        enabled_vk12_features.descriptorIndexing                        = true;
        enabled_vk12_features.shaderSampledImageArrayNonUniformIndexing = true;
        enabled_vk12_features.runtimeDescriptorArray                    = true;
        enabled_vk12_features.descriptorBindingVariableDescriptorCount  = true;
        enabled_vk12_features.bufferDeviceAddress                       = true;
        enabled_vk12_features.scalarBlockLayout                         = true;
        enabled_vk12_features.pNext                                     = &enabled_vk13_features;

        vk::PhysicalDeviceFeatures2 enabled_features{};
        enabled_features.features.samplerAnisotropy = true;
        enabled_features.pNext                      = &enabled_vk12_features;

        vk::DeviceCreateInfo device_create_info{};
        device_create_info.setQueueCreateInfos(queue_create_infos);
        device_create_info.setPEnabledExtensionNames(kRequiredDeviceExtensions);
        device_create_info.pNext = enabled_features;

        Device         = PhysicalDevice.createDevice(device_create_info);
        GraphicsQueue = Device.getQueue2(vk::DeviceQueueInfo2{{}, GraphicsQueueFamily});
        PresentQueue = Device.getQueue2(vk::DeviceQueueInfo2{{}, PresentQueueFamily});
        ComputeQueue = Device.getQueue2(vk::DeviceQueueInfo2{{}, ComputeQueueFamily});
        TransferQueue = Device.getQueue2(vk::DeviceQueueInfo2{{}, TransferQueueFamily});

        VULKAN_HPP_DEFAULT_DISPATCHER.init(Device);
    }

    void DeviceContext::read_physical_device_properties() {
        m_physical_device_properties = PhysicalDevice.getProperties2();

        m_physical_device_memory_properties.pNext = &m_physical_device_memory_budget_properties;
        PhysicalDevice.getMemoryProperties2(&m_physical_device_memory_properties);

        if constexpr (globals::kPrintDeviceMemoryProperties) {
            const auto &properties = m_physical_device_memory_properties.memoryProperties;
            std::cout << "Physical Device Memory Properties:" << std::endl;
            std::cout << "\t Heap Count:" << properties.memoryHeapCount << std::endl;
            std::cout << "\t Type Count:" << properties.memoryTypeCount << std::endl;

            for (int heap_idx = 0; heap_idx < properties.memoryHeapCount; ++heap_idx) {
                const auto &heap = properties.memoryHeaps[heap_idx];
                std::cout << "Heap [" << heap_idx << "] Size: " << (static_cast<float>(heap.size) / 1024.f / 1024.f) << " MiB | Flags: " << vk::to_string(heap.flags) << std::endl;

                for (int i = 0; i < properties.memoryTypeCount; ++i) {
                    const auto &type = properties.memoryTypes[i];
                    if (type.heapIndex == heap_idx)
                        std::cout << "\tMemory Type Properties: " << vk::to_string(type.propertyFlags) << std::endl;
                }
            }
        }
    }

    void DeviceContext::initialize_vma() {
        const auto &dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;

        // Let VMA fetch function pointers dynamically
        VmaVulkanFunctions vulkan_functions{};
        vulkan_functions.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
        vulkan_functions.vkGetDeviceProcAddr   = dispatcher.vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_create_info{};
        allocator_create_info.instance         = Instance;
        allocator_create_info.physicalDevice   = PhysicalDevice;
        allocator_create_info.device           = Device;
        allocator_create_info.vulkanApiVersion = kVulkanAPIVersion;
        allocator_create_info.pVulkanFunctions = &vulkan_functions;

        // Buffer Device Address and Memory Budget extensions are used, they need to be declared to VMA
        allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

        vmaCreateAllocator(&allocator_create_info, &VmaAllocator);
    }
} // namespace vkdemo
