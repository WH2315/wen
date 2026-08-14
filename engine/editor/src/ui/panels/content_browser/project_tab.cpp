#include "ui/panels/content_browser/project_tab.hpp"
#include "ui/editor_scene.hpp"
#include "ui/prefab_actions.hpp"
#include "ui/ui_context.hpp"
#include "ui/undo.hpp"
#include "ui/icons.hpp"
#include "ui/widgets.hpp"
#include "engine/global_context.hpp"
#include <imgui_internal.h>

namespace wen::editor {

namespace fs = std::filesystem;

namespace {

float s_tile_size = 90.0f;

std::string lowerExtension(const fs::path& path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

bool isMeshFile(const fs::path& path) {
    return lowerExtension(path) == ".obj";
}

bool isSceneFile(const fs::path& path) {
    return lowerExtension(path) == ".scene";
}

bool isPrefabFile(const fs::path& path) {
    return lowerExtension(path) == ".prefab";
}

// 请求打开场景:实际加载由 Editor 在 ImGui 帧外执行,这里只发起请求。
void requestOpenScene(const fs::path& file) {
    if (global_ui_context->mode != Mode::eEdit) {
        return;
    }
    global_ui_context->scene_file_actions.requestOpen(file);
}

bool matchesSearch(const std::string& name, const char* search) {
    if (search[0] == '\0') {
        return true;
    }
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    };
    return lower(name).find(lower(search)) != std::string::npos;
}

ImVec4 entryColor(bool is_directory, bool is_mesh) {
    return is_directory ? ImVec4(0.78f, 0.65f, 0.35f, 1.0f)
           : is_mesh    ? ImVec4(0.45f, 0.62f, 0.90f, 1.0f)
                        : ImVec4(0.65f, 0.65f, 0.68f, 1.0f);
}

const char* entryIcon(const fs::path& path, bool is_directory) {
    if (is_directory) {
        return icons::kFolder;
    }
    auto ext = lowerExtension(path);
    if (ext == ".obj") {
        return icons::kCube;
    }
    if (ext == ".ttf" || ext == ".otf") {
        return icons::kFont;
    }
    return icons::kFile;
}

void inlineIcon(const char* icon, const ImVec4& color) {
    ImGui::TextColored(color, "%s", icon);
    ImGui::SameLine();
}

void zoomSlider(const char* id, float* value, float min, float max, float width) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float height = ImGui::GetFrameHeight();
    ImGui::InvisibleButton(id, ImVec2(width, height));
    bool active = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (active) {
        float t = (ImGui::GetIO().MousePos.x - pos.x - 8.0f) / (width - 16.0f);
        *value = min + std::clamp(t, 0.0f, 1.0f) * (max - min);
    }

    auto* draw_list = ImGui::GetWindowDrawList();
    float center_y = pos.y + height * 0.5f;
    draw_list->AddLine(ImVec2(pos.x + 8.0f, center_y), ImVec2(pos.x + width - 8.0f, center_y),
                       ImGui::GetColorU32(ImGuiCol_TextDisabled), 2.0f);
    float t = (*value - min) / (max - min);
    float knob_x = pos.x + 8.0f + t * (width - 16.0f);
    ImU32 knob_color = ImGui::GetColorU32(
        active ? ImGuiCol_SliderGrabActive
               : (ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_SliderGrab));
    draw_list->AddCircleFilled(ImVec2(knob_x, center_y), 7.0f, knob_color);
}

std::string fitLabel(const std::string& text, float max_width) {
    if (ImGui::CalcTextSize(text.c_str()).x <= max_width) {
        return text;
    }
    std::string out = text;
    while (out.size() > 1 && ImGui::CalcTextSize((out + "..").c_str()).x > max_width) {
        out.pop_back();
    }
    return out + "..";
}

}  // namespace

ProjectTab::ProjectTab() {
    root_ = fs::path("engine/assets");
    current_dir_ = root_;
}

void ProjectTab::revealAsset(const fs::path& file) {
    if (file.empty()) {
        return;
    }
    // 直接改当前目录并高亮条目(无需 pending_navigation_,避免被选中清除逻辑重置)。
    current_dir_ = file.parent_path();
    selected_entry_ = file;
    search_[0] = '\0';  // 退出搜索模式,显示文件所在目录
}

std::string ProjectTab::displayPath(const fs::path& dir) const {
    std::error_code ec;
    auto relative = fs::relative(dir, root_, ec);
    if (ec || relative == ".") {
        return "Assets";
    }
    return "Assets/" + relative.generic_string();
}

void ProjectTab::registerIniSettings() {
    ImGuiSettingsHandler handler;
    handler.TypeName = "ContentBrowser";
    handler.TypeHash = ImHashStr("ContentBrowser");
    handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* {
        return (void*)1;
    };
    handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
        if (std::strncmp(line, "Zoom=", 5) == 0) {
            s_tile_size = std::clamp(static_cast<float>(std::atof(line + 5)),
                                     kMinTileSize, kMaxTileSize);
        }
    };
    handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        buf->appendf("[%s][State]\n", handler->TypeName);
        buf->appendf("Zoom=%.0f\n\n", s_tile_size);
    };
    ImGui::AddSettingsHandler(&handler);
}

void ProjectTab::render() {
    std::error_code ec;
    if (!fs::exists(root_, ec)) {
        ImGui::Text("Asset root not found: %s", root_.string().c_str());
        return;
    }

    renderToolbar();

    float footer_height = ImGui::GetFrameHeightWithSpacing();

    // 左侧:带 "Assets" 根节点的文件夹树
    ImGui::SetNextWindowSizeConstraints(ImVec2(140.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::BeginChild("##directory_tree", ImVec2(230.0f, -footer_height),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX,
                      ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                        ImGuiTreeNodeFlags_SpanAvailWidth |
                                        ImGuiTreeNodeFlags_DefaultOpen;
        if (current_dir_ == root_) {
            root_flags |= ImGuiTreeNodeFlags_Selected;
        }
        bool open = ImGui::TreeNodeEx("##assets_root", root_flags);
        bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen();
        ImGui::SameLine();
        inlineIcon(open ? icons::kFolderOpen : icons::kFolder, entryColor(true, false));
        ImGui::TextUnformatted("Assets");
        if (clicked) {
            pending_navigation_ = root_;
        }
        if (open) {
            renderDirectoryTree(root_);
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 右侧:当前文件夹标题条 + 内容区
    ImGui::BeginChild("##content_area", ImVec2(0.0f, -footer_height), ImGuiChildFlags_Borders);
    renderContent();
    ImGui::EndChild();

    renderStatusBar();

    if (!pending_navigation_.empty()) {
        current_dir_ = pending_navigation_;
        pending_navigation_.clear();
        selected_entry_.clear();
    }
}

void ProjectTab::renderToolbar() {
    float search_width = 260.0f;
    float pen_x = ImGui::GetCursorPosX();
    float right_x = pen_x + ImGui::GetContentRegionAvail().x - search_width;
    ImGui::SetCursorPosX(std::max(pen_x, right_x));
    ImGui::SetNextItemWidth(search_width);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
    ImGui::InputTextWithHint("##search", icons::kSearch, search_, sizeof(search_));
    ImGui::PopStyleVar();
}

void ProjectTab::renderDirectoryTree(const fs::path& dir) {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }
        const auto& path = entry.path();

        bool has_subdirectory = false;
        for (const auto& sub : fs::directory_iterator(path, ec)) {
            if (sub.is_directory(ec)) {
                has_subdirectory = true;
                break;
            }
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!has_subdirectory) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (path == current_dir_) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushID(path.string().c_str());
        bool open = ImGui::TreeNodeEx("##dir", flags);
        bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen();
        ImGui::SameLine();
        inlineIcon(open && has_subdirectory ? icons::kFolderOpen : icons::kFolder, entryColor(true, false));
        ImGui::TextUnformatted(path.filename().string().c_str());
        if (clicked) {
            pending_navigation_ = path;
        }
        if (open) {
            renderDirectoryTree(path);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void ProjectTab::renderBreadcrumbPath() {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(entryColor(true, false), "%s", icons::kFolder);
    ImGui::SameLine();

    widgets::breadcrumb(root_, current_dir_, [this](const fs::path& target) {
        pending_navigation_ = target;
    });
}

void ProjectTab::renderContent() {
    bool searching = search_[0] != '\0';

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
    ImGui::BeginChild("##content_header", ImVec2(0.0f, ImGui::GetFrameHeight()), 0,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
    if (searching) {
        ImGui::Text("Search: \"%s\"", search_);
    } else {
        renderBreadcrumbPath();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::BeginChild("##content_items");

    std::error_code ec;
    std::vector<Entry> entries;
    if (searching) {
        constexpr size_t kMaxResults = 500;
        for (auto it = fs::recursive_directory_iterator(root_, fs::directory_options::skip_permission_denied, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            auto name = it->path().filename().string();
            if (matchesSearch(name, search_)) {
                entries.push_back({it->path(), it->is_directory(ec)});
                if (entries.size() >= kMaxResults) {
                    break;
                }
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(current_dir_, ec)) {
            entries.push_back({entry.path(), entry.is_directory(ec)});
        }
    }
    widgets::sortFolderFirst(entries, [](const Entry& e) { return e.is_directory; });

    if (entries.empty()) {
        ImGui::TextDisabled(searching ? "No matches." : "This folder is empty");
    } else if (s_tile_size <= kMinTileSize + 0.5f) {
        renderList(entries);
    } else {
        renderTiles(entries);
    }

    ImGui::EndChild();
}

void ProjectTab::renderStatusBar() {
    auto path_text = displayPath(selected_entry_.empty() ? current_dir_ : selected_entry_);
    inlineIcon(icons::kFolder, entryColor(true, false));
    ImGui::TextUnformatted(path_text.c_str());

    float slider_width = 140.0f;
    ImGui::SameLine();
    float pen_x = ImGui::GetCursorPosX();
    float right_x = pen_x + ImGui::GetContentRegionAvail().x - slider_width;
    ImGui::SameLine(std::max(pen_x, right_x));
    zoomSlider("##tile_size", &s_tile_size, kMinTileSize, kMaxTileSize, slider_width);
}

void ProjectTab::handleItemInteractions(const Entry& entry, bool is_mesh, bool is_prefab,
                                        const std::string& name) {
    if ((is_mesh || is_prefab) && ImGui::BeginDragDropSource()) {
        auto path_str = entry.path.string();
        ImGui::SetDragDropPayload(is_prefab ? kPrefabDragDropPayload : kMeshDragDropPayload,
                                  path_str.c_str(), path_str.size() + 1);
        ImGui::TextUnformatted(name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selected_entry_ = entry.path;
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (entry.is_directory) {
                pending_navigation_ = entry.path;
            } else if (is_prefab) {
                addPrefabToScene(entry.path);
            } else if (is_mesh) {
                addMeshToScene(entry.path);
            } else if (isSceneFile(entry.path)) {
                requestOpenScene(entry.path);
            }
        }
    }

    if (ImGui::BeginPopupContextItem("##context")) {
        selected_entry_ = entry.path;
        if (entry.is_directory) {
            if (ImGui::MenuItem("Open")) {
                pending_navigation_ = entry.path;
            }
        } else if (is_prefab) {
            if (ImGui::MenuItem("Add to Scene")) {
                addPrefabToScene(entry.path);
            }
        } else if (is_mesh) {
            if (ImGui::MenuItem("Add to Scene")) {
                addMeshToScene(entry.path);
            }
        } else if (isSceneFile(entry.path)) {
            if (ImGui::MenuItem("Open Scene")) {
                requestOpenScene(entry.path);
            }
        } else {
            ImGui::TextDisabled("No actions");
        }
        ImGui::EndPopup();
    }
}

void ProjectTab::renderTiles(const std::vector<Entry>& entries) {
    const auto& style = ImGui::GetStyle();
    float cell_width = s_tile_size + style.ItemSpacing.x;
    int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell_width));

    int index = 0;
    for (const auto& entry : entries) {
        auto name = entry.path.filename().string();
        bool is_mesh = !entry.is_directory && isMeshFile(entry.path);
        bool is_prefab = !entry.is_directory && isPrefabFile(entry.path);
        bool selected = entry.path == selected_entry_;

        ImGui::PushID(entry.path.string().c_str());
        ImGui::BeginGroup();
        float group_x = ImGui::GetCursorPosX();

        ImVec4 highlight = ImGui::GetStyleColorVec4(ImGuiCol_Header);
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? highlight : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        ImGui::PushStyleColor(ImGuiCol_Text, entryColor(entry.is_directory, is_mesh));
        ImGui::PushFont(nullptr, s_tile_size * 0.72f);
        ImGui::Button(entryIcon(entry.path, entry.is_directory), ImVec2(s_tile_size, s_tile_size * 0.75f));
        ImGui::PopFont();
        ImGui::PopStyleColor(4);

        bool tile_hovered = ImGui::IsItemHovered();
        handleItemInteractions(entry, is_mesh, is_prefab, name);
        if (tile_hovered) {
            ImGui::SetTooltip("%s", name.c_str());
        }

        auto label = fitLabel(name, s_tile_size);
        float label_width = ImGui::CalcTextSize(label.c_str()).x;
        ImGui::SetCursorPosX(group_x + std::max(0.0f, (s_tile_size - label_width) * 0.5f));
        ImGui::TextUnformatted(label.c_str());

        ImGui::EndGroup();
        ImGui::PopID();

        if ((++index) % columns != 0) {
            ImGui::SameLine();
        }
    }
}

void ProjectTab::renderList(const std::vector<Entry>& entries) {
    for (const auto& entry : entries) {
        auto name = entry.path.filename().string();
        bool is_mesh = !entry.is_directory && isMeshFile(entry.path);
        bool is_prefab = !entry.is_directory && isPrefabFile(entry.path);

        ImGui::PushID(entry.path.string().c_str());
        inlineIcon(entryIcon(entry.path, entry.is_directory), entryColor(entry.is_directory, is_mesh));
        ImGui::Selectable(name.c_str(), entry.path == selected_entry_);
        handleItemInteractions(entry, is_mesh, is_prefab, name);
        ImGui::PopID();
    }
}

void ProjectTab::addMeshToScene(const fs::path& file) {
    if (global_ui_context->mode != Mode::eEdit) {
        return;
    }
    if (auto* game_object = spawnMeshGameObject(file)) {
        if (on_select_game_object_) {
            on_select_game_object_(game_object->getUUID());
        }
        pushGameObjectCreated(game_object);
    }
}

void ProjectTab::addPrefabToScene(const fs::path& file) {
    if (global_ui_context->mode != Mode::eEdit) {
        return;
    }
    auto prefabs_dir = fs::path(global_context->asset_system->getRootDir()) / "prefabs";
    std::error_code ec;
    auto relative = fs::relative(file, prefabs_dir, ec);
    if (ec) {
        WEN_CLIENT_WARN("Prefab: {} is outside <root>/prefabs, ignored.", file.string())
        return;
    }
    if (auto* game_object = instantiatePrefab(relative.generic_string())) {
        if (on_select_game_object_) {
            on_select_game_object_(game_object->getUUID());
        }
        pushGameObjectCreated(game_object);
    }
}

}  // namespace wen::editor
