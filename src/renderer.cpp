#include "renderer.hpp"

#include <SDL3/SDL.h>
#include <random>

namespace vkdemo {

    Renderer::~Renderer() { m_device_context.Device.destroyDescriptorPool(m_descriptor_pool); }

    void Renderer::Initialize(const Window &window) {
        m_device_context.Initialize();

        m_context         = std::make_unique<RendererContext>(m_device_context);
        m_shader_registry = std::make_unique<ShaderRegistry>(m_device_context);

        m_context->Initialize(window);
        m_shader_registry->Initialize();
        m_shader_registry->LoadShaders();

        m_camera = std::make_unique<Camera>(m_device_context, Camera::Type::LookAt);

        prepare_starting_conditions();
        prepare_resources();
        create_shader_passes();
    }

    void Renderer::RenderLoop() {
        uint64_t last_time = SDL_GetTicks();
        uint64_t current_time = last_time;

        bool     stop = false;
        while (!stop) {
            m_frame_context = m_context->BeginFrame();

            // Update world buffer
            current_time = SDL_GetTicks();
            m_world_data.Time = static_cast<float>(current_time) / 1000.0f;
            m_world_data.TimeDelta = static_cast<float>(current_time - last_time) / 1000.0f;
            last_time = current_time;

            memcpy(m_world_uniform_buffers[m_frame_context->Index]->AllocationInfo.pMappedData, &m_world_data, sizeof(World));

            // temporary camera orbit
            static constexpr float kOrbitRadius = 3.0f;
            static constexpr float kOrbitSpeed  = 0.5f;

            m_camera->SetTranslation({glm::sin(m_world_data.Time * kOrbitSpeed) * kOrbitRadius, 2.5f, glm::cos(m_world_data.Time * kOrbitSpeed) * kOrbitRadius});
            m_camera->Update(m_frame_context);

            m_context->BeginComputeCommands();
            particle_compute_pass();
            m_context->EndComputeCommands();

            m_context->BeginGraphicsCommands();
            particle_draw_pass();
            postprocess_pass();
            // todo DrawUI here
            m_context->EndGraphicsCommands();

            m_context->EndFrame();

            for (SDL_Event event; SDL_PollEvent(&event);) {
                if (event.type == SDL_EVENT_QUIT) {
                    stop = true;
                    break;
                }
            }
        }

        // call waitIdle to allow all commands to finish before shutting down
        m_device_context.Device.waitIdle();
    }

    void Renderer::create_shader_passes() {
        create_compute_pass();
        create_particle_draw_pass();
        create_postprocess_pass();
        update_descriptor_sets();
    }

    void Renderer::create_compute_pass() {
        m_compute_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings{// Binding 0: previous frame particle buffer
                                                                        {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
                                                                        // Binding 1: current frame particle buffer
                                                                        {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
                                                                        // Binding 2: world uniform buffer
                                                                        {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_compute_pass->DescriptorSetLayout = m_device_context.Device.createDescriptorSetLayout(descriptor_layout);

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_compute_pass->DescriptorSetLayout);

        m_compute_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_compute_pass->PipelineLayout != nullptr);

        GlslShaderModule                 *shader_module = m_shader_registry->GetGlslModule("particle-compute.comp");
        vk::PipelineShaderStageCreateInfo shader_stage_ci{{}, vk::ShaderStageFlagBits::eCompute, shader_module->Module, "main"};

        vk::ComputePipelineCreateInfo pipeline_ci{};
        pipeline_ci.layout = m_compute_pass->PipelineLayout;
        pipeline_ci.stage  = shader_stage_ci;

        const auto result = m_device_context.Device.createComputePipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create compute pipeline!");
        m_compute_pass->Pipeline = result.value;
    }

    void Renderer::create_particle_draw_pass() {
        m_particle_draw_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings{// Binding 0: camera uniform buffer
                                                                        {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_particle_draw_pass->DescriptorSetLayout = m_device_context.Device.createDescriptorSetLayout(descriptor_layout);

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_particle_draw_pass->DescriptorSetLayout);
        m_particle_draw_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_particle_draw_pass->PipelineLayout != nullptr);

        GlslShaderModule *vert_module = m_shader_registry->GetGlslModule("particle-draw.vert");
        GlslShaderModule *frag_module = m_shader_registry->GetGlslModule("particle-draw.frag");

        std::vector shader_stages{vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, vert_module->Module, globals::kGlslEntryFunctionName.data()},
                                  vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, frag_module->Module, globals::kGlslEntryFunctionName.data()}};

        // Particles will be rendered as points, their point size is defined in the shader
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{{}, vk::PrimitiveTopology::ePointList, false};
        vk::PipelineViewportStateCreateInfo      viewport_state{{}, 1, {}, 1, {}};

        vk::PipelineRasterizationStateCreateInfo rasterization_state{{}, false, false, vk::PolygonMode::ePoint, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise};
        rasterization_state.lineWidth = 1.0f;
        vk::PipelineMultisampleStateCreateInfo multisample_state{{}, vk::SampleCountFlagBits::e1};

        vk::PipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable         = true;
        color_blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        color_blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.colorBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo   color_blend_state{{}, false, {}, {color_blend_attachment_state}};
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{{}, false, false, vk::CompareOp::eAlways, false, false};
        std::vector                             dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo      dynamic_state(vk::PipelineDynamicStateCreateFlags(), dynamic_states);

        std::vector input_bindings = {vk::VertexInputBindingDescription{0, sizeof(Particle), vk::VertexInputRate::eVertex}};

        std::vector<vk::VertexInputAttributeDescription> input_attributes = {
                // Location 0 : Position
                {0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Particle, Position)},
                // Location 1 : Velocity
                {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Particle, Velocity)},
                // Location 2 : Color
                {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Particle, Color)},
        };

        vk::PipelineVertexInputStateCreateInfo vertex_input_ci{{}, input_bindings, input_attributes};

        vk::GraphicsPipelineCreateInfo pipeline_ci{};
        pipeline_ci.setStages(shader_stages);
        pipeline_ci.pVertexInputState   = &vertex_input_ci;
        pipeline_ci.pInputAssemblyState = &input_assembly_state;
        pipeline_ci.pViewportState      = &viewport_state;
        pipeline_ci.pRasterizationState = &rasterization_state;
        pipeline_ci.pMultisampleState   = &multisample_state;
        pipeline_ci.pDepthStencilState  = &depth_stencil_state;
        pipeline_ci.pColorBlendState    = &color_blend_state;
        pipeline_ci.pDynamicState       = &dynamic_state;
        pipeline_ci.layout              = m_particle_draw_pass->PipelineLayout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        pipeline_ci.pNext = &pipeline_rendering_create_info;

        const auto result = m_device_context.Device.createGraphicsPipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_particle_draw_pass->Pipeline = result.value;
    }

    void Renderer::create_postprocess_pass() {
        m_postprocess_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector set_layout_bindings{// Binding 0: input image
                                        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment},
                                        // Binding 1: sampler
                                        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_postprocess_pass->DescriptorSetLayout = m_device_context.Device.createDescriptorSetLayout(descriptor_layout);
        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_postprocess_pass->DescriptorSetLayout);
        m_postprocess_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_postprocess_pass->PipelineLayout != nullptr);

        // The postprocess shader uses a technique described by Sascha Willems:
        // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
        GlslShaderModule *vert_module = m_shader_registry->GetGlslModule("postprocess.vert");
        GlslShaderModule *frag_module = m_shader_registry->GetGlslModule("postprocess.frag");

        std::vector shader_stages{vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, vert_module->Module, globals::kGlslEntryFunctionName.data()},
                                  vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, frag_module->Module, globals::kGlslEntryFunctionName.data()}};

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{{}, vk::PrimitiveTopology::eTriangleList, false};
        vk::PipelineViewportStateCreateInfo      viewport_state{{}, 1, {}, 1, {}};

        // Culling and FrontFace are setup specifically for the geometry-less "fake fullscreen quad" technique
        vk::PipelineRasterizationStateCreateInfo rasterization_state{{}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise, false, {}, {}, {}, 1.0f};
        vk::PipelineMultisampleStateCreateInfo   multisample_state{{}, vk::SampleCountFlagBits::e1};
        vk::PipelineColorBlendAttachmentState    color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable         = true;
        color_blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.colorBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo color_blend_state{{}, false, {}, 1, &color_blend_attachment_state};

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{{}, false, false, vk::CompareOp::eAlways, false, false};

        std::vector<vk::DynamicState>      dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamic_state(vk::PipelineDynamicStateCreateFlags(), dynamic_states);

        // Fullscreen shader is "geometry-less"
        vk::PipelineVertexInputStateCreateInfo vertex_input_ci{};

        vk::GraphicsPipelineCreateInfo pipeline_ci{};
        pipeline_ci.setStages(shader_stages);
        pipeline_ci.pVertexInputState   = &vertex_input_ci;
        pipeline_ci.pInputAssemblyState = &input_assembly_state;
        pipeline_ci.pViewportState      = &viewport_state;
        pipeline_ci.pRasterizationState = &rasterization_state;
        pipeline_ci.pMultisampleState   = &multisample_state;
        pipeline_ci.pDepthStencilState  = &depth_stencil_state;
        pipeline_ci.pColorBlendState    = &color_blend_state;
        pipeline_ci.pDynamicState       = &dynamic_state;
        pipeline_ci.layout              = m_postprocess_pass->PipelineLayout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        pipeline_ci.pNext = &pipeline_rendering_create_info;

        const auto result = m_device_context.Device.createGraphicsPipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_postprocess_pass->Pipeline = result.value;
    }

    void Renderer::prepare_resources() {
        create_descriptor_pool();
        create_buffers();
        create_images();
    }

    void Renderer::prepare_starting_conditions() {
        std::mt19937 rng_engine{globals::kRNGSeed};

        // Define initial particle states using random number generators and distributions
        std::uniform_real_distribution<float> uniform_distribution(-1.0f, 1.0f);
        std::normal_distribution<float>       normal_distribution(0, 0.5f);

        std::stringstream log_string{};
        uint32_t          idx = 0;

        static constexpr bool kLogParticleStartingConditions = false;

        for (auto &particle: m_starting_particle_states) {
            particle.Position.x = uniform_distribution(rng_engine);
            particle.Position.z = uniform_distribution(rng_engine);

            // Their height range is inversely proportional to how close to the world center they are
            // The maximum height will be 2
            float world_center_distance = glm::length(glm::vec2{particle.Position.x, particle.Position.z});
            std::uniform_real_distribution<float> height_distribution(0, std::lerp(2.f, 0.f, world_center_distance / sqrt(2.0f)));
            particle.Position.y = height_distribution(rng_engine);

            particle.Velocity = glm::vec4{0, 0, 0, 0};

            // For now let's color the particles by their starting position
            particle.Color = glm::vec4{(particle.Position + 1.0f) / 2.0f};
            particle.Color.a = 1.0f;

            if constexpr (kLogParticleStartingConditions)
                log_string << "Particle " << idx << ": " << glm::to_string(particle.Position) << " | " << glm::to_string(particle.Velocity) << " | " << glm::to_string(particle.Color) << "\n";

            ++idx;
        }

        if constexpr (kLogParticleStartingConditions)
            std::cout << log_string.str() << std::endl;

        // world state
        m_camera->SetTranslation({0, 5, -5});
        m_camera->SetLookAt({0, 0, 0});
    }

    void Renderer::create_descriptor_pool() {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {// Last frame particle buffer + current frame particle buffer
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 2 * globals::kFramesInFlight},
                                                          // Compute uniform buffer + Camera uniform buffer
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2 * globals::kFramesInFlight},
                                                          // Sampled color attachment for post processing
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, globals::kFramesInFlight},
                                                          // Samplers
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eSampler, globals::kFramesInFlight}};

        vk::DescriptorPoolCreateInfo ci{};
        ci.setPoolSizes(pool_sizes);
        ci.maxSets = static_cast<uint32_t>(pool_sizes.size()) * globals::kFramesInFlight;

        m_descriptor_pool = m_device_context.Device.createDescriptorPool(ci);
        check(m_descriptor_pool != nullptr);
    }

    void Renderer::create_buffers() {
        static constexpr uint64_t kBufferSize = sizeof(Particle) * globals::kParticleCount;

        // Create particle storage buffers
        // upload to staging buffer and then copy from staging onto device buffer
        auto staging_buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eTransferSrc, true, kBufferSize);
        memcpy(staging_buffer->AllocationInfo.pMappedData, m_starting_particle_states.data(), kBufferSize);

        for (auto &buffer: m_particle_storage_buffers) {
            buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, false,
                                              kBufferSize);

            const vk::CommandBuffer upload_cb = m_context->CreateCommandBuffer(true, Graphics);

            vk::BufferCopy copy_region{0, 0, buffer->Size};
            upload_cb.copyBuffer(staging_buffer->Descriptor.buffer, buffer->Descriptor.buffer, {copy_region});

            // Perform queue ownership transfer barrier from graphics to compute (if queues differ)
            if (m_device_context.QueueFamilies[Graphics] != m_device_context.QueueFamilies[Compute]) {
                vk::BufferMemoryBarrier barrier{};
                barrier.srcAccessMask       = vk::AccessFlagBits::eVertexAttributeRead;
                barrier.dstAccessMask       = vk::AccessFlagBits::eNone;
                barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Graphics];
                barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
                barrier.buffer              = buffer->Descriptor.buffer;
                barrier.offset              = 0;
                barrier.size                = buffer->Size;

                upload_cb.pipelineBarrier(vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {barrier}, {});
            }

            m_context->FlushCommandBuffer(upload_cb, Graphics);
        }

        // Create world uniform buffers
        for (auto &buffer: m_world_uniform_buffers) {
            buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(World));
        }
    }

    void Renderer::create_images() { m_post_process_effect_attachment = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color); }

    void Renderer::update_descriptor_sets() {
        update_particle_compute_descriptor_sets();
        update_particle_draw_descriptor_sets();
        update_postprocess_descriptor_sets();
    }

    void Renderer::update_particle_compute_descriptor_sets() {
        m_compute_pass->DescriptorSets.resize(1);

        for (size_t frame_index = 0; frame_index < m_compute_pass->DescriptorSets[0].size(); ++frame_index) {
            vk::DescriptorSet &descriptor_set = m_compute_pass->DescriptorSets[0][frame_index];

            const uint32_t prev_frame = (frame_index - 1 + globals::kFramesInFlight) % globals::kFramesInFlight;

            vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_compute_pass->DescriptorSetLayout}};
            check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));

            std::vector write_descriptor_sets = {// Binding 0: previous frame particle buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, m_particle_storage_buffers[prev_frame]->Descriptor},
                                                 // Binding 1: current frame particle buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eStorageBuffer, nullptr, m_particle_storage_buffers[frame_index]->Descriptor},
                                                 // Binding 2: world uniform buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 2, 0, vk::DescriptorType::eUniformBuffer, nullptr, m_world_uniform_buffers[frame_index]->Descriptor}};

            m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        }
    }

    void Renderer::update_particle_draw_descriptor_sets() {
        m_particle_draw_pass->DescriptorSets.resize(1);

        for (size_t frame_index = 0; frame_index < m_particle_draw_pass->DescriptorSets[0].size(); ++frame_index) {
            vk::DescriptorSet &descriptor_set = m_particle_draw_pass->DescriptorSets[0][frame_index];

            vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_particle_draw_pass->DescriptorSetLayout}};
            check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));

            std::vector write_descriptor_sets = {// Binding 0: camera uniform buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, nullptr, m_camera->UniformBuffers[frame_index]->Descriptor}};

            m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        }
    }

    void Renderer::update_postprocess_descriptor_sets() {
        m_postprocess_pass->DescriptorSets.resize(1);

        for (auto &descriptor_set: m_postprocess_pass->DescriptorSets[0]) {
            vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_postprocess_pass->DescriptorSetLayout}};
            check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));

            // Copy the descriptor and apply the correct image layout to expect
            vk::DescriptorImageInfo expected_descriptor = m_post_process_effect_attachment->Descriptor;
            expected_descriptor.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;

            std::vector write_descriptor_sets = {vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eSampledImage, expected_descriptor},
                                                 vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eSampler, expected_descriptor}};

            m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        }
    }

    void Renderer::particle_compute_pass() {
        const auto &cb = m_frame_context->ComputeCB;

        // Add memory barrier to ensure that the (graphics) vertex shader has fetched attributes before compute starts to write to the buffer
        const bool different_families = m_device_context.QueueFamilies[Graphics] != m_device_context.QueueFamilies[Compute];
        if (different_families) {
            vk::BufferMemoryBarrier barrier{};
            barrier.srcAccessMask       = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask       = vk::AccessFlagBits::eShaderWrite;
            barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Graphics];
            barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
            const auto &buffer          = m_particle_storage_buffers[m_frame_context->Index];
            barrier.buffer              = buffer->Descriptor.buffer;
            barrier.offset              = 0;
            barrier.size                = buffer->Size;

            cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {barrier}, {});
        }

        cb.bindPipeline(vk::PipelineBindPoint::eCompute, m_compute_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_compute_pass->PipelineLayout, 0, {m_compute_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.dispatch(globals::kParticleCount / 256, 1, 1);

        // Add barrier to ensure that compute shader has finished writing to the buffer
        // Without this the (rendering) vertex shader may display incomplete results (partial data from last frame)
        if (different_families) {
            vk::BufferMemoryBarrier barrier{};
            barrier.srcAccessMask       = vk::AccessFlagBits::eShaderWrite;
            barrier.dstAccessMask       = vk::AccessFlagBits::eNone;
            barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
            barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Graphics];
            const auto &buffer          = m_particle_storage_buffers[m_frame_context->Index];
            barrier.buffer              = buffer->Descriptor.buffer;
            barrier.offset              = 0;
            barrier.size                = buffer->Size;

            cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {barrier}, {});
        }
    }

    void Renderer::particle_draw_pass() {
        const auto &cb     = m_frame_context->GraphicsCB;
        const auto &buffer = m_particle_storage_buffers[m_frame_context->Index];

        // todo explain
        const bool different_families = m_device_context.QueueFamilies[Graphics] != m_device_context.QueueFamilies[Compute];
        if (different_families) {
            vk::BufferMemoryBarrier barrier{};
            barrier.srcAccessMask       = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask       = vk::AccessFlagBits::eVertexAttributeRead;
            barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
            barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Graphics];
            barrier.buffer              = buffer->Descriptor.buffer;
            barrier.offset              = 0;
            barrier.size                = buffer->Size;

            cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {barrier}, {});
        }

        vk::Viewport viewport{{}, {}, globals::kRenderExtent2D.width, globals::kRenderExtent3D.height, 0, 1};
        vk::Rect2D   scissor{{}, globals::kRenderExtent2D};
        cb.setViewport(0, {viewport});
        cb.setScissor(0, {scissor});

        auto &color_attachment = *m_post_process_effect_attachment;
        color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);

        vk::RenderingAttachmentInfo color_attachment_info{};
        color_attachment_info.imageView        = color_attachment.Descriptor.imageView;
        color_attachment_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment_info.loadOp           = vk::AttachmentLoadOp::eClear;
        color_attachment_info.clearValue.color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f};
        color_attachment_info.storeOp          = vk::AttachmentStoreOp::eStore;

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea.extent = globals::kRenderExtent2D;
        rendering_info.layerCount        = 1;
        rendering_info.setColorAttachments({color_attachment_info});

        cb.beginRendering(rendering_info);

        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_particle_draw_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_particle_draw_pass->PipelineLayout, 0, {m_particle_draw_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.bindVertexBuffers(0, {buffer->Descriptor.buffer}, {0});
        cb.draw(globals::kParticleCount, 1, 0, 0);

        cb.endRendering();

        if (different_families) {
            vk::BufferMemoryBarrier barrier{};
            barrier.srcAccessMask       = vk::AccessFlagBits::eVertexAttributeRead;
            barrier.dstAccessMask       = vk::AccessFlagBits::eNone;
            barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Graphics];
            barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
            barrier.buffer              = buffer->Descriptor.buffer;
            barrier.offset              = 0;
            barrier.size                = buffer->Size;

            cb.pipelineBarrier(vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {barrier}, {});
        }
    }

    void Renderer::postprocess_pass() {
        const auto &cb = m_frame_context->GraphicsCB;

        m_post_process_effect_attachment->LayoutTransition(cb, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::Viewport viewport{{}, {}, globals::kRenderExtent2D.width, globals::kRenderExtent3D.height, 0, 1};
        vk::Rect2D   scissor{{}, globals::kRenderExtent2D};
        cb.setViewport(0, {viewport});
        cb.setScissor(0, {scissor});

        auto &color_attachment = *m_context->MainColorAttachments[m_frame_context->Index];
        color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);

        vk::RenderingAttachmentInfo color_attachment_info{};
        color_attachment_info.imageView        = color_attachment.Descriptor.imageView;
        color_attachment_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment_info.loadOp           = vk::AttachmentLoadOp::eClear;
        color_attachment_info.clearValue.color = vk::ClearColorValue{0, 0, 0, 0};
        color_attachment_info.storeOp          = vk::AttachmentStoreOp::eStore;

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea.extent = globals::kRenderExtent2D;
        rendering_info.layerCount        = 1;
        rendering_info.setColorAttachments({color_attachment_info});

        cb.beginRendering(rendering_info);

        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_postprocess_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_postprocess_pass->PipelineLayout, 0, {m_postprocess_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.draw(3, 1, 0, 0);
        cb.endRendering();
    }

} // namespace vkdemo
