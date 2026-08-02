#pragma once

#include "engine/engine.hpp"
#include "ui/ui.hpp"

namespace wen::editor {

// 用法:
//     auto engine = std::make_unique<wen::Engine>();
//     engine->startupEngine();
//     .. 构建场景 ...
//     wen::editor::Editor editor(engine.get());
//     editor.initialize();
//     editor.run();
//     editor.destroy();
//     engine->shutdownEngine();
class Editor {
public:
    Editor(Engine* engine);
    ~Editor();

    void initialize();
    void run();
    void destroy();

private:
    Engine* engine_;
    std::unique_ptr<UI> ui_;
};

}  // namespace wen::editor
