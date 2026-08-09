#pragma once

#include "function/camera/camera_system.hpp"

namespace wen::editor {

// 编辑器相机:在引擎 CameraSystem 分配专用相机并上报视图/投影矩阵。
// 交互(悬停视口时):右键飞行 + WASD,Alt+左键环绕,中键平移,滚轮推拉。
class ViewportCamera {
public:
    ViewportCamera();
    ~ViewportCamera();

    void reset();
    void onTick(float dt, bool viewport_hovered, bool gizmo_busy);
    void focusOn(const glm::vec3& center, float radius);

    bool isFlying() const { return flying_; }

    CameraID viewport_camera_id{0};

private:
    void updateViewMatrix();
    void updateProjectMatrix();
    glm::vec3 forwardDirection() const;
    glm::vec3 upDirection() const;
    glm::vec3 leftDirection() const;

private:
    glm::vec3 location_{0.0f, 0.0f, -10.0f};
    float yaw_{0.0f};
    float pitch_{0.0f};
    float orbit_distance_{10.0f};

    bool flying_{false};
    bool orbiting_{false};
    bool panning_{false};
};

}  // namespace wen::editor
