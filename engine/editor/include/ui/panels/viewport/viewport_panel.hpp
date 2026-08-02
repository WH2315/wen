#pragma once

#include "ui/panel.hpp"
#include "function/framework/uuid_manager.hpp"

namespace wen::editor {

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
};

}  // namespace wen::editor
