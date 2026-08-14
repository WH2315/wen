#include "ui/file_dialog_coordinator.hpp"
#include "ui/ui_context.hpp"
#include "ui/file_dialog.hpp"
#include "ui/editor_scene.hpp"
#include "engine/global_context.hpp"
#include "function/window/window_system.hpp"
#include "function/window/window.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>

namespace wen::editor {

void FileDialogCoordinator::render() {
    auto& sfa = global_ui_context->scene_file_actions;
    const auto& op = sfa.pendingOperation();

    // 待确认操作正通过"另存为"完成:等保存(帧外执行)把 current path 写回后收尾。
    if (pending_complete_after_save_) {
        if (!sfa.currentScenePath().empty()) {
            pending_complete_after_save_ = false;
            sfa.clearDirty();
            completePendingOperation(sfa, op);
            return;
        }
        // 保存尚未完成:继续渲染另存为对话框。
        renderFileDialog(sfa);
        return;
    }

    if (op.kind != PendingOperationKind::eNone) {
        renderConfirmDialog(sfa);
        return;
    }

    renderFileDialog(sfa);
}

// "保存更改?"确认框:Save / Don't Save / Cancel。
void FileDialogCoordinator::renderConfirmDialog(SceneFileActions& sfa) {
    const auto& op = sfa.pendingOperation();
    if (!ImGui::IsPopupOpen("Save Changes")) {
        ImGui::OpenPopup("Save Changes");
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string scene_name = "current scene";
        if (auto* scene = global_context->scene_manager->getActiveScene()) {
            scene_name = scene->getName();
        }
        ImGui::TextUnformatted(("Save changes to \"" + scene_name + "\" before continuing?").c_str());
        ImGui::Spacing();

        bool has_path = !sfa.currentScenePath().empty();
        if (ImGui::Button(has_path ? "Save" : "Save As...", ImVec2(128, 0))) {
            if (has_path) {
                if (saveSceneTo(sfa.currentScenePath())) {
                    sfa.clearDirty();
                    completePendingOperation(sfa, op);
                }
            } else {
                // 未保存过:先走"另存为",完成后继续待确认操作。
                pending_complete_after_save_ = true;
                if (auto* dialog = global_ui_context->file_dialog) {
                    std::string default_name = "Untitled";
                    if (auto* scene = global_context->scene_manager->getActiveScene()) {
                        default_name = scene->getName();
                    }
                    dialog->setDefaultFileName(default_name);
                    dialog->open(FileDialog::Mode::eSave, "Save Scene As",
                                 std::filesystem::path("engine/assets/scenes"), ".scene");
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(128, 0))) {
            sfa.clearDirty();
            completePendingOperation(sfa, op);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110, 0))) {
            sfa.clearPendingOperation();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// 渲染文件对话框,并把用户确认结果翻译成场景操作请求。
void FileDialogCoordinator::renderFileDialog(SceneFileActions& sfa) {
    auto* dialog = global_ui_context->file_dialog;
    if (dialog == nullptr) {
        return;
    }
    dialog->render();

    bool confirmed = false;
    std::filesystem::path path;
    if (!dialog->takeResult(confirmed, path)) {
        return;
    }
    if (!confirmed) {
        // 取消;若正为待确认操作做"另存为",放弃该操作。
        if (pending_complete_after_save_) {
            pending_complete_after_save_ = false;
            sfa.clearPendingOperation();
        }
        return;
    }
    if (dialog->mode() == FileDialog::Mode::eOpen) {
        sfa.requestOpen(path);
    } else {
        sfa.requestSaveAs(path);
    }
}

void FileDialogCoordinator::completePendingOperation(SceneFileActions& sfa, PendingOperation op) {
    sfa.clearPendingOperation();
    switch (op.kind) {
        case PendingOperationKind::eNew:
            sfa.requestNew();
            break;
        case PendingOperationKind::eOpen:
            sfa.requestOpen(op.file);
            break;
        case PendingOperationKind::eQuit:
            if (auto* window = global_context->window_system->getRuntimeWindow()) {
                glfwSetWindowShouldClose(window->getWindow(), GLFW_TRUE);
            }
            break;
        case PendingOperationKind::eNone:
            break;
    }
}

void FileDialogCoordinator::openOpenSceneDialog() {
    auto* dialog = global_ui_context->file_dialog;
    if (dialog == nullptr || dialog->isOpen()) {
        return;
    }
    dialog->open(FileDialog::Mode::eOpen, "Open Scene",
                 std::filesystem::path("engine/assets/scenes"), ".scene");
}

void FileDialogCoordinator::saveScene(bool force_save_as) {
    auto& scene_file_actions = global_ui_context->scene_file_actions;
    if (!force_save_as && !scene_file_actions.currentScenePath().empty()) {
        scene_file_actions.requestSave();
        return;
    }

    auto* dialog = global_ui_context->file_dialog;
    if (dialog == nullptr || dialog->isOpen()) {
        return;
    }
    std::string default_name = "Untitled";
    if (auto* scene = global_context->scene_manager->getActiveScene()) {
        default_name = scene->getName();
    }
    dialog->setDefaultFileName(default_name);
    dialog->open(FileDialog::Mode::eSave, "Save Scene As",
                 std::filesystem::path("engine/assets/scenes"), ".scene");
}

}  // namespace wen::editor
