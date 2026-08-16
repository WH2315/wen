#include "ui/panels/viewport_panel.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/widgets.hpp"
#include "ui/editor_scene.hpp"
#include "ui/prefab_actions.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/physics/collider_component.hpp"
#include "function/camera/camera_system.hpp"
#include "core/math/transform_math.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

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
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropPayload)) {
                std::filesystem::path file(std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1));
                auto prefabs_dir = std::filesystem::path(global_context->asset_system->getRootDir()) / "prefabs";
                std::error_code ec;
                auto relative = std::filesystem::relative(file, prefabs_dir, ec);
                if (!ec) {
                    if (auto* game_object = instantiatePrefab(relative.generic_string())) {
                        if (on_select_game_object_) {
                            on_select_game_object_(game_object->getUUID());
                        }
                        pushGameObjectCreated(game_object);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (global_ui_context->mode == Mode::eEdit) {
            handleToolShortcuts();
            renderGizmo(image_pos, region);
            renderColliderWireframe(image_pos, region);
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

void ViewportPanel::renderColliderWireframe(const ImVec2& image_pos, const ImVec2& image_size) {
    if (global_ui_context->selected_game_object_uuid == kInvalidGameObjectUUID) {
        return;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    auto* game_object = scene ? scene->getGameObject(global_ui_context->selected_game_object_uuid) : nullptr;
    if (game_object == nullptr) {
        return;
    }
    auto* collider = game_object->queryComponent<ColliderComponent>();
    auto* transform = game_object->queryComponent<TransformComponent>();
    if (collider == nullptr || transform == nullptr) {
        return;
    }
    auto* camera_data = global_context->camera_system->queryCameraData(global_ui_context->viewport_camera_id);
    if (camera_data == nullptr) {
        return;
    }

    auto* draw_list = ImGui::GetWindowDrawList();
    const ImU32 color = IM_COL32(255, 200, 0, 220);
    constexpr float kPi = 3.14159265358979f;

    // 世界点投影到视口像素(Vulkan NDC y 向上 -> 像素 y 向下)。
    auto project = [&](const glm::vec3& world, ImVec2& out) -> bool {
        glm::vec4 clip = camera_data->project * camera_data->view * glm::vec4(world, 1.0f);
        if (clip.w <= 1e-4f) {
            return false;
        }
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        out = ImVec2(image_pos.x + (ndc.x * 0.5f + 0.5f) * image_size.x,
                     image_pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * image_size.y);
        return true;
    };
    auto drawSeg = [&](const glm::vec3& a, const glm::vec3& b) {
        ImVec2 pa, pb;
        if (project(a, pa) && project(b, pb)) {
            draw_list->AddLine(pa, pb, color, 1.5f);
        }
    };
    auto polyline = [&](const std::vector<glm::vec3>& pts, bool closed) {
        for (size_t i = 1; i < pts.size(); ++i) {
            drawSeg(pts[i - 1], pts[i]);
        }
        if (closed && pts.size() > 2) {
            drawSeg(pts.back(), pts.front());
        }
    };
    // local -> world(旋转 + 平移;与 PhysicsSystem::buildShape 一致:尺寸含 scale,center 偏移不含)。
    glm::mat3 R = math::composeRotation(transform->rotation);
    glm::vec3 s = transform->scale;
    auto toWorld = [&](const glm::vec3& local) { return transform->location + R * local; };
    auto worldPts = [&](const std::vector<glm::vec3>& local_pts) {
        std::vector<glm::vec3> out;
        out.reserve(local_pts.size());
        for (const auto& p : local_pts) {
            out.push_back(toWorld(p));
        }
        return out;
    };

    switch (collider->shape_type) {
        case ColliderComponent::kShapeBox: {
            glm::vec3 h = collider->half_extents * s;
            glm::vec3 c[8];
            for (int i = 0; i < 8; ++i) {
                c[i] = collider->center + glm::vec3((i & 1) ? h.x : -h.x,
                                                    (i & 2) ? h.y : -h.y,
                                                    (i & 4) ? h.z : -h.z);
            }
            constexpr int kEdges[12][2] = {
                {0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
                {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
            for (const auto& e : kEdges) {
                drawSeg(toWorld(c[e[0]]), toWorld(c[e[1]]));
            }
            break;
        }
        case ColliderComponent::kShapeSphere: {
            float r = collider->radius * s.x;
            const int n = 32;
            std::vector<glm::vec3> xy, xz, yz;
            for (int i = 0; i < n; ++i) {
                float a = 2.0f * kPi * i / n;
                xy.emplace_back(collider->center + glm::vec3(r * std::cos(a), r * std::sin(a), 0.0f));
                xz.emplace_back(collider->center + glm::vec3(r * std::cos(a), 0.0f, r * std::sin(a)));
                yz.emplace_back(collider->center + glm::vec3(0.0f, r * std::cos(a), r * std::sin(a)));
            }
            polyline(worldPts(xy), true);
            polyline(worldPts(xz), true);
            polyline(worldPts(yz), true);
            break;
        }
        case ColliderComponent::kShapeCapsule: {
            float r = collider->radius * s.x;
            float half = std::max(collider->height * 0.5f * s.y - r, 0.0f);
            const int n = 24;
            std::vector<glm::vec3> ring_b, ring_t;
            for (int i = 0; i < n; ++i) {
                float a = 2.0f * kPi * i / n;
                glm::vec3 radial(r * std::cos(a), 0.0f, r * std::sin(a));
                ring_b.emplace_back(collider->center + radial + glm::vec3(0.0f, -half, 0.0f));
                ring_t.emplace_back(collider->center + radial + glm::vec3(0.0f, half, 0.0f));
            }
            polyline(worldPts(ring_b), true);
            polyline(worldPts(ring_t), true);
            // 4 根竖线。
            for (int i = 0; i < 4; ++i) {
                float a = 0.5f * kPi * i;
                glm::vec3 side(r * std::cos(a), 0.0f, r * std::sin(a));
                drawSeg(toWorld(collider->center + side + glm::vec3(0.0f, -half, 0.0f)),
                        toWorld(collider->center + side + glm::vec3(0.0f, half, 0.0f)));
            }
            // 4 个经线半圆(φ=0,45,90,135)画上下半球盖。
            for (int plane = 0; plane < 4; ++plane) {
                float phi = plane * kPi / 4.0f;
                for (int cap = -1; cap <= 1; cap += 2) {
                    std::vector<glm::vec3> arc;
                    for (int i = 0; i <= 6; ++i) {
                        float t = i / 6.0f * (kPi / 2.0f);
                        float cy = cap * (half + r * std::sin(t));
                        float cr = r * std::cos(t);
                        arc.emplace_back(collider->center +
                                         glm::vec3(cr * std::cos(phi), cy, cr * std::sin(phi)));
                    }
                    polyline(worldPts(arc), false);
                }
            }
            break;
        }
        case ColliderComponent::kShapeCylinder: {
            float r = collider->radius * s.x;
            float half = collider->height * 0.5f * s.y;
            const int n = 24;
            std::vector<glm::vec3> ring_b, ring_t;
            for (int i = 0; i < n; ++i) {
                float a = 2.0f * kPi * i / n;
                glm::vec3 radial(r * std::cos(a), 0.0f, r * std::sin(a));
                ring_b.emplace_back(collider->center + radial + glm::vec3(0.0f, -half, 0.0f));
                ring_t.emplace_back(collider->center + radial + glm::vec3(0.0f, half, 0.0f));
            }
            polyline(worldPts(ring_b), true);
            polyline(worldPts(ring_t), true);
            for (int i = 0; i < 4; ++i) {
                float a = 0.5f * kPi * i;
                glm::vec3 side(r * std::cos(a), 0.0f, r * std::sin(a));
                drawSeg(toWorld(collider->center + side + glm::vec3(0.0f, -half, 0.0f)),
                        toWorld(collider->center + side + glm::vec3(0.0f, half, 0.0f)));
            }
            break;
        }
        default:
            break;
    }
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
