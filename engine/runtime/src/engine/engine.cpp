#include "engine/engine.hpp"
#include "core/base/macro.hpp"

namespace wen {

void Engine::startupEngine() {
    global_context = new GlobalContext;
    global_context->startup();
    WEN_CORE_INFO("engine start.")
}

void Engine::shutdownEngine() {
    WEN_CORE_INFO("engine shutdown.")
    global_context->shutdown();
    delete global_context;
    global_context = nullptr;
}

void Engine::runEngine() {}

}  // namespace wen