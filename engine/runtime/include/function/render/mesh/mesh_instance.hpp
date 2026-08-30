#pragma once

#include "function/asset/mesh/mesh.hpp"

namespace wen {

// 网格实例:变换 + 网格 + 材质(基础色/纹理索引 + 金属度/粗糙度 + 自发光 + UV 平铺 + 法线/MR/AO 贴图)
// 布局与 GLSL types.glsl 的 MeshInstance 对齐(std430,共 9 个 vec4 的内存)。
struct MeshInstance {
    alignas(16) glm::vec3 location;
    alignas(16) glm::vec3 rotation;
    alignas(16) glm::vec3 scale;
    alignas(4) MeshID mesh_id;
    alignas(16) glm::vec3 base_color = glm::vec3(1.0f);
    alignas(4) uint32_t texture_index = 0;  // 0 = 默认白纹理
    alignas(4) float metallic = 0.0f;
    alignas(4) float roughness = 0.5f;
    alignas(16) glm::vec3 emissive_color = glm::vec3(0.0f);
    alignas(4) float emissive_intensity = 0.0f;
    alignas(8) glm::vec2 tiling = glm::vec2(1.0f);  // UV 平铺
    alignas(4) uint32_t normal_texture_index = 0;   // 0 = 无法线贴图
    alignas(4) float normal_scale = 1.0f;           // 法线贴图强度
    alignas(4) uint32_t mr_texture_index = 0;       // metallic-roughness 贴图(B=金, G=粗糙)
    alignas(4) uint32_t ao_texture_index = 0;       // 环境光遮蔽贴图
    alignas(4) float ao_intensity = 1.0f;           // AO 强度(0=关闭)
};

}  // namespace wen