#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "image.hpp"

namespace vkdemo {

    // Textures are images that can also be sampled in shaders (and other uses)
    class Texture : Image {
        explicit Texture(const DeviceContext &device_context) : Image(device_context) {}
        ~Texture() override;

        vk::DeviceMemory        DeviceMemory{};
        vk::Extent2D            Extent{};
        uint8_t                 MipLevels{};
        uint8_t                 LayerCount{};
        vk::DescriptorImageInfo Descriptor{};
        vk::Sampler             Sampler{};
    };

} // namespace vkdemo

#endif // TEXTURE_HPP
