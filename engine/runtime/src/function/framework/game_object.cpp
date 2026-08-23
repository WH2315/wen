#include "function/framework/game_object.hpp"
#include "engine/global_context.hpp"
#include <algorithm>

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

void GameObject::setParent(GameObject* new_parent) {
    // 已满足目标关系则直接返回(同为 nullptr 或父相同)。
    if (new_parent == parent_) {
        return;
    }
    // 成环校验:不能设为自己、也不能设为自己的后代为父。
    if (new_parent == this || (new_parent != nullptr && new_parent->isDescendantOf(this))) {
        WEN_CORE_ERROR("GameObject \"{}\": cannot set parent (would create a cycle).", name_)
        return;
    }
    removeFromParent();  // 先脱离当前父
    parent_ = new_parent;
    if (new_parent != nullptr) {
        new_parent->children_.push_back(this);
    }
}

void GameObject::removeFromParent() {
    if (parent_ == nullptr) {
        return;
    }
    auto& siblings = parent_->children_;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    parent_ = nullptr;
}

bool GameObject::hasChild(GameObject* child) const {
    if (child == nullptr) {
        return false;
    }
    return std::find(children_.begin(), children_.end(), child) != children_.end();
}

bool GameObject::isDescendantOf(GameObject* ancestor) const {
    if (ancestor == nullptr) {
        return false;
    }
    for (auto* current = parent_; current != nullptr; current = current->parent_) {
        if (current == ancestor) {
            return true;
        }
    }
    return false;
}

}  // namespace wen