#include <wen.hpp>

int main() {
    auto engine = std::make_unique<wen::Engine>();

    engine->startupEngine();

    engine->runEngine();

    engine->shutdownEngine();

    return 0;
}