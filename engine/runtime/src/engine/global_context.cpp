#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_sys.initialize(LogLevel::trace, LogLevel::trace);
    window_sys.initialize(WindowInfo{"cozy 16 : 9", 1600, 900});
}

void GlobalContext::shutdown() {
    window_sys.destroy();
    log_sys.destroy();
}

} // namespace wen