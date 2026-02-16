#include "core/reflect/reflect_system.hpp"
#include "core/reflect/reflect.hpp"

namespace wen {

ReflectSystem::ReflectSystem() {}

const ClassDescriptor& ReflectSystem::getClass(const std::string& name) const {
    if (classes_.find(name) == classes_.end()) {
        WEN_CORE_ERROR("Class {} not found.", name)
        static ClassDescriptor* empty = nullptr;
        return *empty;
    }
    return *classes_.at(name);
}

void ReflectSystem::registerReflectProperties() { Parser(); }

ReflectSystem::~ReflectSystem() {
    for (auto& [name, descriptor] : classes_) {
        delete descriptor;
    }
    classes_.clear();
}

}  // namespace wen