#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "device-context.hpp"
#include "renderer-context.hpp"
#include "resource/buffer.hpp"
#include "shader-registry.hpp"
#include "world/camera.hpp"

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
        struct Particle {
            glm::vec4 Position{};
            glm::vec4 Velocity{};
            glm::vec4 Color{};
        };

        struct World {
            float    DeltaTime{}; // in seconds
            uint32_t ParticleCount{globals::kParticleCount};
        };

        void create_shader_passes();
        void create_compute_pass();
        void create_particle_draw_pass();
        void create_postprocess_pass();

        void prepare_resources();
        void prepare_starting_conditions();
        void create_descriptor_pool();
        void create_buffers();
        void create_images();

        void update_descriptor_sets();
        void update_particle_compute_descriptor_sets();
        void update_particle_draw_descriptor_sets();
        void update_postprocess_descriptor_sets();

        void particle_compute_pass();
        void particle_draw_pass();
        void postprocess_pass();

        DeviceContext                    m_device_context{};
        std::unique_ptr<RendererContext> m_context;
        std::unique_ptr<ShaderRegistry>  m_shader_registry;

        FrameContext *m_frame_context = nullptr;

        vk::DescriptorPool m_descriptor_pool{};

        std::unique_ptr<RenderPass> m_compute_pass{};
        std::unique_ptr<RenderPass> m_particle_draw_pass{};
        std::unique_ptr<RenderPass> m_postprocess_pass{};

        std::unique_ptr<RenderAttachment> m_post_process_effect_attachment{};
        Buffered<std::unique_ptr<Buffer>> m_particle_storage_buffers{};
        Buffered<std::unique_ptr<Buffer>> m_world_uniform_buffers{};

        std::vector<Particle>   m_starting_particle_states{globals::kParticleCount};
        World                   m_world_data{};
        std::unique_ptr<Camera> m_camera{};
    };
} // namespace vkdemo


#endif // RENDERER_HPP
