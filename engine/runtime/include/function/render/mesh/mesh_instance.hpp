#pragma once

#include "function/asset/mesh/mesh.hpp"

namespace wen {

// 网格实例 52字节
struct MeshInstance {
    alignas(16) glm::vec3 location;
    alignas(16) glm::vec3 rotation;
    alignas(16) glm::vec3 scale;
    alignas(4) MeshID mesh_id;
};

}  // namespace wen