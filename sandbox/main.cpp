#include <wen.hpp>
#include <pch.hpp>
#include "editor.hpp"

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    // 编辑器启动时自动创建未保存的默认场景(Main Camera + Directional Light)。
    editor::Editor game_editor(engine.get());
    game_editor.initialize();
    game_editor.run();
    game_editor.destroy();

    engine->shutdownEngine();

    return 0;
}
