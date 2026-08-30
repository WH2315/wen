// -----------------------------------------------------------------
// CustomMaterialPass
// 自定义着色器材质 forward 通道(路线 A 的最小 MVP)。
// 渲染挂有 CustomShaderComponent 的对象,按 .mat 资产逐材质构建 pipeline,
// 逐对象以世界矩阵绘制。MVP 不写深度,叠加在已着色场景之上。
// -----------------------------------------------------------------

#include "function/render/render_framework/pass/custom_material_pass.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include "function/asset/material_asset.hpp"
#include "function/asset/mesh_pool.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/game_object.hpp"
#include "function/framework/component/custom_shader/custom_shader_component.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace wen {

namespace {
constexpr uint32_t kParamSlots = 8;  // slot0-1 内置,slot2+ 通用参数(最多 6)

vk::CullModeFlagBits cullModeFromString(const std::string& cull) {
    if (cull == "front") {
        return vk::CullModeFlagBits::eFront;
    }
    if (cull == "back") {
        return vk::CullModeFlagBits::eBack;
    }
    return vk::CullModeFlagBits::eNone;
}
}

void CustomMaterialPass::fillParams(Renderer::StorageBuffer* buffer, const CustomMaterialAsset& asset) {
    auto* ptr = static_cast<glm::vec4*>(buffer->map());
    ptr[0] = glm::vec4(asset.base_color, asset.intensity);
    ptr[1] = glm::vec4(asset.mode, 0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < kParamSlots - 2; i++) {
        if (i < asset.params.size()) {
            ptr[2 + i] = asset.params[i].value;
        } else {
            ptr[2 + i] = glm::vec4(0.0f);
        }
    }
    buffer->unmap();
}

void CustomMaterialPass::setAttachment(Renderer::RenderSubpass& render_subpass) {
    render_subpass.setDepthAttachment("depth");
    render_subpass.setOutputAttachment(global_context->render_system->output_attachment_name);
}

void CustomMaterialPass::setSubpassDependency(Renderer::RenderPass& render_pass) {
    render_pass.addSubpassDependency(
        "mesh_pass",
        "custom_material_pass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite
        }
    );
    // 共享深度:visibility_pass 写出的深度可供本 pass 做互相遮挡(只读测试,不写深度)。
    render_pass.addSubpassDependency(
        "visibility_pass",
        "custom_material_pass",
        {
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests
        },
        {
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eDepthStencilAttachmentRead
        }
    );
}

void CustomMaterialPass::createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    auto interface = global_context->render_system->getInterface();
    // 顶点输入:位置/法线/UV/顶点色,来自 MeshPool 的四个缓冲(binding 0-3)。
    vertex_input_ = interface->createVertexInput({
        {0, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat3}},
        {1, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat3}},
        {2, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat2}},
        {3, Renderer::InputRate::eVertex, {Renderer::VertexType::eFloat3}}
    });
    // 逐对象世界矩阵:以 4 个 vec4 列推到 push constant(与 shader 的 mat4 对齐)。
    push_constants_ = interface->createPushConstants(Renderer::ShaderStage::eVertex, {
        {"model_col0", Renderer::ConstantType::eFloat4},
        {"model_col1", Renderer::ConstantType::eFloat4},
        {"model_col2", Renderer::ConstantType::eFloat4},
        {"model_col3", Renderer::ConstantType::eFloat4}
    });
}

void CustomMaterialPass::buildPipeline(std::shared_ptr<Renderer::Renderer> renderer, MaterialResource* res,
                                       const CustomMaterialAsset& asset) {
    auto interface = global_context->render_system->getInterface();
    auto program = interface->createGraphicsShaderProgram();
    program->attach(interface->loadShader("custom/custom.vert", Renderer::ShaderStage::eVertex));
    program->attach(interface->loadShader(asset.shader, Renderer::ShaderStage::eFragment));
    auto pipeline = interface->createGraphicsRenderPipeline(renderer, program, getName());
    pipeline->setVertexInput(vertex_input_);
    pipeline->setDescriptorSet(res->descriptor_set);
    pipeline->setPushConstants(push_constants_);
    pipeline->compile({
        // 渲染状态可由 .mat 指定:wireframe 线框、cull 背面剔除。
        .polygon_mode = asset.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill,
        .line_width = 1.0f,
        .cull_mode = cullModeFromString(asset.cull),
        // 与 visibility_pass 一致用 CounterClockwise 作为正面(网格绕序来自 assimp)。
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = true,
        .depth_write_enable = false,
        .depth_compare_op = vk::CompareOp::eLessOrEqual,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
    });
    res->pipeline = pipeline;
    res->shader = asset.shader;
    res->wireframe = asset.wireframe;
    res->cull = asset.cull;
}

std::shared_ptr<CustomMaterialPass::MaterialResource>
CustomMaterialPass::getOrCreateMaterial(std::shared_ptr<Renderer::Renderer> renderer, const std::string& path) {
    auto interface = global_context->render_system->getInterface();
    auto materials_dir = std::filesystem::path(global_context->asset_system->getRootDir()) / "materials" / path;
    const auto full_path = materials_dir.string();

    // 已有缓存:检测 .mat 是否被修改;改了则重刷参数,shader/wireframe 变了则热重建管线。
    if (auto it = materials_.find(path); it != materials_.end()) {
        auto mtime = std::filesystem::last_write_time(full_path);
        if (mtime != it->second->last_mtime) {
            it->second->last_mtime = mtime;
            CustomMaterialAsset asset;
            loadMaterialAsset(full_path, asset);
            fillParams(it->second->params_buffer.get(), asset);
            if (asset.shader != it->second->shader || asset.wireframe != it->second->wireframe ||
                asset.cull != it->second->cull) {
                // 重建前等 GPU 排空,避免销毁仍在管线内使用的 pipeline。
                renderer->waitIdle();
                buildPipeline(renderer, it->second.get(), asset);
                WEN_CORE_INFO("CustomMaterialPass: hot-reloaded material '{}' (shader '{}', wireframe {}, cull {})", path, asset.shader, asset.wireframe, asset.cull)
            }
        }
        return it->second;
    }

    auto res = std::make_shared<MaterialResource>();
    res->full_path = full_path;
    res->last_mtime = std::filesystem::last_write_time(full_path);

    CustomMaterialAsset asset;
    if (!loadMaterialAsset(full_path, asset)) {
        WEN_CORE_WARN("CustomMaterialPass: cannot load material asset \"{}\", using default.", full_path)
        asset = CustomMaterialAsset{};
    }

    // 参数 SSBO:slot0=(base_color.rgb, intensity), slot1=(mode,0,0,0), slot2+ = 通用参数。
    res->params_buffer = std::make_shared<Renderer::StorageBuffer>(
        sizeof(glm::vec4) * kParamSlots,
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    fillParams(res->params_buffer.get(), asset);

    auto ds = interface->createDescriptorSet();
    ds->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eVertex},
        {1, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eFragment}
    });
    ds->build();
    ds->bindUniform(0, global_context->camera_system->getViewportCamera());
    ds->bindStorageBuffer(1, res->params_buffer);
    res->descriptor_set = ds;

    buildPipeline(renderer, res.get(), asset);

    materials_[path] = res;
    WEN_CORE_INFO("CustomMaterialPass: built material '{}' with shader '{}'", path, asset.shader)
    return res;
}

void CustomMaterialPass::executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {
    if (!global_context->camera_system->hasActiveViewportCamera()) {
        return;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr) {
        return;
    }

    auto config = global_context->render_system->getRendererConfig();
    auto w = static_cast<float>(config.swapchain_image_width);
    auto h = static_cast<float>(config.swapchain_image_height);
    renderer->setViewport(0.0f, h, w, -h);
    renderer->setScissor(0, 0, config.swapchain_image_width, config.swapchain_image_height);

    auto& pool = *global_context->asset_system->getMeshPool();
    renderer->bindVertexBuffers({pool.position_buffer, pool.normal_buffer, pool.texcoord_buffer, pool.color_buffer}, 0);
    renderer->bindIndexBuffer(pool.index_buffer);

    // 描述符缓冲的指针为"移动指针"(upload 时前进),基址 = 当前指针 - 已用数量。
    auto* mesh_desc_base = pool.mesh_descriptor_buffer_ptr - pool.current_mesh_descriptor_count;
    auto* prim_desc_base = pool.primitive_descriptor_buffer_ptr - pool.current_primitive_descriptor_count;

    for (auto* go : scene->getGameObjects()) {
        auto* custom = go->queryComponent<CustomShaderComponent>();
        if (custom == nullptr || custom->material_path.empty()) {
            continue;
        }
        auto* mesh = go->queryComponent<MeshComponent>();
        if (mesh == nullptr || mesh->mesh_id == MeshID(-1)) {
            continue;
        }
        auto* transform = go->queryComponent<TransformComponent>();
        if (transform == nullptr) {
            continue;
        }
        auto mesh_id = mesh->mesh_id;
        if (mesh_id >= pool.current_mesh_descriptor_count) {
            continue;
        }
        auto& mesh_descriptor = mesh_desc_base[mesh_id];
        if (mesh_descriptor.lod_count == 0) {
            continue;
        }
        auto primitive_index = mesh_descriptor.lods[0];
        if (primitive_index >= pool.current_primitive_descriptor_count) {
            continue;
        }
        auto& prim = prim_desc_base[primitive_index];

        auto res = getOrCreateMaterial(renderer, custom->material_path);
        if (res->pipeline == nullptr) {
            continue;
        }

        glm::mat4 model = transform->getWorldMatrix();
        push_constants_->pushConstant("model_col0", glm::value_ptr(model[0]));
        push_constants_->pushConstant("model_col1", glm::value_ptr(model[1]));
        push_constants_->pushConstant("model_col2", glm::value_ptr(model[2]));
        push_constants_->pushConstant("model_col3", glm::value_ptr(model[3]));

        renderer->bindPipeline(res->pipeline);
        renderer->bindDescriptorSets(res->pipeline);
        renderer->pushConstants(res->pipeline);
        renderer->drawIndexed(prim.index_count, 1, prim.first_index, prim.vertex_offset, 0);
    }
}

}  // namespace wen