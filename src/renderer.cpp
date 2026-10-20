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

        prepare_resources();
        create_shader_passes();
    }

    void Renderer::RenderLoop() {
        uint64_t last_time{SDL_GetTicks()};
        bool     stop = false;
        while (!stop) {
            m_frame_context = m_context->BeginFrame();

            m_context->BeginComputeCommands();
            // simple_compute_pass();
            particle_compute_pass();
            m_context->EndComputeCommands();

            m_context->BeginGraphicsCommands();
            // scene_render_pass();
            fullscreen_render_pass();
            m_context->EndGraphicsCommands();

            m_context->EndFrame();

            float elapsed_time{static_cast<float>(SDL_GetTicks() - last_time) / 1000.0f};
            last_time = SDL_GetTicks();
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
        create_graphics_passes();
        create_compute_passes();
        update_descriptor_sets();
    }

    void Renderer::create_graphics_passes() {
        m_graphics_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector set_layout_bindings{// Binding 0: input image
                                        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment},
                                        // Binding 1: sampler
                                        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_graphics_pass->DescriptorSetLayout = m_device_context.Device.createDescriptorSetLayout(descriptor_layout);
        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_graphics_pass->DescriptorSetLayout);
        m_graphics_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_graphics_pass->PipelineLayout != nullptr);

        // The fullscreen shader uses a technique described by Sascha Willems:
        // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
        ShaderModule *fullscreen_module = m_shader_registry->GetModule("fullscreen.slang");

        std::vector shader_stages{vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, fullscreen_module->Module, globals::kVertexEntryFunctionName.data()},
                                  vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, fullscreen_module->Module, globals::kFragmentEntryFunctionName.data()}};

        // Fullscreen shader is "geometry-less"
        vk::PipelineVertexInputStateCreateInfo vertex_input_ci{};
        vertex_input_ci.vertexBindingDescriptionCount   = 0;
        vertex_input_ci.pVertexBindingDescriptions      = nullptr;
        vertex_input_ci.vertexAttributeDescriptionCount = 0;
        vertex_input_ci.pVertexAttributeDescriptions    = nullptr;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{};
        input_assembly_state.topology               = vk::PrimitiveTopology::eTriangleList;
        input_assembly_state.primitiveRestartEnable = vk::False;

        vk::PipelineViewportStateCreateInfo viewport_state{};
        viewport_state.pViewports    = nullptr;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount  = 1;

        vk::PipelineRasterizationStateCreateInfo rasterization_state{};
        rasterization_state.depthClampEnable        = vk::False;
        rasterization_state.rasterizerDiscardEnable = vk::False;
        rasterization_state.polygonMode             = vk::PolygonMode::eFill;
        rasterization_state.lineWidth               = 1.0f;
        rasterization_state.depthBiasEnable         = vk::False;

        // Culling and FrontFace are setup specifically for the geometry-less "fake fullscreen quad" technique
        rasterization_state.cullMode  = vk::CullModeFlagBits::eFront;
        rasterization_state.frontFace = vk::FrontFace::eCounterClockwise;

        vk::PipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::PipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable         = true;
        color_blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.colorBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
        color_blend_attachment_state.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.logicOpEnable   = vk::False;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments    = &color_blend_attachment_state;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.depthTestEnable       = false;
        depth_stencil_state.depthWriteEnable      = false;
        depth_stencil_state.depthCompareOp        = vk::CompareOp::eLess;
        depth_stencil_state.depthBoundsTestEnable = vk::False;
        depth_stencil_state.stencilTestEnable     = vk::False;

        std::vector<vk::DynamicState>      dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamic_state(vk::PipelineDynamicStateCreateFlags(), dynamic_states);

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
        pipeline_ci.layout              = m_graphics_pass->PipelineLayout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        pipeline_ci.pNext = &pipeline_rendering_create_info;

        const auto result = m_device_context.Device.createGraphicsPipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_graphics_pass->Pipeline = result.value;
    }

    void Renderer::create_compute_passes() {
        m_compute_pass = std::make_unique<RenderPass>(m_device_context.Device);

        std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings{// Binding 0: input image
                                                                        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute}};

        vk::DescriptorSetLayoutCreateInfo descriptor_layout{{}, set_layout_bindings};
        m_compute_pass->DescriptorSetLayout = m_device_context.Device.createDescriptorSetLayout(descriptor_layout);

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setSetLayouts(m_compute_pass->DescriptorSetLayout);

        m_compute_pass->PipelineLayout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_compute_pass->PipelineLayout != nullptr);

        ShaderModule                     *simple_compute_module = m_shader_registry->GetModule("simple-compute.slang");
        vk::PipelineShaderStageCreateInfo shader_stage_ci{{}, vk::ShaderStageFlagBits::eCompute, simple_compute_module->Module, globals::kComputeEntryFunctionName.data()};

        vk::ComputePipelineCreateInfo pipeline_ci{};
        pipeline_ci.layout = m_compute_pass->PipelineLayout;
        pipeline_ci.stage  = shader_stage_ci;

        const auto result = m_device_context.Device.createComputePipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create compute pipeline!");
        m_compute_pass->Pipeline = result.value;
    }

    void Renderer::prepare_resources() {
        create_descriptor_pool();
        create_buffers();
        create_images();
    }

    void Renderer::create_descriptor_pool() {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {// Input image in fullscreen pass
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, globals::kFramesInFlight},
                                                          // Sampler in fullscreen pass
                                                          vk::DescriptorPoolSize{vk::DescriptorType::eSampler, globals::kFramesInFlight},
                                                          // Image write in compute pass
                                                          vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, globals::kFramesInFlight)};

        vk::DescriptorPoolCreateInfo ci{};
        ci.setPoolSizes(pool_sizes);
        ci.maxSets = static_cast<uint32_t>(pool_sizes.size()) * globals::kFramesInFlight;

        m_descriptor_pool = m_device_context.Device.createDescriptorPool(ci);
        check(m_descriptor_pool != nullptr);
    }

    void Renderer::create_buffers() {
        // todo define proper section for this
        std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

        // Define initial particle states
        std::vector<Particle> initial_particle_states{globals::kParticleCount};
        // for (auto& particle : initial_particle_states) {
        //     particle.Position = glm::vec4{0,0,0,0};
        //     particle.Velocity = glm::vec4{0,0,0,0};
        //     particle.Color = glm::vec4{1,1,1,1};
        // }

        // upload to staging buffer and then copy from staging onto device buffer
        static constexpr uint64_t kBufferSize    = sizeof(Particle) * globals::kParticleCount;
        auto                      staging_buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eTransferSrc, true, kBufferSize);
        memcpy(staging_buffer->AllocationInfo.pMappedData, initial_particle_states.data(), kBufferSize);

        for (auto &buffer: m_compute_storage_buffers) {
            buffer = std::make_unique<Buffer>(m_device_context, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, false,
                                              kBufferSize);

            const vk::CommandBuffer upload_cb = m_context->CreateCommandBuffer(true, Transfer);

            vk::BufferCopy copy_region{0, 0, buffer->Size};
            upload_cb.copyBuffer(staging_buffer->Descriptor.buffer, buffer->Descriptor.buffer, {copy_region});

            // Perform queue ownership transfer barrier from transfer to compute (if queues differ)
            if (m_device_context.QueueFamilies[Transfer] != m_device_context.QueueFamilies[Compute]) {
                vk::BufferMemoryBarrier barrier{};
                barrier.srcAccessMask       = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask       = vk::AccessFlagBits::eNone;
                barrier.srcQueueFamilyIndex = m_device_context.QueueFamilies[Transfer];
                barrier.dstQueueFamilyIndex = m_device_context.QueueFamilies[Compute];
                barrier.buffer              = buffer->Descriptor.buffer;
                barrier.offset              = 0;
                barrier.size                = buffer->Size;

                upload_cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {barrier}, {});
            }

            m_context->FlushCommandBuffer(upload_cb, Transfer);
        }
    }

    void Renderer::create_images() {
        // This compute storage image will be drawn unto by a compute shader. Afterwards it will be read from in a fragment shader.
        m_post_process_effect_texture = std::make_unique<Texture>(m_device_context, TextureType::RGBA8_2D);
        m_post_process_effect_texture->HostLayoutTransition(vk::ImageLayout::eGeneral);
    }

    void Renderer::update_descriptor_sets() {
        // descriptor sets for graphics
        m_graphics_pass->DescriptorSets.resize(1);

        for (auto &descriptor_set: m_graphics_pass->DescriptorSets[0]) {
            vk::DescriptorSetAllocateInfo gfx_alloc_info{m_descriptor_pool, {m_graphics_pass->DescriptorSetLayout}};
            check(m_device_context.Device.allocateDescriptorSets(&gfx_alloc_info, &descriptor_set));

            std::vector gfx_write_descriptor_sets = {vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eSampledImage, m_post_process_effect_texture->Descriptor},
                                                     vk::WriteDescriptorSet{descriptor_set, 1, 0, vk::DescriptorType::eSampler, m_post_process_effect_texture->Descriptor}};

            m_device_context.Device.updateDescriptorSets(gfx_write_descriptor_sets, {});
        }

        // update descriptor sets for compute
        m_compute_pass->DescriptorSets.resize(1);

        for (auto &descriptor_set: m_compute_pass->DescriptorSets[0]) {
            vk::DescriptorSetAllocateInfo comp_alloc_info{m_descriptor_pool, {m_compute_pass->DescriptorSetLayout}};
            check(m_device_context.Device.allocateDescriptorSets(&comp_alloc_info, &descriptor_set));

            std::vector comp_write_descriptor_sets = {vk::WriteDescriptorSet{descriptor_set, 0, 0, vk::DescriptorType::eStorageImage, m_post_process_effect_texture->Descriptor}};

            m_device_context.Device.updateDescriptorSets(comp_write_descriptor_sets, {});
        }
    }

    void Renderer::fullscreen_render_pass() {
        const auto &cb = m_frame_context->GraphicsCB;

        vk::Viewport viewport{{}, {}, globals::kRenderExtent2D.width, globals::kRenderExtent3D.height, 0, 1};
        vk::Rect2D   scissor{{}, globals::kRenderExtent2D};
        cb.setViewport(0, {viewport});
        cb.setScissor(0, {scissor});

        auto &main_color_attachment = *m_context->MainColorAttachments[m_frame_context->Index];
        main_color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);
        vk::RenderingAttachmentInfo color_attachment_info{};
        color_attachment_info.imageView        = main_color_attachment.View;
        color_attachment_info.imageLayout      = vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment_info.loadOp           = vk::AttachmentLoadOp::eClear;
        color_attachment_info.clearValue.color = vk::ClearColorValue{0, 0, 0, 0};
        color_attachment_info.storeOp          = vk::AttachmentStoreOp::eStore;

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea.extent = globals::kRenderExtent2D;
        rendering_info.layerCount        = 1;
        rendering_info.setColorAttachments({color_attachment_info});

        cb.beginRendering(rendering_info);

        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphics_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pass->PipelineLayout, 0, {m_graphics_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.draw(3, 1, 0, 0);
        cb.endRendering();
    }

    void Renderer::simple_compute_pass() {
        const auto &cb = m_frame_context->ComputeCB;

        m_post_process_effect_texture->LayoutTransition(cb, vk::ImageLayout::eGeneral);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, m_compute_pass->Pipeline);
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_compute_pass->PipelineLayout, 0, {m_compute_pass->DescriptorSets[0][m_frame_context->Index]}, {});
        cb.dispatch(globals::kRenderExtent2D.width / 16, globals::kRenderExtent2D.height / 16, 1);
    }

    void Renderer::particle_compute_pass() {}

} // namespace vkdemo
