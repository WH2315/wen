#pragma once

#include "ui/panel.hpp"
#include "ui/panels/inspector/component_ui/component_ui_manager.hpp"

namespace wen::editor {

class InspectorPanel : public Panel {
public:
    void onUnloadScene() override;
    void render() override;

private:
    ComponentUIManager component_ui_manager_;
};

}  // namespace wen::editor
