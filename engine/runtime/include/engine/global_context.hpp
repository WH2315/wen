#pragma once

#include "core/log/log_system.hpp"
#include "function/window/window_system.hpp"

namespace wen {

struct GlobalContext {
    void startup();
    void shutdown();

    Singleton<LogSystem> log_system;
    Singleton<WindowSystem> window_system;
};

extern GlobalContext* global_context;

}  // namespace wen