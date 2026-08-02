#pragma once

#include "function/framework/component.hpp"

namespace wen::editor {

// 通用的组件数据视图:把组件实例与其 UI 关联起来。需要自定义检查器的
// 组件对其做特化(参见 PerspectiveCameraComponent 的特化)。
template <class ComponentType>
class ComponentView {
public:
    ComponentView(ComponentType&) {}
};

// 通用的组件检查器 UI。只有注册了自定义 UI 的组件类型才会实例化
// (特化在别处);未注册的组件由 ComponentUIManager 按反射信息自动绘制。
template <class ComponentType>
class ComponentUI {
public:
    void render(ComponentView<ComponentType>& view);
};

// 绘制组件的检查器 UI:注册了自定义 UI 的组件用自定义 UI,
// 其余组件根据反射元数据自动渲染。
class ComponentUIManager {
public:
    ComponentUIManager();

    void renderComponent(Component* component);

    // 视图缓存以裸 Component 指针为键;所属场景卸载时必须丢弃。
    void clearViewCache() { component_view_cache_.clear(); }

private:
    template <class ComponentCls>
    void registerComponentUI();

    // 按组件类名索引的类型擦除渲染器。渲染回调持有对应的 ComponentUI
    // 实例;缓存的视图是携带类型化删除器的 shared_ptr<void>,
    // 无需手动管理释放。
    std::map<std::string, std::function<void(Component*)>> ui_renderers_;
    std::map<Component*, std::shared_ptr<void>> component_view_cache_;
};

}  // namespace wen::editor
