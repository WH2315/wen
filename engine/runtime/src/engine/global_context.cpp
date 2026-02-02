#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_system.initialize(LogLevel::trace, LogLevel::trace);
    window_system.initialize(WindowInfo("wen 16 : 9", 1600, 900));
    event_system.initialize();
}

void GlobalContext::shutdown() {
    event_system.destroy();
    window_system.destroy();
    log_system.destroy();
}

}  // namespace wen