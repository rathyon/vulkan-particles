#include "renderer.hpp"

namespace vkdemo {

    Renderer::~Renderer() {
        m_device_context.Device.destroyPipelineLayout(m_fullscreen_pipeline.Layout);
        m_device_context.Device.destroyPipeline(m_fullscreen_pipeline.Handle);
    }

    void Renderer::Initialize(const Window &window) {
        m_device_context.Initialize();

        m_context         = std::make_unique<RendererContext>(m_device_context);
        m_shader_registry = std::make_unique<ShaderRegistry>(m_device_context);

        m_context->Initialize(window);
        m_shader_registry->Initialize();
        m_shader_registry->LoadShaders();

        create_pipelines();
    }

    void Renderer::RenderLoop() {
        bool stop = false;

        while (!stop) {
            m_frame_context = m_context->BeginFrame();

            m_context->BeginComputeCommands();
            // simple_compute_pass();
            m_context->EndComputeCommands();

            m_context->BeginGraphicsCommands();
            // fullscreen_render_pass();
            m_context->EndGraphicsCommands();

            m_context->EndFrame();
        }
    }

    void Renderer::create_pipelines() {
        create_fullscreen_pipeline();
        create_compute_pipeline();
    }

    void Renderer::create_fullscreen_pipeline() {
        // This fullscreen shader uses a technique described by Sascha Willems:
        // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/

        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setLayoutCount = 0;
        pipeline_layout_ci.pSetLayouts    = nullptr;

        m_fullscreen_pipeline.Layout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_fullscreen_pipeline.Layout != nullptr);

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
        pipeline_ci.layout              = m_fullscreen_pipeline.Layout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        pipeline_ci.pNext = &pipeline_rendering_create_info;

        const auto result = m_device_context.Device.createGraphicsPipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_fullscreen_pipeline.Handle = result.value;
    }

    void Renderer::create_compute_pipeline() {
        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setLayoutCount = 0;
        pipeline_layout_ci.pSetLayouts    = nullptr;

        m_simple_compute_pipeline.Layout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_simple_compute_pipeline.Layout != nullptr);

        ShaderModule *simple_compute_module = m_shader_registry->GetModule("simple-compute.slang");
        vk::PipelineShaderStageCreateInfo shader_stage_ci{{}, vk::ShaderStageFlagBits::eCompute, simple_compute_module->Module, globals::kComputeEntryFunctionName.data()};

        vk::ComputePipelineCreateInfo pipeline_ci{};
        pipeline_ci.layout = m_simple_compute_pipeline.Layout;
        pipeline_ci.stage = shader_stage_ci;

        const auto result = m_device_context.Device.createComputePipeline(nullptr, pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create compute pipeline!");
        m_simple_compute_pipeline.Handle = result.value;
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

        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_fullscreen_pipeline.Handle);
        cb.draw(3, 1, 0, 0);

        cb.endRendering();
    }

    void Renderer::simple_compute_pass() {
        const auto& cb = m_frame_context->ComputeCB;

        cb.bindPipeline(vk::PipelineBindPoint::eCompute, m_simple_compute_pipeline.Handle);

        // cb.dispatch();
    }

} // namespace vkdemo
