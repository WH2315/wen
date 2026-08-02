#pragma once

#include "function/framework/uuid_manager.hpp"
#include <glm/glm.hpp>

namespace wen {
class GameObject;
}

namespace wen::editor {

// 从 Content Browser 拖出网格资源时的载荷类型
inline constexpr const char* kMeshDragDropPayload = "WEN_MESH_PATH";

// 加载网格(按路径缓存)并在编辑器相机前方生成一个带 Transform + Mesh 组件的游戏对象
GameObject* spawnMeshGameObject(const std::filesystem::path& mesh_file);

// 编辑器相机前方数个单位处的一点(生成位置)
glm::vec3 editorSpawnLocation();

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
