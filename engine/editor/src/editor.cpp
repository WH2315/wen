#include "editor.hpp"
#include "ui/ui_context.hpp"
#include "ui/panels/content_browser_panel.hpp"
#include "engine/global_context.hpp"
#include "function/framework/scene_serializer.hpp"

namespace wen::editor {

Editor::Editor(Engine* engine) : engine_(engine) {}

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

    ui_->onLoadScene();
}

void Editor::run() {
    engine_->startTimer();
    global_ui_context->change_mode_callback(global_ui_context->mode);

    while (engine_->isAlive()) {
        // 场景文件操作/还原会销毁重建场景,必须在 ImGui 帧外执行。
        processPlayModeRestore();
        processSceneFileAction();
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

    auto& scene_manager = *global_context->scene_manager;
    auto* active = scene_manager.getActiveScene();

    switch (action) {
        case SceneFileAction::eSave: {

            const auto& current_path = scene_file_actions.currentScenePath();
            if (active != nullptr && !current_path.empty()) {
                SceneSerializer::save(active, current_path);
            }
            break;
        }
        case SceneFileAction::eSaveAs: {
            auto target = pending.file;
            if (active == nullptr || target.empty()) {
                break;
            }
            // 场景名同步为文件名,保证文件内 "scene" 字段与文件名一致。
            std::string new_name = target.stem().string();
            if (!new_name.empty() && new_name != active->getName()) {
                scene_manager.renameScene(active->getName(), new_name);
            }
            if (SceneSerializer::save(active, target)) {
                scene_file_actions.setCurrentScenePath(target);
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
            scene_manager.setActiveScene(scene_manager.createScene("Untitled"));

            scene_file_actions.setCurrentScenePath({});
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
            }
            ui_->onLoadScene();
            break;
        }
        case SceneFileAction::eNone:
            break;
    }
}

void Editor::destroy() {
    ui_.reset();
}

Editor::~Editor() {
    engine_ = nullptr;
}

}  // namespace wen::editor
