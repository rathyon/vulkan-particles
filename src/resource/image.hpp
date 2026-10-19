#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "vk-util.hpp"
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    class Image {
    public:
        explicit Image(const DeviceContext &device_context) : m_device_context(device_context) {};
        virtual ~Image();

        Image(const Image &)             = delete;
        Image(const Image &&)            = delete;
        Image &operator=(const Image &)  = delete;
        Image &operator=(const Image &&) = delete;

        vk::Image            Handle{};
        vk::ImageView        View{};
        vk::ImageLayout      Layout{vk::ImageLayout::eUndefined};
        vk::ImageAspectFlags Aspect{};
        vk::Format           Format{};
        VmaAllocation        Allocation{nullptr};

        virtual void LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout);

    protected:
        const DeviceContext &m_device_context;
    };
} // namespace vkdemo

#endif // IMAGE_HPP
