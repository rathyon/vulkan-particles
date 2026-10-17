#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "globals.hpp"

#include <vulkan/vulkan.hpp>
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    class Renderer {
    public:
        explicit Renderer(const DeviceContext &device_context);
        virtual ~Renderer() = default;

        void Initialize();

    private:
        struct DepthAttachment {
            vk::Image     Image;
            vk::ImageView ImageView;
            VmaAllocation Allocation;
        } DepthAttachment{};

        void create_attachments();

        const DeviceContext &m_device_context;
        const vk::Extent2D   m_render_extent_2d{globals::kRenderExtent.x, globals::kRenderExtent.y};
        const vk::Extent3D   m_render_extent{globals::kRenderExtent.x, globals::kRenderExtent.y, 1};
    };

} // namespace vkdemo


#endif // RENDERER_HPP
