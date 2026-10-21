#include "sampled-image.hpp"

#include "device-context.hpp"
#include "util.hpp"

namespace vkdemo {
    SampledImage::SampledImage(const DeviceContext &context) : Image(context) {
        // Standard type of sampler, does linear filtering, border clamp, black border
        vk::SamplerCreateInfo sampler_ci{};
        sampler_ci.magFilter     = vk::Filter::eLinear;
        sampler_ci.minFilter     = vk::Filter::eLinear;
        sampler_ci.mipmapMode    = vk::SamplerMipmapMode::eLinear;
        sampler_ci.addressModeU  = vk::SamplerAddressMode::eClampToBorder;
        sampler_ci.addressModeV  = sampler_ci.addressModeU;
        sampler_ci.addressModeW  = sampler_ci.addressModeU;
        sampler_ci.mipLodBias    = 0.0f;
        sampler_ci.maxAnisotropy = 1.0f;
        sampler_ci.compareOp     = vk::CompareOp::eNever;
        sampler_ci.minLod        = 0.0f;
        sampler_ci.maxLod        = 1.0f;
        sampler_ci.borderColor   = vk::BorderColor::eFloatOpaqueBlack;

        Descriptor.sampler = m_device_context.Device.createSampler(sampler_ci);
        check(Descriptor.sampler != nullptr);
    }
    SampledImage::~SampledImage() {
        if (Descriptor.imageView != nullptr)
            m_device_context.Device.destroyImageView(Descriptor.imageView);

        m_device_context.Device.destroySampler(Descriptor.sampler);
    }

    void SampledImage::create_image_view() {
        vk::ImageViewCreateInfo view_ci{};
        view_ci.image            = Handle;
        view_ci.viewType         = vk::ImageViewType::e2D;
        view_ci.format           = Format;
        view_ci.subresourceRange = vk::ImageSubresourceRange{Aspect, 0, 1, 0, 1};

        Descriptor.imageView                 = m_device_context.Device.createImageView(view_ci);
    }
} // namespace vkdemo
