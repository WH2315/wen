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

    uint32_t getMaxMeshInstanceCount() const { return 16384; }

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
};

}  // namespace wen