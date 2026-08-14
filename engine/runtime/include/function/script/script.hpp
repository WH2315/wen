#pragma once

#include "function/framework/game_object.hpp"
#include <glm/glm.hpp>
#include <string>
#include <variant>
#include <vector>

namespace wen {

// 脚本字段值:运行时自描述的变体,与编辑器 MemberValue 结构一致但定义在 runtime,
// 避免 runtime 依赖 editor。索引顺序:0=int 1=float 2=bool 3=vec2 4=vec3 5=vec4 6=string。
using ScriptValue = std::variant<int, float, bool, glm::vec2, glm::vec3, glm::vec4, std::string>;

struct ScriptField {
    std::string name;
    ScriptValue value;
};

// 原生脚本基类:onStart/onTick/onFixedTick/onDestroy 由宿主 ScriptComponent 在 Play
// 模式下驱动。字段在派生类构造函数里 declareField(name, 默认值) 自描述,供 Inspector
// 编辑与场景序列化,不参与 parser 反射代码生成。
class Script {
public:
    virtual ~Script() = default;

    virtual std::string getScriptName() const = 0;

    void setOwner(GameObject* owner) { owner_ = owner; }
    GameObject* owner() const { return owner_; }

    virtual void onStart() {}
    virtual void onTick(float dt) {}
    virtual void onFixedTick() {}
    virtual void onDestroy() {}

    // 声明/读取字段。
    void declareField(std::string name, ScriptValue value) {
        for (auto& field : fields_) {
            if (field.name == name) {
                field.value = std::move(value);
                return;
            }
        }
        fields_.push_back({std::move(name), std::move(value)});
    }

    void setField(const std::string& name, const ScriptValue& value) {
        for (auto& field : fields_) {
            if (field.name == name) {
                field.value = value;
                onFieldChanged(name);
                return;
            }
        }
    }

    std::vector<ScriptField>& fields() { return fields_; }
    const std::vector<ScriptField>& fields() const { return fields_; }

    template <typename T>
    T get(const std::string& name) const {
        for (const auto& field : fields_) {
            if (field.name == name) {
                return std::get<T>(field.value);
            }
        }
        return T{};
    }

    // 字段被编辑(Inspector / 反序列化)后回调;派生类可按需响应。
    virtual void onFieldChanged(const std::string& /*name*/) {}

protected:
    GameObject* owner_ = nullptr;
    std::vector<ScriptField> fields_;
};

}  // namespace wen
