#pragma once

#include "core/base/singleton.hpp"
#include "function/render/render_framework/renderer.hpp"
#include "function/render/swap_data.hpp"

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
    auto getAPIManager() { return Renderer::manager; }
    auto getInterface() { return interface_.get(); }
    auto getRenderer() { return renderer_.get(); }
    auto getSwapData() { return swap_data_.get(); }

private:
    std::shared_ptr<Renderer::Interface> interface_;
    std::unique_ptr<Renderers> renderer_;
    std::unique_ptr<SwapData> swap_data_;
};

}  // namespace wen