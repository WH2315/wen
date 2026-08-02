#pragma once

#include "ui/panel.hpp"
#include "ui/panels/content_browser/project_tab.hpp"
#include "ui/panels/content_browser/console_tab.hpp"

namespace wen::editor {

class ContentBrowserPanel : public Panel {
public:
    void render() override;

    static void registerIniSettings() { ProjectTab::registerIniSettings(); }

    void setOnSelectGameObject(const std::function<void(GameObjectUUID uuid)>& callback) {
        project_.setOnSelectGameObject(callback);
    }

private:
    ProjectTab project_;
    ConsoleTab console_;
};

}  // namespace wen::editor
