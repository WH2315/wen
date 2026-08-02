#pragma once

#include "function/render/render_framework/subpass.hpp"

namespace wen {

// Draws an outline around the selected mesh instance (editor selection).
// Two draws of the same instance, Match-engine style:
//   1. stencil draw (empty fragment shader) marks the silhouette with 1
//   2. thick-wireframe draw passes only where stencil != 1, leaving the
//      outer contour
// The instance data + indirect command are produced on the GPU by
// compact_instance.comp into resource.outlining_buffer. Uses a dedicated
// stencil attachment so the HZB depth attachment stays stencil-free.
class OutliningPass : public Subpass {
public:
    OutliningPass() : Subpass("outlining_pass", false) {}

    void addAttachment(Renderer::RenderPass& render_pass) override;
    void setAttachment(Renderer::RenderSubpass& render_subpass) override;
    void setSubpassDependency(Renderer::RenderPass& render_pass) override;
    void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;
    void executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;

private:
    std::shared_ptr<Renderer::DescriptorSet> descriptor_set_;
    std::shared_ptr<Renderer::GraphicsShaderProgram> stencil_shader_program_;
    std::shared_ptr<Renderer::GraphicsRenderPipeline> stencil_render_pipeline_;
    std::shared_ptr<Renderer::GraphicsShaderProgram> outlining_shader_program_;
    std::shared_ptr<Renderer::GraphicsRenderPipeline> outlining_render_pipeline_;
};

}  // namespace wen
