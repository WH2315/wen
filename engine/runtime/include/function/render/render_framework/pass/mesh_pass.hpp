#pragma once

#include "function/render/render_framework/subpass.hpp"

namespace wen {

class MeshPass : public Subpass {
public:
    MeshPass() : Subpass("mesh_pass", false) {}

    void setAttachment(Renderer::RenderSubpass& render_subpass);
    void setSubpassDependency(Renderer::RenderPass& render_pass);
    void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource);
    void executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource);

private:
    std::shared_ptr<Renderer::DescriptorSet> descriptor_set_;
    std::shared_ptr<Renderer::GraphicsShaderProgram> mesh_shader_program_;
    std::shared_ptr<Renderer::GraphicsRenderPipeline> mesh_pipeline_pipeline_;
};

}  // namespace wen