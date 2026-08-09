#pragma once

#include "function/framework/uuid_manager.hpp"

namespace wen::editor {

// 统一选中通道:设置/清除当前选中的游戏对象,并同步驱动渲染侧选中描边。
// 面板、撤销系统、视口拾取等一律经此设置选中。
void setSelectedGameObject(GameObjectUUID uuid);

}  // namespace wen::editor
