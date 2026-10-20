#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    class Buffer {
    public:
        explicit Buffer(const DeviceContext &context, vk::BufferUsageFlags usage_flags, bool host_access, uint64_t size);
        ~Buffer();

        Buffer(const Buffer &)             = delete;
        Buffer(const Buffer &&)            = delete;
        Buffer &operator=(const Buffer &)  = delete;
        Buffer &operator=(const Buffer &&) = delete;

        VmaAllocation            Allocation{nullptr};
        VmaAllocationInfo        AllocationInfo{};
        vk::DescriptorBufferInfo Descriptor{};
        const uint64_t           Size;

    private:
        const DeviceContext &m_device_context;
    };

} // namespace vkdemo

#endif // BUFFER_HPP
