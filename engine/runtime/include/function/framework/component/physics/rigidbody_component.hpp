#pragma once

#include "function/framework/component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/physics/collider_component.hpp"
#include "function/physics/physics_system.hpp"
#include "engine/global_context.hpp"
#include <glm/glm.hpp>

namespace wen {

// 刚体:运动类型 + 物理材质参数。配合 ColliderComponent 在 Play 模式生成 Jolt body。
// 动态体由物理模拟驱动 Transform;kinematic 由 Transform 反向驱动 body。
class RigidbodyComponent : public Component {
    REFLECT_CLASS("RigidbodyComponent")

public:
    static constexpr int kStatic = 0;
    static constexpr int kDynamic = 1;
    static constexpr int kKinematic = 2;

    RigidbodyComponent() : body_type(kDynamic), use_ccd(false) {
        // 编辑器/撤销改成员后通知物理系统重建/更新 body。
        addMemberUpdateCallback([this](Component*) {
            if (game_object_ != nullptr) {
                global_context->physics_system->notifyChanged(game_object_->getUUID());
            }
        });
    }

    std::string getClassName() const override { return "RigidbodyComponent"; }
    static std::string GetClassName() { return "RigidbodyComponent"; }

    // 运动类型(RigidbodyComponent::kStatic/kDynamic/kKinematic)。数值字面量默认值。
    REFLECT_MEMBER()
    int body_type = 1;

    REFLECT_MEMBER()
    float mass = 1.0f;

    REFLECT_MEMBER()
    float friction = 0.5f;

    REFLECT_MEMBER()
    float restitution = 0.0f;

    REFLECT_MEMBER()
    float linear_damping = 0.05f;

    REFLECT_MEMBER()
    float angular_damping = 0.05f;

    REFLECT_MEMBER()
    float gravity_scale = 1.0f;

    // 连续碰撞检测(防止高速穿透)。默认值放构造函数。
    REFLECT_MEMBER()
    bool use_ccd;

    // 初始线速度(本地? 世界)。Play 开始时应用。
    REFLECT_MEMBER()
    glm::vec3 linear_velocity{0.0f, 0.0f, 0.0f};

    REFLECT_MEMBER()
    glm::vec3 angular_velocity{0.0f, 0.0f, 0.0f};

    void onStart() override {
        global_context->physics_system->registerRigidbody(
            game_object_->getUUID(),
            game_object_->queryComponent<ColliderComponent>(),
            this,
            game_object_->queryComponent<TransformComponent>());
    }

    void onDestroy() override {
        global_context->physics_system->unregisterRigidbody(game_object_->getUUID());
    }
};

}  // namespace wen
