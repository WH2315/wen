#include "function/render/interface/interface.hpp"

namespace wen::Renderer {

Interface::Interface(const std::string& path) : path_(path) {
    shader_dir_ = path_ + "/shaders";
    texture_dir_ = path_ + "/textures";
    model_dir_ = path_ + "/models";
    gltf_dir_ = path_ + "/gltf";
}

std::shared_ptr<RenderPass> Interface::createRenderPass(bool auto_load) {
    return std::make_shared<RenderPass>(auto_load);
}

std::shared_ptr<Renderer> Interface::createRenderer(std::shared_ptr<RenderPass> render_pass) {
    return std::make_shared<Renderer>(render_pass);
}

std::shared_ptr<Shader> Interface::loadShader(const std::string& filename, ShaderStage stage) {
    return std::make_shared<Shader>(shader_dir_ + "/" + filename, stage);
}

std::shared_ptr<GraphicsShaderProgram> Interface::createGraphicsShaderProgram() {
    return std::make_shared<GraphicsShaderProgram>();
}

std::shared_ptr<GraphicsRenderPipeline> Interface::createGraphicsRenderPipeline(std::weak_ptr<Renderer> renderer, std::shared_ptr<GraphicsShaderProgram> shader_program, const std::string& subpass_name) {
    return std::make_shared<GraphicsRenderPipeline>(renderer, shader_program, subpass_name);
}

std::shared_ptr<VertexInput> Interface::createVertexInput(const std::vector<VertexInputInfo>& infos) {
    return std::make_shared<VertexInput>(infos);
}

std::shared_ptr<VertexBuffer> Interface::createVertexBuffer(uint32_t size, uint32_t count) {
    return std::make_shared<VertexBuffer>(size, count);
}

std::shared_ptr<IndexBuffer> Interface::createIndexBuffer(IndexType type, uint32_t count) {
    return std::make_shared<IndexBuffer>(type, count);
}

std::shared_ptr<DescriptorSet> Interface::createDescriptorSet() {
    return std::make_shared<DescriptorSet>();
}

std::shared_ptr<UniformBuffer> Interface::createUniformBuffer(uint64_t size) {
    return std::make_shared<UniformBuffer>(size);
}

std::shared_ptr<DataTexture> Interface::createTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels) {
    return std::make_shared<DataTexture>(data, width, height, mip_levels);
}

std::shared_ptr<SpecificTexture> Interface::createTexture(const std::string& filename, uint32_t mip_levels) {
    auto pos = filename.find_last_of('.') + 1;
    auto filetype = filename.substr(pos, filename.size() - pos);
    std::string filepath = texture_dir_ + "/" + filename;
    if (filetype == "png" || filetype == "jpg") {
        return std::make_shared<ImageTexture>(filepath, mip_levels);
    } else if (filetype == "ktx") {
        //
    } else {
        WEN_CORE_ERROR("Unsupported texture format: {}", filetype)
    }
    return nullptr;
}

std::shared_ptr<Sampler> Interface::createSampler(const SamplerOptions& options) {
    return std::make_shared<Sampler>(options);
}

std::shared_ptr<PushConstants> Interface::createPushConstants(ShaderStages stages, const std::list<std::pair<std::string, ConstantType>>& infos) {
    return std::make_shared<PushConstants>(stages, infos);
}

std::shared_ptr<NormalModel> Interface::loadNormalModel(const std::string& filename, const std::vector<std::string>& blacklist) {
    return std::make_shared<NormalModel>(model_dir_ + "/" + filename, blacklist);
}

std::shared_ptr<StorageImage> Interface::createStorageImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage) {
    return std::make_shared<StorageImage>(width, height, format, usage);
}

std::shared_ptr<RayTracingShaderProgram> Interface::createRayTracingShaderProgram() {
    return std::make_shared<RayTracingShaderProgram>();
}

std::shared_ptr<RayTracingRenderPipeline> Interface::createRayTracingRenderPipeline(std::shared_ptr<RayTracingShaderProgram> shader_program) {
    return std::make_shared<RayTracingRenderPipeline>(shader_program);
}

std::shared_ptr<AccelerationStructure> Interface::createAccelerationStructure() {
    return std::make_shared<AccelerationStructure>();
}

std::shared_ptr<RayTracingInstance> Interface::createRayTracingInstance() {
    return std::make_shared<RayTracingInstance>();
}

std::shared_ptr<GLTFScene> Interface::loadGLTFScene(const std::string& filename, const std::vector<std::string>& attrs) {
    return std::make_shared<GLTFScene>(gltf_dir_ + "/" + filename, attrs);
}

std::shared_ptr<SphereModel> Interface::createSphereModel() {
    return std::make_shared<SphereModel>();
}

}  // namespace wen::Renderer