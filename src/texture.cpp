#include "texture.hpp"

#include "device-context.hpp"

namespace vkdemo {
    Texture::Texture(const DeviceContext &device_context) : Image(device_context) {}

    Texture::~Texture() {
        if (Sampler) {
            m_device_context.Device.destroySampler(Sampler);
        }
        if (DeviceMemory) {
            m_device_context.Device.freeMemory(DeviceMemory);
        }
    }

    void Texture::LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout) {
        Descriptor.imageLayout = new_layout;
        Image::LayoutTransition(cb, new_layout);
    }
} // namespace vkdemo
