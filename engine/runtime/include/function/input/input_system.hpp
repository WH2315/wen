#pragma once

#include "core/base/singleton.hpp"
#include <glm/glm.hpp>

namespace wen {

enum class ButtonState {
    ePressed,
    eReleased,
};

class InputSystem final {
    friend class Singleton<InputSystem>;
    InputSystem();
    ~InputSystem();

public:
    void tick();

    bool isKeyPressed(int keycode) const;
    bool isKeyReleased(int keycode) const;
    bool isMousePressed(int button) const;
    bool isMouseReleased(int button) const;

    glm::dvec2 getMousePosition() const;
    glm::dvec2 getMouseDelta() const;
    glm::dvec2 getMouseScroll() const;

private:
    ButtonState key_states_[350];
    ButtonState mouse_states_[10];
    glm::dvec2 now_mouse_position_;
    glm::dvec2 last_mouse_position_;
    glm::dvec2 mouse_delta_;
    glm::dvec2 mouse_scroll_;
};

}  // namespace wen