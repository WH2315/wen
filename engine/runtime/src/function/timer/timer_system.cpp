#include "function/timer/timer_system.hpp"

namespace wen {

TimerSystem::TimerSystem() = default;

std::shared_ptr<Timer> TimerSystem::registerTimer(float time_rate) {
    timers_.push_back(std::make_shared<Timer>(time_rate));
    return timers_.back();
}

void TimerSystem::stop() {
    for (auto timer : timers_) {
        timer->stop();
    }
}

void TimerSystem::reset() {
    for (auto timer : timers_) {
        timer->reset();
    }
}

TimerSystem::~TimerSystem() { timers_.clear(); }

}  // namespace wen