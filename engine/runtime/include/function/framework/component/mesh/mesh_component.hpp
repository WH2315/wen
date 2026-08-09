#pragma once

#include "function/framework/component/transform/transform_component.hpp"
#include "function/asset/mesh/mesh.hpp"
#include "engine/global_context.hpp"

namespace wen {

class MeshComponent : public Component {
    REFLECT_CLASS("MeshComponent")

public:
    std::string getClassName() const override { return "MeshComponent"; }
    static std::string GetClassName() { return "MeshComponent"; }

    MeshComponent() = default;
    MeshComponent(MeshID mesh_id) : mesh_id(mesh_id) {}

    MeshID mesh_id = MeshID(-1);

    REFLECT_MEMBER()
    std::string mesh_path;

    void onCreate() override {
        if (mesh_id == MeshID(-1) && !mesh_path.empty()) {
            mesh_id = global_context->asset_system->loadMesh(mesh_path);
        } else if (mesh_id != MeshID(-1) && mesh_path.empty()) {
            mesh_path = global_context->asset_system->getMeshFilename(mesh_id);
        }
        if (mesh_id == MeshID(-1)) {
            WEN_CORE_WARN("MeshComponent: no valid mesh (path: \"{}\"), instance not created.", mesh_path)
            return;
        }
        auto mesh_instance_pool = global_context->render_system->getRenderData()->getMeshInstancePool();
        auto transform_component = game_object_->queryComponent<TransformComponent>();
        if (transform_component != nullptr) {
            mesh_instance_pool->createMeshInstance({
                .location = transform_component->location,
                .rotation = transform_component->rotation,
                .scale = transform_component->scale,
                .mesh_id = mesh_id
            }, game_object_->getUUID());
            transform_callback_id_ = transform_component->addMemberUpdateCallback(
                [transform_component, uuid = game_object_->getUUID()](Component* component) {
                    auto ptr = global_context->render_system->getRenderData()->getMeshInstancePool()->getMeshInstancePtr(uuid);
                    ptr->location = transform_component->location;
                    ptr->rotation = transform_component->rotation;
                    ptr->scale = transform_component->scale;
                });
            has_transform_callback_ = true;
        } else {
            mesh_instance_pool->createMeshInstance({
                .location = {0.0f, 0.0f, 0.0f},
                .rotation = {0.0f, 0.0f, 0.0f},
                .scale = {1.0f, 1.0f, 1.0f},
                .mesh_id = mesh_id
            }, game_object_->getUUID());
        }
    }

    // 反注册渲染侧的网格实例,并解绑挂在 TransformComponent 上的同步回调
    void onDestroy() override {
        if (has_transform_callback_) {
            if (auto* transform_component = game_object_->queryComponent<TransformComponent>()) {
                transform_component->removeMemberUpdateCallback(transform_callback_id_);
            }
            has_transform_callback_ = false;
        }
        if (auto* render_data = global_context->render_system->getRenderData()) {
            render_data->getMeshInstancePool()->removeMeshInstance(game_object_->getUUID());
        }
    }

private:
    uint32_t transform_callback_id_ = 0;
    bool has_transform_callback_ = false;
};

}  // namespace wen