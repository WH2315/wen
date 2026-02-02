#pragma once

#include <functional>

namespace wen {

enum class EventType {
    eWindowClose,
    eWindowFocus,
    eWindowLostFocus,
    eWindowResize,
    eKeyPressed,
    eKeyReleased,
    eInputChar,
    eMousePressed,
    eMouseReleased,
    eMouseMoved,
    eMouseScrolled
};

enum class EventCategory {
    eWindow,
    eKey,
    eInput,
    eMouse,
};

template <class Event>
using EventCallback = std::function<bool(Event&)>;

#define REGISTER_EVENT(Category, Type, ...)                                   \
    struct Type##Event {                                                      \
        __VA_ARGS__                                                           \
        static constexpr EventCategory category = EventCategory::e##Category; \
        static constexpr EventType type = EventType::e##Type;                 \
        static std::vector<EventCallback<Type##Event>> callbacks;             \
    };

REGISTER_EVENT(Window, WindowClose)
REGISTER_EVENT(Window, WindowFocus)
REGISTER_EVENT(Window, WindowLostFocus)
REGISTER_EVENT(Window, WindowResize, int width; int height;)
REGISTER_EVENT(Key, KeyPressed, int keycode; bool is_repeat;)
REGISTER_EVENT(Key, KeyReleased, int keycode;)
REGISTER_EVENT(Input, InputChar, char character;)
REGISTER_EVENT(Mouse, MousePressed, int button;)
REGISTER_EVENT(Mouse, MouseReleased, int button;)
REGISTER_EVENT(Mouse, MouseMoved, double x_pos; double y_pos;)
REGISTER_EVENT(Mouse, MouseScrolled, double x_offset; double y_offset;)

}  // namespace wen