#pragma once

#include "function/framework/uuid_manager.hpp"

namespace wen::editor {

// 资源浏览器(Project 标签):目录树/面包屑/磁贴列表/搜索,网格拖放与双击打开场景都从这里发起。
class ProjectTab {
public:
    ProjectTab();

    void render();

    static void registerIniSettings();

    void setOnSelectGameObject(const std::function<void(GameObjectUUID uuid)>& callback) {
        on_select_game_object_ = callback;
    }

private:
    struct Entry {
        std::filesystem::path path;
        bool is_directory;
    };

    void renderToolbar();
    void renderDirectoryTree(const std::filesystem::path& dir);
    void renderBreadcrumbPath();
    void renderContent();
    void renderTiles(const std::vector<Entry>& entries);
    void renderList(const std::vector<Entry>& entries);
    void renderStatusBar();
    void handleItemInteractions(const Entry& entry, bool is_mesh, const std::string& name);
    void addMeshToScene(const std::filesystem::path& file);
    std::string displayPath(const std::filesystem::path& dir) const;

    static constexpr float kMinTileSize = 48.0f;
    static constexpr float kMaxTileSize = 160.0f;

    std::filesystem::path root_;
    std::filesystem::path current_dir_;
    std::filesystem::path pending_navigation_;
    std::filesystem::path selected_entry_;
    char search_[128] = {};

    std::function<void(GameObjectUUID uuid)> on_select_game_object_;
};

}  // namespace wen::editor
