#include "ui/toolbar.hpp"
#include "ui/ui_context.hpp"
#include "ui/icons.hpp"

namespace wen::editor {

namespace {

constexpr float kVerticalPadding = 2.0f;
constexpr float kButtonWidth = 36.0f;

bool toolbarButton(const char* icon, const char* tooltip, bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    bool clicked = ImGui::Button(icon, ImVec2(kButtonWidth, 0.0f));
    if (active) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", tooltip);
    return clicked;
}

}  // namespace

float Toolbar::height() {
    return ImGui::GetFrameHeight() + 2.0f * kVerticalPadding;
}

void Toolbar::render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, height()));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, kVerticalPadding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    ImGui::Begin("EditorToolbar", nullptr, window_flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);

    const bool playing = (global_ui_context->mode == Mode::eGame);

    // 把 Play / Pause / Step 按钮组居中
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float total_width = kButtonWidth * 3.0f + spacing * 2.0f;
    ImGui::SetCursorPosX(std::max((ImGui::GetWindowWidth() - total_width) * 0.5f, spacing));

    const std::string play_label =
        std::string(playing ? icons::kStop : icons::kPlay) + "###PlayStop";
    if (toolbarButton(play_label.c_str(), playing ? "Stop (Ctrl+P)" : "Play (Ctrl+P)", playing)) {
        if (global_ui_context->change_mode_callback) {
            global_ui_context->change_mode_callback(playing ? Mode::eEdit : Mode::eGame);
        }
        global_ui_context->step_one_frame = false;
    }

    ImGui::SameLine();
    // Edit 模式下也可以预先按下暂停，下次播放从暂停开始
    if (toolbarButton(icons::kPause, "Pause", global_ui_context->game_paused)) {
        global_ui_context->game_paused = !global_ui_context->game_paused;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!playing);
    if (toolbarButton(icons::kStepForward, "Step one frame", false)) {
        global_ui_context->game_paused = true;
        global_ui_context->step_one_frame = true;
    }
    ImGui::EndDisabled();

    ImGui::End();
}

}  // namespace wen::editor
