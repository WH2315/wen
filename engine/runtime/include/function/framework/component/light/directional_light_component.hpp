#pragma once

#include "function/framework/component.hpp"
#include <glm/glm.hpp>

namespace wen {

// 方向光:平行光,只关心照射方向(不随距离衰减)。
class DirectionalLightComponent : public Component {
    REFLECT_CLASS("DirectionalLightComponent")

public:
    std::string getClassName() const override { return "DirectionalLightComponent"; }
    static std::string GetClassName() { return "DirectionalLightComponent"; }

    DirectionalLightComponent() : direction(0.0f, -1.0f, 0.0f), color(1.0f), intensity(1.0f) {}

    REFLECT_MEMBER()
    glm::vec3 direction;  // 世界空间照射方向(指向光源)

    REFLECT_MEMBER()
    glm::vec3 color;

    REFLECT_MEMBER()
    float intensity;
};

}  // namespace wen
