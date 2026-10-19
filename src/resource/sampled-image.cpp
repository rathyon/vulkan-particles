#include "sampled-image.hpp"

#include "device-context.hpp"
#include "util.hpp"

namespace vkdemo {
    SampledImage::SampledImage(const DeviceContext &device_context) : Image(device_context) {
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
        Sampler                  = m_device_context.Device.createSampler(sampler_ci);
        check(Sampler != nullptr);
    }

    SampledImage::~SampledImage() {
        m_device_context.Device.destroySampler(Sampler);
    }

    void SampledImage::LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout) {
        Descriptor.imageLayout = new_layout;
        Image::LayoutTransition(cb, new_layout);
    }
} // namespace vkdemo
