#pragma once

#include "pch.hpp"
#include "engine/global_context.hpp"

namespace wen {

class Engine final {
public:
    Engine() = default;
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    ~Engine() = default;

    void startupEngine();
    void shutdownEngine();

    void runEngine();

private:
};

}  // namespace wen