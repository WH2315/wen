#pragma once

#include <filesystem>
#include <string>

namespace wen {
class GameObject;
}

namespace wen::editor {

// Prefab 资产操作门面:对象 <-> <root>/prefabs/<name>.prefab(单个对象 JSON)。
// create: 对象存成 .prefab 并挂 PrefabComponent 使其成为实例;
// instantiate: 从 .prefab 建新对象(位于编辑器相机前方);
// revert: 实例整份重载模板(丢弃本地改动,保留 uuid);apply: 实例整份写回模板。

// 把对象存成 prefab。prefab_path 相对 <root>/prefabs/;文件已存在时自动加 _2/_3 后缀。
// 成功返回 true(对象会挂上 PrefabComponent)。
bool createPrefab(GameObject* game_object, const std::filesystem::path& prefab_path);

// 从 .prefab 实例化一个新对象(prefab_path 相对 <root>/prefabs/),返回新对象。
GameObject* instantiatePrefab(const std::string& prefab_path);

// 把 prefab 实例重载回模板状态(丢弃本地改动,对象身份/选中不变)。成功返回 true。
bool revertPrefabInstance(GameObject* game_object);

// 把 prefab 实例当前状态写回 .prefab 模板。成功返回 true。
bool applyPrefabInstance(GameObject* game_object);

// 判断对象是否为 prefab 实例(挂有 PrefabComponent)。
bool isPrefabInstance(GameObject* game_object);

// 取实例的 prefab_path(相对 <root>/prefabs/);非实例返回空串。
std::string prefabPathFor(GameObject* game_object);

}  // namespace wen::editor
