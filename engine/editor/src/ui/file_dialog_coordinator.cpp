#include "ui/file_dialog_coordinator.hpp"
#include "ui/ui_context.hpp"
#include "ui/file_dialog.hpp"
#include "engine/global_context.hpp"
#include <filesystem>

namespace wen::editor {

void FileDialogCoordinator::render() {
    auto* dialog = global_ui_context->file_dialog;
    if (dialog == nullptr) {
        return;
    }
    dialog->render();

    bool confirmed = false;
    std::filesystem::path path;
    if (!dialog->takeResult(confirmed, path) || !confirmed) {
        return;
    }
    if (dialog->mode() == FileDialog::Mode::eOpen) {
        global_ui_context->scene_file_actions.requestOpen(path);
    } else {
        global_ui_context->scene_file_actions.requestSaveAs(path);
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
