#pragma once

#include "ui/panel.hpp"
#include "ui/panels/viewport_camera.hpp"

namespace wen::editor {

class MenuBar;
class Toolbar;
class FileDialog;
class UndoStack;

// UI 组合根:持有全局状态、所有面板与菜单/工具栏/对话框服务,
// 渲染 Dockspace 宿主,并负责面板间的回调接线。
class UI {
public:
    UI();
    ~UI();

    void onLoadScene();
    void onUnloadScene();
    // Prefab 实例重建后广播给各面板(丢弃缓存的组件视图/指针)。
    void onPrefabReverted();

    void onFrame();
    void render();

    // 注册可停靠面板,按注册顺序渲染(默认面板已由构造函数注册)。
    void registerPanel(std::unique_ptr<Panel> panel);

    auto getViewportCameraID() const {
        return viewport_camera_->viewport_camera_id;
    }

private:
    void handleFocusShortcut();
    void updateWindowTitle();

    std::vector<std::unique_ptr<Panel>> panels_;
    std::unique_ptr<ViewportCamera> viewport_camera_;
    std::unique_ptr<MenuBar> menu_bar_;
    std::unique_ptr<Toolbar> toolbar_;
    std::unique_ptr<FileDialog> file_dialog_;
    std::unique_ptr<UndoStack> undo_stack_;

    std::string window_title_;  // 最近一次设置的窗口标题(脏标记变化时才更新)
};

}  // namespace wen::editor
