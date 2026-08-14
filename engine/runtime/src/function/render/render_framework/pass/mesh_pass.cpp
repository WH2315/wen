// -----------------------------------------------------------------
// MeshPass
// -----------------------------------------------------------------

#include "function/render/render_framework/pass/mesh_pass.hpp"
#include "engine/global_context.hpp"

namespace wen {

void MeshPass::setAttachment(Renderer::RenderSubpass& render_subpass) {
    render_subpass.setInputAttachment("visibility_buffer");
    render_subpass.setOutputAttachment(global_context->render_system->output_attachment_name);
}
void MeshPass::setSubpassDependency(Renderer::RenderPass& render_pass) {
    render_pass.addSubpassDependency(
        "visibility_pass",
        "mesh_pass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentRead
        }
    );
}

void MeshPass::createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto manager = global_context->render_system->getAPIManager();
    auto interface = global_context->render_system->getInterface();

    descriptor_set_ = interface->createDescriptorSet();
    descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eInputAttachment, Renderer::ShaderStage::eFragment},
        {1, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eFragment},
        {2, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {3, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {4, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {5, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {6, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {7, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {8, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {9, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        {10, vk::DescriptorType::eCombinedImageSampler, 16, Renderer::ShaderStage::eFragment},
        {11, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment},
        // IBL:环境立方体 / 辐照度 / 预过滤镜面 / BRDF LUT
        {12, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eFragment},
        {13, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eFragment},
        {14, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eFragment},
        {15, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eFragment}
    });
    descriptor_set_->build();
    visibility_sampler_ = interface->createSampler({
        .mag_filter = vk::Filter::eNearest,
        .min_filter = vk::Filter::eNearest,
        .address_mode_u = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_v = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_w = vk::SamplerAddressMode::eClampToEdge,
    });
    descriptor_set_->bindInputAttachment(0, renderer, "visibility_buffer", visibility_sampler_);
    // The visibility buffer's image view is recreated with the framebuffers
    // on resize; rebind the input attachment or the descriptor goes stale.
    renderer->registerResourceRecreateCallback([this, ptr = renderer.get()]() {
        std::shared_ptr<Renderer::Renderer> alias(ptr, [](Renderer::Renderer*) {});
        descriptor_set_->bindInputAttachment(0, alias, "visibility_buffer", visibility_sampler_);
    });
    descriptor_set_->bindUniform(1, global_context->camera_system->getViewportCamera());
    descriptor_set_->bindStorageBuffer(2, global_context->asset_system->getMeshPool()->index_buffer);
    descriptor_set_->bindStorageBuffer(3, global_context->asset_system->getMeshPool()->position_buffer);
    descriptor_set_->bindStorageBuffer(4, global_context->asset_system->getMeshPool()->normal_buffer);
    descriptor_set_->bindStorageBuffer(5, global_context->asset_system->getMeshPool()->texcoord_buffer);
    descriptor_set_->bindStorageBuffer(6, global_context->asset_system->getMeshPool()->color_buffer);
    descriptor_set_->bindStorageBuffer(7, resource.instance_datas_buffer);
    descriptor_set_->bindStorageBuffer(8, resource.counts_buffer);
    descriptor_set_->bindStorageBuffer(9, resource.available_indirect_commands_buffer);
    descriptor_set_->bindStorageBuffer(11, global_context->light_system->getLightBuffer());

    // 首次绑定纹理数组(纹理池运行时增长时在 executeRenderPass 重绑)。
    if (auto* texture_pool = global_context->asset_system->getTexturePool()) {
        descriptor_set_->bindTextures(10, texture_pool->texturesSamplersPadded());
        last_bound_texture_count_ = texture_pool->getTextureCount();
    }

    // 绑定 IBL 环境资源(EnvironmentSystem 在渲染器创建前已生成完毕)。
    auto& env = *global_context->environment_system;
    if (env.hasEnvironment()) {
        descriptor_set_->bindTexture(12, env.getEnvCubemap(), env.getEnvSampler());
        descriptor_set_->bindTexture(13, env.getIrradianceCubemap(), env.getIrradianceSampler());
        descriptor_set_->bindTexture(14, env.getPrefilteredCubemap(), env.getPrefilteredSampler());
        descriptor_set_->bindTexture(15, env.getBrdfLut(), env.getBrdfSampler());
    } else {
        // 无环境(缺 default_env.hdr):绑定 1x1 黑色兜底立方体贴图 -> 天空黑、IBL 为 0。
        descriptor_set_->bindTexture(12, env.getFallbackCubemap(), env.getFallbackSampler());
        descriptor_set_->bindTexture(13, env.getFallbackCubemap(), env.getFallbackSampler());
        descriptor_set_->bindTexture(14, env.getFallbackCubemap(), env.getFallbackSampler());
        descriptor_set_->bindTexture(15, env.getBrdfLut(), env.getBrdfSampler());
    }

    mesh_shader_program_ = interface->createGraphicsShaderProgram();
    mesh_shader_program_->attach(interface->loadShader(getName() + "/shader.vert", Renderer::ShaderStage::eVertex));
    mesh_shader_program_->attach(interface->loadShader(getName() + "/shader.frag", Renderer::ShaderStage::eFragment));
    mesh_pipeline_pipeline_ = interface->createGraphicsRenderPipeline(renderer, mesh_shader_program_, getName());
    mesh_pipeline_pipeline_->setDescriptorSet(descriptor_set_);
    push_constants_ = interface->createPushConstants(Renderer::ShaderStage::eFragment, {
        {"lod_debug_enabled", Renderer::ConstantType::eFloat},
    });
    mesh_pipeline_pipeline_->setPushConstants(push_constants_);
    mesh_pipeline_pipeline_->compile({
        .cull_mode = vk::CullModeFlagBits::eNone,
        .depth_test_enable = false,
        // Set per frame so the pipeline survives swapchain resizes.
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
    });
}

void MeshPass::executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto config = global_context->render_system->getRendererConfig();
    auto w = static_cast<float>(config.swapchain_image_width);
    auto h = static_cast<float>(config.swapchain_image_height);
    renderer->setViewport(0.0f, h, w, -h);
    renderer->setScissor(0, 0, config.swapchain_image_width, config.swapchain_image_height);

    // 纹理池运行时增长(加载新纹理)时重绑纹理数组;
    // 重绑会改写 descriptor,须等 GPU 空闲(命令缓冲不再引用)。
    if (auto* texture_pool = global_context->asset_system->getTexturePool();
        texture_pool->getTextureCount() != last_bound_texture_count_) {
        renderer->waitIdle();
        descriptor_set_->bindTextures(10, texture_pool->texturesSamplersPadded());
        last_bound_texture_count_ = texture_pool->getTextureCount();
    }

    renderer->bindPipeline(mesh_pipeline_pipeline_);
    renderer->bindDescriptorSets(mesh_pipeline_pipeline_);
    // LOD 调试开关(Setting 面板勾选,存于 renderer_config)。
    float lod_debug = Renderer::renderer_config.lod_debug_enabled ? 1.0f : 0.0f;
    push_constants_->pushConstant("lod_debug_enabled", &lod_debug);
    renderer->pushConstants(mesh_pipeline_pipeline_);
    renderer->draw(3, 1, 0, 0);
}

}  // namespace wen