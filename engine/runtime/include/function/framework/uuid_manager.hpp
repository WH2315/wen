#pragma once

#include "core/base/singleton.hpp"

namespace wen {

using ComponentTypeUUID = uint32_t;
using GameObjectUUID = uint64_t;

class GameObjectUUIDAllocator final {
    friend class Singleton<GameObjectUUIDAllocator>;
    GameObjectUUIDAllocator();
    ~GameObjectUUIDAllocator();

public:
    GameObjectUUID allocate();

private:
    GameObjectUUID current_uuid_;
};

class ComponentTypeUUIDSystem final {
    friend class Singleton<ComponentTypeUUIDSystem>;
    ComponentTypeUUIDSystem();
    ~ComponentTypeUUIDSystem();

public:
    ComponentTypeUUID get(const std::string& class_name);

private:
    std::map<std::string, ComponentTypeUUID> uuid_map_;
    ComponentTypeUUID current_uuid_;
};

}  // namespace wen