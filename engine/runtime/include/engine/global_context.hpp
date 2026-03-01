#pragma once

#include "core/log/log_system.hpp"
#include "function/window/window_system.hpp"
#include "function/event/event_system.hpp"
#include "function/input/input_system.hpp"
#include "function/timer/timer_system.hpp"
#include "core/reflect/reflect_system.hpp"
#include "function/render/render_system.hpp"
#include "function/framework/uuid_manager.hpp"
#include "function/framework/scene_manager.hpp"

namespace wen {

struct GlobalContext {
    void startup();
    void shutdown();

    Singleton<LogSystem> log_system;
    Singleton<WindowSystem> window_system;
    Singleton<EventSystem> event_system;
    Singleton<InputSystem> input_system;
    Singleton<TimerSystem> timer_system;
    Singleton<ReflectSystem> reflect_system;
    Singleton<RenderSystem> render_system;
    Singleton<GameObjectUUIDAllocator> game_object_uuid_allocator;
    Singleton<ComponentTypeUUIDSystem> component_type_uuid_system;
    Singleton<SceneManager> scene_manager;
};

extern GlobalContext* global_context;

}  // namespace wen