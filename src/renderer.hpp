#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "device-context.hpp"
#include "renderer-context.hpp"
#include "resource/buffer.hpp"
#include "shader-registry.hpp"
#include "world/camera.hpp"

namespace vkdemo {
    class Window;

    /// @brief Utility class used for simplifying and wrapping a "pass".
    class RenderPass {
    public:
        RenderPass(vk::Device device) : m_device(device) {}
        ~RenderPass() {
            for (auto &descriptor_set_layout: DescriptorSetLayouts) {
                m_device.destroyDescriptorSetLayout(descriptor_set_layout);
            }
            m_device.destroyPipelineLayout(PipelineLayout);
            m_device.destroyPipeline(Pipeline);
        }

        vk::Pipeline                             Pipeline{};
        vk::PipelineLayout                       PipelineLayout{};
        std::vector<vk::DescriptorSetLayout>     DescriptorSetLayouts{};
        std::vector<Buffered<vk::DescriptorSet>> DescriptorSets{};

    private:
        vk::Device m_device;
    };

    /// @brief Class that defines and runs all the render and compute operations, as well as any startup initializations.
    class Renderer {
    public:
        Renderer() = default;
        virtual ~Renderer();

        void Initialize(const Window &window);
        void RenderLoop();

    private:
        struct DebrisParticle {
            glm::vec4 Position{};
            glm::vec4 Color{};
            float     Radius{};
            float     Theta{};
            float     AngularVelocity{};
        };

        struct World {
            float    Time{};      // in seconds
            float    TimeDelta{}; // in seconds
            uint32_t ParticleCount{globals::kParticleCount};
        };

        void create_shader_passes();
        void create_compute_pass();
        void create_particle_draw_pass();
        void create_blur_passes();

        void prepare_resources();
        void prepare_starting_conditions();
        void create_descriptor_pool();
        void create_buffers();
        void create_images();

        void create_descriptor_sets();
        void create_particle_compute_descriptor_sets();
        void create_particle_draw_descriptor_sets();
        void create_blur_descriptor_sets();

        void particle_compute_pass();
        void particle_draw_pass();
        void blur_pass();

        DeviceContext                    m_device_context{};
        const Window                    *m_window;
        std::unique_ptr<RendererContext> m_context;
        std::unique_ptr<ShaderRegistry>  m_shader_registry;

        FrameContext *m_frame_context = nullptr;

        vk::DescriptorPool m_descriptor_pool{};

        std::unique_ptr<RenderPass>                 m_compute_pass{};
        std::unique_ptr<RenderPass>                 m_particle_draw_pass{};
        std::unique_ptr<RenderPass>                 m_downsample_blur_pass{};
        std::unique_ptr<RenderPass>                 m_upsample_blur_pass{};
        Buffered<std::unique_ptr<RenderAttachment>> m_main_color_attachments{};
        Buffered<std::unique_ptr<Buffer>>           m_particle_storage_buffers{};
        Buffered<std::unique_ptr<Buffer>>           m_world_uniform_buffers{};
        vk::Sampler                                 m_sampler{};
        vk::DescriptorImageInfo                     m_sampler_descriptor{};

        // The main color attachment is used in the first and last pass of the dual kawase blur, so two image allocations can be saved (e.g allocate 5 instead of 7)
        static constexpr uint8_t kBlurStepAttachmentCount = globals::kDualKawaseBlurLevelCount * 2 - 1;
        static_assert(kBlurStepAttachmentCount % 2 == 1); // guarantee odd count of blur step attachments

        std::array<std::unique_ptr<RenderAttachment>, kBlurStepAttachmentCount> m_blur_step_attachments{};

        static constexpr uint8_t kImageDescriptorCount = globals::kFramesInFlight + kBlurStepAttachmentCount;


        std::vector<DebrisParticle> m_starting_particle_states{globals::kParticleCount};
        World                       m_world_data{};
        std::unique_ptr<Camera>     m_camera{};
    };
} // namespace vkdemo


#endif // RENDERER_HPP
