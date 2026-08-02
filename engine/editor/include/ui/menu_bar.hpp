#pragma once

namespace wen::editor {

class MenuBar {
public:
    void render();

private:
    void handleShortcuts();
    void renderFileMenu();
    void renderEditMenu();
};

}  // namespace wen::editor
