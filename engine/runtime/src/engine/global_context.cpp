#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_system.initialize(LogLevel::trace, LogLevel::trace);
    window_system.initialize(WindowInfo("wen 16 : 9", 1600, 900));
    event_system.initialize();
    input_system.initialize();
    timer_system.initialize();
    reflect_system.initialize();
    render_system.initialize(Renderer::Configuration{.debug = true});
    game_object_uuid_allocator.initialize();
    component_type_uuid_system.initialize();
    scene_manager.initialize();
}

void GlobalContext::shutdown() {
    scene_manager.destroy();
    component_type_uuid_system.destroy();
    game_object_uuid_allocator.destroy();
    render_system.destroy();
    reflect_system.destroy();
    timer_system.destroy();
    input_system.destroy();
    event_system.destroy();
    window_system.destroy();
    log_system.destroy();
}

}  // namespace wen