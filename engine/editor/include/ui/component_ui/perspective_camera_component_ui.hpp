#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/undo.hpp"
#include "function/framework/component/camera/perspective_camera_component.hpp"

namespace wen::editor {

// PerspectiveCameraComponent 的自定义检查器 UI 特化。
template <>
class ComponentView<PerspectiveCameraComponent> {
public:
    ComponentView(PerspectiveCameraComponent& camera) : camera_(camera) {}

    CameraID getCameraID() const { return camera_.camera_id; }
    PerspectiveCameraComponent& getCamera() { return camera_; }

private:
    PerspectiveCameraComponent& camera_;
};

template <>
class ComponentUI<PerspectiveCameraComponent> {
public:
    void render(ComponentView<PerspectiveCameraComponent>& view, const std::function<void()>& on_remove = {}) {
        auto& camera = view.getCamera();
        ImGui::PushID(&view);
        bool open = ImGui::TreeNodeEx("PerspectiveCameraComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            bool changed = false;
            float before_fov = camera.fov;
            changed |= ImGui::DragFloat("fov", &camera.fov, 0.1f, 1.0f, 179.0f);
            trackMemberEdit(&camera, "fov", before_fov);
            float before_near = camera.near;
            changed |= ImGui::DragFloat("near", &camera.near, 0.01f, 0.001f, camera.far);
            trackMemberEdit(&camera, "near", before_near);
            float before_far = camera.far;
            changed |= ImGui::DragFloat("far", &camera.far, 1.0f, camera.near, 100000.0f);
            trackMemberEdit(&camera, "far", before_far);
            if (changed) {
                camera.triggerMemberUpdateCallbacks();
            }
            if (ImGui::Button("Set as Primary Viewport")) {
                global_context->camera_system->reportCameraAsPrimaryViewport(view.getCameraID());
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
};

}  // namespace wen::editor
