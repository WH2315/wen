#pragma once

#include "function/render/interface/renderer.hpp"
#include <imgui.h>
#include <functional>

namespace wen::Renderer {

// ImGui integration that owns a dedicated render pass targeting the swapchain.
// Unlike Renderer::Imgui (which adds an imgui subpass to an existing render
// pass), this draws ImGui in a separate pass so the scene can be rendered to an
// offscreen attachment first and then sampled by ImGui (render-to-texture
// viewport). Renderer::Imgui is left intact for the standalone example.
class ImguiPass {
public:
    ImguiPass(Renderer& renderer);
    ~ImguiPass();

    // New ImGui frame, run the UI callback, then record the imgui render pass
    // into the renderer's current command buffer (swapchain framebuffer).
    void render(Renderer& renderer, const std::function<void()>& ui_callback);

private:
    void createRenderPass();
    void createFramebuffers();
    void destroyFramebuffers();

    Renderer& renderer_;
    vk::RenderPass render_pass_;
    std::vector<vk::Framebuffer> framebuffers_;
    vk::DescriptorPool descriptor_pool_;
    uint32_t recreate_callback_id_;
};

}  // namespace wen::Renderer
