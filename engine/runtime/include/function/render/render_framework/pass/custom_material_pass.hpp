#pragma once

#include "function/render/render_framework/subpass.hpp"
#include "function/asset/material_asset.hpp"
#include <map>
#include <filesystem>

namespace wen {

// 自定义着色器材质 forward 通道:渲染挂了 CustomShaderComponent(且带
// MeshComponent + TransformComponent)的对象。按 .mat 资产逐材质缓存
// pipeline(自带着色器 + 参数 SSBO + 相机),逐对象以世界矩阵 push 绘制。
// 共享深度测试;.mat 文件被编辑器改后,下一帧自动重刷参数。
class CustomMaterialPass : public Subpass {
public:
    CustomMaterialPass() : Subpass("custom_material_pass", false) {}

    void addAttachment(Renderer::RenderPass& render_pass) override {}
    void setAttachment(Renderer::RenderSubpass& render_subpass) override;
    void setSubpassDependency(Renderer::RenderPass& render_pass) override;
    void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;
    void executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) override;

private:
    struct MaterialResource {
        std::string shader;
        bool wireframe = false;
        std::string cull = "none";
        std::string full_path;
        std::filesystem::file_time_type last_mtime;
        std::shared_ptr<Renderer::StorageBuffer> params_buffer;
        std::shared_ptr<Renderer::DescriptorSet> descriptor_set;
        std::shared_ptr<Renderer::GraphicsRenderPipeline> pipeline;
    };
    static void fillParams(Renderer::StorageBuffer* buffer, const CustomMaterialAsset& asset);
    void buildPipeline(std::shared_ptr<Renderer::Renderer> renderer, MaterialResource* res, const CustomMaterialAsset& asset);
    std::shared_ptr<MaterialResource> getOrCreateMaterial(std::shared_ptr<Renderer::Renderer> renderer, const std::string& path);

    std::map<std::string, std::shared_ptr<MaterialResource>> materials_;
    std::shared_ptr<Renderer::VertexInput> vertex_input_;
    std::shared_ptr<Renderer::PushConstants> push_constants_;
};

}  // namespace wen