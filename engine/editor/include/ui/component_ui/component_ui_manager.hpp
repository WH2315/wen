#pragma once

#include "function/framework/component.hpp"

namespace wen::editor {

// 组件检查器视图与 UI 的基模板:需要自定义检查器的组件对其做特化。
template <class ComponentType>
class ComponentView {
public:
    ComponentView(ComponentType&) {}
};

template <class ComponentType>
class ComponentUI {
public:
    void render(ComponentView<ComponentType>& view);
};

// 组件检查器管理器:注册了自定义 UI 的组件用自定义 UI,其余组件按反射元数据自动绘制。
class ComponentUIManager {
public:
    ComponentUIManager();

    void renderComponent(Component* component);

    // 视图缓存以裸 Component 指针为键;所属场景卸载时必须丢弃。
    void clearViewCache() { component_view_cache_.clear(); }

    // 注册组件类的自定义检查器 UI(需有 ComponentUI<ComponentCls> 特化)。
    template <class ComponentCls>
    void registerComponentUI() {
        auto ui = std::make_shared<ComponentUI<ComponentCls>>();
        ui_renderers_[ComponentCls::GetClassName()] = [this, ui](Component* component) {
            auto& view = component_view_cache_[component];
            if (!view) {
                view = std::make_shared<ComponentView<ComponentCls>>(*static_cast<ComponentCls*>(component));
            }
            ui->render(*static_cast<ComponentView<ComponentCls>*>(view.get()));
        };
    }

private:

    std::map<std::string, std::function<void(Component*)>> ui_renderers_;
    std::map<Component*, std::shared_ptr<void>> component_view_cache_;
};

}  // namespace wen::editor
