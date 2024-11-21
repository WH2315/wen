#include "engine/global_context.hpp"

namespace wen {

GlobalContext* global_context = nullptr;

void GlobalContext::startup() {
    log_sys.initialize(LogLevel::trace, LogLevel::trace);
}

void GlobalContext::shutdown() {
    log_sys.destroy();
}

} // namespace wen