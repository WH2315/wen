#pragma once

#include "ui/panel.hpp"
#include "function/framework/uuid_manager.hpp"

namespace wen::editor {

class HierarchyPanel : public Panel {
public:
    void selectGameObject(GameObjectUUID uuid = GameObjectUUID(-1));

    void onLoadScene() override;
    void render() override;
};

}  // namespace wen::editor
