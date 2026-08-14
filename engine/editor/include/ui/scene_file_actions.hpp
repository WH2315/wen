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

// 脏场景下需要用户确认后才能继续的操作(切换场景/退出)。
enum class PendingOperationKind {
    eNone,
    eNew,
    eOpen,
    eQuit,
};

struct PendingOperation {
    PendingOperationKind kind{PendingOperationKind::eNone};
    std::filesystem::path file;  // eOpen 的目标文件
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

    // 场景是否含未保存的修改(编辑操作置脏,保存/新建/打开清脏)。
    bool isDirty() const { return dirty_; }
    void markDirty() { dirty_ = true; }
    void clearDirty() { dirty_ = false; }

    // 脏场景下挂起的待确认操作(由 Editor/菜单发起,协调器渲染确认框)。
    const PendingOperation& pendingOperation() const { return pending_operation_; }
    void setPendingOperation(const PendingOperation& op) { pending_operation_ = op; }
    void clearPendingOperation() { pending_operation_ = {}; }

private:
    Pending pending_;
    std::filesystem::path current_scene_path_;
    bool dirty_ = false;
    PendingOperation pending_operation_;
};

}  // namespace wen::editor
