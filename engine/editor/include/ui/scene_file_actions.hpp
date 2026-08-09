#pragma once

#include <filesystem>

namespace wen::editor {

enum class SceneFileAction {
    eNone,
    eNew,
    eOpen,
    eSave,
    eSaveAs,
};

// 场景文件操作的"请求-执行"契约。UI 侧经 request* 发起请求,
// Editor 主循环在 ImGui 帧外 consumePending() 执行。
class SceneFileActions {
public:
    struct Pending {
        SceneFileAction action{SceneFileAction::eNone};
        std::filesystem::path file;  // eOpen/eSaveAs 的目标文件
    };

    void requestNew();
    void requestOpen(const std::filesystem::path& file);
    void requestSave();  // 写回当前场景路径
    void requestSaveAs(const std::filesystem::path& file);

    Pending consumePending();

    // 当前场景的来源/保存路径,决定 Ctrl+S 是直接保存还是走"另存为"。
    const std::filesystem::path& currentScenePath() const { return current_scene_path_; }
    void setCurrentScenePath(const std::filesystem::path& path) { current_scene_path_ = path; }

private:
    Pending pending_;
    std::filesystem::path current_scene_path_;
};

}  // namespace wen::editor
