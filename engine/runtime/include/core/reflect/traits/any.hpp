#pragma once

#include <any>
#include "core/base/macro.hpp"

namespace wen {

class Any {
public:
    Any() { is_void_ = true; }

    Any(std::any rhs) {
        ptr_ = rhs;
        is_const_ = false;
        is_ptr_ = true;
        is_void_ = false;
    }

    template <typename T>
    Any(T&& arg) {
        if constexpr (std::is_reference_v<T>) {
            ptr_ = &arg;
            is_ptr_ = true;
        } else {
            if constexpr (std::is_copy_assignable_v<
                              std::remove_reference_t<T>>) {
                ptr_ = arg;
                is_ptr_ = false;
            } else {
                ptr_ = &arg;
                is_ptr_ = false;
            }
        }
        is_const_ = std::is_const_v<std::remove_reference_t<T>>;
        is_void_ = false;
    }

    template <typename T>
    T cast() {
        if constexpr (std::is_void_v<T>) {
            return;
        }
        if (is_void_) {
            WEN_CORE_FATAL("Convert void to {} is not allowed",
                           typeid(T).name())
        }

        using RealT = std::remove_reference_t<T>;
        using RawT = std::remove_cv_t<RealT>;
        if constexpr (std::is_reference_v<T>) {
            if (is_ptr_) {
                if constexpr (std::is_const_v<RealT>) {
                    if (is_const_) {
                        return *std::any_cast<const RawT*>(ptr_);
                    }
                    return *std::any_cast<RawT*>(ptr_);
                } else {
                    if (is_const_) {
                        WEN_CORE_FATAL(
                            "Convert const reference to non-const "
                            "reference is not allowed")
                    }
                    return *std::any_cast<RawT*>(ptr_);
                }
            }
            return *std::any_cast<RawT>(&ptr_);
        } else {
            if constexpr (std::is_copy_constructible_v<RawT>) {
                if (is_ptr_) {
                    if (is_const_) {
                        return *std::any_cast<const RawT*>(ptr_);
                    }
                    return *std::any_cast<RawT*>(ptr_);
                }
                return std::any_cast<RawT>(ptr_);
            } else {
                WEN_CORE_FATAL(
                    "Convert to non-copy-constructible type {} is not allowed",
                    typeid(T).name())
            }
        }
    }

private:
    std::any ptr_{nullptr};
    bool is_const_;
    bool is_ptr_;
    bool is_void_;
};

template <typename... Args, size_t... Indices>
std::tuple<Args...> AnyToTuple(std::array<Any, sizeof...(Args)>& arr,
                               std::index_sequence<Indices...>) {
    return std::forward_as_tuple(arr[Indices].template cast<Args>()...);
}

}  // namespace wen