#pragma once

#include "function/asset/mesh/mesh.hpp"

namespace wen {

// 网格实例:变换 + 网格 + 材质(基础色/纹理索引 + 金属度/粗糙度)
// 布局与 GLSL types.glsl 的 MeshInstance 对齐(std430,共 6 个 vec4 的内存)。
struct MeshInstance {
    alignas(16) glm::vec3 location;
    alignas(16) glm::vec3 rotation;
    alignas(16) glm::vec3 scale;
    alignas(4) MeshID mesh_id;
    alignas(16) glm::vec3 base_color = glm::vec3(1.0f);
    alignas(4) uint32_t texture_index = 0;  // 0 = 默认白纹理
    alignas(4) float metallic = 0.0f;
    alignas(4) float roughness = 0.5f;
};

}  // namespace wen