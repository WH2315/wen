#include <wen.hpp>
#include <pch.hpp>
#include "editor.hpp"

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    auto scene = global_context->scene_manager->createScene("First Scene");
    global_context->asset_system->setRootDir("engine/assets");

    // auto dragon_mesh_id = global_context->asset_system->loadMesh("dragon_lods.obj");

    auto camera = scene->createGameObject("camera");
    camera->addComponent(new CameraControllerComponent);
    auto transform = new TransformComponent;
    transform->location = {0, 0, -5};
    camera->addComponent(transform);
    camera->addComponent(new PerspectiveCameraComponent(60, 1920.0f, 1080.0f, 0.1, 1000));
    int n = 4, n2 = n / 2;
    // for (int i = 0; i < n * n * n; i++) {
    //     auto dragon = scene->createGameObject("dragon_" + std::to_string(i));
    //     auto t = new TransformComponent;
    //     t->location = {i % n - n2, (i / n) % n - n2, ((i / n) / n) % n - n2};
    //     dragon->addComponent(t);
    //     dragon->addComponent(new MeshComponent(dragon_mesh_id));
    // }

    // Wrap the engine in the editor (overlays ImGui editor panels on the scene).
    editor::Editor game_editor(engine.get());
    game_editor.initialize();
    game_editor.run();
    game_editor.destroy();

    engine->shutdownEngine();

    return 0;
}
