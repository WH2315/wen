#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/widgets.hpp"
#include "ui/undo.hpp"
#include "ui/editor_scene.hpp"
#include "ui/ui_context.hpp"
#include "function/framework/component/material/material_component.hpp"
#include "function/asset/material_asset.hpp"
#include <filesystem>
#include <algorithm>

namespace wen::editor {

template <>
class ComponentView<MaterialComponent> {
public:
    ComponentView(MaterialComponent& material) : material_(material) {}
    MaterialComponent& getMaterial() { return material_; }

private:
    MaterialComponent& material_;
};

// MaterialComponent 的统一检查器 UI:
//  - material_path 为空 → 直接编辑内置 PBR 参数(旧行为,入撤销栈)。
//  - 引用 builtin/pbr .mat → 编辑资产字段(写回 .mat,所有引用者共享)
//    + 实例级覆写开关(入撤销栈)。
//  - 引用自定义 shader .mat → 编辑自定义参数(写回 .mat,pass 热重载)。
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
            drawMaterialPath(material);
            if (material.material_path.empty()) {
                drawBuiltinParams(material);
            } else {
                const fs::path full_path = materialsDir() / material.material_path;
                CustomMaterialAsset asset;
                if (loadMaterialAsset(full_path.string(), asset)) {
                    if (asset.isBuiltinPbr()) {
                        drawPbrAssetEditor(material, asset, full_path.string());
                        drawOverrides(material);
                    } else {
                        drawCustomShaderEditor(asset, full_path.string());
                    }
                } else {
                    ImGui::TextDisabled("Cannot load .mat: %s", material.material_path.c_str());
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

private:
    static std::filesystem::path materialsDir() { return std::filesystem::path("engine/assets") / "materials"; }
    static std::filesystem::path texturesDir() { return std::filesystem::path("engine/assets") / "textures"; }

    static std::string relativeTexturePath(const std::string& full_path) {
        std::error_code ec;
        auto relative = std::filesystem::relative(full_path, texturesDir(), ec);
        return ec ? full_path : relative.generic_string();
    }

    // 纹理资源字段(拖放/拾取/定位),写入 value 并回调 on_change。
    static void drawTextureField(const char* label, std::string& value, const std::function<void()>& on_change) {
        namespace fs = std::filesystem;
        std::string display = value.empty() ? "(none)" : value;
        widgets::assetField(
            label, display, kTextureDragDropPayload,
            /*on_drag*/ [&](const std::string& full_path) {
                value = relativeTexturePath(full_path);
                on_change();
            },
            /*draw_picker*/ [&]() {
                std::error_code ec;
                for (const auto& entry : fs::directory_iterator(texturesDir(), ec)) {
                    auto ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp") {
                        continue;
                    }
                    if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                        std::error_code rel_ec;
                        auto relative = fs::relative(entry.path(), texturesDir(), rel_ec);
                        value = rel_ec ? entry.path().filename().string() : relative.generic_string();
                        on_change();
                    }
                }
            },
            /*on_activate*/ [&]() -> bool {
                if (value.empty()) {
                    return true;
                }
                if (global_ui_context->reveal_asset_callback) {
                    global_ui_context->reveal_asset_callback(texturesDir() / value);
                }
                return true;
            });
    }

    // 材质资产路径字段:空 = 内置参数,非空 = .mat 资产。
    void drawMaterialPath(MaterialComponent& material) {
        namespace fs = std::filesystem;
        constexpr const char* kMatDragDropPayload = "WEN_MAT_PATH";
        auto apply = [&material](const std::string& relative_path) {
            std::string before = material.material_path;
            material.material_path = relative_path;
            pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(),
                             {{"material_path", before, relative_path}});
            material.triggerMemberUpdateCallbacks();
        };
        std::string display = material.material_path.empty() ? "(builtin)" : material.material_path;
        widgets::assetField(
            "##material_path", display, kMatDragDropPayload,
            /*on_drag*/ [&](const std::string& full_path) {
                std::error_code ec;
                auto relative = fs::relative(full_path, materialsDir(), ec);
                apply(ec ? full_path : relative.generic_string());
            },
            /*draw_picker*/ [&]() {
                // "(builtin)" 清空路径,回到内置参数模式。
                if (ImGui::MenuItem("(builtin)")) {
                    apply("");
                }
                std::error_code ec;
                for (const auto& entry : fs::directory_iterator(materialsDir(), ec)) {
                    auto ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (ext != ".mat") {
                        continue;
                    }
                    if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                        std::error_code rel_ec;
                        auto relative = fs::relative(entry.path(), materialsDir(), rel_ec);
                        apply(rel_ec ? entry.path().filename().string() : relative.generic_string());
                    }
                }
            },
            /*on_activate*/ [&]() -> bool {
                if (material.material_path.empty()) {
                    return true;
                }
                if (global_ui_context->reveal_asset_callback) {
                    global_ui_context->reveal_asset_callback(materialsDir() / material.material_path);
                }
                return true;
            });
    }

    // material_path 为空:直接编辑内置 PBR 参数(入撤销栈)。
    void drawBuiltinParams(MaterialComponent& material) {
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

        glm::vec3 before_emissive = material.emissive_color;
        changed |= ImGui::ColorEdit3("emissive_color", &material.emissive_color.x);
        trackMemberEdit(&material, "emissive_color", before_emissive);

        float before_emissive_intensity = material.emissive_intensity;
        changed |= ImGui::DragFloat("emissive_intensity", &material.emissive_intensity, 0.01f, 0.0f, 10.0f);
        trackMemberEdit(&material, "emissive_intensity", before_emissive_intensity);

        glm::vec2 before_tiling = material.tiling;
        changed |= ImGui::DragFloat2("tiling", &material.tiling.x, 0.01f, 0.1f, 100.0f);
        trackMemberEdit(&material, "tiling", before_tiling);

        float before_normal_scale = material.normal_scale;
        changed |= ImGui::DragFloat("normal_scale", &material.normal_scale, 0.01f, 0.0f, 10.0f);
        trackMemberEdit(&material, "normal_scale", before_normal_scale);

        float before_ao_intensity = material.ao_intensity;
        changed |= ImGui::DragFloat("ao_intensity", &material.ao_intensity, 0.01f, 0.0f, 1.0f);
        trackMemberEdit(&material, "ao_intensity", before_ao_intensity);

        // 纹理字段:非空路径即覆写(此处即直接生效)。
        auto texture_field = [&](const char* label, std::string& value, const char* member) {
            std::string before = value;
            drawTextureField(label, value, [&material]() { material.applyToMeshInstance(); });
            if (value != before) {
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(),
                                 {{member, before, value}});
            }
        };
        texture_field("##texture", material.texture_path, "texture_path");
        texture_field("##normal_map", material.normal_map_path, "normal_map_path");
        texture_field("##mr_map", material.mr_map_path, "mr_map_path");
        texture_field("##ao_map", material.ao_map_path, "ao_map_path");

        if (changed) {
            material.triggerMemberUpdateCallbacks();
        }
    }

    // 引用 builtin/pbr .mat:编辑资产字段(写回文件,所有引用者共享)。
    void drawPbrAssetEditor(MaterialComponent& material, CustomMaterialAsset& asset, const std::string& full_path) {
        bool changed = false;
        changed |= ImGui::ColorEdit3("base_color", &asset.base_color.x);
        changed |= ImGui::DragFloat("metallic", &asset.metallic, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("roughness", &asset.roughness, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::ColorEdit3("emissive_color", &asset.emissive_color.x);
        changed |= ImGui::DragFloat("emissive_intensity", &asset.emissive_intensity, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat2("tiling", &asset.tiling.x, 0.01f, 0.1f, 100.0f);
        changed |= ImGui::DragFloat("normal_scale", &asset.normal_scale, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("ao_intensity", &asset.ao_intensity, 0.01f, 0.0f, 1.0f);

        auto asset_texture = [&](const char* label, std::string& value) {
            std::string before = value;
            drawTextureField(label, value, [] {});
            if (value != before) {
                changed = true;
            }
        };
        asset_texture("##asset_texture", asset.texture_path);
        asset_texture("##asset_normal_map", asset.normal_map_path);
        asset_texture("##asset_mr_map", asset.mr_map_path);
        asset_texture("##asset_ao_map", asset.ao_map_path);

        if (changed) {
            saveMaterialAsset(full_path, asset);
            // 资产已落盘,把新值刷进本对象的网格实例(其他对象在下次应用时同步)。
            material.triggerMemberUpdateCallbacks();
        }
    }

    // 引用自定义 shader .mat:编辑自定义参数(写回文件,CustomMaterialPass 热重载)。
    void drawCustomShaderEditor(CustomMaterialAsset& asset, const std::string& full_path) {
        bool changed = false;
        changed |= ImGui::ColorEdit3("base_color", &asset.base_color.x);
        changed |= ImGui::DragFloat("intensity", &asset.intensity, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("mode", &asset.mode, 0.01f, 0.0f, 10.0f);
        changed |= ImGui::Checkbox("wireframe", &asset.wireframe);
        // 背面剔除:outline 常用 front(剔除正面画黑边),布片/双面用 none。
        const char* cull_labels[] = {"none", "front", "back"};
        int cull_idx = asset.cull == "front" ? 1 : (asset.cull == "back" ? 2 : 0);
        changed |= ImGui::Combo("cull", &cull_idx, cull_labels, 3);
        asset.cull = cull_labels[cull_idx];
        for (auto& param : asset.params) {
            if (param.is_color) {
                changed |= ImGui::ColorEdit3(param.name.c_str(), &param.value.x);
            } else {
                changed |= ImGui::DragFloat(param.name.c_str(), &param.value.x, 0.01f, -10.0f, 10.0f);
            }
        }
        if (changed) {
            saveMaterialAsset(full_path, asset);  // 下一帧 pass 自动热重载
        }
    }

    // 引用 builtin/pbr .mat 时的实例级覆写:勾选后对应字段覆盖资产值;纹理非空即覆写。
    void drawOverrides(MaterialComponent& material) {
        ImGui::Separator();
        ImGui::TextUnformatted("Overrides");
        bool changed = false;

        auto override_color = [&](const char* label, bool& flag, glm::vec3& value) {
            ImGui::PushID(label);
            changed |= ImGui::Checkbox("##ov", &flag);
            ImGui::SameLine();
            ImGui::BeginDisabled(!flag);
            glm::vec3 before = value;
            changed |= ImGui::ColorEdit3(label, &value.x);
            trackMemberEdit(&material, label, before);
            ImGui::EndDisabled();
            ImGui::PopID();
        };
        auto override_float = [&](const char* label, bool& flag, float& value, float speed, float mn, float mx) {
            ImGui::PushID(label);
            changed |= ImGui::Checkbox("##ov", &flag);
            ImGui::SameLine();
            ImGui::BeginDisabled(!flag);
            float before = value;
            changed |= ImGui::DragFloat(label, &value, speed, mn, mx);
            trackMemberEdit(&material, label, before);
            ImGui::EndDisabled();
            ImGui::PopID();
        };
        auto override_vec2 = [&](const char* label, bool& flag, glm::vec2& value) {
            ImGui::PushID(label);
            changed |= ImGui::Checkbox("##ov", &flag);
            ImGui::SameLine();
            ImGui::BeginDisabled(!flag);
            glm::vec2 before = value;
            changed |= ImGui::DragFloat2(label, &value.x, 0.01f, 0.1f, 100.0f);
            trackMemberEdit(&material, label, before);
            ImGui::EndDisabled();
            ImGui::PopID();
        };
        auto override_texture = [&](const char* label, std::string& value, const char* member) {
            ImGui::PushID(label);
            ImGui::TextDisabled("%s (non-empty overrides)", label);
            std::string before = value;
            drawTextureField(label, value, [&material]() { material.applyToMeshInstance(); });
            if (value != before) {
                pushPropertyEdit(material.getGameObject()->getUUID(), material.getClassName(),
                                 {{member, before, value}});
            }
            ImGui::PopID();
        };

        override_color("base_color", material.override_base_color, material.base_color);
        override_float("metallic", material.override_metallic, material.metallic, 0.01f, 0.0f, 1.0f);
        override_float("roughness", material.override_roughness, material.roughness, 0.01f, 0.0f, 1.0f);
        override_color("emissive_color", material.override_emissive_color, material.emissive_color);
        override_float("emissive_intensity", material.override_emissive_intensity, material.emissive_intensity,
                       0.01f, 0.0f, 10.0f);
        override_vec2("tiling", material.override_tiling, material.tiling);
        override_float("normal_scale", material.override_normal_scale, material.normal_scale, 0.01f, 0.0f, 10.0f);
        override_float("ao_intensity", material.override_ao_intensity, material.ao_intensity, 0.01f, 0.0f, 1.0f);

        override_texture("texture", material.texture_path, "texture_path");
        override_texture("normal_map", material.normal_map_path, "normal_map_path");
        override_texture("mr_map", material.mr_map_path, "mr_map_path");
        override_texture("ao_map", material.ao_map_path, "ao_map_path");

        if (changed) {
            material.triggerMemberUpdateCallbacks();
        }
    }
};

}  // namespace wen::editor
