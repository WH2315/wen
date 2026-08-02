#pragma once

#include "core/base/singleton.hpp"
#include "function/render/render_framework/render_framework.hpp"
#include "function/render/render_data.hpp"

namespace wen {

class RenderSystem final {
    friend class Singleton<RenderSystem>;
    RenderSystem(const Renderer::Configuration& config);
    ~RenderSystem();

public:
    void createRenderer();
    void render();
    void destroyRenderer();

    void enableEditor(const std::function<void()>& ui_callback);

    // Editor: ImGui texture id for the offscreen scene (Viewport panel image).
    VkDescriptorSet getViewportTexture() const { return render_framework_->viewport_texture_; }

    // Editor: pick the game object at pixel (x, y) of the offscreen scene.
    void pickGameObject(uint32_t x, uint32_t y,
                        const std::function<void(GameObjectUUID uuid)>& picked_callback,
                        const std::function<void()>& miss_callback = nullptr) {
        render_framework_->pickGameObject(x, y, picked_callback, miss_callback);
    }

    // Editor: outline the selected game object (GameObjectUUID(-1) = none).
    // No-op mapping when the object has no mesh instance.
    void setSelectedGameObject(GameObjectUUID uuid) {
        auto& map = render_data_->getMeshInstancePool()->game_object_uuid_to_mesh_instance_index_map;
        auto iter = map.find(uuid);
        render_framework_->resource_->selected_mesh_instance_index =
            (iter != map.end()) ? iter->second : uint32_t(-1);
    }

    uint32_t getMaxMeshInstanceCount() const { return 16384; }

    // Swapchain-resize notifications. Callbacks fire after the swapchain and
    // framebuffers have been recreated (renderer_config already has the new
    // size). Used e.g. by camera components to recompute their aspect ratio.
    uint32_t registerResizeCallback(const std::function<void()>& callback) {
        auto id = next_resize_callback_id_++;
        resize_callbacks_.insert({id, callback});
        return id;
    }
    void unregisterResizeCallback(uint32_t id) { resize_callbacks_.erase(id); }
    void notifyResize() {
        for (const auto& [id, callback] : resize_callbacks_) {
            callback();
        }
    }

    // Aspect ratio of the surface the scene is actually displayed on. By
    // default that is the swapchain (window); the editor overrides it with the
    // Viewport panel's aspect, since the offscreen image is stretched into the
    // panel. Cameras derive their projection aspect from this.
    void setViewportAspectOverride(float aspect) {
        if (viewport_aspect_override_ != aspect) {
            viewport_aspect_override_ = aspect;
            notifyResize();
        }
    }
    float getOutputAspect() const {
        if (viewport_aspect_override_ > 0.0f) {
            return viewport_aspect_override_;
        }
        auto& config = Renderer::renderer_config;
        return (config.swapchain_image_height > 0)
            ? static_cast<float>(config.swapchain_image_width) / static_cast<float>(config.swapchain_image_height)
            : 16.0f / 9.0f;
    }

    std::string output_attachment_name;
    auto getRendererConfig() { return Renderer::renderer_config; }
    auto getAPIManager() { return Renderer::manager; }
    auto getInterface() { return interface_.get(); }
    auto getRenderFramework() { return render_framework_.get(); }
    auto getRenderData() { return render_data_.get(); }

private:
    std::shared_ptr<Renderer::Interface> interface_;
    std::unique_ptr<RenderFramework> render_framework_;
    std::unique_ptr<RenderData> render_data_;

    std::map<uint32_t, std::function<void()>> resize_callbacks_;
    uint32_t next_resize_callback_id_ = 0;
    float viewport_aspect_override_ = 0.0f;
};

}  // namespace wen