#pragma once

#include "function/script/script.hpp"
#include <lua.hpp>
#include <mutex>
#include <string>

namespace wen {

class TransformComponent;

// Lua 脚本适配器:每个实例一个 lua_State(隔离),从 <root>/scripts/<file_name> 加载 .lua。
// 暴露 wen.* API(变换 + 字段 + log);脚本的 fields 全局表自动映射成 ScriptField,
// 复用 Script 的字段机制 —— Inspector 编辑、场景序列化、撤销栈与原生脚本完全一致。
// 所有 Lua 调用经 lua_pcall,脚本报错只 log,不崩引擎。
class LuaScript : public Script {
public:
    explicit LuaScript(std::string file_name);
    ~LuaScript() override;

    std::string getScriptName() const override { return file_name_; }

    void onStart() override;
    void onTick(float dt) override;
    void onFixedTick() override;
    void onDestroy() override;
    // Inspector/反序列化/撤销改了字段后,把新值写回 Lua 的 fields 表。
    void onFieldChanged(const std::string& name) override;

private:
    // wen.* 的 C 函数(经 Lua registry 取回 this)。
    static int l_wen_get_location(lua_State* L);
    static int l_wen_set_location(lua_State* L);
    static int l_wen_get_rotation(lua_State* L);
    static int l_wen_set_rotation(lua_State* L);
    static int l_wen_get_scale(lua_State* L);
    static int l_wen_set_scale(lua_State* L);
    static int l_wen_get_field(lua_State* L);
    static int l_wen_set_field(lua_State* L);
    static int l_wen_log(lua_State* L);

    static LuaScript* getSelf(lua_State* L);
    static TransformComponent* getTransform(lua_State* L);
    static ScriptValue luaValueToScript(lua_State* L, int index);
    static void pushScriptValue(lua_State* L, const ScriptValue& value);

    void registerWenApi();
    void load();
    void readFields();
    void callLuaFunction(const char* name);
    void logLuaError();

    // 固定步长线程(onFixedTick)与主线程(onTick)会并发访问同一个 lua_State,
    // 必须用递归锁串行化;wen.* C 函数在 pcall 内运行,递归锁允许其重入。
    std::string file_name_;
    lua_State* L_ = nullptr;
    mutable std::recursive_mutex mutex_;
};

}  // namespace wen
