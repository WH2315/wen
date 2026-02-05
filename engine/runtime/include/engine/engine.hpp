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

    void prepareTimer();
    void startTimer();
    void stopTimer();

protected:
    void tickOneFrame();
    void tickLogic();
    void tickRender();

private:
    float delta_time_;
    std::shared_ptr<Timer> main_timer_;
    std::shared_ptr<Timer> fixed_timer_;
    std::shared_ptr<Timer> benchmark_timer_;
    std::unique_ptr<std::thread> fixed_tick_thread_;
};

}  // namespace wen