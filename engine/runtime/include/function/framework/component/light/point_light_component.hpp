#pragma once

#include "function/framework/component.hpp"
#include <glm/glm.hpp>

namespace wen {

// 点光源:向四周发光,随距离平方衰减。位置取所在物体的 Transform。
class PointLightComponent : public Component {
    REFLECT_CLASS("PointLightComponent")

public:
    std::string getClassName() const override { return "PointLightComponent"; }
    static std::string GetClassName() { return "PointLightComponent"; }

    PointLightComponent() : color(1.0f), intensity(1.0f), range(10.0f) {}

    REFLECT_MEMBER()
    glm::vec3 color;

    REFLECT_MEMBER()
    float intensity;

    REFLECT_MEMBER()
    float range;  // 衰减距离
};

}  // namespace wen
