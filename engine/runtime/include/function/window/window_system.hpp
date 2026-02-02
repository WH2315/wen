#pragma once

#include "core/base/singleton.hpp"
#include "function/window/window.hpp"

namespace wen {

class WindowSystem final {
    friend class Singleton<WindowSystem>;
    WindowSystem(const WindowInfo& info);
    ~WindowSystem();

public:
    void pollEvents() { window_->pollEvents(); }
    bool shouldClose() const { return window_->shouldClose(); }

    Window* getRuntimeWindow() const { return window_; }

private:
    Window* window_;
};

}  // namespace wen