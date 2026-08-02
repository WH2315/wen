#include "ui/panels/setting/setting_panel.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

void SettingPanel::render() {
    ImGui::Begin("Setting");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("DeltaTime: %.3f ms", ImGui::GetIO().DeltaTime * 1000.0f);
    ImGui::Text("Visibility: %d", global_context->render_system->getRenderFramework()->getResource()->visibility_count);
    ImGui::Text("DrawCall: %d", global_context->render_system->getRenderFramework()->getResource()->draw_call_count);

    ImGui::Separator();

    ImGui::Text("Mode: %s", global_ui_context->mode == Mode::eEdit ? "Edit" : "Game");

    ImGui::Separator();

    ImGui::TextUnformatted("Editor Camera");
    ImGui::SliderFloat("fov", &global_ui_context->editor_camera_fov, 20.0f, 120.0f, "%.0f deg");
    ImGui::SliderFloat("fly speed", &global_ui_context->editor_camera_speed, 0.5f, 100.0f, "%.1f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("sensitivity", &global_ui_context->editor_camera_sensitivity, 0.01f, 1.0f, "%.2f deg/px");

    ImGui::Separator();

    ImGui::TextUnformatted("Debug");
    // 冻结剔除相机，以便从其他视角观察 HZB 剔除效果
    bool freeze_culling = global_context->camera_system->isFixedClip();
    if (ImGui::Checkbox("Freeze Culling", &freeze_culling)) {
        if (freeze_culling) {
            global_context->camera_system->turnOnFixedClip();
        } else {
            global_context->camera_system->turnOffFixedClip();
        }
    }

    ImGui::End();
}

}  // namespace wen::editor
