#ifndef RENDERER_CONTEXT_HPP
#define RENDERER_CONTEXT_HPP

#include "resource/render-attachment.hpp"
#include "util.hpp"

namespace vkdemo {
    class Window;
    class DeviceContext;
    class Swapchain;

    /// @brief Holds data related to a frame, such as its index and command buffers.
    struct FrameContext {
        uint32_t          Index = 0;
        vk::CommandBuffer GraphicsCB{};
        vk::CommandBuffer ComputeCB{};
    };

    /// @brief Class that holds the context backbone for the Renderer. This includes synchronization primitives, command buffers, main attachments and the swap-chain.
    /// A sort of bridge between the Renderer and the DeviceContext.
    class RendererContext {
    public:
        explicit RendererContext(const DeviceContext &device_context);
        virtual ~RendererContext();

        void Initialize(const Window &window);

        [[nodiscard]] FrameContext     *BeginFrame();
        void                            BeginGraphicsCommands();
        void                            BeginComputeCommands();
        void                            CopyColorAttachmentToSwapchain(RenderAttachment &color_attachment);
        [[nodiscard]] vk::CommandBuffer CreateCommandBuffer(bool begin_recording, QueueType queue);
        void                            EndFrame();
        void                            EndGraphicsCommands();
        void                            EndComputeCommands();

        /// @brief Ends the command buffer's recording, submits it to a queue and waits on host for it to complete
        /// @param cb Command buffer to flush
        /// @note The command buffer is also destroyed
        void FlushCommandBuffer(vk::CommandBuffer cb, QueueType queue) const;

        std::unique_ptr<RenderAttachment> DepthStencilAttachment{};

    private:
        void create_sync_objects();
        void create_command_pools_and_buffers();
        void create_attachments();
        void create_swapchain(const Window &window);

        const DeviceContext &m_device_context;
        const vk::Device    &m_device;

        vk::SurfaceKHR             m_surface;
        std::unique_ptr<Swapchain> m_swapchain{};
        uint32_t                   m_swapchain_image_index{};

        Buffered<vk::Fence>     m_graphics_fences{};
        Buffered<vk::Semaphore> m_image_acquired_semaphores{};

        Buffered<vk::Fence>     m_compute_fences{};
        Buffered<vk::Semaphore> m_particle_compute_ready_semaphores{};
        Buffered<vk::Semaphore> m_particle_compute_complete_semaphores{};

        // vk::CommandPool             m_graphics_command_pool{};
        Buffered<vk::CommandBuffer> m_graphics_command_buffers{};

        // vk::CommandPool             m_compute_command_pool{};
        Buffered<vk::CommandBuffer> m_compute_command_buffers{};

        std::array<vk::CommandPool, 3> m_command_pools{};

        uint32_t               m_frame_index = 0;
        Buffered<FrameContext> m_frame_contexts{};
    };

} // namespace vkdemo


#endif //
