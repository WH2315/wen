#pragma once

#include "core/reflect/reflect.hpp"
#include "core/reflect/reflect_system.hpp"

namespace wen {

class RTTI {
public:
    void setupRTTI();
    virtual std::string getClassName() const { return ""; }
    static std::string GetClassName() { return ""; }

public:
    template <typename T>
    const T& getValueConst(const std::string& name) const {
        return descriptor_->getMember(name).getValueConstByPtr<T>(this);
    }

    template <typename T>
    T& getValueRef(const std::string& name) {
        return descriptor_->getMember(name).getValueReferenceByPtr<T>(this);
    }

    template <typename T>
    void setValue(const std::string& name, T&& value) {
        descriptor_->getMember(name).setValueByPtr(this, std::forward<T>(value));
    }

    template <typename... Args>
    Any invoke(const std::string& name, Args&&... args) {
        return descriptor_->getFunction(name).invokeByPtr<Args...>(this, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    R invokeEx(const std::string& name, Args&&... args) {
        return descriptor_->getFunction(name).invokeByPtrEx<R, Args...>(this, std::forward<Args>(args)...);
    }

    const auto& getMember(const std::string& name) const {
        return descriptor_->getMember(name);
    }

    const auto& getFunction(const std::string& name) const {
        return descriptor_->getFunction(name);
    }

private:
    const ClassDescriptor* descriptor_;
};

}  // namespace wen