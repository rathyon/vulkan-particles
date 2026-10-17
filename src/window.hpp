#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#include "glm-usage.hpp"

namespace vkdemo {
    class Window {
    public:
        Window(uint32_t width, uint32_t height);
        ~Window();

        [[nodiscard]] bool       CreateSurface(const vk::Instance &instance, vk::SurfaceKHR &surface) const;
        [[nodiscard]] glm::ivec2 Size() const noexcept;

    private:
        SDL_Window *m_sdl_window{nullptr};
        glm::ivec2  m_size{};
    };

} // namespace vkdemo

#endif // WINDOW_HPP
