#pragma once

#include "function/framework/component.hpp"

namespace wen {

class GameObject final {
public:
    GameObject(const std::string& name);
    GameObject(const std::string& name, GameObjectUUID uuid) : uuid_(uuid), name_(name) {}
    ~GameObject();

    void awake();
    void start();
    void fixedTick();
    void tick(float dt);
    void postTick(float dt);

    void addComponent(Component* component);
    void removeComponent(Component* component);

    Component* queryComponent(const std::string& class_name);

public:
    auto getUUID() const { return uuid_; }

    auto getName() const { return name_; }

    void setName(const std::string& name) { name_ = name; }

    template <class C>
    C* queryComponent() {
        return static_cast<C*>(queryComponent(C::GetClassName()));
    }

    auto getComponents() { return components_; }

private:
    GameObjectUUID uuid_;
    std::string name_;
    std::map<ComponentTypeUUID, Component*> component_map_;
    std::list<Component*> components_;
};

}  // namespace wen