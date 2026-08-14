#include "ui/panels/inspector_panel.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/prefab_actions.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/prefab/prefab_component.hpp"

namespace wen::editor {

void InspectorPanel::onUnloadScene() {
    component_ui_manager_.clearViewCache();
}

// Prefab 实例被重建后:组件指针全变了,丢弃缓存的组件视图,避免命中悬空指针。
void InspectorPanel::onPrefabReverted() {
    component_ui_manager_.clearViewCache();
}

void InspectorPanel::render() {
    ImGui::Begin("Inspector");

    if (global_ui_context->selected_game_object_uuid == kInvalidGameObjectUUID) {
        ImGui::TextUnformatted("No game object selected.");
        ImGui::End();
        return;
    }

    auto* scene = global_context->scene_manager->getActiveScene();
    auto* game_object = scene ? scene->getGameObject(global_ui_context->selected_game_object_uuid) : nullptr;
    if (game_object == nullptr) {
        ImGui::End();
        return;
    }

    ImGui::Text("GameObject: %s", game_object->getName().c_str());
    ImGui::Separator();

    // Prefab 实例:显示来源 + Revert/Apply。
    // Revert 会销毁重建对象,不能在帧内执行 —— 只挂起请求,由 Editor 主循环帧外处理。
    if (isPrefabInstance(game_object)) {
        auto prefab_path = prefabPathFor(game_object);
        ImGui::TextDisabled("Prefab: %s", prefab_path.c_str());
        if (ImGui::Button("Revert to Prefab")) {
            global_ui_context->pending_prefab = {PrefabActionKind::eRevert, game_object->getUUID()};
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply to Prefab")) {
            global_ui_context->pending_prefab = {PrefabActionKind::eApply, game_object->getUUID()};
        }
        ImGui::Separator();
    }

    // 增删组件推迟到遍历结束后执行,避免在遍历 components_ 时修改它。
    std::string pending_add_class;
    std::string pending_remove_class;

    for (auto* component : game_object->getComponents()) {
        std::function<void()> on_remove;
        // Transform 是基础组件,不允许移除。
        if (component->getClassName() != TransformComponent::GetClassName()) {
            on_remove = [&]() { pending_remove_class = component->getClassName(); };
        }
        component_ui_manager_.renderComponent(component, on_remove);
    }

    // Add Component:列出尚未安装的已注册组件类型。
    ImGui::Separator();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - 70.0f);
    if (ImGui::Button("Add Component", ImVec2(160, 0))) {
        ImGui::OpenPopup("Add Component");
    }
    if (ImGui::BeginPopup("Add Component")) {
        for (const auto& class_name : global_context->component_factory->classNames()) {
            if (class_name == PrefabComponent::GetClassName()) {
                continue;  // PrefabComponent 由 prefab 服务管理,不允许手动添加
            }
            if (game_object->queryComponent(class_name) != nullptr) {
                continue;
            }
            if (ImGui::MenuItem(class_name.c_str())) {
                pending_add_class = class_name;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();

    // 延迟执行增删(含撤销)。
    if (!pending_remove_class.empty()) {
        component_ui_manager_.clearViewCache();
        if (auto* component = game_object->queryComponent(pending_remove_class)) {
            pushComponentRemoved(game_object, component);
            game_object->removeComponent(component);
        }
    }
    if (!pending_add_class.empty()) {
        component_ui_manager_.clearViewCache();
        if (auto* component = global_context->component_factory->create(pending_add_class)) {
            game_object->addComponent(component);
            pushComponentAdded(game_object, component);
        }
    }
}

}  // namespace wen::editor
