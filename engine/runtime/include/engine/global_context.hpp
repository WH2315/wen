#pragma once

#include "core/log/log_system.hpp"
#include "function/window/window_system.hpp"
#include "function/event/event_system.hpp"
#include "function/input/input_system.hpp"
#include "function/timer/timer_system.hpp"

namespace wen {

struct GlobalContext {
    void startup();
    void shutdown();

    Singleton<LogSystem> log_system;
    Singleton<WindowSystem> window_system;
    Singleton<EventSystem> event_system;
    Singleton<InputSystem> input_system;
    Singleton<TimerSystem> timer_system;
};

extern GlobalContext* global_context;

}  // namespace wen