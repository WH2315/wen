#pragma once

#include "function/render/render_framework/subpass.hpp"
#include "function/render/render_framework/game_object_picker.hpp"
#include "function/render/interface/imgui_pass.hpp"

namespace wen {

class RenderFramework {
    friend class RenderSystem;

public:
    RenderFramework(bool enable_editor = false, const std::function<void()>& ui_callback = nullptr);
    ~RenderFramework();

    auto getResource() const { return resource_.get(); }

    void render();

    // Editor: pick the game object under pixel (x, y) of the offscreen scene.
    // Callbacks fire after the current frame's rendering completes.
    void pickGameObject(uint32_t x, uint32_t y,
                        const std::function<void(GameObjectUUID uuid)>& picked_callback,
                        const std::function<void()>& miss_callback = nullptr);

private:
    void createViewportTexture();

    std::unique_ptr<Resource> resource_;
    std::shared_ptr<Renderer::Renderer> renderer_;
    std::vector<std::unique_ptr<Subpass>> subpasses_;

    std::unique_ptr<GameObjectPicker> picker_;

    std::unique_ptr<Renderer::ImguiPass> imgui_pass_;
    std::function<void()> ui_callback_;

    std::shared_ptr<Renderer::Sampler> viewport_sampler_;
    VkDescriptorSet viewport_texture_ = VK_NULL_HANDLE;
};

}  // namespace wen