#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/widgets.hpp"
#include "ui/undo.hpp"
#include "ui/editor_scene.hpp"
#include "ui/ui_context.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include <filesystem>

namespace wen::editor {

// MeshComponent 的自定义检查器 UI:Unity 风格资源字段,支持从 Content Browser
// 拖入 .obj 或点圆形按钮从 assets/models 选择。
template <>
class ComponentView<MeshComponent> {
public:
    ComponentView(MeshComponent& mesh) : mesh_(mesh) {}
    MeshComponent& getMesh() { return mesh_; }

private:
    MeshComponent& mesh_;
};

template <>
class ComponentUI<MeshComponent> {
public:
    void render(ComponentView<MeshComponent>& view, const std::function<void()>& on_remove = {}) {
        namespace fs = std::filesystem;
        auto& mesh = view.getMesh();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            const fs::path models_dir = fs::path("engine/assets") / "models";

            // 设置网格:改路径 + 立即重载 + 入撤销栈(undo/redo 也经回调重载)。
            auto apply_mesh = [&mesh](const std::string& relative_path) {
                std::string before = mesh.mesh_path;
                mesh.setMeshPath(relative_path);
                pushPropertyEdit(mesh.getGameObject()->getUUID(), mesh.getClassName(),
                                 {{"mesh_path", before, relative_path}});
            };
            auto rel_to_models = [&models_dir](const fs::path& full_path) {
                std::error_code ec;
                auto relative = fs::relative(full_path, models_dir, ec);
                return ec ? full_path.generic_string() : relative.generic_string();
            };

            std::string display = mesh.mesh_path.empty() ? "(none)" : mesh.mesh_path;
            widgets::assetField(
                "##mesh", display, kMeshDragDropPayload,
                /*on_drag*/ [&](const std::string& full_path) { apply_mesh(rel_to_models(full_path)); },
                /*draw_picker*/ [&]() {
                    std::error_code ec;
                    for (const auto& entry : fs::directory_iterator(models_dir, ec)) {
                        if (entry.path().extension() != ".obj") {
                            continue;
                        }
                        if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                            apply_mesh(rel_to_models(entry.path()));
                        }
                    }
                },
                /*on_activate*/ [&]() -> bool {
                    if (mesh.mesh_path.empty()) {
                        return true;
                    }
                    if (global_ui_context->reveal_asset_callback) {
                        global_ui_context->reveal_asset_callback(models_dir / mesh.mesh_path);
                    }
                    return true;
                });
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
};

}  // namespace wen::editor
