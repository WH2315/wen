#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/undo.hpp"
#include "function/framework/component/physics/rigidbody_component.hpp"

namespace wen::editor {

// RigidbodyComponent 自定义检查器:运动类型下拉 + 质量/摩擦/弹性/阻尼/重力缩放/CCD/初速。
template <>
class ComponentView<RigidbodyComponent> {
public:
    ComponentView(RigidbodyComponent& component) : component_(component) {}
    RigidbodyComponent& getComponent() { return component_; }

private:
    RigidbodyComponent& component_;
};

namespace {

const char* rigidbodyTypeName(int body_type) {
    switch (body_type) {
        case RigidbodyComponent::kDynamic:
            return "Dynamic";
        case RigidbodyComponent::kKinematic:
            return "Kinematic";
        case RigidbodyComponent::kStatic:
        default:
            return "Static";
    }
}

}  // namespace

template <>
class ComponentUI<RigidbodyComponent> {
public:
    void render(ComponentView<RigidbodyComponent>& view, const std::function<void()>& on_remove = {}) {
        auto& component = view.getComponent();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("RigidbodyComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            renderTypeCombo(&component);
            renderFloat(&component, "mass", component.mass, 0.1f);
            renderFloat(&component, "friction", component.friction, 0.01f);
            renderFloat(&component, "restitution", component.restitution, 0.01f);
            renderFloat(&component, "linear_damping", component.linear_damping, 0.01f);
            renderFloat(&component, "angular_damping", component.angular_damping, 0.01f);
            renderFloat(&component, "gravity_scale", component.gravity_scale, 0.05f);
            renderBool(&component, "use_ccd", component.use_ccd);
            renderVec3(&component, "linear_velocity", component.linear_velocity);
            renderVec3(&component, "angular_velocity", component.angular_velocity);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

private:
    void renderTypeCombo(RigidbodyComponent* component) {
        auto uuid = component->getGameObject()->getUUID();
        auto class_name = component->getClassName();
        int before = component->body_type;
        if (ImGui::BeginCombo("type", rigidbodyTypeName(component->body_type))) {
            if (ImGui::Selectable("Static", component->body_type == RigidbodyComponent::kStatic)) {
                component->body_type = RigidbodyComponent::kStatic;
            }
            if (ImGui::Selectable("Dynamic", component->body_type == RigidbodyComponent::kDynamic)) {
                component->body_type = RigidbodyComponent::kDynamic;
            }
            if (ImGui::Selectable("Kinematic", component->body_type == RigidbodyComponent::kKinematic)) {
                component->body_type = RigidbodyComponent::kKinematic;
            }
            ImGui::EndCombo();
        }
        if (component->body_type != before) {
            component->triggerMemberUpdateCallbacks();
            pushPropertyEdit(uuid, class_name, {{"body_type", before, component->body_type}});
        }
    }

    void renderFloat(RigidbodyComponent* component, const char* member, float& value, float speed) {
        float before = value;
        ImGui::DragFloat(member, &value, speed);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }

    void renderVec3(RigidbodyComponent* component, const char* member, glm::vec3& value) {
        glm::vec3 before = value;
        ImGui::DragFloat3(member, &value.x, 0.1f);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }

    void renderBool(RigidbodyComponent* component, const char* member, bool& value) {
        bool before = value;
        ImGui::Checkbox(member, &value);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            component->triggerMemberUpdateCallbacks();
        }
        trackMemberEdit(component, member, before);
    }
};

}  // namespace wen::editor
