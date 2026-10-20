#include "renderer-context.hpp"

#include <SDL3/SDL_vulkan.h>
#include "device-context.hpp"
#include "resource/swapchain.hpp"
#include "window.hpp"

namespace vkdemo {

    static constexpr auto kFenceTimeout = std::numeric_limits<uint64_t>::max();

    RendererContext::RendererContext(const DeviceContext &device_context) : m_device_context(device_context), m_device(m_device_context.Device) {}

    RendererContext::~RendererContext() {
        m_swapchain = nullptr;
        SDL_Vulkan_DestroySurface(m_device_context.Instance, m_surface, nullptr);

        // Let unique_ptr dispose these (out of scope destruction)
        DepthStencilAttachment = nullptr;
        for (auto &attachment: MainColorAttachments) {
            attachment = nullptr;
        }

        for (auto m_command_pool: m_command_pools) {
            m_device.destroyCommandPool(m_command_pool);
        }

        for (int i = 0; i < globals::kFramesInFlight; ++i) {
            m_device.destroySemaphore(m_image_acquired_semaphores[i]);
            m_device.destroySemaphore(m_compute_ready_semaphores[i]);
            m_device.destroySemaphore(m_compute_complete_semaphores[i]);
            m_device.destroyFence(m_graphics_fences[i]);
            m_device.destroyFence(m_compute_fences[i]);
        }
    }

    void RendererContext::Initialize(const Window &window) {
        create_swapchain(window);
        create_sync_objects();
        create_command_pools_and_buffers();
        create_attachments();

        for (size_t i = 0; i < m_frame_contexts.size(); ++i) {
            m_frame_contexts[i].Index      = i;
            m_frame_contexts[i].GraphicsCB = m_graphics_command_buffers[i];
            m_frame_contexts[i].ComputeCB  = m_compute_command_buffers[i];
        }
    }

    FrameContext *RendererContext::BeginFrame() { return &m_frame_contexts[m_frame_index]; }

    void RendererContext::BeginGraphicsCommands() {
        // Wait for graphics frame completion
        check(m_device.waitForFences(m_graphics_fences[m_frame_index], true, kFenceTimeout) == vk::Result::eSuccess, "Failed to wait for graphics fences");
        m_device.resetFences(m_graphics_fences[m_frame_index]);

        vk::Result acquire_result = m_device.acquireNextImageKHR(m_swapchain->Handle, kFenceTimeout, m_image_acquired_semaphores[m_frame_index], nullptr, &m_swapchain_image_index);

        if (acquire_result != vk::Result::eSuccess) {
            throw std::runtime_error(std::format("Failed to acquire next swapchain image! Code: {}", vk::to_string(acquire_result)));
        }

        auto &cb = m_frame_contexts[m_frame_index].GraphicsCB;
        cb.reset();
        cb.begin(vk::CommandBufferBeginInfo{});
    }

    void RendererContext::BeginComputeCommands() {
        // Wait for compute frame completion
        check(m_device.waitForFences(m_compute_fences[m_frame_index], true, kFenceTimeout) == vk::Result::eSuccess, "Failed to wait for compute fences");
        m_device.resetFences(m_compute_fences[m_frame_index]);

        auto &cb = m_frame_contexts[m_frame_index].ComputeCB;
        cb.reset();
        cb.begin(vk::CommandBufferBeginInfo{});
    }

    vk::CommandBuffer RendererContext::CreateCommandBuffer(const bool begin_recording, const QueueType queue) {
        assert(queue != QueueType::Present  && "Cannot use Present queue for commands!");
        vk::CommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandPool        = m_command_pools[queue];
        command_buffer_ai.commandBufferCount = 1;

        vk::CommandBuffer command_buffer{};

        check(m_device.allocateCommandBuffers(&command_buffer_ai, &command_buffer), "Failed to allocate command buffer");
        if (begin_recording)
            command_buffer.begin(vk::CommandBufferBeginInfo{});

        return command_buffer;
    }

    void RendererContext::EndFrame() { m_frame_index = (m_frame_index + 1) % globals::kFramesInFlight; }

    void RendererContext::EndGraphicsCommands() {
        // Copy contents from main color attachment to swapchain image.
        // Because the swapchain image may be of a different format or resolution, blit is used to resolve these differences.
        const auto &frame_context = m_frame_contexts[m_frame_index];
        const auto &cb            = frame_context.GraphicsCB;
        copy_main_color_attachment_to_swapchain(cb);

        // Finish command buffer recording
        cb.end();

        // Submit command buffer to graphics queue
        // Wait for final write into color attachment and for compute to finish
        std::vector<vk::PipelineStageFlags> wait_stage_mask = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eVertexInput};
        std::vector                         wait_semaphores{m_image_acquired_semaphores[m_frame_index], m_compute_complete_semaphores[m_frame_index]};
        std::vector                         signal_semaphores{m_swapchain->RenderCompleteSemaphores[m_swapchain_image_index], m_compute_ready_semaphores[m_frame_index]};

        vk::SubmitInfo submit_info{};
        submit_info.setWaitSemaphores(wait_semaphores);
        submit_info.setWaitDstStageMask(wait_stage_mask);
        submit_info.setCommandBuffers(cb);
        submit_info.setSignalSemaphores(signal_semaphores);

        m_device_context.Queues[Graphics].submit({submit_info}, m_graphics_fences[m_frame_index]);

        // Present the swapchain via the present queue
        // By default VulkanHPP throws exception when result isn't Success. But throwing/catching exceptions isn't desired here so the C API is used.
        vk::PresentInfoKHR present_info{{m_swapchain->RenderCompleteSemaphores[m_swapchain_image_index]}, {m_swapchain->Handle}, {m_swapchain_image_index}};
        auto               present_result = static_cast<vk::Result>(vkQueuePresentKHR(m_device_context.Queues[Present], present_info));
        check(present_result == vk::Result::eSuccess, "Failed to present swapchain!");
    }

    void RendererContext::EndComputeCommands() {
        const auto &frame_context = m_frame_contexts[m_frame_index];
        const auto &cb            = frame_context.ComputeCB;

        // Finish command buffer recording
        cb.end();

        const uint32_t prev_frame_index = std::abs(static_cast<int>(m_frame_index) - 1) % globals::kFramesInFlight;

        std::vector<vk::PipelineStageFlags> wait_stage_mask = {vk::PipelineStageFlagBits::eComputeShader};
        vk::SubmitInfo                      submit_info{{m_compute_ready_semaphores[prev_frame_index]}, wait_stage_mask, {cb}, {m_compute_complete_semaphores[m_frame_index]}};
        m_device_context.Queues[Compute].submit({submit_info}, m_compute_fences[m_frame_index]);
    }

    void RendererContext::FlushCommandBuffer(const vk::CommandBuffer cb, const QueueType queue) const {
        assert(queue != QueueType::Present  && "Cannot use Present queue for commands!");

        if (cb == nullptr)
            return;

        cb.end();

        // Create fence to wait on host for commands to finish
        vk::Fence fence = m_device_context.Device.createFence({});

        vk::SubmitInfo submit_info{};
        submit_info.setCommandBuffers(cb);
        m_device_context.Queues[queue].submit(submit_info, fence);

        check(m_device_context.Device.waitForFences({fence}, true, kFenceTimeout));
        m_device_context.Device.destroyFence(fence);
        m_device_context.Device.freeCommandBuffers(m_command_pools[queue], cb);
    }

    void RendererContext::create_command_pools_and_buffers() {
        // Create graphics pool & buffers
        vk::CommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_ci.queueFamilyIndex = m_device_context.QueueFamilies[Graphics];

        m_command_pools[Graphics] = m_device.createCommandPool(command_pool_ci);
        check(m_command_pools[Graphics] != nullptr, "Failed to create graphics command pool");

        vk::CommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandPool        = m_command_pools[Graphics];
        command_buffer_ai.commandBufferCount = globals::kFramesInFlight;

        check(m_device.allocateCommandBuffers(&command_buffer_ai, m_graphics_command_buffers.data()), "Failed to allocate graphics command buffers");

        // Create compute pool & buffers
        command_pool_ci.queueFamilyIndex = m_device_context.QueueFamilies[Compute];
        m_command_pools[Compute]         = m_device.createCommandPool(command_pool_ci);
        check(m_command_pools[Compute] != nullptr, "Failed to create compute command pool");

        command_buffer_ai.commandPool = m_command_pools[Compute];
        check(m_device.allocateCommandBuffers(&command_buffer_ai, m_compute_command_buffers.data()), "Failed to allocate compute command buffers");

        // Create transfer pool
        command_pool_ci.queueFamilyIndex = m_device_context.QueueFamilies[Transfer];
        m_command_pools[Transfer]        = m_device.createCommandPool(command_pool_ci);
        check(m_command_pools[Transfer] != nullptr, "Failed to create compute command pool");
    }

    void RendererContext::create_sync_objects() {
        vk::FenceCreateInfo fence_create_info{};
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

        vk::SemaphoreCreateInfo semaphore_create_info{};
        for (uint8_t i = 0; i < globals::kFramesInFlight; ++i) {
            m_graphics_fences[i]             = m_device.createFence(fence_create_info);
            m_compute_fences[i]              = m_device.createFence(fence_create_info);
            m_image_acquired_semaphores[i]   = m_device.createSemaphore(semaphore_create_info);
            m_compute_ready_semaphores[i]    = m_device.createSemaphore(semaphore_create_info);
            m_compute_complete_semaphores[i] = m_device.createSemaphore(semaphore_create_info);

            check(m_graphics_fences[i] != nullptr, "Failed to create fence");
            check(m_compute_fences[i] != nullptr, "Failed to create fence");
            check(m_image_acquired_semaphores[i] != nullptr, "Failed to create semaphore");
            check(m_compute_ready_semaphores[i] != nullptr, "Failed to create semaphore");
            check(m_compute_complete_semaphores[i] != nullptr, "Failed to create semaphore");
        }

        // The first used compute ready semaphore must start signaled. First call into EndComputeCommands requires this to not block.
        vk::SubmitInfo submit_info{{}, {}, {}, {m_compute_ready_semaphores[1]}};
        m_device_context.Queues[Compute].submit({submit_info}, nullptr);
    }

    void RendererContext::create_attachments() {
        // Create color attachments
        for (auto &attachment_ptr: MainColorAttachments) {
            attachment_ptr = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color);
        }

        // Create depth attachment
        DepthStencilAttachment = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::DepthStencil);
    }

    void RendererContext::create_swapchain(const Window &window) {
        check(window.CreateSurface(m_device_context.Instance, m_surface), "Failed to create VkSurfaceKHR from window.");
        m_swapchain = std::make_unique<Swapchain>(m_device_context, m_surface);
    }

    void RendererContext::copy_main_color_attachment_to_swapchain(vk::CommandBuffer cb) {
        auto &color_attachment = *MainColorAttachments[m_frame_index];
        auto &swapchain_image  = *m_swapchain->Images[m_swapchain_image_index];

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
    }
} // namespace vkdemo
