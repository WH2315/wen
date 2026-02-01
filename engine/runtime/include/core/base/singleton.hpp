#pragma once

#include <stdexcept>

namespace wen {

template <typename T>
class Singleton {
public:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;

    ~Singleton() { destroy(); }

    template <typename... Args>
    void initialize(Args&&... args) {
        if (instance_ == nullptr) {
            instance_ = new T(std::forward<Args>(args)...);
        } else {
            throw std::runtime_error(
                "This system has already been registered!");
        }
    }

    void destroy() {
        if (instance_ != nullptr) {
            delete instance_;
            instance_ = nullptr;
        }
    }

    T* getInstance() noexcept { return instance_; }
    const T* getInstance() const noexcept { return instance_; }

    T* operator->() noexcept { return instance_; }
    const T* operator->() const noexcept { return instance_; }

    T& operator*() noexcept { return *instance_; }
    const T& operator*() const noexcept { return *instance_; }

private:
    T* instance_ = nullptr;
};

}  // namespace wen