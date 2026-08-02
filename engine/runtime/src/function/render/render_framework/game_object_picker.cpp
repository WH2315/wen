#include "function/render/render_framework/game_object_picker.hpp"
#include "engine/global_context.hpp"

namespace wen {

GameObjectPicker::GameObjectPicker(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto manager = global_context->render_system->getAPIManager();
    auto interface = global_context->render_system->getInterface();

    pick_result_buffer_ = std::make_shared<Renderer::StorageBuffer>(
        sizeof(uint32_t),
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_GPU_TO_CPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
    );

    descriptor_set_ = interface->createDescriptorSet();
    descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute},
        {2, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eCompute}
    });
    descriptor_set_->build();
    descriptor_set_->bindStorageBuffer(1, resource.instance_datas_buffer);
    descriptor_set_->bindStorageBuffer(2, pick_result_buffer_);

    // VisibilityBuffer 是 render pass 的附件，DescriptorSet 没有绑定附件为
    // storage image 的封装，这里手动写入描述符，并在附件重建（resize）时刷新。
    auto update_descriptor = [this, ptr = renderer.get()]() {
        auto manager = global_context->render_system->getAPIManager();
        auto max_frames = global_context->render_system->getRendererConfig().max_frames_in_flight;
        auto idx = ptr->render_pass->getAttachmentIndex("visibility_buffer", true);
        for (uint32_t in_flight_index = 0; in_flight_index < max_frames; in_flight_index++) {
            vk::DescriptorImageInfo image_info{};
            image_info.setImageLayout(vk::ImageLayout::eGeneral)
                .setImageView(ptr->framebuffer_set->attachments.at(idx)->image_view);
            vk::WriteDescriptorSet write{};
            write.setDstSet(descriptor_set_->getDescriptorSets()[in_flight_index])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setImageInfo(image_info);
            manager->device->device.updateDescriptorSets({write}, {});
        }
    };
    renderer->registerResourceRecreateCallback(update_descriptor);
    update_descriptor();

    constants_ = interface->createPushConstants(
        Renderer::ShaderStage::eCompute,
        {
            {"pos", Renderer::ConstantType::eUint32x2}
        }
    );

    shader_program_ = interface->createComputeShaderProgram();
    shader_program_->setComputeShader(interface->loadShader("pick_game_object.comp", Renderer::ShaderStage::eCompute));
    pipeline_ = interface->createComputeRenderPipeline(shader_program_);
    pipeline_->setDescriptorSet(descriptor_set_);
    pipeline_->setPushConstants(constants_);
    pipeline_->compile();
}

void GameObjectPicker::addPickTask(const PickTask& task) {
    pick_tasks_.push_back(task);
}

void GameObjectPicker::processPickTasks(std::shared_ptr<Renderer::Renderer> renderer) {
    auto manager = global_context->render_system->getAPIManager();
    auto image = renderer->framebuffer_set->attachments.at(renderer->render_pass->getAttachmentIndex("visibility_buffer", true))->image->image;
    // present() has already advanced the frame counter; the visibility buffer
    // and instance_datas we read belong to the frame that just rendered.
    auto max_frames = global_context->render_system->getRendererConfig().max_frames_in_flight;
    auto frame_index = (renderer->getCurrentFrame() + max_frames - 1) % max_frames;

    for (auto& task : pick_tasks_) {
        glm::uvec2 pos{task.x, task.y};
        constants_->pushConstant("pos", &pos);

        auto cmdbuf = manager->command_pool->allocateSingleUse();

        // 渲染结束后 VisibilityBuffer 处于 eShaderReadOnlyOptimal，
        // imageLoad 需要 eGeneral，用完再转回去。
        vk::ImageMemoryBarrier image_barrier{};
        image_barrier.setImage(image)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, image_barrier);

        cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline_->pipeline);
        cmdbuf.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            pipeline_->pipeline_layout,
            0,
            descriptor_set_->getDescriptorSets()[frame_index],
            {}
        );
        cmdbuf.pushConstants(pipeline_->pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, constants_->total_size, constants_->constants.data());
        cmdbuf.dispatch(1, 1, 1);

        image_barrier.setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, image_barrier);

        // freeSingleUse 提交并等待队列空闲，返回后结果已写入。
        manager->command_pool->freeSingleUse(cmdbuf);

        auto mesh_instance_index = *static_cast<uint32_t*>(pick_result_buffer_->map());
        pick_result_buffer_->unmap();
        if (mesh_instance_index == uint32_t(-1)) {
            if (task.miss_callback) {
                task.miss_callback();
            }
        } else {
            auto& map = global_context->render_system->getRenderData()->getMeshInstancePool()->mesh_instance_index_to_game_object_uuid_map;
            if (auto iter = map.find(mesh_instance_index); iter != map.end()) {
                task.picked_callback(iter->second);
            } else if (task.miss_callback) {
                task.miss_callback();
            }
        }
    }
    pick_tasks_.clear();
}

}  // namespace wen
