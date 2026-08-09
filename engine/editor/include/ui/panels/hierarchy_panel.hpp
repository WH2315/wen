#pragma once

#include "ui/panel.hpp"
#include "function/framework/uuid_manager.hpp"

namespace wen {
class GameObject;
}

namespace wen::editor {

// 场景对象列表:选中、内联重命名、复制/删除/创建;选中联动 Inspector、
// 描边与 gizmo。视口拾取与 Content Browser 生成的对象也统一经此选中。
class HierarchyPanel : public Panel {
public:
    void selectGameObject(GameObjectUUID uuid = GameObjectUUID(-1));

    void onLoadScene() override;
    void render() override;

private:
    void beginRename(GameObject* game_object);

    GameObjectUUID renaming_uuid_{GameObjectUUID(-1)};
    char rename_buffer_[128] = {};
    bool rename_focus_requested_ = false;
};

}  // namespace wen::editor
