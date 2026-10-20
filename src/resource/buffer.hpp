#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include "vma-usage.hpp"

namespace vkdemo {
    class DeviceContext;

    class Buffer {
    public:
        explicit Buffer(const DeviceContext &context, vk::BufferUsageFlags usage_flags, uint64_t size);
        ~Buffer();

        Buffer(const Buffer &)             = delete;
        Buffer(const Buffer &&)            = delete;
        Buffer &operator=(const Buffer &)  = delete;
        Buffer &operator=(const Buffer &&) = delete;

        vk::Buffer        Handle{};
        VmaAllocation     Allocation{nullptr};
        VmaAllocationInfo AllocationInfo{};

    private:
        const DeviceContext &m_device_context;
    };

} // namespace vkdemo

#endif // BUFFER_HPP
