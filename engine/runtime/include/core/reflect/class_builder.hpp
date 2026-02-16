#pragma once

#include "core/reflect/traits/member.hpp"
#include "core/reflect/traits/function.hpp"

namespace wen {

class ClassDescriptor {
    template <class C>
    friend class ClassBuilder;
    friend class ReflectSystem;

public:
    ClassDescriptor(const std::string& class_name) : class_name_(class_name) {}

    const Member& getMember(const std::string& name) const {
        if (members_.find(name) == members_.end()) {
            WEN_CORE_ERROR("Member {} not in class {}", name, class_name_)
        }
        return members_.at(name);
    }

    const Function& getFunction(const std::string& name) const {
        if (functions_.find(name) == functions_.end()) {
            WEN_CORE_ERROR("Function {} not in class {}", name, class_name_)
        }
        return functions_.at(name);
    }

private:
    std::string class_name_;
    std::map<std::string, Member> members_;
    std::map<std::string, Function> functions_;
};

template <typename C>
class ClassBuilder {
    friend class ReflectSystem;

public:
    ClassBuilder(const std::string& class_name) {
        descriptor_ = new ClassDescriptor(class_name);
    }

    ClassBuilder(ClassBuilder&& rhs) {
        descriptor_ = rhs.descriptor_;
        rhs.descriptor_ = nullptr;
    }

    ~ClassBuilder() {}

    template <typename T>
    ClassBuilder& addMember(const std::string& name, T C::* member) {
        auto& members = descriptor_->members_;
        if (members.find(name) == members.end()) {
            members.insert({name, Member(member)});
        } else {
            WEN_CORE_ERROR("Member {} already exists in class {}.", name,
                           descriptor_->class_name_)
        }
        return *this;
    }

    template <typename T>
    ClassBuilder& addFunction(const std::string& name, T func) {
        auto& functions = descriptor_->functions_;
        if (functions.find(name) == functions.end()) {
            functions.insert({name, Function(func)});
        } else {
            WEN_CORE_ERROR("Function {} already exists in class {}.", name,
                           descriptor_->class_name_)
        }
        return *this;
    }

private:
    ClassDescriptor* descriptor_;
};

}  // namespace wen