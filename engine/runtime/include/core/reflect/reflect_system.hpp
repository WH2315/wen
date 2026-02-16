#pragma once

#include "core/base/singleton.hpp"
#include "core/reflect/class_builder.hpp"

namespace wen {

class ReflectSystem final {
    friend class Singleton<ReflectSystem>;
    ReflectSystem();
    ~ReflectSystem();

public:
    template <class C>
    ClassBuilder<C> addClass(const std::string& class_name) {
        auto class_builder = ClassBuilder<C>(class_name);
        auto descriptor = class_builder.descriptor_;
        if (classes_.find(descriptor->class_name_) == classes_.end()) {
            classes_.insert({descriptor->class_name_, descriptor});
        } else {
            WEN_CORE_ERROR("Class {} has been added.", descriptor->class_name_)
        }
        return class_builder;
    }

    const ClassDescriptor& getClass(const std::string& name) const;

    void registerReflectProperties();

private:
    std::map<std::string, ClassDescriptor*> classes_;
};

}  // namespace wen