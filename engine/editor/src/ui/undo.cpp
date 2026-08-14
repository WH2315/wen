#include "ui/undo.hpp"
#include "ui/ui_context.hpp"
#include "ui/selection.hpp"
#include "ui/editor_scene.hpp"
#include "engine/global_context.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/scene_serializer.hpp"
#include "function/framework/component/script/script_component.hpp"
#include "core/base/macro.hpp"

namespace wen::editor {

UndoStack* global_undo_stack = nullptr;

namespace {

GameObject* findGameObject(GameObjectUUID uuid) {
    auto* scene = global_context->scene_manager->getActiveScene();
    return scene ? scene->getGameObject(uuid) : nullptr;
}

// 经反射把变更集写回组件;use_before 决定应用旧值(undo)还是新值(redo)。
// 对象可能在撤销链中被删又重建,故用 uuid 而非指针定位。
void applyChanges(GameObjectUUID uuid, const std::string& component_class,
                  const std::vector<MemberChange>& changes, bool use_before) {
    auto* game_object = findGameObject(uuid);
    if (game_object == nullptr) {
        WEN_CLIENT_WARN("Undo: game object {} no longer exists, skipped.", uuid)
        return;
    }
    auto* component = game_object->queryComponent(component_class);
    if (component == nullptr) {
        WEN_CLIENT_WARN("Undo: component {} no longer exists on {}, skipped.", component_class,
                        game_object->getName())
        return;
    }
    const auto& descriptor = global_context->reflect_system->getClass(component_class);
    for (const auto& change : changes) {
        const auto& member = descriptor.getMember(change.name);
        const auto& value = use_before ? change.before : change.after;
        std::visit([&](const auto& v) { member.setValueByPtr(component, v); }, value);
    }
    component->triggerMemberUpdateCallbacks();
}

class PropertyEditCommand final : public EditorCommand {
public:
    PropertyEditCommand(GameObjectUUID uuid, std::string component_class, std::vector<MemberChange> changes)
        : uuid_(uuid), component_class_(std::move(component_class)), changes_(std::move(changes)) {}

    void undo() override { applyChanges(uuid_, component_class_, changes_, true); }
    void redo() override { applyChanges(uuid_, component_class_, changes_, false); }

private:
    GameObjectUUID uuid_;
    std::string component_class_;
    std::vector<MemberChange> changes_;
};

// 创建与删除共用一条命令:redo 方向由 created 翻转。构造时序列化对象快照,
// 恢复时沿用原 uuid(重建后撤销栈里后续命令引用的对象身份不变)。
class GameObjectExistenceCommand final : public EditorCommand {
public:
    GameObjectExistenceCommand(GameObject* game_object, bool created)
        : uuid_(game_object->getUUID()),
          snapshot_(SceneSerializer::serializeGameObject(game_object)),
          created_(created) {}

    void undo() override { created_ ? remove() : restore(); }
    void redo() override { created_ ? restore() : remove(); }

private:
    void remove() {
        auto* game_object = findGameObject(uuid_);
        if (game_object == nullptr) {
            return;
        }
        if (global_ui_context->selected_game_object_uuid == uuid_) {
            // 删除的是选中对象:先清选中与描边。
            setSelectedGameObject(kInvalidGameObjectUUID);
        }
        removeGameObject(game_object);
        // 实例池交换删除会移动其他实例的池索引,重新解析描边。
        setSelectedGameObject(global_ui_context->selected_game_object_uuid);
    }

    void restore() {
        auto* scene = global_context->scene_manager->getActiveScene();
        if (scene == nullptr || findGameObject(uuid_) != nullptr) {
            return;
        }
        SceneSerializer::deserializeGameObject(scene, snapshot_, uuid_);
    }

    GameObjectUUID uuid_;
    std::string snapshot_;
    bool created_;
};

class RenameCommand final : public EditorCommand {
public:
    RenameCommand(GameObjectUUID uuid, std::string before, std::string after)
        : uuid_(uuid), before_(std::move(before)), after_(std::move(after)) {}

    void undo() override { rename(before_); }
    void redo() override { rename(after_); }

private:
    void rename(const std::string& name) {
        if (auto* game_object = findGameObject(uuid_)) {
            game_object->setName(name);
        }
    }

    GameObjectUUID uuid_;
    std::string before_;
    std::string after_;
};

// 组件反射成员的快照(名称 -> 值),供组件增删撤销时恢复。
using MemberSnapshot = std::vector<std::pair<std::string, MemberValue>>;

MemberSnapshot snapshotComponentMembers(Component* component) {
    const auto& descriptor = global_context->reflect_system->getClass(component->getClassName());
    MemberSnapshot out;
    for (const auto& name : descriptor.getMemberNames()) {
        const auto& member = descriptor.getMember(name);
        switch (member.getType()) {
            case MemberType::eInt:
                out.push_back({name, member.getValueConstByPtr<int>(component)});
                break;
            case MemberType::eFloat:
                out.push_back({name, member.getValueConstByPtr<float>(component)});
                break;
            case MemberType::eBool:
                out.push_back({name, member.getValueConstByPtr<bool>(component)});
                break;
            case MemberType::eVec2:
                out.push_back({name, member.getValueConstByPtr<glm::vec2>(component)});
                break;
            case MemberType::eVec3:
                out.push_back({name, member.getValueConstByPtr<glm::vec3>(component)});
                break;
            case MemberType::eVec4:
                out.push_back({name, member.getValueConstByPtr<glm::vec4>(component)});
                break;
            case MemberType::eString:
                out.push_back({name, member.getValueConstByPtr<std::string>(component)});
                break;
            case MemberType::eCustom:
                break;
        }
    }
    return out;
}

void applyComponentSnapshot(Component* component, const MemberSnapshot& snapshot) {
    const auto& descriptor = global_context->reflect_system->getClass(component->getClassName());
    for (const auto& [name, value] : snapshot) {
        const auto& member = descriptor.getMember(name);
        std::visit([&](const auto& v) { member.setValueByPtr(component, v); }, value);
    }
    component->triggerMemberUpdateCallbacks();
}

// 组件添加与删除共用一条命令:redo 方向由 created 翻转。移除时快照成员,
// 恢复时经工厂重建并重放快照(运行期状态由 onCreate 重建)。
class ComponentExistenceCommand final : public EditorCommand {
public:
    ComponentExistenceCommand(GameObjectUUID uuid, Component* component, bool created)
        : uuid_(uuid), component_class_(component->getClassName()), created_(created) {
        if (!created_) {
            snapshot_ = snapshotComponentMembers(component);
        }
    }

    void undo() override { created_ ? removeComponent() : restoreComponent(); }
    void redo() override { created_ ? restoreComponent() : removeComponent(); }

private:
    void removeComponent() {
        auto* game_object = findGameObject(uuid_);
        if (game_object == nullptr) {
            return;
        }
        if (auto* component = game_object->queryComponent(component_class_)) {
            game_object->removeComponent(component);
        }
    }

    void restoreComponent() {
        auto* game_object = findGameObject(uuid_);
        if (game_object == nullptr || game_object->queryComponent(component_class_) != nullptr) {
            return;
        }
        auto* component = global_context->component_factory->create(component_class_);
        if (component == nullptr) {
            return;
        }
        // 先挂到对象上(addComponent 设置 game_object_ 并跑 onCreate),再重放成员快照:
        // 否则成员回调(如 MaterialComponent 写网格实例)会在 game_object_ 为空时触发。
        game_object->addComponent(component);
        if (!created_) {
            applyComponentSnapshot(component, snapshot_);
        }
    }

    GameObjectUUID uuid_;
    std::string component_class_;
    bool created_;
    MemberSnapshot snapshot_;  // 仅移除时记录
};

// 脚本字段编辑:按 uuid 定位 GO 的 ScriptComponent,把字段写回脚本。字段值存
// ScriptValue(运行时类型),不经过反射成员。
class ScriptFieldEditCommand final : public EditorCommand {
public:
    ScriptFieldEditCommand(GameObjectUUID uuid, std::string field_name,
                           ScriptValue before, ScriptValue after)
        : uuid_(uuid), field_name_(std::move(field_name)),
          before_(std::move(before)), after_(std::move(after)) {}

    void undo() override { apply(before_); }
    void redo() override { apply(after_); }

private:
    void apply(const ScriptValue& value) {
        auto* game_object = findGameObject(uuid_);
        if (game_object == nullptr) {
            return;
        }
        auto* script_component = game_object->queryComponent<ScriptComponent>();
        if (script_component == nullptr || script_component->script() == nullptr) {
            return;
        }
        script_component->script()->setField(field_name_, value);
    }

    GameObjectUUID uuid_;
    std::string field_name_;
    ScriptValue before_;
    ScriptValue after_;
};

// Prefab 实例整份重载:改前/改后各存一份对象 JSON,undo/redo 都按原 uuid 重建
// (removeGameObject 后 deserializeGameObject),对象身份与选中保持稳定。
class PrefabRevertCommand final : public EditorCommand {
public:
    PrefabRevertCommand(GameObjectUUID uuid, std::string before, std::string after)
        : uuid_(uuid), before_(std::move(before)), after_(std::move(after)) {}

    void undo() override { restore(before_); }
    void redo() override { restore(after_); }

private:
    void restore(const std::string& snapshot) {
        auto* game_object = findGameObject(uuid_);
        if (game_object == nullptr) {
            return;
        }
        auto* scene = global_context->scene_manager->getActiveScene();
        scene->removeGameObject(game_object);
        SceneSerializer::deserializeGameObject(scene, snapshot, uuid_);
    }

    GameObjectUUID uuid_;
    std::string before_;
    std::string after_;
};

}  // namespace

void UndoStack::push(std::unique_ptr<EditorCommand> command) {
    // 新命令入栈时清空 redo 栈
    redo_stack_.clear();
    undo_stack_.push_back(std::move(command));
    if (undo_stack_.size() > kMaxCommands) {
        undo_stack_.pop_front();
    }
    // 任何入栈的编辑操作都视为场景已修改(标题栏打星、切换场景时提示保存)。
    global_ui_context->scene_file_actions.markDirty();
}

void UndoStack::undo() {
    if (undo_stack_.empty()) {
        return;
    }
    auto command = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    command->undo();
    redo_stack_.push_back(std::move(command));
    WEN_CLIENT_INFO("Undo ({} remaining).", undo_stack_.size())
}

void UndoStack::redo() {
    if (redo_stack_.empty()) {
        return;
    }
    auto command = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    command->redo();
    undo_stack_.push_back(std::move(command));
    WEN_CLIENT_INFO("Redo ({} available).", redo_stack_.size())
}

void UndoStack::clear() {
    undo_stack_.clear();
    redo_stack_.clear();
}

MemberValue& pendingMemberEditValue() {
    static MemberValue value;
    return value;
}

ScriptValue& pendingScriptFieldValue() {
    static ScriptValue value;
    return value;
}

void pushScriptFieldEdit(GameObjectUUID uuid, const std::string& field_name,
                         const ScriptValue& before, const ScriptValue& after) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(
            std::make_unique<ScriptFieldEditCommand>(uuid, field_name, before, after));
    }
}

void pushPrefabRevert(GameObjectUUID uuid, const std::string& before, const std::string& after) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(std::make_unique<PrefabRevertCommand>(uuid, before, after));
    }
}

void pushPropertyEdit(GameObjectUUID uuid, const std::string& component_class,
                      std::vector<MemberChange> changes) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(
            std::make_unique<PropertyEditCommand>(uuid, component_class, std::move(changes)));
    }
}

void pushGameObjectCreated(GameObject* game_object) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(std::make_unique<GameObjectExistenceCommand>(game_object, true));
    }
}

void pushGameObjectDeleted(GameObject* game_object) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(std::make_unique<GameObjectExistenceCommand>(game_object, false));
    }
}

void pushGameObjectRenamed(GameObjectUUID uuid, const std::string& before, const std::string& after) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(std::make_unique<RenameCommand>(uuid, before, after));
    }
}

void pushComponentAdded(GameObject* game_object, Component* component) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(
            std::make_unique<ComponentExistenceCommand>(game_object->getUUID(), component, true));
    }
}

void pushComponentRemoved(GameObject* game_object, Component* component) {
    if (global_undo_stack != nullptr) {
        global_undo_stack->push(
            std::make_unique<ComponentExistenceCommand>(game_object->getUUID(), component, false));
    }
}

}  // namespace wen::editor
