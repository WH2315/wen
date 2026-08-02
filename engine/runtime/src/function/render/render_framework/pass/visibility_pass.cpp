// -----------------------------------------------------------------
// VisibilityPass
// -----------------------------------------------------------------

#include "function/render/render_framework/pass/visibility_pass.hpp"
#include "engine/global_context.hpp"

namespace wen {

void VisibilityPass::addAttachment(Renderer::RenderPass& render_pass) {
    render_pass.addAttachment("visibility_buffer", Renderer::AttachmentType::eRG32Uint, vk::ImageUsageFlagBits::eStorage);
}

void VisibilityPass::setAttachment(Renderer::RenderSubpass& render_subpass) {
    render_subpass.setDepthAttachment("depth");
    render_subpass.setOutputAttachment("visibility_buffer");
}

void VisibilityPass::setSubpassDependency(Renderer::RenderPass& render_pass) {
    render_pass.addSubpassDependency(
        Renderer::EXTERNAL_SUBPASS,
        "visibility_pass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests
        },
        {
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );
}

void VisibilityPass::createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto interface = global_context->render_system->getInterface();

    descriptor_set_ = interface->createDescriptorSet();
    descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eVertex}
    });
    descriptor_set_->build();
    descriptor_set_->bindUniform(0, global_context->camera_system->getViewportCamera());

    visibility_shader_program_ = interface->createGraphicsShaderProgram();
    visibility_shader_program_->attach(interface->loadShader(getName() + "/shader.vert", Renderer::ShaderStage::eVertex));
    visibility_shader_program_->attach(interface->loadShader(getName() + "/shader.frag", Renderer::ShaderStage::eFragment));
    visibility_pipeline_pipeline_ = interface->createGraphicsRenderPipeline(renderer, visibility_shader_program_, getName());
    visibility_pipeline_pipeline_->setVertexInput(interface->createVertexInput({
        {0, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat3}},
        {1, Renderer::InputRate::eInstance, {Renderer::VertexType::eFloat4, Renderer::VertexType::eFloat4, Renderer::VertexType::eFloat4}}
    }));
    visibility_pipeline_pipeline_->setDescriptorSet(descriptor_set_);
    visibility_pipeline_pipeline_->compile({
        .cull_mode = vk::CullModeFlagBits::eBack,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = true,
        // Set per frame so the pipeline survives swapchain resizes.
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
    });
}

void VisibilityPass::executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto cmdbuf = renderer->getCurrentBuffer();

    auto config = global_context->render_system->getRendererConfig();
    auto w = static_cast<float>(config.swapchain_image_width);
    auto h = static_cast<float>(config.swapchain_image_height);
    renderer->setViewport(0.0f, h, w, -h);
    renderer->setScissor(0, 0, config.swapchain_image_width, config.swapchain_image_height);

    cmdbuf.bindVertexBuffers(0, {
        global_context->asset_system->getMeshPool()->position_buffer->getBuffer(),
        resource.instance_datas_buffer->getBuffer(renderer->getCurrentFrame())
    }, {0, 0});
    renderer->bindIndexBuffer(global_context->asset_system->getMeshPool()->index_buffer);

    renderer->bindPipeline(visibility_pipeline_pipeline_);
    renderer->bindDescriptorSets(visibility_pipeline_pipeline_);
    cmdbuf.drawIndexedIndirectCount(
        resource.available_indirect_commands_buffer->getBuffer(renderer->getCurrentFrame()),
        0,
        resource.counts_buffer->getBuffer(renderer->getCurrentFrame()),
        sizeof(uint32_t) * 1,
        global_context->asset_system->getMaxMeshCount(),
        sizeof(vk::DrawIndexedIndirectCommand)
    );
}

}  // namespace wen