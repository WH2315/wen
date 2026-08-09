#pragma once

#include "engine/engine.hpp"
#include "ui/ui.hpp"

namespace wen::editor {

// 编辑器应用控制器:持有 Engine + UI,运行主循环;场景文件操作与
// Play 模式还原在 ImGui 帧外执行(执行中会销毁/重建场景)。
class Editor {
public:
    Editor(Engine* engine);
    ~Editor();

    void initialize();
    void run();
    void destroy();

private:
    // 执行菜单/浏览器请求的场景文件操作(New/Open/Save/SaveAs)。
    void processSceneFileAction();
    // Stop 时还原进入 Play 前的场景快照。
    void processPlayModeRestore();

    Engine* engine_;
    std::unique_ptr<UI> ui_;

    std::string play_mode_snapshot_;
    bool restore_scene_pending_ = false;
};

}  // namespace wen::editor
