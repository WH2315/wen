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
};

}  // namespace wen
