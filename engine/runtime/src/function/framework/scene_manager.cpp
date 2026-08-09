#include "function/framework/scene_manager.hpp"
#include "engine/global_context.hpp"

namespace wen {

Scene::~Scene() {
    for (auto& game_object : game_objects_) {
        delete game_object;
    }
    game_objects_.clear();
    game_object_map_.clear();
}

void Scene::awake() {
    for (auto* game_object : game_objects_) {
        game_object->awake();
    }
}

void Scene::start() {
    for (auto* game_object : game_objects_) {
        game_object->start();
    }
}

void Scene::fixedTick() {
    for (auto* game_object : game_objects_) {
        game_object->fixedTick();
    }
}

void Scene::tick(float dt) {
    for (auto* game_object : game_objects_) {
        game_object->tick(dt);
    }
}

void Scene::postTick(float dt) {
    for (auto* game_object : game_objects_) {
        game_object->postTick(dt);
    }
}

GameObject* Scene::createGameObject(const std::string& name) {
    auto* game_object = new GameObject(name);
    game_object_map_.insert({game_object->getUUID(), game_object});
    game_objects_.push_back(game_object);
    return game_object;
}

GameObject* Scene::createGameObject(const std::string& name, GameObjectUUID uuid) {
    if (game_object_map_.find(uuid) != game_object_map_.end()) {
        WEN_CORE_ERROR("game object with uuid {} already exists in scene {}.", uuid, name_)
        return nullptr;
    }
    auto* game_object = new GameObject(name, uuid);
    game_object_map_.insert({uuid, game_object});
    game_objects_.push_back(game_object);
    return game_object;
}

void Scene::removeGameObject(GameObject* game_object) {
    auto uuid = game_object->getUUID();
    auto iter = game_object_map_.find(uuid);
    if (iter == game_object_map_.end()) {
        WEN_CORE_ERROR("game object with uuid {} does not exist in scene {}.", uuid, name_)
        return;
    }
    game_objects_.remove(iter->second);
    game_object_map_.erase(iter);
    // 析构会触发各组件的 onDestroy(网格实例反注册、相机移除等)。
    delete game_object;
}

SceneManager::SceneManager() {
    active_scene_ = nullptr;
    change_scene_ = nullptr;
}

SceneManager::~SceneManager() {
    for (auto& [name, scene] : scenes_) {
        delete scene;
    }
    scenes_.clear();
}

Scene* SceneManager::createScene(const std::string& name) {
    if (scenes_.find(name) != scenes_.end()) {
        WEN_CORE_ERROR("Scene {} already exists.", name)
        return nullptr;
    }
    auto* scene = new Scene(name);
    scenes_.insert({name, scene});
    if (active_scene_ == nullptr) {
        active_scene_ = scene;
    }
    return scene;
}

bool SceneManager::renameScene(const std::string& old_name, const std::string& new_name) {
    if (old_name == new_name) {
        return true;
    }
    auto iter = scenes_.find(old_name);
    if (iter == scenes_.end()) {
        WEN_CORE_ERROR("Scene {} does not exist.", old_name)
        return false;
    }
    if (scenes_.find(new_name) != scenes_.end()) {
        destroyScene(new_name);
    }
    auto* scene = iter->second;
    scene->setName(new_name);
    scenes_.erase(iter);
    scenes_.insert({new_name, scene});
    return true;
}

void SceneManager::loadScene(const std::string& name) {
    auto iter = scenes_.find(name);
    if (iter == scenes_.end()) {
        WEN_CORE_ERROR("Scene {} does not exist.", name)
        return;
    }
    change_scene_ = iter->second;
}

void SceneManager::destroyScene(const std::string& name) {
    auto iter = scenes_.find(name);
    if (iter == scenes_.end()) {
        return;
    }
    if (active_scene_ == iter->second) {
        active_scene_ = nullptr;
    }
    if (change_scene_ == iter->second) {
        change_scene_ = nullptr;
    }
    delete iter->second;
    scenes_.erase(iter);
}

void SceneManager::start() {
    active_scene_->awake();
    active_scene_->start();
}

void SceneManager::fixedTick() {
    active_scene_->fixedTick();
}

void SceneManager::tick(float dt) {
    active_scene_->tick(dt);
    active_scene_->postTick(dt);
}

void SceneManager::swap() {
    if (change_scene_ == nullptr) {
        return;
    }
    active_scene_ = change_scene_;
    change_scene_ = nullptr;
    global_context->render_system->getRenderData()->clear();
    start();
}

}  // namespace wen