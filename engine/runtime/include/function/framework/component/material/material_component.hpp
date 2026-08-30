#pragma once

#include "function/framework/component.hpp"
#include "engine/global_context.hpp"
#include <glm/glm.hpp>

namespace wen {

// 材质:基础色/金属度/粗糙度 + 纹理路径。onCreate 与成员变化时把数据
// 写入兄弟 MeshComponent 的网格实例(基础色 + 纹理索引),供网格 pass 采样。
class MaterialComponent : public Component {
    REFLECT_CLASS("MaterialComponent")

public:
    std::string getClassName() const override { return "MaterialComponent"; }
    static std::string GetClassName() { return "MaterialComponent"; }

    MaterialComponent() : base_color(1.0f), metallic(0.0f), roughness(0.5f) {
        // 编辑器/撤销改成员后自动同步到网格实例。
        addMemberUpdateCallback([this](Component*) { applyToMeshInstance(); });
    }

    REFLECT_MEMBER()
    glm::vec3 base_color;

    REFLECT_MEMBER()
    float metallic;

    REFLECT_MEMBER()
    float roughness;

    // 纹理路径(相对 <资源根>/textures);空 = 无纹理。
    REFLECT_MEMBER()
    std::string texture_path;

    // 自发光(不参与光照,直接叠加到最终颜色)。
    REFLECT_MEMBER()
    glm::vec3 emissive_color{0.0f};

    REFLECT_MEMBER()
    float emissive_intensity = 0.0f;

    // UV 平铺:纹理坐标重复次数。
    REFLECT_MEMBER()
    glm::vec2 tiling{1.0f, 1.0f};

    // 法线贴图(相对 <资源根>/textures);空 = 无法线贴图。
    REFLECT_MEMBER()
    std::string normal_map_path;

    // 法线贴图强度。
    REFLECT_MEMBER()
    float normal_scale = 1.0f;

    // metallic-roughness 贴图(相对 <资源根>/textures;B=金属度, G=粗糙度)。
    REFLECT_MEMBER()
    std::string mr_map_path;

    // 环境光遮蔽贴图。
    REFLECT_MEMBER()
    std::string ao_map_path;

    // AO 强度(0=关闭)。
    REFLECT_MEMBER()
    float ao_intensity = 1.0f;

    void onCreate() override {
        applyToMeshInstance();
    }

    void onDestroy() override {
        writeMeshInstance(glm::vec3(1.0f), 0.0f, 0.5f, 0, glm::vec3(0.0f), 0.0f, glm::vec2(1.0f), 0, 1.0f, 0, 0, 1.0f);
    }

    void applyToMeshInstance() {
        if (game_object_ == nullptr) {
            return;
        }
        uint32_t texture_index =
            texture_path.empty() ? 0 : global_context->asset_system->loadTexture(texture_path);
        uint32_t normal_index =
            normal_map_path.empty() ? 0 : global_context->asset_system->loadNormalTexture(normal_map_path);
        uint32_t mr_index = mr_map_path.empty() ? 0 : global_context->asset_system->loadMrTexture(mr_map_path);
        uint32_t ao_index = ao_map_path.empty() ? 0 : global_context->asset_system->loadAoTexture(ao_map_path);
        writeMeshInstance(base_color, metallic, roughness, texture_index, emissive_color, emissive_intensity,
                          tiling, normal_index, normal_scale, mr_index, ao_index, ao_intensity);
    }

    // 编辑器设置纹理路径后立即重载(与撤销无关的即时应用)。
    void setTexturePath(const std::string& path) {
        texture_path = path;
        applyToMeshInstance();
    }

    void setNormalMapPath(const std::string& path) {
        normal_map_path = path;
        applyToMeshInstance();
    }

    void setMrMapPath(const std::string& path) {
        mr_map_path = path;
        applyToMeshInstance();
    }

    void setAoMapPath(const std::string& path) {
        ao_map_path = path;
        applyToMeshInstance();
    }

private:
    void writeMeshInstance(const glm::vec3& color, float metal, float rough, uint32_t texture_index,
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
};

}  // namespace wen
