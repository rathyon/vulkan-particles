#include "renderer.hpp"

#include "device-context.hpp"
#include "utils.hpp"

namespace vkdemo {

    Renderer::Renderer(const DeviceContext &device_context) : m_device_context(device_context) {}

    void Renderer::Initialize() {
        create_attachments();
    }

    void Renderer::create_attachments() {
        // Create depth attachment
        vk::ImageCreateInfo depth_create_info{};
        depth_create_info.imageType     = vk::ImageType::e2D;
        depth_create_info.format        = vk::Format::eD32SfloatS8Uint;
        depth_create_info.extent        = m_render_extent;
        depth_create_info.mipLevels     = 1;
        depth_create_info.arrayLayers   = 1;
        depth_create_info.samples       = vk::SampleCountFlagBits::e1;
        depth_create_info.tiling        = vk::ImageTiling::eOptimal;
        depth_create_info.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depth_create_info.initialLayout = vk::ImageLayout::eUndefined;

        VmaAllocationCreateInfo depth_alloc_info{};
        depth_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        depth_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

        check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&depth_create_info), &depth_alloc_info, reinterpret_cast<VkImage *>(&DepthAttachment.Image),
                             &DepthAttachment.Allocation, nullptr),
              "Failed to create depth attachment");

        vk::ImageViewCreateInfo depth_view_create_info{};
        depth_view_create_info.image                       = DepthAttachment.Image;
        depth_view_create_info.viewType                    = vk::ImageViewType::e2D;
        depth_view_create_info.format                      = depth_create_info.format;
        depth_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        depth_view_create_info.subresourceRange.levelCount = 1;
        depth_view_create_info.subresourceRange.layerCount = 1;

        DepthAttachment.ImageView = m_device_context.Device.createImageView(depth_view_create_info);
    }

} // namespace vkdemo
