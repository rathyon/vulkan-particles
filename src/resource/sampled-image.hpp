#ifndef SAMPLED_IMAGE_HPP
#define SAMPLED_IMAGE_HPP

#include "image.hpp"

namespace vkdemo {

    class SampledImage : public Image {
    public:
        explicit SampledImage(const DeviceContext &device_context);
        ~SampledImage() override;

        SampledImage(const SampledImage &)             = delete;
        SampledImage(const SampledImage &&)            = delete;
        SampledImage &operator=(const SampledImage &)  = delete;
        SampledImage &operator=(const SampledImage &&) = delete;

        void LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout) override;

        vk::DescriptorImageInfo Descriptor{};
        vk::Sampler             Sampler{};
    };

} // namespace vkdemo

#endif // SAMPLED_IMAGE_HPP
