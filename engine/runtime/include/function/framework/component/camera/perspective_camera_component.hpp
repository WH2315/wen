#pragma once

#include "function/framework/component/camera/camera_component.hpp"
#include "engine/global_context.hpp"

namespace wen {

class PerspectiveCameraComponent : public CameraComponent {
    REFLECT_CLASS("PerspectiveCameraComponent")

public:
    std::string getClassName() const override { return "PerspectiveCameraComponent"; }
    static std::string GetClassName() { return "PerspectiveCameraComponent"; }

    PerspectiveCameraComponent(float fov_degrees, float aspect, float near, float far) {
        calculatePerspectiveMatrix(fov_degrees, aspect, near, far);
    }

    PerspectiveCameraComponent(float fov_degrees, float width, float height, float near, float far) {
        calculatePerspectiveMatrix(fov_degrees, width, height, near, far);
    }

    void calculatePerspectiveMatrix(float fov_degrees, float aspect, float near, float far) {
        global_context->camera_system->reportCameraProjectMatrix(camera_id, glm::perspective(glm::radians(fov_degrees), aspect, near, far), near, far);
    }

    void calculatePerspectiveMatrix(float fov_degrees, float width, float height, float near, float far) {
        global_context->camera_system->reportCameraProjectMatrix(camera_id, glm::perspectiveFov(glm::radians(fov_degrees), width, height, near, far), near, far);
    }
};

}  // namespace wen