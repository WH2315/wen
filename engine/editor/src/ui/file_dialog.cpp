#include "ui/file_dialog.hpp"
#include "ui/icons.hpp"
#include "ui/widgets.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace wen::editor {

namespace fs = std::filesystem;

namespace {

bool sameExtension(const fs::path& path, const std::string& ext) {
    std::string actual = path.extension().string();
    std::string target = ext;
    std::transform(actual.begin(), actual.end(), actual.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::transform(target.begin(), target.end(), target.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return actual == target;
}

std::string trimmed(const char* text) {
    std::string s(text);
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

}  // namespace

void FileDialog::open(Mode mode, const std::string& title, const fs::path& initial_dir,
                      const std::string& filter_ext) {
    mode_ = mode;
    title_ = title;
    filter_ext_ = filter_ext.empty() ? ".scene" : filter_ext;

    std::error_code ec;
    fs::path dir = fs::is_directory(initial_dir, ec) ? initial_dir : asset_root_;
    current_dir_ = fs::weakly_canonical(dir, ec);
    if (ec || !isWithinRoot(current_dir_)) {
        current_dir_ = asset_root_;
    }
    selected_path_.clear();
    confirm_requested_ = false;
    focus_file_name_ = (mode == Mode::eSave);
    result_pending_ = false;

    if (default_file_name_[0] != '\0') {
        std::snprintf(file_name_, sizeof(file_name_), "%s", default_file_name_);
        file_name_[sizeof(file_name_) - 1] = '\0';
    } else {
        file_name_[0] = '\0';
    }
    open_ = true;
}

void FileDialog::setDefaultFileName(const std::string& name) {
    std::snprintf(default_file_name_, sizeof(default_file_name_), "%s", name.c_str());
    default_file_name_[sizeof(default_file_name_) - 1] = '\0';
}

void FileDialog::close() {
    open_ = false;
}

bool FileDialog::isWithinRoot(const fs::path& path) const {
    std::error_code ec;
    auto relative = fs::relative(path, asset_root_, ec);
    if (ec) {
        return false;
    }
    if (relative.empty() || relative == ".") {
        return true;
    }
    return relative.begin()->string() != "..";
}

std::filesystem::path FileDialog::buildResultPath() const {
    fs::path file(trimmed(file_name_));
    if (!sameExtension(file, filter_ext_)) {
        file += filter_ext_;
    }
    return current_dir_ / file;
}

void FileDialog::render() {
    if (!open_) {
        return;
    }
    // 打开期间每帧确保模态弹窗处于打开状态。
    if (!ImGui::IsPopupOpen(title_.c_str())) {
        ImGui::OpenPopup(title_.c_str());
    }

    ImGui::SetNextWindowSize(ImVec2(560.0f, 440.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool confirm = false;
    bool cancel = false;

    if (ImGui::BeginPopupModal(title_.c_str(), &open_)) {
        renderNavigation();
        ImGui::Separator();

        float footer_height = ImGui::GetFrameHeightWithSpacing() * 3.0f;
        ImGui::BeginChild("##file_list", ImVec2(0.0f, -footer_height), ImGuiChildFlags_Borders);
        renderFileList();
        ImGui::EndChild();

        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(mode_ == Mode::eOpen ? "File" : "File name");
        ImGui::SameLine();
        if (focus_file_name_) {
            ImGui::SetKeyboardFocusHere();
            focus_file_name_ = false;
        }
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##file_name", file_name_, sizeof(file_name_),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            confirm_requested_ = true;
        }

        if (mode_ == Mode::eSave) {
            ImGui::TextDisabled("Save to: %s", buildResultPath().generic_string().c_str());
        }
        ImGui::Spacing();

        bool can_confirm = !trimmed(file_name_).empty();
        const char* confirm_label = (mode_ == Mode::eOpen) ? "Open" : "Save";
        const float button_width = 96.0f;
        const float button_gap = ImGui::GetStyle().ItemSpacing.x;
        float buttons_x = ImGui::GetWindowWidth() - 2.0f * button_width - button_gap;
        ImGui::SetCursorPosX(std::max(0.0f, buttons_x));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 4.0f));
        ImGui::BeginDisabled(!can_confirm);
        if (ImGui::Button(confirm_label, ImVec2(button_width, 0.0f))) {
            confirm = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f))) {
            cancel = true;
        }
        ImGui::PopStyleVar();

        if (can_confirm && confirm_requested_) {
            confirm = true;
        }
        confirm_requested_ = false;

        // 关闭弹窗:确认 / 取消 / 点击标题栏 X(*p_open 被置 false)。
        if (confirm || cancel || !open_) {
            open_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 弹窗关闭后留下一次结果,供调用方 takeResult 消费。
    if (confirm) {
        result_pending_ = true;
        result_confirmed_ = true;
        result_path_ = buildResultPath();
    } else if (cancel || !open_) {
        result_pending_ = true;
        result_confirmed_ = false;
    }
}

void FileDialog::renderNavigation() {
    ImGui::AlignTextToFramePadding();

    bool can_up = current_dir_ != asset_root_;
    ImGui::BeginDisabled(!can_up);
    if (ImGui::Button(icons::kArrowUp, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
        current_dir_ = current_dir_.parent_path();
        selected_path_.clear();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();

    widgets::breadcrumb(asset_root_, current_dir_, [this](const fs::path& target) {
        current_dir_ = target;
        selected_path_.clear();
    });
}

void FileDialog::renderFileList() {
    std::error_code ec;

    struct Entry {
        fs::path path;
        bool is_dir;
    };
    std::vector<Entry> entries;
    for (const auto& entry : fs::directory_iterator(current_dir_, ec)) {
        if (entry.is_directory(ec)) {
            entries.push_back({entry.path(), true});
        } else if (sameExtension(entry.path(), filter_ext_)) {
            entries.push_back({entry.path(), false});
        }
    }
    widgets::sortFolderFirst(entries, [](const Entry& e) { return e.is_dir; });

    if (entries.empty()) {
        ImGui::TextDisabled("(empty)");
        return;
    }

    for (const auto& entry : entries) {
        const std::string name = entry.path.filename().string();
        const bool selected = (entry.path == selected_path_);

        ImGui::PushID(entry.path.string().c_str());
        ImGui::PushStyleColor(ImGuiCol_Text,
                              entry.is_dir ? ImVec4(0.78f, 0.65f, 0.35f, 1.0f)
                                           : ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        if (ImGui::Selectable((std::string(entry.is_dir ? icons::kFolder : icons::kFile) +
                               " " + name)
                                  .c_str(),
                              selected)) {
            selected_path_ = entry.path;
            if (!entry.is_dir) {
                std::snprintf(file_name_, sizeof(file_name_), "%s", name.c_str());
                file_name_[sizeof(file_name_) - 1] = '\0';
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (entry.is_dir) {
                current_dir_ = entry.path;
                selected_path_.clear();
            } else {

                std::snprintf(file_name_, sizeof(file_name_), "%s", name.c_str());
                file_name_[sizeof(file_name_) - 1] = '\0';
                if (mode_ == Mode::eOpen) {
                    confirm_requested_ = true;
                }
            }
        }
        ImGui::PopID();
    }
}

bool FileDialog::takeResult(bool& confirmed, std::filesystem::path& path) {
    if (!result_pending_) {
        return false;
    }
    result_pending_ = false;
    confirmed = result_confirmed_;
    path = result_path_;
    return true;
}

}  // namespace wen::editor
