#pragma once

#include "ui/panel.hpp"
#include "ui/panels/viewport/viewport_camera.hpp"

namespace wen::editor {

class MenuBar;
class Toolbar;

class UI {
public:
    UI();
    ~UI();

    void onLoadScene();
    void onUnloadScene();

    void onFrame();
    void render();

    auto getViewportCameraID() const {
        return viewport_camera_->viewport_camera_id;
    }

private:
    void handleFocusShortcut();

    std::vector<std::unique_ptr<Panel>> panels_;
    std::unique_ptr<ViewportCamera> viewport_camera_;
    std::unique_ptr<MenuBar> menu_bar_;
    std::unique_ptr<Toolbar> toolbar_;
};

}  // namespace wen::editor
