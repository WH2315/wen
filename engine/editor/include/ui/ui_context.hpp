#pragma once

#include "function/framework/uuid_manager.hpp"
#include "function/camera/camera_system.hpp"
#include <imgui.h>
#include <ImGuizmo.h>

namespace wen::editor {

enum class Mode {
    eEdit,
    eGame,
};

inline constexpr GameObjectUUID kInvalidGameObjectUUID = GameObjectUUID(-1);

struct UIContext {
    Mode mode{Mode::eEdit};
    std::function<void(Mode mode)> change_mode_callback;

    GameObjectUUID selected_game_object_uuid{kInvalidGameObjectUUID};

    // Viewport 面板状态
    bool viewport_hovered{false};
    float viewport_aspect{16.0f / 9.0f};

    // 编辑器相机
    CameraID viewport_camera_id{0};
    bool viewport_flying{false};

    // 编辑器相机参数
    float editor_camera_fov{60.0f};
    float editor_camera_speed{10.0f};
    float editor_camera_sensitivity{0.15f};  // 度/像素

    // Q 观察、W 移动、E 旋转、R 缩放、Y 万能工具，X 切换世界/局部坐标系
    bool gizmo_enabled{true};
    ImGuizmo::OPERATION gizmo_operation{ImGuizmo::TRANSLATE};
    ImGuizmo::MODE gizmo_mode{ImGuizmo::WORLD};

    // 播放模式状态
    bool game_paused{false};
    bool step_one_frame{false};
};

extern UIContext* global_ui_context;

}  // namespace wen::editor
