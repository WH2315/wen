#include "function/render/render_system.hpp"
#include <glslang/Public/ShaderLang.h>

namespace wen {

RenderSystem::RenderSystem(const Renderer::Configuration& config) {
    Renderer::renderer_config = config;
    glslang::InitializeProcess();
    Renderer::Context::init();
    Renderer::manager = &Renderer::Context::context();
    Renderer::manager->initialize();

    interface_ = std::make_shared<Renderer::Interface>("engine/assets");
}

void RenderSystem::createRenderer() {
    renderer_ = std::make_unique<Renderers>();
}

void RenderSystem::render() {
    renderer_->render();
}

void RenderSystem::destroyRenderer() {
    renderer_.reset();
}

RenderSystem::~RenderSystem() {
    interface_.reset();

    Renderer::manager->destroy();
    Renderer::manager = nullptr;
    delete Renderer::manager;
    Renderer::Context::quit();
    glslang::FinalizeProcess();
}

}  // namespace wen