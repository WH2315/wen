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

    // 让分配器跳过已被场景反序列化占用的 uuid,避免后续 allocate() 撞号。
    void reserve(GameObjectUUID uuid) {
        if (uuid > current_uuid_) {
            current_uuid_ = uuid;
        }
    }

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