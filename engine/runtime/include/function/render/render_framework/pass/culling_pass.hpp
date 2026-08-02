#pragma once

#include "function/render/render_framework/subpass.hpp"

namespace wen {

class CullingPass : public Subpass {
public:
    CullingPass() : Subpass("culling_pass", true) {}

    void addAttachment(Renderer::RenderPass& render_pass) override;
    void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;
    void executePreRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;

private:
    // Descriptor array sizes are baked into the pipeline layout, so allocate
    // for the largest window we ever expect (2^16 px) and pad unused slots.
    static constexpr uint32_t kMaxDepthMipLevels = 16;

    // (Re)creates everything that depends on the swapchain size: transitions
    // the depth attachment, rebuilds the per-in-flight HZB pyramids, rebinds
    // their descriptors and updates the depth_texture_size push constant.
    // Called at creation and from the resource-recreate (resize) callback.
    void recreateDepthResources(Renderer::Renderer* renderer, Resource& resource);
    std::shared_ptr<Renderer::DescriptorSet> depth_descriptor_set_;
    std::shared_ptr<Renderer::PushConstants> depth_constants_;

    std::shared_ptr<Renderer::ComputeShaderProgram> copy_depth_texture_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> copy_depth_texture_render_pipeline_;
    std::shared_ptr<Renderer::ComputeShaderProgram> generate_depth_texture_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> generate_depth_texture_render_pipeline_;

    std::shared_ptr<Renderer::DescriptorSet> readonly_descriptor_set_;
    std::shared_ptr<Renderer::DescriptorSet> descriptor_set_;
    std::shared_ptr<Renderer::PushConstants> constants_;

    std::shared_ptr<Renderer::ComputeShaderProgram> pre_culling_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> pre_culling_render_pipeline_;
    std::shared_ptr<Renderer::ComputeShaderProgram> reset_indirect_command_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> reset_indirect_command_render_pipeline_;
    std::shared_ptr<Renderer::ComputeShaderProgram> compact_instance_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> compact_instance_render_pipeline_;
    std::shared_ptr<Renderer::ComputeShaderProgram> generate_available_indirect_command_shader_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> generate_available_indirect_command_render_pipeline_;
};

}  // namespace wen