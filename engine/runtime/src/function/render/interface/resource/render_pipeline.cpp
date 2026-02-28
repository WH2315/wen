#include "function/render/interface/renderer.hpp"
#include "function/render/interface/context.hpp"
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>

namespace wen::Renderer {

Shader::Shader(const std::string& filename, ShaderStage stage) {
    auto code = readFile(filename);
    this->stage = stage;

    EShLanguage shader_stage;
    switch (stage) {
        case ShaderStage::eVertex:
            shader_stage = EShLangVertex;
            break;
        case ShaderStage::eFragment:
            shader_stage = EShLangFragment;
            break;
        case ShaderStage::eRaygen:
            shader_stage = EShLangRayGen;
            break;
        case ShaderStage::eMiss:
            shader_stage = EShLangMiss;
            break;
        case ShaderStage::eClosestHit:
            shader_stage = EShLangClosestHit;
            break;
        case ShaderStage::eIntersection:
            shader_stage = EShLangIntersect;
            break;
    }
    glslang::TShader shader(shader_stage);
    auto data = code.data();
    int version = 450;
    if (renderer_config.is_enable_ray_tracing) {
        version = 460;
    }
    shader.setStrings(&data, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, shader_stage, glslang::EShClientVulkan, version);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    ShaderIncluder includer(filename);
    if (!shader.parse(GetDefaultResources(), version, ENoProfile, false, false, EShMessages::EShMsgDefault, includer)) {
        WEN_CORE_ERROR("Shader parse failed (file: {}): {}, {}", filename, shader.getInfoLog(), shader.getInfoDebugLog())
        return;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMessages::EShMsgDefault)) {
        WEN_CORE_ERROR("Shader link failed (file: {}): {}, {}", filename, shader.getInfoLog(), shader.getInfoDebugLog())
        return;
    }
    const auto intermediate = program.getIntermediate(shader_stage);
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*intermediate, spirv);

    vk::ShaderModuleCreateInfo shader_module_ci;
    shader_module_ci.setCodeSize(spirv.size() * sizeof(uint32_t))
        .setPCode(spirv.data());
    module = manager->device->device.createShaderModule(shader_module_ci);
}

Shader::~Shader() {
    if (module.has_value()) {
        manager->device->device.destroyShaderModule(module.value());
        module.reset();
    }
}

GraphicsShaderProgram& GraphicsShaderProgram::attach(const std::shared_ptr<Shader>& shader) {
    if (shader->stage == ShaderStage::eVertex) {
        vert_shader_ = shader;
    } else if (shader->stage == ShaderStage::eFragment) {
        frag_shader_ = shader;
    }
    return *this;
}

GraphicsShaderProgram::~GraphicsShaderProgram() {
    if (vert_shader_) {
        vert_shader_.reset();
    }
    if (frag_shader_) {
        frag_shader_.reset();
    }
}

void RayTracingShaderProgram::setRaygenShader(const std::shared_ptr<Shader>& shader) {
    raygen_shader_ = shader;
}

void RayTracingShaderProgram::setMissShader(const std::shared_ptr<Shader>& shader) {
    miss_shaders_.emplace_back(shader);
}

void RayTracingShaderProgram::setHitGroup(const HitGroup& hit_group) {
    hit_groups_.emplace_back(hit_group);
    hit_shader_count_++;
    if (hit_group.intersection_shader.has_value()) {
        hit_shader_count_++;
    }
}

RayTracingShaderProgram::~RayTracingShaderProgram() {
    if (raygen_shader_) {
        raygen_shader_.reset();
    }
    for (auto& shader : miss_shaders_) {
        shader.reset();
    }
    miss_shaders_.clear();
    for (auto& hit_group : hit_groups_) {
        hit_group.closest_hit_shader.reset();
        if (hit_group.intersection_shader.has_value()) {
            hit_group.intersection_shader->reset();
        }
    }
    hit_groups_.clear();
    hit_shader_count_ = 0;
}

}  // namespace wen::Renderer

namespace wen::Renderer {

vk::PipelineShaderStageCreateInfo RenderPipeline::createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module) {
    vk::PipelineShaderStageCreateInfo ci;
    ci.setPSpecializationInfo(nullptr)
        .setStage(stage)
        .setModule(module)
        .setPName("main");
    return ci;
}

void RenderPipeline::createPipelineLayout() {
    vk::PipelineLayoutCreateInfo ci;

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts = {};
    descriptor_set_layouts.reserve(descriptor_sets.size());
    for (const auto& descriptor_set : descriptor_sets) {
        if (descriptor_set.has_value()) {
            descriptor_set_layouts.push_back(descriptor_set.value()->descriptor_layout_);
        }
    }

    ci.setSetLayouts(descriptor_set_layouts);

    if (push_constants.has_value()) {
        ci.setPushConstantRanges(push_constants.value()->range);
    }
    
    pipeline_layout = manager->device->device.createPipelineLayout(ci);
}

RenderPipeline::~RenderPipeline() {
    manager->device->device.destroyPipelineLayout(pipeline_layout);
    manager->device->device.destroyPipeline(pipeline);
    descriptor_sets.clear();
    push_constants.reset();
}

GraphicsRenderPipeline::GraphicsRenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<GraphicsShaderProgram>& shader_program, const std::string& subpass_name)
    : renderer_(renderer), shader_program_(shader_program), subpass_name_(subpass_name) {}

GraphicsRenderPipeline::~GraphicsRenderPipeline() {
    shader_program_.reset();
    vertex_input_.reset();
}

void GraphicsRenderPipeline::setVertexInput(std::shared_ptr<VertexInput> vertex_input) {
    vertex_input_ = std::move(vertex_input);
}

void GraphicsRenderPipeline::compile(const GraphicsRenderPipelineOptions& options) {
    // 1. shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        createShaderStage(vk::ShaderStageFlagBits::eVertex, shader_program_->vert_shader_->module.value()),
        createShaderStage(vk::ShaderStageFlagBits::eFragment, shader_program_->frag_shader_->module.value())
    };

    // 2. vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input = {};
    if (vertex_input_.get() != nullptr) {
        vertex_input.setVertexAttributeDescriptions(vertex_input_->attribute_descriptions_)
            .setVertexBindingDescriptions(vertex_input_->binding_descriptions_);
    }
    
    // 3. input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.setPrimitiveRestartEnable(false)
        .setTopology(vk::PrimitiveTopology::eTriangleList);
    
    // 4. viewport
    vk::PipelineViewportStateCreateInfo viewport = {};
    auto width = renderer_config.swapchain_image_width, height = renderer_config.swapchain_image_height;
    auto w = static_cast<float>(width), h = static_cast<float>(height);
    vk::Viewport view(0.0f, 0.0f, w, h, 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, {width, height});
    viewport.setViewportCount(1)
        .setViewports(view)
        .setScissorCount(1)
        .setScissors(scissor);
    
    // 5. rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.setDepthClampEnable(true)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(options.polygon_mode)
        .setLineWidth(options.line_width)
        .setCullMode(options.cull_mode)
        .setFrontFace(options.front_face)
        .setDepthBiasEnable(false)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(false)
        .setDepthBiasSlopeFactor(0.0f);
    
    // 6. multisample
    vk::PipelineMultisampleStateCreateInfo multisample = {};
    multisample.setSampleShadingEnable(true)
        .setRasterizationSamples(renderer_config.msaa_samples)
        .setMinSampleShading(0.2f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    // 7. depth and stencil
    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.setDepthTestEnable(options.depth_test_enable)
        .setDepthWriteEnable(options.depth_write_enable)
        .setDepthCompareOp(options.depth_compare_op)
        .setDepthBoundsTestEnable(false)
        .setStencilTestEnable(false)
        .setFront({})
        .setBack({});

    // 8. color blending
    vk::PipelineColorBlendStateCreateInfo color_blend = {};
    auto locked_renderer = renderer_.lock();
    uint32_t subpass_index = locked_renderer->render_pass->getSubpassIndex(subpass_name_);
    auto subpass = *locked_renderer->render_pass->subpasses[subpass_index];
    color_blend.setLogicOpEnable(false)
        .setAttachments(subpass.color_blend_attachments)
        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});
    
    // 9. dynamic state
    vk::PipelineDynamicStateCreateInfo dynamic = {};
    dynamic.setDynamicStateCount(options.dynamic_states.size())
        .setDynamicStates(options.dynamic_states);
    
    // create pipeline layout
    createPipelineLayout();

    // pipeline
    vk::GraphicsPipelineCreateInfo create_info;
    create_info.setStageCount(shader_stages.size())
        .setPStages(shader_stages.data())
        .setPVertexInputState(&vertex_input)
        .setPInputAssemblyState(&input_assembly)
        .setPTessellationState(nullptr)
        .setPViewportState(&viewport)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisample)
        .setPDepthStencilState(&depth_stencil)
        .setPColorBlendState(&color_blend)
        .setPDynamicState(&dynamic)
        .setLayout(pipeline_layout)
        .setRenderPass(locked_renderer->render_pass->render_pass)
        .setSubpass(subpass_index)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);
    
    locked_renderer.reset();

    pipeline = manager->device->device.createGraphicsPipeline(nullptr, create_info).value;
}

RayTracingRenderPipeline::RayTracingRenderPipeline(const std::shared_ptr<RayTracingShaderProgram>& shader_program)
    : shader_program_(shader_program) {}

RayTracingRenderPipeline::~RayTracingRenderPipeline() {
    shader_program_.reset();
    buffer_.reset();
}

void RayTracingRenderPipeline::compile(const RayTracingRenderPipelineOptions& options) {
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    shader_groups.reserve(1 + shader_program_->miss_shaders_.size() + shader_program_->hit_groups_.size());
    shader_stages.reserve(1 + shader_program_->miss_shaders_.size() + shader_program_->hit_shader_count_);

    shader_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(shader_stages.size());
    shader_stages.push_back(
        createShaderStage(
            vk::ShaderStageFlagBits::eRaygenKHR,
            shader_program_->raygen_shader_->module.value()
        )
    );
    for (const auto& miss_shader : shader_program_->miss_shaders_) {
        shader_groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(shader_stages.size());
        shader_stages.push_back(
            createShaderStage(
                vk::ShaderStageFlagBits::eMissKHR,
                miss_shader->module.value()
            )
        );
    }
    for (const auto& hit_group : shader_program_->hit_groups_) {
        auto& shader_group = shader_groups.emplace_back();
        shader_group.setClosestHitShader(shader_stages.size());
        shader_stages.push_back(
            createShaderStage(
                vk::ShaderStageFlagBits::eClosestHitKHR,
                hit_group.closest_hit_shader->module.value()
            )
        );
        if (hit_group.intersection_shader.has_value()) {
            shader_group.setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup)
                .setIntersectionShader(shader_stages.size());
            shader_stages.push_back(
                createShaderStage(
                    vk::ShaderStageFlagBits::eIntersectionKHR,
                    hit_group.intersection_shader.value()->module.value()
                )
            );
        } else {
            shader_group.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
        }
    }

    createPipelineLayout();

    vk::RayTracingPipelineCreateInfoKHR rt_pipeline_ci{};
    rt_pipeline_ci.setStages(shader_stages)
        .setGroups(shader_groups)
        .setMaxPipelineRayRecursionDepth(options.max_ray_recursion_depth)
        .setLayout(pipeline_layout);
    pipeline = manager->device->device.createRayTracingPipelineKHR({}, {}, rt_pipeline_ci, nullptr, manager->dispatcher).value;

    vk::PhysicalDeviceProperties2 properties = {};
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rt_pipeline_properties = {};
    properties.pNext = &rt_pipeline_properties;
    manager->device->physical_device.getProperties2(&properties);

    auto align_address = [](uint32_t size, uint32_t align) {
        return (size + (align - 1)) & ~(align - 1);
    };
    uint32_t handle_size = rt_pipeline_properties.shaderGroupHandleSize;
    // 着色器绑定表 (缓存) 需要开头的组已经完成对齐并且组中的句柄也已经对齐完成
    uint32_t handle_size_aligned = align_address(handle_size, rt_pipeline_properties.shaderGroupHandleAlignment);
    uint32_t base_alignment = rt_pipeline_properties.shaderGroupBaseAlignment;
    raygen_region_.stride = align_address(handle_size, base_alignment);
    raygen_region_.size = raygen_region_.stride;
    miss_region_.stride = handle_size_aligned;
    miss_region_.size = align_address(shader_program_->miss_shaders_.size() * handle_size_aligned, base_alignment);
    hit_region_.stride = handle_size_aligned;
    hit_region_.size = align_address(shader_program_->hit_groups_.size() * handle_size_aligned, base_alignment);

    std::vector<uint8_t> handles(handle_size * shader_groups.size());
    auto res = manager->device->device.getRayTracingShaderGroupHandlesKHR(pipeline, 0, shader_groups.size(), handles.size(), handles.data(), manager->dispatcher);
    assert(res == vk::Result::eSuccess);
    // 分配用于存储着色器绑定表的缓存
    buffer_ = std::make_unique<Buffer>(
        raygen_region_.size + miss_region_.size + hit_region_.size,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    // 获取每组的着色器绑定表
    auto address = getBufferAddress(buffer_->buffer);
    raygen_region_.setDeviceAddress(address);
    miss_region_.setDeviceAddress(address + raygen_region_.size);
    hit_region_.setDeviceAddress(address + raygen_region_.size + miss_region_.size);

    auto* ptr = static_cast<uint8_t*>(buffer_->map());
    auto get_handle = [&](int i) {
        return handles.data() + i * handle_size;
    };
    memcpy(ptr, get_handle(0), handle_size);
    ptr += raygen_region_.size;
    for (uint32_t i = 0; i < shader_program_->miss_shaders_.size(); i++) {
        memcpy(ptr + i * handle_size_aligned, get_handle(1 + i), handle_size);
    }
    ptr += miss_region_.size;
    for (uint32_t i = 0; i < shader_program_->hit_groups_.size(); i++) {
        memcpy(ptr + i * handle_size_aligned, get_handle(1 + shader_program_->miss_shaders_.size() + i), handle_size);
    }
    buffer_->unmap();
}

}  // namespace wen::Renderer