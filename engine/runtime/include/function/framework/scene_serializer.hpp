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

    static std::string serializeGameObject(GameObject* game_object);
    static GameObject* deserializeGameObject(Scene* scene, const std::string& text, GameObjectUUID uuid);
};

}  // namespace wen
