#include "window.hpp"

#include "globals.hpp"
#include "util.hpp"

#include <SDL3/SDL_vulkan.h>

namespace vkdemo {
    Window::Window(const uint32_t width, const uint32_t height) {
        check(SDL_Init(SDL_INIT_VIDEO), "Failed to init SDL");
        check(SDL_Vulkan_LoadLibrary(nullptr), "Failed to load Vulkan loader library");
        m_sdl_window = SDL_CreateWindow(globals::kApplicationName.data(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        check(m_sdl_window, "Failed to create SDL window");
        check(SDL_GetWindowSizeInPixels(m_sdl_window, &m_size.x, &m_size.y), "Failed to get window size for SDL window");
    }

    Window::~Window() {
        SDL_DestroyWindow(m_sdl_window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
    }

    bool Window::CreateSurface(const vk::Instance &instance, vk::SurfaceKHR &surface) const {
        return SDL_Vulkan_CreateSurface(m_sdl_window, instance, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface));
    }

    void Window::SetTitle(const std::string &title) const {
        SDL_SetWindowTitle(m_sdl_window, title.c_str());
    }

    glm::ivec2 Window::Size() const noexcept {
        return m_size;
    }
} // vkdemo