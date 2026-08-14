#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/widgets.hpp"
#include "ui/undo.hpp"
#include "ui/editor_scene.hpp"
#include "ui/ui_context.hpp"
#include "function/framework/component/material/material_component.hpp"
#include <filesystem>

namespace wen::editor {

// MaterialComponent 的自定义检查器 UI:基础色/金属度/粗糙度 + 纹理选择。
template <>
class ComponentView<MaterialComponent> {
public:
    ComponentView(MaterialComponent& material) : material_(material) {}
    MaterialComponent& getMaterial() { return material_; }

private:
    MaterialComponent& material_;
};

template <>
class ComponentUI<MaterialComponent> {
public:
    void render(ComponentView<MaterialComponent>& view, const std::function<void()>& on_remove = {}) {
        namespace fs = std::filesystem;
        auto& material = view.getMaterial();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("MaterialComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            // 基础色 / 金属度 / 粗糙度(入撤销栈)
            bool changed = false;
            glm::vec3 before_color = material.base_color;
            changed |= ImGui::ColorEdit3("base_color", &material.base_color.x);
            trackMemberEdit(&material, "base_color", before_color);

            float before_metallic = material.metallic;
            changed |= ImGui::DragFloat("metallic", &material.metallic, 0.01f, 0.0f, 1.0f);
            trackMemberEdit(&material, "metallic", before_metallic);

            float before_roughness = material.roughness;
            changed |= ImGui::DragFloat("roughness", &material.roughness, 0.01f, 0.0f, 1.0f);
            trackMemberEdit(&material, "roughness", before_roughness);
            if (changed) {
                material.triggerMemberUpdateCallbacks();
            }

            // 纹理:字段显示文件名,圆钮从 assets/textures 选择。
            const fs::path textures_dir = fs::path("engine/assets") / "textures";
            auto apply_texture = [&material](const std::string& relative_path) {
                std::string before = material.texture_path;
                material.setTexturePath(relative_path);
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(),
                                 {{"texture_path", before, relative_path}});
            };

            std::string display = material.texture_path.empty() ? "(none)" : material.texture_path;
            widgets::assetField(
                "##texture", display, kTextureDragDropPayload,
                /*on_drag*/ [&](const std::string& full_path) {
                    std::error_code ec;
                    auto relative = fs::relative(full_path, textures_dir, ec);
                    apply_texture(ec ? full_path : relative.generic_string());
                },
                /*draw_picker*/ [&]() {
                    std::error_code ec;
                    for (const auto& entry : fs::directory_iterator(textures_dir, ec)) {
                        auto ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(),
                                       [](unsigned char c) { return std::tolower(c); });
                        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp") {
                            continue;
                        }
                        if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                            std::error_code rel_ec;
                            auto relative = fs::relative(entry.path(), textures_dir, rel_ec);
                            apply_texture(rel_ec ? entry.path().filename().string()
                                                 : relative.generic_string());
                        }
                    }
                },
                /*on_activate*/ [&]() -> bool {
                    // 有纹理:定位到 Content Browser;空:点击无反应(拾取只由圆钮打开)。
                    if (material.texture_path.empty()) {
                        return true;
                    }
                    if (global_ui_context->reveal_asset_callback) {
                        global_ui_context->reveal_asset_callback(textures_dir / material.texture_path);
                    }
                    return true;
                });
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
};

}  // namespace wen::editor
