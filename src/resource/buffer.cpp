#include "buffer.hpp"

#include "device-context.hpp"
#include "vma-usage.hpp"

namespace vkdemo {
    Buffer::Buffer(const DeviceContext &context, const vk::BufferUsageFlags usage_flags, const uint64_t size) : m_device_context(context) {
        vk::BufferCreateInfo create_info{{}, size, usage_flags};

        VmaAllocationCreateInfo vma_allocation_ci{};
        vma_allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vma_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateBuffer(m_device_context.VmaAllocator, reinterpret_cast<VkBufferCreateInfo *>(&create_info), &vma_allocation_ci, reinterpret_cast<VkBuffer *>(&Handle), &Allocation, &AllocationInfo);
    }
    Buffer::~Buffer() { vmaDestroyBuffer(m_device_context.VmaAllocator, Handle, Allocation); }
} // namespace vkdemo
