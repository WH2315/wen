#pragma once

#include "core/reflect/traits/any.hpp"
#include <glm/glm.hpp>

namespace wen {

enum class MemberType {
    eInt,
    eFloat,
    eBool,
    eVec2,
    eVec3,
    eVec4,
    eString,
    eCustom
};

class Member {
public:
    template <class C, typename T>
    Member(T C::* member) {
        if constexpr (std::is_same_v<T, int>) {
            type_ = MemberType::eInt;
        } else if constexpr (std::is_same_v<T, float>) {
            type_ = MemberType::eFloat;
        } else if constexpr (std::is_same_v<T, bool>) {
            type_ = MemberType::eBool;
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            type_ = MemberType::eVec2;
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            type_ = MemberType::eVec3;
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            type_ = MemberType::eVec4;
        } else if constexpr (std::is_same_v<T, std::string>) {
            type_ = MemberType::eString;
        } else {
            type_ = MemberType::eCustom;
        }

        getter_const_ = [ptr = member](const void* obj) {
            return &(static_cast<const C*>(obj)->*ptr);
        };

        getter_reference_ = [ptr = member](void* obj) {
            return &(static_cast<C*>(obj)->*ptr);
        };

        setter_copy_ = [ptr = member](void* obj, std::any value) {
            static_cast<C*>(obj)->*ptr = *std::any_cast<T*>(value);
        };

        setter_copy_const_ = [ptr = member](void* obj, std::any value) {
            static_cast<C*>(obj)->*ptr = *std::any_cast<const T*>(value);
        };

        setter_move_ = [ptr = member](void* obj, std::any value) {
            static_cast<C*>(obj)->*ptr = std::move(*std::any_cast<T*>(value));
        };

        setter_value_ = [ptr = member](void* obj, std::any value) {
            static_cast<C*>(obj)->*ptr = std::any_cast<T>(value);
        };

        create_result_const_ = [](std::any value) {
            return Any(*std::any_cast<const T*>(value));
        };

        create_result_reference_ = [](std::any value) {
            return Any(*std::any_cast<T*>(value));
        };
    }

    Member(const Member& rhs) {
        type_ = rhs.type_;
        getter_const_ = rhs.getter_const_;
        getter_reference_ = rhs.getter_reference_;
        setter_copy_ = rhs.setter_copy_;
        setter_copy_const_ = rhs.setter_copy_const_;
        setter_move_ = rhs.setter_move_;
        setter_value_ = rhs.setter_value_;
        create_result_const_ = rhs.create_result_const_;
        create_result_reference_ = rhs.create_result_reference_;
    }

    Member(Member&& rhs) {
        type_ = rhs.type_;
        getter_const_ = std::move(rhs.getter_const_);
        getter_reference_ = std::move(rhs.getter_reference_);
        setter_copy_ = std::move(rhs.setter_copy_);
        setter_copy_const_ = std::move(rhs.setter_copy_const_);
        setter_move_ = std::move(rhs.setter_move_);
        setter_value_ = std::move(rhs.setter_value_);
        create_result_const_ = std::move(rhs.create_result_const_);
        create_result_reference_ = std::move(rhs.create_result_reference_);
    }

    template <class C>
    Any getValueConst(const C& obj) const {
        return create_result_const_(
            getter_const_(static_cast<const void*>(&obj)));
    }

    template <typename T, class C>
    const T& getValueConst(const C& obj) const {
        return *std::any_cast<const T*>(
            getter_const_(static_cast<const void*>(&obj)));
    }

    Any getValueConstByPtr(const void* obj) const {
        return create_result_const_(getter_const_(obj));
    }

    template <typename T>
    const T& getValueConstByPtr(const void* obj) const {
        return *std::any_cast<const T*>(getter_const_(obj));
    }

    template <class C>
    Any getValueReference(C& obj) const {
        return create_result_reference_(
            getter_reference_(static_cast<void*>(&obj)));
    }

    template <typename T, class C>
    T& getValueReference(C& obj) const {
        return *std::any_cast<T*>(getter_reference_(static_cast<void*>(&obj)));
    }

    Any getValueReferenceByPtr(void* obj) const {
        return create_result_reference_(getter_reference_(obj));
    }

    template <typename T>
    T& getValueReferenceByPtr(void* obj) const {
        return *std::any_cast<T*>(getter_reference_(obj));
    }

    template <class C, typename T>
    void setValue(C& obj, T&& value) const {
        if constexpr (std::is_lvalue_reference_v<T>) {
            if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
                setter_copy_const_(static_cast<void*>(&obj), &value);
            } else {
                setter_copy_(static_cast<void*>(&obj), &value);
            }
        } else if constexpr (std::is_rvalue_reference_v<T>) {
            setter_move_(static_cast<void*>(&obj), &value);
        } else {
            setter_value_(static_cast<void*>(&obj), value);
        }
    }

    template <typename T>
    void setValueByPtr(void* obj, T&& value) const {
        if constexpr (std::is_lvalue_reference_v<T>) {
            if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
                setter_copy_const_(obj, &value);
            } else {
                setter_copy_(obj, &value);
            }
        } else if constexpr (std::is_rvalue_reference_v<T>) {
            setter_move_(obj, &value);
        } else {
            setter_value_(obj, value);
        }
    }

    MemberType getType() const { return type_; }

private:
    MemberType type_;
    std::function<std::any(const void*)> getter_const_;
    std::function<std::any(void*)> getter_reference_;
    std::function<void(void*, std::any)> setter_copy_;
    std::function<void(void*, std::any)> setter_copy_const_;
    std::function<void(void*, std::any)> setter_move_;
    std::function<void(void*, std::any)> setter_value_;
    std::function<Any(std::any)> create_result_const_;
    std::function<Any(std::any)> create_result_reference_;
};

}  // namespace wen