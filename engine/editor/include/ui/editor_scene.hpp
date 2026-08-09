#pragma once

#include <filesystem>
#include <glm/glm.hpp>

namespace wen {
class GameObject;
}

namespace wen::editor {

// 编辑器对场景的变更操作门面。面板经它修改场景,不直接触碰引擎内部;
// 撤销与选中由调用方围绕这些操作组织。
inline constexpr const char* kMeshDragDropPayload = "WEN_MESH_PATH";

// 编辑器相机前方数个单位处的一点(生成位置)。
glm::vec3 editorSpawnLocation();

// 在活动场景创建空对象(带 Transform,位于编辑器相机前方)。
GameObject* createEmptyGameObject();

// 深拷贝对象(逐组件克隆,组件添加顺序保持不变)。
GameObject* duplicateGameObject(GameObject* source);

// 从活动场景删除对象。
void removeGameObject(GameObject* game_object);

// 加载网格并生成一个带 Transform + Mesh 组件的游戏对象。
GameObject* spawnMeshGameObject(const std::filesystem::path& mesh_file);

}  // namespace wen::editor
