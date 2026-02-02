#pragma once

#include "core/base/singleton.hpp"
#include "function/event/event.hpp"

namespace wen {

class EventSystem final {
    friend class Singleton<EventSystem>;
    EventSystem();
    ~EventSystem();

public:
    template <class Event>
    void attach(const EventCallback<Event>& callback) {
        Event::callbacks.push_back(callback);
    }

private:
    template <class Event>
    void dispatch(Event event) {
        for (auto& callback : Event::callbacks) {
            if (callback(event)) {
                break;
            }
        }
    }
};

}  // namespace wen