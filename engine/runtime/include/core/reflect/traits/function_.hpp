#pragma once

#include "core/reflect/traits/any.hpp"

namespace wen {

class Function {
public:
    template <class C, typename... Args>
    Function(void (C::*func)(Args...)) {
        callback_ = [func_ptr = func](void* obj, std::any args) {
            auto wrapper =
                std::any_cast<std::array<Any, sizeof...(Args) + 1>*>(args);
            (*wrapper)[0] = Any(*static_cast<C*>(obj));
            std::tuple<C&, Args...> call_args = AnyToTuple<C&, Args...>(
                *wrapper, std::make_index_sequence<sizeof...(Args) + 1>{});
            std::apply(func_ptr, call_args);
            return std::any();
        };
        create_result_ = [](std::any) { return Any(); };
    }

    template <class C, typename... Args>
    Function(void (C::*func)(Args...) const) {
        callback_ = [func_ptr = func](void* obj, std::any args) {
            auto wrapper =
                std::any_cast<std::array<Any, sizeof...(Args) + 1>*>(args);
            (*wrapper)[0] = Any(*static_cast<const C*>(obj));
            std::tuple<const C&, Args...> call_args =
                AnyToTuple<const C&, Args...>(
                    *wrapper, std::make_index_sequence<sizeof...(Args) + 1>{});
            std::apply(func_ptr, call_args);
            return std::any();
        };
        create_result_ = [](std::any) { return Any(); };
    }

    template <typename R, class C, typename... Args>
    Function(R (C::*func)(Args...)) {
        callback_ = [func_ptr = func](void* obj, std::any args) {
            auto wrapper =
                std::any_cast<std::array<Any, sizeof...(Args) + 1>*>(args);
            (*wrapper)[0] = Any(*static_cast<C*>(obj));
            std::tuple<C&, Args...> call_args = AnyToTuple<C&, Args...>(
                *wrapper, std::make_index_sequence<sizeof...(Args) + 1>{});
            return std::any(std::apply(func_ptr, call_args));
        };
        create_result_ = [](std::any result) {
            return Any(std::any_cast<R>(result));
        };
    }

    template <typename R, class C, typename... Args>
    Function(R (C::*func)(Args...) const) {
        callback_ = [func_ptr = func](void* obj, std::any args) {
            auto wrapper =
                std::any_cast<std::array<Any, sizeof...(Args) + 1>*>(args);
            (*wrapper)[0] = Any(*static_cast<const C*>(obj));
            std::tuple<const C&, Args...> call_args =
                AnyToTuple<const C&, Args...>(
                    *wrapper, std::make_index_sequence<sizeof...(Args) + 1>{});
            return std::any(std::apply(func_ptr, call_args));
        };
        create_result_ = [](std::any result) {
            return Any(std::any_cast<R>(result));
        };
    }

    Function(const Function& rhs) {
        callback_ = rhs.callback_;
        create_result_ = rhs.create_result_;
    }

    Function(Function&& rhs) {
        callback_ = std::move(rhs.callback_);
        create_result_ = std::move(rhs.create_result_);
    }

    template <class C, typename... Args>
    Any invoke(C& obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return create_result_(callback_(&obj, &wrapper));
    }

    template <typename... Args>
    Any invokeByPtr(void* obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return create_result_(callback_(obj, &wrapper));
    }

    template <typename... Args>
    Any invokeByPtr(const void* obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return create_result_(callback_(obj, &wrapper));
    }

    template <typename R, class C, typename... Args>
    R invokeEx(C& obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return std::any_cast<R>(callback_(&obj, &wrapper));
    }

    template <typename R, typename... Args>
    R invokeByPtrEx(void* obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return std::any_cast<R>(callback_(obj, &wrapper));
    }

    template <typename R, typename... Args>
    R invokeByPtrEx(const void* obj, Args&&... args) const {
        std::array<Any, sizeof...(Args) + 1> wrapper = {
            Any(),
            Any(std::forward<Args>(args))...,
        };
        return std::any_cast<R>(callback_(obj, &wrapper));
    }

private:
    std::function<std::any(void*, std::any)> callback_;
    std::function<Any(std::any)> create_result_;
};

}  // namespace wen