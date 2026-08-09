#pragma once

#include "ui/panel.hpp"
#include "ui/component_ui/component_ui_manager.hpp"

namespace wen::editor {

// 检查器:展示选中对象的组件,经 ComponentUIManager 渲染可编辑控件。
class InspectorPanel : public Panel {
public:
    void onUnloadScene() override;
    void render() override;

private:
    ComponentUIManager component_ui_manager_;
};

}  // namespace wen::editor
