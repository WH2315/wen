#pragma once

#include <wen.hpp>
#include <iostream>

struct Parent {
    REFLECT_CLASS()
    SERIALIZABLE_CLASS

    REFLECT_MEMBER()
    SERIALIZABLE_MEMBER
    double num = 0.0;

    REFLECT_MEMBER()
    SERIALIZABLE_MEMBER
    uint32_t num2 = -1u;

    REFLECT_FUNCTION()
    virtual void func() { std::cout << "Parent::func" << std::endl; }
};

class Child : public Parent {
    REFLECT_CLASS()
    SERIALIZABLE_CLASS
    SERIALIZE_PARENT_CLASS

public:
    REFLECT_MEMBER()
    SERIALIZABLE_MEMBER
    std::string name = "qwe";

    REFLECT_MEMBER()
    SERIALIZABLE_MEMBER
    double num = 11.11;

    REFLECT_FUNCTION()
    void func() override { std::cout << "Child::func" << std::endl; }
};

namespace B::C {

namespace A::D {

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

}  // namespace A::D

}  // namespace B::C