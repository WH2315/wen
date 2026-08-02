#include "ui/panels/hierarchy/hierarchy_panel.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

void HierarchyPanel::onLoadScene() {
    selectGameObject();
}

void HierarchyPanel::render() {
    ImGui::Begin("Hierarchy");

    if (auto* scene = global_context->scene_manager->getActiveScene()) {
        for (auto* game_object : scene->getGameObjects()) {
            auto uuid = game_object->getUUID();
            bool selected = (global_ui_context->selected_game_object_uuid == uuid);
            ImGui::PushID(static_cast<int>(uuid));
            if (ImGui::Selectable(game_object->getName().c_str(), selected)) {
                selectGameObject(selected ? kInvalidGameObjectUUID : uuid);
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void HierarchyPanel::selectGameObject(GameObjectUUID uuid) {
    global_ui_context->selected_game_object_uuid = uuid;
    global_context->render_system->setSelectedGameObject(uuid);
}

}  // namespace wen::editor
