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
    // 两阶段析构:先让所有组件跑 onDestroy(此时还能互相 queryComponent,
    // 例如 MeshComponent 需要反查 TransformComponent 解绑回调),再统一释放。
    for (auto& component : components_) {
        component->onDestroy();
    }
    for (auto& component : components_) {
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
    component->game_object_ = this;
    component->setupRTTI();
    component_map_.insert({component->getComponentTypeUUID(), component});
    components_.push_back(component);
    component->onCreate();
}

void GameObject::removeComponent(Component* component) {
    auto uuid = component->getComponentTypeUUID();
    auto iter = component_map_.find(uuid);
    if (iter == component_map_.end()) {
        WEN_CORE_ERROR("component with uuid {} does not exist in game object {}.", uuid, name_)
        return;
    }
    auto* removed = iter->second;
    components_.remove(removed);
    component_map_.erase(iter);
    removed->onDestroy();
    delete removed;
}

Component* GameObject::queryComponent(const std::string& class_name) {
    auto uuid = global_context->component_type_uuid_system->get(class_name);
    if (auto iter = component_map_.find(uuid); iter != component_map_.end()) {
        return component_map_.at(uuid);
    }
    return nullptr;
}

}  // namespace wen