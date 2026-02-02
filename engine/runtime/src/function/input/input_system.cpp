#include "engine/global_context.hpp"

namespace wen {

InputSystem::InputSystem() {
    auto& event_system = global_context->event_system;

    event_system->attach<KeyPressedEvent>([&](const KeyPressedEvent& event) {
        key_states_[event.keycode] = ButtonState::ePressed;
        return false;
    });

    event_system->attach<KeyReleasedEvent>([&](const KeyReleasedEvent& event) {
        key_states_[event.keycode] = ButtonState::eReleased;
        return false;
    });

    event_system->attach<MousePressedEvent>([&](const MousePressedEvent& event) {
        mouse_states_[event.button] = ButtonState::ePressed;
        return false;
    });

    event_system->attach<MouseReleasedEvent>([&](const MouseReleasedEvent& event) {
        mouse_states_[event.button] = ButtonState::eReleased;
        return false;
    });

    event_system->attach<MouseMovedEvent>([&](const MouseMovedEvent& event) {
        now_mouse_position_ = {event.x_pos, event.y_pos};
        mouse_delta_ = now_mouse_position_ - last_mouse_position_;
        last_mouse_position_ = now_mouse_position_;
        return false;
    });

    event_system->attach<MouseScrolledEvent>([&](const MouseScrolledEvent& event) {
        mouse_scroll_ = {event.x_offset, event.y_offset};
        return false;
    });
}

bool InputSystem::isKeyPressed(int key) const {
    return key_states_[key] == ButtonState::ePressed;
}

bool InputSystem::isKeyReleased(int key) const {
    return key_states_[key] == ButtonState::eReleased;
}

bool InputSystem::isMousePressed(int button) const {
    return mouse_states_[button] == ButtonState::ePressed;
}

bool InputSystem::isMouseReleased(int button) const {
    return mouse_states_[button] == ButtonState::eReleased;
}

glm::dvec2 InputSystem::getMousePosition() const {
    return now_mouse_position_;
}

glm::dvec2 InputSystem::getMouseDelta() const {
    return mouse_delta_;
}

glm::dvec2 InputSystem::getMouseScroll() const {
    return mouse_scroll_;
}

InputSystem::~InputSystem() {}

}  // namespace wen