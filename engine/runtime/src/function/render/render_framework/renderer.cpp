#include "function/render/render_framework/renderer.hpp"
#include "engine/global_context.hpp"

namespace wen {

Renderers::Renderers() {
    auto& render_system = global_context->render_system;
    auto render_pass = render_system->getInterface()->createRenderPass(false);
    render_pass->addAttachment(Renderer::SWAPCHAIN_IMAGE_ATTACHMENT, Renderer::AttachmentType::eColor);
    render_system->output_attachment_name = Renderer::SWAPCHAIN_IMAGE_ATTACHMENT;

    resource_ = std::make_unique<Resource>();

    // subpasses_.push_back(std::make_unique<>());
    // subpasses_.push_back(std::make_unique<>());
    // subpasses_.push_back(std::make_unique<>());

    for (auto& subpass : subpasses_) {
        subpass->addAttachment(*render_pass);
    }

    for (auto& subpass : subpasses_) {
        if (!subpass->isOnlyCompute()) {
            subpass->setAttachment(render_pass->addSubpass(subpass->getName()));
        }
    }

    render_pass->build();

    renderer_ = render_system->getInterface()->createRenderer(std::move(render_pass));

    for (auto& subpass : subpasses_) {
        subpass->createRenderResource(renderer_, *resource_);
    }
}

Renderers::~Renderers() {
    renderer_->waitIdle();
    renderer_.reset();
    subpasses_.clear();
    auto manager = global_context->render_system->getAPIManager();

    resource_.reset();
}

void Renderers::render() {
    renderer_->acquireNextImage();
    for (auto& subpass : subpasses_) {
        subpass->executePreRenderPass(renderer_, *resource_);
    }
    renderer_->beginRenderPass();
    for (auto& subpass : subpasses_) {
        if (subpass->isOnlyCompute()) {
            continue;
        }
        renderer_->nextSubpass(subpass->getName());
        subpass->executeRenderPass(renderer_, *resource_);
    }
    renderer_->endRenderPass();
    for (auto& subpass : subpasses_) {
        subpass->executePostRenderPass(renderer_, *resource_);
    }
    renderer_->present();
}

}  // namespace wen