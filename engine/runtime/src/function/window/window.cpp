#include "function/window/window.hpp"
#include "core/base/macro.hpp"

namespace wen {

Window::Window(const WindowInfo& info) {
    auto t = info.title;
    auto w = info.width;
    auto h = info.height;
    WEN_CORE_INFO("Create Window : (\"{}\", {}, {})", t, w, h)

    glfwSetErrorCallback([](int error, const char* description) {
        WEN_CORE_ERROR("GLFW Error ({}): {}", error, description)
    });

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(static_cast<int>(w), static_cast<int>(h),
                               t.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
}

}  // namespace wen