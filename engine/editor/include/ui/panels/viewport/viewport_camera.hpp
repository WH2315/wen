#pragma once

#include "function/camera/camera_system.hpp"

namespace wen::editor {

// 在引擎 CameraSystem 中分配一个专用的编辑器相机,并上报它的视图/投影矩阵。
//
// 交互(仅当 Viewport 面板被悬停时才会开始):
//   - 按住右键:飞行 - 鼠标转向 + WASD 沿视线移动,QE 垂直移动,
//     Shift 加速,滚轮调整飞行速度。
//   - Alt+左键:围绕相机前方的枢轴点环绕。
//   - 中键:平移。
//   - 滚轮(非飞行时):沿视线方向推拉。
//   - focusOn():框选目标(F 快捷键,由 UI 处理)。
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
