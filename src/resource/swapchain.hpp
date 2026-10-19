#ifndef SWAPCHAIN_IMAGE_HPP
#define SWAPCHAIN_IMAGE_HPP

#include "image.hpp"

namespace vkdemo {
    class DeviceContext;

    class Swapchain {
    public:
        explicit Swapchain(const DeviceContext &context, vk::SurfaceKHR surface);
        ~Swapchain();

        Swapchain(const Swapchain &)             = delete;
        Swapchain(const Swapchain &&)            = delete;
        Swapchain &operator=(const Swapchain &)  = delete;
        Swapchain &operator=(const Swapchain &&) = delete;

        vk::SwapchainKHR                    Handle{};
        std::vector<std::unique_ptr<Image>> Images{};
        std::vector<vk::Semaphore>          RenderCompleteSemaphores{};

    private:
        const DeviceContext &m_device_context;
    };

} // namespace vkdemo

#endif // SWAPCHAIN_IMAGE_HPP
