#include "renderer-context.hpp"

#include "device-context.hpp"
#include "utils.hpp"

namespace vkdemo {

    RendererContext::RendererContext(const DeviceContext &device_context) : m_device_context(device_context), m_device(m_device_context.Device) {}

    RendererContext::~RendererContext() {
        m_device.destroyImageView(m_depth_attachment.ImageView);
        vmaDestroyImage(m_device_context.VmaAllocator, m_depth_attachment.Image, m_depth_attachment.Allocation);

        for (const auto &attachment: m_color_attachments) {
            m_device.destroyImageView(attachment.ImageView);
            vmaDestroyImage(m_device_context.VmaAllocator, attachment.Image, attachment.Allocation);
        }

        m_device.destroyCommandPool(m_graphics_command_pool);

        for (const auto &semaphore: m_render_complete_semaphores) {
            m_device.destroySemaphore(semaphore);
        }
        for (const auto &semaphore: m_image_acquired_semaphores) {
            m_device.destroySemaphore(semaphore);
        }
        for (const auto &fence: m_fences) {
            m_device.destroyFence(fence);
        }
    }

    void RendererContext::Initialize() {
        create_sync_objects();
        create_command_buffers();
        create_attachments();
    }

    void RendererContext::create_command_buffers() {
        vk::CommandPoolCreateInfo graphics_command_pool_ci{};
        graphics_command_pool_ci.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        graphics_command_pool_ci.queueFamilyIndex = m_device_context.GraphicsQueueFamily;

        m_graphics_command_pool = m_device.createCommandPool(graphics_command_pool_ci);
        check(m_graphics_command_pool != nullptr, "Failed to create graphics command pool");

        vk::CommandBufferAllocateInfo graphics_command_buffer_ai{};
        graphics_command_buffer_ai.commandPool        = m_graphics_command_pool;
        graphics_command_buffer_ai.commandBufferCount = globals::kFramesInFlight;

        check(m_device.allocateCommandBuffers(&graphics_command_buffer_ai, m_graphics_command_buffers.data()));
    }

    void RendererContext::create_sync_objects() {
        vk::FenceCreateInfo fence_create_info{};
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

        vk::SemaphoreCreateInfo semaphore_create_info{};

        // todo return to this later on once the swapchain image is no longer the main render target
        for (uint8_t i = 0; i < globals::kFramesInFlight; ++i) {
            m_fences[i]                    = m_device.createFence(fence_create_info);
            m_image_acquired_semaphores[i] = m_device.createSemaphore(semaphore_create_info);

            check(m_fences[i] != nullptr, "Failed to create fence");
            check(m_image_acquired_semaphores[i] != nullptr, "Failed to create semaphore");
        }

        for (size_t i = 0; i < m_device_context.SwapchainImages.size(); ++i) {
            m_render_complete_semaphores.emplace_back(m_device.createSemaphore(semaphore_create_info));
            check(m_render_complete_semaphores[i] != nullptr, "Failed to create semaphore");
        }
    }

    void RendererContext::create_attachments() {
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

        // Create color attachments
        vk::ImageCreateInfo color_create_info{};
        color_create_info.imageType   = vk::ImageType::e2D;
        color_create_info.format      = globals::kColorAttachmentFormat;
        color_create_info.extent      = m_render_extent;
        color_create_info.mipLevels   = 1;
        color_create_info.arrayLayers = 1;
        color_create_info.samples     = vk::SampleCountFlagBits::e1;
        color_create_info.tiling      = vk::ImageTiling::eOptimal;

        // Color attachments will be rendered into, sampled and copied to the swapchain image
        color_create_info.usage         = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc;
        color_create_info.initialLayout = vk::ImageLayout::eUndefined;

        vk::ImageViewCreateInfo color_view_create_info{};
        color_view_create_info.viewType                    = vk::ImageViewType::e2D;
        color_view_create_info.format                      = color_create_info.format;
        color_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        color_view_create_info.subresourceRange.levelCount = 1;
        color_view_create_info.subresourceRange.layerCount = 1;

        for (auto &attachment: m_color_attachments) {
            check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&color_create_info), &alloc_info, reinterpret_cast<VkImage *>(&attachment.Image),
                                 &attachment.Allocation, nullptr),
                  "Failed to create color attachment");

            color_view_create_info.image = attachment.Image;
            attachment.ImageView         = m_device.createImageView(color_view_create_info);
        }

        // Create depth attachment
        vk::ImageCreateInfo depth_create_info{};
        depth_create_info.imageType     = vk::ImageType::e2D;
        depth_create_info.format        = globals::kDepthStencilAttachmentFormat;
        depth_create_info.extent        = m_render_extent;
        depth_create_info.mipLevels     = 1;
        depth_create_info.arrayLayers   = 1;
        depth_create_info.samples       = vk::SampleCountFlagBits::e1;
        depth_create_info.tiling        = vk::ImageTiling::eOptimal;
        depth_create_info.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depth_create_info.initialLayout = vk::ImageLayout::eUndefined;

        check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&depth_create_info), &alloc_info, reinterpret_cast<VkImage *>(&m_depth_attachment.Image),
                             &m_depth_attachment.Allocation, nullptr),
              "Failed to create depth attachment");

        vk::ImageViewCreateInfo depth_view_create_info{};
        depth_view_create_info.image                       = m_depth_attachment.Image;
        depth_view_create_info.viewType                    = vk::ImageViewType::e2D;
        depth_view_create_info.format                      = depth_create_info.format;
        depth_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        depth_view_create_info.subresourceRange.levelCount = 1;
        depth_view_create_info.subresourceRange.layerCount = 1;

        m_depth_attachment.ImageView = m_device.createImageView(depth_view_create_info);
    }

} // namespace vkdemo
