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
    std::shared_ptr<Renderer::Sampler> visibility_sampler_;
    std::shared_ptr<Renderer::GraphicsShaderProgram> mesh_shader_program_;
    std::shared_ptr<Renderer::GraphicsRenderPipeline> mesh_pipeline_pipeline_;
    std::shared_ptr<Renderer::PushConstants> push_constants_;  // LOD 调试开关等

    uint32_t last_bound_texture_count_ = 0;  // 最近一次绑定时的纹理池大小
    uint32_t last_bound_normal_texture_count_ = 0;  // 法线贴图池大小
    uint32_t last_bound_mr_texture_count_ = 0;      // metallic-roughness 池大小
    uint32_t last_bound_ao_texture_count_ = 0;      // AO 池大小
};

}  // namespace wen