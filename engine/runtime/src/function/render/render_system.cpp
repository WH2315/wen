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
    render_data_ = std::make_unique<RenderData>();
    render_framework_ = std::make_unique<RenderFramework>();
}

void RenderSystem::waitIdle() {
    Renderer::manager->device->device.waitIdle();
}

void RenderSystem::render() {
    render_framework_->render();
}

void RenderSystem::enableEditor(const std::function<void()>& ui_callback) {
    render_framework_.reset();
    render_framework_ = std::make_unique<RenderFramework>(true, ui_callback);
}

void RenderSystem::destroyRenderer() {
    render_framework_.reset();
    render_data_.reset();
}

RenderSystem::~RenderSystem() {
    interface_.reset();

    Renderer::manager->destroy();
    Renderer::manager = nullptr;
    Renderer::Context::quit();
    glslang::FinalizeProcess();
}

}  // namespace wen