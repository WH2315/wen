#include "ui/panels/content_browser/content_browser_panel.hpp"
#include "ui/icons.hpp"

namespace wen::editor {

namespace {
constexpr float kBrowserFontSize = 26.0f;
}  // namespace

void ContentBrowserPanel::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::Begin("Content Browser");
    ImGui::PopStyleVar();
    ImGui::PushFont(nullptr, kBrowserFontSize);

    // Project(资源浏览)和 Console(引擎日志)
    if (ImGui::BeginTabBar("##browser_tabs")) {
        auto project_label = std::string(icons::kFolder) + " Project";
        auto console_label = std::string(icons::kTerminal) + " Console";
        if (ImGui::BeginTabItem(project_label.c_str())) {
            project_.render();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(console_label.c_str())) {
            console_.render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::PopFont();
    ImGui::End();
}

}  // namespace wen::editor
