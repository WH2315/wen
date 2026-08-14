#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/component_ui/perspective_camera_component_ui.hpp"
#include "ui/component_ui/mesh_component_ui.hpp"
#include "ui/component_ui/material_component_ui.hpp"
#include "ui/component_ui/script_component_ui.hpp"
#include "ui/undo.hpp"

namespace wen::editor {

ComponentUIManager::ComponentUIManager() {
    registerComponentUI<PerspectiveCameraComponent>();
    registerComponentUI<MeshComponent>();
    registerComponentUI<MaterialComponent>();
    registerComponentUI<ScriptComponent>();
}

void ComponentUIManager::renderComponent(Component* component, const std::function<void()>& on_remove) {
    // 注册过自定义 UI 的组件用自定义 UI,否则按反射元数据通用绘制。
    if (auto iter = ui_renderers_.find(component->getClassName()); iter != ui_renderers_.end()) {
        iter->second(component, on_remove);
        return;
    }

    ImGui::PushID(static_cast<const void*>(component));
    bool open = ImGui::TreeNodeEx(component->getClassName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    if (on_remove) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            on_remove();
        }
    }
    if (open) {
        const auto& descriptor = global_context->reflect_system->getClass(component->getClassName());
        bool changed = false;
        for (const auto& member_name : descriptor.getMemberNames()) {
            const auto& member = descriptor.getMember(member_name);
            switch (member.getType()) {
                case MemberType::eInt: {
                    int before = member.getValueReferenceByPtr<int>(component);
                    changed |= ImGui::DragInt(member_name.c_str(), &member.getValueReferenceByPtr<int>(component));
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eFloat: {
                    float before = member.getValueReferenceByPtr<float>(component);
                    changed |= ImGui::DragFloat(member_name.c_str(), &member.getValueReferenceByPtr<float>(component), 0.1f);
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eBool: {
                    bool before = member.getValueReferenceByPtr<bool>(component);
                    changed |= ImGui::Checkbox(member_name.c_str(), &member.getValueReferenceByPtr<bool>(component));
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eVec2: {
                    glm::vec2 before = member.getValueReferenceByPtr<glm::vec2>(component);
                    changed |= ImGui::DragFloat2(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec2>(component).x, 0.1f);
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eVec3: {
                    glm::vec3 before = member.getValueReferenceByPtr<glm::vec3>(component);
                    changed |= ImGui::DragFloat3(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec3>(component).x, 0.1f);
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eVec4: {
                    glm::vec4 before = member.getValueReferenceByPtr<glm::vec4>(component);
                    changed |= ImGui::DragFloat4(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec4>(component).x, 0.1f);
                    trackMemberEdit(component, member_name, before);
                    break;
                }
                case MemberType::eString:
                    ImGui::Text("%s: %s", member_name.c_str(), member.getValueReferenceByPtr<std::string>(component).c_str());
                    break;
                case MemberType::eCustom:
                    ImGui::Text("%s: <custom>", member_name.c_str());
                    break;
            }
        }
        if (changed) {
            component->triggerMemberUpdateCallbacks();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

}  // namespace wen::editor
