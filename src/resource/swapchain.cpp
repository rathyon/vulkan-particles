#include "swapchain.hpp"

#include "device-context.hpp"
#include "globals.hpp"
#include "util.hpp"

namespace vkdemo {

    static constexpr vk::Format        kSwapchainFormat     = vk::Format::eB8G8R8A8Srgb;
    static constexpr vk::ColorSpaceKHR kSwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    Swapchain::Swapchain(const DeviceContext &context, vk::SurfaceKHR surface) : m_device_context(context) {
        vk::SurfaceCapabilitiesKHR surface_capabilities = m_device_context.PhysicalDevice.getSurfaceCapabilitiesKHR({surface});
        vk::SwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.surface         = surface;
        swapchain_create_info.minImageCount   = surface_capabilities.minImageCount;
        swapchain_create_info.imageFormat     = kSwapchainFormat;
        swapchain_create_info.imageColorSpace = kSwapchainColorSpace;
        swapchain_create_info.imageExtent     = vk::Extent2D{surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height};
        assert(globals::kWindowSize.x == swapchain_create_info.imageExtent.width && globals::kWindowSize.y == swapchain_create_info.imageExtent.height);
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        swapchain_create_info.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        swapchain_create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;

        if constexpr (globals::kVSyncEnabled) {
            swapchain_create_info.presentMode = vk::PresentModeKHR::eMailbox; // V-SYNC, no screen tearing
        } else {
            swapchain_create_info.presentMode = vk::PresentModeKHR::eImmediate; // Uncapped framerate, screen tearing
        }

        Handle                                  = m_device_context.Device.createSwapchainKHR(swapchain_create_info);
        std::vector<vk::Image> swapchain_images = m_device_context.Device.getSwapchainImagesKHR(Handle);
        Images.resize(swapchain_images.size());

        // Assign images and create image views
        vk::ImageViewCreateInfo image_view_create_info{};
        image_view_create_info.viewType         = vk::ImageViewType::e2D;
        image_view_create_info.format           = kSwapchainFormat;
        image_view_create_info.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        for (size_t i = 0; i < swapchain_images.size(); ++i) {
            Images[i]         = std::make_unique<Image>(m_device_context);
            Images[i]->Handle = swapchain_images[i];

            image_view_create_info.image = Images[i]->Handle;
            Images[i]->View              = m_device_context.Device.createImageView(image_view_create_info);
            Images[i]->Aspect            = image_view_create_info.subresourceRange.aspectMask;
        }

        vk::SemaphoreCreateInfo semaphore_create_info{};
        for (size_t i = 0; i < Images.size(); ++i) {
            RenderCompleteSemaphores.emplace_back(m_device_context.Device.createSemaphore(semaphore_create_info));
            check(RenderCompleteSemaphores[i] != nullptr, "Failed to create semaphore");
        }
    }

    Swapchain::~Swapchain() {
        for (const auto &semaphore: RenderCompleteSemaphores) {
            m_device_context.Device.destroySemaphore(semaphore);
        }

        Images.clear();
        m_device_context.Device.destroySwapchainKHR(Handle);
    }
} // namespace vkdemo
