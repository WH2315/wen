#pragma once

#include "function/render/interface/renderer.hpp"
#include <imgui.h>

namespace wen::Renderer {

class Imgui {
public:
    Imgui(Renderer& renderer);
    ~Imgui();

    // Start a new ImGui frame (no subpass switch yet).
    void newFrame();
    // Render current ImGui draw data; switches to imgui_subpass internally.
    void renderFrame();

private:
    Renderer& renderer_;
    vk::DescriptorPool descriptor_pool_;
};

}  // namespace wen::Renderer