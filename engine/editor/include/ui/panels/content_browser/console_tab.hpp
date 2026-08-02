#pragma once

namespace wen::editor {

class ConsoleTab {
public:
    ConsoleTab();  // 把捕获 sink 挂到引擎日志器上

    void render();

private:
    bool auto_scroll_ = true;
};

}  // namespace wen::editor
