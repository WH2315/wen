#include "function/render/render_framework/render_framework.hpp"
#include "function/render/render_framework/pass/culling_pass.hpp"
#include "function/render/render_framework/pass/visibility_pass.hpp"
#include "function/render/render_framework/pass/mesh_pass.hpp"
#include "function/render/render_framework/pass/outlining_pass.hpp"
#include "engine/global_context.hpp"
#include <backends/imgui_impl_vulkan.h>

namespace wen {

RenderFramework::RenderFramework(bool enable_editor, const std::function<void()>& ui_callback) : ui_callback_(ui_callback) {
    auto& render_system = global_context->render_system;
    auto render_pass = render_system->getInterface()->createRenderPass();
    render_pass->addAttachment(Renderer::SWAPCHAIN_IMAGE_ATTACHMENT, Renderer::AttachmentType::eColor);
    if (enable_editor) {
        render_pass->addAttachment("viewport_color", Renderer::AttachmentType::eRGBA32Sfloat);
        render_system->output_attachment_name = "viewport_color";
    } else {
        render_system->output_attachment_name = Renderer::SWAPCHAIN_IMAGE_ATTACHMENT;
    }

    resource_ = std::make_unique<Resource>();

    subpasses_.push_back(std::make_unique<CullingPass>());
    subpasses_.push_back(std::make_unique<VisibilityPass>());
    subpasses_.push_back(std::make_unique<MeshPass>());
    subpasses_.push_back(std::make_unique<OutliningPass>());

    for (auto& subpass : subpasses_) {
        subpass->addAttachment(*render_pass);
    }

    for (auto& subpass : subpasses_) {
        if (!subpass->isOnlyCompute()) {
            subpass->setAttachment(render_pass->addSubpass(subpass->getName()));
        }
    }

    for (auto& subpass : subpasses_) {
        subpass->setSubpassDependency(*render_pass);
    }

    if (enable_editor) {
        // Ensure the scene writes to viewport_color are available and visible
        // to the subsequent ImGui pass's fragment-shader sampling. The render
        // pass transitions viewport_color to eShaderReadOnlyOptimal on
        // completion. The outlining pass is the last subpass writing it.
        vk::SubpassDependency dependency;
        dependency.setSrcSubpass(render_pass->getSubpassIndex("outlining_pass"))
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        render_pass->final_dependencies.push_back(dependency);
    }

    render_pass->build();

    renderer_ = render_system->getInterface()->createRenderer(std::move(render_pass));

    // Propagate swapchain recreation (window resize) to render-system-level
    // subscribers (e.g. cameras recomputing their aspect ratio).
    renderer_->registerResourceRecreateCallback([]() {
        global_context->render_system->notifyResize();
    });

    for (auto& subpass : subpasses_) {
        subpass->createRenderResource(renderer_, *resource_);
    }

    if (enable_editor) {
        // The picker reads resource_.instance_datas_buffer, created by the
        // culling pass in createRenderResource above.
        picker_ = std::make_unique<GameObjectPicker>(renderer_, *resource_);

        imgui_pass_ = std::make_unique<Renderer::ImguiPass>(*renderer_);
        viewport_sampler_ = render_system->getInterface()->createSampler();
        createViewportTexture();
        renderer_->registerResourceRecreateCallback([this]() {
            createViewportTexture();
        });
    }
}

RenderFramework::~RenderFramework() {
    if (viewport_texture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(viewport_texture_);
        viewport_texture_ = VK_NULL_HANDLE;
    }
    viewport_sampler_.reset();
    imgui_pass_.reset();
    picker_.reset();
    renderer_.reset();
    subpasses_.clear();
    resource_.reset();
}

void RenderFramework::createViewportTexture() {
    if (viewport_texture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(viewport_texture_);
        viewport_texture_ = VK_NULL_HANDLE;
    }
    auto idx = renderer_->render_pass->getAttachmentIndex("viewport_color", true);
    viewport_texture_ = ImGui_ImplVulkan_AddTexture(
        viewport_sampler_->sampler,
        renderer_->framebuffer_set->attachments[idx]->image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void RenderFramework::render() {
    renderer_->acquireNextImage();
    // Snapshot the camera matrices for this in-flight frame (report calls
    // during logic only touch CPU staging, so a frame is always consistent).
    global_context->camera_system->uploadFrameData(renderer_->getCurrentFrame());
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

    // Editor: composite ImGui (including the Viewport panel that samples the
    // offscreen scene) into the swapchain in its own render pass.
    if (imgui_pass_) {
        imgui_pass_->render(*renderer_, ui_callback_);
    }

    for (auto& subpass : subpasses_) {
        subpass->executePostRenderPass(renderer_, *resource_);
    }
    renderer_->present();

    // Pick tasks read this frame's visibility buffer; wait for the GPU to
    // finish before dispatching the readback compute (editor-only, on click).
    if (picker_ && picker_->needProcess()) {
        renderer_->waitIdle();
        picker_->processPickTasks(renderer_);
    }
}

void RenderFramework::pickGameObject(uint32_t x, uint32_t y,
                                     const std::function<void(GameObjectUUID uuid)>& picked_callback,
                                     const std::function<void()>& miss_callback) {
    if (picker_ == nullptr) {
        return;
    }
    picker_->addPickTask({x, y, picked_callback, miss_callback});
}

}  // namespace wen
