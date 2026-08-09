#pragma once

namespace wen::editor {

// 场景文件对话框协调者:桥接 FileDialog(纯 UI)与 SceneFileActions,
// 负责打开对话框、每帧渲染,并把确认结果翻译成场景操作请求。
class FileDialogCoordinator {
public:
    void render();

    // Ctrl+O / "Open Scene..."。
    void openOpenSceneDialog();

    // Ctrl+S / "Save Scene"/"Save Scene As..."。
    void saveScene(bool force_save_as);
};

}  // namespace wen::editor
