#include "engine/engine.hpp"
#include "core/base/macro.hpp"

namespace wen {

void Engine::startupEngine() {
    global_context = new GlobalContext;
    global_context->startup();
    WEN_CORE_INFO("engine startup.")
}

void Engine::shutdownEngine() {
    WEN_CORE_INFO("engine shutdown.")
    global_context->shutdown();
    delete global_context;
    global_context = nullptr;
}

void Engine::runEngine() {
    while (!global_context->window_system->shouldClose()) {
        global_context->window_system->pollEvents();
    }
}

}  // namespace wen