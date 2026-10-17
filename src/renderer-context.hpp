#ifndef RENDERER_CONTEXT_HPP
#define RENDERER_CONTEXT_HPP

#include "globals.hpp"
#include "utils.hpp"

#include <vulkan/vulkan.hpp>
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    class RendererContext {
    public:
        explicit RendererContext(const DeviceContext &device_context);
        virtual ~RendererContext();

        void Initialize();

    private:
        struct ImageAttachment {
            vk::Image     Image;
            vk::ImageView ImageView;
            VmaAllocation Allocation;
        };

        Buffered<ImageAttachment> m_color_attachments{};
        ImageAttachment           m_depth_attachment{};

        void create_sync_objects();
        void create_command_buffers();
        void create_attachments();

        const DeviceContext &m_device_context;
        const vk::Device    &m_device;
        const vk::Extent2D   m_render_extent_2d{globals::kRenderExtent.x, globals::kRenderExtent.y};
        const vk::Extent3D   m_render_extent{globals::kRenderExtent.x, globals::kRenderExtent.y, 1};

        Buffered<vk::Fence>        m_fences{};
        Buffered<vk::Semaphore>    m_image_acquired_semaphores{};
        std::vector<vk::Semaphore> m_render_complete_semaphores{};

        vk::CommandPool             m_graphics_command_pool{};
        Buffered<vk::CommandBuffer> m_graphics_command_buffers{};
    };

} // namespace vkdemo


#endif // RENDERER_CONTEXT_HPP
