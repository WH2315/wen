#pragma once

namespace wen::editor {

// 顶部工具栏:Play / Pause / Step 按钮,切换编辑/播放模式。
class Toolbar {
public:
    void render();

    static float height();  // 本帧工具栏占用的高度(供 Dockspace 布局用)
};

}  // namespace wen::editor
