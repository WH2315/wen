#pragma once

#include "function/framework/scene_manager.hpp"
#include <filesystem>

namespace wen {

// 场景 <-> JSON 文件(.scene)
class SceneSerializer final {
public:
    static bool save(Scene* scene, const std::filesystem::path& file);

    static Scene* load(const std::filesystem::path& file);

    static std::string saveToString(Scene* scene);
    static Scene* loadFromString(const std::string& text);

    // 单对象序列化;exclude_class 非空时跳过该类组件(如 prefab 模板排除 PrefabComponent)。
    static std::string serializeGameObject(GameObject* game_object,
                                           const std::string& exclude_class = "");
    static GameObject* deserializeGameObject(Scene* scene, const std::string& text, GameObjectUUID uuid);

    // 序列化对象及其整棵后代子树(供撤销跨子树删除/复制的恢复)。
    static std::string serializeGameObjectTree(GameObject* game_object);
    // 从子树快照恢复:按持久化 uuid 重建对象与父子关系。
    static void deserializeGameObjectTree(Scene* scene, const std::string& text);
};

}  // namespace wen
