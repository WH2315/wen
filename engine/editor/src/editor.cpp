#include "editor.hpp"
#include "ui/ui_context.hpp"
#include "ui/panels/content_browser/content_browser_panel.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

Editor::Editor(Engine* engine) : engine_(engine) {}

void Editor::initialize() {
    ui_ = std::make_unique<UI>();

    // Edit 模式暂停引擎逻辑，用编辑器相机驱动视图
    // Game 模式恢复逻辑，把视图交还给场景相机
    global_ui_context->change_mode_callback = [this](Mode mode) {
        switch (mode) {
            case Mode::eEdit:
                global_ui_context->mode = Mode::eEdit;
                engine_->stopTimer();
                global_context->camera_system->activeEditorCamera(ui_->getViewportCameraID());
                break;
            case Mode::eGame:
                global_ui_context->mode = Mode::eGame;
                global_context->camera_system->deactiveEditorCamera();
                engine_->startTimer();
                break;
        }
    };

    // 回调在每帧的渲染通道内执行
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

void Editor::destroy() {
    ui_.reset();
}

Editor::~Editor() {
    engine_ = nullptr;
}

}  // namespace wen::editor
