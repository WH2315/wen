#pragma once

#include "core/base/singleton.hpp"
#include "function/render/render_framework/renderer.hpp"

namespace wen {

class RenderSystem final {
    friend class Singleton<RenderSystem>;
    RenderSystem(const Renderer::Configuration& config);
    ~RenderSystem();

public:
    void createRenderer();
    void render();
    void destroyRenderer();

    std::string output_attachment_name;
    auto getAPIManager() { return Renderer::manager; }
    auto getInterface() { return interface_.get(); }

private:
    std::shared_ptr<Renderer::Interface> interface_;
    std::unique_ptr<Renderers> renderer_;
};

}  // namespace wen