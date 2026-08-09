#include "ui/scene_file_actions.hpp"

namespace wen::editor {

void SceneFileActions::requestNew() {
    pending_.action = SceneFileAction::eNew;
    pending_.file.clear();
}

void SceneFileActions::requestOpen(const std::filesystem::path& file) {
    pending_.action = SceneFileAction::eOpen;
    pending_.file = file;
}

void SceneFileActions::requestSave() {
    pending_.action = SceneFileAction::eSave;
    pending_.file.clear();
}

void SceneFileActions::requestSaveAs(const std::filesystem::path& file) {
    pending_.action = SceneFileAction::eSaveAs;
    pending_.file = file;
}

SceneFileActions::Pending SceneFileActions::consumePending() {
    Pending result = pending_;
    pending_.action = SceneFileAction::eNone;
    pending_.file.clear();
    return result;
}

}  // namespace wen::editor
