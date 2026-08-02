#pragma once

#include "function/framework/component/camera/camera_component.hpp"

namespace wen {

class PerspectiveCameraComponent : public CameraComponent {
    REFLECT_CLASS("PerspectiveCameraComponent")

public:
    std::string getClassName() const override { return "PerspectiveCameraComponent"; }
    static std::string GetClassName() { return "PerspectiveCameraComponent"; }

    PerspectiveCameraComponent(float fov_degrees, float width, float height, float near_plane, float far_plane)
        : fov(fov_degrees), near(near_plane), far(far_plane), fallback_aspect_(width / height) {
        updateProjection();
        addMemberUpdateCallback([this](Component*) {
            updateProjection();
        });
        resize_callback_id_ = global_context->render_system->registerResizeCallback([this]() {
            updateProjection();
        });
    }

    ~PerspectiveCameraComponent() override {
        global_context->render_system->unregisterResizeCallback(resize_callback_id_);
    }

    REFLECT_MEMBER()
    float fov;

    REFLECT_MEMBER()
    float near;

    REFLECT_MEMBER()
    float far;

private:
    void updateProjection() {
        float aspect = global_context->render_system->getOutputAspect();
        if (aspect <= 0.0f) {
            aspect = fallback_aspect_;
        }
        global_context->camera_system->reportCameraProjectMatrix(
            camera_id, glm::perspective(glm::radians(fov), aspect, near, far), near, far);
    }

    float fallback_aspect_;
    uint32_t resize_callback_id_;
};

}  // namespace wen
