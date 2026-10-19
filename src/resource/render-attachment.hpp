#ifndef RENDER_ATTACHMENT_HPP
#define RENDER_ATTACHMENT_HPP
#include "sampled-image.hpp"

namespace vkdemo {

    enum class RenderAttachmentType : uint8_t {
        Color,
        DepthStencil
    };

    class RenderAttachment : public SampledImage {
    public:
        explicit RenderAttachment(const DeviceContext& context, RenderAttachmentType type);
        ~RenderAttachment() override = default;

        RenderAttachment(const Image&) = delete;
        RenderAttachment(const RenderAttachment&&) = delete;
        RenderAttachment& operator=(const RenderAttachment&) = delete;
        RenderAttachment& operator=(const RenderAttachment&&) = delete;
    };

} // namespace vkdemo

#endif // RENDER_ATTACHMENT_HPP
