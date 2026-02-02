#pragma once

#include <GLFW/glfw3.h>
#include <string>

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

    void pollEvents() { glfwPollEvents(); }
    bool shouldClose() const { return glfwWindowShouldClose(window_); }

    GLFWwindow* getWindow() const { return window_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

private:
    GLFWwindow* window_;
    uint32_t width_;
    uint32_t height_;
};

}  // namespace wen