#pragma once

#include "function/framework/component/camera/camera_component.hpp"

namespace wen {

class OrthographicCameraComponent : public CameraComponent {
    REFLECT_CLASS("OrthographicCameraComponent")

public:
    std::string getClassName() const override { return "OrthographicCameraComponent"; }
    static std::string GetClassName() { return "OrthographicCameraComponent"; }

    // 反序列化/工厂构造用:成员随后由反射填入,再经 triggerMemberUpdateCallbacks 重建投影。
    OrthographicCameraComponent() : OrthographicCameraComponent(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f) {}

    OrthographicCameraComponent(float left, float right, float bottom, float top, float near_plane, float far_plane)
        : left(left), right(right), bottom(bottom), top(top), near(near_plane), far(far_plane) {
        updateProjection();
        addMemberUpdateCallback([this](Component*) {
            updateProjection();
        });
    }

    REFLECT_MEMBER()
    float left;

    REFLECT_MEMBER()
    float right;

    REFLECT_MEMBER()
    float bottom;

    REFLECT_MEMBER()
    float top;

    REFLECT_MEMBER()
    float near;

    REFLECT_MEMBER()
    float far;

private:
    void updateProjection() {
        global_context->camera_system->reportCameraProjectMatrix(
            camera_id, glm::ortho(left, right, bottom, top, near, far), near, far);
    }
};

}  // namespace wen
