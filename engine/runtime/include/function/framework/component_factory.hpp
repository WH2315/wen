#pragma once

#include "core/base/singleton.hpp"
#include "function/framework/component.hpp"
#include <vector>

namespace wen {

// 组件工厂:按类名默认构造组件实例(场景反序列化、编辑器复制等用途)。
// 注册由代码生成的 Parser() 自动完成:所有可默认构造的 Component 派生类
// 都会被注册,非组件类型与不可默认构造的类型在编译期被 tryRegister 筛掉。
class ComponentFactory final {
    friend class Singleton<ComponentFactory>;
    ComponentFactory();
    ~ComponentFactory();

public:
    template <class C>
    void tryRegister(const std::string& class_name) {
        if constexpr (std::is_base_of_v<Component, C> &&
                      std::is_default_constructible_v<C> &&
                      !std::is_same_v<Component, C>) {
            factories_[class_name] = []() -> Component* { return new C; };
        }
    }

    Component* create(const std::string& class_name) const {
        auto iter = factories_.find(class_name);
        return iter == factories_.end() ? nullptr : iter->second();
    }

    bool contains(const std::string& class_name) const {
        return factories_.find(class_name) != factories_.end();
    }

    // 所有已注册组件类名(编辑器"Add Component"列表用)。
    std::vector<std::string> classNames() const {
        std::vector<std::string> out;
        out.reserve(factories_.size());
        for (const auto& [class_name, _] : factories_) {
            out.push_back(class_name);
        }
        return out;
    }

private:
    std::map<std::string, std::function<Component*()>> factories_;
};

}  // namespace wen
