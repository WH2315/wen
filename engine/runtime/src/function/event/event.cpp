#include "function/event/event.hpp"

namespace wen {

#define DEFINE_EVENT_CALLBACKS(Type) \
    std::vector<EventCallback<Type##Event>> Type##Event::callbacks;

DEFINE_EVENT_CALLBACKS(WindowClose)
DEFINE_EVENT_CALLBACKS(WindowFocus)
DEFINE_EVENT_CALLBACKS(WindowLostFocus)
DEFINE_EVENT_CALLBACKS(WindowResize)
DEFINE_EVENT_CALLBACKS(KeyPressed)
DEFINE_EVENT_CALLBACKS(KeyReleased)
DEFINE_EVENT_CALLBACKS(InputChar)
DEFINE_EVENT_CALLBACKS(MousePressed)
DEFINE_EVENT_CALLBACKS(MouseReleased)
DEFINE_EVENT_CALLBACKS(MouseMoved)
DEFINE_EVENT_CALLBACKS(MouseScrolled)

}  // namespace wen