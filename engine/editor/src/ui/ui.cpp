#include "ui/ui.hpp"
#include "ui/ui_context.hpp"
#include "ui/menu_bar.hpp"
#include "ui/file_dialog.hpp"
#include "ui/toolbar.hpp"
#include "ui/undo.hpp"
#include "ui/panels/setting_panel.hpp"
#include "ui/panels/hierarchy_panel.hpp"
#include "ui/panels/viewport_panel.hpp"
#include "ui/panels/inspector_panel.hpp"
#include "ui/panels/content_browser_panel.hpp"
#include "engine/global_context.hpp"
#include "function/window/window_system.hpp"
#include "function/window/window.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include "core/math/transform_math.hpp"

namespace wen::editor {

UI::UI() {
    global_ui_context = new UIContext();

    undo_stack_ = std::make_unique<UndoStack>();
    global_undo_stack = undo_stack_.get();

    file_dialog_ = std::make_unique<FileDialog>();
    global_ui_context->file_dialog = file_dialog_.get();

    viewport_camera_ = std::make_unique<ViewportCamera>();
    menu_bar_ = std::make_unique<MenuBar>();
    toolbar_ = std::make_unique<Toolbar>();

    auto viewport_panel = std::make_unique<ViewportPanel>();
    auto hierarchy_panel = std::make_unique<HierarchyPanel>();
    auto content_browser_panel = std::make_unique<ContentBrowserPanel>();
    // 视口拾取/资源生成的对象统一经 Hierarchy 设置选中。
    auto select_callback = [ptr = hierarchy_panel.get()](GameObjectUUID uuid) {
        ptr->selectGameObject(uuid);
    };
    viewport_panel->setOnSelectGameObject(select_callback);
    content_browser_panel->setOnSelectGameObject(select_callback);

    // Inspector 对象字段点击"揭示":定位到 Content Browser 的对应资源。
    auto* content_browser = content_browser_panel.get();
    global_ui_context->reveal_asset_callback = [content_browser](const std::filesystem::path& path) {
        content_browser->revealAsset(path);
    };

    registerPanel(std::move(viewport_panel));
    registerPanel(std::move(hierarchy_panel));
    registerPanel(std::make_unique<InspectorPanel>());
    registerPanel(std::make_unique<SettingPanel>());
    registerPanel(std::move(content_browser_panel));
}

void UI::registerPanel(std::unique_ptr<Panel> panel) {
    panels_.push_back(std::move(panel));
}

UI::~UI() {
    panels_.clear();
    viewport_camera_.reset();
    menu_bar_.reset();
    toolbar_.reset();

    file_dialog_.reset();
    undo_stack_.reset();
    global_undo_stack = nullptr;
    delete global_ui_context;
    global_ui_context = nullptr;
}

void UI::onLoadScene() {

    global_undo_stack->clear();
    viewport_camera_->reset();
    for (auto& panel : panels_) {
        panel->onLoadScene();
    }
}

void UI::onUnloadScene() {
    for (auto& panel : panels_) {
        panel->onUnloadScene();
    }
}

void UI::onPrefabReverted() {
    for (auto& panel : panels_) {
        panel->onPrefabReverted();
    }
}

void UI::onFrame() {
    auto& io = ImGui::GetIO();
    if (global_ui_context->mode == Mode::eEdit) {
        viewport_camera_->onTick(io.DeltaTime, global_ui_context->viewport_hovered, ImGuizmo::IsUsing());
        global_ui_context->viewport_flying = viewport_camera_->isFlying();
        handleFocusShortcut();
    } else {
        global_ui_context->viewport_flying = false;
    }
    updateWindowTitle();
    render();
}

// 窗口标题 = "<场景名> - wen",未保存时追加 "*"。仅在变化时更新。
void UI::updateWindowTitle() {
    std::string title = "wen";
    if (auto* scene = global_context->scene_manager->getActiveScene()) {
        title = scene->getName() + " - wen";
    }
    if (global_ui_context->scene_file_actions.isDirty()) {
        title += " *";
    }
    if (title != window_title_) {
        window_title_ = title;
        if (auto* window = global_context->window_system->getRuntimeWindow()) {
            window->setTitle(title);
        }
    }
}

// 悬停视口时按 F 框选:以选中对象(或其网格包围盒)为中心聚焦编辑器相机。
void UI::handleFocusShortcut() {
    if (!global_ui_context->viewport_hovered ||
        viewport_camera_->isFlying() ||
        ImGui::GetIO().WantTextInput ||
        !ImGui::IsKeyPressed(ImGuiKey_F, false) ||
        global_ui_context->selected_game_object_uuid == kInvalidGameObjectUUID) {
        return;
    }

    auto* scene = global_context->scene_manager->getActiveScene();
    auto* game_object = scene ? scene->getGameObject(global_ui_context->selected_game_object_uuid) : nullptr;
    auto* transform = game_object ? game_object->queryComponent<TransformComponent>() : nullptr;
    if (transform == nullptr) {
        return;
    }

    glm::vec3 center = transform->location;
    float radius = 1.0f;
    if (auto* mesh = game_object->queryComponent<MeshComponent>()) {
        const auto& descriptor = global_context->asset_system->getMeshPool()->mesh_descriptor_buffer_ptr[mesh->mesh_id];
        // 包围盒中心经模型矩阵变换到世界空间,半径按最大缩放修正。
        glm::vec3 local_center = (descriptor.aabb_min + descriptor.aabb_max) * 0.5f;
        center = transform->location + math::composeModel(transform->rotation, transform->scale) * local_center;
        float max_scale = std::max({std::abs(transform->scale.x),
                                    std::abs(transform->scale.y),
                                    std::abs(transform->scale.z)});
        radius = std::max(descriptor.radius * max_scale, 0.1f);
    }

    viewport_camera_->focusOn(center, radius);
}

void UI::render() {
    ImGuizmo::BeginFrame();

    menu_bar_->render();
    toolbar_->render();

    // Dockspace 宿主窗口:铺满工具栏下方的剩余区域。
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float toolbar_height = Toolbar::height();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + toolbar_height));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - toolbar_height));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockSpaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    for (auto& panel : panels_) {
        panel->render();
    }

    ImGui::End();
}

}  // namespace wen::editor
