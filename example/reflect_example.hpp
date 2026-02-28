#pragma once

#include <wen.hpp>

namespace A {

class ExampleReflect {
    REFLECT_CLASS("ExampleReflect")
    SERIALIZABLE_CLASS

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
    ALL_PRIVATE_SERIALIZATION(ExampleReflect)

    REFLECT_MEMBER("secret_value")
    SERIALIZABLE_MEMBER
    float secret_value = 1.0f;

    REFLECT_FUNCTION()
    int add(int a, int b) const { return a + b; }
};

}  // namespace A