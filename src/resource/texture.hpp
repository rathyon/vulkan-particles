#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "sampled-image.hpp"

namespace vkdemo {

    enum class TextureType : uint8_t { RGBA8_2D };

    class Texture : public SampledImage {
    public:
        explicit Texture(const DeviceContext &device_context, TextureType type);
        ~Texture() override = default;

        Texture(const Texture &)             = delete;
        Texture(const Texture &&)            = delete;
        Texture &operator=(const Texture &)  = delete;
        Texture &operator=(const Texture &&) = delete;

        void HostLayoutTransition(vk::ImageLayout new_layout);
        void LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout) override;
    };

} // namespace vkdemo

#endif // TEXTURE_HPP
