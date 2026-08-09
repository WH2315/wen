#include "ui/editor_scene.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "core/base/macro.hpp"

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

GameObject* duplicateGameObject(GameObject* source) {
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr || source == nullptr) {
        return nullptr;
    }
    auto* clone = scene->createGameObject(source->getName() + " (copy)");
    for (auto* component : source->getComponents()) {
        if (auto* cloned = cloneComponent(component)) {
            clone->addComponent(cloned);
        }
    }
    return clone;
}

void removeGameObject(GameObject* game_object) {
    if (auto* scene = global_context->scene_manager->getActiveScene()) {
        scene->removeGameObject(game_object);
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

}  // namespace wen::editor
