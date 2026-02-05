#include "function/timer/timer.hpp"
#include <thread>

namespace wen {

Timer::Timer(float time_rate) : time_rate_(time_rate) {
    last_time_ = clock_.now();
    stopped_ = false;
}

float Timer::tick(std::chrono::milliseconds dt_ms) {
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
                  clock_.now() - last_time_)
                  .count();
    auto wait =
        std::chrono::duration_cast<std::chrono::microseconds>(dt_ms).count() -
        dt;

    if (wait > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(wait));
        last_time_ = clock_.now();
        return time_rate_ * static_cast<float>(dt_ms.count()) * 0.001f;
    }
    last_time_ = clock_.now();
    return time_rate_ * static_cast<float>(dt) * 0.000001f;
}

void Timer::reset() {
    last_time_ = clock_.now();
    stopped_ = false;
}

}  // namespace wen