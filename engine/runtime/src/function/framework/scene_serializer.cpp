#include "function/framework/scene_serializer.hpp"
#include "function/framework/game_object.hpp"
#include "function/framework/component_factory.hpp"
#include "function/framework/component/script/script_component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/script/script.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include <json.hpp>
#include <type_traits>
#include <fstream>
#include <functional>

namespace wen {

namespace {

using json = nlohmann::json;

// 按脚本字段当前类型(ScriptValue index)把 JSON 值转回 ScriptValue,int/float 互相宽容。
ScriptValue scriptValueFromJson(const json& value, size_t field_index) {
    switch (field_index) {
        case 0:  // int
            return value.is_number_integer() ? ScriptValue(value.get<int>())
                                             : ScriptValue(static_cast<int>(value.get<float>()));
        case 1:  // float
            return value.is_number_integer() ? ScriptValue(static_cast<float>(value.get<int>()))
                                             : ScriptValue(value.get<float>());
        case 2:  // bool
            return ScriptValue(value.get<bool>());
        case 3:  // vec2
            return ScriptValue(glm::vec2(value.at(0).get<float>(), value.at(1).get<float>()));
        case 4:  // vec3
            return ScriptValue(glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(),
                                         value.at(2).get<float>()));
        case 5:  // vec4
            return ScriptValue(glm::vec4(value.at(0).get<float>(), value.at(1).get<float>(),
                                         value.at(2).get<float>(), value.at(3).get<float>()));
        case 6:  // string
            return ScriptValue(value.get<std::string>());
        default:
            return ScriptValue(0.0f);
    }
}

json serializeMembers(Component* component, const ClassDescriptor& descriptor) {
    json members = json::object();
    for (const auto& name : descriptor.getMemberNames()) {
        const auto& member = descriptor.getMember(name);
        switch (member.getType()) {
            case MemberType::eInt:
                members[name] = member.getValueConstByPtr<int>(component);
                break;
            case MemberType::eFloat:
                members[name] = member.getValueConstByPtr<float>(component);
                break;
            case MemberType::eBool:
                members[name] = member.getValueConstByPtr<bool>(component);
                break;
            case MemberType::eVec2: {
                const auto& v = member.getValueConstByPtr<glm::vec2>(component);
                members[name] = {v.x, v.y};
                break;
            }
            case MemberType::eVec3: {
                const auto& v = member.getValueConstByPtr<glm::vec3>(component);
                members[name] = {v.x, v.y, v.z};
                break;
            }
            case MemberType::eVec4: {
                const auto& v = member.getValueConstByPtr<glm::vec4>(component);
                members[name] = {v.x, v.y, v.z, v.w};
                break;
            }
            case MemberType::eString:
                members[name] = member.getValueConstByPtr<std::string>(component);
                break;
            case MemberType::eCustom:
                break;
        }
    }
    return members;
}

void deserializeMembers(Component* component, const ClassDescriptor& descriptor, const json& members) {
    for (const auto& name : descriptor.getMemberNames()) {
        if (!members.contains(name)) {
            continue;  // 文件里没有的成员保持默认值
        }
        const auto& member = descriptor.getMember(name);
        const auto& value = members.at(name);
        try {
            switch (member.getType()) {
                case MemberType::eInt:
                    member.setValueByPtr(component, value.get<int>());
                    break;
                case MemberType::eFloat:
                    member.setValueByPtr(component, value.get<float>());
                    break;
                case MemberType::eBool:
                    member.setValueByPtr(component, value.get<bool>());
                    break;
                case MemberType::eVec2:
                    member.setValueByPtr(component, glm::vec2(value.at(0).get<float>(), value.at(1).get<float>()));
                    break;
                case MemberType::eVec3:
                    member.setValueByPtr(component,
                                         glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(),
                                                   value.at(2).get<float>()));
                    break;
                case MemberType::eVec4:
                    member.setValueByPtr(component,
                                         glm::vec4(value.at(0).get<float>(), value.at(1).get<float>(),
                                                   value.at(2).get<float>(), value.at(3).get<float>()));
                    break;
                case MemberType::eString:
                    member.setValueByPtr(component, value.get<std::string>());
                    break;
                case MemberType::eCustom:
                    break;
            }
        } catch (const json::exception& e) {
            WEN_CORE_WARN("SceneSerializer: member \"{}\" has wrong type in file, skipped ({}).", name, e.what())
        }
    }
}

// 单个游戏对象 -> JSON。
json gameObjectToJson(GameObject* game_object, const std::string& exclude_class = "") {
    json object;
    object["name"] = game_object->getName();
    // 持久化对象身份,供父关系等按 uuid 交叉引用。
    object["uuid"] = game_object->getUUID();
    // 序列化父关系:有父时写入父的 uuid(无父不写,保持向后兼容)。
    if (auto* parent = game_object->getParent()) {
        object["parent"] = parent->getUUID();
    }
    json components = json::array();
    for (auto* component : game_object->getComponents()) {
        if (!exclude_class.empty() && component->getClassName() == exclude_class) {
            continue;  // 序列化时跳过指定类组件(如 prefab 模板里的 PrefabComponent)
        }
        json comp;
        comp["class"] = component->getClassName();
        comp["members"] = serializeMembers(
            component, global_context->reflect_system->getClass(component->getClassName()));
        // 脚本组件额外序列化脚本字段(非反射成员,特判处理)。
        if (auto* script_component = dynamic_cast<ScriptComponent*>(component)) {
            if (auto* script = script_component->script()) {
                json script_fields = json::object();
                for (const auto& field : script->fields()) {
                    std::visit([&](const auto& v) {
                        using T = std::decay_t<decltype(v)>;
                        if constexpr (std::is_same_v<T, glm::vec2>) {
                            script_fields[field.name] = {v.x, v.y};
                        } else if constexpr (std::is_same_v<T, glm::vec3>) {
                            script_fields[field.name] = {v.x, v.y, v.z};
                        } else if constexpr (std::is_same_v<T, glm::vec4>) {
                            script_fields[field.name] = {v.x, v.y, v.z, v.w};
                        } else {
                            script_fields[field.name] = v;
                        }
                    }, field.value);
                }
                comp["script_fields"] = std::move(script_fields);
            }
        }
        components.push_back(std::move(comp));
    }
    object["components"] = std::move(components);
    return object;
}

// 按 JSON 描述为已创建的游戏对象装配组件。
void populateGameObject(GameObject* game_object, const json& object_json) {
    for (const auto& comp_json : object_json.value("components", json::array())) {
        auto class_name = comp_json.value("class", "");
        auto* component = global_context->component_factory->create(class_name);
        if (component == nullptr) {
            WEN_CORE_WARN("SceneSerializer: component \"{}\" is not constructible, skipped.", class_name)
            continue;
        }
        const auto& descriptor = global_context->reflect_system->getClass(class_name);
        if (comp_json.contains("members")) {
            deserializeMembers(component, descriptor, comp_json["members"]);
        }
        game_object->addComponent(component);
        component->triggerMemberUpdateCallbacks();
        // 脚本组件:恢复脚本字段(类型以脚本声明为准,addComponent 已实例化脚本)。
        if (auto* script_component = dynamic_cast<ScriptComponent*>(component)) {
            if (auto* script = script_component->script()) {
                if (comp_json.contains("script_fields")) {
                    for (auto it = comp_json["script_fields"].begin();
                         it != comp_json["script_fields"].end(); ++it) {
                        size_t field_index = 0;
                        for (const auto& field : script->fields()) {
                            if (field.name == it.key()) {
                                field_index = field.value.index();
                                break;
                            }
                        }
                        script->setField(it.key(), scriptValueFromJson(it.value(), field_index));
                    }
                }
            }
        }
    }
}

// 场景 -> JSON(save/saveToString 共用)。
json sceneToJson(Scene* scene) {
    json root;
    root["scene"] = scene->getName();
    json objects = json::array();
    for (auto* game_object : scene->getGameObjects()) {
        objects.push_back(gameObjectToJson(game_object));
    }
    root["game_objects"] = std::move(objects);
    return root;
}

// JSON -> 新场景(load/loadFromString 共用):先销毁同名旧场景再重建。
Scene* sceneFromJson(const json& root, const std::string& fallback_name) {
    auto& scene_manager = *global_context->scene_manager;
    auto name = root.value("scene", fallback_name);
    scene_manager.destroyScene(name);
    auto* scene = scene_manager.createScene(name);
    if (scene == nullptr) {
        return nullptr;
    }

    // 第一趟:创建全部对象并装配组件,同时记录父子关系(父 uuid)。
    // 优先还原持久化的 uuid 以保证父关系/身份稳定,并让分配器跳过已占用 id。
    std::vector<std::pair<GameObject*, GameObjectUUID>> parent_pairs;
    for (const auto& object_json : root.value("game_objects", json::array())) {
        auto name = object_json.value("name", "GameObject");
        GameObject* game_object = nullptr;
        if (object_json.contains("uuid")) {
            auto uuid = object_json["uuid"].get<GameObjectUUID>();
            global_context->game_object_uuid_allocator->reserve(uuid);
            game_object = scene->createGameObject(name, uuid);
        } else {
            game_object = scene->createGameObject(name);
        }
        if (game_object == nullptr) {
            continue;
        }
        populateGameObject(game_object, object_json);
        if (object_json.contains("parent")) {
            parent_pairs.emplace_back(game_object, object_json["parent"].get<GameObjectUUID>());
        }
    }
    // 第二趟:按 uuid 建立父子关系(父必须先已创建)。
    for (const auto& [game_object, parent_uuid] : parent_pairs) {
        if (auto* parent = scene->getGameObject(parent_uuid)) {
            game_object->setParent(parent);
        } else {
            WEN_CORE_WARN("SceneSerializer: parent uuid {} for \"{}\" not found.", parent_uuid, game_object->getName())
        }
    }
    // 父关系就绪后,从每个根节点向下脏传播,把真实世界变换推送给渲染实例等。
    for (auto* game_object : scene->getGameObjects()) {
        if (game_object->getParent() == nullptr) {
            if (auto* transform = game_object->queryComponent<TransformComponent>()) {
                transform->propagateWorldChange();
            }
        }
    }
    return scene;
}

}  // namespace

bool SceneSerializer::save(Scene* scene, const std::filesystem::path& file) {
    if (scene == nullptr) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    std::ofstream out(file);
    if (!out) {
        WEN_CORE_ERROR("SceneSerializer: cannot write {}.", file.string())
        return false;
    }
    out << sceneToJson(scene).dump(2);
    WEN_CORE_INFO("SceneSerializer: scene \"{}\" saved to {}.", scene->getName(), file.string())
    return true;
}

Scene* SceneSerializer::load(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) {
        WEN_CORE_ERROR("SceneSerializer: cannot open {}.", file.string())
        return nullptr;
    }
    json root = json::parse(in, nullptr, false);
    if (root.is_discarded()) {
        WEN_CORE_ERROR("SceneSerializer: {} is not valid json.", file.string())
        return nullptr;
    }

    auto* scene = sceneFromJson(root, file.stem().string());
    if (scene != nullptr) {
        WEN_CORE_INFO("SceneSerializer: scene \"{}\" loaded from {}.", scene->getName(), file.string())
    }
    return scene;
}

std::string SceneSerializer::saveToString(Scene* scene) {
    if (scene == nullptr) {
        return {};
    }
    return sceneToJson(scene).dump();
}

Scene* SceneSerializer::loadFromString(const std::string& text) {
    json root = json::parse(text, nullptr, false);
    if (root.is_discarded()) {
        WEN_CORE_ERROR("SceneSerializer: snapshot is not valid json.")
        return nullptr;
    }
    auto* scene = sceneFromJson(root, "Untitled");
    if (scene != nullptr) {
        WEN_CORE_INFO("SceneSerializer: scene \"{}\" restored from snapshot.", scene->getName())
    }
    return scene;
}

std::string SceneSerializer::serializeGameObject(GameObject* game_object,
                                                 const std::string& exclude_class) {
    if (game_object == nullptr) {
        return {};
    }
    return gameObjectToJson(game_object, exclude_class).dump();
}

GameObject* SceneSerializer::deserializeGameObject(Scene* scene, const std::string& text, GameObjectUUID uuid) {
    if (scene == nullptr) {
        return nullptr;
    }
    json object_json = json::parse(text, nullptr, false);
    if (object_json.is_discarded()) {
        WEN_CORE_ERROR("SceneSerializer: game object snapshot is not valid json.")
        return nullptr;
    }
    auto* game_object = scene->createGameObject(object_json.value("name", "GameObject"), uuid);
    if (game_object == nullptr) {
        return nullptr;
    }
    populateGameObject(game_object, object_json);
    // 恢复父关系(undo/prefab 还原用;父须已存在于当前场景)。
    if (object_json.contains("parent")) {
        if (auto* parent = scene->getGameObject(object_json["parent"].get<GameObjectUUID>())) {
            game_object->setParent(parent);
        }
    }
    if (auto* transform = game_object->queryComponent<TransformComponent>()) {
        transform->propagateWorldChange();
    }
    return game_object;
}

std::string SceneSerializer::serializeGameObjectTree(GameObject* root) {
    if (root == nullptr) {
        return {};
    }
    json objects = json::array();
    std::function<void(GameObject*)> collect = [&](GameObject* game_object) {
        objects.push_back(gameObjectToJson(game_object));
        for (auto* child : game_object->getChildren()) {
            collect(child);
        }
    };
    collect(root);
    json r;
    r["objects"] = std::move(objects);
    return r.dump();
}

void SceneSerializer::deserializeGameObjectTree(Scene* scene, const std::string& text) {
    if (scene == nullptr) {
        return;
    }
    json root = json::parse(text, nullptr, false);
    if (root.is_discarded()) {
        WEN_CORE_ERROR("SceneSerializer: game object tree snapshot is not valid json.")
        return;
    }
    // 第一趟:创建全部对象(按持久化 uuid)并装配组件,记录父子关系。
    std::vector<std::pair<GameObject*, GameObjectUUID>> parent_pairs;
    for (const auto& object_json : root.value("objects", json::array())) {
        if (!object_json.contains("uuid")) {
            continue;
        }
        auto uuid = object_json["uuid"].get<GameObjectUUID>();
        global_context->game_object_uuid_allocator->reserve(uuid);
        auto* game_object = scene->createGameObject(object_json.value("name", "GameObject"), uuid);
        if (game_object == nullptr) {
            continue;  // 已存在(如快照里含当前场景已有对象)
        }
        populateGameObject(game_object, object_json);
        if (object_json.contains("parent")) {
            parent_pairs.emplace_back(game_object, object_json["parent"].get<GameObjectUUID>());
        }
    }
    // 第二趟:建立父子关系。
    for (const auto& [game_object, parent_uuid] : parent_pairs) {
        if (auto* parent = scene->getGameObject(parent_uuid)) {
            game_object->setParent(parent);
        }
    }
    // 关系就绪后从每个根节点向下脏传播,刷新世界变换。
    for (auto* game_object : scene->getGameObjects()) {
        if (game_object->getParent() == nullptr) {
            if (auto* transform = game_object->queryComponent<TransformComponent>()) {
                transform->propagateWorldChange();
            }
        }
    }
}

}  // namespace wen
