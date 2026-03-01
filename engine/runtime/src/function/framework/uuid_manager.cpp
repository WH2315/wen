#include "function/framework/uuid_manager.hpp"

namespace wen {

GameObjectUUIDAllocator::GameObjectUUIDAllocator() { current_uuid_ = 0; }

GameObjectUUIDAllocator::~GameObjectUUIDAllocator() {}

GameObjectUUID GameObjectUUIDAllocator::allocate() {
    current_uuid_++;
    return current_uuid_;
}

ComponentTypeUUIDSystem::ComponentTypeUUIDSystem() { current_uuid_ = 0; }

ComponentTypeUUIDSystem::~ComponentTypeUUIDSystem() {}

ComponentTypeUUID ComponentTypeUUIDSystem::get(const std::string& class_name) {
    auto iter = uuid_map_.find(class_name);
    if (iter != uuid_map_.end()) {
        return iter->second;
    } else {
        current_uuid_++;
        uuid_map_[class_name] = current_uuid_;
        return current_uuid_;
    }
}

}  // namespace wen