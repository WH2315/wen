#pragma once

#include "ui/file_dialog_coordinator.hpp"

namespace wen::editor {

// 主菜单栏:File/Edit 菜单 + 全局快捷键。
class MenuBar {
public:
    void render();

private:
    void handleShortcuts();
    void renderFileMenu();
    void renderEditMenu();

    FileDialogCoordinator file_dialog_coordinator_;
};

}  // namespace wen::editor
