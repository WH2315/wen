#pragma once

#include "function/render/render_framework/subpass.hpp"

namespace wen {

class VisibilityPass : public Subpass {
public:
    VisibilityPass() : Subpass("visibility_pass", false) {}

    void addAttachment(Renderer::RenderPass& render_pass);
    void setAttachment(Renderer::RenderSubpass& render_subpass);
    void setSubpassDependency(Renderer::RenderPass& render_pass);
    void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource);
    void executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource);

private:
    std::shared_ptr<Renderer::DescriptorSet> descriptor_set_;
    std::shared_ptr<Renderer::GraphicsShaderProgram> visibility_shader_program_;
    std::shared_ptr<Renderer::GraphicsRenderPipeline> visibility_pipeline_pipeline_;
};

}  // namespace wen