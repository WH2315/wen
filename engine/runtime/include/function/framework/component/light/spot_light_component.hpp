#pragma once

#include "function/framework/component.hpp"
#include <glm/glm.hpp>

namespace wen {

// 聚光灯:位置取自所在物体 Transform,向 direction 锥形照射,带内外锥角。
class SpotLightComponent : public Component {
    REFLECT_CLASS("SpotLightComponent")

public:
    std::string getClassName() const override { return "SpotLightComponent"; }
    static std::string GetClassName() { return "SpotLightComponent"; }

    SpotLightComponent()
        : direction(0.0f, -1.0f, 0.0f),
          color(1.0f),
          intensity(1.0f),
          range(10.0f),
          inner_angle_deg(20.0f),
          outer_angle_deg(45.0f) {}

    REFLECT_MEMBER()
    glm::vec3 direction;  // 锥轴方向

    REFLECT_MEMBER()
    glm::vec3 color;

    REFLECT_MEMBER()
    float intensity;

    REFLECT_MEMBER()
    float range;

    REFLECT_MEMBER()
    float inner_angle_deg;  // 内锥角(度),内锥内全亮

    REFLECT_MEMBER()
    float outer_angle_deg;  // 外锥角(度),内外锥之间平滑衰减
};

}  // namespace wen
