#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "device-context.hpp"
#include "renderer-context.hpp"
#include "shader-registry.hpp"
#include "texture.hpp"

namespace vkdemo {
    class Window;

    class Renderer {
    public:
        Renderer() = default;
        virtual ~Renderer();

        void Initialize(const Window &window);
        void RenderLoop();

    private:
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

        void create_render_passes();
        void create_graphics_pass();
        void create_compute_pass();

        void prepare_resources();
        void create_descriptor_pool();
        void create_images();
        void update_descriptor_sets();

        void fullscreen_render_pass();
        void simple_compute_pass();

        DeviceContext                    m_device_context{};
        std::unique_ptr<RendererContext> m_context;
        std::unique_ptr<ShaderRegistry>  m_shader_registry;

        FrameContext *m_frame_context = nullptr;

        vk::DescriptorPool        m_descriptor_pool{};

        std::unique_ptr<RenderPass> m_graphics_pass{};
        std::unique_ptr<RenderPass> m_compute_pass{};

        std::unique_ptr<Texture> m_storage_image{};
    };
} // namespace vkdemo


#endif // RENDERER_HPP
