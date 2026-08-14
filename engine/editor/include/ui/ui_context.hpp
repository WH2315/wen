#pragma once

#include "function/framework/uuid_manager.hpp"
#include "function/camera/camera_types.hpp"
#include "ui/scene_file_actions.hpp"
#include <imgui.h>
#include <ImGuizmo.h>

namespace wen::editor {

class FileDialog;

enum class Mode {
    eEdit,  // 编辑模式:编辑器相机驱动视口,引擎逻辑暂停
    eGame,  // 播放模式:引擎逻辑恢复,视图交给场景相机
};

inline constexpr GameObjectUUID kInvalidGameObjectUUID = GameObjectUUID(-1);

// Prefab 实例操作(面板发起,Editor 主循环在 ImGui 帧外执行 —— 因为 revert 会销毁重建对象,
// 帧内执行会让正在渲染该对象的面板持悬空指针而崩溃)。
enum class PrefabActionKind : int { eNone = 0, eRevert, eApply };

struct PendingPrefabAction {
    PrefabActionKind kind = PrefabActionKind::eNone;
    GameObjectUUID uuid = kInvalidGameObjectUUID;
};

// 编辑器的全局共享状态(由 UI 持有)。面板/服务直接读写它,不另设访问层。
struct UIContext {
    Mode mode{Mode::eEdit};
    std::function<void(Mode mode)> change_mode_callback;
    // 在 Content Browser 中定位资源(Inspector 对象字段"揭示"用)。
    std::function<void(const std::filesystem::path&)> reveal_asset_callback;

    // 选中对象
    GameObjectUUID selected_game_object_uuid{kInvalidGameObjectUUID};

    // Viewport 面板状态
    bool viewport_hovered{false};
    float viewport_aspect{16.0f / 9.0f};

    // 编辑器相机
    CameraID viewport_camera_id{0};
    bool viewport_flying{false};
    float editor_camera_fov{60.0f};
    float editor_camera_speed{10.0f};
    float editor_camera_sensitivity{0.15f};

    // Gizmo 工具状态(Q/W/E/R/Y 切换, X 切换世界/局部)
    bool gizmo_enabled{true};
    ImGuizmo::OPERATION gizmo_operation{ImGuizmo::TRANSLATE};
    ImGuizmo::MODE gizmo_mode{ImGuizmo::WORLD};

    // 播放模式状态
    bool game_paused{false};
    bool step_one_frame{false};

    // 场景文件操作(菜单/浏览器发起,Editor 帧外执行)
    SceneFileActions scene_file_actions;

    // Prefab 实例操作(Inspector 发起,Editor 帧外执行)
    PendingPrefabAction pending_prefab;

    // 文件浏览器对话框(由 UI 持有)
    FileDialog* file_dialog{nullptr};
};

extern UIContext* global_ui_context;

}  // namespace wen::editor
