#pragma once

#include "core/base/singleton.hpp"
#include "function/script/script.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace wen {

// 脚本注册表:脚本名 → 工厂。内置脚本在构造函数里注册(见 script_registry.cpp),
// 后续可注册自定义脚本。ScriptComponent 按 script_name 从这里实例化脚本。
class ScriptRegistry final {
    friend class Singleton<ScriptRegistry>;
    ScriptRegistry();
    ~ScriptRegistry();

public:
    template <class ScriptCls>
    void registerScript(const std::string& name) {
        factories_[name] = []() -> Script* { return new ScriptCls; };
    }

    Script* create(const std::string& name) const {
        auto iter = factories_.find(name);
        return iter == factories_.end() ? nullptr : iter->second();
    }

    bool contains(const std::string& name) const {
        return factories_.find(name) != factories_.end();
    }

    std::vector<std::string> names() const {
        std::vector<std::string> out;
        out.reserve(factories_.size());
        for (const auto& [name, factory] : factories_) {
            (void)factory;
            out.push_back(name);
        }
        return out;
    }

private:
    std::map<std::string, std::function<Script*()>> factories_;
};

}  // namespace wen
