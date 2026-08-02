#include "ui/panels/inspector/component_ui/component_ui_manager.hpp"
#include "ui/panels/inspector/component_ui/perspective_camera_component_ui.hpp"

namespace wen::editor {

ComponentUIManager::ComponentUIManager() {
    registerComponentUI<PerspectiveCameraComponent>();
}

template <class ComponentCls>
void ComponentUIManager::registerComponentUI() {
    auto ui = std::make_shared<ComponentUI<ComponentCls>>();
    ui_renderers_[ComponentCls::GetClassName()] = [this, ui](Component* component) {
        auto& view = component_view_cache_[component];
        if (!view) {
            view = std::make_shared<ComponentView<ComponentCls>>(*static_cast<ComponentCls*>(component));
        }
        ui->render(*static_cast<ComponentView<ComponentCls>*>(view.get()));
    };
}

void ComponentUIManager::renderComponent(Component* component) {
    if (auto iter = ui_renderers_.find(component->getClassName()); iter != ui_renderers_.end()) {
        iter->second(component);
        return;
    }

    // 没有注册自定义 UI:按反射信息通用地绘制组件
    ImGui::PushID(static_cast<const void*>(component));
    if (ImGui::TreeNodeEx(component->getClassName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& descriptor = global_context->reflect_system->getClass(component->getClassName());
        bool changed = false;
        for (const auto& member_name : descriptor.getMemberNames()) {
            const auto& member = descriptor.getMember(member_name);
            switch (member.getType()) {
                case MemberType::eInt:
                    changed |= ImGui::DragInt(member_name.c_str(), &member.getValueReferenceByPtr<int>(component));
                    break;
                case MemberType::eFloat:
                    changed |= ImGui::DragFloat(member_name.c_str(), &member.getValueReferenceByPtr<float>(component), 0.1f);
                    break;
                case MemberType::eBool:
                    changed |= ImGui::Checkbox(member_name.c_str(), &member.getValueReferenceByPtr<bool>(component));
                    break;
                case MemberType::eVec2:
                    changed |= ImGui::DragFloat2(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec2>(component).x, 0.1f);
                    break;
                case MemberType::eVec3:
                    changed |= ImGui::DragFloat3(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec3>(component).x, 0.1f);
                    break;
                case MemberType::eVec4:
                    changed |= ImGui::DragFloat4(member_name.c_str(), &member.getValueReferenceByPtr<glm::vec4>(component).x, 0.1f);
                    break;
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
