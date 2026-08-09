#include "ui/panels/viewport_camera.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"
#include <imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

namespace wen::editor {

namespace {
constexpr float kPanFactorPerPixel = 0.0015f;
}  // namespace

ViewportCamera::ViewportCamera() {
    viewport_camera_id = global_context->camera_system->addCamera(true);
    global_ui_context->viewport_camera_id = viewport_camera_id;
    reset();
}

ViewportCamera::~ViewportCamera() {
    global_context->camera_system->removeCamera(viewport_camera_id);
}

void ViewportCamera::reset() {
    location_ = {0.0f, 0.0f, -10.0f};
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    orbit_distance_ = 10.0f;
    flying_ = orbiting_ = panning_ = false;
    updateProjectMatrix();
    updateViewMatrix();
}

glm::vec3 ViewportCamera::forwardDirection() const {
    return glm::rotateY(glm::rotateX(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(pitch_)), glm::radians(yaw_));
}

glm::vec3 ViewportCamera::upDirection() const {
    return glm::rotateY(glm::rotateX(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(pitch_)), glm::radians(yaw_));
}

glm::vec3 ViewportCamera::leftDirection() const {
    return glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(yaw_ + 90.0f));
}

void ViewportCamera::onTick(float dt, bool viewport_hovered, bool gizmo_busy) {
    auto& io = ImGui::GetIO();

    updateProjectMatrix();

    // 交互状态机:悬停时按下才进入,按键松开后结束。
    bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    bool lmb = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool mmb = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (!flying_ && rmb && viewport_hovered) {
        flying_ = true;
    }
    if (!rmb) {
        flying_ = false;
    }
    if (!orbiting_ && lmb && io.KeyAlt && viewport_hovered && !gizmo_busy) {
        orbiting_ = true;
    }
    if (!lmb || !io.KeyAlt) {
        orbiting_ = false;
    }
    if (!panning_ && mmb && viewport_hovered) {
        panning_ = true;
    }
    if (!mmb) {
        panning_ = false;
    }

    bool changed = false;
    const float look_speed = global_ui_context->editor_camera_sensitivity;

    if (flying_) {
        yaw_ -= io.MouseDelta.x * look_speed;
        pitch_ = std::clamp(pitch_ + io.MouseDelta.y * look_speed, -89.0f, 89.0f);
        changed |= (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f);

        // 滚轮调整飞行速度
        if (io.MouseWheel != 0.0f) {
            global_ui_context->editor_camera_speed =
                std::clamp(global_ui_context->editor_camera_speed * (1.0f + io.MouseWheel * 0.1f), 0.5f, 100.0f);
        }

        float boost = io.KeyShift ? 3.0f : 1.0f;
        float delta = dt * global_ui_context->editor_camera_speed * boost;
        auto forward = forwardDirection();
        auto left = leftDirection();

        if (ImGui::IsKeyDown(ImGuiKey_W)) { location_ += delta * forward; changed = true; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { location_ -= delta * forward; changed = true; }
        if (ImGui::IsKeyDown(ImGuiKey_A)) { location_ += delta * left; changed = true; }
        if (ImGui::IsKeyDown(ImGuiKey_D)) { location_ -= delta * left; changed = true; }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) { location_.y -= delta; changed = true; }
        if (ImGui::IsKeyDown(ImGuiKey_E)) { location_.y += delta; changed = true; }
    } else if (orbiting_) {
        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
            // 绕前方固定枢轴点环绕:先求枢轴,改朝向后再回到枢轴距离。
            glm::vec3 pivot = location_ + forwardDirection() * orbit_distance_;
            yaw_ -= io.MouseDelta.x * look_speed;
            pitch_ = std::clamp(pitch_ + io.MouseDelta.y * look_speed, -89.0f, 89.0f);
            location_ = pivot - forwardDirection() * orbit_distance_;
            changed = true;
        }
    } else if (panning_) {
        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
            float factor = orbit_distance_ * kPanFactorPerPixel;
            location_ += leftDirection() * (io.MouseDelta.x * factor);
            location_ += upDirection() * (io.MouseDelta.y * factor);
            changed = true;
        }
    } else if (viewport_hovered && io.MouseWheel != 0.0f) {
        // 沿视线方向推拉;尽量保持环绕枢轴点稳定。
        float step = std::max(orbit_distance_ * 0.1f, 0.5f) * io.MouseWheel;
        location_ += forwardDirection() * step;
        orbit_distance_ = std::max(orbit_distance_ - step, 1.0f);
        changed = true;
    }

    if (changed) {
        updateViewMatrix();
    }
}

void ViewportCamera::focusOn(const glm::vec3& center, float radius) {
    orbit_distance_ = std::clamp(radius * 2.5f, 1.0f, 10000.0f);
    location_ = center - forwardDirection() * orbit_distance_;
    updateViewMatrix();
}

void ViewportCamera::updateViewMatrix() {
    global_context->camera_system->reportCameraViewMatrix(
        viewport_camera_id,
        glm::lookAt(location_, location_ + forwardDirection(), glm::vec3(0.0f, 1.0f, 0.0f)),
        true);
}

void ViewportCamera::updateProjectMatrix() {
    float aspect = global_ui_context->viewport_aspect;
    if (aspect <= 0.0f) {
        aspect = 16.0f / 9.0f;
    }
    constexpr float near = 0.1f;
    constexpr float far = 1000.0f;
    global_context->camera_system->reportCameraProjectMatrix(
        viewport_camera_id,
        glm::perspective(glm::radians(global_ui_context->editor_camera_fov), aspect, near, far),
        near, far, true);
}

}  // namespace wen::editor
