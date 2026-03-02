#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace wen {

using PrimitiveID = uint32_t;

struct PrimitiveData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> colors;
    std::vector<uint32_t> indices;
};

struct PrimitiveDescriptor {
    alignas(4) int32_t vertex_offset;
    alignas(4) uint32_t first_index;
    alignas(4) uint32_t index_count;
    alignas(4) uint32_t pad;
};

}  // namespace wen