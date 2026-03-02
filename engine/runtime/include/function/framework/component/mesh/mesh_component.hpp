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

    MeshComponent(MeshID mesh_id) : mesh_id(mesh_id) {}
    MeshID mesh_id;

    void onCreate() override {
        auto mesh_instance_pool = global_context->render_system->getSwapData()->getMeshInstancePool(); 
        auto transform_component = master_->queryComponent<TransformComponent>();
        if (transform_component != nullptr) {
            mesh_instance_pool->createMeshInstance({
                .location = transform_component->location,
                .rotation = transform_component->rotation,
                .scale = transform_component->scale,
                .mesh_id = mesh_id
            }, master_->getUUID());
            transform_component->addMemberUpdateCallback([transform_component, uuid = master_->getUUID()](Component* component) {
                auto ptr = global_context->render_system->getSwapData()->getMeshInstancePool()->getMeshInstancePtr(uuid);
                ptr->location = transform_component->location;
                ptr->rotation = transform_component->rotation;
                ptr->scale = transform_component->scale;
            });
        } else {
            mesh_instance_pool->createMeshInstance({
                .location = {0.0f, 0.0f, 0.0f},
                .rotation = {0.0f, 0.0f, 0.0f},
                .scale = {1.0f, 1.0f, 1.0f},
                .mesh_id = mesh_id
            }, master_->getUUID());
        }
    }
};

}  // namespace wen