#include <wen.hpp>
#include <pch.hpp>

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}