#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "vk-util.hpp"
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    // Class that encapsulates most Image logic. Swapchain images and images used exclusively as attachments are of this type.
    class Image {
    public:
        explicit Image(const DeviceContext &device_context) : m_device_context(device_context) {};
        virtual ~Image();

        vk::Image            Handle{};
        vk::ImageView        View{};
        vk::ImageLayout      Layout{vk::ImageLayout::eUndefined};
        vk::ImageAspectFlags Aspect{};
        vk::Format           Format{};
        VmaAllocation        Allocation{nullptr};

        void LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout);

    private:
        const DeviceContext &m_device_context;
    };
} // namespace vkdemo

#endif // IMAGE_HPP
