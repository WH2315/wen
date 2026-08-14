#pragma once

#include "ui/component_ui/component_ui_manager.hpp"
#include "ui/undo.hpp"
#include "function/framework/component/script/script_component.hpp"
#include "function/script/script_registry.hpp"
#include "engine/global_context.hpp"
#include <filesystem>
#include <type_traits>
#include <algorithm>

namespace wen::editor {

// ScriptComponent 自定义检查器:类型下拉(Native/Lua)+ 脚本下拉(按类型分流)+
// 脚本字段编辑。字段是脚本自描述的 ScriptValue 集合,编辑后经 setField 写回脚本
// (Lua 同步回 fields 表)并推 ScriptFieldEditCommand 入撤销栈。
template <>
class ComponentView<ScriptComponent> {
public:
    ComponentView(ScriptComponent& component) : component_(component) {}
    ScriptComponent& getComponent() { return component_; }

private:
    ScriptComponent& component_;
};

namespace {

std::vector<std::string> listLuaScripts() {
    std::vector<std::string> out;
    auto scripts_dir = std::filesystem::path(global_context->asset_system->getRootDir()) / "scripts";
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(scripts_dir, ec)) {
        if (entry.path().extension() == ".lua") {
            out.push_back(entry.path().filename().string());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

}  // namespace

template <>
class ComponentUI<ScriptComponent> {
public:
    void render(ComponentView<ScriptComponent>& view, const std::function<void()>& on_remove = {}) {
        auto& component = view.getComponent();
        ImGui::PushID(&view);

        bool open = ImGui::TreeNodeEx("ScriptComponent", ImGuiTreeNodeFlags_DefaultOpen);
        if (on_remove) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) {
                on_remove();
            }
        }
        if (open) {
            renderScriptSelector(&component);
            if (auto* script = component.script()) {
                ImGui::Separator();
                for (auto& field : script->fields()) {
                    renderScriptField(&component, field);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

private:
    void renderScriptSelector(ScriptComponent* component) {
        renderTypeCombo(component);
        if (component->script_type == "Lua") {
            renderScriptCombo(component, listLuaScripts());
        } else {
            renderScriptCombo(component, global_context->script_registry->names());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Reload")) {
            component->reload();
        }
    }

    void renderTypeCombo(ScriptComponent* component) {
        bool is_lua = (component->script_type == "Lua");
        if (ImGui::BeginCombo("type", is_lua ? "Lua" : "Native")) {
            if (ImGui::Selectable("Native", !is_lua)) {
                if (is_lua) {
                    std::string before = component->script_type;
                    component->script_type = "Native";
                    component->reload();
                    pushPropertyEdit(component->getGameObject()->getUUID(), component->getClassName(),
                                     {{"script_type", before, std::string("Native")}});
                }
            }
            if (ImGui::Selectable("Lua", is_lua)) {
                if (!is_lua) {
                    std::string before = component->script_type;
                    component->script_type = "Lua";
                    component->reload();
                    pushPropertyEdit(component->getGameObject()->getUUID(), component->getClassName(),
                                     {{"script_type", before, std::string("Lua")}});
                }
            }
            ImGui::EndCombo();
        }
    }

    void renderScriptCombo(ScriptComponent* component, const std::vector<std::string>& names) {
        std::string current = component->script_name.empty() ? "(none)" : component->script_name;
        auto uuid = component->getGameObject()->getUUID();
        auto class_name = component->getClassName();
        if (ImGui::BeginCombo("script", current.c_str())) {
            if (ImGui::Selectable("(none)", component->script_name.empty())) {
                if (!component->script_name.empty()) {
                    std::string before = component->script_name;
                    component->setScriptName("");
                    pushPropertyEdit(uuid, class_name, {{"script_name", before, std::string("")}});
                }
            }
            for (const auto& name : names) {
                bool selected = (name == component->script_name);
                if (ImGui::Selectable(name.c_str(), selected)) {
                    if (name != component->script_name) {
                        std::string before = component->script_name;
                        component->setScriptName(name);
                        pushPropertyEdit(uuid, class_name, {{"script_name", before, name}});
                    }
                }
            }
            ImGui::EndCombo();
        }
    }

    void renderScriptField(ScriptComponent* component, ScriptField& field) {
        ImGui::PushID(field.name.c_str());
        auto* script = component->script();
        auto uuid = component->getGameObject()->getUUID();
        std::visit([&](auto& v) {
            using T = std::decay_t<decltype(v)>;
            T before = v;
            if constexpr (std::is_same_v<T, int>) {
                ImGui::DragInt(field.name.c_str(), &v);
            } else if constexpr (std::is_same_v<T, float>) {
                ImGui::DragFloat(field.name.c_str(), &v, 0.1f);
            } else if constexpr (std::is_same_v<T, bool>) {
                ImGui::Checkbox(field.name.c_str(), &v);
            } else if constexpr (std::is_same_v<T, glm::vec2>) {
                ImGui::DragFloat2(field.name.c_str(), &v.x, 0.1f);
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                ImGui::DragFloat3(field.name.c_str(), &v.x, 0.1f);
            } else if constexpr (std::is_same_v<T, glm::vec4>) {
                ImGui::DragFloat4(field.name.c_str(), &v.x, 0.1f);
            } else {
                ImGui::Text("%s: %s", field.name.c_str(), v.c_str());
                return;
            }
            // 手势结束:经 setField 统一写入(更新 C++ 字段并同步回 Lua),再入撤销栈。
            if (ImGui::IsItemActivated()) {
                pendingScriptFieldValue() = ScriptValue(before);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                script->setField(field.name, ScriptValue(v));
                pushScriptFieldEdit(uuid, field.name, pendingScriptFieldValue(), ScriptValue(v));
            }
        }, field.value);
        ImGui::PopID();
    }
};

}  // namespace wen::editor
