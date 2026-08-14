#include "function/script/script_registry.hpp"
#include "function/script/builtin_scripts.hpp"

namespace wen {

ScriptRegistry::ScriptRegistry() {
    registerScript<RotateScript>("RotateScript");
    registerScript<MoveScript>("MoveScript");
}

ScriptRegistry::~ScriptRegistry() = default;

}  // namespace wen
