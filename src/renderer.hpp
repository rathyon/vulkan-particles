#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "device-context.hpp"
#include "renderer-context.hpp"
#include "shader-registry.hpp"

namespace vkdemo {
    class Window;

    class Renderer {
    public:
        Renderer() = default;
        virtual ~Renderer();

        void Initialize(const Window &window);

    private:
        struct Pipeline {
            vk::Pipeline       Handle;
            vk::PipelineLayout Layout;
        };

        void create_pipelines();
        void create_fullscreen_pipeline();

        DeviceContext                    m_device_context{};
        std::unique_ptr<RendererContext> m_context;
        std::unique_ptr<ShaderRegistry>  m_shader_registry;

        Pipeline m_fullscreen_pipeline{};
    };
} // namespace vkdemo


#endif // RENDERER_HPP
