// -----------------------------------------------------------------
// OutliningPass
// 选中物体轮廓（stencil 剪影 + 粗线框）
// -----------------------------------------------------------------

#include "function/render/render_framework/pass/outlining_pass.hpp"
#include "engine/global_context.hpp"

namespace wen {

void OutliningPass::addAttachment(Renderer::RenderPass& render_pass) {
    // Dedicated stencil attachment (the main "depth" attachment is D32 with
    // no stencil bits, and is sampled by the HZB pass - keep it untouched).
    render_pass.addAttachment("outlining_stencil", Renderer::AttachmentType::eStencil);
}

void OutliningPass::setAttachment(Renderer::RenderSubpass& render_subpass) {
    render_subpass.setDepthAttachment("outlining_stencil");
    render_subpass.setOutputAttachment(global_context->render_system->output_attachment_name);
}

void OutliningPass::setSubpassDependency(Renderer::RenderPass& render_pass) {
    // Draw on top of the shaded scene.
    render_pass.addSubpassDependency(
        "mesh_pass",
        "outlining_pass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );
    // The stencil attachment's loadOp clear must wait for last frame's use.
    render_pass.addSubpassDependency(
        Renderer::EXTERNAL_SUBPASS,
        "outlining_pass",
        {
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests
        },
        {
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );
}

void OutliningPass::createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto interface = global_context->render_system->getInterface();

    descriptor_set_ = interface->createDescriptorSet();
    descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eVertex}
    });
    descriptor_set_->build();
    descriptor_set_->bindUniform(0, global_context->camera_system->getViewportCamera());

    auto vert_shader = interface->loadShader(getName() + "/shader.vert", Renderer::ShaderStage::eVertex);
    auto vertex_input = interface->createVertexInput({
        {0, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat3}},
        {1, Renderer::InputRate::eInstance, {Renderer::VertexType::eFloat4, Renderer::VertexType::eFloat4, Renderer::VertexType::eFloat4}}
    });

    // 1. Fill the stencil with the selected instance's silhouette.
    stencil_shader_program_ = interface->createGraphicsShaderProgram();
    stencil_shader_program_->attach(vert_shader);
    stencil_shader_program_->attach(interface->loadShader(getName() + "/stencil.frag", Renderer::ShaderStage::eFragment));
    stencil_render_pipeline_ = interface->createGraphicsRenderPipeline(renderer, stencil_shader_program_, getName());
    stencil_render_pipeline_->setVertexInput(vertex_input);
    stencil_render_pipeline_->setDescriptorSet(descriptor_set_);
    stencil_render_pipeline_->compile({
        .cull_mode = vk::CullModeFlagBits::eBack,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = false,
        .depth_write_enable = false,
        .stencil_test_enable = true,
        .stencil_front = vk::StencilOpState{}
            .setFailOp(vk::StencilOp::eReplace)
            .setPassOp(vk::StencilOp::eReplace)
            .setCompareOp(vk::CompareOp::eAlways)
            .setCompareMask(0xff)
            .setWriteMask(0xff)
            .setReference(1),
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
    });

    // 2. Thick wireframe of the same instance, visible only outside the
    //    silhouette (stencil != 1) - the outer contour.
    outlining_shader_program_ = interface->createGraphicsShaderProgram();
    outlining_shader_program_->attach(vert_shader);
    outlining_shader_program_->attach(interface->loadShader(getName() + "/outlining.frag", Renderer::ShaderStage::eFragment));
    outlining_render_pipeline_ = interface->createGraphicsRenderPipeline(renderer, outlining_shader_program_, getName());
    outlining_render_pipeline_->setVertexInput(vertex_input);
    outlining_render_pipeline_->setDescriptorSet(descriptor_set_);
    outlining_render_pipeline_->compile({
        .polygon_mode = vk::PolygonMode::eLine,
        .line_width = 6.0f,
        .cull_mode = vk::CullModeFlagBits::eBack,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = false,
        .depth_write_enable = false,
        .stencil_test_enable = true,
        .stencil_front = vk::StencilOpState{}
            .setFailOp(vk::StencilOp::eKeep)
            .setPassOp(vk::StencilOp::eReplace)
            .setCompareOp(vk::CompareOp::eNotEqual)
            .setCompareMask(0xff)
            .setWriteMask(0xff)
            .setReference(1),
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
    });
}

void OutliningPass::executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    if (resource.selected_mesh_instance_index == uint32_t(-1)) {
        return;
    }
    // No active camera: culling early-outs (reset/compact never ran), so the
    // outlining command may be stale and the camera uniform is outdated.
    if (!global_context->camera_system->hasActiveViewportCamera()) {
        return;
    }

    auto config = global_context->render_system->getRendererConfig();
    auto w = static_cast<float>(config.swapchain_image_width);
    auto h = static_cast<float>(config.swapchain_image_height);
    renderer->setViewport(0.0f, h, w, -h);
    renderer->setScissor(0, 0, config.swapchain_image_width, config.swapchain_image_height);

    auto cmdbuf = renderer->getCurrentBuffer();
    auto outlining_buffer = resource.outlining_buffer->getBuffer(renderer->getCurrentFrame());

    cmdbuf.bindVertexBuffers(0, {
        global_context->asset_system->getMeshPool()->position_buffer->getBuffer(),
        outlining_buffer
    }, {0, 0});
    renderer->bindIndexBuffer(global_context->asset_system->getMeshPool()->index_buffer);

    constexpr vk::DeviceSize command_offset = sizeof(glm::vec4) * 3;

    renderer->bindPipeline(stencil_render_pipeline_);
    renderer->bindDescriptorSets(stencil_render_pipeline_);
    cmdbuf.drawIndexedIndirect(outlining_buffer, command_offset, 1, sizeof(vk::DrawIndexedIndirectCommand));

    renderer->bindPipeline(outlining_render_pipeline_);
    renderer->bindDescriptorSets(outlining_render_pipeline_);
    cmdbuf.drawIndexedIndirect(outlining_buffer, command_offset, 1, sizeof(vk::DrawIndexedIndirectCommand));
}

}  // namespace wen
