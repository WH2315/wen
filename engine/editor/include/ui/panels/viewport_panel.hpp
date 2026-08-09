#pragma once

#include "ui/panel.hpp"
#include "function/framework/uuid_manager.hpp"
#include <glm/glm.hpp>

namespace wen::editor {

// 场景视口:离屏图像、ImGuizmo 变换 Gizmo、鼠标拾取、网格拖放。
class ViewportPanel : public Panel {
public:
    void render() override;

    void setOnSelectGameObject(const std::function<void(GameObjectUUID uuid)>& callback) {
        on_select_game_object_ = callback;
    }

private:
    void renderToolbar(const ImVec2& image_pos);
    void handleToolShortcuts();
    void renderGizmo(const ImVec2& image_pos, const ImVec2& image_size);
    void handlePicking(const ImVec2& image_pos, const ImVec2& image_size);

    std::function<void(GameObjectUUID uuid)> on_select_game_object_;
    bool toolbar_hovered_ = false;

    bool gizmo_using_ = false;
    glm::vec3 gizmo_start_location_{0.0f};
    glm::vec3 gizmo_start_rotation_{0.0f};
    glm::vec3 gizmo_start_scale_{1.0f};
};

}  // namespace wen::editor
