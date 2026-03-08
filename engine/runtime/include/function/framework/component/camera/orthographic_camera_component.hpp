#pragma once

#include "function/framework/component/camera/camera_component.hpp"
#include "engine/global_context.hpp"

namespace wen {

class OrthographicCameraComponent : public CameraComponent {
    REFLECT_CLASS("OrthographicCameraComponent")

public:
    std::string getClassName() const override { return "OrthographicCameraComponent"; }
    static std::string GetClassName() { return "OrthographicCameraComponent"; }

    OrthographicCameraComponent(float left, float right, float bottom, float top, float near, float far) {
        calculateOrthographicMatrix(left, right, bottom, top, near, far);
    }

    void calculateOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) {
        global_context->camera_system->reportCameraProjectMatrix(camera_id, glm::ortho(left, right, bottom, top, near, far), near, far);
    }
};

}  // namespace wen