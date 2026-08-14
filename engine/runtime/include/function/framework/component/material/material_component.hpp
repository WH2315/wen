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

    void onCreate() override {
        applyToMeshInstance();
    }

    // 移除材质时把兄弟网格实例复位为默认材质(白、无纹理、metallic 0 / roughness 0.5),
    // 否则删除组件后场景里仍残留材质效果。
    void onDestroy() override {
        writeMeshInstance(glm::vec3(1.0f), 0.0f, 0.5f, 0);
    }

    // 把材质写入兄弟 MeshComponent 的网格实例。
    void applyToMeshInstance() {
        if (game_object_ == nullptr) {
            return;
        }
        uint32_t texture_index =
            texture_path.empty() ? 0 : global_context->asset_system->loadTexture(texture_path);
        writeMeshInstance(base_color, metallic, roughness, texture_index);
    }

    // 编辑器设置纹理路径后立即重载(与撤销无关的即时应用)。
    void setTexturePath(const std::string& path) {
        texture_path = path;
        applyToMeshInstance();
    }

private:
    void writeMeshInstance(const glm::vec3& color, float metal, float rough, uint32_t texture_index) {
        if (game_object_ == nullptr) {
            return;
        }
        auto* pool = global_context->render_system->getRenderData()->getMeshInstancePool();
        // 没有网格实例(未挂 MeshComponent)时静默跳过;getMeshInstancePtr 对缺失 uuid 会抛异常。
        if (pool->game_object_uuid_to_mesh_instance_index_map.find(game_object_->getUUID()) ==
            pool->game_object_uuid_to_mesh_instance_index_map.end()) {
            return;
        }
        auto* instance = pool->getMeshInstancePtr(game_object_->getUUID());
        instance->base_color = color;
        instance->metallic = metal;
        instance->roughness = rough;
        instance->texture_index = texture_index;
    }
};

}  // namespace wen
