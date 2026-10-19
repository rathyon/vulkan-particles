#include "image.hpp"

#include "device-context.hpp"

namespace vkdemo {
    Image::~Image() {
        m_device_context.Device.destroyImageView(View);

        if (Allocation != nullptr) {
            vmaDestroyImage(m_device_context.VmaAllocator, Handle, Allocation);
        }
    }

    void Image::LayoutTransition(const vk::CommandBuffer &cb, const vk::ImageLayout new_layout) {
        // Avoid redudant transitions
        if (Layout == new_layout)
            return;

        vk::ImageMemoryBarrier2 barrier{};
        barrier.image               = Handle;
        barrier.subresourceRange    = vk::ImageSubresourceRange{Aspect, 0, 1, 0, 1};
        barrier.oldLayout           = Layout;
        barrier.newLayout           = new_layout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        vkutil::InferSrcStageAndAccessMasks(barrier);
        vkutil::InferDstStageAndAccessMasks(barrier);

        vk::DependencyInfo dependency_info{};
        dependency_info.setImageMemoryBarriers(barrier);
        cb.pipelineBarrier2(&dependency_info);
        Layout = new_layout;
    }

} // namespace vkdemo
