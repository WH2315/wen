#include "ui/panels/inspector/inspector_panel.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

void InspectorPanel::onUnloadScene() {
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

    for (auto* component : game_object->getComponents()) {
        component_ui_manager_.renderComponent(component);
    }

    ImGui::End();
}

}  // namespace wen::editor
