#include <wen.hpp>
#include <pch.hpp>

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    auto scene = global_context->scene_manager->createScene("First Scene");
    global_context->asset_system->setRootDir("sandbox/resources");

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}