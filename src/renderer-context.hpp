#ifndef RENDERER_CONTEXT_HPP
#define RENDERER_CONTEXT_HPP

#include "globals.hpp"
#include "util.hpp"
#include "vk-util.hpp"

#include <vulkan/vulkan.hpp>

namespace vkdemo {
    class Window;
    class DeviceContext;

    struct FrameContext {
        uint8_t           Index = 0;
        vk::CommandBuffer GraphicsCommandBuffer{};
    };

    class RendererContext {
    public:
        explicit RendererContext(const DeviceContext &device_context);
        virtual ~RendererContext();

        void                        Initialize(const Window &window);
        [[nodiscard]] FrameContext &BeginFrame();
        void                        EndFrame();

        Buffered<ImageDescription> MainColorAttachments{};
        ImageDescription           DepthStencilAttachment{};

    private:
        static constexpr vk::Format        kSwapchainFormat     = vk::Format::eB8G8R8A8Srgb;
        static constexpr vk::ColorSpaceKHR kSwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

        void create_sync_objects();
        void create_command_buffers();
        void create_attachments();
        void create_swapchain(const Window &window);

        const DeviceContext &m_device_context;
        const vk::Device    &m_device;

        vk::SurfaceKHR                m_surface;
        vk::SwapchainKHR              m_swapchain;
        std::vector<ImageDescription> m_swapchain_images{};
        uint32_t                      m_swapchain_image_index{};

        Buffered<vk::Fence>        m_fences{};
        Buffered<vk::Semaphore>    m_image_acquired_semaphores{};
        std::vector<vk::Semaphore> m_render_complete_semaphores{};

        vk::CommandPool             m_graphics_command_pool{};
        Buffered<vk::CommandBuffer> m_graphics_command_buffers{};

        uint8_t                m_frame_index = 0;
        Buffered<FrameContext> m_frame_contexts{};
    };

} // namespace vkdemo


#endif // RENDERER_CONTEXT_HPP
