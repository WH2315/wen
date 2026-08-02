#pragma once

#include "pch.hpp"
#include "engine/global_context.hpp"

namespace wen {

class Engine final {
public:
    Engine() = default;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    ~Engine() = default;

    void startupEngine();
    void shutdownEngine();

    void runEngine();

    // Phase API: runEngine() is implemented with these, and a host (e.g. the
    // editor) may drive its own loop instead of calling runEngine():
    //     while (engine->isAlive()) {
    //         engine->pollEvents();
    //         engine->tickLogic();  // host decides whether to run logic
    //         engine->tickRender();
    //     }
    //     engine->waitFixedTickThread();
    bool isAlive() const;
    void pollEvents();
    void tickLogic();
    void tickRender();
    void waitFixedTickThread();

    void prepareTimer();
    void startTimer();
    void stopTimer();

private:
    float delta_time_;
    std::shared_ptr<Timer> main_timer_;
    std::shared_ptr<Timer> fixed_timer_;
    std::shared_ptr<Timer> benchmark_timer_;
    std::unique_ptr<std::thread> fixed_tick_thread_;
};

}  // namespace wen