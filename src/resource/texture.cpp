#include "texture.hpp"

#include "device-context.hpp"
#include "util.hpp"

namespace vkdemo {
    Texture::Texture(const DeviceContext &device_context, const TextureType type) : SampledImage(device_context) {
        // For now, only 1 TextureType exists (Color, RGBA8, 2D)
        static constexpr vk::Format kFormat = globals::kColorImageFormat;
        Format                              = kFormat;
        Aspect                              = vk::ImageAspectFlagBits::eColor;
        Descriptor.imageLayout              = Layout;

        vk::FormatProperties format_properties{};
        m_device_context.PhysicalDevice.getFormatProperties(kFormat, &format_properties);
        check(static_cast<bool>(format_properties.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eStorageImage)), "Device does not contain format capable of specified usage");

        vk::ImageCreateInfo create_info{};
        create_info.imageType   = vk::ImageType::e2D;
        create_info.format      = kFormat;
        create_info.extent      = globals::kRenderExtent3D;
        create_info.mipLevels   = 1;
        create_info.arrayLayers = 1;
        create_info.samples     = vk::SampleCountFlagBits::e1;
        create_info.tiling      = vk::ImageTiling::eOptimal;

        // Host transfer usage to allow host layout transitions
        create_info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eHostTransfer;

        // Optimization point: it would be more efficient to use exclusive and perform queue ownership transfers, but I don't have the time to do that right now
        const bool graphics_and_compute_same_family = m_device_context.QueueFamilies[Graphics] == m_device_context.QueueFamilies[Compute];
        create_info.sharingMode                     = graphics_and_compute_same_family ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount           = graphics_and_compute_same_family ? 1 : 2;
        std::vector<uint32_t> queue_family_indices{m_device_context.QueueFamilies[Graphics], m_device_context.QueueFamilies[Compute]};
        create_info.pQueueFamilyIndices = queue_family_indices.data();

        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.usage    = VMA_MEMORY_USAGE_AUTO;
        allocation_create_info.flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocation_create_info.priority = 1.0f;

        check(vmaCreateImage(m_device_context.VmaAllocator, reinterpret_cast<VkImageCreateInfo *>(&create_info), &allocation_create_info, reinterpret_cast<VkImage *>(&Handle), &Allocation, &AllocationInfo));

        create_image_view();
    }

    void Texture::HostLayoutTransition(vk::ImageLayout new_layout) {
        // Avoid redundant transitions
        if (Layout == new_layout)
            return;

        m_device_context.Device.transitionImageLayout(vk::HostImageLayoutTransitionInfo{Handle, vk::ImageLayout::eUndefined, new_layout, vk::ImageSubresourceRange{Aspect, 0, 1, 0, 1}});

        Layout                 = new_layout;
        Descriptor.imageLayout = new_layout;
    }

    void Texture::LayoutTransition(const vk::CommandBuffer &cb, vk::ImageLayout new_layout) {
        Descriptor.imageLayout = new_layout;
        Image::LayoutTransition(cb, new_layout);
    }
} // namespace vkdemo
