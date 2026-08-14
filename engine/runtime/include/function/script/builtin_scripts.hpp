#pragma once

#include "function/script/script.hpp"
#include "function/framework/component/transform/transform_component.hpp"

namespace wen {

// 绕 axis 轴匀速旋转。字段: speed(度/秒), axis(旋转轴)。
class RotateScript : public Script {
public:
    std::string getScriptName() const override { return "RotateScript"; }

    RotateScript() {
        declareField("speed", 90.0f);
        declareField("axis", glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void onTick(float dt) override {
        if (auto* transform = owner()->queryComponent<TransformComponent>()) {
            transform->rotation += get<glm::vec3>("axis") * (get<float>("speed") * dt);
            transform->triggerMemberUpdateCallbacks();
        }
    }
};

// 沿 direction 方向匀速移动。字段: direction(单位方向), speed(单位/秒)。
class MoveScript : public Script {
public:
    std::string getScriptName() const override { return "MoveScript"; }

    MoveScript() {
        declareField("direction", glm::vec3(1.0f, 0.0f, 0.0f));
        declareField("speed", 1.0f);
    }

    void onTick(float dt) override {
        if (auto* transform = owner()->queryComponent<TransformComponent>()) {
            transform->location += get<glm::vec3>("direction") * (get<float>("speed") * dt);
            transform->triggerMemberUpdateCallbacks();
        }
    }
};

}  // namespace wen
