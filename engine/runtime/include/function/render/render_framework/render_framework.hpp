#pragma once

#include "function/render/render_framework/subpass.hpp"

namespace wen {

class RenderFramework {
    friend class RenderSystem;

public:
    RenderFramework();
    ~RenderFramework();

    void render();

private:
    std::unique_ptr<Resource> resource_;
    std::shared_ptr<Renderer::Renderer> renderer_;
    std::vector<std::unique_ptr<Subpass>> subpasses_;
};

}  // namespace wen