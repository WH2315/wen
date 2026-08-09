#include "ui/panels/viewport_panel.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/widgets.hpp"
#include "ui/editor_scene.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace wen::editor {

namespace {

struct GizmoTool {
    ImGuiKey key;
    const char* label;
    const char* tooltip;
    bool enable;
    ImGuizmo::OPERATION operation;
};

constexpr GizmoTool kGizmoTools[] = {
    {ImGuiKey_Q, "Q", "View - hide gizmo (Q)", false, ImGuizmo::TRANSLATE},
    {ImGuiKey_W, "W", "Move (W)", true, ImGuizmo::TRANSLATE},
    {ImGuiKey_E, "E", "Rotate (E)", true, ImGuizmo::ROTATE},
    {ImGuiKey_R, "R", "Scale (R)", true, ImGuizmo::SCALE},
    {ImGuiKey_Y, "Y", "Universal (Y)", true, ImGuizmo::UNIVERSAL},
};

void applyGizmoTool(const GizmoTool& tool) {
    global_ui_context->gizmo_enabled = tool.enable;
    if (tool.enable) {
        global_ui_context->gizmo_operation = tool.operation;
    }
}

bool isGizmoToolActive(const GizmoTool& tool) {
    const auto& ctx = *global_ui_context;
    return tool.enable ? (ctx.gizmo_enabled && ctx.gizmo_operation == tool.operation)
                       : !ctx.gizmo_enabled;
}

void toggleGizmoMode() {
    global_ui_context->gizmo_mode =
        (global_ui_context->gizmo_mode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
}

}  // namespace

void ViewportPanel::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    global_ui_context->viewport_hovered = ImGui::IsWindowHovered();

    ImVec2 image_pos = ImGui::GetCursorScreenPos();
    ImVec2 region = ImGui::GetContentRegionAvail();
    if (region.y > 0.0f) {
        global_ui_context->viewport_aspect = region.x / region.y;
        global_context->render_system->setViewportAspectOverride(global_ui_context->viewport_aspect);
    }

    auto texture = global_context->render_system->getViewportTexture();
    if (texture != VK_NULL_HANDLE && region.x > 0.0f && region.y > 0.0f) {
        ImGui::Image(texture, region);

        if (global_ui_context->mode == Mode::eEdit && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kMeshDragDropPayload)) {
                std::filesystem::path file(std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1));
                if (auto* game_object = spawnMeshGameObject(file)) {
                    if (on_select_game_object_) {
                        on_select_game_object_(game_object->getUUID());
                    }
                    pushGameObjectCreated(game_object);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (global_ui_context->mode == Mode::eEdit) {
            handleToolShortcuts();
            renderGizmo(image_pos, region);
            renderToolbar(image_pos);
            handlePicking(image_pos, region);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::handleToolShortcuts() {
    if (!global_ui_context->viewport_hovered ||
        global_ui_context->viewport_flying ||
        ImGui::GetIO().WantTextInput) {
        return;
    }

    for (const auto& tool : kGizmoTools) {
        if (ImGui::IsKeyPressed(tool.key, false)) {
            applyGizmoTool(tool);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        toggleGizmoMode();
    }
}

void ViewportPanel::renderToolbar(const ImVec2& image_pos) {
    ImGui::SetCursorScreenPos(ImVec2(image_pos.x + 8.0f, image_pos.y + 8.0f));
    ImGui::BeginGroup();

    for (const auto& tool : kGizmoTools) {
        if (&tool != &kGizmoTools[0]) {
            ImGui::SameLine();
        }
        if (widgets::toggleButton(tool.label, isGizmoToolActive(tool), tool.tooltip)) {
            applyGizmoTool(tool);
        }
    }
    ImGui::SameLine();
    const char* mode_label = (global_ui_context->gizmo_mode == ImGuizmo::WORLD) ? "World" : "Local";
    if (widgets::toggleButton(mode_label, false, "Toggle World/Local (X)")) {
        toggleGizmoMode();
    }

    ImGui::EndGroup();
    toolbar_hovered_ = ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

void ViewportPanel::renderGizmo(const ImVec2& image_pos, const ImVec2& image_size) {
    if (!global_ui_context->gizmo_enabled) {
        return;
    }
    if (global_ui_context->selected_game_object_uuid == kInvalidGameObjectUUID) {
        return;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    auto* game_object = scene ? scene->getGameObject(global_ui_context->selected_game_object_uuid) : nullptr;
    if (game_object == nullptr) {
        return;
    }
    auto* transform = game_object->queryComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    // 手势尚未开始时记录拖动起点,手势结束推一条撤销命令。
    if (!gizmo_using_) {
        gizmo_start_location_ = transform->location;
        gizmo_start_rotation_ = transform->rotation;
        gizmo_start_scale_ = transform->scale;
    }

    auto* camera_data = global_context->camera_system->queryCameraData(global_ui_context->viewport_camera_id);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(image_pos.x, image_pos.y, image_size.x, image_size.y);

    glm::mat4 matrix;
    ImGuizmo::RecomposeMatrixFromComponents(
        glm::value_ptr(transform->location),
        glm::value_ptr(transform->rotation),
        glm::value_ptr(transform->scale),
        glm::value_ptr(matrix));

    // Ctrl 启用吸附:平移/缩放 0.5,旋转按 15° 步进。
    float snap_values[3] = {0.0f, 0.0f, 0.0f};
    const float* snap = nullptr;
    if (ImGui::GetIO().KeyCtrl) {
        switch (global_ui_context->gizmo_operation) {
            case ImGuizmo::ROTATE:
                snap_values[0] = 15.0f;
                break;
            case ImGuizmo::SCALE:
                snap_values[0] = snap_values[1] = snap_values[2] = 0.1f;
                break;
            default:
                snap_values[0] = snap_values[1] = snap_values[2] = 0.5f;
                break;
        }
        snap = snap_values;
    }

    ImGuizmo::Manipulate(
        glm::value_ptr(camera_data->view),
        glm::value_ptr(camera_data->project),
        global_ui_context->gizmo_operation,
        global_ui_context->gizmo_mode,
        glm::value_ptr(matrix),
        nullptr,
        snap);
    if (ImGuizmo::IsUsing()) {
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(matrix),
            &transform->location.x,
            &transform->rotation.x,
            &transform->scale.x);
        transform->triggerMemberUpdateCallbacks();
    }

    // 一次拖动手势结束 → 一条撤销命令(三个成员合并,一次 Ctrl+Z 撤销整个手势)。
    bool using_now = ImGuizmo::IsUsing();
    if (gizmo_using_ && !using_now) {
        pushPropertyEdit(global_ui_context->selected_game_object_uuid, TransformComponent::GetClassName(),
                         {{"location", gizmo_start_location_, transform->location},
                          {"rotation", gizmo_start_rotation_, transform->rotation},
                          {"scale", gizmo_start_scale_, transform->scale}});
    }
    gizmo_using_ = using_now;
}

void ViewportPanel::handlePicking(const ImVec2& image_pos, const ImVec2& image_size) {
    if (!global_ui_context->viewport_hovered ||
        global_ui_context->viewport_flying ||
        toolbar_hovered_ ||
        ImGui::GetIO().KeyAlt ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }
    bool gizmo_visible = global_ui_context->gizmo_enabled &&
                         global_ui_context->selected_game_object_uuid != kInvalidGameObjectUUID;
    if (gizmo_visible && (ImGuizmo::IsOver() || ImGuizmo::IsUsing())) {
        return;
    }

    ImVec2 mouse = ImGui::GetMousePos();
    float x_factor = (mouse.x - image_pos.x) / image_size.x;
    float y_factor = (mouse.y - image_pos.y) / image_size.y;
    if (x_factor < 0.0f || x_factor >= 1.0f || y_factor < 0.0f || y_factor >= 1.0f) {
        return;
    }

    // 面板里的图像是离屏场景拉伸后的结果,把面板坐标映射回离屏像素。
    auto config = global_context->render_system->getRendererConfig();
    auto x = static_cast<uint32_t>(x_factor * config.swapchain_image_width);
    auto y = static_cast<uint32_t>(y_factor * config.swapchain_image_height);

    global_context->render_system->pickGameObject(
        x, y,
        [this](GameObjectUUID uuid) {
            if (on_select_game_object_) {
                on_select_game_object_(uuid);
            }
        },
        [this]() {
            if (on_select_game_object_) {
                on_select_game_object_(kInvalidGameObjectUUID);
            }
        });
}

}  // namespace wen::editor
