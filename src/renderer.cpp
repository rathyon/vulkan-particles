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
            FrameContext frame_context = m_context->BeginFrame();

            const auto &cb = frame_context.GraphicsCommandBuffer;

            vk::Viewport viewport{ {}, {}, globals::kRenderExtent2D.width, globals::kRenderExtent3D.height, 0, 1};
            vk::Rect2D scissor{{}, globals::kRenderExtent2D};
            cb.setViewport(0, {viewport});
            cb.setScissor(0, {scissor});

            // todo encapsulate this more neatly into a RenderPass?
            auto &main_color_attachment = m_context->MainColorAttachments[frame_context.Index];
            main_color_attachment.LayoutTransition(cb, vk::ImageLayout::eColorAttachmentOptimal);
            vk::RenderingAttachmentInfo color_attachment_info{};
            color_attachment_info.imageView               = main_color_attachment.View;
            color_attachment_info.imageLayout             = vk::ImageLayout::eColorAttachmentOptimal;
            color_attachment_info.loadOp                  = vk::AttachmentLoadOp::eClear;
            color_attachment_info.clearValue.color        = vk::ClearColorValue{0, 0, 0, 0};
            color_attachment_info.storeOp                 = vk::AttachmentStoreOp::eStore;

            vk::RenderingInfo rendering_info{};
            rendering_info.renderArea.extent = globals::kRenderExtent2D;
            rendering_info.layerCount = 1;
            rendering_info.setColorAttachments({color_attachment_info});

            cb.beginRendering(rendering_info);

            cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_fullscreen_pipeline.Handle);
            cb.draw(3, 1, 0, 0);

            cb.endRendering();

            m_context->EndFrame();
        }
    }

    void Renderer::create_pipelines() { create_fullscreen_pipeline(); }

    void Renderer::create_fullscreen_pipeline() {
        vk::PipelineLayoutCreateInfo pipeline_layout_ci{};
        pipeline_layout_ci.setLayoutCount = 0;
        pipeline_layout_ci.pSetLayouts    = nullptr;

        m_fullscreen_pipeline.Layout = m_device_context.Device.createPipelineLayout(pipeline_layout_ci);
        check(m_fullscreen_pipeline.Layout != nullptr);

        ShaderModule *fullscreen_module = m_shader_registry->GetModule("fullscreen.slang");

        std::vector shader_stages{vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, fullscreen_module->Module, "vs_main"},
                                  vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, fullscreen_module->Module, "fs_main"}};

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

        // .DepthTestEnable = false,

        vk::PipelineRasterizationStateCreateInfo rasterization_state{};
        rasterization_state.depthClampEnable        = vk::False;
        rasterization_state.rasterizerDiscardEnable = vk::False;
        rasterization_state.polygonMode             = vk::PolygonMode::eFill;
        rasterization_state.lineWidth               = 1.0f;
        rasterization_state.cullMode                = vk::CullModeFlagBits::eFront;
        rasterization_state.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterization_state.depthBiasEnable         = vk::False;

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

        vk::GraphicsPipelineCreateInfo graphics_pipeline_ci{};
        graphics_pipeline_ci.setStages(shader_stages);
        graphics_pipeline_ci.pVertexInputState   = &vertex_input_ci;
        graphics_pipeline_ci.pInputAssemblyState = &input_assembly_state;
        graphics_pipeline_ci.pViewportState      = &viewport_state;
        graphics_pipeline_ci.pRasterizationState = &rasterization_state;
        graphics_pipeline_ci.pMultisampleState   = &multisample_state;
        graphics_pipeline_ci.pDepthStencilState  = &depth_stencil_state;
        graphics_pipeline_ci.pColorBlendState    = &color_blend_state;
        graphics_pipeline_ci.pDynamicState       = &dynamic_state;
        graphics_pipeline_ci.layout              = m_fullscreen_pipeline.Layout;

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{{}, globals::kColorAttachmentFormat, globals::kDepthStencilAttachmentFormat};
        graphics_pipeline_ci.pNext = &pipeline_rendering_create_info;

        auto result = m_device_context.Device.createGraphicsPipeline(nullptr, graphics_pipeline_ci);
        check(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

        m_fullscreen_pipeline.Handle = result.value;
    }
} // namespace vkdemo
