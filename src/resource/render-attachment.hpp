#ifndef RENDER_ATTACHMENT_HPP
#define RENDER_ATTACHMENT_HPP
#include "globals.hpp"
#include "texture.hpp"

namespace vkdemo {

    enum class RenderAttachmentType : uint8_t { Color, DepthStencil };

    class RenderAttachment : public SampledImage {
    public:
        explicit RenderAttachment(const DeviceContext &context, RenderAttachmentType type, vk::Extent3D extent = globals::kRenderExtent3D);
        ~RenderAttachment() override = default;

        RenderAttachment(const Image &)                        = delete;
        RenderAttachment(const RenderAttachment &&)            = delete;
        RenderAttachment &operator=(const RenderAttachment &)  = delete;
        RenderAttachment &operator=(const RenderAttachment &&) = delete;

        const vk::Extent3D Extent;
    };

} // namespace vkdemo

#endif // RENDER_ATTACHMENT_HPP
