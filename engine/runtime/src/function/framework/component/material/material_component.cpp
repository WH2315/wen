#include "function/framework/component/material/material_component.hpp"
#include "function/framework/component/mesh/mesh_component.hpp"
#include <filesystem>

namespace wen {

std::string MaterialComponent::materialFullPath() const {
    if (material_path.empty()) {
        return {};
    }
    return (std::filesystem::path(global_context->asset_system->getRootDir()) / "materials" / material_path)
        .string();
}

bool MaterialComponent::isCustomShaderMaterial() const {
    if (material_path.empty()) {
        return false;
    }
    CustomMaterialAsset asset;
    return loadMaterialAsset(materialFullPath(), asset) && !asset.isBuiltinPbr();
}

void MaterialComponent::onCreate() {
    applyToMeshInstance();
}

void MaterialComponent::onDestroy() {
    writeMeshInstance(glm::vec3(1.0f), 0.0f, 0.5f, 0, glm::vec3(0.0f), 0.0f, glm::vec2(1.0f), 0, 1.0f, 0, 0, 1.0f);
}

void MaterialComponent::applyToMeshInstance() {
    if (game_object_ == nullptr) {
        return;
    }
    bool custom = isCustomShaderMaterial();
    auto* pool = global_context->render_system->getRenderData()->getMeshInstancePool();
    bool has_instance =
        pool != nullptr && pool->game_object_uuid_to_mesh_instance_index_map.find(game_object_->getUUID()) !=
                               pool->game_object_uuid_to_mesh_instance_index_map.end();

    if (custom) {
        if (has_instance) {
            // 从标准材质切换为自定义:重建网格以脱离 deferred 通道,避免双份绘制。
            if (auto* mesh = game_object_->queryComponent<MeshComponent>()) {
                mesh->reloadFromPath();
            }
        }
        applied_custom_ = true;
        return;
    }
    if (applied_custom_ && !has_instance) {
        // 从自定义切回标准:重建网格以回到 deferred 通道(重建过程中会再次进入本函数完成写入)。
        if (auto* mesh = game_object_->queryComponent<MeshComponent>()) {
            mesh->reloadFromPath();
        }
        applied_custom_ = false;
        return;
    }
    applied_custom_ = false;

    // 解析最终参数:material_path 非空时以资产为底,覆写项取内置字段。
    glm::vec3 final_color = base_color;
    float final_metallic = metallic;
    float final_roughness = roughness;
    std::string final_texture = texture_path;
    glm::vec3 final_emissive = emissive_color;
    float final_emissive_intensity = emissive_intensity;
    glm::vec2 final_tiling = tiling;
    std::string final_normal = normal_map_path;
    float final_normal_scale = normal_scale;
    std::string final_mr = mr_map_path;
    std::string final_ao = ao_map_path;
    float final_ao_intensity = ao_intensity;

    if (!material_path.empty()) {
        CustomMaterialAsset asset;
        if (loadMaterialAsset(materialFullPath(), asset) && asset.isBuiltinPbr()) {
            final_color = override_base_color ? base_color : asset.base_color;
            final_metallic = override_metallic ? metallic : asset.metallic;
            final_roughness = override_roughness ? roughness : asset.roughness;
            final_texture = texture_path.empty() ? asset.texture_path : texture_path;
            final_emissive = override_emissive_color ? emissive_color : asset.emissive_color;
            final_emissive_intensity =
                override_emissive_intensity ? emissive_intensity : asset.emissive_intensity;
            final_tiling = override_tiling ? tiling : asset.tiling;
            final_normal = normal_map_path.empty() ? asset.normal_map_path : normal_map_path;
            final_normal_scale = override_normal_scale ? normal_scale : asset.normal_scale;
            final_mr = mr_map_path.empty() ? asset.mr_map_path : mr_map_path;
            final_ao = ao_map_path.empty() ? asset.ao_map_path : ao_map_path;
            final_ao_intensity = override_ao_intensity ? ao_intensity : asset.ao_intensity;
        }
    }

    uint32_t texture_index =
        final_texture.empty() ? 0 : global_context->asset_system->loadTexture(final_texture);
    uint32_t normal_index =
        final_normal.empty() ? 0 : global_context->asset_system->loadNormalTexture(final_normal);
    uint32_t mr_index = final_mr.empty() ? 0 : global_context->asset_system->loadMrTexture(final_mr);
    uint32_t ao_index = final_ao.empty() ? 0 : global_context->asset_system->loadAoTexture(final_ao);
    writeMeshInstance(final_color, final_metallic, final_roughness, texture_index, final_emissive,
                      final_emissive_intensity, final_tiling, normal_index, final_normal_scale, mr_index,
                      ao_index, final_ao_intensity);
}

void MaterialComponent::writeMeshInstance(const glm::vec3& color, float metal, float rough, uint32_t texture_index,
                                          const glm::vec3& emissive_color, float emissive_intensity,
                                          const glm::vec2& tiling, uint32_t normal_index, float normal_scale,
                                          uint32_t mr_index, uint32_t ao_index, float ao_intensity) {
    if (game_object_ == nullptr) {
        return;
    }
    auto* pool = global_context->render_system->getRenderData()->getMeshInstancePool();
    if (pool->game_object_uuid_to_mesh_instance_index_map.find(game_object_->getUUID()) ==
        pool->game_object_uuid_to_mesh_instance_index_map.end()) {
        return;
    }
    auto* instance = pool->getMeshInstancePtr(game_object_->getUUID());
    instance->base_color = color;
    instance->metallic = metal;
    instance->roughness = rough;
    instance->texture_index = texture_index;
    instance->emissive_color = emissive_color;
    instance->emissive_intensity = emissive_intensity;
    instance->tiling = tiling;
    instance->normal_texture_index = normal_index;
    instance->normal_scale = normal_scale;
    instance->mr_texture_index = mr_index;
    instance->ao_texture_index = ao_index;
    instance->ao_intensity = ao_intensity;
}

}  // namespace wen
