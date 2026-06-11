// -----------------------------------------------------------------
// CullingPass
// GPU 驱动的层次化深度剔除(Hierarchical Z-Buffer Culling)
// -----------------------------------------------------------------

#include "function/render/render_framework/pass/culling_pass.hpp"
#include "engine/global_context.hpp"

namespace wen {

void CullingPass::addAttachment(Renderer::RenderPass& render_pass) {
    render_pass.addAttachment("depth", Renderer::AttachmentType::eDepth);
}

void CullingPass::createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto manager = global_context->render_system->getAPIManager();
    auto interface = global_context->render_system->getInterface();
    auto max_primitive_count = global_context->asset_system->getMaxPrimitiveCount();
    auto max_mesh_instance_count = global_context->render_system->getMaxMeshInstanceCount();
    auto width = global_context->render_system->getRendererConfig().swapchain_image_width;
    auto height = global_context->render_system->getRendererConfig().swapchain_image_height;
    resource.depth_mip_level_count = glm::floor(glm::log2<float>(glm::max(width, height))) + 1;

    depth_descriptor_set_ = interface->createDescriptorSet();
    depth_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, 1, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageImage, resource.depth_mip_level_count, Renderer::ShaderStage::eCompute},
        {2, vk::DescriptorType::eStorageImage, resource.depth_mip_level_count, Renderer::ShaderStage::eCompute}
    });
    depth_descriptor_set_->build();

    depth_constants_ = interface->createPushConstants(
        Renderer::ShaderStage::eCompute,
        {
            {"image_size", Renderer::ConstantType::eUint32x2},
            {"mip_level", Renderer::ConstantType::eUint32}
        }
    );

    vk::ImageMemoryBarrier image_barrier{};
    image_barrier.setImage(renderer->framebuffer_set->attachments.at(renderer->render_pass->getAttachmentIndex("depth", true))->image->image)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eEarlyFragmentTests,
        {},
        {},
        {},
        image_barrier
    );
    manager->command_pool->freeSingleUse(cmdbuf);

    vk::SamplerReductionModeCreateInfo reduction_mode_ci{};
    reduction_mode_ci.setReductionMode(vk::SamplerReductionModeEXT::eMax);
    vk::SamplerCreateInfo sampler_ci{};
    sampler_ci.setMagFilter(vk::Filter::eNearest)
        .setMinFilter(vk::Filter::eNearest)
        .setMipmapMode(vk::SamplerMipmapMode::eNearest)
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setMinLod(0)
        .setMaxLod(resource.depth_mip_level_count - 1)
        .setPNext(&reduction_mode_ci);
    resource.depth_sampler = interface->createSampler();
    manager->device->device.destroySampler(resource.depth_sampler->sampler);
    resource.depth_sampler->sampler = manager->device->device.createSampler(sampler_ci);

    renderer->registerResourceRecreateCallback([&, ptr = renderer.get()]() {
        vk::ImageMemoryBarrier image_barrier{};
        image_barrier.setImage(ptr->framebuffer_set->attachments.at(ptr->render_pass->getAttachmentIndex("depth", true))->image->image)
            .setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
        auto cmdbuf = manager->command_pool->allocateSingleUse();
        cmdbuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            {},
            {},
            {},
            image_barrier
        );
        manager->command_pool->freeSingleUse(cmdbuf);

        for (size_t in_flight_index = 0; in_flight_index < global_context->render_system->getRendererConfig().max_frames_in_flight; in_flight_index++) {
            vk::DescriptorImageInfo depth_image_info{};
            depth_image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(ptr->framebuffer_set->attachments.at(ptr->render_pass->getAttachmentIndex("depth", true))->image_view)
                .setSampler(resource.depth_sampler->sampler);
            vk::WriteDescriptorSet write{};
            write.setDstSet(depth_descriptor_set_->getDescriptorSets()[in_flight_index])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(depth_image_info);
            manager->device->device.updateDescriptorSets({write}, {});
        }
    });

    for (size_t in_flight_index = 0; in_flight_index < global_context->render_system->getRendererConfig().max_frames_in_flight; in_flight_index++) {
        auto depth_image = std::make_shared<Renderer::DepthImage>(
            width,
            height,
            vk::Format::eR32Sfloat,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            resource.depth_mip_level_count
        );
        resource.depth_images.push_back(depth_image);

        vk::DescriptorImageInfo image_info{};
        image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(renderer->framebuffer_set->attachments.at(renderer->render_pass->getAttachmentIndex("depth", true))->image_view)
            .setSampler(resource.depth_sampler->sampler);
        vk::WriteDescriptorSet write{};
        write.setDstSet(depth_descriptor_set_->getDescriptorSets()[in_flight_index])
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setImageInfo(image_info);
        manager->device->device.updateDescriptorSets({write}, {});
        std::vector<vk::DescriptorImageInfo> image_infos(resource.depth_mip_level_count);
        for (uint32_t i = 0; i < resource.depth_images.back()->getMipLevels(); i++) {
            image_infos[i].setImageLayout(resource.depth_images.back()->getImageLayout())
                .setImageView(resource.depth_images.back()->getMipmapViews()[i]);
        }
        write.setDstBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setImageInfo(image_infos);
        manager->device->device.updateDescriptorSets({write}, {});
        write.setDstBinding(2);
        manager->device->device.updateDescriptorSets({write}, {});
    }

    auto a = [&](const char* shader_name, auto& program, auto& pipeline) {
        program = interface->createComputeShaderProgram();
        program->setComputeShader(interface->loadShader(getName() + "/" + shader_name, Renderer::ShaderStage::eCompute));
        pipeline = interface->createComputeRenderPipeline(program);
        pipeline->setDescriptorSet(depth_descriptor_set_);
        pipeline->setPushConstants(depth_constants_);
        pipeline->compile();
    };
    a("copy_depth_texture.comp", copy_depth_texture_shader_program_, copy_depth_texture_render_pipeline_);
    a("generate_depth_texture.comp", generate_depth_texture_shader_program_, generate_depth_texture_render_pipeline_);

    resource.counts_buffer = std::make_shared<Renderer::InFlightBuffer>(
        sizeof(uint32_t) * 2,
        vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    auto createBuffer = [&](uint32_t size, vk::BufferUsageFlags buffer_usage = {}) {
        return std::make_shared<Renderer::InFlightBuffer>(
            size,
            vk::BufferUsageFlagBits::eStorageBuffer | buffer_usage,
            VMA_MEMORY_USAGE_CPU_COPY,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        );
    };
    resource.visible_mesh_instance_indices_buffer = createBuffer(sizeof(uint32_t) * 2 * max_mesh_instance_count);
    resource.primitive_counts_buffer = createBuffer(sizeof(uint32_t) * max_primitive_count);
    resource.indirect_commands_buffer = createBuffer(sizeof(vk::DrawIndexedIndirectCommand) * max_primitive_count);
    resource.available_indirect_commands_buffer = createBuffer(resource.indirect_commands_buffer->getSize(), vk::BufferUsageFlagBits::eIndirectBuffer);
    resource.instance_datas_buffer = createBuffer(sizeof(glm::vec4) * 3 * max_mesh_instance_count, vk::BufferUsageFlagBits::eVertexBuffer);
    resource.selected_mesh_instance_index = uint32_t(-1);

    for (size_t in_flight_index = 0; in_flight_index < global_context->render_system->getRendererConfig().max_frames_in_flight; in_flight_index++) {
        auto ptr = static_cast<uint32_t*>(resource.counts_buffer->map(in_flight_index));
        ptr[0] = 0;  // visible_mesh_instance_count
        ptr[1] = 0;  // available_indirect_command_count
    }

    readonly_descriptor_set_ = interface->createDescriptorSet();
    readonly_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {2, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {3, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eCompute}
    });
    readonly_descriptor_set_->build();
    readonly_descriptor_set_->bindStorageBuffer(0, global_context->render_system->getRenderData()->getMeshInstancePool()->mesh_instance_buffer);
    readonly_descriptor_set_->bindStorageBuffer(1, global_context->asset_system->getMeshPool()->mesh_descriptor_buffer);
    readonly_descriptor_set_->bindStorageBuffer(2, global_context->asset_system->getMeshPool()->primitive_descriptor_buffer);
    readonly_descriptor_set_->bindUniform(3, global_context->camera_system->getViewportCamera());

    descriptor_set_ = interface->createDescriptorSet();
    descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {2, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {3, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {4, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {5, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {6, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eCompute},
    });
    descriptor_set_->build();
    descriptor_set_->bindStorageBuffer(0, resource.counts_buffer);
    descriptor_set_->bindStorageBuffer(1, resource.visible_mesh_instance_indices_buffer);
    descriptor_set_->bindStorageBuffer(2, resource.primitive_counts_buffer);
    descriptor_set_->bindStorageBuffer(3, resource.indirect_commands_buffer);
    descriptor_set_->bindStorageBuffer(4, resource.available_indirect_commands_buffer);
    descriptor_set_->bindStorageBuffer(5, resource.instance_datas_buffer);
    descriptor_set_->bindDepthImages(6, resource.depth_images, resource.depth_sampler);

    constants_ = interface->createPushConstants(
        Renderer::ShaderStage::eCompute,
        {
            {"mesh_instance_count", Renderer::ConstantType::eUint32},
            {"primitive_count", Renderer::ConstantType::eUint32},
            {"depth_texture_size", Renderer::ConstantType::eUint32x2},
            {"selected_mesh_instance_index", Renderer::ConstantType::eUint32}
        }
    );
    glm::uvec2 size(width, height);
    constants_->pushConstant("depth_texture_size", &size);

    auto b = [&](const char* shader_name, auto& program, auto& pipeline) {
        program = interface->createComputeShaderProgram();
        program->setComputeShader(interface->loadShader(getName() + "/" + shader_name, Renderer::ShaderStage::eCompute));
        pipeline = interface->createComputeRenderPipeline(program);
        pipeline->setDescriptorSet(descriptor_set_, 0);
        pipeline->setDescriptorSet(readonly_descriptor_set_, 1);
        pipeline->setPushConstants(constants_);
        pipeline->compile();
    };
    b("pre_culling.comp", pre_culling_shader_program_, pre_culling_render_pipeline_);
    b("reset_indirect_command.comp", reset_indirect_command_shader_program_, reset_indirect_command_render_pipeline_);
    b("compact_instance.comp", compact_instance_shader_program_, compact_instance_render_pipeline_);
    b("generate_available_indirect_command.comp", generate_available_indirect_command_shader_program_, generate_available_indirect_command_render_pipeline_);
}

void CullingPass::executePreRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    constants_->pushConstant("selected_mesh_instance_index", &resource.selected_mesh_instance_index);

    auto cmdbuf = renderer->getCurrentBuffer();

    // 1.将深度缓存复制到深度纹理的 mip_level 0
    auto width = global_context->render_system->getRendererConfig().swapchain_image_width;
    auto height = global_context->render_system->getRendererConfig().swapchain_image_height;
    glm::uvec2 image_size(width, height);
    depth_constants_->pushConstant("image_size", &image_size);
    renderer->bindPipeline(copy_depth_texture_render_pipeline_);
    renderer->bindDescriptorSets(copy_depth_texture_render_pipeline_);
    renderer->pushConstants(copy_depth_texture_render_pipeline_);

    vk::ImageMemoryBarrier image_barrier{};
    image_barrier.setImage(renderer->framebuffer_set->attachments.at(renderer->render_pass->getAttachmentIndex("depth", true))->image->image)
        .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::PipelineStageFlagBits::eComputeShader,
        {},
        {image_barrier}
    );

    if (!global_context->camera_system->isFixedClip()) {
        renderer->dispatch(std::ceil(image_size.x / 16.0), std::ceil(image_size.y / 16.0), 1);
    }

    image_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
        .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eEarlyFragmentTests,
        {},
        {image_barrier}
    );

    // 2.生成 mip_level 1~N 的层次化深度纹理
    renderer->bindPipeline(generate_depth_texture_render_pipeline_);

    image_barrier.setImage(resource.depth_images[renderer->getCurrentFrame()]->getImage())
        .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {},
        {image_barrier}
    );

    for (uint32_t i = 1; i < resource.depth_mip_level_count; i++) {
        glm::uvec2 image_size(std::max<uint32_t>(1, width >> i), std::max<uint32_t>(1, height >> i));
        depth_constants_->pushConstant("image_size", &image_size);
        depth_constants_->pushConstant("mip_level", &i);
        renderer->pushConstants(generate_depth_texture_render_pipeline_);
        renderer->dispatch(std::ceil(image_size.x / 16.0), std::ceil(image_size.y / 16.0), 1);

        image_barrier.setImage(resource.depth_images[renderer->getCurrentFrame()]->getImage())
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, i, 1, 0, 1});
        renderer->pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            {},
            {image_barrier}
        );
    }

    // 3.遍历所有的 MeshInstance，将不被剔除的实例收集起来
    uint32_t mesh_instance_count = global_context->render_system->getRenderData()->getMeshInstancePool()->current_instance_count;
    uint32_t primitive_count = global_context->asset_system->getMeshPool()->getPrimitiveCount();
    constants_->pushConstant("mesh_instance_count", &mesh_instance_count);
    constants_->pushConstant("primitive_count", &primitive_count);
    auto count_ptr = static_cast<uint32_t*>(resource.counts_buffer->map(renderer->getCurrentFrame()));
    count_ptr[0] = 0;
    count_ptr[1] = 0;

    renderer->bindPipeline(pre_culling_render_pipeline_);
    renderer->bindDescriptorSets(pre_culling_render_pipeline_);
    renderer->pushConstants(pre_culling_render_pipeline_);
    renderer->dispatch(std::ceil(mesh_instance_count / 256.0), 1, 1);

    vk::BufferMemoryBarrier buffer_barrier{};
    buffer_barrier.setBuffer(resource.primitive_counts_buffer->getBuffer(renderer->getCurrentFrame()))
        .setOffset(0)
        .setSize(resource.primitive_counts_buffer->getSize())
        .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {buffer_barrier},
        {}
    );

    // 4.重置间接命令模板
    renderer->bindPipeline(reset_indirect_command_render_pipeline_);
    renderer->dispatch(std::ceil(primitive_count / 256.0), 1, 1);

    buffer_barrier.setBuffer(resource.counts_buffer->getBuffer(renderer->getCurrentFrame()))
        .setSize(resource.counts_buffer->getSize())
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {buffer_barrier},
        {}
    );

    buffer_barrier.setBuffer(resource.visible_mesh_instance_indices_buffer->getBuffer(renderer->getCurrentFrame()))
        .setSize(resource.visible_mesh_instance_indices_buffer->getSize())
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {buffer_barrier},
        {}
    );

    buffer_barrier.setBuffer(resource.indirect_commands_buffer->getBuffer(renderer->getCurrentFrame()))
        .setSize(resource.indirect_commands_buffer->getSize())
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {buffer_barrier},
        {}
    );

    // 5.将可见的 MeshInstance 索引压缩到连续的缓冲区中
    renderer->bindPipeline(compact_instance_render_pipeline_);
    renderer->dispatch(std::ceil(mesh_instance_count / 256.0), 1, 1);

    buffer_barrier.setBuffer(resource.indirect_commands_buffer->getBuffer(renderer->getCurrentFrame()))
        .setSize(resource.indirect_commands_buffer->getSize())
        .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    renderer->pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eComputeShader,
        {buffer_barrier},
        {}
    );

    // 6.根据可见的 MeshInstance 生成间接绘制命令
    renderer->bindPipeline(generate_available_indirect_command_render_pipeline_);
    renderer->dispatch(std::ceil(primitive_count / 256.0), 1, 1);

    vk::MemoryBarrier compute_to_graphics_barrier{};
    compute_to_graphics_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
        .setDstAccessMask(vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead);
    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eFragmentShader,
        {},
        compute_to_graphics_barrier,
        {},
        {}
    );
}

}  // namespace wen