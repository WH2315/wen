#include "function/environment/environment_system.hpp"
#include "engine/global_context.hpp"
#include "function/render/interface/context.hpp"
#include "core/base/macro.hpp"
#include <stb_image.h>
#include <filesystem>

namespace wen {

namespace {

// 带完整子资源的图像布局转换(单次使用命令缓冲,transfer 队列提交并等待)。
void transitionImage(vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                     vk::AccessFlags src_access, vk::AccessFlags dst_access,
                     vk::PipelineStageFlags src_stage, vk::PipelineStageFlags dst_stage,
                     uint32_t mip_levels, uint32_t layer_count) {
    auto cmdbuf = Renderer::manager->command_pool->allocateSingleUse();
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setSrcAccessMask(src_access)
        .setDstAccessMask(dst_access)
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, layer_count})
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
    cmdbuf.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);
    Renderer::manager->command_pool->freeSingleUse(cmdbuf);
}

// 执行一次计算 dispatch(单次使用命令缓冲,transfer 队列提交并等待)。
void runCompute(const std::shared_ptr<Renderer::ComputeRenderPipeline>& pipeline,
                const std::shared_ptr<Renderer::PushConstants>& constants,
                uint32_t gx, uint32_t gy, uint32_t gz) {
    auto cmdbuf = Renderer::manager->command_pool->allocateSingleUse();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->pipeline);
    for (auto& descriptor_set : pipeline->descriptor_sets) {
        if (descriptor_set.has_value()) {
            cmdbuf.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute, pipeline->pipeline_layout, 0,
                descriptor_set.value()->getDescriptorSets()[0], {});
        }
    }
    if (constants) {
        cmdbuf.pushConstants(pipeline->pipeline_layout, constants->range.stageFlags, 0,
                             constants->total_size, constants->constants.data());
    }
    cmdbuf.dispatch(gx, gy, gz);
    Renderer::manager->command_pool->freeSingleUse(cmdbuf);
}

}  // namespace

EnvironmentSystem::~EnvironmentSystem() {
    destroy();
}

void EnvironmentSystem::destroy() {
    clear_cubemap_pipeline_.reset();
    clear_cubemap_program_.reset();
    clear_cubemap_descriptor_set_.reset();
    brdf_pipeline_.reset();
    brdf_program_.reset();
    brdf_descriptor_set_.reset();
    prefilter_pipeline_.reset();
    prefilter_program_.reset();
    prefilter_descriptor_set_.reset();
    irradiance_pipeline_.reset();
    irradiance_program_.reset();
    irradiance_descriptor_set_.reset();
    equirect_pipeline_.reset();
    equirect_program_.reset();
    equirect_descriptor_set_.reset();

    fallback_sampler_.reset();
    brdf_sampler_.reset();
    prefiltered_sampler_.reset();
    irradiance_sampler_.reset();
    env_sampler_.reset();
    fallback_cubemap_.reset();
    equirect_tex_.reset();
    brdf_lut_.reset();
    prefiltered_cubemap_.reset();
    irradiance_cubemap_.reset();
    env_cubemap_.reset();
}

// setup 放在构造里(GlobalContext 调 Singleton::initialize,它只构造实例)。
// 没有 default_env.hdr 时禁用环境:只保留黑色兜底立方体贴图 + BRDF LUT。
EnvironmentSystem::EnvironmentSystem() {
    auto interface = global_context->render_system->getInterface();

    // BRDF LUT 与黑色兜底立方体贴图始终创建(与是否有环境无关)。
    brdf_lut_ = interface->createStorageImage(
        kBrdfLutSize, kBrdfLutSize, vk::Format::eR16G16Sfloat, vk::ImageUsageFlagBits::eSampled);
    fallback_cubemap_ = interface->createCubeTexture(
        1, 1, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage);

    Renderer::SamplerOptions linear_clamp{
        .mag_filter = vk::Filter::eLinear,
        .min_filter = vk::Filter::eLinear,
        .address_mode_u = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_v = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_w = vk::SamplerAddressMode::eClampToEdge,
        .mipmap_mode = vk::SamplerMipmapMode::eLinear,
        .mip_levels = 1,
    };
    brdf_sampler_ = interface->createSampler(linear_clamp);
    fallback_sampler_ = interface->createSampler(linear_clamp);

    buildClearPipeline();
    buildBrdfPipeline();
    clearCube(fallback_cubemap_);
    generateBrdfLut();

    // 没有 default_env.hdr -> 不加载环境(hasEnvironment()==false)。
    auto hdr_path = std::filesystem::path(global_context->asset_system->getRootDir()) / "textures" / "default_env.hdr";
    if (!std::filesystem::exists(hdr_path)) {
        WEN_CORE_INFO("EnvironmentSystem: default_env.hdr not found, skybox disabled")
        return;
    }

    // 有 HDR:创建环境资源并预积分。
    env_cubemap_ = interface->createCubeTexture(
        kEnvSize, 1, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage);
    irradiance_cubemap_ = interface->createCubeTexture(
        kIrradianceSize, 1, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage);
    prefiltered_cubemap_ = interface->createCubeTexture(
        kPrefilterSize, kPrefilterMipLevels, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage);

    env_sampler_ = interface->createSampler(linear_clamp);
    irradiance_sampler_ = interface->createSampler(linear_clamp);
    prefiltered_sampler_ = interface->createSampler(Renderer::SamplerOptions{
        .mag_filter = vk::Filter::eLinear,
        .min_filter = vk::Filter::eLinear,
        .address_mode_u = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_v = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_w = vk::SamplerAddressMode::eClampToEdge,
        .mipmap_mode = vk::SamplerMipmapMode::eLinear,
        .mip_levels = kPrefilterMipLevels,
    });

    buildEnvPipelines();
    uploadEquirect("default_env.hdr");  // 已确认存在,内部完成 GPU 上传 + descriptor 绑定
    generateEnvCube();
    generateIrradiance();
    generatePrefiltered();
    WEN_CORE_INFO("EnvironmentSystem: initialized from default_env.hdr")
}

void EnvironmentSystem::buildClearPipeline() {
    auto interface = global_context->render_system->getInterface();
    clear_cubemap_descriptor_set_ = interface->createDescriptorSet();
    clear_cubemap_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
    });
    clear_cubemap_descriptor_set_->build();
    clear_cubemap_program_ = interface->createComputeShaderProgram();
    clear_cubemap_program_->setComputeShader(
        interface->loadShader("environment/clear_cubemap.comp", Renderer::ShaderStage::eCompute));
    clear_cubemap_pipeline_ = interface->createComputeRenderPipeline(clear_cubemap_program_);
    clear_cubemap_pipeline_->setDescriptorSet(clear_cubemap_descriptor_set_, 0);
    clear_cubemap_pipeline_->compile();
}

void EnvironmentSystem::buildBrdfPipeline() {
    auto interface = global_context->render_system->getInterface();
    brdf_descriptor_set_ = interface->createDescriptorSet();
    brdf_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
    });
    brdf_descriptor_set_->build();
    brdf_descriptor_set_->bindStorageImage(0, brdf_lut_);
    brdf_program_ = interface->createComputeShaderProgram();
    brdf_program_->setComputeShader(
        interface->loadShader("environment/brdf_lut.comp", Renderer::ShaderStage::eCompute));
    brdf_pipeline_ = interface->createComputeRenderPipeline(brdf_program_);
    brdf_pipeline_->setDescriptorSet(brdf_descriptor_set_, 0);
    brdf_pipeline_->compile();
}

void EnvironmentSystem::buildEnvPipelines() {
    auto interface = global_context->render_system->getInterface();

    // 等距柱状 HDR -> 环境立方体
    equirect_descriptor_set_ = interface->createDescriptorSet();
    equirect_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
    });
    equirect_descriptor_set_->build();
    equirect_program_ = interface->createComputeShaderProgram();
    equirect_program_->setComputeShader(
        interface->loadShader("environment/equirect_to_cubemap.comp", Renderer::ShaderStage::eCompute));
    equirect_pipeline_ = interface->createComputeRenderPipeline(equirect_program_);
    equirect_pipeline_->setDescriptorSet(equirect_descriptor_set_, 0);
    equirect_pipeline_->compile();

    // 环境 -> 辐照度(漫反射预积分)
    irradiance_descriptor_set_ = interface->createDescriptorSet();
    irradiance_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
    });
    irradiance_descriptor_set_->build();
    irradiance_descriptor_set_->bindTexture(0, env_cubemap_, env_sampler_);
    irradiance_descriptor_set_->bindStorageImageView(1, irradiance_cubemap_->getArrayView(0), vk::ImageLayout::eGeneral);
    irradiance_constants_ = interface->createPushConstants(Renderer::ShaderStage::eCompute, {
        {"sample_count", Renderer::ConstantType::eUint32},
        {"delta_phi", Renderer::ConstantType::eFloat},
        {"delta_theta", Renderer::ConstantType::eFloat},
    });
    irradiance_program_ = interface->createComputeShaderProgram();
    irradiance_program_->setComputeShader(
        interface->loadShader("environment/irradiance.comp", Renderer::ShaderStage::eCompute));
    irradiance_pipeline_ = interface->createComputeRenderPipeline(irradiance_program_);
    irradiance_pipeline_->setDescriptorSet(irradiance_descriptor_set_, 0);
    irradiance_pipeline_->setPushConstants(irradiance_constants_);
    irradiance_pipeline_->compile();

    // 环境 -> 预过滤镜面立方体(GGX 重要性采样,每个 mip 一个 roughness)
    prefilter_descriptor_set_ = interface->createDescriptorSet();
    prefilter_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eCompute},
        {1, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eCompute},
    });
    prefilter_descriptor_set_->build();
    prefilter_descriptor_set_->bindTexture(0, env_cubemap_, env_sampler_);
    prefilter_descriptor_set_->bindStorageImageView(1, prefiltered_cubemap_->getArrayView(0), vk::ImageLayout::eGeneral);
    prefilter_constants_ = interface->createPushConstants(Renderer::ShaderStage::eCompute, {
        {"sample_count", Renderer::ConstantType::eUint32},
        {"roughness", Renderer::ConstantType::eFloat},
    });
    prefilter_program_ = interface->createComputeShaderProgram();
    prefilter_program_->setComputeShader(
        interface->loadShader("environment/prefilter.comp", Renderer::ShaderStage::eCompute));
    prefilter_pipeline_ = interface->createComputeRenderPipeline(prefilter_program_);
    prefilter_pipeline_->setDescriptorSet(prefilter_descriptor_set_, 0);
    prefilter_pipeline_->setPushConstants(prefilter_constants_);
    prefilter_pipeline_->compile();
}

// 把立方体贴图清除为纯黑(兜底用)。
void EnvironmentSystem::clearCube(std::shared_ptr<Renderer::CubeTexture> cube) {
    transitionImage(cube->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
                    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader,
                    1, 6);
    clear_cubemap_descriptor_set_->bindStorageImageView(0, cube->getArrayView(0), vk::ImageLayout::eGeneral);
    runCompute(clear_cubemap_pipeline_, nullptr, 1, 1, 6);
    transitionImage(cube->getImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader,
                    1, 6);
}

void EnvironmentSystem::generateEnvCube() {
    // 计算写入前,环境立方体须处于 eGeneral
    transitionImage(env_cubemap_->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
                    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader,
                    1, 6);

    uint32_t groups = (kEnvSize + 31) / 32;
    runCompute(equirect_pipeline_, nullptr, groups, groups, 6);

    // 采样读取前,转回 shader-read-only
    transitionImage(env_cubemap_->getImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader,
                    1, 6);
}

void EnvironmentSystem::generateIrradiance() {
    transitionImage(irradiance_cubemap_->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
                    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader,
                    1, 6);

    uint32_t sample_count = 64;
    float delta_phi = 6.28318530718f / static_cast<float>(sample_count);
    float delta_theta = 1.57079632679f / static_cast<float>(sample_count);
    irradiance_constants_->pushConstant("sample_count", &sample_count);
    irradiance_constants_->pushConstant("delta_phi", &delta_phi);
    irradiance_constants_->pushConstant("delta_theta", &delta_theta);
    uint32_t groups = (kIrradianceSize + 31) / 32;
    runCompute(irradiance_pipeline_, irradiance_constants_, groups, groups, 6);

    transitionImage(irradiance_cubemap_->getImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader,
                    1, 6);
}

void EnvironmentSystem::generatePrefiltered() {
    transitionImage(prefiltered_cubemap_->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
                    vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader,
                    kPrefilterMipLevels, 6);

    for (uint32_t mip = 0; mip < kPrefilterMipLevels; mip++) {
        prefilter_descriptor_set_->bindStorageImageView(
            1, prefiltered_cubemap_->getArrayView(mip), vk::ImageLayout::eGeneral);
        float roughness = static_cast<float>(mip) / static_cast<float>(kPrefilterMipLevels - 1);
        uint32_t sample_count = 256;
        prefilter_constants_->pushConstant("sample_count", &sample_count);
        prefilter_constants_->pushConstant("roughness", &roughness);
        uint32_t size = kPrefilterSize >> mip;
        uint32_t groups = (size + 31) / 32;
        runCompute(prefilter_pipeline_, prefilter_constants_, groups, groups, 6);
    }

    transitionImage(prefiltered_cubemap_->getImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eFragmentShader,
                    kPrefilterMipLevels, 6);
}

void EnvironmentSystem::generateBrdfLut() {
    // brdf_lut 在 StorageImage 构造时已转 eGeneral,采样可从 General 布局读取。
    runCompute(brdf_pipeline_, nullptr, kBrdfLutSize / 16, kBrdfLutSize / 16, 1);
}

bool EnvironmentSystem::uploadEquirect(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    auto full_path = std::filesystem::path(global_context->asset_system->getRootDir()) / "textures" / filename;
    if (!std::filesystem::exists(full_path)) {
        return false;
    }

    // 等距柱状图:stb 加载后 v=0 是图像顶部(=+Y 北极),与 shader 映射一致。
    stbi_set_flip_vertically_on_load(false);
    int w = 0, h = 0, channels = 0;
    float* pixels = stbi_loadf(full_path.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(true);
    if (!pixels || w <= 0 || h <= 0) {
        WEN_CORE_WARN("EnvironmentSystem: failed to load HDR equirect: {}", full_path.string())
        if (pixels) {
            stbi_image_free(pixels);
        }
        return false;
    }

    auto interface = global_context->render_system->getInterface();
    equirect_tex_ = interface->createStorageImage(
        w, h, vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

    auto manager = global_context->render_system->getAPIManager();
    uint32_t bytes = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * 4 * sizeof(float);
    Renderer::Buffer staging(bytes, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU,
                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
    memcpy(staging.map(), pixels, bytes);
    staging.unmap();
    stbi_image_free(pixels);

    transitionImage(equirect_tex_->getImage(), vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal,
                    vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
                    vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer,
                    1, 1);

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferImageCopy region;
    region.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setImageOffset({0, 0, 0})
        .setImageExtent({static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1});
    cmdbuf.copyBufferToImage(staging.buffer, equirect_tex_->getImage(),
                             vk::ImageLayout::eTransferDstOptimal, region);
    manager->command_pool->freeSingleUse(cmdbuf);

    transitionImage(equirect_tex_->getImage(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral,
                    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader,
                    1, 1);

    // 绑定等距柱状采样 + 环境立方体写入目标
    equirect_descriptor_set_->bindTexture(0, equirect_tex_, env_sampler_);
    equirect_descriptor_set_->bindStorageImageView(1, env_cubemap_->getArrayView(0), vk::ImageLayout::eGeneral);
    return true;
}

}  // namespace wen
