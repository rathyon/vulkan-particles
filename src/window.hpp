#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>
#include "glm-usage.hpp"

namespace vkdemo {

    /// @brief Container for Window related functionalities. Also allows for the creation of Vulkan surfaces and swapchains.
    class Window {
    public:
        Window(uint32_t width, uint32_t height);
        ~Window();

        /// @brief Creates a vulkan rendering surface for this window.
        /// @param instance The vulkan instance to create the surface for.
        /// @param surface The surface handle to write to.
        /// @return True if the surface was created, false otherwise.
        [[nodiscard]] bool CreateSurface(const vk::Instance &instance, vk::SurfaceKHR &surface) const;

        /// @brief Sets the title of the window.
        /// @param title The title to set.
        void SetTitle(const std::string &title) const;

        /// @brief Returns the client size of the window, in pixels.
        /// @return The client size, in pixels.
        [[nodiscard]] glm::ivec2 Size() const noexcept;

    private:
        SDL_Window *m_sdl_window{nullptr};
        glm::ivec2  m_size{};
    };

} // namespace vkdemo

#endif // WINDOW_HPP
