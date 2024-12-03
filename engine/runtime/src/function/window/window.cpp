#include "function/window/window.hpp"
#include "core/base/macro.hpp"

namespace wen {

Window::Window(const WindowInfo& info) {
    std::string t = data_.title = info.title;
    uint32_t w = data_.width = info.width;
    uint32_t h = data_.height = info.height;
    WEN_CORE_INFO("Create Window : (\"{0}\", {1}, {2})", t, w, h)

    glfwSetErrorCallback([](int error, const char* description) {
        WEN_CORE_ERROR("GLFW Error ({0}): {1}", error, description)
    });

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(static_cast<int>(w), static_cast<int>(h), t.c_str(),
                               nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
}

} // namespace wen