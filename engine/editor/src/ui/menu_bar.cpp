#include "ui/menu_bar.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "engine/global_context.hpp"
#include "function/window/window_system.hpp"
#include "function/window/window.hpp"
#include <GLFW/glfw3.h>

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
    file_dialog_coordinator_.render();
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
    if (global_ui_context->mode == Mode::eEdit) {
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N)) {
            global_ui_context->scene_file_actions.requestNew();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O)) {
            file_dialog_coordinator_.openOpenSceneDialog();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
            file_dialog_coordinator_.saveScene(false);
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
            file_dialog_coordinator_.saveScene(true);
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) {
            global_undo_stack->undo();
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y)) {
            global_undo_stack->redo();
        }
    }
}

void MenuBar::renderFileMenu() {
    if (ImGui::BeginMenu("File")) {
        bool is_edit = (global_ui_context->mode == Mode::eEdit);
        if (ImGui::MenuItem("New Scene", "Ctrl+N", false, is_edit)) {
            global_ui_context->scene_file_actions.requestNew();
        }
        if (ImGui::MenuItem("Open Scene...", "Ctrl+O", false, is_edit)) {
            file_dialog_coordinator_.openOpenSceneDialog();
        }
        ImGui::Separator();

        if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, is_edit)) {
            file_dialog_coordinator_.saveScene(false);
        }
        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S", false, is_edit)) {
            file_dialog_coordinator_.saveScene(true);
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
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, is_edit && global_undo_stack->canUndo())) {
            global_undo_stack->undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, is_edit && global_undo_stack->canRedo())) {
            global_undo_stack->redo();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(is_edit ? "Play" : "Stop", "Ctrl+P")) {
            togglePlayMode();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Frame Selected", "F")) {

        }
        ImGui::Separator();
        if (ImGui::MenuItem("Preferences...", nullptr)) {

        }
        ImGui::EndMenu();
    }
}

}  // namespace wen::editor
