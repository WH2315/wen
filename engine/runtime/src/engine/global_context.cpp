#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_system.initialize(LogLevel::trace, LogLevel::trace);
    window_system.initialize(WindowInfo("wen", 1920, 1080));
    event_system.initialize();
    input_system.initialize();
    timer_system.initialize();
    reflect_system.initialize();
    component_factory.initialize();
    reflect_system->registerReflectProperties();
    render_system.initialize(Renderer::Configuration{.debug = true});
    game_object_uuid_allocator.initialize();
    component_type_uuid_system.initialize();
    asset_system.initialize();
    asset_system->setRootDir("engine/assets");
    camera_system.initialize();
    light_system.initialize();
    environment_system.initialize();
    script_registry.initialize();
    physics_system.initialize();
    scene_manager.initialize();
    render_system->createRenderer();
}

void GlobalContext::shutdown() {
    render_system->waitIdle();
    scene_manager.destroy();
    physics_system.destroy();
    camera_system.destroy();
    light_system.destroy();
    environment_system.destroy();
    script_registry.destroy();
    asset_system.destroy();
    component_type_uuid_system.destroy();
    game_object_uuid_allocator.destroy();
    render_system->destroyRenderer();
    render_system.destroy();
    component_factory.destroy();
    reflect_system.destroy();
    timer_system.destroy();
    input_system.destroy();
    event_system.destroy();
    window_system.destroy();
    log_system.destroy();
}

}  // namespace wen