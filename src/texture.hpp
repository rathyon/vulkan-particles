#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "image.hpp"

namespace vkdemo {

    // Textures are images that can also be sampled in shaders (and other uses)
    class Texture final : public Image {
    public:
        explicit Texture(const DeviceContext &device_context);
        ~Texture() override;

        vk::DeviceMemory        DeviceMemory{};
        vk::DescriptorImageInfo Descriptor{};
        vk::Sampler             Sampler{};
    };

} // namespace vkdemo

#endif // TEXTURE_HPP
