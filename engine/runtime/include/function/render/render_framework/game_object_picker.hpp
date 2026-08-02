#pragma once

#include "function/render/render_framework/resource.hpp"
#include "function/framework/uuid_manager.hpp"

namespace wen {

// GameObject 拾取器：读取 VisibilityBuffer 上某像素的 InstanceIndex，
// 经 instance_datas 找到 MeshInstance 池索引，再映射到 GameObjectUUID。
class GameObjectPicker {
public:
    struct PickTask {
        uint32_t x, y;
        std::function<void(GameObjectUUID uuid)> picked_callback;
        std::function<void()> miss_callback;
    };

    GameObjectPicker(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource);

    void addPickTask(const PickTask& task);
    void processPickTasks(std::shared_ptr<Renderer::Renderer> renderer);
    bool needProcess() const { return !pick_tasks_.empty(); }

private:
    std::shared_ptr<Renderer::StorageBuffer> pick_result_buffer_;
    std::shared_ptr<Renderer::DescriptorSet> descriptor_set_;
    std::shared_ptr<Renderer::PushConstants> constants_;
    std::shared_ptr<Renderer::ComputeShaderProgram> shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> pipeline_;
    std::vector<PickTask> pick_tasks_;
};

}  // namespace wen
