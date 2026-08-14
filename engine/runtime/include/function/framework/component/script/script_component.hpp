#pragma once

#include "function/framework/component.hpp"
#include "function/script/script.hpp"
#include "function/script/lua_script.hpp"
#include "engine/global_context.hpp"

namespace wen {

// 脚本组件:按反射成员 script_type + script_name 实例化脚本(原生从 ScriptRegistry,
// Lua 加载 <root>/scripts/<name>),转发生命周期(onStart/onTick/onFixedTick)并持有
// 脚本字段(Inspector 编辑 + 场景序列化)。
// 脚本只在 Play 模式运行:Editor 的 Edit 模式不调用 tickLogic,onTick 不会触发。
class ScriptComponent : public Component {
    REFLECT_CLASS("ScriptComponent")

public:
    std::string getClassName() const override { return "ScriptComponent"; }
    static std::string GetClassName() { return "ScriptComponent"; }

    ScriptComponent() {
        // 脚本类型/名字变化(编辑器下拉 / 撤销恢复)时重载脚本;字段编辑不经过这里。
        addMemberUpdateCallback([this](Component*) { reloadScript(); });
    }

    // 反射成员:脚本语言类型("Native" 原生 C++ / "Lua"),空脚本名 = 未选。
    REFLECT_MEMBER()
    std::string script_type = "Native";

    // 反射成员:要运行的脚本名(原生 = 注册表名;Lua = <root>/scripts 下的 .lua 文件名)。
    REFLECT_MEMBER()
    std::string script_name;

    void onCreate() override {
        reloadScript();
        // 若脚本在 addComponent 前已通过 setScriptName 实例化(reload 守卫会跳过),
        // 这里确保 owner 指向当前对象。
        if (script_ != nullptr) {
            script_->setOwner(game_object_);
        }
    }

    void onDestroy() override {
        releaseScript();
    }

    void onStart() override {
        if (script_ != nullptr) {
            script_->onStart();
        }
    }

    void onTick(float dt) override {
        if (script_ != nullptr) {
            script_->onTick(dt);
        }
    }

    void onFixedTick() override {
        if (script_ != nullptr) {
            script_->onFixedTick();
        }
    }

    Script* script() const { return script_; }

    // 编辑器设置脚本名后立即重载(成员回调是撤销/重做路径的唯一重载点)。
    void setScriptName(const std::string& name) {
        script_name = name;
        reloadScript();
    }

    // 手动重载(Inspector "Reload" 按钮,改 .lua 后热重载)。
    void reload() {
        reloadScript();
    }

private:
    void releaseScript() {
        if (script_ != nullptr) {
            script_->onDestroy();
            delete script_;
            script_ = nullptr;
        }
    }

    void reloadScript() {
        // 类型与名字都未变且已有脚本:保留现有脚本(及其已恢复的字段),避免场景加载后
        // 成员回调把序列化的字段重置回默认值。
        if (loaded_type_ == script_type && loaded_name_ == script_name && script_ != nullptr) {
            return;
        }
        releaseScript();
        loaded_type_ = script_type;
        loaded_name_ = script_name;
        if (script_name.empty()) {
            return;
        }
        if (script_type == "Lua") {
            script_ = new LuaScript(script_name);
        } else {
            script_ = global_context->script_registry->create(script_name);
        }
        if (script_ == nullptr) {
            WEN_CORE_WARN("ScriptComponent: unknown script \"{}\" (type {}).", script_name, script_type)
            return;
        }
        script_->setOwner(game_object_);
    }

    Script* script_ = nullptr;
    std::string loaded_type_;
    std::string loaded_name_;
};

}  // namespace wen
