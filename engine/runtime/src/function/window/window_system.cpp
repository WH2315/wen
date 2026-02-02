#include "function/window/window_system.hpp"

namespace wen {

WindowSystem::WindowSystem(const WindowInfo& info) {
    window_ = new Window(info);
}

WindowSystem::~WindowSystem() {
    if (window_) {
        delete window_;
        window_ = nullptr;
    }
}

}  // namespace wen