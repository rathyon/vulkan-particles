#include "renderer-context.hpp"

#include <SDL3/SDL_vulkan.h>
#include "device-context.hpp"
#include "window.hpp"

namespace vkdemo {

    RendererContext::RendererContext(const DeviceContext &device_context) : m_device_context(device_context), m_device(m_device_context.Device) {}

    RendererContext::~RendererContext() {
        m_swapchain_images.clear();
        SDL_Vulkan_DestroySurface(m_device_context.Instance, m_surface, nullptr);

        m_device_context.Device.destroySwapchainKHR(m_swapchain);

        // Let unique_ptr dispose these (out of scope destruction)
        DepthStencilAttachment = nullptr;
        for (auto &attachment: MainColorAttachments) {
            attachment = nullptr;
        }

        m_device.destroyCommandPool(m_graphics_command_pool);
        m_device.destroyCommandPool(m_compute_command_pool);

        // render complete semaphores can have a different count from the other semaphores
        for (const auto &semaphore: m_render_complete_semaphores) {
            m_device.destroySemaphore(semaphore);
        }

        for (int i = 0; i < globals::kFramesInFlight; ++i) {
            m_device.destroySemaphore(m_image_acquired_semaphores[i]);
            m_device.destroySemaphore(m_compute_ready_semaphores[i]);
            m_device.destroySemaphore(m_compute_complete_semaphores[i]);
            m_device.destroyFence(m_graphics_fences[i]);
        }
    }

    void RendererContext::Initialize(const Window &window) {
        create_swapchain(window);
        create_sync_objects();
        create_command_buffers();
        create_attachments();

        for (size_t i = 0; i < m_frame_contexts.size(); ++i) {
            m_frame_contexts[i].Index      = i;
            m_frame_contexts[i].GraphicsCB = m_graphics_command_buffers[i];
            m_frame_contexts[i].ComputeCB  = m_compute_command_buffers[i];
        }
    }

    FrameContext *RendererContext::BeginFrame() {
        static constexpr auto kTimeout = std::numeric_limits<uint64_t>::max();

        // Wait for frame completion
        check(m_device.waitForFences(m_graphics_fences[m_frame_index], true, kTimeout) == vk::Result::eSuccess, "Failed to wait for fences");
        m_device.resetFences(m_graphics_fences[m_frame_index]);

        vk::Result acquire_result = m_device.acquireNextImageKHR(m_swapchain, kTimeout, m_image_acquired_semaphores[m_frame_index], nullptr, &m_swapchain_image_index);

        if (acquire_result != vk::Result::eSuccess) {
            throw std::runtime_error(std::format("Failed to acquire next swapchain image! Code: {}", vk::to_string(acquire_result)));
        }

        auto &cb = m_frame_contexts[m_frame_index].GraphicsCB;
        cb       = m_graphics_command_buffers[m_frame_index];

        cb.reset();
        cb.begin(vk::CommandBufferBeginInfo{});

        return &m_frame_contexts[m_frame_index];
    }

    void RendererContext::EndFrame() {
        // Copy contents from main color attachment to swapchain image.
        // Because the swapchain image may be of a different format or resolution, blit is used to resolve these differences.
        const auto &frame_context    = m_frame_contexts[m_frame_index];
        const auto &cb               = frame_context.GraphicsCB;
        auto       &color_attachment = *MainColorAttachments[m_frame_index];
        auto       &swapchain_image  = *m_swapchain_images[m_swapchain_image_index];

        color_attachment.LayoutTransition(cb, vk::ImageLayout::eTransferSrcOptimal);
        swapchain_image.LayoutTransition(cb, vk::ImageLayout::eTransferDstOptimal);

        // Define the regions to blit (we will blit the whole swapchain image)
        const vk::Offset3D src_start{0, 0, 0};
        const vk::Offset3D src_end{globals::kRenderExtent2D.width, globals::kRenderExtent2D.height, 1};

        const vk::Offset3D dst_start{0, 0, 0};
        const vk::Offset3D dst_end{globals::kWindowSize.x, globals::kWindowSize.y, 1};

        vk::BlitImageInfo2 blit_info{};
        blit_info.srcImage       = color_attachment.Handle;
        blit_info.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        blit_info.dstImage       = swapchain_image.Handle;
        blit_info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        blit_info.filter         = vk::Filter::eNearest;

        constexpr vk::ImageSubresourceLayers subresource{vk::ImageAspectFlagBits::eColor, 0, 0, 1};
        vk::ImageBlit2                       blit{subresource, {src_start, src_end}, subresource, {dst_start, dst_end}};
        blit_info.setRegions(blit);

        cb.blitImage2(blit_info);

        color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);
        swapchain_image.LayoutTransition(cb, vk::ImageLayout::ePresentSrcKHR);

        // Finish command buffer recording
        cb.end();

        // Submit command buffer to graphics queue
        // Wait for final write into color attachment
        std::vector<vk::PipelineStageFlags> wait_stage = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo                      submit_info{{m_image_acquired_semaphores[m_frame_index]}, wait_stage, {cb}, {m_render_complete_semaphores[m_swapchain_image_index]}};
        m_device_context.GraphicsQueue.submit({submit_info}, m_graphics_fences[m_frame_index]);

        // Present the swapchain via the present queue
        // By default VulkanHPP throws exception when result isn't Success. But throwing/catching exceptions isn't desired here so the C API is used.
        vk::PresentInfoKHR present_info{{m_render_complete_semaphores[m_swapchain_image_index]}, {m_swapchain}, {m_swapchain_image_index}};
        auto               present_result = static_cast<vk::Result>(vkQueuePresentKHR(m_device_context.PresentQueue, present_info));
        check(present_result == vk::Result::eSuccess, "Failed to present swapchain!");

        m_frame_index = (m_frame_index + 1) % globals::kFramesInFlight;
    }

    void RendererContext::create_command_buffers() {
        // Create graphics buffers
        vk::CommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_ci.queueFamilyIndex = m_device_context.GraphicsQueueFamily;

        m_graphics_command_pool = m_device.createCommandPool(command_pool_ci);
        check(m_graphics_command_pool != nullptr, "Failed to create graphics command pool");

        vk::CommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandPool        = m_graphics_command_pool;
        command_buffer_ai.commandBufferCount = globals::kFramesInFlight;

        check(m_device.allocateCommandBuffers(&command_buffer_ai, m_graphics_command_buffers.data()), "Failed to allocate graphics command buffers");

        // Create compute buffers
        command_pool_ci.queueFamilyIndex = m_device_context.ComputeQueueFamily;
        m_compute_command_pool           = m_device.createCommandPool(command_pool_ci);
        check(m_compute_command_pool != nullptr, "Failed to create compute command pool");

        command_buffer_ai.commandPool = m_compute_command_pool;
        check(m_device.allocateCommandBuffers(&command_buffer_ai, m_compute_command_buffers.data()), "Failed to allocate compute command buffers");
    }

    void RendererContext::create_sync_objects() {
        vk::FenceCreateInfo fence_create_info{};
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

        vk::SemaphoreCreateInfo semaphore_create_info{};
        for (uint8_t i = 0; i < globals::kFramesInFlight; ++i) {
            m_graphics_fences[i]           = m_device.createFence(fence_create_info);
            m_compute_fences[i]            = m_device.createFence(fence_create_info);
            m_image_acquired_semaphores[i] = m_device.createSemaphore(semaphore_create_info);

            check(m_graphics_fences[i] != nullptr, "Failed to create fence");
            check(m_image_acquired_semaphores[i] != nullptr, "Failed to create semaphore");
        }

        for (size_t i = 0; i < m_swapchain_images.size(); ++i) {
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
        color_create_info.extent      = globals::kRenderExtent3D;
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

        for (auto &attachment_ptr: MainColorAttachments) {
            attachment_ptr   = std::make_unique<Image>(m_device_context);
            auto &attachment = *attachment_ptr;

            check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&color_create_info), &alloc_info, reinterpret_cast<VkImage *>(&attachment.Handle),
                                 &attachment.Allocation, nullptr),
                  "Failed to create color attachment");

            color_view_create_info.image = attachment.Handle;
            attachment.View              = m_device.createImageView(color_view_create_info);
            attachment.Aspect            = color_view_create_info.subresourceRange.aspectMask;
        }

        // Create depth attachment
        DepthStencilAttachment = std::make_unique<Image>(m_device_context);
        vk::ImageCreateInfo depth_create_info{};
        depth_create_info.imageType     = vk::ImageType::e2D;
        depth_create_info.format        = globals::kDepthStencilAttachmentFormat;
        depth_create_info.extent        = globals::kRenderExtent3D;
        depth_create_info.mipLevels     = 1;
        depth_create_info.arrayLayers   = 1;
        depth_create_info.samples       = vk::SampleCountFlagBits::e1;
        depth_create_info.tiling        = vk::ImageTiling::eOptimal;
        depth_create_info.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depth_create_info.initialLayout = vk::ImageLayout::eUndefined;

        check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&depth_create_info), &alloc_info, reinterpret_cast<VkImage *>(&DepthStencilAttachment->Handle),
                             &DepthStencilAttachment->Allocation, nullptr),
              "Failed to create depth attachment");

        vk::ImageViewCreateInfo depth_view_create_info{};
        depth_view_create_info.image                       = DepthStencilAttachment->Handle;
        depth_view_create_info.viewType                    = vk::ImageViewType::e2D;
        depth_view_create_info.format                      = depth_create_info.format;
        depth_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        depth_view_create_info.subresourceRange.levelCount = 1;
        depth_view_create_info.subresourceRange.layerCount = 1;

        DepthStencilAttachment->View   = m_device.createImageView(depth_view_create_info);
        DepthStencilAttachment->Aspect = depth_view_create_info.subresourceRange.aspectMask;
    }

    void RendererContext::create_swapchain(const Window &window) {
        check(window.CreateSurface(m_device_context.Instance, m_surface), "Failed to create VkSurfaceKHR from window.");

        vk::SurfaceCapabilitiesKHR surface_capabilities = m_device_context.PhysicalDevice.getSurfaceCapabilitiesKHR({m_surface});
        vk::SwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.surface         = m_surface;
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

        m_swapchain                             = m_device.createSwapchainKHR(swapchain_create_info);
        std::vector<vk::Image> swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);
        m_swapchain_images.resize(swapchain_images.size());

        vk::ImageViewCreateInfo image_view_create_info{};
        image_view_create_info.viewType         = vk::ImageViewType::e2D;
        image_view_create_info.format           = kSwapchainFormat;
        image_view_create_info.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        for (size_t i = 0; i < swapchain_images.size(); ++i) {
            m_swapchain_images[i]         = std::make_unique<Image>(m_device_context);
            m_swapchain_images[i]->Handle = swapchain_images[i];

            image_view_create_info.image = m_swapchain_images[i]->Handle = swapchain_images[i];
            m_swapchain_images[i]->View                                  = m_device.createImageView(image_view_create_info);
            m_swapchain_images[i]->Aspect                                = image_view_create_info.subresourceRange.aspectMask;
        }
    }
} // namespace vkdemo
