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

    // ---- 场景图父子层级 ----
    // 注意:父子关系只存在于 GameObject 内部,Scene 仍用扁平 list 持有所有对象并负责所有权。

    GameObject* getParent() const { return parent_; }

    const std::vector<GameObject*>& getChildren() const { return children_; }

    // 把本对象挂到 new_parent 之下(先脱离当前父)。传 nullptr 表示成为根。
    // 自动校验成环(不能设为自己/自己的后代为父)。
    void setParent(GameObject* new_parent);

    // 从当前父的子列表中脱离,本对象成为根。不触发任何销毁。
    void removeFromParent();

    bool hasChild(GameObject* child) const;

    bool isDescendantOf(GameObject* ancestor) const;

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

    GameObject* parent_ = nullptr;
    std::vector<GameObject*> children_;
};

}  // namespace wen