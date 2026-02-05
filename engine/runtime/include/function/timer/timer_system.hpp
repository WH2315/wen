#pragma once

#include "core/base/singleton.hpp"
#include "function/timer/timer.hpp"

namespace wen {

class TimerSystem final {
    friend class Singleton<TimerSystem>;
    TimerSystem();
    ~TimerSystem();

public:
    std::shared_ptr<Timer> registerTimer(float time_rate = 1.0f);
    void stop();
    void reset();

private:
    std::vector<std::shared_ptr<Timer>> timers_;
};

}  // namespace wen