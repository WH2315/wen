#pragma once

#include "function/render/interface/basic/enums.hpp"
#include "function/render/interface/resource/buffer.hpp"
#include "function/render/interface/resource/image.hpp"
#include "function/render/interface/resource/ray_tracing.hpp"

namespace wen::Renderer {

class Renderer;
class RenderPipeline;

struct DescriptorSetLayoutBinding {
    DescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType descriptor_type, ShaderStages stage)
        : binding(binding), descriptor_type(descriptor_type), descriptor_count(1), stage(stage), samples(nullptr) {}
    
    DescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType descriptor_type, uint32_t count, ShaderStages stage)
        : binding(binding), descriptor_type(descriptor_type), descriptor_count(count), stage(stage), samples(nullptr) {}

    uint32_t binding;
    vk::DescriptorType descriptor_type;
    uint32_t descriptor_count;
    ShaderStages stage;
    const vk::Sampler* samples;
};

class DescriptorSet {
    friend class RenderPipeline; // descriptor_layout_
    friend class Renderer; // descriptor_sets_

public:
    DescriptorSet() = default;
    ~DescriptorSet();

    DescriptorSet& addDescriptors(const std::vector<DescriptorSetLayoutBinding>& bindings);
    void build();

    const vk::DescriptorSetLayoutBinding& getBinding(uint32_t binding);

    void bindUniforms(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>>& uniform_buffers);
    void bindUniform(uint32_t binding, std::shared_ptr<UniformBuffer> uniform_buffer);
    void bindTextures(uint32_t binding, const std::vector<std::pair<std::shared_ptr<SpecificTexture>, std::shared_ptr<Sampler>>>& textures_samplers);
    void bindTexture(uint32_t binding, std::shared_ptr<SpecificTexture> texture, std::shared_ptr<Sampler> sampler);
    void bindInputAttachments(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::vector<std::pair<std::string, std::shared_ptr<Sampler>>>& names_samplers);
    void bindInputAttachment(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::string& name, std::shared_ptr<Sampler> sampler);
    void bindStorageBuffers(uint32_t binding, const std::vector<std::shared_ptr<StorageBuffer>>& storage_buffers);
    void bindStorageBuffer(uint32_t binding, std::shared_ptr<StorageBuffer> storage_buffer);
    void bindStorageImages(uint32_t binding, const std::vector<std::shared_ptr<StorageImage>>& storage_images);
    void bindStorageImage(uint32_t binding, std::shared_ptr<StorageImage> storage_image);
    void bindAccelerationStructures(uint32_t binding, const std::vector<std::shared_ptr<RayTracingInstance>>& instances);
    void bindAccelerationStructure(uint32_t binding, std::shared_ptr<RayTracingInstance> instance);

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings_;
    vk::DescriptorSetLayout descriptor_layout_;
    std::vector<vk::DescriptorSet> descriptor_sets_;
};

}  // namespace wen::Renderer