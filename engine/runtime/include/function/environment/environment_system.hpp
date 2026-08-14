#pragma once

#include "core/base/singleton.hpp"
#include "function/render/interface/resource/image.hpp"
#include "function/render/interface/resource/render_pipeline.hpp"
#include "function/render/interface/resource/descriptor_set.hpp"

namespace wen {

// IBL 环境系统:从 HDR 等距柱状图生成并预积分环境贴图(环境立方体 / 辐照度 / 预过滤镜面 / BRDF LUT),
// 供 mesh pass 做天空盒背景 + 基于图像的光照。
// 若 <资源根>/textures/default_env.hdr 不存在,则不加载环境(hasEnvironment()==false):
// mesh pass 绑定 1x1 黑色兜底立方体贴图 —— 天空为黑、IBL 为 0,仅剩直接光照。
class EnvironmentSystem final {
    friend class Singleton<EnvironmentSystem>;
    EnvironmentSystem();
    ~EnvironmentSystem();

public:
    void destroy();

    bool hasEnvironment() const { return env_cubemap_ != nullptr; }

    auto getEnvCubemap() const { return env_cubemap_; }
    auto getIrradianceCubemap() const { return irradiance_cubemap_; }
    auto getPrefilteredCubemap() const { return prefiltered_cubemap_; }
    auto getBrdfLut() const { return brdf_lut_; }
    auto getEnvSampler() const { return env_sampler_; }
    auto getIrradianceSampler() const { return irradiance_sampler_; }
    auto getPrefilteredSampler() const { return prefiltered_sampler_; }
    auto getBrdfSampler() const { return brdf_sampler_; }

    // 无环境时的兜底:1x1 纯黑立方体贴图(天空渲染为黑)。
    auto getFallbackCubemap() const { return fallback_cubemap_; }
    auto getFallbackSampler() const { return fallback_sampler_; }

    static constexpr uint32_t kEnvSize = 512;        // 环境立方体贴图分辨率(每面)
    static constexpr uint32_t kIrradianceSize = 32;  // 辐照度图(漫反射,可小)
    static constexpr uint32_t kPrefilterSize = 256;  // 预过滤镜面图(每面)
    static constexpr uint32_t kPrefilterMipLevels = 5;
    static constexpr uint32_t kBrdfLutSize = 512;

private:
    void buildClearPipeline();
    void buildBrdfPipeline();
    void buildEnvPipelines();
    void generateEnvCube();
    void generateIrradiance();
    void generatePrefiltered();
    void generateBrdfLut();
    void clearCube(std::shared_ptr<Renderer::CubeTexture> cube);
    bool uploadEquirect(const std::string& filename);

    std::shared_ptr<Renderer::CubeTexture> env_cubemap_;
    std::shared_ptr<Renderer::CubeTexture> irradiance_cubemap_;
    std::shared_ptr<Renderer::CubeTexture> prefiltered_cubemap_;
    std::shared_ptr<Renderer::StorageImage> brdf_lut_;
    std::shared_ptr<Renderer::StorageImage> equirect_tex_;
    std::shared_ptr<Renderer::CubeTexture> fallback_cubemap_;

    std::shared_ptr<Renderer::Sampler> env_sampler_;
    std::shared_ptr<Renderer::Sampler> irradiance_sampler_;
    std::shared_ptr<Renderer::Sampler> prefiltered_sampler_;
    std::shared_ptr<Renderer::Sampler> brdf_sampler_;
    std::shared_ptr<Renderer::Sampler> fallback_sampler_;

    // 环境生成(等距柱状 HDR)
    std::shared_ptr<Renderer::DescriptorSet> equirect_descriptor_set_;
    std::shared_ptr<Renderer::ComputeShaderProgram> equirect_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> equirect_pipeline_;

    std::shared_ptr<Renderer::DescriptorSet> irradiance_descriptor_set_;
    std::shared_ptr<Renderer::PushConstants> irradiance_constants_;
    std::shared_ptr<Renderer::ComputeShaderProgram> irradiance_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> irradiance_pipeline_;

    std::shared_ptr<Renderer::DescriptorSet> prefilter_descriptor_set_;
    std::shared_ptr<Renderer::PushConstants> prefilter_constants_;
    std::shared_ptr<Renderer::ComputeShaderProgram> prefilter_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> prefilter_pipeline_;

    std::shared_ptr<Renderer::DescriptorSet> brdf_descriptor_set_;
    std::shared_ptr<Renderer::ComputeShaderProgram> brdf_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> brdf_pipeline_;

    std::shared_ptr<Renderer::DescriptorSet> clear_cubemap_descriptor_set_;
    std::shared_ptr<Renderer::ComputeShaderProgram> clear_cubemap_program_;
    std::shared_ptr<Renderer::ComputeRenderPipeline> clear_cubemap_pipeline_;
};

}  // namespace wen
