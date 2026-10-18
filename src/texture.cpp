#include "texture.hpp"

#include "device-context.hpp"

namespace vkdemo {
    Texture::~Texture() {
        if (Sampler) {
            m_device_context.Device.destroySampler(Sampler);
        }
        if (DeviceMemory) {
            m_device_context.Device.freeMemory(DeviceMemory);
        }
    }
} // vkdemo