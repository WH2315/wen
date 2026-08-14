#include "ui/panels/hierarchy_panel.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/selection.hpp"
#include "ui/editor_scene.hpp"
#include "ui/prefab_actions.hpp"
#include "engine/global_context.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/game_object.hpp"
#include <cstdio>

namespace wen::editor {

void HierarchyPanel::onLoadScene() {
    renaming_uuid_ = kInvalidGameObjectUUID;
    selectGameObject();
}

void HierarchyPanel::beginRename(GameObject* game_object) {
    renaming_uuid_ = game_object->getUUID();
    std::snprintf(rename_buffer_, sizeof(rename_buffer_), "%s", game_object->getName().c_str());
    rename_focus_requested_ = true;
}

void HierarchyPanel::render() {
    ImGui::Begin("Hierarchy");

    auto* scene = global_context->scene_manager->getActiveScene();
    auto selected_uuid = global_ui_context->selected_game_object_uuid;

    // 增删/复制/建 prefab 推迟到遍历结束后执行,避免在遍历 game_objects_ 时修改它。
    GameObjectUUID pending_delete = kInvalidGameObjectUUID;
    GameObjectUUID pending_duplicate = kInvalidGameObjectUUID;
    GameObjectUUID pending_prefab = kInvalidGameObjectUUID;
    bool pending_create = false;

    if (scene && ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput &&
        selected_uuid != kInvalidGameObjectUUID) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            pending_delete = selected_uuid;
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_D)) {
            pending_duplicate = selected_uuid;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            if (auto* game_object = scene->getGameObject(selected_uuid)) {
                beginRename(game_object);
            }
        }
    }

    if (scene) {
        for (auto* game_object : scene->getGameObjects()) {
            auto uuid = game_object->getUUID();
            bool selected = (selected_uuid == uuid);

            ImGui::PushID(static_cast<int>(uuid));

            if (renaming_uuid_ == uuid) {
                // 内联重命名:回车提交,Esc 取消,失焦提交。
                if (rename_focus_requested_) {
                    ImGui::SetKeyboardFocusHere();
                    rename_focus_requested_ = false;
                }
                ImGui::SetNextItemWidth(-FLT_MIN);
                bool committed = ImGui::InputText("##rename", rename_buffer_, sizeof(rename_buffer_),
                                                  ImGuiInputTextFlags_EnterReturnsTrue |
                                                  ImGuiInputTextFlags_AutoSelectAll);
                if (committed || ImGui::IsItemDeactivated()) {
                    if (!ImGui::IsKeyPressed(ImGuiKey_Escape) && rename_buffer_[0] != '\0') {
                        auto old_name = game_object->getName();
                        if (old_name != rename_buffer_) {
                            game_object->setName(rename_buffer_);
                            pushGameObjectRenamed(uuid, old_name, rename_buffer_);
                        }
                    }
                    renaming_uuid_ = kInvalidGameObjectUUID;
                }
            } else {
                if (ImGui::Selectable(game_object->getName().c_str(), selected)) {
                    selectGameObject(selected ? kInvalidGameObjectUUID : uuid);
                }

                if (ImGui::BeginPopupContextItem("##go_context")) {
                    if (ImGui::IsWindowAppearing() && !selected) {
                        selectGameObject(uuid);
                    }
                    if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                        pending_duplicate = uuid;
                    }
                    if (ImGui::MenuItem("Rename", "F2")) {
                        beginRename(game_object);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Create Prefab")) {
                        pending_prefab = uuid;
                    }
                    if (ImGui::MenuItem("Delete", "Del")) {
                        pending_delete = uuid;
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Create Empty")) {
                        pending_create = true;
                    }
                    ImGui::EndPopup();
                }
            }

            ImGui::PopID();
        }

        if (ImGui::BeginPopupContextWindow("##hierarchy_context",
                                           ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Empty")) {
                pending_create = true;
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();

    if (scene == nullptr) {
        return;
    }

    if (pending_create) {
        if (auto* game_object = createEmptyGameObject()) {
            selectGameObject(game_object->getUUID());
            beginRename(game_object);
            pushGameObjectCreated(game_object);
        }
    }

    if (pending_duplicate != kInvalidGameObjectUUID) {
        if (auto* source = scene->getGameObject(pending_duplicate)) {
            if (auto* clone = duplicateGameObject(source)) {
                selectGameObject(clone->getUUID());
                pushGameObjectCreated(clone);
            }
        }
    }

    if (pending_prefab != kInvalidGameObjectUUID) {
        if (auto* game_object = scene->getGameObject(pending_prefab)) {
            // 存成 <root>/prefabs/<对象名>.prefab(相对路径,重名自动加 _2/_3)。
            createPrefab(game_object, game_object->getName() + ".prefab");
        }
    }

    if (pending_delete != kInvalidGameObjectUUID) {
        if (auto* game_object = scene->getGameObject(pending_delete)) {
            pushGameObjectDeleted(game_object);
            if (global_ui_context->selected_game_object_uuid == pending_delete) {
                selectGameObject();
            }
            if (renaming_uuid_ == pending_delete) {
                renaming_uuid_ = kInvalidGameObjectUUID;
            }
            removeGameObject(game_object);

            selectGameObject(global_ui_context->selected_game_object_uuid);
        }
    }
}

void HierarchyPanel::selectGameObject(GameObjectUUID uuid) {
    setSelectedGameObject(uuid);
}

}  // namespace wen::editor
