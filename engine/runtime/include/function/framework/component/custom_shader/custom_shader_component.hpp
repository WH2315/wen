#pragma once

#include "function/framework/component.hpp"

namespace wen {

// 自定义着色器材质:引用一个 .mat 资产(决定片元着色器 + 参数)。
// 挂载该组件的对象(还需 MeshComponent + TransformComponent)会脱离默认
// deferred 通道,改由 CustomMaterialPass 用目标 .mat 的着色器做 forward 绘制。
class CustomShaderComponent : public Component {
    REFLECT_CLASS("CustomShaderComponent")

public:
    std::string getClassName() const override { return "CustomShaderComponent"; }
    static std::string GetClassName() { return "CustomShaderComponent"; }

    // 相对 <资源根>/materials 的 .mat 资产路径。
    REFLECT_MEMBER()
    std::string material_path;
};

}  // namespace wen