#include "render-attachment.hpp"

#include "device-context.hpp"
#include "globals.hpp"
#include "util.hpp"

namespace vkdemo {

    RenderAttachment::RenderAttachment(const DeviceContext &context, const RenderAttachmentType type) : SampledImage(context) {
        Aspect = type == RenderAttachmentType::Color ? vk::ImageAspectFlagBits::eColor : vk::ImageAspectFlagBits::eDepth;

        // Create color attachments
        vk::ImageCreateInfo color_create_info{};
        color_create_info.imageType   = vk::ImageType::e2D;
        color_create_info.format      = type == RenderAttachmentType::Color ? globals::kColorAttachmentFormat : globals::kDepthStencilAttachmentFormat;
        color_create_info.extent      = globals::kRenderExtent3D;
        color_create_info.mipLevels   = 1;
        color_create_info.arrayLayers = 1;
        color_create_info.samples     = vk::SampleCountFlagBits::e1;
        color_create_info.tiling      = vk::ImageTiling::eOptimal;

        // Color attachments will be rendered into, sampled and copied to the swapchain image
        if (type == RenderAttachmentType::Color) {
            color_create_info.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc;
        } else {
            color_create_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }

        color_create_info.initialLayout = vk::ImageLayout::eUndefined;

        // Create image
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

        check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&color_create_info), &alloc_info, reinterpret_cast<VkImage *>(&Handle), &Allocation, nullptr),
              "Failed to create attachment");

        // Create view
        vk::ImageViewCreateInfo color_view_create_info{};
        color_view_create_info.viewType                    = vk::ImageViewType::e2D;
        color_view_create_info.format                      = color_create_info.format;
        color_view_create_info.subresourceRange.aspectMask = Aspect;
        color_view_create_info.subresourceRange.levelCount = 1;
        color_view_create_info.subresourceRange.layerCount = 1;

        color_view_create_info.image = Handle;
        View                         = m_device_context.Device.createImageView(color_view_create_info);
    }

} // namespace vkdemo
