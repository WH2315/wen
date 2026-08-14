#pragma once

#include "function/framework/component.hpp"

namespace wen {

// Prefab 实例的"链接":记录来源 .prefab 资产路径(相对 <root>/prefabs/)。
// 由 prefab 服务添加/管理;Revert 从 .prefab 重载、Apply 把当前状态写回。
// 该组件随场景序列化,保证实例跨场景保存后仍保留模板链接。
class PrefabComponent : public Component {
    REFLECT_CLASS("PrefabComponent")

public:
    std::string getClassName() const override { return "PrefabComponent"; }
    static std::string GetClassName() { return "PrefabComponent"; }

    REFLECT_MEMBER()
    std::string prefab_path;
};

}  // namespace wen
