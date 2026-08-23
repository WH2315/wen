#include "ui/editor_scene.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/scene_serializer.hpp"
#include "core/base/macro.hpp"
#include <functional>

namespace wen::editor {

namespace fs = std::filesystem;

namespace {

// 按反射元数据深拷贝组件:工厂构造默认实例 + 拷贝可序列化成员。
// 运行期状态(如 MeshComponent::mesh_id)不由反射保存,由 onCreate 重建。
Component* cloneComponent(Component* source) {
    const auto& class_name = source->getClassName();
    auto* clone = global_context->component_factory->create(class_name);
    if (clone == nullptr) {
        WEN_CLIENT_WARN("Duplicate: component {} is not constructible, skipped.", class_name)
        return nullptr;
    }

    const auto& descriptor = global_context->reflect_system->getClass(class_name);
    for (const auto& member_name : descriptor.getMemberNames()) {
        const auto& member = descriptor.getMember(member_name);
        switch (member.getType()) {
            case MemberType::eInt:
                member.setValueByPtr(clone, member.getValueConstByPtr<int>(source));
                break;
            case MemberType::eFloat:
                member.setValueByPtr(clone, member.getValueConstByPtr<float>(source));
                break;
            case MemberType::eBool:
                member.setValueByPtr(clone, member.getValueConstByPtr<bool>(source));
                break;
            case MemberType::eVec2:
                member.setValueByPtr(clone, member.getValueConstByPtr<glm::vec2>(source));
                break;
            case MemberType::eVec3:
                member.setValueByPtr(clone, member.getValueConstByPtr<glm::vec3>(source));
                break;
            case MemberType::eVec4:
                member.setValueByPtr(clone, member.getValueConstByPtr<glm::vec4>(source));
                break;
            case MemberType::eString:
                member.setValueByPtr(clone, member.getValueConstByPtr<std::string>(source));
                break;
            case MemberType::eCustom:
                break;
        }
    }
    return clone;
}

}  // namespace

// 编辑器相机前方 8 个单位处,作为新对象的生成位置。
glm::vec3 editorSpawnLocation() {
    auto* camera_data = global_context->camera_system->queryCameraData(global_ui_context->viewport_camera_id);
    auto world = glm::inverse(camera_data->view);
    glm::vec3 position(world[3]);
    glm::vec3 forward(-world[2]);
    return position + glm::normalize(forward) * 8.0f;
}

GameObject* createEmptyGameObject() {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr) {
        return nullptr;
    }
    auto* game_object = scene->createGameObject("GameObject");
    auto* transform = new TransformComponent;
    transform->location = editorSpawnLocation();
    game_object->addComponent(transform);
    return game_object;
}

GameObject* createChildGameObject(GameObject* parent) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || parent == nullptr) {
        return nullptr;
    }
    auto* game_object = scene->createGameObject("GameObject");
    auto* transform = new TransformComponent;  // 本地坐标原点,跟随父变换
    game_object->addComponent(transform);
    game_object->setParent(parent);
    if (auto* t = game_object->queryComponent<TransformComponent>()) {
        t->propagateWorldChange();
    }
    return game_object;
}

GameObject* duplicateGameObject(GameObject* source) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || source == nullptr) {
        return nullptr;
    }
    // 整棵子树深拷贝:逐组件克隆,并递归克隆子对象后挂到克隆父下。
    std::function<GameObject*(GameObject*, bool is_root)> clone = [&](GameObject* src, bool is_root) {
        auto* clone_go = scene->createGameObject(src->getName() + (is_root ? " (copy)" : ""));
        for (auto* component : src->getComponents()) {
            if (auto* cloned = cloneComponent(component)) {
                clone_go->addComponent(cloned);
            }
        }
        for (auto* child : src->getChildren()) {
            if (auto* child_clone = clone(child, false)) {
                child_clone->setParent(clone_go);
            }
        }
        return clone_go;
    };
    return clone(source, true);
}

void removeGameObject(GameObject* game_object) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || game_object == nullptr) {
        return;
    }
    // 级联删除整棵子树:先收集(自底向上),再逐个从场景移除。
    std::vector<GameObject*> to_remove;
    std::function<void(GameObject*)> collect = [&](GameObject* go) {
        for (auto* child : go->getChildren()) {
            collect(child);
        }
        to_remove.push_back(go);
    };
    collect(game_object);
    for (auto* go : to_remove) {
        scene->removeGameObject(go);
    }
}

GameObject* spawnMeshGameObject(const fs::path& mesh_file) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr) {
        return nullptr;
    }

    // 网格按 <资源根>/models 下的相对路径加载(与序列化存储的 mesh_path 一致)。
    fs::path models_dir = fs::path(global_context->asset_system->getRootDir()) / "models";
    std::error_code ec;
    auto relative = fs::relative(mesh_file, models_dir, ec);
    if (ec) {
        return nullptr;
    }
    auto relative_str = relative.generic_string();

    auto mesh_id = global_context->asset_system->loadMesh(relative_str);
    if (mesh_id == MeshID(-1)) {
        return nullptr;
    }

    auto* game_object = scene->createGameObject(mesh_file.stem().string());
    auto* transform = new TransformComponent;
    transform->location = editorSpawnLocation();
    game_object->addComponent(transform);
    game_object->addComponent(new MeshComponent(mesh_id));
    return game_object;
}

bool saveSceneTo(const fs::path& path) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || path.empty()) {
        return false;
    }
    // 场景名同步为文件名,保证文件内 "scene" 字段与文件名一致。
    std::string new_name = path.stem().string();
    if (!new_name.empty() && new_name != scene->getName()) {
        global_context->scene_manager->renameScene(scene->getName(), new_name);
    }
    return SceneSerializer::save(scene, path);
}

}  // namespace wen::editor
