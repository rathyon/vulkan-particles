#include "renderer.hpp"

#include "window.hpp"

#include <SDL3/SDL.h>
#include <random>

namespace vkdemo {

    Renderer::~Renderer() {
        m_device_context.Device.destroyDescriptorPool(m_descriptor_pool);

        // Let unique_ptr dispose these (out of scope destruction)
        for (auto &attachment: m_main_color_attachments) {
            attachment = nullptr;
        }
    }

    void Renderer::Initialize(const Window &window) {
        m_device_context.Initialize();
        m_window = &window;

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
        uint64_t last_time    = SDL_GetTicks();
        uint64_t current_time = last_time;

        bool stop = false;
        while (!stop) {
            m_frame_context = m_context->BeginFrame();

            // Update world data buffer
            current_time           = SDL_GetTicks();
            m_world_data.Time      = static_cast<float>(current_time) / 1000.0f;
            m_world_data.TimeDelta = static_cast<float>(current_time - last_time) / 1000.0f;
            last_time              = current_time;

            m_window->SetTitle("FPS: " + std::to_string(1.0f / m_world_data.TimeDelta));

            memcpy(m_world_uniform_buffers[m_frame_context->Index]->AllocationInfo.pMappedData, &m_world_data, sizeof(World));

            // temporary camera orbit
            // static constexpr float kOrbitRadius = 3.0f;
            // static constexpr float kOrbitSpeed  = 0.5f;
            // m_camera->SetTranslation({glm::sin(m_world_data.Time * kOrbitSpeed) * kOrbitRadius, 2.5f, glm::cos(m_world_data.Time * kOrbitSpeed) * kOrbitRadius});

            m_camera->Update(m_frame_context);

            m_context->BeginComputeCommands();
            particle_compute_pass();
            m_context->EndComputeCommands();

            m_context->BeginGraphicsCommands();
            particle_draw_pass();
            blur_pass();
            // draw UI here
            m_context->CopyColorAttachmentToSwapchain(*m_main_color_attachments[m_frame_context->Index]);
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
        create_blur_passes();
        create_descriptor_sets();
    }

    void Renderer::create_compute_pass() {
        m_compute_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings{// Binding 0: previous frame particle buffer
                                                                        {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
                                                                        // Binding 1: current frame particle buffer
                                                                        {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_compute_pass->DescriptorSetLayouts.emplace_back(m_device_context.Device.createDescriptorSetLayout(descriptor_layout));

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_compute_pass->DescriptorSetLayouts);

        // A pointer to the world data buffer is passed
        vk::PushConstantRange push_constant_range{};
        push_constant_range.size       = sizeof(vk::DeviceAddress);
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pipeline_layout_ci.setPushConstantRanges(push_constant_range);

        m_compute_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_compute_pass->PipelineLayout != nullptr);

        GlslShaderModule                 *shader_module = m_shader_registry->GetGlslModule("particle-compute.comp");
        vk::PipelineShaderStageCreateInfo shader_stage_ci{{}, vk::ShaderStageFlagBits::eCompute, shader_module->Module, globals::kGlslEntryFunctionName.data()};

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
        m_particle_draw_pass->DescriptorSetLayouts.emplace_back(m_device_context.Device.createDescriptorSetLayout(descriptor_layout));

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_particle_draw_pass->DescriptorSetLayouts);
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

        std::vector input_bindings = {vk::VertexInputBindingDescription{0, sizeof(DebrisParticle), vk::VertexInputRate::eVertex}};

        std::vector<vk::VertexInputAttributeDescription> input_attributes = {
                // Location 0 : Position
                {0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(DebrisParticle, Position)},
                // Location 1 : Velocity
                {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(DebrisParticle, AngularVelocity)},
                // Location 2 : Color
                {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(DebrisParticle, Color)},
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

    void Renderer::create_blur_passes() {
        // todo this code is messy because it was rushed, it needs a dire cleanup
        m_downsample_blur_pass = std::make_unique<RenderPass>(m_device_context.Device);
        m_upsample_blur_pass   = std::make_unique<RenderPass>(m_device_context.Device);

        static constexpr vk::ShaderStageFlags kVertFragStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

        std::vector sampler_set_layout_bindings{
                // Binding 0: sampler
                vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eSampler, 1, kVertFragStages},
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_layout_ci{{}, sampler_set_layout_bindings};
        m_downsample_blur_pass->DescriptorSetLayouts.emplace_back(m_device_context.Device.createDescriptorSetLayout(descriptor_layout_ci));

        std::vector source_image_set_layout_bindings{// Binding 0: source image
                                                     vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eSampledImage, kImageDescriptorCount, kVertFragStages}};

        // Reuse previous variables
        std::vector<vk::DescriptorBindingFlags> descriptor_binding_flags{};
        descriptor_binding_flags.emplace_back(vk::DescriptorBindingFlagBits::eVariableDescriptorCount);
        vk::DescriptorSetLayoutBindingFlagsCreateInfo set_layout_binding_flags{};
        set_layout_binding_flags.setBindingFlags(descriptor_binding_flags);

        descriptor_layout_ci.setBindings(source_image_set_layout_bindings);
        descriptor_layout_ci.pNext = &set_layout_binding_flags;

        m_downsample_blur_pass->DescriptorSetLayouts.emplace_back(m_device_context.Device.createDescriptorSetLayout(descriptor_layout_ci));

        // std::vector destination_image_layout_bindings{// Binding 0: destination image
        //                                               vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageImage, kImageDescriptorCount, vk::ShaderStageFlagBits::eCompute}};
        //
        // descriptor_layout_ci.setBindings(destination_image_layout_bindings);
        // m_downsample_blur_pass->DescriptorSetLayouts.emplace_back(m_device_context.Device.createDescriptorSetLayout(descriptor_layout_ci));


        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_downsample_blur_pass->DescriptorSetLayouts);

        vk::PushConstantRange push_constant_range{};
        push_constant_range.size       = sizeof(vk::DeviceAddress);
        push_constant_range.stageFlags = kVertFragStages;
        pipeline_layout_ci.setPushConstantRanges(push_constant_range);

        m_downsample_blur_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_downsample_blur_pass->PipelineLayout != nullptr);

        // Same thing, just copy
        m_upsample_blur_pass->DescriptorSetLayouts = m_downsample_blur_pass->DescriptorSetLayouts;
        m_upsample_blur_pass->PipelineLayout       = m_downsample_blur_pass->PipelineLayout;


        GlslShaderModule *down_vert_module = m_shader_registry->GetGlslModule("dual-kawase-blur-down.vert");
        GlslShaderModule *down_frag_module = m_shader_registry->GetGlslModule("dual-kawase-blur-down.frag");

        std::vector shader_stages{vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, down_vert_module->Module, globals::kGlslEntryFunctionName.data()},
                                  vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, down_frag_module->Module, globals::kGlslEntryFunctionName.data()}};

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{{}, vk::PrimitiveTopology::eTriangleList, false};
        vk::PipelineViewportStateCreateInfo      viewport_state{{}, 1, {}, 1, {}};

        vk::PipelineRasterizationStateCreateInfo rasterization_state{{}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise};
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

        vk::PipelineVertexInputStateCreateInfo vertex_input_ci{};
        vertex_input_ci.vertexAttributeDescriptionCount = 0;
        vertex_input_ci.pVertexAttributeDescriptions    = nullptr;
        vertex_input_ci.vertexBindingDescriptionCount   = 0;
        vertex_input_ci.pVertexBindingDescriptions      = nullptr;

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
        pipeline_ci.layout              = m_downsample_blur_pass->PipelineLayout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        pipeline_ci.pNext = &pipeline_rendering_create_info;

        const auto result = m_device_context.Device.createGraphicsPipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_downsample_blur_pass->Pipeline = result.value;
    }

    void Renderer::prepare_resources() {
        create_buffers();
        create_images();
        create_descriptor_pool();
    }

    void Renderer::prepare_starting_conditions() {
        std::mt19937 rng_engine{globals::kRNGSeed};

        // Define initial particle states using random number generators and distributions
        std::uniform_real_distribution<float> uniform_distribution(-1.0f, 1.0f);
        std::normal_distribution<float>       normal_distribution(0, 3.0f);

        std::stringstream log_string{};
        uint32_t          particle_idx = 0;

        static constexpr bool  kLogParticleStartingConditions = false;
        static constexpr float kSliceRadius                   = globals::kMaxOrbitRadius - globals::kMinOrbitRadius;

        std::uniform_real_distribution<float> orbit_distribution(globals::kMinOrbitRadius, globals::kMaxOrbitRadius);
        std::uniform_real_distribution<float> angle_distribution(0.0f, 360.0f);
        std::uniform_real_distribution<float> orbit_angular_velocity_distribution(globals::kMinAngularSpeed, globals::kMaxAngularSpeed); // angles per second

        // The particles will be distributed along a torus, emulating a debris field that could be orbiting a planet, for example.
        for (auto &particle: m_starting_particle_states) {
            // Define starting position in the X-Z plane. This can be described with the distance from the center of rotation, plus an angle (here named theta).
            particle.Radius     = orbit_distribution(rng_engine);
            particle.Theta      = glm::radians(angle_distribution(rng_engine));
            particle.Position.x = glm::cos(particle.Theta) * particle.Radius;
            particle.Position.z = glm::sin(particle.Theta) * particle.Radius;
            particle.Position.w = 1.0;

            // Define starting position in the Y axis. Given that X and Z are known, this restricts the value range in the Y axis to satisfy the condition that the Y coordinate remains inside a circle
            // (A circle is the torus' slice shape)
            // todo explain this math later...no time to explain for now
            const float radius_ratio = (particle.Radius - globals::kMinOrbitRadius) / kSliceRadius; // calc range [0,1] relative to min and max radius
            const float cos_phi      = (radius_ratio * 2.0f) - 1.0f;                                // change range to [-1,1]
            const float phi          = glm::acos(cos_phi);

            // The height can now range from 0 to sin(phi)
            const float                           height = std::abs(glm::sin(phi)) * kSliceRadius;
            std::uniform_real_distribution<float> height_distribution(-height, height);
            particle.Position.y = height_distribution(rng_engine);

            particle.AngularVelocity = glm::radians(orbit_angular_velocity_distribution(rng_engine));

            // Color by starting theta
            const float revolution_percentage = particle.Theta / glm::two_pi<float>();
            particle.Color                    = glm::vec4(revolution_percentage, 1.0, 1.0 - revolution_percentage, 1.0);

            if constexpr (kLogParticleStartingConditions)
                log_string << "Particle " << particle_idx << ":\n"
                           << "Position: " << glm::to_string(particle.Position) << " | AngularVelocity :" << std::to_string(particle.AngularVelocity) << " | Color: " << glm::to_string(particle.Color)
                           << "\n";

            ++particle_idx;
        }

        if constexpr (kLogParticleStartingConditions)
            std::cout << log_string.str() << std::endl;

        // world state
        m_camera->SetTranslation({3, 1, -3});
        m_camera->SetLookAt({0, 0, 0});
    }

    void Renderer::create_descriptor_pool() {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {// Last frame particle buffer + current frame particle buffer
                                                          {vk::DescriptorType::eStorageBuffer, 2 * globals::kFramesInFlight},
                                                          // World data uniform buffer + Camera uniform buffer
                                                          {vk::DescriptorType::eUniformBuffer, 2 * globals::kFramesInFlight},
                                                          // Source image for post processing
                                                          {vk::DescriptorType::eSampledImage, kImageDescriptorCount * globals::kFramesInFlight},
                                                          // Destination image for post processing
                                                          {vk::DescriptorType::eStorageImage, kImageDescriptorCount * globals::kFramesInFlight},
                                                          // Samplers
                                                          {vk::DescriptorType::eSampler, globals::kFramesInFlight}};

        vk::DescriptorPoolCreateInfo ci{};
        ci.setPoolSizes(pool_sizes);
        ci.maxSets = static_cast<uint32_t>(pool_sizes.size()) * globals::kFramesInFlight;

        m_descriptor_pool = m_device_context.Device.createDescriptorPool(ci);
        check(m_descriptor_pool != nullptr);
    }

    void Renderer::create_buffers() {
        static constexpr uint64_t kBufferSize = sizeof(DebrisParticle) * globals::kParticleCount;

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

        // Create blur data buffers
        for (auto &buffer: m_blur_data_buffers) {
            buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(BlurData));
        }
    }

    void Renderer::create_images() {
        for (auto &attachment_ptr: m_main_color_attachments) {
            attachment_ptr = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color);
        }

        static constexpr uint8_t kLowestBlurLevel = kBlurStepAttachmentCount / 2;
        for (int i = 0; i < kLowestBlurLevel; ++i) {
            const uint8_t      factor                                 = 2 << i;
            const vk::Extent3D extent                                 = vk::Extent3D{globals::kRenderExtent3D.width / factor, globals::kRenderExtent3D.height / factor, globals::kRenderExtent3D.depth};
            m_blur_step_attachments[i]                                = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color, extent);
            m_blur_step_attachments[kBlurStepAttachmentCount - 1 - i] = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color, extent);
        }

        assert(m_blur_step_attachments[kLowestBlurLevel] == nullptr);
        const vk::Extent3D lowest_extent =
                vk::Extent3D{globals::kRenderExtent3D.width / (2 << kLowestBlurLevel), globals::kRenderExtent3D.height / (2 << kLowestBlurLevel), globals::kRenderExtent3D.depth};
        m_blur_step_attachments[kLowestBlurLevel] = std::make_unique<RenderAttachment>(m_device_context, RenderAttachmentType::Color, lowest_extent);

        // Create sampler
        vk::SamplerCreateInfo sampler_ci{};
        sampler_ci.magFilter     = vk::Filter::eLinear;
        sampler_ci.minFilter     = vk::Filter::eLinear;
        sampler_ci.mipmapMode    = vk::SamplerMipmapMode::eLinear;
        sampler_ci.addressModeU  = vk::SamplerAddressMode::eClampToBorder; // todo review this
        sampler_ci.addressModeV  = sampler_ci.addressModeU;
        sampler_ci.addressModeW  = sampler_ci.addressModeU;
        sampler_ci.mipLodBias    = 0.0f;
        sampler_ci.maxAnisotropy = 1.0f;
        sampler_ci.compareOp     = vk::CompareOp::eNever;
        sampler_ci.minLod        = 0.0f;
        sampler_ci.maxLod        = 1.0f;
        sampler_ci.borderColor   = vk::BorderColor::eFloatOpaqueBlack;

        m_sampler = m_device_context.Device.createSampler(sampler_ci);
        check(m_sampler != nullptr);

        m_sampler_descriptor.sampler = m_sampler;
    }

    void Renderer::create_descriptor_sets() {
        create_particle_compute_descriptor_sets();
        create_particle_draw_descriptor_sets();
        create_blur_descriptor_sets();
    }

    void Renderer::create_particle_compute_descriptor_sets() {
        m_compute_pass->DescriptorSets.resize(1);

        for (size_t frame_index = 0; frame_index < m_compute_pass->DescriptorSets[0].size(); ++frame_index) {
            vk::DescriptorSet &descriptor_set = m_compute_pass->DescriptorSets[0][frame_index];

            const uint32_t prev_frame = (frame_index - 1 + globals::kFramesInFlight) % globals::kFramesInFlight;

            vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_compute_pass->DescriptorSetLayouts[0]}};
            check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));

            std::vector write_descriptor_sets = {// Binding 0: previous frame particle buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, m_particle_storage_buffers[prev_frame]->Descriptor},
                                                 // Binding 1: current frame particle buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eStorageBuffer, nullptr, m_particle_storage_buffers[frame_index]->Descriptor}};

            m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        }
    }

    void Renderer::create_particle_draw_descriptor_sets() {
        m_particle_draw_pass->DescriptorSets.resize(1);

        for (size_t frame_index = 0; frame_index < m_particle_draw_pass->DescriptorSets[0].size(); ++frame_index) {
            vk::DescriptorSet &descriptor_set = m_particle_draw_pass->DescriptorSets[0][frame_index];

            vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_particle_draw_pass->DescriptorSetLayouts[0]}};
            check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));

            std::vector write_descriptor_sets = {// Binding 0: camera uniform buffer
                                                 vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, nullptr, m_camera->UniformBuffers[frame_index]->Descriptor}};

            m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        }
    }

    void Renderer::create_blur_descriptor_sets() {
        // TODO use bindless
        // m_downsample_blur_pass->DescriptorSets.resize(1);
        //
        // for (auto &descriptor_set: m_downsample_blur_pass->DescriptorSets[0]) {
        //     vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_downsample_blur_pass->DescriptorSetLayout}};
        //     check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));
        //
        //     // Copy the descriptor and apply the correct image layout to expect
        //     // vk::DescriptorImageInfo expected_descriptor = m_post_process_effect_attachment->Descriptor; // TODO
        //     expected_descriptor.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;
        //
        //     std::vector write_descriptor_sets = {vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eSampledImage, vk::ImageLayout::eShaderReadOnlyOptimal},
        //                                          vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eSampler, expected_descriptor},
        //                                          vk::WriteDescriptorSet{descriptor_set, 2, 0, vk::DescriptorType::eStorageImage, expected_descriptor}};
        //
        //     m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        // }
        //
        // m_upsample_blur_pass->DescriptorSets.resize(1);
        //
        // for (auto &descriptor_set: m_upsample_blur_pass->DescriptorSets[0]) {
        //     vk::DescriptorSetAllocateInfo alloc_info{m_descriptor_pool, {m_upsample_blur_pass->DescriptorSetLayout}};
        //     check(m_device_context.Device.allocateDescriptorSets(&alloc_info, &descriptor_set));
        //
        //     // Copy the descriptor and apply the correct image layout to expect
        //     // vk::DescriptorImageInfo expected_descriptor = m_post_process_effect_attachment->Descriptor; // TODO
        //     expected_descriptor.imageLayout             = vk::ImageLayout::eShaderReadOnlyOptimal;
        //
        //     std::vector write_descriptor_sets = {vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eSampledImage, expected_descriptor},
        //                                          vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eSampler, expected_descriptor},
        //                                          vk::WriteDescriptorSet{descriptor_set, 2, 0, vk::DescriptorType::eStorageImage, expected_descriptor}};
        //
        //     m_device_context.Device.updateDescriptorSets(write_descriptor_sets, {});
        // }

        std::vector<uint32_t> variable_desciptor_counts{kImageDescriptorCount, kImageDescriptorCount};

        vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_ai{};
        variable_descriptor_count_ai.setDescriptorCounts(variable_desciptor_counts);

        uint32_t                        bindless_index = 0;
        std::vector<RenderAttachment *> blur_images{};
        for (auto &attachment: m_main_color_attachments) {
            blur_images.emplace_back(attachment.get());

            m_attachment_to_bindless_index_map.emplace(attachment.get(), bindless_index++);
        }
        for (auto &attachment: m_blur_step_attachments) {
            blur_images.emplace_back(attachment.get());
            m_attachment_to_bindless_index_map.emplace(attachment.get(), bindless_index++);
        }

        // Sampler
        vk::WriteDescriptorSet sampler_descriptor_write{};
        sampler_descriptor_write.dstBinding      = 0;
        sampler_descriptor_write.dstArrayElement = 0;
        sampler_descriptor_write.descriptorCount = 1;
        sampler_descriptor_write.descriptorType  = vk::DescriptorType::eSampler;
        sampler_descriptor_write.pImageInfo      = &m_sampler_descriptor;

        // Source images
        std::vector<vk::DescriptorImageInfo> source_image_descriptors(blur_images.size());
        for (size_t i = 0; i < source_image_descriptors.size(); ++i) {
            source_image_descriptors[i].sampler     = m_sampler;
            source_image_descriptors[i].imageView   = blur_images[i]->Descriptor.imageView;
            source_image_descriptors[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
        vk::WriteDescriptorSet source_image_array_descriptor_write{};
        source_image_array_descriptor_write.dstBinding      = 0;
        source_image_array_descriptor_write.dstArrayElement = 0;
        source_image_array_descriptor_write.descriptorCount = kImageDescriptorCount;
        source_image_array_descriptor_write.descriptorType  = vk::DescriptorType::eSampledImage;
        source_image_array_descriptor_write.pImageInfo      = source_image_descriptors.data();

        // // Destination images
        // std::vector<vk::DescriptorImageInfo> destination_image_descriptors(blur_images.size());
        // for (size_t i = 0; i < destination_image_descriptors.size(); ++i) {
        //     destination_image_descriptors[i].sampler     = m_sampler;
        //     destination_image_descriptors[i].imageView   = blur_images[i]->Descriptor.imageView;
        //     destination_image_descriptors[i].imageLayout = vk::ImageLayout::eGeneral;
        // }
        //
        // vk::WriteDescriptorSet destination_image_array_descriptor_write{};
        // destination_image_array_descriptor_write.dstBinding      = 0;
        // destination_image_array_descriptor_write.dstArrayElement = 0;
        // destination_image_array_descriptor_write.descriptorCount = kImageDescriptorCount;
        // destination_image_array_descriptor_write.descriptorType  = vk::DescriptorType::eStorageImage;
        // destination_image_array_descriptor_write.pImageInfo      = destination_image_descriptors.data();


        // Allocate descriptor sets
        m_downsample_blur_pass->DescriptorSets.resize(2);
        m_upsample_blur_pass->DescriptorSets.resize(2);

        vk::DescriptorSetAllocateInfo        alloc_info{m_descriptor_pool};
        std::vector<vk::DescriptorSetLayout> set_layouts;

        // Sampler set layout
        set_layouts = {m_downsample_blur_pass->DescriptorSetLayouts[0], m_downsample_blur_pass->DescriptorSetLayouts[0]};
        alloc_info.setSetLayouts(set_layouts);
        check(m_device_context.Device.allocateDescriptorSets(&alloc_info, m_downsample_blur_pass->DescriptorSets[0].data()));

        // Source image set layout
        alloc_info.pNext = &variable_descriptor_count_ai;

        set_layouts = {m_downsample_blur_pass->DescriptorSetLayouts[1], m_downsample_blur_pass->DescriptorSetLayouts[1]};
        alloc_info.setSetLayouts(set_layouts);
        check(m_device_context.Device.allocateDescriptorSets(&alloc_info, m_downsample_blur_pass->DescriptorSets[1].data()));

        // Destination image set layout
        // set_layouts = {m_downsample_blur_pass->DescriptorSetLayouts[2], m_downsample_blur_pass->DescriptorSetLayouts[2]};
        // alloc_info.setSetLayouts(set_layouts);
        // check(m_device_context.Device.allocateDescriptorSets(&alloc_info, m_downsample_blur_pass->DescriptorSets[2].data()));

        // Update descriptor sets
        for (int frame = 0; frame < globals::kFramesInFlight; ++frame) {
            auto &sampler_set   = m_downsample_blur_pass->DescriptorSets[0][frame];
            auto &src_image_set = m_downsample_blur_pass->DescriptorSets[1][frame];
            // auto &dst_image_set = m_downsample_blur_pass->DescriptorSets[2][frame];

            sampler_descriptor_write.dstSet            = sampler_set;
            source_image_array_descriptor_write.dstSet = src_image_set;
            // destination_image_array_descriptor_write.dstSet = dst_image_set;

            m_device_context.Device.updateDescriptorSets(sampler_descriptor_write, {});
            m_device_context.Device.updateDescriptorSets(source_image_array_descriptor_write, {});
            // m_device_context.Device.updateDescriptorSets(destination_image_array_descriptor_write, {});
        }

        // Same layout, same descriptor sets, just copy cheekily
        m_upsample_blur_pass->DescriptorSets = m_downsample_blur_pass->DescriptorSets;
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
        cb.pushConstants(m_compute_pass->PipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(vk::DeviceAddress), &m_world_uniform_buffers[m_frame_context->Index]->DeviceAddress);
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

        auto &color_attachment = *m_main_color_attachments[m_frame_context->Index];
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

    void Renderer::blur_pass() {
        const auto &cb = m_frame_context->GraphicsCB;

        // todo calc all this based on pass index
        auto &src_image                    = m_main_color_attachments[m_frame_context->Index];
        auto &dst_image                    = m_blur_step_attachments[0];

        vk::Extent2D dst_image_extent{dst_image->Extent.width, dst_image->Extent.height};

        m_blur_data.DestinationImageSize.x = static_cast<float>(dst_image_extent.width);
        m_blur_data.DestinationImageSize.y = static_cast<float>(dst_image_extent.height);
        m_blur_data.SourceImageIndex       = m_attachment_to_bindless_index_map[m_main_color_attachments[m_frame_context->Index].get()];
        m_blur_data.DestinationImageIndex  = m_attachment_to_bindless_index_map[m_blur_step_attachments[0].get()];

        src_image->LayoutTransition(cb, vk::ImageLayout::eShaderReadOnlyOptimal);


        vk::Viewport viewport{{}, {}, static_cast<float>(dst_image_extent.width), static_cast<float>(dst_image_extent.height), 0, 1};
        vk::Rect2D   scissor{{}, dst_image_extent};
        cb.setViewport(0, {viewport});
        cb.setScissor(0, {scissor});

        auto &color_attachment = *dst_image;
        color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);

        vk::RenderingAttachmentInfo color_attachment_info{};
        color_attachment_info.imageView        = color_attachment.Descriptor.imageView;
        color_attachment_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment_info.loadOp           = vk::AttachmentLoadOp::eClear;
        color_attachment_info.clearValue.color = vk::ClearColorValue{0, 0, 0, 0};
        color_attachment_info.storeOp          = vk::AttachmentStoreOp::eStore;

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea.extent = dst_image_extent;
        rendering_info.layerCount        = 1;
        rendering_info.setColorAttachments({color_attachment_info});

        cb.beginRendering(rendering_info);

        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_downsample_blur_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_downsample_blur_pass->PipelineLayout, 0, {m_downsample_blur_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_downsample_blur_pass->PipelineLayout, 1, {m_downsample_blur_pass->DescriptorSets[1][m_frame_context->Index]}, {});
        // cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_downsample_blur_pass->PipelineLayout, 2, {m_downsample_blur_pass->DescriptorSets[2][m_frame_context->Index]}, {});

        memcpy(m_blur_data_buffers[m_frame_context->Index]->AllocationInfo.pMappedData, &m_blur_data, sizeof(BlurData));
        cb.pushConstants(m_downsample_blur_pass->PipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment , 0, sizeof(vk::DeviceAddress), &m_blur_data_buffers[m_frame_context->Index]->DeviceAddress);
        cb.draw(3, 1, 0, 0);
        cb.endRendering();
    }

} // namespace vkdemo
