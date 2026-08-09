#pragma once

namespace wen::editor {

// 引擎日志控制台:构造时把 spdlog sink 挂到引擎日志器,渲染捕获的日志。
class ConsoleTab {
public:
    ConsoleTab();

    void render();

private:
    bool auto_scroll_ = true;
};

}  // namespace wen::editor
