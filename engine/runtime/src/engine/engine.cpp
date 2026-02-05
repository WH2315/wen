#include "engine/engine.hpp"
#include "core/base/macro.hpp"

namespace wen {

void Engine::startupEngine() {
    global_context = new GlobalContext;
    global_context->startup();
    prepareTimer();
    WEN_CORE_INFO("engine startup.")
}

void Engine::shutdownEngine() {
    WEN_CORE_INFO("engine shutdown.")
    global_context->shutdown();
    delete global_context;
    global_context = nullptr;
}

void Engine::runEngine() {
    startTimer();
    while (!global_context->window_system->shouldClose()) {
        global_context->window_system->pollEvents();
        tickOneFrame();
    }
}

void Engine::prepareTimer() {
    delta_time_ = 0.03f;
    main_timer_ = global_context->timer_system->registerTimer();
    fixed_timer_ = global_context->timer_system->registerTimer();
    benchmark_timer_ = global_context->timer_system->registerTimer();
    stopTimer();
}

void Engine::startTimer() {
    main_timer_->reset();
    fixed_timer_->reset();
    main_timer_->tick();
    fixed_timer_->tick();
}

void Engine::stopTimer() {
    main_timer_->stop();
    fixed_timer_->stop();
}

void Engine::tickOneFrame() {
    tickLogic();
    tickRender();
}

void Engine::tickLogic() {
    if (!main_timer_->stopped()) {
        // 
    }
    benchmark_timer_->tick();
    if (!main_timer_->stopped()) {
        // 
    }
    global_context->input_system->tick();
}

void Engine::tickRender() {
    benchmark_timer_->tick();
    delta_time_ = main_timer_->tick();
}

}  // namespace wen