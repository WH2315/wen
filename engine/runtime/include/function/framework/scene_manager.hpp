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
    GameObject* createGameObject(const std::string& name, GameObjectUUID uuid);
    void removeGameObject(GameObject* game_object);

    const std::string& getName() const { return name_; }

    void setName(const std::string& name) { name_ = name; }

    const std::list<GameObject*>& getGameObjects() const { return game_objects_; }
    GameObject* getGameObject(GameObjectUUID uuid) const {
        auto iter = game_object_map_.find(uuid);
        return iter == game_object_map_.end() ? nullptr : iter->second;
    }

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
    // 重命名场景并更新按名字索引的表(编辑器"另存为"后同步场景名)。
    bool renameScene(const std::string& old_name, const std::string& new_name);
    void loadScene(const std::string& name);

    // 销毁一个场景(游戏对象析构会触发组件 onDestroy 反注册渲染资源)。
    // 若它是活动/待切换场景,对应指针会被清空,调用方负责随后设置新场景。
    void destroyScene(const std::string& name);
    void setActiveScene(Scene* scene) { active_scene_ = scene; }

    Scene* getActiveScene() const { return active_scene_; }

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