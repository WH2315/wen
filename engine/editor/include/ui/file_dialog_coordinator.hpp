#pragma once

namespace wen::editor {

class SceneFileActions;
struct PendingOperation;

// 场景文件对话框协调者:桥接 FileDialog(纯 UI)与 SceneFileActions,
// 负责打开对话框、每帧渲染,并把确认结果翻译成场景操作请求。
// 脏场景切换/退出时渲染"保存更改?"确认框。
class FileDialogCoordinator {
public:
    void render();

    // Ctrl+O / "Open Scene..."。
    void openOpenSceneDialog();

    // Ctrl+S / "Save Scene"/"Save Scene As..."。
    void saveScene(bool force_save_as);

private:
    void renderConfirmDialog(SceneFileActions& sfa);
    void renderFileDialog(SceneFileActions& sfa);
    void completePendingOperation(SceneFileActions& sfa, PendingOperation op);

    // 待确认操作通过"另存为"完成时,等保存就位(current path 非空)后收尾。
    bool pending_complete_after_save_ = false;
};

}  // namespace wen::editor
