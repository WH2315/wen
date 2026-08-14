#include "function/script/lua_script.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include <filesystem>
#include <type_traits>

namespace wen {

LuaScript::LuaScript(std::string file_name) : file_name_(std::move(file_name)) {
    L_ = luaL_newstate();
    if (L_ == nullptr) {
        WEN_CORE_ERROR("LuaScript: failed to create lua state for \"{}\".", file_name_)
        return;
    }
    luaL_openlibs(L_);
    registerWenApi();
    load();
}

LuaScript::~LuaScript() {
    if (L_ != nullptr) {
        lua_close(L_);
        L_ = nullptr;
    }
}

void LuaScript::onStart() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    callLuaFunction("onStart");
}

void LuaScript::onTick(float dt) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (L_ == nullptr) {
        return;
    }
    lua_getglobal(L_, "onTick");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return;
    }
    lua_pushnumber(L_, static_cast<lua_Number>(dt));
    if (lua_pcall(L_, 1, 0, 0) != LUA_OK) {
        logLuaError();
    }
}

void LuaScript::onFixedTick() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    callLuaFunction("onFixedTick");
}

void LuaScript::onDestroy() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    callLuaFunction("onDestroy");
}

void LuaScript::onFieldChanged(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (L_ == nullptr) {
        return;
    }
    for (const auto& field : fields_) {
        if (field.name != name) {
            continue;
        }
        lua_getglobal(L_, "fields");
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            return;
        }
        pushScriptValue(L_, field.value);
        lua_setfield(L_, -2, name.c_str());
        lua_pop(L_, 1);
        return;
    }
}

// ------------------------------------------------------------------
// 内部
// ------------------------------------------------------------------

void LuaScript::registerWenApi() {
    // 在 registry 里存 this,供 wen.* C 函数取回宿主。
    lua_pushlightuserdata(L_, this);
    lua_setfield(L_, LUA_REGISTRYINDEX, "wen_script");

    static const luaL_Reg funcs[] = {
        {"get_location", LuaScript::l_wen_get_location},
        {"set_location", LuaScript::l_wen_set_location},
        {"get_rotation", LuaScript::l_wen_get_rotation},
        {"set_rotation", LuaScript::l_wen_set_rotation},
        {"get_scale", LuaScript::l_wen_get_scale},
        {"set_scale", LuaScript::l_wen_set_scale},
        {"get_field", LuaScript::l_wen_get_field},
        {"set_field", LuaScript::l_wen_set_field},
        {"log", LuaScript::l_wen_log},
        {nullptr, nullptr},
    };
    lua_newtable(L_);
    luaL_setfuncs(L_, funcs, 0);
    lua_setglobal(L_, "wen");
}

void LuaScript::load() {
    if (L_ == nullptr) {
        return;
    }
    auto scripts_dir = std::filesystem::path(global_context->asset_system->getRootDir()) / "scripts";
    auto path = scripts_dir / file_name_;
    if (luaL_loadfile(L_, path.string().c_str()) != LUA_OK) {
        WEN_CORE_ERROR("LuaScript: cannot load {}: {}", path.string(), lua_tostring(L_, -1))
        lua_pop(L_, 1);
        return;
    }
    if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
        WEN_CORE_ERROR("LuaScript: error running {}: {}", path.string(), lua_tostring(L_, -1))
        lua_pop(L_, 1);
        return;
    }
    readFields();
}

void LuaScript::readFields() {
    if (L_ == nullptr) {
        return;
    }
    lua_getglobal(L_, "fields");
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        return;
    }
    lua_pushnil(L_);
    while (lua_next(L_, -2) != 0) {
        // 栈:... fields key value
        if (lua_type(L_, -2) == LUA_TSTRING) {
            std::string name = lua_tostring(L_, -2);
            fields_.push_back({std::move(name), luaValueToScript(L_, lua_gettop(L_))});
        }
        lua_pop(L_, 1);  // 弹出 value,保留 key 供下一轮
    }
    lua_pop(L_, 1);  // 弹出 fields 表
}

void LuaScript::callLuaFunction(const char* name) {
    if (L_ == nullptr) {
        return;
    }
    lua_getglobal(L_, name);
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return;
    }
    if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
        logLuaError();
    }
}

void LuaScript::logLuaError() {
    WEN_CORE_ERROR("LuaScript[{}]: {}", file_name_, lua_tostring(L_, -1))
    lua_pop(L_, 1);
}

LuaScript* LuaScript::getSelf(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "wen_script");
    auto* self = static_cast<LuaScript*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return self;
}

TransformComponent* LuaScript::getTransform(lua_State* L) {
    auto* self = getSelf(L);
    if (self == nullptr || self->owner() == nullptr) {
        return nullptr;
    }
    return self->owner()->queryComponent<TransformComponent>();
}

ScriptValue LuaScript::luaValueToScript(lua_State* L, int index) {
    switch (lua_type(L, index)) {
        case LUA_TBOOLEAN:
            return ScriptValue(lua_toboolean(L, index) != 0);
        case LUA_TSTRING:
            return ScriptValue(std::string(lua_tostring(L, index)));
        case LUA_TNUMBER:
            if (lua_isinteger(L, index)) {
                return ScriptValue(static_cast<int>(lua_tointeger(L, index)));
            }
            return ScriptValue(static_cast<float>(lua_tonumber(L, index)));
        case LUA_TTABLE: {
            // 数组 {x,y,z} → vec2/3/4,按元素个数判定。
            size_t n = lua_rawlen(L, index);
            if (n >= 4) {
                glm::vec4 v(0.0f);
                for (size_t i = 0; i < 4; i++) {
                    lua_rawgeti(L, index, static_cast<int>(i + 1));
                    v[i] = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                }
                return ScriptValue(v);
            }
            if (n == 3) {
                glm::vec3 v(0.0f);
                for (size_t i = 0; i < 3; i++) {
                    lua_rawgeti(L, index, static_cast<int>(i + 1));
                    v[i] = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                }
                return ScriptValue(v);
            }
            if (n == 2) {
                glm::vec2 v(0.0f);
                for (size_t i = 0; i < 2; i++) {
                    lua_rawgeti(L, index, static_cast<int>(i + 1));
                    v[i] = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                }
                return ScriptValue(v);
            }
            return ScriptValue(0.0f);
        }
        default:
            return ScriptValue(0.0f);
    }
}

void LuaScript::pushScriptValue(lua_State* L, const ScriptValue& value) {
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) {
            lua_pushinteger(L, v);
        } else if constexpr (std::is_same_v<T, float>) {
            lua_pushnumber(L, static_cast<lua_Number>(v));
        } else if constexpr (std::is_same_v<T, bool>) {
            lua_pushboolean(L, v);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            lua_createtable(L, 2, 0);
            lua_pushnumber(L, v.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, v.y);
            lua_rawseti(L, -2, 2);
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            lua_createtable(L, 3, 0);
            lua_pushnumber(L, v.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, v.y);
            lua_rawseti(L, -2, 2);
            lua_pushnumber(L, v.z);
            lua_rawseti(L, -2, 3);
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            lua_createtable(L, 4, 0);
            lua_pushnumber(L, v.x);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, v.y);
            lua_rawseti(L, -2, 2);
            lua_pushnumber(L, v.z);
            lua_rawseti(L, -2, 3);
            lua_pushnumber(L, v.w);
            lua_rawseti(L, -2, 4);
        } else {
            lua_pushstring(L, v.c_str());
        }
    }, value);
}

// ------------------------------------------------------------------
// wen.* C 函数
// ------------------------------------------------------------------

int LuaScript::l_wen_get_location(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 3;
    }
    lua_pushnumber(L, t->location.x);
    lua_pushnumber(L, t->location.y);
    lua_pushnumber(L, t->location.z);
    return 3;
}

int LuaScript::l_wen_set_location(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        return 0;
    }
    t->location = glm::vec3(static_cast<float>(luaL_checknumber(L, 1)),
                            static_cast<float>(luaL_checknumber(L, 2)),
                            static_cast<float>(luaL_checknumber(L, 3)));
    t->triggerMemberUpdateCallbacks();
    return 0;
}

int LuaScript::l_wen_get_rotation(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 3;
    }
    lua_pushnumber(L, t->rotation.x);
    lua_pushnumber(L, t->rotation.y);
    lua_pushnumber(L, t->rotation.z);
    return 3;
}

int LuaScript::l_wen_set_rotation(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        return 0;
    }
    t->rotation = glm::vec3(static_cast<float>(luaL_checknumber(L, 1)),
                            static_cast<float>(luaL_checknumber(L, 2)),
                            static_cast<float>(luaL_checknumber(L, 3)));
    t->triggerMemberUpdateCallbacks();
    return 0;
}

int LuaScript::l_wen_get_scale(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 1);
        return 3;
    }
    lua_pushnumber(L, t->scale.x);
    lua_pushnumber(L, t->scale.y);
    lua_pushnumber(L, t->scale.z);
    return 3;
}

int LuaScript::l_wen_set_scale(lua_State* L) {
    auto* t = getTransform(L);
    if (t == nullptr) {
        return 0;
    }
    t->scale = glm::vec3(static_cast<float>(luaL_checknumber(L, 1)),
                         static_cast<float>(luaL_checknumber(L, 2)),
                         static_cast<float>(luaL_checknumber(L, 3)));
    t->triggerMemberUpdateCallbacks();
    return 0;
}

int LuaScript::l_wen_get_field(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    // 从 Lua 的 fields 表读取(setField/onFieldChanged 始终保持它与 C++ 同步)。
    lua_getglobal(L, "fields");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, name);
    lua_remove(L, -2);  // 弹出 fields 表,保留字段值
    return 1;
}

int LuaScript::l_wen_set_field(lua_State* L) {
    auto* self = getSelf(L);
    if (self == nullptr) {
        return 0;
    }
    const char* name = luaL_checkstring(L, 1);
    self->setField(name, luaValueToScript(L, 2));
    return 0;
}

int LuaScript::l_wen_log(lua_State* L) {
    int n = lua_gettop(L);
    std::string msg;
    for (int i = 1; i <= n; i++) {
        if (i > 1) {
            msg += "  ";
        }
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        msg.append(s, len);
        lua_pop(L, 1);  // 弹出 luaL_tolstring 压入的字符串
    }
    WEN_CLIENT_INFO("Lua: {}", msg)
    return 0;
}

}  // namespace wen
