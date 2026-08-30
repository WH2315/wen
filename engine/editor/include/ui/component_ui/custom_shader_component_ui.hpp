#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/widgets.hpp"
#include "ui/undo.hpp"
#include "ui/ui_context.hpp"
#include "function/framework/component/custom_shader/custom_shader_component.hpp"
#include "function/asset/material_asset.hpp"
#include <filesystem>

namespace wen::editor {

template <>
class ComponentView<CustomShaderComponent> {
public:
    ComponentView(CustomShaderComponent& component) : component_(component) {}
    CustomShaderComponent& getComponent() { return component_; }

private:
    CustomShaderComponent& component_;
};

// CustomShaderComponent 的检查器 UI:选择 .mat 资产 + 通用参数编辑。
// 参数编辑会写回 .mat 文件;CustomMaterialPass 检测到文件变化后下一帧重刷到 GPU。
template <>
class ComponentUI<CustomShaderComponent> {
public:
    void render(ComponentView<CustomShaderComponent>& view, const std::function<void()>& on_remove = {}) {
        namespace fs = std::filesystem;
        auto& component = view.getComponent();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("CustomShaderComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            const fs::path materials_dir = fs::path("engine/assets") / "materials";
            constexpr const char* kMatDragDropPayload = "WEN_MAT_PATH";

            auto apply = [&component](const std::string& relative_path) {
                std::string before = component.material_path;
                component.material_path = relative_path;
                pushPropertyEdit(component.getGameObject()->getUUID(), component.getClassName(),
                                 {{"material_path", before, relative_path}});
                component.triggerMemberUpdateCallbacks();
            };

            std::string display = component.material_path.empty() ? "(none)" : component.material_path;
            widgets::assetField(
                "##material_path", display, kMatDragDropPayload,
                /*on_drag*/ [&](const std::string& full_path) {
                    std::error_code ec;
                    auto relative = fs::relative(full_path, materials_dir, ec);
                    apply(ec ? full_path : relative.generic_string());
                },
                /*draw_picker*/ [&]() {
                    std::error_code ec;
                    for (const auto& entry : fs::directory_iterator(materials_dir, ec)) {
                        auto ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(),
                                       [](unsigned char c) { return std::tolower(c); });
                        if (ext != ".mat") {
                            continue;
                        }
                        if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                            std::error_code rel_ec;
                            auto relative = fs::relative(entry.path(), materials_dir, rel_ec);
                            apply(rel_ec ? entry.path().filename().string()
                                         : relative.generic_string());
                        }
                    }
                },
                /*on_activate*/ [&]() -> bool {
                    if (component.material_path.empty()) {
                        return true;
                    }
                    if (global_ui_context->reveal_asset_callback) {
                        global_ui_context->reveal_asset_callback(materials_dir / component.material_path);
                    }
                    return true;
                });

            // ---- 通用参数编辑 ----
            if (!component.material_path.empty()) {
                const fs::path full_path = materials_dir / component.material_path;
                CustomMaterialAsset asset;
                if (loadMaterialAsset(full_path.string(), asset)) {
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
                            changed |= ImGui::DragFloat(param.name.c_str(), &param.value.x,
                                                        0.01f, -10.0f, 10.0f);
                        }
                    }
                    if (changed) {
                        saveMaterialAsset(full_path.string(), asset);  // 下一帧 pass 自动重刷
                    }
                } else {
                    ImGui::TextDisabled("Cannot load .mat: %s", component.material_path.c_str());
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
};

}  // namespace wen::editor