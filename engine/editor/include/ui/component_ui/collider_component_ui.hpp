#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/undo.hpp"
#include "function/framework/component/physics/collider_component.hpp"

namespace wen::editor {

// ColliderComponent 自定义检查器:形状下拉 + 按形状显示尺寸参数 + 中心偏移 + 触发器勾选。
template <>
class ComponentView<ColliderComponent> {
public:
    ComponentView(ColliderComponent& component) : component_(component) {}
    ColliderComponent& getComponent() { return component_; }

private:
    ColliderComponent& component_;
};

namespace {

const char* colliderShapeName(int shape_type) {
    switch (shape_type) {
        case ColliderComponent::kShapeSphere:
            return "Sphere";
        case ColliderComponent::kShapeCapsule:
            return "Capsule";
        case ColliderComponent::kShapeCylinder:
            return "Cylinder";
        case ColliderComponent::kShapeBox:
        default:
            return "Box";
    }
}

}  // namespace

template <>
class ComponentUI<ColliderComponent> {
public:
    void render(ComponentView<ColliderComponent>& view, const std::function<void()>& on_remove = {}) {
        auto& component = view.getComponent();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("ColliderComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            renderShapeCombo(&component);
            switch (component.shape_type) {
                case ColliderComponent::kShapeBox:
                    renderVec3(&component, "half_extents", component.half_extents);
                    break;
                case ColliderComponent::kShapeSphere:
                    renderFloat(&component, "radius", component.radius);
                    break;
                case ColliderComponent::kShapeCapsule:
                case ColliderComponent::kShapeCylinder:
                    renderFloat(&component, "radius", component.radius);
                    renderFloat(&component, "height", component.height);
                    break;
                default:
                    break;
            }
            renderVec3(&component, "center", component.center);
            renderBool(&component, "is_trigger", component.is_trigger);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

private:
    void renderShapeCombo(ColliderComponent* component) {
        auto uuid = component->getGameObject()->getUUID();
        auto class_name = component->getClassName();
        int before = component->shape_type;
        if (ImGui::BeginCombo("shape", colliderShapeName(component->shape_type))) {
            if (ImGui::Selectable("Box", component->shape_type == ColliderComponent::kShapeBox)) {
                component->shape_type = ColliderComponent::kShapeBox;
            }
            if (ImGui::Selectable("Sphere", component->shape_type == ColliderComponent::kShapeSphere)) {
                component->shape_type = ColliderComponent::kShapeSphere;
            }
            if (ImGui::Selectable("Capsule", component->shape_type == ColliderComponent::kShapeCapsule)) {
                component->shape_type = ColliderComponent::kShapeCapsule;
            }
            if (ImGui::Selectable("Cylinder", component->shape_type == ColliderComponent::kShapeCylinder)) {
                component->shape_type = ColliderComponent::kShapeCylinder;
            }
            ImGui::EndCombo();
        }
        if (component->shape_type != before) {
            component->triggerMemberUpdateCallbacks();
            pushPropertyEdit(uuid, class_name, {{"shape_type", before, component->shape_type}});
        }
    }

    void renderFloat(ColliderComponent* component, const char* member, float& value) {
        float before = value;
        ImGui::DragFloat(member, &value, 0.05f);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }

    void renderVec3(ColliderComponent* component, const char* member, glm::vec3& value) {
        glm::vec3 before = value;
        ImGui::DragFloat3(member, &value.x, 0.05f);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }

    void renderBool(ColliderComponent* component, const char* member, bool& value) {
        bool before = value;
        ImGui::Checkbox(member, &value);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }
};

}  // namespace wen::editor
