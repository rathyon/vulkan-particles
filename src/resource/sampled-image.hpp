#ifndef SAMPLED_IMAGE_HPP
#define SAMPLED_IMAGE_HPP

#include "image.hpp"

namespace vkdemo {

    class SampledImage : public Image {
    public:
        SampledImage(const SampledImage &)             = delete;
        SampledImage(const SampledImage &&)            = delete;
        SampledImage &operator=(const SampledImage &)  = delete;
        SampledImage &operator=(const SampledImage &&) = delete;

        vk::DescriptorImageInfo Descriptor{};

    protected:
        explicit SampledImage(const DeviceContext &context);
        ~SampledImage() override;

        void create_image_view();
    };

} // namespace vkdemo

#endif // SAMPLED_IMAGE_HPP
