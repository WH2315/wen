#pragma once

#include "ui/panel.hpp"
#include "ui/panels/content_browser/project_tab.hpp"
#include "ui/panels/content_browser/console_tab.hpp"

namespace wen::editor {

// 内容浏览器:组合 Project(资源浏览)与 Console(日志)两个子标签。
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
