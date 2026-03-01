#include "function/framework/game_object.hpp"
#include "engine/global_context.hpp"

namespace wen {

void RTTI::setupRTTI() {
    descriptor_ = &global_context->reflect_system->getClass(getClassName());
}

GameObject::GameObject(const std::string& name) : name_(name) {
    uuid_ = global_context->game_object_uuid_allocator->allocate();
}

GameObject::~GameObject() {
    for (auto& component : components_) {
        component->onDestroy();
        delete component;
    }
    components_.clear();
    component_map_.clear();
}

void GameObject::awake() {
    for (auto* component : components_) {
        component->onAwake();
    }
}

void GameObject::start() {
    for (auto* component : components_) {
        component->onStart();
    }
}

void GameObject::fixedTick() {
    for (auto* component : components_) {
        component->onFixedTick();
    }
}

void GameObject::tick(float dt) {
    for (auto* component : components_) {
        component->onTick(dt);
    }
}

void GameObject::postTick(float dt) {
    for (auto* component : components_) {
        component->onPostTick(dt);
    }
}

void GameObject::addComponent(Component* component) {
    component->uuid_ = global_context->component_type_uuid_system->get(component->getClassName());
    if (component_map_.find(component->getComponentTypeUUID()) != component_map_.end()) {
        WEN_CORE_ERROR("component with uuid {} already exists in game object {}.", component->getComponentTypeUUID(), name_)
        return;
    }
    component->master_ = this;
    component->setupRTTI();
    component_map_.insert({component->getComponentTypeUUID(), component});
    components_.push_back(component);
    component->onCreate();
}

void GameObject::removeComponent(Component* component) {
    removeComponent(component->getComponentTypeUUID());
}

void GameObject::removeComponent(ComponentTypeUUID uuid) {
    auto iter = component_map_.find(uuid);
    if (iter == component_map_.end()) {
        WEN_CORE_ERROR("component with uuid {} does not exist in game object {}.", uuid, name_)
        return;
    }
    components_.remove(iter->second);
    component_map_.erase(iter);
}

Component* GameObject::queryComponent(const std::string& class_name) {
    auto uuid = global_context->component_type_uuid_system->get(class_name);
    if (auto iter = component_map_.find(uuid); iter != component_map_.end()) {
        return component_map_.at(uuid);
    }
    return nullptr;
}

Component* GameObject::forceGetComponent(const std::string& class_name) {
    return component_map_.at(global_context->component_type_uuid_system->get(class_name));
}

}  // namespace wen