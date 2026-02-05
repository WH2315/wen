#pragma once

#include <chrono>

namespace wen {

class Timer final {
public:
    Timer(float time_rate);
    ~Timer() = default;

    float tick(std::chrono::milliseconds dt_ms = std::chrono::milliseconds(0));

    void stop() { stopped_ = true; }
    bool stopped() const { return stopped_; }
    void reset();

private:
    float time_rate_;
    std::chrono::high_resolution_clock clock_;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time_;
    bool stopped_;
};

}  // namespace wen