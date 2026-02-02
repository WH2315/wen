#include "engine/global_context.hpp"

namespace wen {

EventSystem::EventSystem() {
    auto* window = global_context->window_system->getRuntimeWindow()->getWindow();

    glfwSetWindowCloseCallback(window, [](GLFWwindow*) {
        global_context->event_system->dispatch<WindowCloseEvent>({});
    });

    glfwSetWindowFocusCallback(window, [](GLFWwindow*, int focused) {
        if (focused) {
            global_context->event_system->dispatch<WindowFocusEvent>({});
        } else {
            global_context->event_system->dispatch<WindowLostFocusEvent>({});
        }
    });

    glfwSetWindowSizeCallback(window, [](GLFWwindow*, int width, int height) {
        global_context->event_system->dispatch<WindowResizeEvent>({width, height});
    });

    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
        if (action == GLFW_PRESS) {
            global_context->event_system->dispatch<KeyPressedEvent>({key, false});
        } else if (action == GLFW_REPEAT) {
            global_context->event_system->dispatch<KeyPressedEvent>({key, true});
        } else if (action == GLFW_RELEASE) {
            global_context->event_system->dispatch<KeyReleasedEvent>({key});
        }
    });

    glfwSetCharCallback(window, [](GLFWwindow*, unsigned int character) {
        global_context->event_system->dispatch<InputCharEvent>({static_cast<char>(character)});
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int) {
        if (action == GLFW_PRESS) {
            global_context->event_system->dispatch<MousePressedEvent>({button});
        } else if (action == GLFW_RELEASE) {
            global_context->event_system->dispatch<MouseReleasedEvent>({button});
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow*, double x_pos, double y_pos) {
        global_context->event_system->dispatch<MouseMovedEvent>({x_pos, y_pos});
    });

    glfwSetScrollCallback(window, [](GLFWwindow*, double x_offset, double y_offset) {
        global_context->event_system->dispatch<MouseScrolledEvent>({x_offset, y_offset});
    });
}

#define DESTROY_EVENT_CALLBACKS(Type) Type##Event::callbacks.clear();

EventSystem::~EventSystem() {
    DESTROY_EVENT_CALLBACKS(WindowClose)
    DESTROY_EVENT_CALLBACKS(WindowFocus)
    DESTROY_EVENT_CALLBACKS(WindowLostFocus)
    DESTROY_EVENT_CALLBACKS(WindowResize)
    DESTROY_EVENT_CALLBACKS(KeyPressed)
    DESTROY_EVENT_CALLBACKS(KeyReleased)
    DESTROY_EVENT_CALLBACKS(InputChar)
    DESTROY_EVENT_CALLBACKS(MousePressed)
    DESTROY_EVENT_CALLBACKS(MouseReleased)
    DESTROY_EVENT_CALLBACKS(MouseMoved)
    DESTROY_EVENT_CALLBACKS(MouseScrolled)
}

}  // namespace wen