#include "ui/menu_bar.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

namespace {

void togglePlayMode() {
    bool is_edit = (global_ui_context->mode == Mode::eEdit);
    if (global_ui_context->change_mode_callback) {
        global_ui_context->change_mode_callback(is_edit ? Mode::eGame : Mode::eEdit);
    }
}

void quitApplication() {
    auto* window = global_context->window_system->getRuntimeWindow();
    glfwSetWindowShouldClose(window->getWindow(), GLFW_TRUE);
}

}  // namespace

void MenuBar::render() {
    handleShortcuts();
    if (ImGui::BeginMainMenuBar()) {
        renderFileMenu();
        renderEditMenu();
        ImGui::EndMainMenuBar();
    }
}

void MenuBar::handleShortcuts() {
    if (ImGui::GetIO().WantTextInput) {
        return;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_P)) {
        togglePlayMode();
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Q)) {
        quitApplication();
    }
}

void MenuBar::renderFileMenu() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            // TODO: 新建场景
        }
        if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
            // TODO: 打开场景文件
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            // TODO: 保存当前场景
        }
        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
            // TODO: 场景另存为
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
            quitApplication();
        }
        ImGui::EndMenu();
    }
}

void MenuBar::renderEditMenu() {
    if (ImGui::BeginMenu("Edit")) {
        bool is_edit = (global_ui_context->mode == Mode::eEdit);
        if (ImGui::MenuItem(is_edit ? "Play" : "Stop", "Ctrl+P")) {
            togglePlayMode();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Frame Selected", "F")) {
            // TODO: 触发框选当前选中的游戏对象
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Preferences...", nullptr)) {
            // TODO: 打开偏好设置面板
        }
        ImGui::EndMenu();
    }
}

}  // namespace wen::editor
