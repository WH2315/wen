#include "ui/undo.hpp"
#include "ui/ui_context.hpp"
#include "ui/selection.hpp"
#include "ui/editor_scene.hpp"
#include "engine/global_context.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/scene_serializer.hpp"
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

}  // namespace

void UndoStack::push(std::unique_ptr<EditorCommand> command) {
    // 新命令入栈时清空 redo 栈
    redo_stack_.clear();
    undo_stack_.push_back(std::move(command));
    if (undo_stack_.size() > kMaxCommands) {
        undo_stack_.pop_front();
    }
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

}  // namespace wen::editor
