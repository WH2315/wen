#include "ui/ui.hpp"
#include "ui/ui_context.hpp"
#include "ui/menu_bar.hpp"
#include "ui/toolbar.hpp"
#include "ui/panels/setting/setting_panel.hpp"
#include "ui/panels/hierarchy/hierarchy_panel.hpp"
#include "ui/panels/viewport/viewport_panel.hpp"
#include "ui/panels/inspector/inspector_panel.hpp"
#include "ui/panels/content_browser/content_browser_panel.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include "core/math/transform_math.hpp"

namespace wen::editor {

UI::UI() {
    global_ui_context = new UIContext();

    viewport_camera_ = std::make_unique<ViewportCamera>();
    menu_bar_ = std::make_unique<MenuBar>();
    toolbar_ = std::make_unique<Toolbar>();

    auto viewport_panel = std::make_unique<ViewportPanel>();
    auto hierarchy_panel = std::make_unique<HierarchyPanel>();
    auto content_browser_panel = std::make_unique<ContentBrowserPanel>();
    auto select_callback = [ptr = hierarchy_panel.get()](GameObjectUUID uuid) {
        ptr->selectGameObject(uuid);
    };
    viewport_panel->setOnSelectGameObject(select_callback);
    content_browser_panel->setOnSelectGameObject(select_callback);
    panels_.push_back(std::move(viewport_panel));
    panels_.push_back(std::move(hierarchy_panel));
    panels_.push_back(std::make_unique<InspectorPanel>());
    panels_.push_back(std::make_unique<SettingPanel>());
    panels_.push_back(std::move(content_browser_panel));
}

UI::~UI() {
    panels_.clear();
    viewport_camera_.reset();
    menu_bar_.reset();
    toolbar_.reset();
    delete global_ui_context;
    global_ui_context = nullptr;
}

void UI::onLoadScene() {
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

void UI::onFrame() {
    auto& io = ImGui::GetIO();
    if (global_ui_context->mode == Mode::eEdit) {
        viewport_camera_->onTick(io.DeltaTime, global_ui_context->viewport_hovered, ImGuizmo::IsUsing());
        global_ui_context->viewport_flying = viewport_camera_->isFlying();
        handleFocusShortcut();
    } else {
        global_ui_context->viewport_flying = false;
    }
    render();
}

// 悬停 Viewport 时按 F 框选当前选中的游戏对象
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
