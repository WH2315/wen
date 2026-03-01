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

void Scene::removeGameObject(GameObject* game_object) {
    removeGameObject(game_object->getUUID());
}

void Scene::removeGameObject(GameObjectUUID uuid) {
    auto iter = game_object_map_.find(uuid);
    if (iter == game_object_map_.end()) {
        WEN_CORE_ERROR("game object with uuid {} does not exist in scene {}.", uuid, name_)
        return;
    }
    game_objects_.remove(iter->second);
    game_object_map_.erase(iter);
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

void SceneManager::loadScene(const std::string& name) {
    auto iter = scenes_.find(name);
    if (iter == scenes_.end()) {
        WEN_CORE_ERROR("Scene {} does not exist.", name)
        return;
    }
    change_scene_ = iter->second;
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
    // global_context->render_system->getSwapData()->clear();
    start();
}

}  // namespace wen