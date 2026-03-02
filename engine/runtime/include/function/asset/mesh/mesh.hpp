#pragma once

#include "function/asset/mesh/primitive.hpp"

namespace wen {

using MeshID = uint32_t;

struct MeshData {
    std::vector<PrimitiveData> lods;
};

constexpr size_t max_level_of_details = 7;

struct MeshDescriptor {
    alignas(4) uint32_t lod_count;
    alignas(4) PrimitiveID lods[max_level_of_details];
    alignas(4) glm::vec3 aabb_min;
    alignas(4) float radius;
    alignas(4) glm::vec3 aabb_max;
    alignas(4) float pad;
};

}  // namespace wen