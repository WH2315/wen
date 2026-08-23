#include "ui/panels/hierarchy_panel.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/selection.hpp"
#include "ui/editor_scene.hpp"
#include "ui/prefab_actions.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/game_object.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include <cstdio>
#include <cstring>
#include <functional>

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

    // 增删/复制/建 prefab/建子对象/重设父推迟到遍历结束后执行,避免在遍历时修改树。
    GameObjectUUID pending_delete = kInvalidGameObjectUUID;
    GameObjectUUID pending_duplicate = kInvalidGameObjectUUID;
    GameObjectUUID pending_prefab = kInvalidGameObjectUUID;
    GameObjectUUID pending_create_child = kInvalidGameObjectUUID;
    bool pending_create = false;
    struct ReparentRequest {
        GameObjectUUID child;
        GameObjectUUID parent;
    };
    std::vector<ReparentRequest> pending_reparents;

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

    constexpr const char* kGoPayload = "WEN_GO_UUID";

    if (scene) {
        // 递归绘制一个对象节点(含选中/内联重命名/右键菜单/拖拽源与拖放目标)。
        std::function<void(GameObject*)> drawNode = [&](GameObject* game_object) {
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
                bool leaf = game_object->getChildren().empty();
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                           ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                if (leaf) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                bool open = ImGui::TreeNodeEx(game_object->getName().c_str(), flags);

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
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
                    if (ImGui::MenuItem("Create Child")) {
                        pending_create_child = uuid;
                    }
                    if (ImGui::MenuItem("Unparent", nullptr, false,
                                       game_object->getParent() != nullptr)) {
                        // 脱离父(设为根),保持世界位置不变。与拖到空白处同路径。
                        pending_reparents.push_back({uuid, kInvalidGameObjectUUID});
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

                // 拖拽源:拖动本对象。
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload(kGoPayload, &uuid, sizeof(uuid));
                    ImGui::Text("%s", game_object->getName().c_str());
                    ImGui::EndDragDropSource();
                }
                // 拖放目标:放到本对象行上 -> 成为其子对象。
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kGoPayload)) {
                        if (payload->DataSize == sizeof(GameObjectUUID)) {
                            GameObjectUUID dragged;
                            std::memcpy(&dragged, payload->Data, sizeof(dragged));
                            if (dragged != uuid) {
                                pending_reparents.push_back({dragged, uuid});
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (!leaf && open) {
                    for (auto* child : game_object->getChildren()) {
                        drawNode(child);
                    }
                    ImGui::TreePop();
                }
            }

            ImGui::PopID();
        };

        for (auto* game_object : scene->getGameObjects()) {
            if (game_object->getParent() == nullptr) {
                drawNode(game_object);
            }
        }

        // 放到窗口空白处 -> 设为根(脱离父)。
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kGoPayload)) {
                if (payload->DataSize == sizeof(GameObjectUUID)) {
                    GameObjectUUID dragged;
                    std::memcpy(&dragged, payload->Data, sizeof(dragged));
                    pending_reparents.push_back({dragged, kInvalidGameObjectUUID});
                }
            }
            ImGui::EndDragDropTarget();
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

    if (pending_create_child != kInvalidGameObjectUUID) {
        if (auto* parent = scene->getGameObject(pending_create_child)) {
            if (auto* game_object = createChildGameObject(parent)) {
                selectGameObject(game_object->getUUID());
                beginRename(game_object);
                pushGameObjectCreated(game_object);
            }
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

    // 重设父:保持世界位置不变,仅改变所在层级。
    for (const auto& req : pending_reparents) {
        auto* child = scene->getGameObject(req.child);
        if (child == nullptr) {
            continue;
        }
        GameObject* new_parent = (req.parent == kInvalidGameObjectUUID)
                                     ? nullptr
                                     : scene->getGameObject(req.parent);
        if (new_parent == child || new_parent == child->getParent()) {
            continue;
        }
        // 不能拖到自己的后代下(成环)。
        if (new_parent != nullptr && new_parent->isDescendantOf(child)) {
            WEN_CLIENT_INFO("Hierarchy: cannot reparent \"{}\" under its own descendant.", child->getName())
            continue;
        }
        auto* transform = child->queryComponent<TransformComponent>();
        if (transform == nullptr) {
            continue;
        }
        glm::mat4 world = transform->getWorldMatrix();
        auto before_parent = child->getParent() ? child->getParent()->getUUID() : kInvalidGameObjectUUID;
        child->setParent(new_parent);
        transform->setFromWorldMatrix(world);  // 保持世界位置
        transform->propagateWorldChange();
        pushReparentGameObject(child->getUUID(), before_parent,
                               new_parent ? new_parent->getUUID() : kInvalidGameObjectUUID, world);
    }
}

void HierarchyPanel::selectGameObject(GameObjectUUID uuid) {
    setSelectedGameObject(uuid);
}

}  // namespace wen::editor
