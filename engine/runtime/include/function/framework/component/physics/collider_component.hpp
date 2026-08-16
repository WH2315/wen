#pragma once

#include "function/framework/component.hpp"
#include "function/physics/physics_system.hpp"
#include "engine/global_context.hpp"
#include <glm/glm.hpp>

namespace wen {

// 碰撞体:形状 + 尺寸参数。配合 RigidbodyComponent 在 Play 模式生成 Jolt 碰撞体;
// Edit 模式下 Inspector 可编辑、视口画线框。
class ColliderComponent : public Component {
    REFLECT_CLASS("ColliderComponent")

public:
    static constexpr int kShapeBox = 0;
    static constexpr int kShapeSphere = 1;
    static constexpr int kShapeCapsule = 2;
    static constexpr int kShapeCylinder = 3;

    ColliderComponent() : is_trigger(false) {
        // 编辑器/撤销改成员后通知物理系统重建/更新 body。
        addMemberUpdateCallback([this](Component*) {
            if (game_object_ != nullptr) {
                global_context->physics_system->notifyChanged(game_object_->getUUID());
            }
        });
    }

    std::string getClassName() const override { return "ColliderComponent"; }
    static std::string GetClassName() { return "ColliderComponent"; }

    // 形状类型(ColliderComponent::kShape*)。默认必须用数值字面量(parser.py 陷阱)。
    REFLECT_MEMBER()
    int shape_type = 0;

    // Box 半尺寸(本地空间, 乘以 Transform.scale)。
    REFLECT_MEMBER()
    glm::vec3 half_extents{0.5f, 0.5f, 0.5f};

    // Sphere/Capsule/Cylinder 半径。
    REFLECT_MEMBER()
    float radius = 0.5f;

    // Capsule/Cylinder 总高度(沿 Y 轴, 含端盖)。
    REFLECT_MEMBER()
    float height = 2.0f;

    // 形状中心相对 Transform 原点的偏移。
    REFLECT_MEMBER()
    glm::vec3 center{0.0f, 0.0f, 0.0f};

    // 触发器:只检测接触, 不产生碰撞响应。默认值放构造函数(bool 无初始化器)。
    REFLECT_MEMBER()
    bool is_trigger;
};

}  // namespace wen
