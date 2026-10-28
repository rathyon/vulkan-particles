#include "buffer.hpp"

#include "device-context.hpp"
#include "vma-usage.hpp"

namespace vkdemo {
    Buffer::Buffer(const DeviceContext &context, const vk::BufferUsageFlags usage_flags, const bool host_access, const uint64_t size) : Size(size), m_device_context(context) {
        vk::BufferCreateInfo create_info{{}, size, usage_flags};
        create_info.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;

        VmaAllocationCreateInfo vma_allocation_ci{};

        if (host_access) {
            vma_allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            vma_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
        } else {
            vma_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        }

        vmaCreateBuffer(m_device_context.VmaAllocator, reinterpret_cast<VkBufferCreateInfo *>(&create_info), &vma_allocation_ci, reinterpret_cast<VkBuffer *>(&Descriptor.buffer), &Allocation,
                        &AllocationInfo);

        Descriptor.offset = 0;
        Descriptor.range  = size;
        DeviceAddress = m_device_context.Device.getBufferAddress(vk::BufferDeviceAddressInfo{Descriptor.buffer});
    }
    Buffer::~Buffer() { vmaDestroyBuffer(m_device_context.VmaAllocator, Descriptor.buffer, Allocation); }
} // namespace vkdemo
