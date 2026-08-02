#pragma once

#include "ui/panels/inspector/component_ui/component_ui_manager.hpp"
#include "function/framework/component/camera/perspective_camera_component.hpp"

namespace wen::editor {

// PerspectiveCameraComponent 的数据视图
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
    void render(ComponentView<PerspectiveCameraComponent>& view) {
        auto& camera = view.getCamera();
        ImGui::PushID(&view);
        if (ImGui::TreeNodeEx("PerspectiveCameraComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool changed = false;
            changed |= ImGui::DragFloat("fov", &camera.fov, 0.1f, 1.0f, 179.0f);
            changed |= ImGui::DragFloat("near", &camera.near, 0.01f, 0.001f, camera.far);
            changed |= ImGui::DragFloat("far", &camera.far, 1.0f, camera.near, 100000.0f);
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
