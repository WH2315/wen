#pragma once

#include "function/framework/component.hpp"

namespace wen {

class GameObject final {
public:
    GameObject(const std::string& name);
    ~GameObject();

    void awake();
    void start();
    void fixedTick();
    void tick(float dt);
    void postTick(float dt);

    void addComponent(Component* component);
    void removeComponent(Component* component);
    void removeComponent(ComponentTypeUUID uuid);

    Component* queryComponent(const std::string& class_name);
    Component* forceGetComponent(const std::string& class_name);

public:
    auto getUUID() const { return uuid_; }

    auto getName() const { return name_; }

    template <class C>
    C* queryComponent() {
        return static_cast<C*>(queryComponent(C::GetClassName()));
    }

    template <class C>
    C* forceGetComponent() {
        return static_cast<C*>(forceGetComponent(C::GetClassName()));
    }

    auto getComponents() { return components_; }

private:
    GameObjectUUID uuid_;
    std::string name_;
    std::map<ComponentTypeUUID, Component*> component_map_;
    std::list<Component*> components_;
};

}  // namespace wen