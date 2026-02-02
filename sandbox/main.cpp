#include <wen.hpp>

int main() {
    auto engine = std::make_unique<wen::Engine>();

    engine->startupEngine();

    auto& event_system = wen::global_context->event_system;
    event_system->attach<wen::KeyPressedEvent>([](wen::KeyPressedEvent& event) {
        WEN_CLIENT_INFO("Key: {}, {}, {}, {}", (char)event.keycode, event.is_repeat, (int)event.category, (int)event.type)
        return false;
    });
    event_system->attach<wen::MousePressedEvent>([](wen::MousePressedEvent& event) {
        WEN_CLIENT_INFO("1. mouse button: {}", event.button)
        return true;
    });
    event_system->attach<wen::MousePressedEvent>([](wen::MousePressedEvent& event) {
        WEN_CLIENT_INFO("2. mouse button: {}", event.button)
        return false;
    });

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}