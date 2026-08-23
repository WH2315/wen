#pragma once

#include "function/framework/component.hpp"
#include "function/framework/game_object.hpp"
#include "function/script/script.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <deque>
#include <memory>
#include <variant>

namespace wen::editor {

// 可撤销的编辑器操作。命令通过 uuid/类名/成员名定位目标而不持有指针:
// 对象可能在撤销链中被删除又重建(重建沿用原 uuid,身份保持不变)。
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

// 反射成员值,与反射系统支持的成员类型一一对应。
using MemberValue = std::variant<int, float, bool, glm::vec2, glm::vec3, glm::vec4, std::string>;

struct MemberChange {
    std::string name;
    MemberValue before;
    MemberValue after;
};

// 撤销/重做栈(上限 64 条)。
class UndoStack {
public:
    void push(std::unique_ptr<EditorCommand> command);
    void undo();
    void redo();
    void clear();

    bool canUndo() const { return !undo_stack_.empty(); }
    bool canRedo() const { return !redo_stack_.empty(); }

private:
    static constexpr size_t kMaxCommands = 64;
    std::deque<std::unique_ptr<EditorCommand>> undo_stack_;
    std::deque<std::unique_ptr<EditorCommand>> redo_stack_;
};

// 全局撤销栈(由 UI 持有生命周期)。
extern UndoStack* global_undo_stack;

// 命令入口(在操作发生处调用)。
void pushPropertyEdit(GameObjectUUID uuid, const std::string& component_class,
                      std::vector<MemberChange> changes);

void pushGameObjectCreated(GameObject* game_object);

void pushGameObjectDeleted(GameObject* game_object);

void pushGameObjectRenamed(GameObjectUUID uuid, const std::string& before, const std::string& after);

// 重设父子关系:记录前后父 uuid 与手时的世界矩阵,undo/redo 恢复到同一世界位置。
void pushReparentGameObject(GameObjectUUID uuid, GameObjectUUID before_parent,
                            GameObjectUUID after_parent, const glm::mat4& world_matrix);

// 组件增删(Inspector):添加/移除组件可撤销,移除时快照成员、恢复时重放。
void pushComponentAdded(GameObject* game_object, Component* component);
void pushComponentRemoved(GameObject* game_object, Component* component);

// 跨帧记录拖动手势起点的值(全局仅一个控件处于激活态)。
MemberValue& pendingMemberEditValue();
ScriptValue& pendingScriptFieldValue();

// 脚本字段编辑命令(Inspector 编辑脚本字段入撤销栈)。
void pushScriptFieldEdit(GameObjectUUID uuid, const std::string& field_name,
                         const ScriptValue& before, const ScriptValue& after);

// Prefab 实例整份重载:存改前/改后两份 JSON 快照,undo/redo 都按 uuid 重建对象。
void pushPrefabRevert(GameObjectUUID uuid, const std::string& before, const std::string& after);

// 紧跟在一个编辑控件之后调用:激活瞬间记为手势起点,手势结束
// (IsItemDeactivatedAfterEdit)时推入一条属性命令。
template <typename T>
void trackMemberEdit(Component* component, const std::string& member_name, const T& before_widget) {
    if (ImGui::IsItemActivated()) {
        pendingMemberEditValue() = before_widget;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        T after = component->getMember(member_name).getValueReferenceByPtr<T>(component);
        pushPropertyEdit(component->getGameObject()->getUUID(), component->getClassName(),
                         {{member_name, pendingMemberEditValue(), MemberValue(after)}});
    }
}

}  // namespace wen::editor
