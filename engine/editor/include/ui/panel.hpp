#pragma once

#include <imgui.h>

namespace wen::editor {

// 可停靠编辑器面板(Hierarchy、Viewport、Inspector 等)的基类
class Panel {
public:
    Panel() = default;
    virtual ~Panel() = default;

    virtual void onLoadScene() {}
    virtual void onUnloadScene() {}
    virtual void render() = 0;
};

}  // namespace wen::editor
