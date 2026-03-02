#pragma once

#include "function/framework/component.hpp"

namespace wen {

class TransformComponent : public Component {
    REFLECT_CLASS("TransformComponent")

public:
    std::string getClassName() const override { return "TransformComponent"; }
    static std::string GetClassName() { return "TransformComponent"; }

    REFLECT_MEMBER()
    glm::vec3 location{0, 0, 0};

    REFLECT_MEMBER()
    glm::vec3 rotation{0, 0, 0};

    REFLECT_MEMBER()
    glm::vec3 scale{1, 1, 1};
};

}  // namespace wen