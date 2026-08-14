#pragma once

#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace wen::editor::widgets {

// 带激活高亮和悬停提示的按钮。active = true 时保持按下态高亮(播放/工具切换等 toggle 语义)。
inline bool toggleButton(const char* label, bool active, const char* tooltip,
                         const ImVec2& size = ImVec2(0.0f, 0.0f)) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    bool clicked = ImGui::Button(label, size);
    if (active) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

// 资产目录面包屑:从 root 到 current 逐段渲染,非当前段可点击跳转。
inline void breadcrumb(const std::filesystem::path& root,
                       const std::filesystem::path& current,
                       const std::function<void(const std::filesystem::path&)>& navigate) {
    std::error_code ec;
    std::vector<std::pair<std::string, std::filesystem::path>> segments;
    segments.push_back({"Assets", root});
    if (current != root) {
        if (auto relative = std::filesystem::relative(current, root, ec); !ec) {
            std::filesystem::path acc = root;
            for (const auto& part : relative) {
                acc /= part;
                segments.push_back({part.string(), acc});
            }
        }
    }

    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            ImGui::TextUnformatted("/");
            ImGui::SameLine(0.0f, 2.0f);
        }
        const bool is_current = (segments[i].second == current);
        if (is_current) {
            ImGui::TextUnformatted(segments[i].first.c_str());
        } else {
            if (ImGui::SmallButton(segments[i].first.c_str())) {
                navigate(segments[i].second);
            }
            ImGui::SameLine(0.0f, 2.0f);
        }
    }
}

// 目录条目排序:文件夹在前,再按文件名升序。
template <typename Entry, typename IsDir>
inline void sortFolderFirst(std::vector<Entry>& entries, IsDir is_dir) {
    std::sort(entries.begin(), entries.end(), [&](const Entry& a, const Entry& b) {
        if (is_dir(a) != is_dir(b)) {
            return is_dir(a);
        }
        return a.path.filename() < b.path.filename();
    });
}

// Unity 风格资源字段:显示资源名,右侧圆形拾取按钮,并接受拖放资源路径。
//   display      字段显示的文本(如网格名或 "(none)")。
//   drag_payload 接受的拖放载荷类型(如 kMeshDragDropPayload),载荷为路径字符串。
//   on_drag      拖放命中时回调(载荷字符串)。
//   draw_picker  拾取弹窗内容(点圆钮打开)。
//   on_activate  字段点击回调,返回 true 表示已处理(如定位到资源浏览器),
//                false/未提供则回退打开拾取器。
// 返回本帧是否通过拖放/拾取设置了一次资源。
inline bool assetField(const char* label,
                       const std::string& display,
                       const char* drag_payload,
                       const std::function<void(const std::string&)>& on_drag,
                       const std::function<void()>& draw_picker,
                       const std::function<bool()>& on_activate = {}) {
    ImGui::PushID(label);
    const float picker_size = 20.0f;
    bool changed = false;

    ImGui::BeginGroup();
    // 字段本身:有 on_activate 时点击定位资源,否则/未处理时打开拾取器;也是拖放目标。
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    if (ImGui::Button(display.c_str(), ImVec2(-picker_size, 0.0f))) {
        if (!on_activate || !on_activate()) {
            ImGui::OpenPopup("##asset_picker");
        }
    }
    ImGui::PopStyleVar();
    ImVec2 field_min = ImGui::GetItemRectMin();
    ImVec2 field_max = ImGui::GetItemRectMax();

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(drag_payload)) {
            if (on_drag) {
                std::string path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                on_drag(path);
                changed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    // 圆形拾取按钮(Unity 风格):描边圆 + 中心点。
    // 高度取帧高(与字段同高),InvisibleButton 不允许零尺寸。
    ImGui::InvisibleButton("##picker", ImVec2(picker_size, ImGui::GetFrameHeight()));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Select");
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImGui::OpenPopup("##asset_picker");
    }
    // 圆心取整行(字段 + 圆钮)的垂直中心,保证垂直居中。
    ImVec2 item_min = ImGui::GetItemRectMin();
    ImVec2 item_max = ImGui::GetItemRectMax();
    float center_y = (std::min(field_min.y, item_min.y) + std::max(field_max.y, item_max.y)) * 0.5f;
    float center_x = (item_min.x + item_max.x) * 0.5f;
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddCircle(ImVec2(center_x, center_y), picker_size * 0.35f, ImGui::GetColorU32(ImGuiCol_Text));
    dl->AddCircleFilled(ImVec2(center_x, center_y), picker_size * 0.12f, ImGui::GetColorU32(ImGuiCol_Text));

    if (ImGui::BeginPopup("##asset_picker")) {
        if (draw_picker) {
            draw_picker();
        }
        ImGui::EndPopup();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

}  // namespace wen::editor::widgets
