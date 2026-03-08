#include <wen.hpp>
#include <pch.hpp>

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    auto scene = global_context->scene_manager->createScene("First Scene");
    global_context->asset_system->setRootDir("sandbox/resources");

    auto dragon_mesh_id = global_context->asset_system->loadMesh("dragon_lods.obj");

    auto camera = scene->createGameObject("camera");
    camera->addComponent(new CameraControllerComponent);
    camera->addComponent(new TransformComponent);
    camera->addComponent(new CameraComponent);
    int n = 4, n2 = n / 2;
    for (int i = 0; i < n * n * n; i++) {
        auto dragon = scene->createGameObject("dragon");
        auto t = new TransformComponent;
        t->location = {i % n - n2, (i / n) % n - n2, ((i / n) / n) % n - n2};
        dragon->addComponent(t);
        dragon->addComponent(new MeshComponent(dragon_mesh_id));
    }

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}