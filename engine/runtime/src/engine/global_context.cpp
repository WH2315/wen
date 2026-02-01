#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_system.initialize(LogLevel::trace, LogLevel::trace);
}

void GlobalContext::shutdown() { log_system.destroy(); }

}  // namespace wen