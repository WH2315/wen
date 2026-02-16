#pragma once

#include "core/reflect/reflect.hpp"

#include <string>

namespace B::C {

namespace A::D {

class ExampleReflect {
    REFLECT_CLASS("ExampleReflect")

public:
    ExampleReflect() = default;

    REFLECT_MEMBER("Health")
    int health = 100;

    REFLECT_MEMBER()
    std::string name = "hero";

    REFLECT_FUNCTION("Reset")
    void reset() { health = 100; }

private:
    ALLOW_PRIVATE_REFLECT()

    REFLECT_MEMBER("secret_value")
    float secret_value = 1.0f;

    REFLECT_FUNCTION()
    int add(int a, int b) const { return a + b; }
};

}  // namespace A::D

}  // namespace B::C