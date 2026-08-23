#pragma once

#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/material/material_component.hpp"
#include "function/asset/mesh/mesh.hpp"
#include "engine/global_context.hpp"

namespace wen {

class MeshComponent : public Component {
    REFLECT_CLASS("MeshComponent")

public:
    std::string getClassName() const override { return "MeshComponent"; }
    static std::string GetClassName() { return "MeshComponent"; }

    MeshComponent() { init(); }
    MeshComponent(MeshID mesh_id) : mesh_id(mesh_id) { init(); }

    MeshID mesh_id = MeshID(-1);

    REFLECT_MEMBER()
    std::string mesh_path;

    // 编辑器"Pick Mesh"设置相对 <资源根>/models 的路径,并立即重载网格。
    void setMeshPath(const std::string& path) {
        mesh_path = path;
        reloadFromPath();
    }

    // 按当前 mesh_path 重新解析网格并重建实例(先反注册旧实例)。
    void reloadFromPath() {
        onDestroy();
        mesh_id = MeshID(-1);
        onCreate();
    }

    void onCreate() override {
        if (mesh_id == MeshID(-1) && !mesh_path.empty()) {
            mesh_id = global_context->asset_system->loadMesh(mesh_path);
        } else if (mesh_id != MeshID(-1) && mesh_path.empty()) {
            mesh_path = global_context->asset_system->getMeshFilename(mesh_id);
        }
        loaded_path_ = mesh_path;  // 记录本次加载尝试,mesh_path 未变时不重复加载
        if (mesh_id == MeshID(-1)) {
            WEN_CORE_WARN("MeshComponent: no valid mesh (path: \"{}\"), instance not created.", mesh_path)
            return;
        }
        auto mesh_instance_pool = global_context->render_system->getRenderData()->getMeshInstancePool();
        auto transform_component = game_object_->queryComponent<TransformComponent>();
        if (transform_component != nullptr) {
            mesh_instance_pool->createMeshInstance({
                .location = transform_component->getWorldLocation(),
                .rotation = transform_component->getWorldRotation(),
                .scale = transform_component->getWorldScale(),
                .mesh_id = mesh_id
            }, game_object_->getUUID());
            transform_callback_id_ = transform_component->addMemberUpdateCallback(
                [transform_component, uuid = game_object_->getUUID()](Component* component) {
                    auto ptr = global_context->render_system->getRenderData()->getMeshInstancePool()->getMeshInstancePtr(uuid);
                    ptr->location = transform_component->getWorldLocation();
                    ptr->rotation = transform_component->getWorldRotation();
                    ptr->scale = transform_component->getWorldScale();
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
        // 实例就绪后应用兄弟材质(材质可能在网格之前添加,加载顺序不敏感)。
        if (auto* material = game_object_->queryComponent<MaterialComponent>()) {
            material->applyToMeshInstance();
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
    void init() {
        loaded_path_ = mesh_path;
        // mesh_path 变化时重新加载(编辑器 Pick Mesh / 撤销恢复都经此)。
        addMemberUpdateCallback([this](Component*) {
            if (mesh_path != loaded_path_) {
                reloadFromPath();
            }
        });
    }

    uint32_t transform_callback_id_ = 0;
    bool has_transform_callback_ = false;
    std::string loaded_path_;  // 当前已加载的 mesh_path,用于检测变化
};

}  // namespace wen