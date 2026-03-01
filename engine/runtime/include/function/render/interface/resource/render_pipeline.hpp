#pragma once

#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/resource/vertex_input.hpp"
#include "function/render/interface/resource/descriptor_set.hpp"
#include "function/render/interface/resource/push_constants.hpp"
#include <glslang/Public/ShaderLang.h>

namespace wen::Renderer {

class ShaderIncluder final : public glslang::TShader::Includer {
public:
    ShaderIncluder(const std::filesystem::path& filename) {
        filepath_ = filename.parent_path();
    }

    IncludeResult* includeLocal(const char* header_name, const char* includer_name, size_t inclusion_depth) {
        auto data = readFile((filepath_ / std::string(header_name)).string());
        auto* contents = new std::string(data.data(), data.size());
        return new IncludeResult(header_name, contents->c_str(), contents->length(), static_cast<void*>(contents));
    }

    void releaseInclude(IncludeResult* result) {
        if (result) {
            delete static_cast<std::string*>(result->userData);
            delete result;
        }
    }

    ~ShaderIncluder() {}

private:
    std::filesystem::path filepath_;
};

class Shader final {
public:
    Shader(const std::string& filename, ShaderStage stage);
    ~Shader();

    ShaderStage stage;
    std::optional<vk::ShaderModule> module;
};

class GraphicsShaderProgram {
    friend class GraphicsRenderPipeline;

public:
    GraphicsShaderProgram() = default;
    ~GraphicsShaderProgram();

    GraphicsShaderProgram& attach(const std::shared_ptr<Shader>& shader);

private:
    std::shared_ptr<Shader> vert_shader_;
    std::shared_ptr<Shader> frag_shader_;
};

class RayTracingShaderProgram {
    friend class RayTracingRenderPipeline;

public:
    struct HitGroup {
        std::shared_ptr<Shader> closest_hit_shader;
        std::optional<std::shared_ptr<Shader>> intersection_shader;
    };

    RayTracingShaderProgram() = default;
    ~RayTracingShaderProgram();

    void setRaygenShader(const std::shared_ptr<Shader>& shader);
    void setMissShader(const std::shared_ptr<Shader>& shader);
    void setHitGroup(const HitGroup& hit_group);

private:
    std::shared_ptr<Shader> raygen_shader_;
    std::vector<std::shared_ptr<Shader>> miss_shaders_;
    std::vector<HitGroup> hit_groups_;
    uint32_t hit_shader_count_ = 0;
};

class ComputeShaderProgram {
    friend class ComputeRenderPipeline;

public:
    ComputeShaderProgram() = default;
    ~ComputeShaderProgram();

    void setComputeShader(const std::shared_ptr<Shader>& shader);

private:
    std::shared_ptr<Shader> compute_shader_;
};

}  // namespace wen::Renderer

namespace wen::Renderer {

class RenderPipeline {
public:
    RenderPipeline() = default;
    virtual ~RenderPipeline();

protected:
    vk::PipelineShaderStageCreateInfo createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module);
    void createPipelineLayout();

public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    std::vector<std::optional<std::shared_ptr<DescriptorSet>>> descriptor_sets;
    std::optional<std::shared_ptr<PushConstants>> push_constants;
};

template <class RenderPipelineClass, typename Options>
class RenderPipelineTemplate : public RenderPipeline {
public:
    ~RenderPipelineTemplate() override = default;

    void setDescriptorSet(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t index = 0) {
        if (index + 1 > descriptor_sets.size()) {
            descriptor_sets.resize(index + 1);
        }
        descriptor_sets[index] = std::move(descriptor_set);
    }

    void setPushConstants(std::shared_ptr<PushConstants> push_constants) {
        this->push_constants = std::move(push_constants);
    }

    virtual void compile(const Options& options) = 0;
};

struct GraphicsRenderPipelineOptions {
    vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
    float line_width = 1.0f;
    vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eNone;
    vk::FrontFace front_face = vk::FrontFace::eClockwise;
    vk::Bool32 depth_test_enable = true;
    vk::Bool32 depth_write_enable = true;
    vk::CompareOp depth_compare_op = vk::CompareOp::eLess;
    std::vector<vk::DynamicState> dynamic_states = {};
};

class Renderer;
class GraphicsRenderPipeline : public RenderPipelineTemplate<GraphicsRenderPipeline, GraphicsRenderPipelineOptions> {
public:
    GraphicsRenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<GraphicsShaderProgram>& shader_program, const std::string& subpass_name);
    ~GraphicsRenderPipeline() override;

    void setVertexInput(std::shared_ptr<VertexInput> vertex_input);
    void compile(const GraphicsRenderPipelineOptions& options) override;

    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;

private:
    std::weak_ptr<Renderer> renderer_;
    std::shared_ptr<GraphicsShaderProgram> shader_program_;
    std::string subpass_name_;
    std::shared_ptr<VertexInput> vertex_input_;
};

struct RayTracingRenderPipelineOptions {
    uint32_t max_ray_recursion_depth = 1;
};

class RayTracingRenderPipeline : public RenderPipelineTemplate<RayTracingRenderPipeline, RayTracingRenderPipelineOptions> {
    friend class Renderer;

public:
    RayTracingRenderPipeline(const std::shared_ptr<RayTracingShaderProgram>& shader_program);
    ~RayTracingRenderPipeline() override;

    void compile(const RayTracingRenderPipelineOptions& options) override;

    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eRayTracingKHR;

private:
    std::shared_ptr<RayTracingShaderProgram> shader_program_;
    std::unique_ptr<Buffer> buffer_;
    vk::StridedDeviceAddressRegionKHR raygen_region_{};
    vk::StridedDeviceAddressRegionKHR miss_region_{};
    vk::StridedDeviceAddressRegionKHR hit_region_{};
    vk::StridedDeviceAddressRegionKHR callable_region_{};
};

struct ComputeRenderPipelineOptions {};

class ComputeRenderPipeline : public RenderPipelineTemplate<ComputeRenderPipeline, ComputeRenderPipelineOptions> {
public:
    ComputeRenderPipeline();
    ~ComputeRenderPipeline() override;

    void compile(const ComputeRenderPipelineOptions& options) override;

    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eCompute;

private:
    std::shared_ptr<ComputeShaderProgram> shader_program_;
};

}  // namespace wen::Renderer