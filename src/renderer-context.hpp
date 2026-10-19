#ifndef RENDERER_CONTEXT_HPP
#define RENDERER_CONTEXT_HPP

#include "resource/render-attachment.hpp"
#include "util.hpp"

namespace vkdemo {
    class Window;
    class DeviceContext;
    class Swapchain;

    struct FrameContext {
        uint32_t          Index = 0;
        vk::CommandBuffer GraphicsCB{};
        vk::CommandBuffer ComputeCB{};
    };

    class RendererContext {
    public:
        explicit RendererContext(const DeviceContext &device_context);
        virtual ~RendererContext();

        void                        Initialize(const Window &window);
        [[nodiscard]] FrameContext *BeginFrame();
        void                        BeginGraphicsCommands();
        void                        BeginComputeCommands();
        void                        EndFrame();
        void                        EndGraphicsCommands();
        void                        EndComputeCommands();

        Buffered<std::unique_ptr<RenderAttachment>> MainColorAttachments{};
        std::unique_ptr<RenderAttachment>           DepthStencilAttachment{};

    private:
        void create_sync_objects();
        void create_command_buffers();
        void create_attachments();
        void create_swapchain(const Window &window);
        void copy_main_color_attachment_to_swapchain(vk::CommandBuffer cb);

        const DeviceContext &m_device_context;
        const vk::Device    &m_device;

        vk::SurfaceKHR             m_surface;
        std::unique_ptr<Swapchain> m_swapchain{};
        uint32_t                   m_swapchain_image_index{};

        Buffered<vk::Fence>     m_graphics_fences{};
        Buffered<vk::Semaphore> m_image_acquired_semaphores{};

        Buffered<vk::Fence>     m_compute_fences{};
        Buffered<vk::Semaphore> m_compute_ready_semaphores{};
        Buffered<vk::Semaphore> m_compute_complete_semaphores{};

        vk::CommandPool             m_graphics_command_pool{};
        Buffered<vk::CommandBuffer> m_graphics_command_buffers{};

        vk::CommandPool             m_compute_command_pool{};
        Buffered<vk::CommandBuffer> m_compute_command_buffers{};

        uint32_t               m_frame_index = 0;
        Buffered<FrameContext> m_frame_contexts{};
    };

} // namespace vkdemo


#endif //
