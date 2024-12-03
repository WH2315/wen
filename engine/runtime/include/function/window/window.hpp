#pragma once

#include <GLFW/glfw3.h>

namespace wen {

struct WindowInfo {
    std::string title;
    uint32_t width, height;

    WindowInfo(const std::string& title, uint32_t width, uint32_t height)
        : title(title), width(width), height(height) {}
};

class Window final {
public:
    Window(const WindowInfo& info);
    ~Window();

    static void pollEvents() { glfwPollEvents(); }

    bool shouldClose() const { return glfwWindowShouldClose(window_); }

    GLFWwindow* getWindow() const { return window_; }
    uint32_t getWidth() const { return data_.width; }
    uint32_t getHeight() const { return data_.height; }

private:
    GLFWwindow* window_;

    struct WindowData {
        std::string title;
        uint32_t width, height;
    } data_;
};

} // namespace wen