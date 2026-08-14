#include "ui/prefab_actions.hpp"
#include "ui/editor_scene.hpp"
#include "ui/undo.hpp"
#include "engine/global_context.hpp"
#include "function/framework/game_object.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/scene_serializer.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/prefab/prefab_component.hpp"
#include "core/base/macro.hpp"
#include <fstream>
#include <iterator>

namespace wen::editor {

namespace fs = std::filesystem;

namespace {

fs::path prefabsDir() {
    return fs::path(global_context->asset_system->getRootDir()) / "prefabs";
}

bool writeTextFile(const fs::path& path, const std::string& text) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out) {
        WEN_CORE_ERROR("PrefabActions: cannot write {}.", path.string())
        return false;
    }
    out << text;
    return true;
}

bool readTextFile(const fs::path& path, std::string& out) {
    std::ifstream in(path);
    if (!in) {
        WEN_CORE_ERROR("PrefabActions: cannot open {}.", path.string())
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

}  // namespace

bool createPrefab(GameObject* game_object, const fs::path& prefab_path) {
    if (game_object == nullptr || prefab_path.empty()) {
        return false;
    }
    auto file = prefabsDir() / prefab_path;
    if (file.extension().empty()) {
        file += ".prefab";
    }
    // 已存在:自动加 _2/_3 后缀(基于原始文件名)。
    auto stem = file.stem().string();
    auto ext = file.extension().string();
    int n = 2;
    while (fs::exists(file)) {
        file = file.parent_path() / (stem + "_" + std::to_string(n) + ext);
        n++;
    }
    auto rel = fs::relative(file, prefabsDir()).generic_string();

    auto json = SceneSerializer::serializeGameObject(game_object, PrefabComponent::GetClassName());
    if (!writeTextFile(file, json)) {
        return false;
    }
    if (!isPrefabInstance(game_object)) {
        auto* prefab_component = new PrefabComponent;
        prefab_component->prefab_path = rel;
        game_object->addComponent(prefab_component);
        pushComponentAdded(game_object, prefab_component);
    }
    WEN_CLIENT_INFO("Prefab: created {}", file.string())
    return true;
}

GameObject* instantiatePrefab(const std::string& prefab_path) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || prefab_path.empty()) {
        return nullptr;
    }
    std::string json;
    if (!readTextFile(prefabsDir() / prefab_path, json)) {
        return nullptr;
    }
    // 新 uuid 建对象;uuid 冲突/解析失败时重试几次。
    GameObject* game_object = nullptr;
    for (int attempt = 0; attempt < 4 && game_object == nullptr; attempt++) {
        auto uuid = global_context->game_object_uuid_allocator->allocate();
        game_object = SceneSerializer::deserializeGameObject(scene, json, uuid);
    }
    if (game_object == nullptr) {
        WEN_CORE_ERROR("Prefab: failed to instantiate {}.", prefab_path)
        return nullptr;
    }
    // 落点在编辑器相机前方。
    if (auto* transform = game_object->queryComponent<TransformComponent>()) {
        transform->location = editorSpawnLocation();
        transform->triggerMemberUpdateCallbacks();
    }
    auto* prefab_component = new PrefabComponent;
    prefab_component->prefab_path = prefab_path;
    game_object->addComponent(prefab_component);
    return game_object;
}

bool revertPrefabInstance(GameObject* game_object) {
    auto path = prefabPathFor(game_object);
    if (path.empty()) {
        return false;
    }
    std::string json;
    if (!readTextFile(prefabsDir() / path, json)) {
        return false;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr) {
        return false;
    }
    auto uuid = game_object->getUUID();

    std::string before = SceneSerializer::serializeGameObject(game_object);  // 撤销快照(改前)

    scene->removeGameObject(game_object);
    auto* rebuilt = SceneSerializer::deserializeGameObject(scene, json, uuid);
    if (rebuilt == nullptr) {
        WEN_CORE_ERROR("Prefab: revert failed (uuid {} lost).", uuid)
        return false;
    }
    auto* prefab_component = new PrefabComponent;
    prefab_component->prefab_path = path;
    rebuilt->addComponent(prefab_component);

    pushPrefabRevert(uuid, std::move(before), SceneSerializer::serializeGameObject(rebuilt));
    return true;
}

bool applyPrefabInstance(GameObject* game_object) {
    auto path = prefabPathFor(game_object);
    if (path.empty()) {
        return false;
    }
    auto json = SceneSerializer::serializeGameObject(game_object, PrefabComponent::GetClassName());
    if (!writeTextFile(prefabsDir() / path, json)) {
        return false;
    }
    WEN_CLIENT_INFO("Prefab: applied to {}", path)
    return true;
}

bool isPrefabInstance(GameObject* game_object) {
    return game_object != nullptr && game_object->queryComponent<PrefabComponent>() != nullptr;
}

std::string prefabPathFor(GameObject* game_object) {
    auto* prefab_component = game_object ? game_object->queryComponent<PrefabComponent>() : nullptr;
    return prefab_component ? prefab_component->prefab_path : std::string();
}

}  // namespace wen::editor
