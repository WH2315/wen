#include <wen.hpp>
#include "reflect_example.hpp"

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    auto& rs = *global_context->reflect_system;
    rs.registerReflectProperties();
    B::C::A::D::ExampleReflect example1;
    B::C::A::D::ExampleReflect example2;
    example2.name = "小明";
    example2.health = 101;
    auto& my_class = rs.getClass("ExampleReflect");

    // 类成员的反射测试
    {
        auto& member0 = my_class.getMember("Health");
        auto& member1 = my_class.getMember("name");
        WEN_CLIENT_DEBUG("example1: name : {}, health : {}",
                         member1.getValueConst(example1).cast<std::string>(),
                         member0.getValueConst<int>(example1))
        WEN_CLIENT_DEBUG("example2: name : {}, health : {}",
                         member1.getValueConst(example2).cast<std::string>(),
                         member0.getValueConst<int>(example2))
        member0.setValue(example1, 200);
        member1.setValueByPtr(&example1, (std::string) "小红");
        WEN_CLIENT_DEBUG("example1: name : {}, health : {}",
                         member1.getValueReference<std::string>(example1),
                         member0.getValueReference(example1).cast<int>())
        WEN_CLIENT_DEBUG("成员的类型：{}, name(std::string)",
                         (int)member1.getType())
        WEN_CLIENT_DEBUG("成员的类型：{}, health(int)", (int)member0.getType())

        auto& secret_member = my_class.getMember("secret_value");
        WEN_CLIENT_DEBUG("example1: secret_value : {}",
                         secret_member.getValueConstByPtr<float>(&example1))
        secret_member.setValue(example1, 2.0f);
        WEN_CLIENT_DEBUG("example1: secret_value(changed) : {}",
                         secret_member.getValueReferenceByPtr<float>(&example1))
    }
    // 类函数的反射测试
    {
        auto& func0 = my_class.getFunction("Reset");
        auto& func1 = my_class.getFunction("add");
        WEN_CLIENT_DEBUG(
            "example1: name : {}, health : {}",
            my_class.getMember("name").getValueConst<std::string>(example1),
            my_class.getMember("Health").getValueConst<int>(example1))
        func0.invoke(example1);
        WEN_CLIENT_DEBUG(
            "example1: name : {}, health : {}",
            my_class.getMember("name").getValueConst<std::string>(example1),
            my_class.getMember("Health").getValueConst<int>(example1))
        auto result = func1.invoke(example1, 10, 20);
        WEN_CLIENT_DEBUG("add(10, 20) = {}", result.cast<int>())
    }

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}