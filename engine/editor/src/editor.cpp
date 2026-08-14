#include "editor.hpp"
#include "ui/ui_context.hpp"
#include "ui/editor_scene.hpp"
#include "ui/prefab_actions.hpp"
#include "ui/selection.hpp"
#include "ui/panels/content_browser_panel.hpp"
#include "engine/global_context.hpp"
#include "function/framework/scene_serializer.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/camera/perspective_camera_component.hpp"
#include "function/framework/component/camera/camera_controller_component.hpp"
#include "function/framework/component/light/directional_light_component.hpp"

namespace wen::editor {

Editor::Editor(Engine* engine) : engine_(engine) {}

// 启动时创建未保存的默认场景:Main Camera + Directional Light。
// 场景未落盘(current_scene_path 为空),Ctrl+S 会走"另存为"。
void Editor::createDefaultScene() {
    auto& scene_manager = *global_context->scene_manager;
    if (auto* existing = scene_manager.getActiveScene()) {
        scene_manager.destroyScene(existing->getName());
    }
    auto* scene = scene_manager.createScene("Untitled");
    if (scene == nullptr) {
        return;
    }
    createDefaultSceneObjects(scene);
    scene_manager.setActiveScene(scene);
}

// Unity 风格默认对象:Main Camera + Directional Light。
void Editor::createDefaultSceneObjects(Scene* scene) {
    auto* camera = scene->createGameObject("Main Camera");
    auto* camera_transform = new TransformComponent;
    camera_transform->location = {0.0f, 0.0f, -5.0f};
    camera->addComponent(camera_transform);
    camera->addComponent(new PerspectiveCameraComponent(60.0f, 1920.0f, 1080.0f, 0.1f, 1000.0f));
    camera->addComponent(new CameraControllerComponent);

    auto* light = scene->createGameObject("Directional Light");
    light->addComponent(new TransformComponent);
    light->addComponent(new DirectionalLightComponent);
}

void Editor::initialize() {
    ui_ = std::make_unique<UI>();

    // Play 模式:进入前快照场景,Stop 时还原)。
    global_ui_context->change_mode_callback = [this](Mode mode) {
        switch (mode) {
            case Mode::eEdit:
                global_ui_context->mode = Mode::eEdit;
                engine_->stopTimer();
                global_context->camera_system->activeEditorCamera(ui_->getViewportCameraID());
                // 场景还原不能发生在 ImGui 帧内,挂起标志由主循环执行。
                restore_scene_pending_ = !play_mode_snapshot_.empty();
                break;
            case Mode::eGame:
                play_mode_snapshot_ = SceneSerializer::saveToString(
                    global_context->scene_manager->getActiveScene());
                global_ui_context->mode = Mode::eGame;
                global_context->camera_system->deactiveEditorCamera();
                engine_->startTimer();
                break;
        }
    };

    // 编辑器 UI 作为渲染通道的一部分每帧回调。
    global_context->render_system->enableEditor([this]() {
        ui_->onFrame();
    });

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ContentBrowserPanel::registerIniSettings();

    // 启动时自动创建未保存的默认场景(Unity 风格:Main Camera + Directional Light)。
    createDefaultScene();

    ui_->onLoadScene();
}

void Editor::run() {
    engine_->startTimer();
    global_ui_context->change_mode_callback(global_ui_context->mode);

    while (engine_->isAlive()) {
        // 场景文件操作/还原会销毁重建场景,必须在 ImGui 帧外执行。
        processPlayModeRestore();
        processSceneFileAction();
        processPrefabAction();
        engine_->pollEvents();

        if (global_ui_context->mode == Mode::eGame) {
            bool should_tick = true;

            if (global_ui_context->game_paused) {
                should_tick = global_ui_context->step_one_frame;
                global_ui_context->step_one_frame = false;
            }

            if (should_tick) {
                engine_->tickLogic();
            }
        }

        engine_->tickRender();
    }
    engine_->waitFixedTickThread();
}

void Editor::processPlayModeRestore() {
    if (!restore_scene_pending_) {
        return;
    }
    restore_scene_pending_ = false;
    if (play_mode_snapshot_.empty()) {
        return;
    }

    global_context->render_system->waitIdle();
    ui_->onUnloadScene();

    auto* restored = SceneSerializer::loadFromString(play_mode_snapshot_);
    if (restored != nullptr) {
        global_context->scene_manager->setActiveScene(restored);
    }
    play_mode_snapshot_.clear();
    ui_->onLoadScene();
}

// 执行菜单/浏览器请求的场景文件操作(新建/打开/保存/另存为)。
void Editor::processSceneFileAction() {
    auto& scene_file_actions = global_ui_context->scene_file_actions;
    auto pending = scene_file_actions.consumePending();
    auto action = pending.action;
    if (action == SceneFileAction::eNone) {
        return;
    }
    if (global_ui_context->mode != Mode::eEdit) {
        return;
    }

    // 脏场景下切换(新建/打开)先挂起,由保存确认框决定是否继续。
    if (scene_file_actions.isDirty() &&
        (action == SceneFileAction::eNew || action == SceneFileAction::eOpen)) {
        scene_file_actions.setPendingOperation(
            action == SceneFileAction::eNew
                ? PendingOperation{PendingOperationKind::eNew}
                : PendingOperation{PendingOperationKind::eOpen, pending.file});
        return;
    }

    auto& scene_manager = *global_context->scene_manager;
    auto* active = scene_manager.getActiveScene();

    switch (action) {
        case SceneFileAction::eSave: {
            const auto& current_path = scene_file_actions.currentScenePath();
            if (active != nullptr && !current_path.empty()) {
                if (SceneSerializer::save(active, current_path)) {
                    scene_file_actions.clearDirty();
                }
            }
            break;
        }
        case SceneFileAction::eSaveAs: {
            auto target = pending.file;
            if (active == nullptr || target.empty()) {
                break;
            }
            if (saveSceneTo(target)) {
                scene_file_actions.setCurrentScenePath(target);
                scene_file_actions.clearDirty();
            }
            break;
        }
        case SceneFileAction::eNew: {
            global_context->render_system->waitIdle();
            ui_->onUnloadScene();
            if (active != nullptr) {
                scene_manager.destroyScene(active->getName());
            }
            scene_manager.destroyScene("Untitled");
            auto* new_scene = scene_manager.createScene("Untitled");
            if (new_scene != nullptr) {
                createDefaultSceneObjects(new_scene);
            }
            scene_manager.setActiveScene(new_scene);

            scene_file_actions.setCurrentScenePath({});
            scene_file_actions.clearDirty();
            ui_->onLoadScene();
            break;
        }
        case SceneFileAction::eOpen: {
            global_context->render_system->waitIdle();
            ui_->onUnloadScene();
            std::string old_name = active ? active->getName() : "";

            auto* loaded = SceneSerializer::load(pending.file);
            if (loaded != nullptr) {
                if (!old_name.empty() && old_name != loaded->getName()) {
                    scene_manager.destroyScene(old_name);
                }
                scene_manager.setActiveScene(loaded);

                scene_file_actions.setCurrentScenePath(pending.file);
                scene_file_actions.clearDirty();
            }
            ui_->onLoadScene();
            break;
        }
        case SceneFileAction::eNone:
            break;
    }
}

// 执行面板挂起的 Prefab 实例操作。Revert 会销毁重建对象,必须在 ImGui 帧外执行,
// 否则正在渲染该对象的面板会持悬空指针崩溃。
void Editor::processPrefabAction() {
    auto& pending = global_ui_context->pending_prefab;
    if (pending.kind == PrefabActionKind::eNone) {
        return;
    }
    auto kind = pending.kind;
    auto uuid = pending.uuid;
    pending.kind = PrefabActionKind::eNone;

    auto* scene = global_context->scene_manager->getActiveScene();
    auto* game_object = scene ? scene->getGameObject(uuid) : nullptr;
    if (game_object == nullptr) {
        return;
    }
    if (kind == PrefabActionKind::eRevert) {
        if (revertPrefabInstance(game_object)) {
            // 对象按同 uuid 重建,刷新选中描边索引(实例池索引可能已变化)。
            setSelectedGameObject(global_ui_context->selected_game_object_uuid);
            ui_->onPrefabReverted();
        }
    } else if (kind == PrefabActionKind::eApply) {
        applyPrefabInstance(game_object);
    }
}

void Editor::destroy() {
    ui_.reset();
}

Editor::~Editor() {
    engine_ = nullptr;
}

}  // namespace wen::editor
