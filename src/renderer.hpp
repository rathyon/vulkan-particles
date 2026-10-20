#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "device-context.hpp"
#include "renderer-context.hpp"
#include "resource/texture.hpp"
#include "resource/buffer.hpp"
#include "shader-registry.hpp"

namespace vkdemo {
    class Window;

    class RenderPass {
    public:
        RenderPass(vk::Device device) : m_device(device) {}
        ~RenderPass() {
            m_device.destroyDescriptorSetLayout(DescriptorSetLayout);
            m_device.destroyPipelineLayout(PipelineLayout);
            m_device.destroyPipeline(Pipeline);
        }

        vk::Pipeline                             Pipeline{};
        vk::PipelineLayout                       PipelineLayout{};
        vk::DescriptorSetLayout                  DescriptorSetLayout{};
        std::vector<Buffered<vk::DescriptorSet>> DescriptorSets{};

    private:
        vk::Device m_device;
    };

    class Renderer {
    public:
        Renderer() = default;
        virtual ~Renderer();

        void Initialize(const Window &window);
        void RenderLoop();

    private:
        void create_render_passes();
        void create_graphics_pass();
        void create_compute_pass();

        void prepare_resources();
        void create_descriptor_pool();
        void create_buffers();
        void create_images();
        void update_descriptor_sets();

        void simple_compute_pass();
        void particle_compute_pass();
        void world_render_pass();
        void fullscreen_render_pass();

        DeviceContext                    m_device_context{};
        std::unique_ptr<RendererContext> m_context;
        std::unique_ptr<ShaderRegistry>  m_shader_registry;

        FrameContext *m_frame_context = nullptr;

        vk::DescriptorPool m_descriptor_pool{};

        std::unique_ptr<RenderPass> m_graphics_pass{};
        std::unique_ptr<RenderPass> m_compute_pass{};

        std::unique_ptr<Texture>          m_post_process_effect_texture{};
        Buffered<std::unique_ptr<Buffer>> m_compute_storage_buffers{};
    };
} // namespace vkdemo


#endif // RENDERER_HPP
