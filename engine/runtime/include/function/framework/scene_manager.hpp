#pragma once

#include "function/framework/game_object.hpp"

namespace wen {

class Scene final {
public:
    Scene(const std::string& name) : name_(name) {}
    ~Scene();

    void awake();
    void start();
    void fixedTick();
    void tick(float dt);
    void postTick(float dt);

    GameObject* createGameObject(const std::string& name);
    void removeGameObject(GameObject* game_object);
    void removeGameObject(GameObjectUUID uuid);

private:
    std::string name_;
    std::map<uint64_t, GameObject*> game_object_map_;
    std::list<GameObject*> game_objects_;
};

class SceneManager final {
    friend class Singleton<SceneManager>;
    SceneManager();
    ~SceneManager();

public:
    Scene* createScene(const std::string& name);
    void loadScene(const std::string& name);

    void start();
    void fixedTick();
    void tick(float dt);
    void swap();

private:
    std::map<std::string, Scene*> scenes_;
    Scene* active_scene_;
    Scene* change_scene_;
};

}  // namespace wen