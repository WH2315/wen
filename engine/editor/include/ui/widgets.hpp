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

}  // namespace wen::editor::widgets
