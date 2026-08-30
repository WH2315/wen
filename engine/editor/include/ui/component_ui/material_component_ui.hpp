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

            // 自发光:颜色 + 强度(入撤销栈)。
            glm::vec3 before_emissive = material.emissive_color;
            changed |= ImGui::ColorEdit3("emissive_color", &material.emissive_color.x);
            trackMemberEdit(&material, "emissive_color", before_emissive);

            float before_emissive_intensity = material.emissive_intensity;
            changed |= ImGui::DragFloat("emissive_intensity", &material.emissive_intensity,
                                        0.01f, 0.0f, 10.0f);
            trackMemberEdit(&material, "emissive_intensity", before_emissive_intensity);

            // UV 平铺。
            glm::vec2 before_tiling = material.tiling;
            changed |= ImGui::DragFloat2("tiling", &material.tiling.x, 0.01f, 0.1f, 100.0f);
            trackMemberEdit(&material, "tiling", before_tiling);

            // 法线贴图强度。
            float before_normal_scale = material.normal_scale;
            changed |= ImGui::DragFloat("normal_scale", &material.normal_scale, 0.01f, 0.0f, 10.0f);
            trackMemberEdit(&material, "normal_scale", before_normal_scale);

            // AO 强度。
            float before_ao_intensity = material.ao_intensity;
            changed |= ImGui::DragFloat("ao_intensity", &material.ao_intensity, 0.01f, 0.0f, 1.0f);
            trackMemberEdit(&material, "ao_intensity", before_ao_intensity);
            if (changed) {
                material.triggerMemberUpdateCallbacks();
            }

            // 法线贴图:字段显示文件名,圆钮从 assets/textures 选择。
            const fs::path textures_dir = fs::path("engine/assets") / "textures";
            auto apply_normal = [&material](const std::string& relative_path) {
                std::string before = material.normal_map_path;
                material.setNormalMapPath(relative_path);
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(),
                                 {{"normal_map_path", before, relative_path}});
            };
            std::string normal_display = material.normal_map_path.empty() ? "(none)" : material.normal_map_path;
            widgets::assetField(
                "##normal_map", normal_display, kTextureDragDropPayload,
                /*on_drag*/ [&](const std::string& full_path) {
                    std::error_code ec;
                    auto relative = fs::relative(full_path, textures_dir, ec);
                    apply_normal(ec ? full_path : relative.generic_string());
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
                            apply_normal(rel_ec ? entry.path().filename().string()
                                                : relative.generic_string());
                        }
                    }
                },
                /*on_activate*/ [&]() -> bool {
                    if (material.normal_map_path.empty()) {
                        return true;
                    }
                    if (global_ui_context->reveal_asset_callback) {
                        global_ui_context->reveal_asset_callback(textures_dir / material.normal_map_path);
                    }
                    return true;
                });

            // metallic-roughness 贴图(B=金属, G=粗糙度)。
            auto draw_texture_picker = [&](const std::function<void(const std::string&)>& apply) {
                for (const auto& entry : fs::directory_iterator(textures_dir, {})) {
                    auto ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp") {
                        continue;
                    }
                    if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                        std::error_code rel_ec;
                        auto relative = fs::relative(entry.path(), textures_dir, rel_ec);
                        apply(rel_ec ? entry.path().filename().string() : relative.generic_string());
                    }
                }
            };
            auto apply_mr = [&material](const std::string& p) {
                std::string before = material.mr_map_path;
                material.setMrMapPath(p);
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(), {{"mr_map_path", before, p}});
            };
            widgets::assetField("##mr_map", material.mr_map_path.empty() ? "(none)" : material.mr_map_path,
                                kTextureDragDropPayload,
                                [&](const std::string& full_path) {
                                    std::error_code ec;
                                    auto relative = fs::relative(full_path, textures_dir, ec);
                                    apply_mr(ec ? full_path : relative.generic_string());
                                },
                                [&]() { draw_texture_picker(apply_mr); },
                                [&]() -> bool {
                                    if (!material.mr_map_path.empty() && global_ui_context->reveal_asset_callback) {
                                        global_ui_context->reveal_asset_callback(textures_dir / material.mr_map_path);
                                    }
                                    return true;
                                });

            // AO 贴图。
            auto apply_ao = [&material](const std::string& p) {
                std::string before = material.ao_map_path;
                material.setAoMapPath(p);
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(), {{"ao_map_path", before, p}});
            };
            widgets::assetField("##ao_map", material.ao_map_path.empty() ? "(none)" : material.ao_map_path,
                                kTextureDragDropPayload,
                                [&](const std::string& full_path) {
                                    std::error_code ec;
                                    auto relative = fs::relative(full_path, textures_dir, ec);
                                    apply_ao(ec ? full_path : relative.generic_string());
                                },
                                [&]() { draw_texture_picker(apply_ao); },
                                [&]() -> bool {
                                    if (!material.ao_map_path.empty() && global_ui_context->reveal_asset_callback) {
                                        global_ui_context->reveal_asset_callback(textures_dir / material.ao_map_path);
                                    }
                                    return true;
                                });

            // 纹理:字段显示文件名,圆钮从 assets/textures 选择。
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
