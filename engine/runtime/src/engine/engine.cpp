#include "engine/engine.hpp"
#include "core/base/macro.hpp"

namespace wen {

void Engine::startupEngine() {
    global_context = new GlobalContext;
    global_context->startup();
    global_context->render_system->createRenderer();
    prepareTimer();
    WEN_CORE_INFO("engine startup.")
}

void Engine::shutdownEngine() {
    WEN_CORE_INFO("engine shutdown.")
    global_context->render_system->destroyRenderer();
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
    // fixed_tick_thread_->join();
    // fixed_tick_thread_.reset();
}

void Engine::prepareTimer() {
    delta_time_ = 0.03f;
    main_timer_ = global_context->timer_system->registerTimer();
    fixed_timer_ = global_context->timer_system->registerTimer();
    benchmark_timer_ = global_context->timer_system->registerTimer();
    stopTimer();
    // fixed_tick_thread_ = std::make_unique<std::thread>([this]() {
    //     while (!global_context->window_system->shouldClose()) {
    //         if ((!fixed_timer_->stopped()) && (!main_timer_->stopped())) {
    //             // global_context->scene_manager->fixedTick();
    //         }
    //         constexpr float fixed_tick_delta_time = 1.0 / 30.0;
    //         fixed_timer_->tick(std::chrono::milliseconds(static_cast<int>(fixed_tick_delta_time * 1000)));
    //     }
    // });
}

void Engine::startTimer() {
    global_context->scene_manager->start();

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
        global_context->scene_manager->swap();
    }
    benchmark_timer_->tick();
    if (!main_timer_->stopped()) {
        global_context->scene_manager->tick(delta_time_);
    }
    float benchmark_dt = benchmark_timer_->tick();
    WEN_CORE_DEBUG("BENCHMARK: Logic Delta Time(ms): {}", benchmark_dt * 1000)
}

void Engine::tickRender() {
    benchmark_timer_->tick();
    global_context->render_system->render(); 
    float benchmark_dt = benchmark_timer_->tick();
    WEN_CORE_DEBUG("BENCHMARK: Render CPU Delta Time(ms): {}", benchmark_dt * 1000)
    delta_time_ = main_timer_->tick();
    global_context->input_system->tick();
}

}  // namespace wen